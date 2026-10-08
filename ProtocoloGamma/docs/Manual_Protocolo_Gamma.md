# MANUAL DEL PROTOCOLO GAMMA (Aristocrat)
## Comunicación directa ESP32 ↔ Máquina (sustituyendo la SPC)

**Versión 1.2 — 02/09/2026** (v1.2: tercera sesión en una **máquina distinta** — núcleo del bloque `02 03 03` estable entre máquinas, **zona drop variable** ([9] = drop en la máq. 2, en cero en la máq. 1) — ANEXO I.5. Mismo día: **fuente de drop adaptable IMPLEMENTADA en el firmware** (regla sticky individual-vs-bloque[9] con guard de orden) — ANEXO I.6. v1.1: mapa completo de las 24 entradas del bloque con los datos de las dos sesiones — ANEXO I)
**Fuente**: análisis forense de capturas reales con analizador lógico — `TX-27-08-202.txt` (5.634 bytes, SPC→máquina) y `RX-27-08-202.txt` (6.322 bytes, máquina→SPC), registrados entre t=9,9s y t=62,8s. Máquina: Aristocrat **Dolphin Treasure** (G1607, MkV-USA, base code A4.2, paytable 87,928%) con SPC-2.

Cada afirmación de este manual está verificada contra las 1.561 tramas válidas de la captura (los scripts `analisis_gamma*.py` de esta carpeta reproducen todo el análisis).

---

## 1. RESUMEN EJECUTIVO

| Característica | Valor |
|---|---|
| Interfaz física | RS-232 (TXD, RXD, GND) |
| Baudios / formato | **19.200, 8N1** (verificado: 528 µs/byte ≈ 520,8 µs teórico) |
| Modelo | **Maestro-esclavo**: el ESP32 (rol SPC) es SIEMPRE el maestro |
| Iniciativa de la máquina | **Nula** — jamás transmite sin poll previo (0/719 tramas RX) |
| CRC | **CRC-16/XMODEM** — poly 0x1021, init 0x0000, sin reflexión, MSB primero |
| Validez del CRC | **1.561/1.561 tramas (100%)**, cero excepciones |
| Secuencia | 8 slots rotativos: 40→48→50→58→60→68→70→78→40… |
| Respuesta | Siempre con el **mismo SEQ** recibido (718/718) |
| Contadores | Enteros de 32 bits **Big-Endian** en RX[9..12] |
| Lectura de 1 meter | ~60 ms (2 slots); ráfaga de 7 meters ≈ 420 ms |

---

## 2. CAPA FÍSICA

- RS-232 estándar. En el ESP32: transceptor **MAX3232** (RS-232 ±12 V → TTL 3,3 V) + GND común.
- ⚠ **POLARIDAD INVERTIDA en el punto de captura — CAUSA CONFIRMADA**: el analizador se pinchó **directo en la línea RS-232 entre la máquina y la SPC** (sin transceiver intermedio), por lo que hubo que activar *inverted* en el decodificador UART: en una línea RS-232 el reposo (marco) es tensión **negativa**, que el analizador lee como 0 ⇒ línea en reposo LOW, bit de arranque HIGH (inverso del UART TTL estándar). **Conclusión: el puerto de la máquina es RS-232 verdadera (±V)**, y **no afecta en nada a los bytes del protocolo** (el decodificador ya entregó los bytes correctos analizados). Implicación para el ESP32 — ver §10:
  - Con **MAX232/MAX3232** en la placa (caso de Globus IM): conectar el lado RS-232 del transceiver directo a los pines de la máquina. El transceptor resuelve niveles Y polaridad por hardware → entrega UART TTL estándar al ESP32 → **no se necesita ningún conversor TTL adicional ni inversión por software**.
- 19.200 baudios, 8 bits, sin paridad, 1 bit de parada. Un byte = 10 bits = 520,8 µs.
- Los bytes dentro de una trama van contiguos (gap = 1 tiempo de byte). **Entre tramas hay silencio ≥ 3,5 ms** → un parser puede delimitar tramas por gap > ~2 ms.
- Sin errores de paridad/framing durante la operación (solo basura `FF FF` / `DD 00` en el encendido, t≈9,9 s).
- Comportamiento en arranque: la máquina está muda mientras bota. En la captura, la SPC sondeó con el poll del slot 0x40 cada ~230 ms durante 28 s (t=11,8→39,9 s) **sin respuesta alguna**; al primer eco de la máquina (t=39,97) pasó a la rotación completa de 8 slots y a los comandos.

---

## 3. ESTRUCTURA DE LA TRAMA

```
┌─────┬─────┬─────────────────────┬──────────┐
│ SEQ │ LEN │  PAYLOAD (LEN B)    │  CRC(2)  │
└─────┴─────┴─────────────────────┴──────────┘
  1B    1B       LEN bytes         2B (MSB 1º)

Longitud total de la trama = LEN + 4   (cumplido en 1.561/1.566 tramas;
las 5 restantes son la basura del encendido)
```

- **SEQ** — byte de secuencia. Solo 8 valores válidos: `0x40 0x48 0x50 0x58 0x60 0x68 0x70 0x78`. El ESP32 debe rechazar cualquier otro (filtra la basura del arranque).
- **LEN** — nº de bytes de payload (sin contar SEQ, LEN ni CRC).
- **CRC** — CRC-16/XMODEM calculado sobre SEQ+LEN+PAYLOAD (los primeros LEN+2 bytes), transmitido **MSB primero**.

### 3.1 Payload de los comandos (visto en captura)

El payload se subdivide en: `[SUBLEN][TYPE][OP/CODE][BODY…]`

| MSG | Dirección | Trama completa | SUBLEN | TYPE | OP |
|---|---|---|---|---|---|
| Poll vacío | ESP32→MÁQ | `SEQ 00 CRC` | — | — | — |
| ACK vacío | MÁQ→ESP32 | `SEQ 00 CRC` (idéntico al poll) | — | — | — |
| **READ meter** | ESP32→MÁQ | `SEQ 06 05 30 02 CL SM MT CRC` | 05 | 30 | 02 = leer |
| **DATA meter 16B** | MÁQ→ESP32 | `SEQ 0C 0A 31 82 CL SM MT 00 V4 00 CRC` | 0A | 31 | 82 = ok |
| WRITE (sync SPC) | ESP32→MÁQ | `SEQ 08 06 30 04 CL SM MT D1 D0 CRC` | 06 | 30 | 04 = escribir |
| STATUS | ambos | `SEQ 04 02 33/34 M 00 CRC` | 02 | 33/34 | M = modo |
| Bloques de datos | MÁQ→ESP32 | `SEQ LEN SUBLEN 31 82 CL SM MT …datos… CRC` | var | 31 | 82 |

Donde `CL SM MT` = **Clase, Sub, Meter** — la dirección del contador (ver §7).

Notas:
- SUBLEN varía (LEN−1 en comandos de la SPC, LEN−2 en varias respuestas). **No es crítico**: para enmarcar se usa LEN y para validar el CRC. No construyas lógica sobre SUBLEN.
- En respuestas de datos, TYPE = `0x31` y el byte siguiente es el estado: `0x82` = lectura OK (el habitual), se observó también `0x84` en confirmaciones de escritura.

---

## 4. CRC-16/XMODEM — ALGORITMO EXACTO

```
width=16  poly=0x1021  init=0x0000  refin=false  refout=false  xorout=0x0000
```

```c
// ESP32 / C — verificado contra 1.561 tramas de la captura (100%)
uint16_t gamma_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0x0000;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++)
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021)
                                 : (uint16_t)(crc << 1);
    }
    return crc;
}

// Enviar:  crc = gamma_crc16(tx, LEN+2);
//          tx[LEN+2] = crc >> 8;      // MSB primero
//          tx[LEN+3] = crc & 0xFF;
// Verificar al recibir:  gamma_crc16(rx, LEN+4) == 0x0000
// (propiedad del CRC: procesar la trama completa incluido su CRC da residuo 0)
```

**Ejemplos verificados contra la captura:**

| Trama (sin CRC) | CRC calculado | CRC capturado |
|---|---|---|
| `78 06 05 30 02 02 04 05` | 0x5F75 | 5F 75 ✔ |
| `48 06 05 30 02 02 03 01` | 0xC0BA | C0 BA ✔ |
| `60 06 05 30 02 02 04 04` | 0x6C3A | 6C 3A ✔ |
| `78 06 05 30 02 04 09 01` | 0xDB0D | DB 0D ✔ |
| `40 00` (poll) | 0x0DCC | 0D CC ✔ |
| `40 0C 0A 31 82 02 04 05 00 02 21 1E FF 00` (respuesta) | 0xCBF3 | CB F3 ✔ |

---

## 5. SECUENCIA — LOS 8 SLOTS

```
0x40 → 0x48 → 0x50 → 0x58 → 0x60 → 0x68 → 0x70 → 0x78 → 0x40 → …
```

- El SEQ avanza **una posición por transacción** (una trama enviada por el maestro = una transacción, sea poll, query o write).
- La máquina responde **con el mismo SEQ** (718/718 verificado) → el ESP32 puede validar cada respuesta contra su slot.
- Cadencia observada: **una transacción cada ~30 ms** (periodo medio medido: 30,4 ms). Ciclo completo de rotación: 240 ms.

### 5.1 Polls vacíos precalculados (keep-alive)

Tramas de 4 bytes `SEQ 00 CRC`. En idle la SPC los envía cada 30 ms rotando slots; la máquina responde exactamente la misma trama:

| Slot | Trama (lista para enviar) |
|---|---|
| 0x40 | `40 00 0D CC` |
| 0x48 | `48 00 84 65` |
| 0x50 | `50 00 0E BF` |
| 0x58 | `58 00 87 16` |
| 0x60 | `60 00 0B 2A` |
| 0x68 | `68 00 82 83` |
| 0x70 | `70 00 08 59` |
| 0x78 | `78 00 81 F0` |

Estas 8 tramas aparecen decenas de veces en la captura con bytes idénticos — el ACK vacío de la máquina a cualquier comando de un slot es **byte-igual** al poll de ese slot.

---

## 6. MODELO TRANSACCIONAL — LO MÁS IMPORTANTE

**La respuesta de datos NUNCA llega en el slot de la consulta.** El patrón, verificado en las 24/24 respuestas de meter de la captura:

```
slot N    ESP32 → QUERY (10 B)        "lee el meter X"
slot N    MÁQ   → ACK vacío (4 B)     "recibido, preparando"  (~10 ms después)
slot N+1  ESP32 → POLL vacío (4 B)
slot N+1  MÁQ   → DATA (16 B)         el meter X, con el SEQ del poll
```

Ejemplo real (Credits Bet, t=52,19s):

```
52.191  ESP32→ 78 06 05 30 02 02 04 05 5F 75     QUERY
52.203  MÁQ → 78 00 81 F0                        ACK vacío
52.221  ESP32→ 40 00 0D CC                       POLL (slot siguiente)
52.224  MÁQ → 40 0C 0A 31 82 02 04 05 00 02 21 1E FF 00 CB F3   DATA = 35.725.055
```

Reglas operativas:

