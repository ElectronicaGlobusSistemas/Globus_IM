# PROTOCOLO GAMMA — MANUAL OPERATIVO
## Comunicación ESP32 ↔ Máquinas Aristocrat (sin SPC)

**Versión 1.0 — 28/08/2026**
**Fuente**: `Manual_Protocolo_Gamma.md` (análisis forense de 1.561 tramas) + integración y verificación en máquina real (Dolphin Treasure G1607) con el firmware Globus IM.

Este documento es la **guía operativa condensada**: qué configurar, cómo sincronizar y qué se envía/recibe por cada contador. Para el detalle forense, ver el manual completo.

---

## 1. INTERFAZ FÍSICA Y CONFIGURACIÓN SERIAL

### 1.1 Interfaz

| Parámetro | Valor |
|---|---|
| Nivel eléctrico | **RS-232 verdadera (±V)** en el puerto de la máquina |
| Interfaz del ESP32 | UART2 (UART_NUM_2) → transceiver **MAX232** de la placa |
| Topología | Punto a punto, **maestro-esclavo** |
| Rol del ESP32 | **Maestro** (reemplaza a la SPC-2) |
| Regla de oro | **La máquina NUNCA transmite sin poll** — no hay eventos push |
| Requisito crítico | **Tierra común (GND)** entre ESP32 y máquina — sin ella, nada funciona |

### 1.2 Configuración serial

| Parámetro | Valor | Nota |
|---|---|---|
| Baudios | **19.200** | verificado por timing: 528 µs/byte ≈ teórico 520,8 µs |
| Bits de datos | **8** | |
| Paridad | **NINGUNA** | ⚠ Gamma **NO** usa 8E1 como SAS |
| Bits de parada | **1** | 10 bits por byte en total |
| Control de flujo | **Ninguno** | |
| Inversión de línea | **NO** | no requiere `uart_set_line_inverse` (el MAX232 la maneja) |

```c
// Configuración ESP-IDF equivalente (UART2)
uart_config_t cfg = {
    .baud_rate = 19200,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
};
```

---

## 2. ESTRUCTURA DE TRAMA

### 2.1 Trama base (aplica a TODO lo que se transmite y recibe)

```
┌─────┬─────┬──────────────────┬─────────────┐
│ SEQ │ LEN │  PAYLOAD         │  CRC (2 B)  │
│ 1 B │ 1 B │  LEN bytes       │  MSB primero│
└─────┴─────┴──────────────────┴─────────────┘
  Longitud total de la trama = LEN + 4 bytes
```

| Campo | Descripción |
|---|---|
| **SEQ** | Slot de secuencia rotativo: **0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78** (8 slots, paso +8). Cada transmisión del maestro consume el siguiente slot. La respuesta llega **con el mismo SEQ** de la transmisión que contesta |
| **LEN** | Longitud del payload en bytes (0–255). Define el largo total |
| **PAYLOAD** | Contenido según tipo de trama (ver §2.3) |
| **CRC** | CRC-16/XMODEM calculado sobre **SEQ+LEN+PAYLOAD** (toda la trama menos sus 2 bytes de CRC). Propiedad útil: al pasarle la trama completa, residuo = 0 → valida lo recibido sin extraer campos |

### 2.2 CRC-16/XMODEM (verificado 100 % contra máquina real)

```
poly=0x1021  init=0x0000  refin=false  refout=false  xorout=0x0000  MSB first
```

Vectores de comprobación:
| Entrada | CRC |
|---|---|
| `40 00` (poll slot 0x40) | `0D CC` |
| `78 06 05 30 02 02 04 05` (QUERY Credits Bet) | `5F 75` |
| `40 0C 0A 31 82 02 04 05 00 02 21 1E FF 00` (DATA) | residuo 0 |

### 2.3 Tipos de trama

