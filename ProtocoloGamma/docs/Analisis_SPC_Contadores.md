# Análisis: Qué hace la SPC con los contadores

**Fecha:** 04-09-2026
**Máquina de verificación:** máq. 3 (Mk6, Queen of Sheba, coin $10 / crédito $0,01)
**Fuentes cruzadas:**
- Test Gamma por PC (GUI v2.4, sesión 10:14–10:22)
- Pantalla de la máquina: "CURRENT METERS - MAIN1" y "- OTHER" (fotos Multimedia (1)/(4))
- Trama real SPC→sistema capturada a las 11:23:39 (IP tarjeta 192.168.5.124)

**Conclusión en una frase:** la SPC mantiene un acumulado propio del valor
verdadero de cada contador (construido desde que se instaló, época en la que
los contadores de la máquina eran jóvenes) y al sistema le envía
**los últimos 8 dígitos de ese acumulado** (`acumulado mod 100.000.000`).

---

## 1. Los tres números de cada contador

Para cada contador hay tres valores distintos que conviene no mezclar:

| Nombre | Dónde vive | Dominio |
|---|---|---|
| **Verdadero (T)** | Dentro de la máquina (lo muestra la pantalla MAIN1) | 64 bits, crece sin límite |
| **Lectura Gamma (R)** | Lo que entrega el protocolo (objeto individual o bloque 02 03 03) | 32 bits: `R = T mod 2³²` |
| **Valor SPC (S)** | Lo que viaja al sistema (campos de 8 dígitos) | 10⁸: `S = T mod 10⁸` |

El meter que la máquina expone por Gamma **ya descartó las vueltas de 32 bits**
(el cociente de la división). El número de vueltas **N** de cada contador:

```
T = N × 4.294.967.296 + R          (división entera: N = ⌊T / 2³²⌋)
S = T mod 100.000.000
```

Cada contador da la vuelta **a su propio ritmo** (crecen a velocidades
distintas): en la máq. 3, Coin In llevaba 13 vueltas y Coin Out 11.

## 2. El mecanismo de la SPC (lógica de acumulación)

La SPC lee cada pocos segundos y suma diferencias a un acumulado en memoria
no volativa:

```
si lectura_nueva > lectura_anterior:
        delta = lectura_nueva − lectura_anterior              (juego normal)
si lectura_nueva < lectura_anterior:
        delta = lectura_nueva + 4.294.967.296 − lectura_anterior   (¡dio la vuelta!)

acumulado_SPC += delta                (guardado en NVM, sobrevive cortes)
```

- Entre dos lecturas suyas (segundos) la máquina no puede jugar $43M (una
  vuelta completa), así que la detección de vuelta **nunca falla**.
- Si la SPC se apaga horas o días, al volver el primer delta recupera todo lo
  jugado en su ausencia.
- Al enviar al sistema: `acumulado_SPC mod 100.000.000`, formateado con 8
  dígitos (`%08lu`).

## 3. Ejemplo verificado 1 — Coin In / Total Credits Bet

```
Verdadero (pantalla MAIN1):  $566.785.240,00 = 56.678.524.000¢

Paso 1 — la lectura Gamma es el verdadero menos las vueltas:
    13 vueltas × 4.294.967.296 = 55.834.574.848
    56.678.524.000 − 55.834.574.848 = 843.949.152      ← EXACTO lo del test ✓

Paso 2 — la SPC manda el verdadero con 8 dígitos:
    56.678.524.000 mod 100.000.000 = 78.524.000        ← EXACTO lo de la SPC ✓

Paso 3 — no existe transformación directa lectura→SPC:
    843.949.152 mod 100.000.000 = 43.949.152           ≠ 78.524.000
    cada vuelta aporta  4.294.967.296 mod 100.000.000 = 94.967.296
    13 vueltas × 94.967.296 = 1.234.574.848 → mod 10⁸ = 34.574.848
    43.949.152 + 34.574.848 = 78.524.000               ✓ cierra exacto
```

## 4. Ejemplo verificado 2 — Coin Out / Total Credits Won