1. Tras cada QUERY, el ESP32 **debe seguir polleando** para recolectar la DATA. Normalmente llega en el primer poll siguiente (latencia 3,5–10 ms); para bloques grandes puede tardar 2 slots (se observó hasta 2 en el bloque de 220 B).
2. Si no llega en 2–3 polls, reintentar la QUERY.
3. La máquina **nunca** transmite por iniciativa propia (0/719 tramas RX sin poll en los 100 ms previos). Sin poll no hay comunicación.
4. Latencias medidas: QUERY→ACK ≈ 9–13 ms; POLL→DATA ≈ 3,5–9 ms.
5. Ritmo resultante: **~60 ms por meter** (2 slots). Ráfaga real de 7 meters en la captura: 52,19→52,59 s ≈ 400 ms.

### 6.1 Lectura encadenada de varios contadores

No se "repite el poll de un contador hasta responder": la QUERY se envía **una vez**, y el POLL que la sigue es **vacío** (no lleva contador). Cada trama enviada —sea QUERY o POLL— consume un slot y avanza la secuencia. Cadena real para leer 3 contadores:

```
slot 0x48:  ESP32→ QUERY Current Credits (10 B)   MÁQ→ ACK (4 B)
slot 0x50:  ESP32→ POLL vacío (4 B)               MÁQ→ DATA Current Credits (16 B) ✓
slot 0x58:  ESP32→ QUERY Games Played (10 B)      MÁQ→ ACK (4 B)
slot 0x60:  ESP32→ POLL vacío (4 B)               MÁQ→ DATA Games Played (16 B) ✓
slot 0x68:  ESP32→ QUERY Credits Bet (10 B)       MÁQ→ ACK (4 B)
slot 0x70:  ESP32→ POLL vacío (4 B)               MÁQ→ DATA Credits Bet (16 B) ✓
slot 0x78+: volver a polls vacíos de keep-alive (o seguir con el siguiente contador)
```

- No enviar una QUERY nueva antes de recolectar la DATA de la anterior (la captura nunca muestra encimado: la DATA siempre llegó tras un poll vacío).
- En reposo (sin contadores pendientes) se mantiene el ciclo de **polls vacíos rotando slots cada ~30 ms** — es el keep-alive que mantiene viva la comunicación.

### 6.3 Supervisión del enlace — hallazgo de la prueba en vivo (27-08-2026)

Verificación con PC + adaptador USB-RS232 + `gamma_test.py` contra la máquina real (CONFIRMA el protocolo: eco de polls, modelo QUERY→ACK→POLL→DATA, bloques de identidad byte-idénticos, denominación 100 = 0,01 $, meters embebidos en `03 01 00` idénticos a la captura; la máquina tolera cadencias lentas — respondió bien con latencias de 60–110 ms contra los 4–15 ms de la SPC). Además reveló un comportamiento invisible en la captura original:

1. **La máquina tiene timeout de enlace**: tras un periodo de silencio del maestro (observado: cae en algún punto entre ~1 s y 56 s; valor exacto por medir), **deja de responder a todo**.
2. **Readquisición SOLO con slot 0x40**: con el enlace caído, ignora QUERYs y polls de cualquier slot hasta ver un poll `40 00 0D CC` → responde el eco → a partir de ahí acepta la secuencia estricta (espera 0x48, 0x50…). El slot 0x40 actúa como "slot de re-enganche".
3. **Consecuencia para el ESP32 (CRÍTICO)**: el maestro **nunca debe dejar de pollear**. El diseño §10 (tarea de polls continuos) no es opcional, es requisito. Ante pérdida de enlace (máquina botando, desconexión), re-engachar con polls del slot 0x40 y luego continuar la rotación.
4. **Respuesta no-Gamma determinista (93 B) — encuadre propio descubierto (31/08)**: con el modo sniffer de `gamma_gui_v2.py` se capturó **completa** (15:57:54.878, marcada `RX**`) la respuesta que la máquina ya había emitido dos veces byte-idénticas el 27-08 (entonces se vio truncada, «~80 B»). Trama completa:

   ```
   26 59 41 31 0A 00 D0 EA 0A 00 D0 EA 0A 00 D0 EA 0A 00 D0 EA 0A 00 D0 EA
   0A 00 D0 EA 0A 00 D0 EA 95 F8 DB 92 05 99 46 25 C8 08 29 10 18 00 CE E9
   95 FA 7B 96 45 99 E6 2D C8 48 2D 14 18 02 CE F9 95 FA FB BF 65 BB EE 2F
   E8 08 BF 16 BA 22 CE F9 95 FE FB B7 65 FB EE 2F EA 68 AD 16 BE
   ```

   **Encuadre NO-Gamma con campo de longitud exacto**: `[26] [59] [payload 89 B] [2 B trailer]` — 0x59 = 89 coincide al byte con el payload entre cabecera y trailer (2+89+2 = 93). El byte 0 no es ningún slot válido y el CRC XMODEM no aplica. Estructura interna: `41 31 0A` ("A1" + LF) + patrón `0A 00 D0 EA` ×7 + bloque `95 F8 DB 92 05 99 46 25 C8 08 29 10 18 00` + tres segmentos delimitados por `0xCE` (15/15/14 B). Probado y descartado (`analisis_gamma6.py`): 12 variantes de CRC-16 sobre 3 rangos y en ambos órdenes de bytes, residuo-cero de la trama completa, Fletcher-16, sumas simples, desplazamientos de bit 1–7 (± inversión/reflexión de bits) buscando tramas Gamma embebidas, y decodificación de baud mismatch (9600 emitido / leído a 19200). **Hipótesis**: respuesta de una segunda capa de protocolo (modo diagnóstico/descarga de la máquina) que contesta cuando recibe bytes mal encuadrados o fuera de secuencia — el 27-08 apareció tras una QUERY fuera de secuencia con el enlace caído. Pendiente (§11.8): capturar con analizador para verificar baud/timing, anotar qué TX la precede y ensayar eco de su cabecera como handshake.

### 6.4 Cortes periódicos del enlace — verificación con el ESP32 integrado (02-09-2026)

Con el firmware corriendo el ciclo completo contra la máquina (instrumentación: timestamps de caída, duración del re-enganche y detector de huecos de TX en `Gamma_EnviarTrama`), se capturaron episodios a mitad de operación que **cierran la causa de las caídas del enlace**:

1. **La máquina sufre cortes periódicos de su propio enlace** (~1 a 1,5 s medidos): durante el corte queda **sorda a todo tráfico** — QUERYs, polls y **también polls del slot 0x40**. En su pantalla se ve `link currently Down`; pasa a `Up` al recibir el 0x40 de re-enganche del SPC. Intervalo entre cortes: variable (10 s a 4 min en banco, máquina en reposo).
2. **Al despertar re-adquiere solo con el 0x40** (confirma §6.3 punto 2): respondió el eco del primer 0x40 recibido en **~97 ms** — en un episodio ni siquiera hizo falta un poll de re-enganche dedicado: la máquina despertó y ecoó el 0x40 que venía en la rotación normal. En arranque en frío el puerto de la máquina tarda ~6 s desde power-on (re-enganche confirmado a los 597 ms de iniciada la espera con reintentos).
3. **Descartado el lado del ESP32 (instrumentado)**: cadencia de TX constante de 100 ms **sin un solo hueco > 250 ms** en toda la sesión, incluso bajo la carga del arranque (SD/WiFi/RFID). Ningún otro módulo escribe el UART2 en modo Gamma (la rama SAS de `Encuestas_Maquina` no corre; los transmisores `Transmite_DIR` no se invocan). Cero tramas desconocidas o corruptas recibidas durante los episodios.
4. **Recuperación del firmware (verificada)**: DATA ausente → 3 reintentos (~1,2 s) → salto del meter en curso → re-enganche con polls 0x40 **validado por eco** (`gamma_reenganche_ok`: corta la ventana de 1 s en cuanto llega, ~100–600 ms) → retomar el ciclo. Costo total por episodio: ~1,5–2 s y un meter sin refrescar esa vuelta (se refresca en la siguiente). El ciclo de contadores siempre completa.

### 6.2 ¿Cómo sé a qué contador pertenece una respuesta?

Dos mecanismos, uno contextual y uno explícito (usar ambos):

1. **Contextual (turno único)**: el protocolo nunca tiene más de una QUERY pendiente — no se envía la siguiente hasta recolectar la DATA de la anterior (§6.1). Por construcción, la próxima DATA que llegue pertenece a la última QUERY enviada. El poll "¿ya?" no lleva contador: **la máquina recuerda qué le pediste** y te lo entrega.
2. **Explícito (eco del objeto — VALIDAR SIEMPRE)**: toda respuesta de datos **repite el identificador CLASE/SUB/METER en rx[5..7]**. Ese eco dice inequívocamente de qué objeto es el valor recibido. Si no coincide con lo esperado, descartar la trama y reintentar.

Y una tercera validación barata: el SEQ de la respuesta (rx[0]) debe coincidir con el de la última trama enviada.

```c
// ¿Esta DATA es del objeto (cl,sm,mt) que estoy esperando?
bool Gamma_EsDataDe(const uint8_t *rx, uint8_t slot_enviado, uint8_t cl, uint8_t sm, uint8_t mt)
{
    return rx[0] == slot_enviado                        // eco del SEQ
        && rx[1] >= 0x0C                                // LEN: al menos 1 valor
        && rx[3] == 0x31                                // bloque de datos
        && rx[5] == cl && rx[6] == sm && rx[7] == mt;   // eco del objeto
}
```

---

## 7. CATÁLOGO DE CONTADORES (METERS)

Objeto = `CLASE SUB METER`. Consulta: `SEQ 06 05 30 02 CL SM MT CRC`.

### 7.1 Confirmados contra la pantalla de meters (valores únicos, sin ambigüedad)

| Contador (pantalla) | Objeto | Valor pantalla | Valor capturado | TX con SEQ 0x78 |

> 💡 **Tramas byte a byte (envío y recepción rotulados campo por campo) de cada contador: ver ANEXO E.**
|---|---|---|---|---|
| Current Credits | `02 03 01` | 1.194 | 1.194 | `78 06 05 30 02 02 03 01 86 66` |
| Total Games Played | `02 04 04` | 1.353.790 | 1.353.790 | `78 06 05 30 02 02 04 04 4F 54` |
| Total Credits Bet | `02 04 05` | 35.725.055 | 35.725.055 | `78 06 05 30 02 02 04 05 5F 75` |
| Total Credits Won | `02 04 06` | 31.052.141 | 31.052.141 | `78 06 05 30 02 02 04 06 6F 16` |
| True In (Coins Inserted) | `04 09 01` | 12.754.500 | 12.754.500 | `78 06 05 30 02 04 09 01 DB 0D` |

### 7.2 Grupo del 8.080.392 (ambigüedad de pantalla)

| Objeto | Valor capturado | Comentario |
|---|---|---|
| `02 05 03` | 8.080.392 | Coincide con **Cashout Handpays / Split Voucher Pays / Total Handpays** (en la pantalla esos tres valen lo mismo: 8.080.392). La foto muestra columnas por Meter Set; la correspondencia exacta objeto↔línda de pantalla queda para desambiguar (§11). |
| `02 05 04` | **0** | El análisis previo lo etiquetó "Total Handpay", pero devuelve 0. Es cualquier meter que hoy valga 0 (Jackpot Wins, Cashout Handpays del set 1…). **No asignar todavía.** |