| Tipo | Dirección | Formato | Longitud |
|---|---|---|---|
| **POLL vacío** | ESP→MÁQ | `[SEQ][00][CRC]` | 4 B |
| **ACK / eco** | MÁQ→ESP | `[SEQ][00][CRC]` | 4 B |
| **QUERY lectura** | ESP→MÁQ | `[SEQ][06][05][30][02][CL][SM][MT][CRC]` | 10 B |
| **DATA meter** | MÁQ→ESP | `[SEQ][0C][0A][31][82][CL][SM][MT][00][VALOR BE32][00][CRC]` | 16 B |
| **DATA bloque** | MÁQ→ESP | `[SEQ][LEN][SIZE][31][82][CL][SM][MT][00]...valores...[00][CRC]` | variable |
| **WRITE** | ESP→MÁQ | `[SEQ][08][06][30][04][CL][SM][MT][D1][D0][CRC]` | 12 B |
| **STATUS** | ambos | `[SEQ][04][02][33/34][MODO][00][CRC]` | 8 B |

`CL SM MT` = **Clase, Subgrupo, Meter** — la dirección del objeto consultado. La DATA siempre **repite (hace eco)** estos 3 bytes en las posiciones 5–7: hay que validarlos contra lo pedido.

---

## 3. SINCRONIZACIÓN DEL ENLACE

### 3.1 Arranque / re-enganche (adquisición del enlace)

La máquina readquiere la comunicación **SOLO al ver un poll del slot 0x40**. Secuencia de arranque:

| Paso | Quién | Trama | Observación |
|---|---|---|---|
| 1 | ESP | `40 00 0D CC` (poll slot 0x40) | repetir cada ~250 ms. La máquina puede estar muda hasta 28 s mientras arranca |
| 2 | MÁQ | `40 00 0D CC` (eco) | al primer eco, el enlace está vivo |
| 3 | ESP | siguiente slot (**0x48**) | a partir de aquí la secuencia es estricta: 0x48, 0x50, 0x58… |

⚠ Con el enlace caído, la máquina **ignora** polls y QUERYs de cualquier otro slot.

### 3.2 Modelo diferido de lectura (QUERY → ACK → POLL → DATA)

La DATA **nunca** responde directamente a la QUERY: llega con el poll siguiente.

| Paso | Quién | Trama | SEQ | Latencia |
|---|---|---|---|---|
| 1 | ESP | `QUERY 02 04 04` → `48 06 05 30 02 02 04 04 09 88` | 0x48 | — |
| 2 | MÁQ | **ACK vacío** → `48 00 84 65` | **0x48** (el de la QUERY) | ~10 ms |
| 3 | ESP | **POLL** del slot siguiente → `50 00 0E BF` | 0x50 | — |
| 4 | MÁQ | **DATA** → `50 0C 0A 31 82 02 04 04 00 00 14 A8 4B 00 FF DC` | **0x50** (¡el del POLL!) | 3,5–10 ms |

**Reglas del modelo:**
- La QUERY se envía **UNA** vez; el poll que la sigue es **vacío** (la máquina recuerda qué le pediste)
- No enviar una QUERY nueva antes de recolectar la DATA de la anterior
- Validar siempre: CRC = 0, SEQ = última transmisión, eco del objeto = consultado

### 3.3 Reglas de tiempo

| Parámetro | Valor | Nota |
|---|---|---|
| Cadencia entre transacciones | **~30 ms** (SPC real) | la máquina tolera 60–110 ms verificado; el firmware Globus opera a ~90 ms |
| Silencio mínimo entre tramas | ≥ 3,5 ms | un parser puede delimitar tramas por gap > 2 ms |
| Latencia de la DATA tras el poll | 3,5–10 ms | bloques grandes (220 B) pueden tardar 2 polls |
| **Timeout de enlace** | entre ~1 s y 56 s de silencio del maestro | al vencer, la máquina **deja de responder a todo** → re-enganchar con slot 0x40 (§3.1) |
| Regla crítica | **NUNCA dejar de pollear** | el polling continuo mantiene vivo el enlace |

### 3.4 Mantenimiento y recuperación

| Situación | Acción |
|---|---|
| Enlace estable | rotar polls/queries consumiendo slots (0x40→0x78→0x40…) |
| Meter sin DATA tras 3 reintentos | saltar al siguiente objeto y seguir |
| Pérdida de enlace | solo polls `40 00 0D CC` (1000 ms aprox) → reanudar rotación desde 0x48 |

---

## 4. CONTADORES Y OBJETOS

