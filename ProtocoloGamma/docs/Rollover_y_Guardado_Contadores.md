# Rollover (fase SPC) y guardado de contadores en GammaProLink

**Fecha:** 22-09-2026
**Alcance:** cómo maneja el firmware las vueltas de contador (rollover) en el
enlace Gamma (tipo de máquina 19) y cómo queda guardado cada contador en la
interfaz (`Contadores_SAS`) desde que se consulta a la máquina hasta que viaja
al servidor.
**Fuentes:** `include/GammaProLink.h`, `src/GammaProLink.cpp`,
`include/Contadores.h`, `src/Contadores.cpp`, `src/Buffers_Cashless.cpp`,
`src/Buffers.cpp` y `ProtocoloGamma/docs/Analisis_SPC_Contadores.md`
(análisis de campo que cerró el diseño).

---

## 1. Los tres números de cada contador (el problema)

Para cada meter de vida hay tres valores distintos que no hay que mezclar:

| Nombre | Dónde vive | Dominio | Fórmula |
|---|---|---|---|
| **Verdadero (T)** | Dentro de la máquina (pantalla MAIN1) | 64 bits, crece sin límite | — |
| **Lectura Gamma (R)** | Lo que entrega el protocolo (objeto individual o bloque `02 03 03`) | 32 bits | `R = T mod 2³²` |
| **Valor SPC (S)** | Lo que viaja al sistema (campos de 8 dígitos) | 10⁸ | `S = T mod 10⁸` |

La máquina **descarta las vueltas de 2³²** (el cociente N de la división):
cada contador da la vuelta a su propio ritmo (en la máq. 3, Coin In llevaba
13 vueltas y Coin Out 11). El servidor, por su parte, reconstruye el total
absoluto acumulando los módulos de 10⁸ con sus **"Contadores rollover"**
(⌊T/10⁸⌋ por contador) y espera recibir **la misma fase de 8 dígitos que
enviaba la SPC**.

Desde R no se puede derivar S (la información de las vueltas no viaja en la
lectura — ver §6 del análisis). El firmware resuelve esa falta con una
**tabla N por contador**, calibrada una sola vez en la instalación.

## 2. La fórmula implementada

```
salida = (R + N × 2³²) mod 10⁸        (uint64 obligatorio: N × 2³² no cabe en 32 bits)
```

- **N = 0 (default):** máquina nueva / RAM clear → sale exacto sin calibrar.
- **Retrofit:** el comando de calibración carga los N una sola vez con los
  totales de la pantalla MAIN1.
- **Un N equivocado SOLO deja la fase corrida:** sumar una constante no cambia
  las diferencias → **los deltas/eventos jamás se rompen**.
- El `% 10⁸` adicional evita que un meter de 9+ dígitos (máq. 3: Coin Out
  2.287.693.744) quede truncado en sus primeros 8 dígitos por el `%08lu`.

Ejemplo verificado (máq. 3, Coin In): T = 56.678.524.000¢, N = 13,
R = 843.949.152 → `(843.949.152 + 13×4.294.967.296) mod 10⁸ = 78.524.000`
= exacto lo que enviaba la SPC.

## 3. Componentes del rollover en `GammaProLink`

### 3.1 Tabla N y persistencia (NVS)

| Qué | Dónde |
|---|---|
| Enum de índices `GAMMA_N_*` (7 contadores) | `include/GammaProLink.h:33-42` |
| Tabla en RAM: `N_vueltas[7]`, `R_previa[7]`, `R_valida[7]` | `include/GammaProLink.h:381-384` |
| Carga perezosa (lee NVS, crea defaults 0 si no existe la clave) | `Gamma_Cargar_N_Vueltas()` — `src/GammaProLink.cpp:224` |
| Persistencia write-through | `Gamma_Guardar_N_Vueltas()` — `src/GammaProLink.cpp:238` |

- **NVS:** namespace `"Gamma"`, clave `"N_Vueltas"` (7 bytes, uno por contador).
- Los 7 contadores con N: **Coin In, Coin Out, Drop (billetero), Cancelled,
  Jackpot, Coin Drop, Games**. Current Credits y referencias de configuración
  no llevan N (no son meters monotónicos de por vida).
- La carga es perezosa desde el primer meter o el primer Set del comando
  (el orden de construcción de los globales entre unidades es indefinido y
  NVS vive en `main.cpp`).
- `R_previa`/`R_valida` son solo RAM: tras un reboot la primera lectura
  simplemente siembra el estado del vigilante.

### 3.2 `Gamma_ProcesarMeter()` — punto único de salida

`src/GammaProLink.cpp:551`. Lo llaman las **dos vías** de cada contador
(meter individual y bloque `02 03 03`), y hace dos cosas:

1. **Vigilante de vuelta de 2³²:** si `r < R_previa` (la lectura cayó):
   - Caída desde un valor **pegado al tope** (`R_previa > GAMMA_UMBRAL_VUELTA_32`
     = 3.900.000.000) → es una vuelta REAL: `N++`, persiste en NVS
     (1 escritura cada ~$43M de juego) y avisa por serial.
   - Caída desde **rango medio** → NO es vuelta: RAM clear o box cambiado de
     máquina → avisa por serial, **N queda intacto** (recargar calibración).
   - Fundamento: entre dos lecturas del ciclo (segundos) la máquina no puede
     jugar $43M, así que una vuelta real solo puede verse como caída desde
     el tope.
2. **Salida en fase SPC:** `(uint32_t)(((uint64_t)r + (uint64_t)N × 2³²) % 10⁸)`.

Con `idx_n == GAMMA_IDX_SIN_N` (meter sin N asociado) solo aplica el
formateo `% 10⁸`.

### 3.3 Mapeo objeto → índice N

- `Gamma_Idx_N_Objeto(obj)` — vía individual (`src/GammaProLink.cpp:527`):
  `02 04 04`→Games, `02 04 05`→Coin In, `02 04 06`→Coin Out,
  `04 09 01`→Drop, `04 04 01`→Coin Drop.
- `Gamma_Idx_N_Bloque(entrada)` — bloque `02 03 03` (`src/GammaProLink.cpp:508`):
  `[9]`→Drop (fuente adaptable), `[13]`→Cancelled, `[17]`→Jackpot.
  Las entradas `[2]/[4]/[5]` del bloque devuelven `GAMMA_IDX_SIN_N`
  a propósito: en máq. 1 traían una **época distinta** del meter individual
  (mismo contador con offset) y el vigilante/N sigue **solo a la vía
  individual** — mezclar dos fuentes con offset disparaba el aviso de caída
  en cada ciclo.
- **DROP (caso especial):** el drop vive en el objeto individual `04 09 01`
  o en la entrada `[9]` del bloque según la población de máquina (regla
  adaptable `Gamma_DecidirFuenteDrop`). **Solo la fuente ACTIVA alimenta el
  vigilante/N**; la inactiva sale con `% 10⁸` simple y su `Set_Contadores`
  está gateado por la misma regla.

### 3.4 Calibración por pantalla (instalación / retrofit)

`Gamma_Calibrar_N_Desde_Pantalla()` — `src/GammaProLink.cpp:316`.
Entrada natural del comando: los **5 totales de vida de MAIN1 tal cual los
muestra la pantalla** (el operario no sabe si su máquina los muestra en
dólares —Mk6— o créditos —Mk5—). Fases:

1. **Lectura R vigente en todos los contadores pedidos** (si no: rechazo
   "esperar 1 ciclo de encuestas").
2. **Detección de unidad:** prueba CREDITOS (`T = pantalla`) y DOLARES
   (`T = pantalla × 100 / valor_del_credito` del objeto `02 01 05`, guardado
   BCD en `info_gamma[5]`); gana la interpretación que cierre contra las R
   reales en TODOS los contadores.
3. **N por contador:** `N = ⌊T / 2³²⌋`.
4. **Aplicar y persistir — ATÓMICA:** primero se validan los 5; si uno falla
   no se guarda nada (rollback en RAM si falla NVS).

Validación contra la última lectura R real con tolerancia
`GAMMA_CALIB_TOLERANCIA_C` = 10.000.000¢ ($100.000 de juego entre la lectura
de la pantalla y el comando):

| Resultado | Causa |
|---|---|
| `R_implicito > R + tol` | la máquina "retrocedió": typo u otra máquina |
| `R > R_implicito + tol` | la pantalla quedó vieja: releer y reenviar |

Propiedad de la validación: un typo chico (que no cruza el corchete de 2³²)
no cambia N y se absorbe en silencio; un typo que lo cruzaría desvía
~2³² enteros (>400× la tolerancia) y siempre se rechaza.

### 3.5 Auto-recalibración ante RAM clear

`Gamma_Verificar_RAMClear()` — `src/GammaProLink.cpp:597`. La llaman los dos
puntos de cierre de ciclo de `Encuestas_Maquinas_Gamma()`: al cerrar un ciclo
con comunicación viva, si **TODOS los contadores de dinero están en 0**
(lecturas válidas — los meters de vida jamás vuelven a 0, y menos los 5 a la
vez por fuentes distintas), la única explicación física es RAM clear o
máquina nueva → el total real ES 0 → pisa los N con 0, persiste, avisa y
**reinicia el ESP32** (re-siembra de las bases de detección de cambios).
One-shot: con N ya en 0 no dispara.

### 3.6 Reinicio tras calibración OK