### 7.3 Grupo del 12.754.500 (ambigüedad de pantalla)

True In = Coin Drop = Gross Drop = True Out = 12.754.500 en la pantalla. Objetos capturados con ese valor:

| Objeto | Valor | Nota |
|---|---|---|
| `04 09 01` | 12.754.500 | True In o uno de su grupo |
| `04 04 01` | 12.754.500 | Otro del grupo (Coin Drop / Gross Drop / True Out) |

**Comportamiento observado en vivo (15:52–15:58 del 27-08)**: al insertar $2 (200 créditos a denom. 0,01), `04 09 01` (True In) y `04 04 01` **subieron +200 inmediatamente al insertar**, mientras `02 04 05` (Credits Bet) no se movió hasta apostar y `02 03 01` (Current Credits) pasó de 0 a 200. Es decir: **True In cuenta dinero aceptado (en la inserción), Credits Bet cuenta lo apostado**. Ambos objetos del grupo subieron juntos — la diferencia semántica entre ellos no se separó aún.

Desambiguar en la próxima sesión (§11): insertar monedas y ver cuál se mueve.

### 7.4 Otros objetos leídos por la SPC (con respuesta real)

| Objeto | Valor | Interpretación probable |
|---|---|---|
| `04 01 01` | 100 | **Denominación SAS** (unidades de 0,0001): 100 = 0,01 $ — ✔ confirmado contra el menú de la máquina |
| `04 01 16` | 20.000 | Candidato a apuesta máxima (20.000 × 0,01 = 200 $) — sin confirmar |
| `04 01 03` | 0 | — |
| `02 05 07` | 119.999 | Config/meter (¿límite?) |
| `02 04 01` | 16.777.216 (0x01000000) | Flag/BCD empaquetado |
| `02 02 04` | 315.532.800 | Meter grande no identificado (ver §7.6) |
| `80 02 0A` | 1 | Estado host/SPC |
| `03 01 81` | 0 | — |
| `03 01 80` | 1.512 (0x05E8, trama LEN=08) | — |
| `03 01 0E` | 200 (0x00C8, LEN=0A) | — |
| `02 02 01` | 32.800 (0x8020, LEN=0A) | — |
| `02 02 0A` | 675 (0x02A3, LEN=0A) | — |
| `02 02 1C` / `1D` | 99 / 249 (LEN=08) | — |
| `04 06 01` | 0 | — |
| `04 0C 01` / `02` | 0 | — |

**Ancho de valor variable**: la respuesta estándar de meter es LEN=0x0C con valor de 4 B en RX[9..12]. Existen variantes más cortas (LEN=0x08/0x0A: valores de 0–2 B, también a partir de RX[9]) y más largas (LEN=0x0E/0x10: valores de 8/10 B, todos 0 en esta captura → meters de 64 bits). Para los meters principales de §7.1 la respuesta es siempre de 16 B.

### 7.5 Bloques de identidad (lectura opcional)

Misma consulta estándar de 10 B; respuestas de hasta 108 B con texto ASCII:

| Objeto | Contenido |
|---|---|
| `01 02 01` | `"01.04.003"` / `"000001"` (versión / serie) |
| `01 02 02` | `"Aristocrat Technologies, Inc."`, `"MkV-USA"`, `"G1607"`, `"AH"`, `"USA A4.2 Base Code"` |
| `03 01 00` | `"Dolphin Treasure"`, `"87.928%"`, Credits Bet, Credits Won, Games Played… (102 B) |
| `04 06 00` | Tabla de 220 B con valores 100/200/500/1000/2000/5000/10000… (tabla de créditos/denominación) |

#### Cruce con el menú de la máquina (pantalla de identificación)

Campos leídos del menú de la máquina vs. lo observado en la captura Gamma:

| Campo del menú | Valor en pantalla | En captura Gamma | Objeto Gamma |
|---|---|---|---|
| ID de juego | `AT` | `AH` — ⚠ difiere (ver §11) | `01 02 02`, `03 01 00` |
| ID adicional | `000` | `000001` — no coincide exacto | `01 02 01`? |
| **Denominación SAS** | **0,01** | **100 (unidades de 0,0001)** ✔ | `04 01 01` |
| Denominación usuario | 0 | no consultado | — |
| Apuesta máxima | 0 | candidato: 20.000 | `04 01 16`? |
| Grupo progresivo | 00 | no consultado | — |
| Opciones de juego | 0000 | no consultado | — |
| **Porcentaje Base** | **87,93** | **cadena ASCII `"87.928%"`** ✔ | `03 01 00` |
| Denominación Base | 0 | no consultado | — |
| **Tabla de pagos** | **G1607** | **cadena ASCII `"G1607"`** ✔ | `01 02 02`, `03 01 00` |
| (extra, no está en ese menú) | — | plataforma `"MkV-USA"`, base code `"USA A4.2 Base Code"`, juego `"Dolphin Treasure"` | `01 02 02`, `03 01 00` |

**Nota sobre `04 01 01` = 100 = denominación SAS 0,01**: en SAS la denominación se expresa en unidades de 0,0001 (100 → 0,01 $). Esta confirmación externa convierte al objeto `04 01 01` en la fuente autorizada de denominación para convertir créditos ↔ dinero en el ESP32.

La SPC además **se presentó** a la máquina al final (t=57,5 s) escribiendo su identidad en el objeto `01 04 08` ("SPC-2", "GamBS2M", "01.06.1"). **No es necesario**: los meters se leyeron desde t=41,7 s sin esa presentación.

### 7.6 Bloques multi-meter — varios contadores en UNA consulta

**Se piden exactamente igual que un meter individual**: la misma QUERY de 10 bytes, solo cambia el objeto consultado. La diferencia está en la **respuesta**, que es más larga y trae N valores empaquetados:

```
TX:  48 06 05 30 02 02 03 03 E0 F8          QUERY del bloque 02 03 03 (slot 0x48)
RX:  48 00 84 65                              ACK
TX:  50 00 0E BF                              POLL (slot 0x50)
RX:  50 68 66 31 82 02 03 03 00 [96 B de valores] 00 CRC CRC
     └SEQ └LEN=0x68 └TYPE=31 └ST=82 └eco objeto └pad └ 24 valores   └trailer └ CRC
     (trama de 108 bytes — los N valores son consecutivos de 4 B BE)
```

**La estructura es idéntica a la respuesta simple (§8) — solo cambia N**, el número de valores:

```
[0]=SEQ   [1]=LEN   [2]=SIZE   [3]=0x31   [4]=0x82   [5..7]=eco del objeto   [8]=0x00 (pad)
[9 .. 8+4N] = N valores uint32 Big-Endian consecutivos →  entry[i] está en pos 9+4i
[9+4N]=0x00 (trailer)   [10+4N .. 11+4N]=CRC

Meter simple (16 B):  LEN=0x0C → N=1  → 1 valor en pos 9–12
Bloque 02 03 03:      LEN=0x68 → N=24 → 24 valores, el último en pos 101–104
```

```c
// Parser genérico de DATA (sirve para N=1 y para bloques):
uint8_t  n_entries(const uint8_t *rx) { return (rx[1] - 8) / 4; }     // LEN=0x0C→1, 0x68→24
uint32_t entry(const uint8_t *rx, uint8_t i)
{
    return be32(&rx[9 + 4 * i]);                                      // valor i (0..n-1)
}
// Nota: los formatos cortos (LEN<0x0C, respuestas a escrituras) difieren — no aplicar esta fórmula.
```

#### Contenido del bloque `02 03 03` (24 entradas, verificado en captura)

| entry | Posición RX | Valor capturado | Identificado como |
|---|---|---|---|
| 0 | 9–12 | 315.532.800 | = `02 02 04` individual (verificado) |
| **1** | 13–16 | 1.194 | **Current Credits** |
| **2** | 17–20 | 1.353.790 | **Total Games Played** |
| 3 | 21–24 | 476.780 | sin asignar |
| **4** | 25–28 | 35.725.055 | **Total Credits Bet** |
| **5** | 29–32 | 31.052.141 | **Total Credits Won** |
| 8 | 41–44 | 12.828.900 | sin asignar |
| 13 | 61–64 | 8.080.392 | grupo Handpay (`02 05 03`) |
| 15 | 69–72 | 12.754.500 | grupo True In (`04 09 01`) |
| 6, 7, 9–12, 14, 16–23 | — | 0 | en cero |

> 💡 **Mapa completo de las 24 entradas** — valores de ambas sesiones (27-08 y 28-08), deltas y uso desde el firmware: **ANEXO I**.

**Cómo se mapeó**: los valores del bloque se cruzaron con los obtenidos por consulta individual (los 6 marcados coinciden exactamente). La posición↔contador es fija para este bloque en esta máquina/firmware — **verificado 02-09 en una segunda máquina: el núcleo ([0],[1],[2],[4],[5],[13]) es estable, la zona drop ([8]/[9]/[15]) NO** (ANEXO I.5).

**Recomendación**: para producción usar las consultas individuales (semántica inequívoca); el bloque sirve para polling rápido (1 transacción en vez de 7) una vez validado el layout. Otros bloques detectados: `04 03 01` (66 B, parcialmente identificado), `04 06 00` (220 B, tabla de créditos/denominación), `03 01 00` (102 B, nombre del juego + payback% + varios meters).

---

## 8. RESPUESTA DE METER — MAPA DE BYTES (16 B)

```
Pos:  0     1     2     3     4     5     6     7     8     9  10  11  12  13    14  15
     ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬───┴───┴───┴───┬─────┬────┴────┐
     │ SEQ │ 0C  │ 0A  │ 31  │ 82  │ CL  │ SM  │ MT  │ 00  │  VALOR (BE32) │ 00  │  CRC    │
     └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴───────────────┴─────┴─────────┘
```

| Pos | Campo | Valor |
|---|---|---|
| 0 | SEQ | igual al poll que la trajo |
| 1 | LEN | 0x0C (payload de 12 B) |
| 2 | SUBLEN | 0x0A (a veces 0x0B — no usar para lógica) |
| 3 | TYPE | 0x31 (objeto de datos) |
| 4 | STATUS | 0x82 = lectura OK |
| 5–7 | Objeto | eco de CL SM MT consultado — **validarlo** |
| 8 | padding | 0x00 |
| **9–12** | **VALOR** | **uint32 Big-Endian** |
| 13 | trailer | 0x00 (se vio 0x01 en un objeto — no crítico) |
| 14–15 | CRC | XMODEM MSB primero |

```c
// Extracción del valor (trama de 16 B ya validada por CRC)
uint32_t Gamma_MeterValue(const uint8_t *rx)
{
    return ((uint32_t)rx[9]  << 24) |
           ((uint32_t)rx[10] << 16) |
           ((uint32_t)rx[11] << 8)  |
           (uint32_t)rx[12];
}
```

---

