# FICHA DE EVALUACIÓN DE CUMPLIMIENTO MET
### Resolución Coljuegos 20211200034224 de 2021 — uso interno del equipo

> **Cómo usar**: una ficha por máquina. Marcar el resultado de cada ítem con:
> **C** = Cumple · **NC** = No cumple · **NA** = No aplica (justificar) · **PV** = Pendiente verificar
> La columna **Exigencia** indica desde cuándo el ítem es obligatorio: **E1** = Etapa I (vigente) · **E2** = Etapa II (29-nov-2026) · **E3** = Etapa III / certificación plena.
> Un ítem E1 en NC hoy ya es incumplimiento. Un ítem E2 en NC hoy es **alerta** (queda exigible el 29-nov-2026).

---

## A. IDENTIFICACIÓN DE LA MÁQUINA

| Campo | Dato |
|---|---|
| Fecha de evaluación | |
| Evaluador | |
| Operador / contrato | |
| Local / ciudad | |
| Marca y modelo | |
| Tipo de máquina en firmware (Tipo_Maquina) | |
| NUC (Coljuegos) | |
| Serial de máquina (SAS 0x54) | |
| Versión SAS | |
| Denominación base (bloque 0x1F, byte 5) | |
| ¿Tiene TITO? (SÍ/NO) | |
| ¿Tiene aceptador de monedas? (SÍ/NO) | |
| Id máquina en backend / IP del recolector | |

---

## B. CONTADORES (anexo §3.3)

> **Cómo verificar**: arrancar la máquina con el recolector conectado y esperar el cierre automático de la certificación (monitor serial, tabla "CERTIFICACION DE CONTADORES") o consultar `GET http://<ip-recolector>/api/Cumplimiento/Acta`.
> Cada fila debe salir **OK con P/R ≥ 1/1** y valor. Filas en NO RESPONDE = la máquina no entrega ese contador.

| # | Contador | Exigencia | Si la máquina no lo tiene | Res. | Observaciones |
|---|---|---|---|---|---|
| a | Coin in (total entradas) | **E1** | NC — exigible siempre | | |
| b | Coin out (total salidas) | **E1** | NC — exigible siempre | | |
| c | Premio mayor pagado por asistente (jackpot) | **E1** | NC — exigible siempre | | |
| n | Denominación base / valor mínimo crédito | **E1** | NC — exigible siempre | | |
| o | Juegos jugados (total y desde reset energía) | **E1** | NC — exigible siempre | | |
| d | Créditos cancelados pagados por asistente | **E2** | NC desde nov-2026 | | |
| e | Monedas físicas ingresadas | **E2** | NC desde nov-2026 (aunque no acepte monedas, el contador debe existir y reportar 0) | | |
| f | Monedas físicas entregadas | **E2** | NC desde nov-2026 (ídem) | | |
| g | Billetes ingresados | **E2** | NC desde nov-2026 | | |
| l | Progresivo pagado por asistente | **E2** | NC desde nov-2026 | | |
| m | Progresivo pagado por máquina | **E2** | NC desde nov-2026 | | |
| h | Ticket in | **E2** | **NA legítimo solo si NO tiene TITO** (parágrafo Art. 4) | | |
| i | Ticket out | **E2** | **NA legítimo solo si NO tiene TITO** | | |
| j | Bono externo pagado por máquina | **E2** | **NA legítimo solo si NO tiene TITO** | | |
| k | Bono externo pagado por asistente | **E2** | **NA legítimo solo si NO tiene TITO** | | |

**Controles adicionales de contadores:**

| # | Requisito | Exigencia | Cómo verificar | Res. |
|---|---|---|---|---|
| B.1 | Contadores etiquetados con claridad por función | E3 | Trama/acta: cada valor corresponde a su campo | |
| B.2 | Rollover a 0 al llegar al máximo (no se truncan ni corrompen) | E3 | Ficha técnica del fabricante | |
| B.3 | Retención del valor ≥ 180 días sin energía | E3 | Ficha técnica / batería de la máquina | |
| B.4 | Imposible borrar contadores por usuario no autorizado | E3 | Probar menús de operador; el recolector no expone borrado | |
| B.5 | Los meters no consultados por la cadena NO deben viajar al backend como si fueran 0 real | Interna | Revisar log `ENVIO_TRAMA_CONTADORES_API` vs meters consultados | |

---

## C. PROTOCOLO DE COMUNICACIÓN Y REPORTE EN LÍNEA (Art. 4)

> El recolector Globus es el **dispositivo recolector** de la información de la MET (def. Art. 2). No es "dispositivo intermedio": consulta por el protocolo propio de la máquina y reenvía sin procesar ni alterar contadores.

| # | Requisito | Exigencia | Cómo verificar | Res. |
|---|---|---|---|---|
| C.1 | Comunicación digital por protocolo propio de la máquina (SAS/Gamma), NO pulsos | **E2** | Configuración del tipo de máquina en el recolector | |
| C.2 | Reporte en línea de contadores al sistema (SCLM) | **E1** | Log del recolector: `CONTADORES_RECIBIDOS_CON_EXITO` + ver recepción en servidor | |
| C.3 | Los contadores exigibles de la sección B llegan al backend con valor | **E1/E2** | Comparar acta vs trama recibida en el servidor | |
| C.4 | Contadores digitales, sin procesos mecánicos/análogos | **E2** | Máquinas mecánicas (tipos 9/15) NO certificables — verificar tipo | |
| C.5 | Integridad de la información en el trayecto | E2 | Trama encriptada (AES) + CRC — diseño del recolector | |
| C.6 | El recolector no altera ni recalcula contadores (passthrough) | Interna | Revisión: valor del acta = valor reportado al backend | |

