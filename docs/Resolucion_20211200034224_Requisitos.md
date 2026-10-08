# Resolución 20211200034224 de 2021 (Coljuegos) — Extracto de requisitos

> Fuente: `docs/01 - RESOLUCION 20211200034224 DE 2021.pdf` (32 páginas escaneadas).
> Transcripción asistida por OCR visual; verificar contra el PDF original antes de uso legal/certificatorio.

**Objeto (Art. 1):** establecer las condiciones de confiabilidad de las Máquinas Electrónicas Tragamonedas (MET) y su gradualidad.

**Confiabilidad (Art. 3):** cumplimiento de los Estándares Técnicos del Anexo "Requerimientos Técnicos de MET", verificado por laboratorio certificador acreditado por Coljuegos.

## Artículo 4 — Gradualidad

| Etapa | Vence (desde vigencia 29-nov-2021) | Requisitos mínimos |
|---|---|---|
| **I** | nov-2023 (vigente) | 1) Elementos electrónicos/electromecánicos/multipuesto. 2) Contadores electrónicos reportados **en línea vía SCLM**: 2.1 Coin in, 2.2 Coin out, 2.3 Attendant paid Jackpots, 2.4 Games Played, 2.5 Denominación base. Contador de crédito en créditos o pesos, mostrando siempre todo valor apostable/cobrable. |
| **II** | **nov-2026** | Además de Etapa I: 1) **Protocolo de comunicación digital propio (no pulsos), sin dispositivo entre la MET y los sistemas externos**. 2) Contadores digitales sin procesos análogos/mecánicos. 3) Contadores adicionales: 3.1 Attendant paid cancelled credits, 3.2 Physical coin in, 3.3 Physical coin out, 3.4 Bill in, 3.5 Ticket/Voucher in, 3.6 Ticket/Voucher out, 3.7 Machine paid external bonus, 3.8 Attendant paid external bonus, 3.9 Attendant paid progressive, 3.10 Machine paid progressive. *Parágrafo: sin TITO no aplican 3.5–3.8.* |
| **III** | nov-2031 | Certificación en Colombia por laboratorio acreditado del Anexo Técnico, **o** certificación en otra jurisdicción (Art. 6). |

Art. 5: cumplir la etapa vigente ⇒ tarifa del 12% (Art. 59 Ley 1955/2019); si no, tarifas del Art. 34 Ley 643/2001.

## Anexo Técnico (lo que certifica el laboratorio)

### 2. Hardware del gabinete
- Diseño seguro para jugador/operador/terceros; materiales resistentes, duraderos, no combustibles.
- Fuente de poder dentro del gabinete; requisitos eléctricos.
- **Medios de almacenamiento**: ante falla total de energía, garantizar operación de contadores ≥ **180 s** y preservación de registros (transcripción OCR aproximada: "5 registros"); retención del valor de TODOS los contadores ≥ **180 días** sin energía.
- Encendido/apagado sin riesgo; el apagado no puede afectar contadores ni registros.
- **Registro histórico de eventos (log)** de cambios de estado del elemento de juego.

### 3. Programa de juego
- **3.1 Generalidades**: todo evento de juego debe completarse y registrarse aun si la MET se apaga en cualquier fase. Pantalla debe mostrar: jugadas posibles, combinaciones ganadoras, premios por combinación (pesos o créditos), valor en pesos de lo aceptado, aviso de anulación de jugada/pago por mal funcionamiento.
- **3.2 RNG**: semilla no dependiente de valores anteriores, no constante, evento asíncrono; sin patrones/rachas; independiente de apuestas/resultados previos; **confiabilidad mínima 95% en pruebas estadísticas**; periodo > rango; "continuo en segundo plano" (no aplica si criptográfico); ante interrupción, siguiente dato función del último valor; escalamiento uniforme; protección contra interferencias. (Mecánicos: aislados, sensores de alteración.)

