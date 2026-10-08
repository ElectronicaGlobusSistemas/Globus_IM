#ifndef CUMPLIMIENTO_H
#define CUMPLIMIENTO_H

#include "Arduino.h"

// ============================================================================
//  Cumplimiento - Maquina de estados de consulta de contadores SAS
//
//  Objetivo: poder verificar si la maquina entrega TODOS los contadores que
//  exige la Resolucion 20211200034224 de 2021 (Coljuegos). Cada contador del
//  firmware (ids de Contadores.h) lleva su propia maquina de estados:
//
//    SIN_CONSULTAR ──(se envia la encuesta)──► EN_PROGRESO ──llega respuesta──► CONSULTADO
//                          │                                  (sticky: basta 1 vez)
//                          │
//                          └──(vence TIMEOUT_METER_MS sin respuesta alguna)──► ERROR_TIMEOUT
//
//  Reglas de la certificacion (una sola pasada):
//   - ARMADO por enlace: las marcas NO cuentan hasta que la maquina de senal
//     de vida (Comunicacion_Maq en 1 o primera respuesta de un contador). Sin
//     esto, un arranque con la maquina apagada/iniciando quemaria los 20 s de
//     cada contador con la maquina muda y cerraria NO VERIFICABLE sin haber
//     medido nada. La ronda en curso al armarse no cuenta (empezo muda): la
//     pasada mide desde la primera ronda completa con enlace
//   - El timeout arranca con la PRIMERA vez que se pregunta el contador; las
//     vueltas siguientes del ciclo de polling NO lo refrescan (si en 20 s no
//     contesto, ya casi seguro no lo va a contestar).
//   - Una respuesta llegada a tiempo (de la vuelta que sea) lo marca
//     CONSULTADO para siempre.
//   - La certificacion CONCLUYE cuando la cadena completo una ronda y ningun
//     contador quedo EN_PROGRESO (todos respondieron o vencieron su timeout):
//     el modulo IMPRIME el resultado por serial automaticamente (una sola vez,
//     en el latido siguiente al cierre, cuando todos los valores ya estan
//     almacenados) y queda inactivo - las marcas dejan de contar. Si ya
//     respondio todo a la primera, ya se garantizo que cumple: no hay nada mas
//     que verificar. Reiniciar el ESP32 = certificacion nueva.
//   - Si al concluir NO respondio NI UN contador (enlace caido o maquina
//     apagada), o la bandera global Comunicacion_Maq quedo en 0, el veredicto
//     es NO VERIFICABLE / SIN COMUNICACION: el "NO RESPONDE" de cada fila es
//     falta de enlace, no un fallo real de los contadores. NO CUMPLE se
//     reserva para maquinas que SI contestan pero omiten algun literal de la
//     resolucion (si todo lo requerido respondio, CUMPLE gana aunque el
//     enlace se caiga despues)
//
//  Integracion (solo lineas anhadidas, cero cambios de logica existente):
//   - Marcar_Consulta(id)  -> primera linea del case en TODAS las cadenas
//                             Encuestas_Maquinas_* de RS232.h (Simple,
//                             Simple_No_Cancel, Genericas, EFT, IRT, Poker,
//                             Poker_Ertech_Slot, Aristocrat_Australiana,
//                             Ruleta_IRT, 0F, IGT_Riel, IGT_Riel_Bill) y en
//                             Encuestas_Mecanicas (tipos 9/15: un frame pide
//                             todos los contadores, las marcas van agrupadas
//                             antes del TX). Los meters NO requeridos por la
//                             resolucion (casheables, billetes, copias) no se
//                             marcan
//   - Marcar_Respuesta(id) -> una linea al inicio de Contadores_SAS::
//                             Set_Contadores() (todo RX termina ahi)
//   - Marcar_Fin_Ronda()   -> junto a los Conta_Encuestas = 0 de cada cadena
//                             (en mecanicas: cada frame TX/RX es una ronda)
//   - MECANICAS (9 y 15) FUERA de la medicion: la resolucion NO acepta
//                             contadores mecanicos (pueden ser modificados).
//                             Su cadena no lleva marcas -> la certificacion
//                             jamas cierra. El boot les sintetiza un acta EN
//                             RAM (NO CUMPLE por tipo, filas MECANICA, P/R 0,
//                             observacion "NO TIENE SAS") servible por serial
//                             y por /api/Cumplimiento/Acta; nunca se escribe
//                             NVS. El veto en Cumplimiento_OK() queda como
//                             red de seguridad. Gamma (tipo 19) tambien queda
//                             fuera (prototipo)
//
//  Concurrency: el registro lo tocan 3 tasks (encuestas, UART y quien consulte
//  estados): los incrementos van bajo seccion critica (portMUX). Las lecturas
//  de estado son derivadas (no se almacenan), asi que no hay nada que sincronizar.
// ============================================================================