```
Verdadero (pantalla MAIN1):  $495.323.340,00 = 49.532.334.000¢

Paso 1 — este contador dio 11 vueltas (no 13 — cada uno a su ritmo):
    11 vueltas × 4.294.967.296 = 47.244.640.256
    49.532.334.000 − 47.244.640.256 = 2.287.693.744    ← EXACTO lo del test ✓

Paso 2 —
    49.532.334.000 mod 100.000.000 = 32.334.000        ← EXACTO lo de la SPC ✓

Paso 3 —
    2.287.693.744 mod 100.000.000 = 87.693.744         ≠ 32.334.000
    11 vueltas × 94.967.296 = 1.044.640.256 → mod 10⁸ = 44.640.256
    87.693.744 + 44.640.256 = 132.334.000 → mod 10⁸ = 32.334.000  ✓ cierra exacto
```

## 5. Los cinco contadores verificados (máq. 3)

| Contador | Verdadero (pantalla) | N | Test Gamma (T mod 2³²) | SPC (T mod 10⁸) |
|---|---|---|---|---|
| Coin In (Bet) | 56.678.524.000 | 13 | 843.949.152 | 78.524.000 |
| Coin Out (Won) | 49.532.334.000 | 11 | 2.287.693.744 | 32.334.000 |
| Bill In / Gross Drop | 22.334.800.000 | 5 | 859.963.520 | 34.800.000 |
| Attendant Paid Cancel Cr. | 15.188.610.000 | 3 | 2.303.708.112 | 88.610.000 |
| Attendant Paid Jackpots | 101.050.000 | 0 | 101.050.000 | 01.050.000 |

Directos (sin vuelta posible, idénticos en test y SPC):
Games = 03381566, Door openings = 00001552 (y re-valida que el cierre de
puerta no suma: abierta a las 10:22 con 1552 → SPC reporta 1552 a las 11:23),
Current credits = 0, Physical coin in/out = 0, progressive/bonus = 0.

Curiosidad de la propia máquina: la página "OTHER" muestra Gross Drop
$8.599.635,20 (el valor **envuelto** de 32 bits, el mismo del test) mientras
MAIN1 muestra Bill In $223.348.000 (el total de 64 bits) — la "contradicción"
test↔pantalla existe dentro de la propia máquina.

## 6. Por qué NO existe transformación directa (imposibilidad matemática)

Desde la lectura `R` no se puede derivar `S`: la información de las vueltas
no viaja en la lectura. La misma lectura es compatible con
**390.625 valores distintos de S** (los T = R + k·2³² con k ≥ 0 producen
residuos mod 10⁸ que difieren). Conocer el máximo (2³²−1) no ayuda: el máximo
dice *cuándo* da la vuelta, no *cuántas* ya dio. Acotar por edad/ritmo de la
máquina tampoco: al ritmo teórico máximo (apuesta $4.000 × 2 juegos/s) el
contador daría una vuelta completa en ~1,5 horas, así que cualquier cota por
tiempo es inútilmente ancha. La contabilidad cruzada entre meters
(CI = CO + BI − APCC) se cumple módulo 2³² automáticamente y no revela las N
individuales.

La información faltante (bits altos / cociente N) solo puede venir de tres
lugares:

1. **Más bits desde la máquina** — un objeto Gamma de 64 bits (tipo BE64,
   T=0C) que cargue el total. Pendiente de confirmar: decodificar las QUERY
   de la SPC en la captura del 04-09, o barrer las colas de familias no
   cubiertas (02 02 P29, 04 01 P24, 04 06 P36, y las nuevas de Mk6:
   02 06/07, 04 0D/0E, 05 15-1C, 80 03).
2. **Historia** — acumular deltas contando las vueltas mientras pasan
   (lo que hace la SPC).
3. **Conocimiento externo una sola vez** — pantalla o última trama SPC
   (calibración).

## 7. Por qué la SPC no necesita calibración inicial

Su acumulado coincide con el total de la máquina **al dígito** → su época y
la época de los contadores empezaron juntas: la SPC se instala con la máquina
nueva o tras un RAM clear, con los contadores en 00000000, y desde ahí ambos
crecen en paralelo. **El procedimiento de instalación ES la calibración.**

## 8. Consecuencia para el firmware Globus (diseño acordado)

La frontera exacta: mientras un contador **no haya dado su primera vuelta de
32 bits** (lectura < 4.294.967.296), `lectura % 10⁸` = `T mod 10⁸` = el valor
exacto de la SPC. Las vueltas de 10⁸ no separan a nadie (la SPC también
envuelve ahí, cada $1M jugado, en los mismos puntos). El instante
`4.294.967.296` es el "momento del disfraz": el contador vuelve a 00000000 y
se lee igual que uno recién nacido — ahí se pierde la referencia.