### 3.3 Contadores (lista a–o)
- Retención ≥ 180 días sin energía; rollover a 0 al llegar al máximo; etiquetados claramente.
- **a)** Total de entradas (coin in) · **b)** Total de salidas (coin out) · **c)** Premio mayor pagado por asistente · **d)** Créditos cancelados pagados por asistente · **e)** Monedas físicas ingresadas · **f)** Monedas físicas entregadas · **g)** Billetes ingresados · **h)** Boletos/cupones ingresados · **i)** Boletos/cupones entregados · **j)** Bono externo pagado por máquina · **k)** Bono externo pagado por asistente · **l)** Progresivo pagado por asistente · **m)** Progresivo pagado por máquina · **n)** Denominación base (valor mínimo de un crédito) · **o)** Juegos jugados (desde reset de energía / cierre de puerta / inicialización).
- **Prohibido**: mecanismo para que un usuario no autorizado borre contadores, información de control o eventos.

### 3.4 Información de control (eventos significativos)
- Cada evento con: **fecha y hora, NUID de la MET, código exclusivo del evento, descripción breve**.
- No se aceptan códigos de error genéricos hacia sistemas externos.
- Eventos mínimos a recopilar y transmitir: **corrupción de RAM de la MET, cambio del juego, reinicio o falla de corriente**.

### 4./5. Locales con/sin restricción de menores
- Mismas especificaciones técnico-funcionales. Sin restricción de menores (5): no pago en efectivo, validación de tiquete, créditos desde/hacia **TDV**, sin alarmas audibles, luz superior de estados, **retorno teórico ≥ 83%**, premios ≤ **48 UVT** (progresivos interconectados ≤ **96 UVT**), TDV con acceso restringido, log de transacciones, información del operador/tiquetes/créditos a reportar a Coljuegos (la TDV no hace parte de la certificación de laboratorio).

### 6. Certificación
- Emitida por laboratorios avalados por Coljuegos; debe evidenciar el cumplimiento de **cada punto** del documento.

### 7. Bibliografía
- GLI-11 (Dispositivos de Juego), Standard Series v3.0, 21-sep-2016.

---

# CHECKLIST DE CERTIFICACIÓN MET (Res. 20211200034224)

> Uso: marcar cada ítem al auditar una máquina. "FW" = verifiable por este firmware
> (módulo Cumplimiento / trama al backend); "MET" = aplica al hardware/juego de la
> máquina (fabricante); "LAB" = solo lo puede certificar un laboratorio acreditado.
> Etapas: I vigente desde nov-2023 · II desde nov-2026 · III desde nov-2031 (o inmediata si se certifica el anexo completo).

## A. Gradualidad y reporte en línea (Art. 4 — operador)

- [ ] A.1 Contadores reportados en línea a Coljuegos vía SCLM (no lecturas manuales)
- [ ] A.2 **Etapa I**: coin in (2.1), coin out (2.2), attendant paid jackpots (2.3), games played (2.4), denominación base (2.5) — *FW: literales a), b), c), n), o) del módulo Cumplimiento*
- [ ] A.3 **Etapa II**: los 10 contadores adicionales 3.1–3.10 (cancelled credits, physical coin in/out, bill in, ticket in/out, bonos ×2, progresivos ×2) — *FW: literales d)–m)*
- [ ] A.4 **Etapa II**: protocolo de comunicación digital propio (no pulsos), sin intervención de dispositivos que procesen los contadores entre la MET y los sistemas externos — *FW: SAS/Gamma son digitales; para la resolución la interfaz es el DISPOSITIVO RECOLECTOR de la información de la MET (Art. 2 y glosario del anexo: "Protocolo de comunicación de la MET: dispositivo electrónico o componente de software que permite la recolección de la información en la MET"). Recolecta por el protocolo propio de la máquina y reenvía SIN procesar ni alterar contadores — no es un dispositivo intermedio en el sentido de la cláusula*
- [ ] A.5 **Etapa II**: contadores digitales sin procesos análogos o mecánicos — *FW: veto automático a tipos mecánicos 9/15*
- [ ] A.6 Contador de crédito en créditos o pesos colombianos, mostrando siempre todo valor apostable/cobrable — MET
- [ ] A.7 Si la MET no tiene TITO: exenta de 3.5–3.8 (tickets y bonos) — documentarlo
- [ ] A.8 Modelo inscrito ante Coljuegos (si ya cumple el anexo completo: sin gradualidad) — operador