//  Estados derivados de un contador (no se guardan: se calculan al leerlos)
enum Estado_Consulta : uint8_t
{
  SIN_CONSULTAR = 0,   // nunca se le ha preguntado
  EN_PROGRESO,         // preguntado, ventana de respuesta abierta
  CONSULTADO,          // respondio al menos una vez (cumple)
  ERROR_TIMEOUT        // venio el timeout sin respuesta alguna (no cumple)
};

//  Plazo de respuesta por contador desde su PRIMERA consulta. Con el ciclo de
//  polling corriendo, 20 s cubren varias vueltas: si en ese tiempo jamas
//  contesto, la maquina no lo va a contestar. (Default; ajustable en runtime
//  con Cumplimiento_Set_Timeout para pruebas)
#define TIMEOUT_METER_MS 20000UL

//  Impresion por serial de las TABLAS de certificacion (cierre automatico de
//  la certificacion, acta guardada en NVS y acta sintetica de mecanicas).
//  Comentado = produccion: serial limpio, el acta sigue disponible por
//  GET /api/Cumplimiento/Acta. Descomentar solo para depurar en banco
//#define Debug_Cumplimiento

//  Rango de ids validos del registro (Contadores.h usa ids 1..58)
#define CUMPLIMIENTO_MAX_METERS 64

// --------------------------------------------------> Marcas de la maquina de estados
//  TX: se esta enviando la encuesta del contador (llamarla en el case que lo
//  pregunta, antes del Transmite_Poll). Arranca el timeout SOLO la primera vez
void Marcar_Consulta(int Filtro_Contador);

//  RX: llego una respuesta para el contador (la llama Set_Contadores). Solo
//  cuenta si hay una consulta pendiente que cerrar (guarda anti-falsos: el
//  parser a veces escribe dos contadores con una sola respuesta -p.ej. 0x13
//  escribe Total_Drop y Bill_Amount-; los acompanantes que nunca se preguntaron
//  quedan en SIN_CONSULTAR en vez de falsos CONSULTADO). Ademas ARMA la
//  certificacion (primera senal de vida de la maquina) y, para el 0x54, iza
//  Serial_SAS_Recibido - vive fuera del cierre del modulo (dato de runtime)
void Marcar_Respuesta(int Filtro_Contador);

//  La cadena de encuestas completo una vuelta (todos los meters preguntados
//  una vez). Habilita el veredicto de certificacion
void Marcar_Fin_Ronda(void);

// --------------------------------------------------> Consulta de estado (derivada)
Estado_Consulta Estado_De(int Filtro_Contador);

//  Contadores P/R de cada meter (para depuracion en banco: "3/3 OK", "5/0 ERROR")
uint16_t Preguntado_De(int Filtro_Contador);
uint16_t Respondido_De(int Filtro_Contador);

//  Texto del estado ("SIN CONSULTAR" / "EN PROGRESO" / "CONSULTADO" /
//  "ERROR TIMEOUT") para impresion por serial
const char *Estado_Texto(int Filtro_Contador);

// --------------------------------------------------> Version y serial SAS (0x54)
//  Version SAS formateada ("6.01") o serial de maquina, del contador 35
//  (poll 0x54). Disponible desde la PRIMERA respuesta 0x54 del boot (bandera
//  propia Serial_SAS_Recibido, NO el registro de certificacion: ese se
//  congela al cerrar y una maquina que arranca tarde quedaria "Esperando
//  respuesta" para siempre aunque ya este contestando - la cadena sigue
//  preguntando 0x54 cada ronda). Buffers estaticos SEPARADOS por tipo: pedir
//  version y serial a la vez y retener ambos punteros es seguro dentro de un
//  mismo task
enum TipoDatoSAS
{
  SAS_SERIAL,          // serial de maquina, recortado al primer byte 0
  SAS_VERSION          // version SAS "mayor.minor" ("000" -> "0.00")
};