## 9. OTROS MENSAJES OBSERVADOS (no requeridos para leer meters)

### 9.1 Intercambio de STATUS (8 B, al inicio de sesión)

```
ESP32→ 58 04 02 33 02 00 E3 2F      (0x33 = status SPC, modo 02)
MÁQ → 58 04 02 34 02 00 66 BF      (0x34 = status máquina)
```
Se observó con modo `01` y `02`. No es precondición para leer meters (había lectura de meters antes y después).

### 9.2 Escrituras (12 B) — el "sync" de la SPC

```
SEQ 08 06 30 04 CL SM MT D1 D0 CRC      (ej.: 40 08 06 30 04 02 05 03 01 00 16 A5)
```
La SPC escribió ~110 objetos (valor 1 + byte 00) a 30 ms por objeto tras conectar — espejo/sync de contadores. **El ESP32 que solo quiere LEER no necesita esto.**

### 9.3 Errores/limitaciones observadas

- A objetos raros, la máquina responde ACK vacío y la SPC simplemente sigue (sin NAK visible).
- No se observaron tramas NAK/RETRY en toda la captura (línea limpia).

---

## 10. DISEÑO RECOMENDADO PARA EL ESP32

*(Especificación — sin código de producción todavía.)*

**Hardware**: la placa Globus IM ya trae transceiver RS-232 (MAX232) — **conectar directo, sin adaptador TTL adicional**: lado RS-232 del MAX232 a los pines del conector de la SPC de la máquina, lado TTL al UART del ESP32 (el transceiver resuelve niveles y polaridad). Cableado cruzado y configuración:

```
Máquina (conector SPC)              Placa Globus IM
  TX de la máquina  ─────────────►  R1IN del MAX232  ──TTL──►  RX del ESP32 (UART2)
  RX de la máquina  ◄─────────────  T1OUT del MAX232 ◄─TTL───  TX del ESP32 (UART2)
  GND               ─────────────  GND común (imprescindible)
```
- ⚠ **UART a 19.200 8N1** — no 8E1 (SAS usa paridad par; Gamma NO, verificado por timing: 10 bits/byte).
- Verificar que el transceiver sea MAX3232 (3,3 V) o que exista adaptación de niveles: un MAX232 clásico a 5 V entrega R1OUT de 0–5 V y **los GPIO del ESP32 no toleran 5 V** (usar divisor o cambiar a MAX3232).
- Buffer RX ≥ 256 B (trama máxima vista: 220 B).
- Identificación de pines: la línea donde se vio el tráfico "RX de la captura" (máquina→SPC) es el **TX de la máquina**; la del tráfico "TX de la captura" es su RX.

**Tarea de protocolo (FreeRTOS)**:
- Estado REPOSO: cada 30 ms envía el poll del slot actual y avanza slot. Es el corazón del sistema: mantiene la línea viva y recolecta DATA pendiente.
- Estado LECTURA: intercala QUERY + polls como en §6.
- Parser de recepción por máquina de estados: SEQ válido (∈ los 8 slots) → LEN → LEN+2 bytes → validar CRC → validar SEQ esperado → despachar (ACK vacío / DATA 16 B / bloque). Delimitar tramas por silencio > 2 ms (los bytes intra-trama son contiguos).

**Builder de consulta**:

```c
size_t Gamma_BuildRead(uint8_t slot, uint8_t cl, uint8_t sm, uint8_t mt, uint8_t *tx)
{
    tx[0] = slot;  tx[1] = 0x06;  tx[2] = 0x05;  tx[3] = 0x30;
    tx[4] = 0x02;                    // operación: READ
    tx[5] = cl;    tx[6] = sm;   tx[7] = mt;
    uint16_t c = gamma_crc16(tx, 8);
    tx[8] = c >> 8;  tx[9] = c & 0xFF;
    return 10;
}
```

**Flujo `Gamma_ReadMeter(cl, sm, mt, &valor)`**:

```
1. Enviar QUERY en el slot actual; avanzar slot. Esperar respuesta ≤ 50 ms
   (debe llegar ACK vacío de 4 B con el mismo SEQ).
2. Hasta 3 veces:
     Enviar POLL del slot actual; avanzar slot. Esperar ≤ 50 ms.
     Si llega trama de 16 B con CRC OK, SEQ correcto, rx[3]==0x31,
     rx[5..7]==(cl,sm,mt)  →  valor = Gamma_MeterValue(rx);  return OK.
3. return ERR (reintentar la QUERY).
```

**Validaciones de respuesta**: CRC == 0 (o recompute == CRC recibido), SEQ == enviado, objeto eco == solicitado, LEN == 0x0C.

**Cron**: releer los meters principales cada 1–5 s (7 meters ≈ 420 ms por pasada). Exponer los valores a la lógica de fidelización existente (API Gmaster / SesionPlayerTracking) mapeando meter→variable global.

**Arranque**: sondear `40 00 0D CC` cada ~250 ms hasta recibir eco (máquina botada) y solo entonces entrar en rotación completa — imita el comportamiento observado de la SPC. (Alternativa válida: rotar siempre; la máquina muda simplemente no responde.)

**No implementar por ahora**: escrituras (§9.2), presentación de identidad (§7.5), bloques multi-meter (§7.6, opcional más adelante).

---

## 11. PREGUNTAS ABIERTAS Y PLAN DE LA PRÓXIMA CAPTURA

1. **Desambiguar el grupo 12.754.500** (True In / Coin Drop / Gross Drop / True Out ↔ objetos `04 09 01`, `04 04 01`): insertar monedas/billete y ver cuál objeto se mueve. Probar también `04 04 02/03`, `04 05 01..04`.
2. **Desambiguar el grupo 8.080.392** (Cashout Handpays / Split Voucher Pays / Total Handpays ↔ `02 05 03`, `02 05 04`): cobrar un voucher y repetir. Explorar `02 05 01/02/05/06/08`.
3. **Identificar** 476.780 y 12.828.900 del bloque `02 03 03`, y el meter grande 315.532.800 (`02 02 04`). **02-09**: en la máq. 2 el drop vive en la entrada [9] del bloque (verificado contra pantalla, ANEXO I.5); las consultas individuales `04 09 01`/`04 04 01` devuelven ambas 0 en esa máquina → el drop solo existe en el bloque.
4. **Probar la tolerancia de la máquina a violaciones de slot** (repetir slot, saltar slots) en un banco de pruebas — toda la captura usa rotación estricta, así que lo seguro es imitarla.
5. **Capturar un evento de juego** (meter corriendo durante poll) para ver si la máquina inserta tramas de evento espontáneas dentro del ciclo de polls.
6. **ID de juego**: el menú de la máquina muestra `AT` pero Gamma reporta `AH` (`01 02 02`, `03 01 00`). Determinar si son campos distintos (SAS game ID vs. Gamma game ID) buscando el objeto que devuelve `AT`.
7. **Localizar los campos del menú faltantes**: ID adicional (`000`), Denominación usuario, Apuesta máxima, Grupo progresivo (`00`), Opciones de juego (`0000`), Denominación Base — candidatos a estar entre los objetos no asignados (`04 01 03`=0, `04 01 16`=20.000, `04 06 01`=0, `02 05 07`=119.999…).
8. **Identificar la respuesta no-Gamma de 93 B** (§6.3-4): encuadre `[26][LEN][payload][2 B]` descubierto, CRC del trailer sin identificar tras 12 variantes + bit-shifts + decodificación de baud. Siguientes pasos: capturar con analizador lógico (baud/timing/gaps de esta emisión), registrar en el log qué TX la dispara inmediatamente antes, y ensayar el eco de su cabecera (`26 59 41 31`) como posible handshake.

---

## ANEXO A — VALIDACIÓN NUMÉRICA

| Prueba | Resultado |
|---|---|
| CRC-16/XMODEM en TX | 842/842 (100%) |
| CRC-16/XMODEM en RX | 719/719 (100%) |
| Alternativas descartadas: CRC-16/ARC, CCITT-FALSE | 0/842 y 0/719 |
| Regla LEN+4 | 1.561/1.566 (5 basura de arranque) |
| Respuesta con mismo SEQ | 718/718 |
| RX sin poll previo ≤100 ms | 0/719 |
| DATA 16B precedida de poll simple | 24/24 |
| Bytes con CRC correcto en polls de los 8 slots | 8/8 |
| Periodo entre transacciones idle | 30,4 ms medio |
| Tiempo por byte | 528 µs → 19.200 baudios |
| **Verificación en vivo (gamma_test.py, 27-08-2026)** | eco, QUERY/ACK/POLL/DATA, identidad y meters — todo confirmado; + descubrimiento del timeout de enlace y re-enganche por slot 0x40 (§6.3) |

## ANEXO B — CORRECCIONES AL ANÁLISIS PREVIO (`Sincronizacionn o Polling.txt`)

1. **"La respuesta llega en el mismo slot de la query"** — Falso. Llega **un slot después**, precedida de un poll (§6). El análisis previo no vio el ACK vacío intermedio porque agrupaba mal por tiempo.
2. **"El CRC de cada consulta es siempre el mismo"** — Solo si el SEQ es el mismo; el SEQ rota, así que hay 8 CRCs posibles por objeto. Solución: calcularlo en runtime (§4).
3. **"`40 00 0D CC` es EL polling"** — Es uno de **8 polls rotativos** (uno por slot, §5.1).
4. **"`02 05 04` = Total Handpay"** — Sin fundamento: devuelve 0. Queda como objeto no identificado (§7.2).
5. **No detectado por el análisis previo**: el intercambio STATUS 0x33/0x34, el bloque multi-meter `02 03 03`, la identidad de la máquina, la escritura-sync de la SPC (y que no es necesaria), el sondeo de arranque con slot único, el ancho de valor variable, y la prueba de que la máquina jamás transmite sin poll.

## ANEXO C — TABLA DE CONSULTAS POR SLOT (CRC ya calculados)

Generados con `Gamma_BuildRead()` — verificados contra la captura donde aparecieron. Formato: `SEQ 06 05 30 02 CL SM MT CRC_HI CRC_LO`.

| Contador | Objeto | slot 40 | slot 48 | slot 50 | slot 58 | slot 60 | slot 68 | slot 70 | slot 78 |
|---|---|---|---|---|---|---|---|---|---|
| Current Credits | 02 03 01 | DE 60 | C0 BA | E3 D4 | FD 0E | A5 08 | BB D2 | 98 BC | 86 66 |
| Games Played | 02 04 04 | 17 52 | 09 88 | 2A E6 | 34 3C | 6C 3A | 72 E0 | 51 8E | 4F 54 |
| Credits Bet | 02 04 05 | 07 73 | 19 A9 | 3A C7 | 24 1D | 7C 1B | 62 C1 | 41 AF | 5F 75 |
| Credits Won | 02 04 06 | 37 10 | 29 CA | 0A A4 | 14 7E | 4C 78 | 52 A2 | 71 CC | 6F 16 |
| Cashout Handpays* | 02 05 03 | 54 84 | 4A 5E | 69 30 | 77 EA | 2F EC | 31 36 | 12 58 | 0C 82 |
| True In | 04 09 01 | 83 0B | 9D D1 | BE BF | A0 65 | F8 63 | E6 B9 | C5 D7 | DB 0D |
| CoinDrop/GrossDrop? | 04 04 01 | F5 57 | EB 8D | C8 E3 | D6 39 | 8E 3F | 90 E5 | B3 8B | AD 51 |