| Escenario de instalación | Configuración | Resultado de `% 10⁸` |
|---|---|---|
| Máquina nueva / RAM clear / contador < 2³² | nada | **EXACTO** (= SPC) desde el día 1 |
| Retrofit sobre máquina con vueltas (máq. 3) | N por contador (opcional) | Deltas/eventos perfectos; fase corrida salvo que se carguen los N |
| Cualquiera, cuando el contador llegue a 2³² a futuro | vigilante de vuelta (opcional) | Un salto cosmético de −94.967.296 (el servidor lo interpreta como vuelta más) |

**Opciones de diseño (menú final):**

| | `% 10⁸` puro | N calibrado | Acumulador |
|---|---|---|---|
| Estado | ninguno | 1 número chico por contador en NVS | deltas + guardas en RAM |
| Número exacto SPC | no (fase corrida) | **sí** (mientras N vigente) | sí |
| Si se equivoca / swap | fase corrida | fase corrida (deltas intactos) | re-siembra |
| Mantenimiento | cero | recalibrar tras vuelta de 2³² (~años) o swap | ninguno |

El **N calibrado** usa la fórmula inversa: `salida = (R + N × 2³²) mod 10⁸`.
N se calcula con UN par (última trama SPC, lectura Gamma actual) — p. ej. con
los datos de la máq. 3: `13 × 94.967.296 + 43.949.152 ≡ 78.524.000` → N = 13.
Un N equivocado **jamás rompe los deltas** (sumar una constante no cambia las
diferencias): degrada a "fase corrida", nunca a corrupción.

## 9. Mapeo trama SPC ↔ fuente Gamma (máq. 3, verificado)

| Campo trama SPC | Fuente Gamma | Valor del 04-09 |
|---|---|---|
| Total cancelled credits | bloque 02 03 03 **[13]** | 88.610.000 |
| Coin in | **[4]** / 02 04 05 | 78.524.000 |
| Coin out | **[5]** / 02 04 06 | 32.334.000 |
| Total drop (billetero) | **[9]** | 34.800.000 |
| Jackpot (pago manual) | **[17]** | 01.050.000 |
| Physical coin in / out | (hopper — $0 en máq. 3) | 0 |
| Total coin drop | [9] + physical coin in | 34.800.000 |
| Progressive / external bonus (×4) | MAIN2 (todo $0) | 0 |
| Ticket voucher in (10 dígitos) | **sin fuente Gamma aún** | 9.023.255.552 |
| Ticket voucher out (10 dígitos) | candidato `02 05 03` (Split Voucher Pays) | 0 |
| Games since power up | fuente por hallar | 0 |
| Total games played | **[2]** / 02 04 04 | 03381566 |
| Door openings | `06 02 01` (bits bajos) | 00001552 |
| Current credits | **[1]** / 02 03 01 | 0 |
| Firma máquina (987) / serie trama / IDs | sin fuente / propios | — |

Nota: `02 05 03` NO es "Cashout Handpays" — y el dato de campo (07-09) mostró
algo más interesante: **en la máq. 3 el individual `02 05 03` lee
2.303.708.112 = exactamente el `[13]`** → son el MISMO meter por dos caminos,
pero su **identidad cambia por máquina**: en la máq. 1 es Split Voucher Pays
(~8,08M creciendo +164 con handpays en 0 en pantalla) y en la máq. 3 es el
Attendant Paid Cancel Credits (2,3B con Split Pays $0). El "Cashout
Handpays" del manual no describe a ninguna. Consecuencia: el campo "cancel"
de la trama NO puede alimentarse uniformemente desde 02 05 03/[13].
Verificación pendiente en máq. 1: `GET /api/Gamma/Vueltas?idx=3` → si r ≈
8,08M confirma [13]=Split Voucher también ahí; si r ≈ 0, [13]=cancel y
02 05 03 sería otro meter.

## 10. Plan de desarrollo (firmware)

En `GammaProLink.cpp`:

1. **Módulo 10⁸**: `valor %= 100000000UL` antes de los dos `snprintf`
   (vía individual ~línea 1249 y bloque 02 03 03 ~línea 885) — resuelve el
   truncado de 9+ dígitos y reproduce el formato SPC.
   **✅ IMPLEMENTADO 07-09-2026, extendido a FASE SPC completa** (ver §12):
   `salida = (R + N×2³²) mod 10⁸` en ambos puntos vía `Gamma_ProcesarMeter()`.