const char *Informacion_Protocolo_SAS(TipoDatoSAS tipo);

// --------------------------------------------------> Veredicto de certificacion
//  True cuando la certificacion ya se cerro (se imprimo el resultado y el
//  modulo quedo inactivo). Para saber si el resultado final esta disponible
bool Certificacion_Terminada(void);

//  La certificacion esta lista: hubo al menos una ronda completa de la cadena
//  y ningun contador preguntado sigue EN_PROGRESO (todos resueltos, llegaran
//  o no). Antes de eso el veredicto seria prematuro
bool Certificacion_Lista(void);

//  Resultado final (solo valido con Certificacion_Lista()):
//    true  -> todos los contadores preguntados respondieron (CUMPLE)
//    false -> al menos uno quedo en ERROR_TIMEOUT (NO CUMPLE), o la maquina
//             es tipo mecanico (9/15): veto por tipo, nunca CUMPLE
bool Cumplimiento_OK(void);

//  True cuando los timeouts del cierre pueden ser culpa del enlace y no de
//  los contadores: Comunicacion_Maq == 0 (el firmware reporta enlace caido)
//  o 0 respuestas en todo el registro. Consultarlo DESPUES que
//  Cumplimiento_OK(): si todo respondio, CUMPLE gana aunque el enlace cayera
bool Certificacion_Sin_Comunicacion(void);

//  Resumen por serial: cada contador preguntado con su estado y su P/R + el
//  veredicto. El propio modulo la invoca AUTOMATICAMENTE al concluir la
//  certificacion (una sola vez); tambien se puede llamar a mano en cualquier
//  momento para ver el estado actual. SOLO imprime con Debug_Cumplimiento:
//  en produccion la funcion queda vacia (el acta se consulta por API)
void Cumplimiento_Imprimir(void);

// --------------------------------------------------> Identidad y cache del acta
//  Blindaje de la certificacion: la tabla que sale por serial se congela al
//  cierre en un Snap_Acta y se guarda en NVS, namespace propio "Certif" (NO
//  "Gamma": ese pertenece al modulo Gamma; este sirve a SAS y a Gamma a
//  futuro, porque ambos llenan el mismo bloque Informacion_Maquina). El acta
//  queda anclada a la IDENTIDAD de la maquina medida, asi ningun cambio de
//  tarjeta o de maquina hereda un veredicto prestado. Reglas:
//    0. AL BOOT, tipo de maquina distinto al cacheado -> el acta previa se
//       BORRA (Cumplimiento_Cache_Boot): lo guardado siempre corresponde al
//       tipo vigente, aunque el tipo nuevo no tenga cadena instrumentada
//    1. Identidad distinta a la cacheada     -> CAMBIO DE MAQUINA: se
//       descarta el veredicto anterior y se registra la medicion actual
//    2. Misma identidad, veredicto distinto  -> se actualiza (reparacion)
//    3. Misma identidad, veredicto igual     -> nada que hacer: no se escribe
//    4. NO VERIFICABLE / sin respuesta 0x1F  -> la cache queda intacta
//
//  Identidad = FNV-1a 32 bits sobre: bloque 0x1F bytes [0..4] y [8..19] +
//  ROM_Signature (2 B) + Serial_Maquina_SAS (44 B del poll 0x54: version SAS
//  + serial real de la maquina, solo si respondio - identidad POR UNIDAD) +
//  Tipo_Maquina. Bytes [5][6][7] EXCLUIDOS: denom SAS, denom usuario y
//  apuesta max son configuracion de operador (cambiar la denominacion no es
//  cambiar de maquina).
//
//  Este snapshot ES el diagnostico que a futuro se enviara al sistema (cada
//  contador con su P/R, valor medido y estado). Escritura solo al cierre y
//  solo si algo cambio: desgaste NVS despreciable.
#define CUMPLIMIENTO_NVS_NS       "Certif"  // namespace dedicado (no "Gamma")
#define CUMPLIMIENTO_NUM_LITERALES 16       // filas de REQUERIDOS (se valida en .cpp)

//  Una fila de la tabla (Lit. | Contador | P/R | Valor | Resultado). El
//  nombre del contador NO se guarda: se re-deriva de id_repr con Nombre_Meter
struct Snap_Literal
{
  char     lit[6];       // "a)", "o.2)" - ancla al literal de la resolucion
  uint8_t  id_repr;      // id del contador que represento el literal
  uint8_t  estado;       // 0=NO APLICA, 1=OK, 2=NO RESPONDE, 3=PENDIENTE, 4=MECANICA
  uint16_t preguntado;   // P de la id representativa
  uint16_t respondido;   // R de la id representativa
  char     valor[16];    // valor MEDIDO al cierre ("-", o "$X.XX" en n))
};