\* grupo handpay — desambiguar (§7.2).

**Recomendación**: usar el generador de CRC en runtime y no esta tabla; la tabla es solo para depuración con el analizador.

## ANEXO D — ARCHIVOS

- `analisis_gamma.py` … `analisis_gamma5.py` — scripts Python del análisis (reproducibles: parseo por gap, validación CRC, correlación TX→RX, catálogo, tablas).
- `TX-27-08-202.txt` / `RX-27-08-202.txt` — capturas crudas (CSV del analizador: timestamp, byte, errores).
- `WhatsApp Image ….jpeg` — pantalla CURRENT METERS usada para validar valores.
- `docs/Manual_Protocolo_Gamma.md` — este documento.

---

## ANEXO E — CONSULTA Y RESPUESTA BYTE A BYTE, POR CONTADOR

Tramas **reales de la captura** (todas con CRC verificado). Cada bloque muestra la transacción completa de 4 pasos (QUERY → ACK → POLL → DATA, §6). Recuerda: el SEQ de la DATA corresponde al **POLL que la trajo** (un slot después de la QUERY), no al de la QUERY original.

### E.1 Mapa de bytes — CONSULTA TX (10 bytes)

```
Pos:     0     1     2     3     4     5     6     7     8     9
       ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬───────────┐
       │ SEQ │ LEN │SIZE │TYPE │ OP  │CLASE│ SUB │METER│    CRC    │
       └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴───────────┘
        slot   06    05    30    02   └── identificador ──┘  2 B, MSB 1º
```

| Pos | Ejemplo | Campo | Descripción |
|---|---|---|---|
| 0 | `78` | **SEQ** | Slot de secuencia (40/48/50/58/60/68/70/78). Cambia con cada transacción ⇒ cambia el CRC. |
| 1 | `06` | **LEN** | Bytes de payload (6). Longitud total = LEN+4 = 10. |
| 2 | `05` | SIZE | Tamaño del bloque de objeto (= LEN−1). |
| 3 | `30` | TYPE | 0x30 = bloque de comandos/propiedades. |
| 4 | `02` | OP | 0x02 = **LEER** (READ). |
| 5 | `02` | **CLASE** | Identificador del contador — byte 1 de 3. |
| 6 | `04` | **SUB** | Identificador del contador — byte 2 de 3. |
| 7 | `05` | **METER** | Identificador del contador — byte 3 de 3. |
| 8 | `5F` | **CRC MSB** | CRC-16/XMODEM de pos 0–7. |
| 9 | `75` | **CRC LSB** | |

### E.2 Mapa de bytes — RESPUESTA DATA RX (16 bytes)

```
Pos:     0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15
       ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬───────────────────┬───────┬───────────┐
       │ SEQ │ LEN │SIZE │TYPE │STAT │CLASE│ SUB │METER│ PAD │      VALOR        │TRAILER│    CRC    │
       └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴───────────────────┴───────┴───────────┘
        slot   0C    0A    31    82   └── eco del contador ─┘    uint32 BIG-ENDIAN    00     2 B, MSB 1º
```

| Pos | Ejemplo | Campo | Descripción |
|---|---|---|---|
| 0 | `40` | **SEQ** | Igual al POLL que la trajo. |
| 1 | `0C` | **LEN** | Payload = 12 B → total 16 B. |
| 2 | `0A` | SIZE | (= LEN−2; a veces 0x0B — no usar en lógica). |
| 3 | `31` | TYPE | 0x31 = bloque de datos. |
| 4 | `82` | STATUS | 0x82 = lectura OK. |
| 5 | `02` | **CLASE** | Eco del identificador — **validar contra lo consultado**. |
| 6 | `04` | **SUB** | — |
| 7 | `05` | **METER** | — |
| 8 | `00` | PAD | Relleno fijo 0x00. |
| **9–12** | `02 21 1E FF` | **VALOR** | **El contador: uint32 Big-Endian.** |
| 13 | `00` | TRAILER | 0x00 (0x01 visto una vez). |
| 14 | `CB` | **CRC MSB** | XMODEM de pos 0–13. |
| 15 | `F3` | **CRC LSB** | |

Tramas auxiliares de 4 bytes: POLL/ACK = `SEQ 00 CRC CRC` (la máquina devuelve byte a byte el poll del slot).

---

### E.3 CURRENT CREDITS — objeto `02 03 01` — pantalla: 1.194

```
1) ENVIAR QUERY (slot 0x48):
   Pos:  0     1     2     3     4     5     6     7     8     9
         48    06    05    30    02    02    03    01    C0    BA
         SEQ   LEN   SIZE  TYPE  OP    CL    SB    MT    └─ CRC ─┘
2) RECIBIR ACK (4 B, mismo SEQ):        48 00 84 65
3) ENVIAR POLL (slot siguiente 0x50):   50 00 0E BF
4) RECIBIR DATA (16 B):
   Pos:  0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15
         50    0C    0A    31    82    02    03    01    00    00    00    04    AA    00    7B    4B
         SEQ   LEN   SIZE  TYPE  ST    CL    SB    MT    PAD   └───── VALOR ─────┘    TR    └─ CRC ─┘
   VALOR (pos 9–12) = 0x000004AA = 1.194  ✓
```

### E.4 TOTAL GAMES PLAYED — objeto `02 04 04` — pantalla: 1.353.790

```
1) ENVIAR QUERY (slot 0x60):
   Pos:  0     1     2     3     4     5     6     7     8     9
         60    06    05    30    02    02    04    04    6C    3A
         SEQ   LEN   SIZE  TYPE  OP    CL    SB    MT    └─ CRC ─┘
2) RECIBIR ACK:                         60 00 0B 2A
3) ENVIAR POLL (slot siguiente 0x68):   68 00 82 83
4) RECIBIR DATA (16 B):
   Pos:  0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15
         68    0C    0A    31    82    02    04    04    00    00    14    A8    3E    00    B8    23
         SEQ   LEN   SIZE  TYPE  ST    CL    SB    MT    PAD   └───── VALOR ─────┘    TR    └─ CRC ─┘
   VALOR (pos 9–12) = 0x0014A83E = 1.353.790  ✓
```

### E.5 TOTAL CREDITS BET — objeto `02 04 05` — pantalla: 35.725.055

```
1) ENVIAR QUERY (slot 0x78):
   Pos:  0     1     2     3     4     5     6     7     8     9
         78    06    05    30    02    02    04    05    5F    75
         SEQ   LEN   SIZE  TYPE  OP    CL    SB    MT    └─ CRC ─┘
2) RECIBIR ACK:                         78 00 81 F0
3) ENVIAR POLL (slot siguiente 0x40):   40 00 0D CC
4) RECIBIR DATA (16 B):
   Pos:  0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15
         40    0C    0A    31    82    02    04    05    00    02    21    1E    FF    00    CB    F3
         SEQ   LEN   SIZE  TYPE  ST    CL    SB    MT    PAD   └───── VALOR ─────┘    TR    └─ CRC ─┘
   VALOR (pos 9–12) = 0x02211EFF = 35.725.055  ✓
```

### E.6 TOTAL CREDITS WON — objeto `02 04 06` — pantalla: 31.052.141

```
1) ENVIAR QUERY (slot 0x48):
   Pos:  0     1     2     3     4     5     6     7     8     9
         48    06    05    30    02    02    04    06    29    CA
         SEQ   LEN   SIZE  TYPE  OP    CL    SB    MT    └─ CRC ─┘
2) RECIBIR ACK:                         48 00 84 65
3) ENVIAR POLL (slot siguiente 0x50):   50 00 0E BF
4) RECIBIR DATA (16 B):
   Pos:  0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15
         50    0C    0A    31    82    02    04    06    00    01    D9    D1    6D    00    55    95
         SEQ   LEN   SIZE  TYPE  ST    CL    SB    MT    PAD   └───── VALOR ─────┘    TR    └─ CRC ─┘
   VALOR (pos 9–12) = 0x01D9D16D = 31.052.141  ✓
```

### E.7 CASHOUT HANDPAYS* — objeto `02 05 03` — pantalla: 8.080.392

```
1) ENVIAR QUERY (slot 0x58):
   Pos:  0     1     2     3     4     5     6     7     8     9
         58    06    05    30    02    02    05    03    77    EA
         SEQ   LEN   SIZE  TYPE  OP    CL    SB    MT    └─ CRC ─┘
2) RECIBIR ACK:                         58 00 87 16
3) ENVIAR POLL (slot siguiente 0x60):   60 00 0B 2A
4) RECIBIR DATA (16 B):
   Pos:  0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15
         60    0C    0A    31    82    02    05    03    00    00    7B    4C    08    00    53    21
         SEQ   LEN   SIZE  TYPE  ST    CL    SB    MT    PAD   └───── VALOR ─────┘    TR    └─ CRC ─┘
   VALOR (pos 9–12) = 0x007B4C08 = 8.080.392  ✓
```

\* del grupo Handpay — desambiguar (§7.2).

### E.8 SIN ASIGNAR (¿Handpay set 1?) — objeto `02 05 04` — devuelve 0

```
1) ENVIAR QUERY (slot 0x68):
   Pos:  0     1     2     3     4     5     6     7     8     9
         68    06    05    30    02    02    05    04    41    D1
         SEQ   LEN   SIZE  TYPE  OP    CL    SB    MT    └─ CRC ─┘
2) RECIBIR ACK:                         68 00 82 83
3) ENVIAR POLL (slot siguiente 0x70):   70 00 08 59
4) RECIBIR DATA (16 B):
   Pos:  0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15
         70    0C    0A    31    82    02    05    04    00    00    00    00    00    00    31    9D
         SEQ   LEN   SIZE  TYPE  ST    CL    SB    MT    PAD   └───── VALOR ─────┘    TR    └─ CRC ─┘
   VALOR (pos 9–12) = 0  (meter en cero o no soportado — no asignar todavía, §11)
```

### E.9 TRUE IN (COINS INSERTED)* — objeto `04 09 01` — pantalla: 12.754.500

```
1) ENVIAR QUERY (slot 0x78):
   Pos:  0     1     2     3     4     5     6     7     8     9
         78    06    05    30    02    04    09    01    DB    0D
         SEQ   LEN   SIZE  TYPE  OP    CL    SB    MT    └─ CRC ─┘
2) RECIBIR ACK:                         78 00 81 F0
3) ENVIAR POLL (slot siguiente 0x40):   40 00 0D CC
4) RECIBIR DATA (16 B):
   Pos:  0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15
         40    0C    0A    31    82    04    09    01    00    00    C2    9E    44    00    A4    C1
         SEQ   LEN   SIZE  TYPE  ST    CL    SB    MT    PAD   └───── VALOR ─────┘    TR    └─ CRC ─┘
   VALOR (pos 9–12) = 0x00C29E44 = 12.754.500  ✓
```

\* del grupo True In/Coin Drop/Gross Drop — desambiguar (§7.3).