2. **Jackpot**: case 17 del bloque → `Set_Contadores(Jackpot, contador)`.
   **✅ IMPLEMENTADO** (case activo).
3. **Door_Open**: en el handler de `06 02 01`, `Set_Contadores(Door_Open, ...)`
   con las aperturas (BE16 en trama[10..11]).
   **✅ IMPLEMENTADO 07-09** (aperturas → campo de la trama, formato 8 dígitos).
4. **Fix puerta Mk6**: mover el decode de `06 02 01` ANTES de la bifurcación
   `len != 0x0C` (la máq. 3 manda la DATA con LEN 0x0C — hoy el detector nunca
   dispararía ahí; las posiciones trama[9]/trama[10..11] son idénticas en
   ambos formatos).
   **✅ IMPLEMENTADO 07-09** (handler único antes de la bifurcación, cubre
   LEN 0x0A Mk5 y LEN 0x0C Mk6; eventos 0x11/0x12 por transición, sin evento
   de estado inicial por decisión del usuario).
5. **Cancelled ← [13]**: activar en case 13
   `Cancel_Credit_Hand_Pay` + `Total_Cancel_Credit` (hoy los alimenta
   `02 05 03` = ticket-out). **Decisión pendiente del usuario.**
6. ~~N por contador en NVS (default 0) + vigilante de vuelta en RAM~~
   **✅ IMPLEMENTADO 07-09-2026** (ver §12).

No se toca: ciclo de encuestas, re-enganche, fuente adaptable del drop,
`Change_Counters`/premio (sigue en `02 05 03`), camino SAS completo.

## 11. Pendientes abiertos relacionados

- Decodificar las QUERY de la SPC en la captura "104954" (¿existe objeto
  BE64 con el total? → resolvería la fase sin acumulador ni N).
- Barrido dirigido en máq. 3 de las colas de familias sin cubrir (buscar
  T=0C con valores ~10¹⁰-10¹¹).
- **Ticket voucher in (campo de 10 dígitos, máq. 3: 9.023.255.552)**:
  aritmética = 2 vueltas de 2³² + **433.320.960** (≈$4,3M redimidos — si hay
  meter, hoy lee eso; el campo de 10 dígitos admite el total hasta 10¹⁰ sin
  envolver). Prueba: barrir `04 0C` (Vouchers P22) en máq. 3 buscando
  433.320.960 o un BE64 con ~9.023.255.552. Alternativa si está en cero:
  contador PROPIO de la SPC (validaciones que ella tramita) → sin fuente
  Gamma posible, decisión de negocio (MAIN1 muestra Voucher In $0). Si se
  conecta: fase SPC con módulo 10¹⁰ y N=2.
- Fuente de "firma 987",
  games-since-power-up, objetos [0] (02 02 04) y [3].
- Decisión de negocio: premio keyado en ticket-out (02 05 03) vs handpays
  de asistente ([13]/[17]).

## 12. Implementación en el firmware (07-09-2026)

**Hallazgo que cerró el diseño (captura del servidor):** el servidor ya
reconstruye el total absoluto por su cuenta — mantiene los "Contadores
rollover" (⌊T/10⁸⌋ por contador, verificados exactos en los 5 de la máq. 3)
y suma la fase de 8 dígitos que le llega. Por eso el firmware NO necesita
acumulador: debe entregar la **fase SPC** (`S = T mod 10⁸`) para que los
rollover del servidor sigan válidos. Una fase corrida no es corregible con
el formulario del servidor (solo ajusta en pasos de $1M = 10⁸¢).

**Implementado en `GammaProLink` (.h/.cpp):**

- `Gamma_ProcesarMeter(idx_N, R)` — punto único de salida de todo meter:
  vigilante de vuelta de 2³² + `salida = (R + N×2³²) mod 10⁸` (uint64).
  Aplicado en los dos `snprintf` (bloque 02 03 03 y vía individual).
- Tabla N de 7 contadores (enum `GAMMA_N_*` en el header): Coin In, Coin Out,
  Drop, Cancelled, Jackpot, Coin Drop, Games. Current Credits y referencias
  de configuración sin N (no son meters monotónicos de por vida).