//  El acta congelada: encabezado + las 16 filas. Se persiste como un solo
//  blob en la key "Acta" (~525 bytes)
struct Snap_Acta
{
  uint32_t ident;        // identidad de la maquina medida (hash)
  uint16_t tipo;         // Tipo_Maquina vigente al medir
  uint8_t  vered;        // 1=CUMPLE, 0=NO_CUMPLE
  uint8_t  n_ok;         // resumen de filas: cuantas OK
  uint8_t  n_no_aplica;  // cuantas NO APLICA
  uint8_t  n_error;      // cuantas NO RESPONDE
  char     fecha[20];    // fecha/hora RTC de la medicion
  char     version_sas[6];  // version SAS formateada "6.01" (0x54, "" si no respondio)
  char     serial_sas[41];  // serial real de la maquina (0x54, "" si no respondio)
  Snap_Literal lit[CUMPLIMIENTO_NUM_LITERALES];
};

//  Hash de identidad de la maquina conectada AHORA (0 si el literal n) nunca
//  respondio: sin bloque 0x1F no hay identidad y la cache no se toca)
uint32_t Cumplimiento_Identidad_Maquina(void);

//  Lo mismo en texto: "3FA2B1C9" / "N/A"
const char *Cumplimiento_Identidad_Texto(void);

//  True si hay un acta registrada en NVS (la carga si aun no se ha hecho)
bool Cumplimiento_Cache_Disponible(void);

//  LLamar en setup(): carga la cache y, si el Tipo_Maquina vigente difiere
//  del que estaba al medir el acta guardada, BORRA la certificacion previa
//  (el acta en NVS siempre corresponde al tipo con el que arranca el equipo)
void Cumplimiento_Cache_Boot(void);

//  Entrega una copia del acta guardada (false si no hay). El futuro modulo de
//  envio al sistema la usara como fuente del diagnostico
bool Cumplimiento_Cache_Leer(Snap_Acta &acta);

//  Imprime por serial la TABLA GUARDADA en NVS (la evidencia registrada, no
//  una re-medicion)
void Cumplimiento_Cache_Imprimir(void);

//  Borra el namespace "Certif" (pruebas de banco / uso manual)
void Cumplimiento_Cache_Reiniciar(void);

//  Nombre del contador por id (el mismo de la tabla serial) y texto del
//  estado de una fila del snapshot (0=NO APLICA, 1=OK, 2=NO RESPONDE,
//  3=PENDIENTE, 4=MECANICA) - para armar el JSON de la API
const char *Cumplimiento_Nombre_Contador(int id);
const char *Cumplimiento_Estado_Texto(uint8_t estado_snap);

//  Nota que acompania el acta servida: "" para todo acta medida; para tipos
//  mecanicos (9/15) "MAQUINA MECANICA - NO TIENE SAS - NO CUMPLE POR TIPO"
const char *Cumplimiento_Observacion_Acta(const Snap_Acta &acta);

// --------------------------------------------------> Pruebas de banco
//  Ajusta el timeout de espera por contador (default TIMEOUT_METER_MS = 20 s).
//  Pensado para pruebas: acortarlo hace visible el vencimiento en segundos
void Cumplimiento_Set_Timeout(uint32_t ms);

//  Autotest de la maquina de estados SIN maquina conectada (usa delay: no
//  llamar desde ISR/task critica; ideal en el arranque o por comando serial):
//    1. consulta simulada          -> debe quedar EN_PROGRESO
//    2. timeout vencido (300 ms)   -> debe pasar a ERROR_TIMEOUT
//    3. respuesta tardia           -> debe subir a CONSULTADO (sticky gana)
//    4. respuesta SIN consulta     -> NO debe contar (guarda anti-falsos)
//  Usa los ids 28/29 (fuera de los literales de la resolucion y no consultados
//  por la cadena Simple) y devuelve el registro a cero al terminar, asi que no
//  contamina la certificacion real. Retorna true si TODO paso
bool Cumplimiento_Test_Timeout(void);

#endif // CUMPLIMIENTO_H