### E.10 COIN DROP / GROSS DROP?* — objeto `04 04 01` — pantalla: 12.754.500

```
1) ENVIAR QUERY (slot 0x58):
   Pos:  0     1     2     3     4     5     6     7     8     9
         58    06    05    30    02    04    04    01    D6    39
         SEQ   LEN   SIZE  TYPE  OP    CL    SB    MT    └─ CRC ─┘
2) RECIBIR ACK:                         58 00 87 16
3) ENVIAR POLL (slot siguiente 0x60):   60 00 0B 2A
4) RECIBIR DATA (16 B):
   Pos:  0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15
         60    0C    0A    31    82    04    04    01    00    00    C2    9E    44    00    6F    6A
         SEQ   LEN   SIZE  TYPE  ST    CL    SB    MT    PAD   └───── VALOR ─────┘    TR    └─ CRC ─┘
   VALOR (pos 9–12) = 0x00C29E44 = 12.754.500  ✓
```

\* mismo valor que True In en esta máquina — desambiguar insertando monedas (§11).

### E.11 Tabla resumen (qué cambia entre contadores)

Entre un contador y otro, en la QUERY **solo cambian 3 bytes** (pos 5–6–7: CLASE/SUB/METER) más el CRC; y en la DATA cambian los mismos 3 bytes (eco, pos 5–7) y los 4 del VALOR (pos 9–12). Todo lo demás es constante:

| Campo | QUERY TX | DATA RX |
|---|---|---|
| SEQ (pos 0) | slot actual (rota +8 por transacción) | slot del POLL que la trajo |
| LEN (pos 1) | **06** (fijo) | **0C** (fijo) |
| SIZE (pos 2) | **05** (fijo) | **0A** (fijo) |
| TYPE (pos 3) | **30** (fijo) | **31** (fijo) |
| OP/STATUS (pos 4) | **02** = leer (fijo) | **82** = OK (fijo) |
| **Contador (pos 5–7)** | **CLASE / SUB / METER ← lo único que eliges** | eco de lo consultado (validar) |
| PAD (pos 8) | — | **00** (fijo) |
| **Valor (pos 9–12)** | — | **uint32 Big-Endian ← lo que buscas** |
| TRAILER (pos 13) | — | **00** (fijo) |
| **CRC (últimos 2)** | XMODEM de pos 0–7 | XMODEM de pos 0–13 |

---

## ANEXO F — MAPA LÓGICO COMPLETO: LAS 4 TRAMAS DE CADA CONTADOR

Cheat-sheet de implementación: por cada contador, la transacción completa (QUERY → ACK → POLL → DATA) con los bytes exactos. **Todas las tramas con CRC verificado (33/33).** El valor del ejemplo es el de la captura; en otra máquina/estado cambiarán los bytes 9–12 de la DATA (y su CRC), el resto es idéntico.

> El SEQ de cada ejemplo es el usado en la captura. Si tu rotación de slots está en otra posición, regenera la QUERY con el builder (§10) o usa la tabla por-slot del Anexo C. El ACK de un slot es siempre byte-idéntico al poll de ese slot.

### F.1 Current Credits — `02 03 01`
```
1. TX QUERY : 48 06 05 30 02 02 03 01 C0 BA
2. RX ACK   : 48 00 84 65
3. TX POLL  : 50 00 0E BF
4. RX DATA  : 50 0C 0A 31 82 02 03 01 00 00 00 04 AA 00 7B 4B   → valor = 1.194
```

### F.2 Total Games Played — `02 04 04`
```
1. TX QUERY : 60 06 05 30 02 02 04 04 6C 3A
2. RX ACK   : 60 00 0B 2A
3. TX POLL  : 68 00 82 83
4. RX DATA  : 68 0C 0A 31 82 02 04 04 00 00 14 A8 3E 00 B8 23   → valor = 1.353.790
```

### F.3 Total Credits Bet — `02 04 05`
```
1. TX QUERY : 78 06 05 30 02 02 04 05 5F 75
2. RX ACK   : 78 00 81 F0
3. TX POLL  : 40 00 0D CC
4. RX DATA  : 40 0C 0A 31 82 02 04 05 00 02 21 1E FF 00 CB F3   → valor = 35.725.055
```

### F.4 Total Credits Won — `02 04 06`
```
1. TX QUERY : 48 06 05 30 02 02 04 06 29 CA
2. RX ACK   : 48 00 84 65
3. TX POLL  : 50 00 0E BF
4. RX DATA  : 50 0C 0A 31 82 02 04 06 00 01 D9 D1 6D 00 55 95   → valor = 31.052.141
```

### F.5 Cashout Handpays* — `02 05 03`
```
1. TX QUERY : 58 06 05 30 02 02 05 03 77 EA
2. RX ACK   : 58 00 87 16
3. TX POLL  : 60 00 0B 2A
4. RX DATA  : 60 0C 0A 31 82 02 05 03 00 00 7B 4C 08 00 53 21   → valor = 8.080.392
```

### F.6 Sin asignar (¿handpay set 1?) — `02 05 04`
```
1. TX QUERY : 68 06 05 30 02 02 05 04 41 D1
2. RX ACK   : 68 00 82 83
3. TX POLL  : 70 00 08 59
4. RX DATA  : 70 0C 0A 31 82 02 05 04 00 00 00 00 00 00 31 9D   → valor = 0
```

### F.7 True In* — `04 09 01`
```
1. TX QUERY : 78 06 05 30 02 04 09 01 DB 0D
2. RX ACK   : 78 00 81 F0
3. TX POLL  : 40 00 0D CC
4. RX DATA  : 40 0C 0A 31 82 04 09 01 00 00 C2 9E 44 00 A4 C1   → valor = 12.754.500
```

### F.8 Coin Drop / Gross Drop?* — `04 04 01`
```
1. TX QUERY : 58 06 05 30 02 04 04 01 D6 39
2. RX ACK   : 58 00 87 16
3. TX POLL  : 60 00 0B 2A
4. RX DATA  : 60 0C 0A 31 82 04 04 01 00 00 C2 9E 44 00 6F 6A   → valor = 12.754.500
```

### F.9 Bloque multi-meter — `02 03 03` (24 contadores en una respuesta)
```
1. TX QUERY : 48 06 05 30 02 02 03 03 E0 F8
2. RX ACK   : 48 00 84 65
3. TX POLL  : 50 00 0E BF
4. RX DATA  : 50 68 66 31 82 02 03 03 00 [96 B = 24 valores BE consecutivos desde pos 9] 00 10 35
   (108 B en total — layout de entradas en §7.6)
```

\* Grupos con ambigüedad pendiente de desambiguar (§7.2, §7.3, §11).

### F.10 Cadena completa lista para ensayo (reproduce la ráfaga de la captura)

Secuencia de tramas a enviar, una tras otra (~30 ms entre cada una), para leer los 7 contadores principales empezando desde el slot 0x78 — con las respuestas esperadas intercaladas:

```
TX  78 06 05 30 02 02 04 05 5F 75   (QUERY Credits Bet)      RX  78 00 81 F0
TX  40 00 0D CC                     (POLL)                  RX  40 0C 0A 31 82 02 04 05 00 02 21 1E FF 00 CB F3
TX  48 06 05 30 02 02 04 06 29 CA   (QUERY Credits Won)     RX  48 00 84 65
TX  50 00 0E BF                     (POLL)                  RX  50 0C 0A 31 82 02 04 06 00 01 D9 D1 6D 00 55 95
TX  58 06 05 30 02 02 05 03 77 EA   (QUERY Handpays)        RX  58 00 87 16
TX  60 00 0B 2A                     (POLL)                  RX  60 0C 0A 31 82 02 05 03 00 00 7B 4C 08 00 53 21
TX  68 06 05 30 02 02 05 04 41 D1   (QUERY 02 05 04)        RX  68 00 82 83
TX  70 00 08 59                     (POLL)                  RX  70 0C 0A 31 82 02 05 04 00 00 00 00 00 00 31 9D
TX  78 06 05 30 02 04 09 01 DB 0D   (QUERY True In)         RX  78 00 81 F0
TX  40 00 0D CC                     (POLL)                  RX  40 0C 0A 31 82 04 09 01 00 00 C2 9E 44 00 A4 C1
TX  48 06 05 30 02 02 03 01 C0 BA   (QUERY Current Credits) RX  48 00 84 65
TX  50 00 0E BF                     (POLL)                  RX  50 0C 0A 31 82 02 03 01 00 00 00 04 AA 00 7B 4B
TX  58 06 05 30 02 04 04 01 D6 39   (QUERY CoinDrop?)       RX  58 00 87 16
TX  60 00 0B 2A                     (POLL)                  RX  60 0C 0A 31 82 04 04 01 00 00 C2 9E 44 00 6F 6A
TX  68 00 82 83 …                   (seguir con polls vacíos rotando — keep-alive)
```

Esta cadena es exactamente la secuencia capturada entre t=52,19 s y t=52,59 s: sirve como script de prueba contra la máquina real o contra un simulador.

Todo objeto consultado con su valor en ese momento. Útil para localizar meters no identificados (juegos desde encendido, aperturas de puerta, etc.).

### H.1 Objetos con valor no cero

| Objeto | Valor | Identificación |
|---|---|---|
| `02 02 01` | 32.800 (0x8020) | ¿flags/config? |
| `02 02 04` | 315.532.800 (0x12CEA600) | ¿bitmask de características? |
| `02 02 0A` | 675 | sin asignar |
| `02 03 01` | 1.194 | ✔ Current Credits |
| `02 04 01` | 16.777.216 (0x01000000) | ¿flags? |
| `02 04 04` | 1.353.790 | ✔ **Total Games Played** (desde reset) |
| `02 04 05` | 35.725.055 | ✔ Total Credits Bet |
| `02 04 06` | 31.052.141 | ✔ Total Credits Won |
| `02 05 03` | 8.080.392 | ✔ grupo Handpay |
| `02 05 07` | 119.999 | sin asignar (¿límite?) |
| `03 01 0E` | 200 | sin asignar (también aparece en bloque 03 01 00) |
| `04 01 01` | 100 | ✔ Denominación SAS (0,01 $) |
| `04 01 16` | 20.000 | candidato apuesta máxima |
| `04 04 01` | 12.754.500 | grupo True In |
| `04 06 08..0E` | 100/200/500/1000/2000/5000/10000 | tabla de créditos |
| `04 09 01` | 12.754.500 | ✔ True In |
| `06 02 01` | 263 | sin asignar |
| `80 02 0A` | 1 | estado host |
| bloques | `01 02 01`, `01 02 02`, `02 03 03`, `02 03 04`, `02 04 03`, `02 05` varios, `03 01 00`, `04 02 0C`, `04 03 01`, `04 05 03`, `04 06 00`, `04 06 16`, `04 07 01`, `04 08 01`, `04 0C 06`, `80 02 05..09` | ver §7.5/§7.6 |

### H.2 Cola del bloque `03 01 00` (tras el texto)