## B. Hardware del gabinete (anexo §2 — MET/LAB)

- [ ] B.1 Gabinete seguro para jugador/operador/terceros; materiales resistentes, duraderos, no combustibles
- [ ] B.2 Fuente de poder dentro del gabinete, cumple requisitos eléctricos de la sección
- [ ] B.3 Ante falla total de energía: contadores operan ≥ 180 s y los registros se preservan
- [ ] B.4 **Valor de TODOS los contadores retenido ≥ 180 días sin energía** (batería/medio de soporte; el operador revisa periódicamente su estado)
- [ ] B.5 Encendido/apagado sin riesgo; el apagado total no afecta contadores ni registros
- [ ] B.6 Cables de energía y datos no accesibles al público
- [ ] B.7 Log histórico de eventos (cambios de estado del elemento de juego)

## C. Programa de juego (anexo §3.1 — MET/LAB)

- [ ] C.1 Todo evento de juego se completa y registra aunque la MET se apague en cualquier fase
- [ ] C.2 Pantalla muestra: jugadas posibles, combinaciones ganadoras, premio por combinación (pesos o créditos), valor en pesos de lo aceptado, aviso de anulación por mal funcionamiento
- [ ] C.3 RTP calculado incluyendo mecanismos de bonificación jugados en cada partida

## D. RNG (anexo §3.2 — LAB)

- [ ] D.1 Semilla no dependiente de valores anteriores, no constante, basada en evento asíncrono
- [ ] D.2 Sin patrones, rachas ni influencia externa (apuestas, resultados previos, estilo de juego)
- [ ] D.3 Confiabilidad ≥ 95% en pruebas estadísticas
- [ ] D.4 Periodo del generador > rango de valores generados
- [ ] D.5 Continuo en segundo plano a velocidad no determinable (excepto RNG criptográfico)
- [ ] D.6 Tras interrupción: siguiente dato función del último valor previo
- [ ] D.7 Escalamiento a rangos menores con probabilidad uniforme; valores fuera de rango descartables
- [ ] D.8 Protegido de interferencias electromagnéticas/radio (mecánicos: aislado + sensores de alteración)

## E. Contadores electrónicos (anexo §3.3 — FW + MET)

- [ ] E.a Total de entradas (coin in) — *FW: `Coin_In`*
- [ ] E.b Total de salidas (coin out) — *FW: `Coin_Out`*
- [ ] E.c Premio mayor pagado por asistente — *FW: `Jackpot`*
- [ ] E.d Créditos cancelados pagados por asistente — *FW: `Total_Cancel_Credit`/`Cancel_Credit_Hand_Pay`*
- [ ] E.e Monedas físicas ingresadas — *FW: `Physical_Coin_In`*
- [ ] E.f Monedas físicas entregadas — *FW: `Physical_Coin_Out`*
- [ ] E.g Billetes ingresados — *FW: `Bill_Amount`/`Total_Drop`*
- [ ] E.h Boletos/cupones ingresados (ticket in) — *FW: `Ticket_In`* (N/A sin TITO)
- [ ] E.i Boletos/cupones entregados (ticket out) — *FW: `Ticket_Out`* (N/A sin TITO)
- [ ] E.j Bono externo pagado por la máquina — *FW: `Machine_Paid_External_Bonus_Payout`* (N/A sin TITO)
- [ ] E.k Bono externo pagado por asistente — *FW: `Attendant_Paid_External_Bonus_Payout`* (N/A sin TITO)
- [ ] E.l Progresivo pagado por asistente — *FW: `Attendant_Paid_Progresive_Payout`*
- [ ] E.m Progresivo pagado por la máquina — *FW: `Machine_Paid_Progresive_Payout`*
- [ ] E.n Denominación base / valor mínimo de un crédito — *FW: `Informacion_Maquina` (bloque 0x1F)*
- [ ] E.o Juegos jugados (total, desde reset de energía, desde cierre de puerta, desde inicialización) — *FW: `Games_Played` + `Games_Since_Last_Power_Up`*
- [ ] E.1 Contadores etiquetados con claridad según su función
- [ ] E.2 Rollover a 0 al alcanzar el máximo (no se detienen ni se corrompen)
- [ ] E.3 **Sin mecanismo para que un usuario no autorizado borre contadores, información de control o eventos** — MET + FW (el firmware no expone borrado de contadores; el acta NVS solo se borra por cambio de tipo/máquina)