Cada ítem muestra la **QUERY que se envía** y la **DATA que se recibe** (ejemplos reales de máquina). Valor del meter: **4 bytes Big-Endian en posiciones 9–12**, dominio de **8 dígitos** con roll-over a 0 (la máquina lo maneja sola).

### 4.1 Total Games Played — objeto `02 04 04`
```
TX QUERY : 48 06 05 30 02 02 04 04 09 88
TX POLL  : 50 00 0E BF
RX DATA  : 50 0C 0A 31 82 02 04 04 00 00 14 A8 4B 00 FF DC
                     └─eco─┘    └──valor BE32──┘
           valor = 0x0014A84B = 1.353.803 juegos
```

### 4.2 Total Credits Bet — objeto `02 04 05`
```
TX QUERY : 58 06 05 30 02 02 04 05 24 1D
RX DATA  : 60 0C 0A 31 82 02 04 05 00 02 21 29 27 00 8A 87
           valor = 0x02212927 = 35.727.655 créditos apostados
```

### 4.3 Total Credits Won — objeto `02 04 06`
```
TX QUERY : 68 06 05 30 02 02 04 06 52 A2
RX DATA  : 70 0C 0A 31 82 02 04 06 00 01 D9 D6 C7 00 BF 7F
           valor = 0x01D9D6C7 = 31.053.511 créditos ganados
```

### 4.4 Cashout Handpays — objeto `02 05 03`
```
TX QUERY : 78 06 05 30 02 02 05 03 0C 82
RX DATA  : 40 0C 0A 31 82 02 05 03 00 00 7B 4C AC 00 1F 54
           valor = 0x007B4CAC = 8.080.556
```

### 4.5 True In (Coins In) — objeto `04 09 01`
```
TX QUERY : 48 06 05 30 02 04 09 01 9D D1
RX DATA  : 50 0C 0A 31 82 04 09 01 00 00 C2 9F D4 00 4D ED
           valor = 0x00C29FD4 = 12.754.900
```

### 4.6 Coin Drop / Gross Drop — objeto `04 04 01`
```
TX QUERY : 58 06 05 30 02 04 04 01 D6 39
RX DATA  : 60 0C 0A 31 82 04 04 01 00 00 C2 9F D4 00 40 B1
           valor = 12.754.900 (⚠ devuelve idéntico a True In en esta máquina — desambiguación pendiente)
```

### 4.7 Current Credits — objeto `02 03 01`
```
TX QUERY : 68 06 05 30 02 02 03 01 BB D2
RX DATA  : 70 0C 0A 31 82 02 03 01 00 00 00 00 C8 00 57 0C
           valor = 0x000000C8 = 200 créditos (= $2,00 a denominación 0,01)
```

### 4.8 Denominación SAS — objeto `04 01 01`
```
TX QUERY : 68 06 05 30 02 04 01 01 6F 10
RX DATA  : 70 0C 0A 31 82 04 01 01 00 00 00 00 64 00 DA 31
           valor = 100, en unidades de 0,0001 → 100 × 0,0001 = 0,01 $  ✔ confirmado contra menú
```

### 4.9 Apuesta máxima (candidato) — objeto `04 01 16`
```
TX QUERY : 78 06 05 30 02 04 01 16 30 72
RX DATA  : 40 0C 0A 31 82 04 01 16 00 00 00 4E 20 00 68 92
           valor = 20.000 (20.000 × 0,01 = 200 $) — sin confirmar contra menú
```

### 4.10 Bloque de identidad — objeto `03 01 00` (98 B de payload)
```
TX QUERY : 60 06 05 30 02 03 01 00 E4 7B
RX DATA  : 68 62 60 31 82 03 01 00 00 01 [ASCII + meters] ... 6C 5E
           └LEN┘         └eco─┘
           ASCII: "G1607" / "AH" / "Dolphin Treasure" / "87.928%"
           Tras el texto, meters embebidos BE32: ...Bet, Won, Games, 476.786...
Formatos: tabla de pagos = primera cadena ASCII (ej. "G1607")
          porcentaje base = cadena ASCII "NN.NNN%" (87,928 → 87,93)
```