El handler HTTP programa un reboot 2 s después en una **task aparte**
(`Buffers_Cashless.cpp:7666-7772`): los detectores de cambio (premio,
billete, máquina en juego) comparan contra bases en RAM sembradas con la
fase vieja — un salto de fase les dejaría de detectar o dispararía un evento
falso gigante. El reboot re-siembra todo con la fase nueva. (La espera NO
puede ir en el handler: corre en la task `async_tcp` y un `delay()` ahí
bloquea la propia respuesta — bug visto en campo.)

## 4. Flujo completo: de la consulta al guardado en la interfaz

```
Encuestas_Maquinas_Gamma()          (task de encuestas, 1 transacción por llamada)
  ├─ FASE_QUERY  → QUERY del meter actual de TABLA_METERS_GAMMA
  ├─ FASE_ESPERA → POLL ("¿ya?"); con DATA avanza de meter; sin DATA reintenta
  └─ FASE_RESYNC → polls del slot 0x40 (re-enganche del enlace)
                    │
                    ▼  (la DATA llega como respuesta al POLL del slot siguiente)
Gamma_ProcesarRx()                  (task UART: acumula bytes, valida CRC=0 y SEQ)
  └─ Gamma_DespacharTrama()         (src/GammaProLink.cpp:1335)
       ├─ valor BE32: trama[9..12] (individual) o trama[9+4e] (bloque)
       ├─ idx_n  = Gamma_Idx_N_Objeto() / Gamma_Idx_N_Bloque()   (+gateo DROP)
       ├─ salida = Gamma_ProcesarMeter(idx_n, valor)   ← vigilante 2³² + fase SPC
       ├─ snprintf(contador, 9, "%08lu", salida)       ← 8 dígitos ASCII + '\0'
       │                                                (byte-compatible con los
       │                                                 char[9] de Contadores_SAS)
       └─ contadores.Set_Contadores(Filtro, contador)  ← GUARDADO en la interfaz
```

### 4.1 El guardado en `Contadores_SAS` (la interfaz)

`Set_Contadores()` (`src/Contadores.cpp:447`) hace dos cosas por contador:

1. `Marcar_Respuesta(Filtro_Contador)` — marca inline del módulo de
   cumplimiento (Res. 20211200034224): "llegó respuesta de este contador".
2. `memcpy` del string de 9 bytes (8 dígitos + terminador) al buffer
   `char[9]` del contador correspondiente. **Los contadores viven en RAM**
   (no se persisten a SD/NVS desde este camino): son la interfaz viva de la
   que se sirve la trama al servidor (`Buffers.cpp` los lee con
   `Get_Contadores_Char()` y los copia a los campos de la trama).

Mapeo activo (vía individual — `src/GammaProLink.cpp:1879-1940`):

| Objeto Gamma | Contador(s) en `Contadores_SAS` | Extras que se llaman juntos |
|---|---|---|
| `02 04 04` Total Games Played | `Games_Played` | — |
| `02 04 05` Total Credits Bet | `Coin_In` | — |
| `02 04 06` Total Credits Won | `Coin_Out` | — |
| `02 05 03` (premio keyado) | `Cancel_Credit_Hand_Pay` + `Total_Cancel_Credit` | `Change_Counters` (subida → `Flag_Premio`) + `Ajusta_Base_Vuelta_Counters` |
| `04 09 01` True In (si es la fuente activa del drop) | `Total_Drop` + `Bill_Amount` | `Change_Counters_Bill_In` (subida → `Flag_Bill_In`) + `Ajusta_Base_Vuelta_Bill_In` |
| `04 04 01` Coin Drop / Gross Drop | `Total_Coin_Drop` | — |
| `02 03 01` Current Credits | `Current_Credits` | — |
| `06 02 01` EGM Doors | `Door_Open` (aperturas BE16 de `trama[10..11]`) | transición del bit → evento 0x11/0x12 |

Mapeo activo (bloque `02 03 03`, `src/GammaProLink.cpp:1478-1681`):

| Entrada | Contador | Nota |
|---|---|---|
| `[9]` Drop (si es la fuente activa) | `Total_Drop` + `Bill_Amount` + detección de billete | espejo del gateo individual |
| `[17]` | `Jackpot` | solo existe en el bloque |

Las entradas `[2]/[4]/[5]` del bloque tienen su `Set_Contadores` comentado:
el sistema se alimenta de los **individuales** (época verificada); activar
ambos haría oscilar el contador cuando hay offset.

### 4.2 La otra vuelta: 10⁸ del lado de la interfaz

La salida en fase SPC **envuelve cada 10⁸** (~$1M jugado). Los detectores de
cambio (`Change_Counters` / `Change_Counters_Bill_In`, `src/Contadores.cpp:1828`
y `:1865`) comparan contra bases en RAM (`Last_Cancel_Credit`,
`Last_Bill_In`): tras una vuelta de 10⁸ el valor queda por **debajo** de la
base y dejarían de detectar hasta recuperarla. Por eso `GammaProLink` llama
junto a cada guardado:

- `Ajusta_Base_Vuelta_Counters()` (`src/Contadores.cpp:1910`) — si
  `actual < base`, re-siembra la base con el valor actual.
- `Ajusta_Base_Vuelta_Bill_In()` (`src/Contadores.cpp:1929`) — ídem para el
  billete.

No disparan eventos: solo re-siembran, y la próxima subida se detecta normal.

### 4.3 Reparto del trabajo (firmware vs servidor)

| Responsabilidad | Quién |
|---|---|
| Detectar la vuelta de 2³² y mantener N | **Firmware** (vigilante, NVS) |
| Calibrar N en la instalación | **Operador** (1 comando con los 5 totales de MAIN1) |
| Recalibrar ante RAM clear | **Firmware** (auto, one-shot) |
| Detectar la vuelta de 10⁸ en los detectores de cambio | **Firmware** (`Ajusta_Base_Vuelta_*`) |
| Reconstruir el total absoluto (⌊T/10⁸⌋ + fase) | **Servidor** (sus "Contadores rollover") |

El firmware **NO necesita acumulador** estilo SPC: entrega la fase y el
servidor hace el resto. Una fase corrida no es corregible con el formulario
del servidor (solo ajusta en pasos de $1M = 10⁸¢) — de ahí la importancia de
calibrar los N en retrofit.

## 5. Interfaz HTTP (solo tipo de máquina 19)

`src/Buffers_Cashless.cpp`:

### `POST /api/Gamma/Calibrar_Contadores` (:7687)

```json
{ "coin_in": 566785240, "coin_out": 495323340, "bill_in": 223348000,
  "cancel_credit": 151886100, "jackpot": 1010500 }
```
(5 totales de vida de MAIN1 en dólares, sin separadores.) Respuesta:
`IsSuccess`, `Message` (el porqué), `Contador` (si hubo rechazo),
`Unidad` (`"creditos"`/`"dolares"` detectada) y eco de la tabla `N` cargada.
Si fue OK, reboot diferido 2 s (ver §3.6).

### `GET /api/Gamma/Vueltas` (:7786)

- Tabla completa (7 contadores) o `?idx=0..6` individual.
- Cada entrada: `n` (vueltas cargadas), `r` (última lectura cruda),
  `t_centavos` (total implícito `T = r + n×2³²`).
  (`Gamma_Consultar_Vueltas()` — `src/GammaProLink.cpp:292`.)

## 6. Escenarios cerrados

| Escenario | Comportamiento |
|---|---|
| Máquina nueva / RAM clear | N=0 default → sale exacto de por vida, sin calibrar |
| Retrofit sobre máquina con vueltas | 5 números de MAIN1 una sola vez (comando HTTP/TCP) |
| Typo en la calibración | chico: se absorbe (N correcto igual); grande: rechazado |
| Vuelta futura de 2³² | N++ automático, persistido (aviso por serial) |
| Caída de lectura desde rango medio | aviso; N intacto; recargar calibración |
| RAM clear en campo | auto-detección (5 meters de dinero en 0) → N=0 + reboot |
| Swap de box / cambio de máquina | contabilidad intacta (deltas), aviso, recalibrar |
| N equivocado | solo fase corrida; deltas/eventos intactos |

## 7. Referencias rápidas

| Qué | Dónde |
|---|---|
| Diseño y evidencia de campo (T/R/S, imposibilidad matemática, ejemplos) | `ProtocoloGamma/docs/Analisis_SPC_Contadores.md` |
| Manual del protocolo (tramas, CRC, ciclo, bloque 02 03 03 §7.6) | `ProtocoloGamma/docs/Manual_Protocolo_Gamma.md` |
| Enum `GAMMA_N_*` y API pública de calibración/consulta | `include/GammaProLink.h:33-57, 217-255` |
| `Gamma_ProcesarMeter` (vigilante + fase) | `src/GammaProLink.cpp:551-582` |
| Calibración por pantalla | `src/GammaProLink.cpp:316-483` |
| Vigilante RAM clear | `src/GammaProLink.cpp:597-629` |
| Despachador (extracción + `Set_Contadores`) | `src/GammaProLink.cpp:1335-1957` |
| `Set_Contadores` (memcpy a los `char[9]`) | `src/Contadores.cpp:447` |
| `Change_Counters*` / `Ajusta_Base_Vuelta_*` | `src/Contadores.cpp:1828-1950` |
| Endpoints HTTP | `src/Buffers_Cashless.cpp:7687-7843` |
| Constantes (`GAMMA_UMBRAL_VUELTA_32`, `GAMMA_CALIB_TOLERANCIA_C`) | `src/GammaProLink.cpp:115, 124` |