---

## D. EVENTOS SIGNIFICATIVOS / INFORMACIÓN DE CONTROL (anexo §3.4)

> **Cómo verificar**: generar el evento en la máquina y confirmar que llega al backend y/o al archivo `Eventos-*.CSV` de la SD, con fecha/hora, código y descripción (no error genérico).

| # | Evento / requisito | Exigencia | Cómo provocarlo | Res. |
|---|---|---|---|---|
| D.1 | Corrupción de RAM de la MET | E3 | Simular con máquina de banco (0x31/0x32/0x33) | |
| D.2 | Cambio del juego | E3 | Cambiar juego/programa en la máquina | |
| D.3 | Reinicio o falla de corriente | E3 | Apagar/encender la máquina (0x17/0x18) | |
| D.4 | Cada evento viaja con fecha/hora + identidad de máquina + código exclusivo + descripción | E3 | Inspeccionar trama/evento recibido | |
| D.5 | Prohibido enviar eventos como código de error genérico | E3 | Inspeccionar trama/evento recibido | |
| D.6 | Fecha/hora correcta (RTC sincronizado) | Interna | Verificar sincronización RTC por API | |

---

## E. HARDWARE DE LA MÁQUINA (anexo §2 — inspección + documentación)

| # | Requisito | Exigencia | Cómo verificar | Res. |
|---|---|---|---|---|
| E.1 | Gabinete seguro, materiales resistentes/no combustibles | E3 | Inspección visual | |
| E.2 | Ante corte de energía: contadores operan ≥ 180 s y registros se preservan | E3 | Prueba de corte + ficha técnica | |
| E.3 | Encendido/apagado sin riesgo; no afecta contadores ni registros | E3 | Ciclo de energía: comparar contadores antes/después | |
| E.4 | Cables de energía y datos no accesibles al público | E3 | Inspección visual | |
| E.5 | Log histórico de eventos (cambios de estado) | E3 | SD del recolector: `Log-*.TXT` + `Eventos-*.CSV` | |

---

## F. PROGRAMA DE JUEGO — PANTALLA (anexo §3.1)

| # | Requisito | Exigencia | Cómo verificar | Res. |
|---|---|---|---|---|
| F.1 | Muestra jugadas posibles y combinaciones ganadoras | E3 | Pantalla de la máquina | |
| F.2 | Muestra premio por combinación en pesos o créditos | E3 | Pantalla de la máquina | |
| F.3 | Muestra valor en pesos de moneda/billete/tiquete aceptado | E3 | Pantalla de la máquina | |
| F.4 | Aviso de anulación de jugada/pago por mal funcionamiento | E3 | Manual + pantalla | |
| F.5 | Todo evento de juego se completa y registra aunque se corte la energía | E3 | Prueba de corte durante juego | |

---

## G. RNG (anexo §3.2 — solo laboratorio)

| # | Requisito | Exigencia | Cómo verificar | Res. |
|---|---|---|---|---|
| G.1 | RNG certificado (95% confiabilidad, sin patrones, semilla no predecible, etc.) | E3 | **Solo laboratorio acreditado** — pedir informe al fabricante | |
| G.2 | Si aplica: RTP teórico ≥ 83% (locales sin restricción de menores) | E3 | Hoja de datos del juego / laboratorio | |

---

## H. MODALIDAD LOCAL SIN RESTRICCIÓN DE MENORES (anexo §5 — si aplica)

| # | Requisito | Exigencia | Cómo verificar | Res. |
|---|---|---|---|---|
| H.1 | La máquina NO paga efectivo directamente | — | Inspección | |
| H.2 | Premios ≤ 48 UVT (progresivos interconectados ≤ 96 UVT) | — | Configuración / certificado | |
| H.3 | Sin alarmas audibles; solo luz superior | — | Inspección | |
| H.4 | Créditos/tiquetes gestionados vía TDV | — | Operativa del local | |

---

## I. CERTIFICACIÓN DOCUMENTAL (anexo §6)

| # | Requisito | Cómo verificar | Res. |
|---|---|---|---|
| I.1 | Certificado de laboratorio acreditado por Coljuegos que evidencie CADA punto del anexo (o certificación en otra jurisdicción, Art. 6) | Documento del fabricante | |
| I.2 | Modelo inscrito ante Coljuegos | Radicado/registro | |
| I.3 | NUC asignado y visible | Placa/registro | |

---

## VEREDICTO

**Reglas:**

1. **Cumple Etapa I (vigente hoy)**: a, b, c, n, o en C **y** C.2 en C **y** ningún ítem E1 en NC.
2. **Cumple Etapa II (desde 29-nov-2026)**: lo anterior **y** d, e, f, g, l, m en C **y** h, i, j, k en C o NA-justificado (sin TITO) **y** C.1, C.4 en C.
3. **Cumple certificación plena (Etapa III / tarifa 12% sin gradualidad)**: todo el anexo verificado por laboratorio (I.1 en C).
4. Todo **NC** se reporta al operador indicando: ítem, causa, y si el responsable es la máquina, la instalación o el recolector.
5. Todo **NA** debe quedar justificado por escrito (ej. "sin TITO — parágrafo Art. 4").

| Resultado de la evaluación | |
|---|---|
| Cumple Etapa I (vigente) | SÍ / NO |
| Cumple Etapa II (nov-2026) | SÍ / NO / N/D |
| Alertas antes de nov-2026 (ítems E2 en NC hoy) | |
| Ítems PV (responsable y fecha de cierre) | |
| Veredicto final y firma del evaluador | |