### 4.11 Bloque multi-meter — objeto `02 03 03` (24 valores en UNA consulta)
```
TX QUERY : 48 06 05 30 02 02 03 03 E0 F8
RX DATA  : 50 68 66 31 82 02 03 03 00 [96 B = 24 × uint32 BE] 00 CRC
           └LEN=0x68┘          └─24 valores consecutivos─┘
           entrada i en posición 9 + 4·i ;  N entradas = (LEN − 8) / 4
Contenido observado: [01]=Current Credits, [02]=Games, [04]=Bet, [05]=Won,
           [13]=Handpays, [00]=315.532.800 (constante), familia drop en [08]/[15]
```

### 4.12 Tabla resumen de objetos

| Objeto (CL SM MT) | Contenido | Formato de respuesta |
|---|---|---|
| `02 03 01` | Current Credits | 16 B, BE32 |
| `02 04 04` | Total Games Played | 16 B, BE32 |
| `02 04 05` | Total Credits Bet | 16 B, BE32 |
| `02 04 06` | Total Credits Won | 16 B, BE32 |
| `02 05 03` | Cashout Handpays | 16 B, BE32 |
| `04 09 01` | True In (Coins In) | 16 B, BE32 |
| `04 04 01` | Coin Drop / Gross Drop | 16 B, BE32 |
| `04 01 01` | Denominación SAS (×0,0001 $) | 16 B, BE32 |
| `04 01 16` | ¿Apuesta máxima? (200 $) | 16 B, BE32 |
| `03 01 00` | Identidad: paytable/%/juego + meters | 102 B, ASCII + BE32 |
| `02 03 03` | Bloque 24 meters | 108 B, 24 × BE32 |
| `01 02 01` | Versión / serie ("01.04.003"/"000001") | ASCII |
| `01 02 02` | Fabricante/plataforma/paytable/ID/base code | 98 B, ASCII |
| `04 06 00` | Tabla de créditos/denominaciones | 220 B — ⚠ tarda 2 polls |

---

## 5. FORMATOS DE DATOS Y ANCHO VARIABLE

El ancho del valor depende del LEN de la DATA:

| LEN de la DATA | Ancho del valor | Interpretación |
|---|---|---|
| 0x08 (12 B total) | **16 bits** | uint16 BE en posiciones 9–10 |
| 0x0C (16 B total) | **32 bits** | uint32 BE en posiciones 9–12 (el caso estándar) |
| 0x0E | **64 bits** | uint64 BE |
| ≥ 0x16 | **multi-entrada** | N = (LEN−8)/4 valores; entrada i en pos 9+4·i |

**Dominio de los meters**: 8 dígitos decimales con **roll-over a 0** gestionado por la máquina (el BE32 llega siempre dentro de 0–99.999.999). Para consumir deltas entre lecturas: si `nuevo < anterior`, hubo vuelta → incremento real = `nuevo + 100.000.000 − anterior`.

**Denominación**: unidades de 0,0001 $ (100 = 0,01 $).
**Porcentaje**: cadena ASCII `"87.928%"`.
**Cadenas de identidad**: ASCII terminado, imprimibles 0x20–0x7E.

---

## 6. ANEXO — EJEMPLO DE CICLO COMPLETO REAL (log del ESP32)

```
[GAMMA] TX QUERY: 48 06 05 30 02 02 04 04 09 88      ← QUERY Games Played
[GAMMA] RX Datos: 48 00 84 65                          ← ACK (SEQ de la QUERY)
[GAMMA] TX POLL:  50 00 0E BF                          ← POLL slot siguiente
[GAMMA] RX Datos: 50 0C 0A 31 82 02 04 04 00 00 14 A8 4B 00 FF DC   ← DATA (SEQ del POLL)
[GAMMA] DATA 2 4 4 = 01353803
... (repetir por cada objeto de la tabla) ...
[GAMMA] == Ciclo de contadores completo ==
```

CRC de la QUERY de Games Played por slot (verificación rápida):
`0x40→17 52 · 0x48→09 88 · 0x50→2A E6 · 0x58→34 3C · 0x60→6C 3A · 0x68→72 E0 · 0x70→51 8E · 0x78→4F 54`

---

*Documento generado desde la integración real del módulo `GammaProLink` (firmware Globus IM). Toda trama citada fue capturada o recibida contra máquina real.*