## F. Información de control / eventos significativos (anexo §3.4 — FW)

- [ ] F.1 Cada evento transmitido con: fecha y hora, NUID de la MET, código exclusivo del evento, descripción breve — *FW: trama de eventos con RTC sincronizado por API + identidad de máquina*
- [ ] F.2 Prohibido enviar eventos como código de error genérico — *FW: `Tabla_Eventos.h` mapea cada código SAS a descripción*
- [ ] F.3 Evento de corrupción de RAM de la MET — *FW: 0x31/0x32/0x33/0x3A*
- [ ] F.4 Evento de cambio del juego — *FW: 0x8C (verificar variante por marca en producción)*
- [ ] F.5 Evento de reinicio o falla de corriente — *FW: 0x17/0x18*

## G. Locales sin restricción de acceso a menores (anexo §5 — MET/operador)

- [ ] G.1 No pago de dinero en efectivo directo por la máquina
- [ ] G.2 Validación de tiquete antes de iniciar sesión; créditos desde/hacia la TDV
- [ ] G.3 Sin alarmas audibles; solo luz superior de estados (premio no pagable, error, llamada al asistente)
- [ ] G.4 Retorno teórico al jugador ≥ 83%
- [ ] G.5 Premios ≤ 48 UVT; progresivos interconectados ≤ 96 UVT
- [ ] G.6 TDV con acceso restringido por contraseña, valida y paga tiquetes, log de transacciones, consulta en línea por Coljuegos
- [ ] G.7 Información TDV (operador, tiquetes, créditos) disponible para reporte a Coljuegos (no hace parte de la certificación de laboratorio)

## H. Certificación (anexo §6 — LAB/operador)

- [ ] H.1 Certificado emitido por laboratorio acreditado/avalado por Coljuegos que evidencie el cumplimiento de **cada punto** del anexo
- [ ] H.2 (Alternativa Etapa III) certificación en otra jurisdicción conforme al Art. 6 de la resolución

---

## Notas internas (NO son requisitos de la resolución)

- **Acta del módulo Cumplimiento** (`Snap_Acta` en NVS namespace "Certif" + `GET /api/Cumplimiento/Acta`): herramienta interna de verificación de campo/banco. Sirve como evidencia propia de que la máquina responde los literales a)–o) y como insumo del futuro envío al backend, pero **la resolución no la exige** — lo exigible es el reporte SCLM (A.1–A.5), los eventos 3.4 (F.*) y el certificado de laboratorio (H.1/H.2).
- **A.4 — La interfaz ES el dispositivo recolector (interpretación del equipo)**: para la resolución, la interfaz Globus es el **dispositivo recolector** de la información de la MET — figura definida por la propia norma en el Art. 2 y en el glosario del anexo técnico: *"Protocolo de comunicación de la MET: dispositivo electrónico o componente de software que permite la recolección de la información en la MET"*. En ese rol, recolecta los contadores directamente por el protocolo propio de la máquina (SAS/Gamma) y los reenvía tal cual al sistema, **sin procesarlos ni alterarlos**. La cláusula de la Etapa II ("sin la intervención de ningún dispositivo entre la MET y los sistemas externos") apunta a otra cosa: un dispositivo que **procesa o recalcula** los contadores y del que la interfaz obtendría la información (agregadores, conversores de pulsos, cajas intermedias) — escenario que la interfaz no reproduce. Coherente con el anexo §3.3/§3.4, que exige transmitir contadores y eventos *"a través del protocolo o forma de comunicación en la MET"*, que es exactamente lo que hace la interfaz.