`…, Games Played (1.353.790), 476.780, **1, 1, 200**, 0, 0` — los dos campos con valor **1** y el **200** son candidatos a meters de sesión (p. ej. algo contado desde el encendido: la máquina llevaba encendida ~1 min y el analizador se conectó abriendo la puerta). Requieren confirmación conductual.

### H.3 Objetos barridos en cero

Clase `80 03 01..22` (toda la familia), `80 02 02..09/0D`, `81 01 01..03`, `02 05 09/0D/0E/0F/10`, `02 01 06`, `02 02 16/1C/1D`, `02 03 05`, `02 04 02`, `02 06 01`, `02 07 02`, `03 01 80/81`, `04 01 03/06/0A`, `04 02 02/0A`, `04 05 01/02/04` (64 bit), `04 0A 04`, `04 0C 03`, `04 0D 03/04`, `04 0E 03/05`, `04 0C 08/09/0A`.

### H.4 Meters buscados aún sin mapear

| Meter buscado | Estado en captura | Candidatos a probar |
|---|---|---|
| Total juegos **desde reset** | ✔ `02 04 04` (coincide con pantalla) | — |
| Total juegos **desde encendido** | ✖ no identificado | los "1" de 03 01 00; explorar `02 04 07..0A`, `02 03 02`, `06 02 02..` |
| Total **aperturas de puerta** | ✖ no identificado | `06 02 01` (263); explorar `02 05 05/06/08`, `04 07 03/05`, `04 08 03/05` |

**Procedimiento de identificación** (los valores ya cambiaron, así que por comportamiento): leer candidatos con gamma_gui → jugar exactamente 1 juego → re-leer (el de juegos suma +1) → abrir/cerrar la puerta 1 vez → re-leer (el de puertas suma +1). Cruzar también con los números que muestra el menú de meters de la máquina en ese momento.

---

## ANEXO G — SECUENCIA: LEER LA INFORMACIÓN DE LA MÁQUINA (IDENTIDAD/CONFIG)

Las 6 consultas de información (todas las tramas reales de la captura, CRC verificado). Mismo modelo QUERY→ACK→POLL→DATA que los meters; la diferencia es que la DATA es un bloque largo con **cadenas ASCII**.

> En los bloques de texto, las cadenas arrancan tras la cabecera (entre pos 9 y 12 según el bloque — cabecera corta variable). Lo robusto es escanear runs de ASCII imprimible separados por 0x00/espacios, no fijar offset.

### G.1 Versión / serie — `01 02 01` (respuesta de 60 B)
```
1. TX QUERY : 58 06 05 30 02 01 02 01 97 6F
2. RX ACK   : 58 00 87 16
3. TX POLL  : 60 00 0B 2A
4. RX DATA  : 60 38 36 31 82 01 02 01 00 30 31 2E 30 34 2E 30 30 33 20 20 20 20 20 20 00
              ... 20 00 30 30 30 30 30 31 00 00 ... 93 83        (60 B)
   ASCII: "01.04.003"  /  "000001"      (versión SW / serie)
```

### G.2 Fabricante / plataforma / paytable / ID juego / base code — `01 02 02` (98 B)
```
1. TX QUERY : 68 06 05 30 02 01 02 02 E1 D0
2. RX ACK   : 68 00 82 83
3. TX POLL  : 70 00 08 59
4. RX DATA  : 70 5E 5C 31 82 01 02 02 00 00 00 [ASCII] ... B1 3E   (98 B)
   ASCII: "Aristocrat Technologies, Inc." / "MkV-USA" / "G1607" / "AH" / "USA A4.2 Base Code"
```

### G.3 Juego / % payback / meters resumen — `03 01 00` (102 B)
```
1. TX QUERY : 60 06 05 30 02 03 01 00 E4 7B
2. RX ACK   : 60 00 0B 2A
3. TX POLL  : 68 00 82 83
4. RX DATA  : 68 62 60 31 82 03 01 00 00 01 [ASCII + meters] ... 6C 5E   (102 B)
   ASCII: "G1607" / "AH" / "Dolphin Treasure" / "87.928%"
   y tras el texto: Credits Bet, Credits Won, Games Played, 476.780… (uint32 BE)
```

### G.4 Denominación SAS — `04 01 01` (16 B, formato meter estándar)
```
1. TX QUERY : 68 06 05 30 02 04 01 01 6F 10
2. RX ACK   : 68 00 82 83
3. TX POLL  : 70 00 08 59
4. RX DATA  : 70 0C 0A 31 82 04 01 01 00 00 00 00 64 00 DA 31
   valor (pos 9–12) = 100 → 100 × 0,0001 = 0,01 $   ✔ confirmado contra menú de máquina
```

### G.5 Apuesta máxima (candidato) — `04 01 16` (16 B)
```
1. TX QUERY : 78 06 05 30 02 04 01 16 30 72
2. RX ACK   : 78 00 81 F0
3. TX POLL  : 40 00 0D CC
4. RX DATA  : 40 0C 0A 31 82 04 01 16 00 00 00 4E 20 00 68 92
   valor (pos 9–12) = 20.000 (20.000 × 0,01 = 200 $) — sin confirmar contra menú
```

### G.6 Tabla de créditos/denominaciones — `04 06 00` (220 B) — ⚠ tarda 2 polls
```
1. TX QUERY : 70 06 05 30 02 04 06 00 C5 C8
2. RX ACK   : 70 00 08 59
3. TX POLL  : 78 00 81 F0    →  RX: 78 00 81 F0   (¡vacio! la maquina aun preparaba el bloque)
4. TX POLL  : 40 00 0D CC    →  RX: 40 D8 D7 31 82 04 06 00 00 ... (220 B)
   contiene valores 100/200/500/1000/2000/5000/10000/20000... (tabla de creditos)
```
Este es el único caso de la captura donde la DATA no llegó en el primer poll — regla práctica: si el poll responde vacío, seguir polleando (hasta 2–3) antes de reintentar la QUERY.

### G.7 Cadena recomendada para el arranque del ESP32

Al conectar (tras el sondeo de arranque, §10), leer en este orden — igual que hizo la SPC:

```
1) 04 01 01   denominación SAS          → saber convertir créditos ↔ dinero
2) 01 02 02   fabricante/paytable/ID     → identificar la máquina
3) 03 01 00   juego/%/paytable           → payback y nombre de juego
4) 01 02 01   versión/serie              → para logs/telemetría
5) (opcional) 04 01 16, 04 06 00
```
Después de esto, pasar al ciclo normal de lectura de meters (Anexo F).

---

## ANEXO H — INVENTARIO COMPLETO DE OBJETOS LEÍDOS POR LA SPC (captura 27-08)

Todo objeto consultado con su valor en ese momento. Útil para localizar meters no identificados (juegos desde encendido, aperturas de puerta, etc.).

### H.1 Objetos con valor no cero

| Objeto | Valor | Identificación |
|---|---|---|
| `02 02 01` | 32.800 (0x8020) | ¿flags/config? |
| `02 02 04` | 315.532.800 (0x12CEA600) | ¿bitmask de características? |
| `02 02 0A` | 675 | sin asignar |
| `02 03 01` | 1.194 | ✔ Current Credits |
| `02 04 01` | 16.777.216 (0x01000000) | ¿flags? |
| `02 04 04` | 1.353.790 | ✔ **Total Games Played** (desde reset) |
| `02 04 05` | 35.725.055 | ✔ Total Credits Bet |
| `02 04 06` | 31.052.141 | ✔ Total Credits Won |
| `02 05 03` | 8.080.392 | ✔ grupo Handpay |
| `02 05 07` | 119.999 | sin asignar (¿límite?) |
| `03 01 0E` | 200 | sin asignar (también aparece en bloque 03 01 00) |
| `04 01 01` | 100 | ✔ Denominación SAS (0,01 $) |
| `04 01 16` | 20.000 | candidato apuesta máxima |
| `04 04 01` | 12.754.500 | grupo True In |
| `04 06 08..0E` | 100/200/500/1000/2000/5000/10000 | tabla de créditos |
| `04 09 01` | 12.754.500 | ✔ True In |
| `06 02 01` | 263 | sin asignar |
| `80 02 0A` | 1 | estado host |
| bloques | `01 02 01`, `01 02 02`, `02 03 03`, `02 03 04`, `02 04 03`, `03 01 00`, `04 02 0C`, `04 03 01`, `04 05 03`, `04 06 00`, `04 06 16`, `04 07 01`, `04 08 01`, `04 0C 06`, `80 02 05..09` | ver §7.5/§7.6 |

### H.2 Cola del bloque `03 01 00` (tras el texto)

`…, Games Played (1.353.790), 476.780, **1, 1, 200**, 0, 0` — los dos campos con valor **1** y el **200** son candidatos a meters de sesión (algo contado desde el encendido: la máquina llevaba encendida ~1 min y la puerta se abrió para conectar el analizador). Requieren confirmación conductual.

### H.3 Objetos barridos en cero

Clase `80 03 01..22` (toda la familia), `80 02 02..09/0D`, `81 01 01..03`, `02 05 09/0D/0E/0F/10`, `02 01 06`, `02 02 16/1C/1D`, `02 03 05`, `02 04 02`, `02 06 01`, `02 07 02`, `03 01 80/81`, `04 01 03/06/0A`, `04 02 02/0A`, `04 05 01/02/04` (64 bit), `04 0A 04`, `04 0C 03`, `04 0D 03/04`, `04 0E 03/05`, `04 0C 08/09/0A`.

### H.4 Meters buscados aún sin mapear

| Meter buscado | Estado en captura | Candidatos a probar |
|---|---|---|
| Total juegos **desde reset** | ✔ `02 04 04` (coincide con pantalla) | — |
| Total juegos **desde encendido** | ✖ no identificado | los "1" de 03 01 00; explorar `02 04 07..0A`, `02 03 02`, `06 02 02..` |
| Total **aperturas de puerta** | ✖ no identificado | `06 02 01` (263); explorar `02 05 05/06/08`, `04 07 03/05`, `04 08 03/05` |

**Procedimiento de identificación** (los valores ya cambiaron, así que por comportamiento): leer candidatos con gamma_gui → jugar exactamente 1 juego → re-leer (el de juegos suma +1) → abrir/cerrar la puerta 1 vez → re-leer (el de puertas suma +1). Cruzar también con los números que muestra el menú de meters de la máquina en ese momento.

---

## ANEXO I — BLOQUE `02 03 03`: EL PAQUETE COMPLETO DE CONTADORES (v1.1)

**El "comando largo"**: la respuesta extendida del objeto `02 03 03` entrega **todo el paquete de contadores de la máquina en una sola transacción**. La consulta es la QUERY estándar de 10 B (§7.6) y la respuesta es una trama de **108 B** (LEN=0x68) con **24 valores uint32 Big-Endian consecutivos**:

```
[0]=SEQ  [1]=0x68  [2]=0x66  [3]=0x31  [4]=0x82  [5..7]=02 03 03  [8]=0x00
[9..104] = 24 valores BE32 → la entrada i está en RX[9+4i]
[105]=0x00 (trailer)   [106..107]=CRC
```