- NVS: namespace `"Gamma"`, clave `"N_Vueltas"` (7 bytes); defaults 0 creados
  en el primer arranque (carga perezosa desde el primer meter o el primer Set).
- **Comando TCP (API lista para cablear en AsyncTCP):**
  - `Gamma_Calibrar_N_Desde_Pantalla(coin_in, coin_out, bill_in, cancel, jackpot, &idx, &cred)`
    — **entrada natural del comando**: recibe los 5 totales de la pantalla
    **el número tal cual lo muestra** (el operario no sabe ni necesita saber
    si su máquina la muestra en dólares —Mk6/MAIN1— o en créditos —Mk5—).
    El firmware prueba ambas interpretaciones contra las lecturas R reales
    y gana la que cierre en TODOS los contadores: CREDITOS (T = pantalla,
    probada primero — un crédito tipeado pegado al tope de 2³² puede cerrar
    por casualidad como dólares; el falso positivo inverso solo ocurre con
    números chicos donde ambas dan N=0) o DOLARES (T = pantalla×100/valor¢,
    con el valor del crédito del `02 01 05` — soporta denominaciones ≠
    $0,01). Calcula N = ⌊T/2³²⌋, valida (tolerancia 10.000.000¢ = $100.000)
    y guarda. **Atómica**: si un contador falla no se guarda nada. Devuelve
    `Gamma_Calib_Res` + índice del rechazado + unidad detectada (la respuesta
    HTTP incluye `"Unidad": "creditos"/"dolares"`).
    Propiedad: un typo que no cruza el corchete de 2³² no cambia N (se absorbe
    — el N guardado es siempre el correcto); uno que lo cruzaría desvía
    ~2³² enteros y se rechaza siempre. Verificado con máq. 3: los 5 valores
    de MAIN1 → N = 13/11/5/3/0 → salidas 78524000/32334000/34800000/
    88610000/01050000 = SPC exacta (incluso con un traspuesto de dígitos en
    Coin In, que dio desvío +18.000¢ ≤ tolerancia y N=13 correcto).
  - `Gamma_Consultar_Vueltas(idx, &n, &r, &t)` — consulta individual: N
    cargado, última lectura cruda y total implícito T = R + N×2³²
  - `Gamma_Set_N_Vueltas(valores, GAMMA_N_TOTAL)` / `Gamma_Set_N_Vuelta(idx, n)`
    — carga directa de N ya calculados (alternativa si el servidor prefiere
    hacer la división); `Gamma_Get_N_Vueltas(idx)` — lectura/eco
- **Endpoints HTTP ya cableados en `Init_API_Server` (Buffers_Cashless.cpp,
  07-09-2026, solo tipo de máquina 19):**
  - `POST /api/Gamma/Calibrar_Contadores` — body JSON con los 5 totales de
    MAIN1 en dólares: `{"coin_in":…, "coin_out":…, "bill_in":…,
    "cancel_credit":…, "jackpot":…}` → responde
    `{"IsSuccess", "Message" (el porqué), "Contador" (si hubo rechazo),
    "N": {tabla de 7}}`
  - `GET /api/Gamma/Vueltas` — tabla completa; `?idx=0..6` — consulta
    individual: `{"n", "r", "t_centavos"}` (vueltas, última lectura cruda,
    total implícito = r + n×2³²)
- Vigilante con guard `GAMMA_UMBRAL_VUELTA_32` (3,9e9): una caída solo cuenta
  como vuelta si la lectura previa estaba pegada al tope; una caída desde
  rango medio (RAM clear / box cambiado de máquina) avisa por serial y no
  toca N (recargar calibración).
- **Auto-recalibración ante RAM clear** (07-09, idea del usuario): al cerrar
  cada ciclo con comunicación viva, si TODOS los contadores de dinero están
  en 0 (lecturas válidas — los meters de vida jamás vuelven a 0, y menos los
  5 a la vez por fuentes distintas), la única explicación es RAM clear o
  máquina nueva → el total real ES 0 → `Gamma_Verificar_RAMClear()` pisa los
  N con 0, persiste y avisa. One-shot: con N ya en 0 no dispara.

**Escenarios cerrados** (ver discusión §8): máquina nueva → default 0 exacto
de por vida; retrofit → 5 números de MAIN1 una sola vez (por comando TCP);
swap de box / RAM clear → contabilidad intacta, aviso y recalibración;
vuelta futura de 2³² → N++ automático y persistido.