Layout **verificado dos veces**:
- **27-08 (captura con analizador)**: valores del bloque cruzados con las consultas individuales de la misma sesión.
- **28-08 (en vivo, desde el firmware ESP32)**: consulta bajo demanda — `Gamma_Activar_Query_Bloque()` en `GammaProLink` — con las **mismas posiciones** y valores evolucionados de forma coherente.

### I.1 Mapa de las 24 entradas

| entry | Posición RX | Valor 27-08 (captura) | Valor 28-08 (vivo) | Identificado como |
|---|---|---|---|---|
| 0 | 9–12 | 315.532.800 | — | = `02 02 04` individual (verificado) — ¿bitmask/config? |
| **1** | 13–16 | 1.194 | 200 † | **Current Credits** (`02 03 01`) |
| **2** | 17–20 | 1.353.790 | 1.353.803 † | **Total Games Played** (`02 04 04`) |
| 3 | 21–24 | 476.780 | 476.786 | sin asignar — meter vivo; mismo valor que la cola de `03 01 00` (§H.2) |
| **4** | 25–28 | 35.725.055 | 35.727.655 † | **Total Credits Bet** (`02 04 05`) |
| **5** | 29–32 | 31.052.141 | 31.053.511 † | **Total Credits Won** (`02 04 06`) |
| 6 | 33–36 | 0 | — | en cero |
| 7 | 37–40 | 0 | — | en cero |
| 8 | 41–44 | 12.828.900 | 12.830.700 | sin asignar — **familia drop** (valor propio, ≠ `04 09 01`/`04 04 01`); desambiguar con billete (§11) |
| 9 | 45–48 | 0 | — | en cero |
| 10 | 49–52 | 0 | — | en cero |
| 11 | 53–56 | 0 | — | en cero |
| 12 | 57–60 | 0 | — | en cero |
| **13** | 61–64 | 8.080.392 | 8.080.556 † | **grupo Handpay** (`02 05 03`) |
| 14 | 65–68 | 0 | — | en cero |
| 15 | 69–72 | 12.754.500 | 12.755.500 | **familia drop**: el 27-08 coincidió exacto con `04 09 01`; el 28-08 **NO** (individual = 12.754.900) → es un miembro propio de la familia, no True In (§11) |
| 16 | 73–76 | 0 | — | en cero |
| 17 | 77–80 | 0 | — | en cero |
| 18 | 81–84 | 0 | — | en cero |
| 19 | 85–88 | 0 | — | en cero |
| 20 | 89–92 | 0 | — | en cero |
| 21 | 93–96 | 0 | — | en cero |
| 22 | 97–100 | 0 | — | en cero |
| 23 | 101–104 | 0 | — | en cero |

† valor de ese día obtenido por consulta individual del objeto correspondiente; la lectura en vivo del bloque citó expresamente las entradas [3], [8] y [15].

### I.2 Cómo se validó el mapa

- **27-08**: cada entrada se cruzó contra la consulta individual del objeto — coincidieron exacto las entradas **0, 1, 2, 4, 5, 13 y 15**.
- **28-08**: relectura en vivo con las posiciones estables y los meters avanzando de forma coherente entre sesiones:

| Entrada / objeto | Δ 27-08 → 28-08 |
|---|---|
| [4] Credits Bet | +2.600 |
| [5] Credits Won | +1.370 |
| [2] Games Played | +13 |
| [13] Handpay | +164 |
| `04 09 01` True In (individual) | +400 |
| **[8] sin asignar** | **+1.800** |
| **[15] sin asignar** | **+1.000** |
| **[3] sin asignar** | **+6** |

Los deltas de [8], [15] y [3] son la pista de identificación (§11): cantidades compatibles con dinero aceptado / actividad de sesión — probar con un billete y anotar cuál se mueve.

### I.3 Uso desde el firmware

El bloque **no está en el ciclo permanente** de encuestas: se consulta **bajo demanda**. `Gamma_Activar_Query_Bloque()` iza `Flag_Query_Gamma` → la tarea de encuestas llama a `Gamma_ConsultarBloque()` (QUERY única + polls, ventana de 3 s) → el despachador imprime las 24 entradas indexadas con `Debug_Gamma` activo y cierra la sesión. El modelo transaccional es el estándar de §6: QUERY → ACK → POLL → DATA de 108 B.

### I.4 Recomendación

- Layout del **núcleo** ([0],[1],[2],[4],[5],[13]) validado en dos sesiones sobre esta máquina/firmware (Dolphin Treasure, G1607) y re-confirmado en una segunda máquina (02-09, ANEXO I.5): **apto para polling rápido** — 1 transacción (~90 ms) en vez de 7.
- La **zona drop ([8]/[9]/[15]) es dependiente de la máquina/config** — no asumir mapeo fijo (ver I.5).
- Para la semántica inequívoca de un meter suelto, la consulta individual sigue siendo la fuente autorizada (eco CL/SM/MT verificable trama a trama, §6.2).

### I.5 Tercera sesión (02-09-2026) — MÁQUINA DISTINTA: la zona drop cambia de entrada

Consulta del bloque desde el firmware sobre una **máquina diferente** a las dos primeras sesiones. Frame de 108 B con CRC válido; los objetos individuales consultados en la misma sesión coinciden exacto con las mismas posiciones del bloque:

| Entrada | Pos RX | Valor 02-09 (máq. 2) | Nota |
|---|---|---|---|
| [0] | 9–12 | 315.589.657 | = `02 02 04` individual ✓ (misma familia de valor grande que máq. 1) |
| [1] | 13–16 | 502 | Current Credits ✓ |
| [2] | 17–20 | 88.016 | Games Played ✓ |
| [3] | 21–24 | 20.505 | vivo, como en máq. 1 (también en cola de `03 01 00`) |
| [4] | 25–28 | 2.030.269 | Credits Bet ✓ |
| [5] | 29–32 | 1.830.730 | Credits Won ✓ |
| **[8]** | 41–44 | **0** | familia drop **en cero** en esta máquina |
| **[9]** | 45–48 | **758.600** | **DROP** — verificado contra la pantalla de meters de esta máquina |
| [13] | 61–64 | 558.559 | grupo Handpay (`02 05 03`) ✓ |
| **[15]** | 69–72 | **0** | familia drop **en cero** en esta máquina |

Resto de entradas en cero. Lectura del frame: `… 00 1B EF 4A` ([5]) + 12 B en cero ([6][7][8]) + `00 0B 93 48` ([9] = 758.600) + …

**Conclusiones**:

1. **Núcleo estable entre máquinas**: [0],[1],[2],[4],[5],[13] conservan su significado en las dos máquinas → el bloque sirve para polling rápido de esos meters sin re-validación por máquina.
2. **Zona drop (8/9/15) dependiente de máquina/config**: máq. 1 reportaba la familia drop en [8] y [15] con [9]=0; máq. 2 reporta el drop en [9] con [8]=[15]=0. Cada entrada parece ser una variante de fuente de drop (billete/moneda/ticket…) que se puebla según el equipamiento/config de la máquina.
3. **Implicación de firmware**: nunca mapear la zona drop por posición fija. Para el drop en producción, consulta individual (eco verificable) — o auto-aprender el layout al arranque leyendo N meters individuales y casando valores contra el bloque. → **RESUELTO e implementado (I.6)**: regla adaptable individual-vs-bloque[9], sin scanner de arranque.
4. **02-09 (resuelto)**: en máq. 2 las consultas individuales `04 09 01` (True In) y `04 04 01` (Coin Drop) devuelven **ambas 0** — los objetos drop individuales están muertos en esa máquina; el drop SOLO existe en el bloque ([9]). No hay ambigüedad de variantes ahí: es la única fuente viva.

> Curiosidad aritmética de este snapshot: `[9] = [13] + ([4]−[5]) + [1]` (758.600 = 558.559 + 199.539 + 502, exacto). Con [9]=drop se leería «drop = handpay + retención + créditos actuales» — probable coincidencia del corte; verificar si se mantiene al evolucionar los meters antes de sacar conclusiones.

### I.6 Fuente de drop adaptable — IMPLEMENTADA en el firmware (02-09-2026)

Cierra la implicación de I.5.3. Regla que corre en el ciclo de encuestas (`Gamma_DecidirFuenteDrop` en `GammaProLink.cpp`; cableada en el handler del objeto `04 09 01`, en el case 9 del bloque y en la rama de salto de `Encuestas_Maquinas_Gamma`). Poblaciones de máquinas reportadas en campo:

- **A**: el individual `04 09 01` responde **0** y el drop vive solo en el bloque **[9]** (máq. 2)
- **B**: bloque [9] en 0 y el drop solo en el individual (máq. 1)
- **C**: valor por **ambos lados**, que puede **diferir** → gana el individual (decisión del operador)

**Regla — evaluada en cada lectura, decisión sticky (exactamente UNA vía setea por ciclo, sin el 0/valor alternado):**

| Condición | Fuente |
|---|---|
| individual > 0 | **individual** (gana siempre, aunque el bloque traiga otro valor) |
| individual == 0 (ya respondió) y bloque [9] > 0 | **bloque [9]** |
| bloque > 0 pero el individual **nunca respondió** | **sin decidir** — el bloque NO decide solo por haberse leído primero |
| individual muerto (saltado por timeout tras 3 reintentos) y bloque > 0 | **bloque [9]** |
| ambos en 0 | **sin decidir** — la primera lectura no-cero decide sola |

**El guard `drop_individual_leido`** (fila 3) evita el transitorio bloque→individual: con ambos lados vivos y el bloque leído ANTES que el individual (reorden de la tabla, consulta bajo demanda temprana), sin el guard se seteaba del bloque y al llegar el individual se re-seteaba — dos escrituras, dos líneas de cambio de fuente y un posible **Flag_Bill_In falso** (la subida bloque→individual detectada por `Change_Counters_Bill_In`). Con el guard, el bloque espera a que el individual haya respondido (aunque sea con 0) o haya sido saltado como muerto.

Casos cubiertos **sin scanner de arranque** (un scanner pre-`Comunicacion_Maq` no aporta: la bandera se iza tras el primer ciclo completo, cuando ambas lecturas ya llegaron):

- **RAM clear a mitad de vida**: ambos vuelven a 0; el primer drop posterior revela por qué lado reporta la máquina y la regla re-decide sola (cambia de fuente si hace falta)
- **Máquina recién limpiada** (ambos 0 legítimos): no se setea nada hasta la primera lectura no-cero — un scanner de arranque la clasificaría mal
- **Costo aceptado**: en una máquina tipo A donde el bloque se leyera antes que el individual, el primer seteo tarda ≤1 ciclo extra (el guard retiene la primera lectura del bloque)

Setea `Total_Drop` + `Bill_Amount` y llama `Change_Counters_Bill_In` (detección de billete) **desde la fuente activa**. Bajo `Debug_Gamma`, la decisión se anota únicamente en los cambios: `[GAMMA] Fuente del drop -> individual (04 09 01)` / `bloque 02 03 03 [09]`.

**Fuera de alcance** (a decisión): `04 04 01` (Total_Coin_Drop) sigue con su objeto individual — en máquinas tipo A está muerto y su contador queda en 0; la zona drop [8]/[15] del bloque no se usa como fuente (en las máquinas vistas con el individual muerto, [8]=[15]=0).
