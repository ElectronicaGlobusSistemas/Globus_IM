#include "Cumplimiento.h"
#include "Contadores.h"   // ids de contadores del firmware (Coin_In, Jackpot, ...)
#include "Clase_Variables_Globales.h" // Comunicacion_Maq: enlace con la maquina
#include "Configuracion.h" // Tipo_Maquina: parte de la identidad del acta
#include "Preferences.h"  // NVS: cache del acta (namespace "Certif")
#include "ESP32Time.h"    // RTC: fecha de la medicion
#include <stdio.h>

extern Contadores_SAS contadores; // Objeto global con los contadores de la maquina
extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales
extern Configuracion_ESP32 Configuracion; // Configuracion del equipo (Tipo_Maquina)
extern Preferences NVS;  // Objeto global NVS (main.cpp)
extern ESP32Time RTC;    // Reloj global (main.cpp)


//#define Debug_Cumplimiento

// ============================================================================
//  Requisitos de la Resolucion 20211200034224 (anexo, literales a-o):
//  cada literal se satisface con CUALQUIERA de sus contadores del firmware.
//  Dos literales son pares INDIVIDUAL/TOTAL del mismo contador (hay maquinas
//  que responden los 2 y maquinas que responden solo uno):
//    d) Total_Cancel_Credit / Cancel_Credit_Hand_Pay
//    g) Bill_Amount / Total_Drop
//  Un literal APLICA para un tipo de maquina solo si su cadena lo consulta
//  (alguna de sus ids quedo preguntada); si aplica, debe responder para cumplir
// ============================================================================
struct Req_Literal
{
  const char *lit;     // literal de la resolucion ("a)", "o.2)", ...)
  uint8_t ids[2];      // contadores del firmware que lo satisfacen (0 = fin)
};

static const Req_Literal REQUERIDOS[] = {
  {"a)",   {Coin_In, 0}},                              // 2
  {"b)",   {Coin_Out, 0}},                             // 3
  {"c)",   {Jackpot, 0}},                              // 4
  {"d)",   {Total_Cancel_Credit, Cancel_Credit_Hand_Pay}}, // 1 / 6 (indiv/total)
  {"e)",   {Physical_Coin_In, 0}},                     // 15
  {"f)",   {Physical_Coin_Out, 0}},                    // 16
  {"g)",   {Bill_Amount, Total_Drop}},                 // 7 / 5 (indiv/total)
  {"h)",   {Ticket_In, 0}},                            // 22
  {"i)",   {Ticket_Out, 0}},                           // 23
  {"j)",   {Machine_Paid_External_Bonus_Payout, 0}},   // 19
  {"k)",   {Attendant_Paid_External_Bonus_Payout, 0}}, // 21
  {"l)",   {Attendant_Paid_Progresive_Payout, 0}},     // 20
  {"m)",   {Machine_Paid_Progresive_Payout, 0}},       // 18
  {"n)",   {Informacion_Maquina, 0}},                  // 27
  {"o)",   {Games_Played, 0}},                         // 14
  {"o.2)", {Games_Since_Last_Power_Up, 0}},            // 26
};
#define NUM_REQUERIDOS (sizeof(REQUERIDOS) / sizeof(REQUERIDOS[0]))

//  El snapshot del acta (Snap_Acta.lit[]) debe cubrir TODOS los literales de
//  la tabla: si se anade o quita uno, ajustar CUMPLIMIENTO_NUM_LITERALES
static_assert(NUM_REQUERIDOS == CUMPLIMIENTO_NUM_LITERALES,
              "REQUERIDOS y Snap_Acta.lit[] deben tener la misma cantidad de literales");

//  (definida en la seccion de identidad/cache, mas abajo: extrae del contador
//  35 la version SAS y el serial de maquina respondidos por el poll 0x54)
static void Copiar_Serial_SAS(char version[6], char serial[41]);

//  (definida junto a Cumplimiento_Estado_Texto, mas abajo: texto del estado
//  de una fila del snapshot - la usa Imprimir_Acta, declarada antes)
static const char *Estado_Texto_Snap(uint8_t estado);

// ----------------------------------------------------------------------------
//  Nombre_Meter - nombre EXACTO del define de Contadores.h (para que la tabla
//  hable el mismo idioma que el codigo)
// ----------------------------------------------------------------------------
static const char *Nombre_Meter(int id)
{
  switch (id)
  {
  case Total_Cancel_Credit:               return "Total_Cancel_Credit";
  case Coin_In:                           return "Coin_In";
  case Coin_Out:                          return "Coin_Out";
  case Jackpot:                           return "Jackpot";
  case Total_Drop:                        return "Total_Drop";
  case Cancel_Credit_Hand_Pay:            return "Cancel_Credit_Hand_Pay";
  case Bill_Amount:                       return "Bill_Amount";
  case Physical_Coin_In:                  return "Physical_Coin_In";
  case Physical_Coin_Out:                 return "Physical_Coin_Out";
  case Machine_Paid_Progresive_Payout:    return "Machine_Paid_Progresive_Payout";
  case Machine_Paid_External_Bonus_Payout:return "Machine_Paid_External_Bonus_Payout";
  case Attendant_Paid_Progresive_Payout:  return "Attendant_Paid_Progresive_Payout";
  case Attendant_Paid_External_Bonus_Payout:return "Attendant_Paid_External_Bonus_Payout";
  case Ticket_In:                         return "Ticket_In";
  case Ticket_Out:                        return "Ticket_Out";
  case Games_Played:                      return "Games_Played";
  case Games_Since_Last_Power_Up:         return "Games_Since_Last_Power_Up";
  case Informacion_Maquina:               return "Informacion_Maquina";
  default:                                return "?";
  }
}

//  Estado de un literal (derivado del estado de sus contadores)
enum Estado_Fila : uint8_t
{
  FILA_NO_APLICA = 0,  // este tipo de maquina no lo consulta
  FILA_PENDIENTE,      // consultado, ventana de respuesta abierta
  FILA_OK,             // consultado y respondio (cumple)
  FILA_ERROR           // consultado y vencio el timeout sin respuesta
};

// ----------------------------------------------------------------------------
//  Estado_Fila_De - evalua un literal contra el registro. id_repr (si se pasa)
//  recibe la id representativa: la que ya respondio, o la que esta preguntada
// ----------------------------------------------------------------------------
static Estado_Fila Estado_Fila_De(const Req_Literal &req, int *id_repr = nullptr)
{
  Estado_Consulta mejor = SIN_CONSULTAR;
  int id_mejor = -1;

  for (uint8_t i = 0; i < 2 && req.ids[i] != 0; i++)
  {
    Estado_Consulta e = Estado_De(req.ids[i]);

    if (e == CONSULTADO)              // cualquiera que respondio satisface
    {
      if (id_repr) *id_repr = req.ids[i];
      return FILA_OK;
    }
    //  nos quedamos con el estado mas avanzado de los consultados
    if ((e == EN_PROGRESO || e == ERROR_TIMEOUT) && mejor == SIN_CONSULTAR)
    {
      mejor = e;
      id_mejor = req.ids[i];
    }
  }

  if (id_repr) *id_repr = id_mejor;
  if (mejor == SIN_CONSULTAR)  return FILA_NO_APLICA;
  if (mejor == EN_PROGRESO)    return FILA_PENDIENTE;
  return FILA_ERROR;
}

// ============================================================================
//  Registro interno: un slot por id de Contadores.h. Solo datos crudos; el
//  estado SIEMPRE se deriva al leerlo (no hay task ni timer vigilando nada:
//  la propia cadena de encuestas hace avanzar el tiempo al llamar a las marcas)
// ============================================================================
struct Registro_Meter
{
  uint32_t t_ask;      // millis() de la PRIMERA consulta (el timeout no se refresca)
  uint16_t preguntado; // veces que la cadena lo ha preguntado
  uint16_t respondido; // veces que ha respondido (con consulta pendiente)
};

static Registro_Meter Meters[CUMPLIMIENTO_MAX_METERS];

//  Rondas completas de la cadena de encuestas (todas las preguntas hechas >= 1 vez)
static uint16_t Rondas_Completas = 0;

//  Certificacion de UNA SOLA PASADA: cuando concluye (todo respondido o con
//  timeout vencido) se imprime el resultado automaticamente y el modulo queda
//  inactivo - las marcas dejan de contar. Reinicio del ESP = certificacion nueva
static volatile bool Certificacion_Cerrada = false;
static volatile bool Certificacion_Impresa = false;

//  Alguna respuesta 0x54 (version SAS + serial) desde el boot. VIVE FUERA del
//  ciclo de vida de la certificacion: el registro se congela al cerrar (una
//  sola pasada, por diseno), pero el dato de la maquina sigue vivo - la cadena
//  de encuestas sigue preguntando 0x54 cada ronda, y pantalla/API deben
//  reflejarlo cuando la maquina por fin conteste, sin reiniciar la interfaz
static volatile bool Serial_SAS_Recibido = false;

//  Certificacion ARMADA: la maquina ya dio senal de vida desde el boot
//  (bandera del sistema Comunicacion_Maq en 1 o primera respuesta de un
//  contador). Antes de eso las marcas NO cuentan: un arranque con la maquina
//  apagada o iniciando quemaria los 20 s de timeout de cada contador con la
//  maquina muda y cerraria NO VERIFICABLE sin haber medido nada. Armado =
//  la unica pasada corre con la maquina viva. Sticky: un enlace que se cae
//  despues de armar NO desarma (el timeout de cada contador y el veredicto
//  SIN_COMUNICACION ya reportan la caida)
static volatile bool Certificacion_Armada = false;

//  La ronda de encuestas en curso al armarse empezo con la maquina muda (sus
//  marcas TX no contaron): su fin de ronda se TRAGA para que Rondas_Completas
//  solo cuente rondas medidas de principio a fin
static volatile bool Tragar_Proximo_Fin_Ronda = false;

//  Candado de las 3 tasks que tocan el registro (encuestas / UART / consulta)
static portMUX_TYPE Mux_Cumplimiento = portMUX_INITIALIZER_UNLOCKED;

//  Timeout vigente por contador (default TIMEOUT_METER_MS; ajustable para
//  pruebas con Cumplimiento_Set_Timeout)
static uint32_t Timeout_Meter_ms = TIMEOUT_METER_MS;

// ----------------------------------------------------------------------------
//  Intentar_Cierre - la invocan las marcas (el latido de la propia cadena de
//  encuestas hace avanzar la certificacion: no hay task ni timer propio).
//  Cuando todos los contadores preguntados quedaron resueltos, CIERRA el modulo
//  (solo iza la bandera; la impresion se difiere al proximo latido)
// ----------------------------------------------------------------------------
static void Intentar_Cierre(void)
{
  if (Certificacion_Cerrada || !Certificacion_Lista())
    return;                       // ya cerro / todavia hay ventanas abiertas

  taskENTER_CRITICAL(&Mux_Cumplimiento);
  Certificacion_Cerrada = true;
  taskEXIT_CRITICAL(&Mux_Cumplimiento);
}

// ----------------------------------------------------------------------------
//  Imprimir_Si_Cerro - saca la tabla final UNA sola vez, en el LATIDO SIGUIENTE
//  al cierre. Motivo: el cierre suele dispararlo la ultima respuesta desde
//  DENTRO de Set_Contadores, ANTES del memcpy que guarda el valor - una
//  impresion inmediata mostraria el valor viejo (buffer sin actualizar). Con el
//  diferido, el print corre cuando la cadena ya dio otra marca y todos los
//  valores estan almacenados
// ----------------------------------------------------------------------------

//  Blindaje de la cache tras la tabla final (definida al final del archivo:
//  necesita Denominacion_Texto y el motor de la tabla)
static void Blindaje_Si_Cerro(void);

static void Imprimir_Si_Cerro(void)
{
  bool imprimir = false;

  if (!Certificacion_Cerrada || Certificacion_Impresa)
    return;

  taskENTER_CRITICAL(&Mux_Cumplimiento);
  if (!Certificacion_Impresa)     // solo la primera task en llegar imprime
  {
    Certificacion_Impresa = true;
    imprimir = true;
  }
  taskEXIT_CRITICAL(&Mux_Cumplimiento);

  if (imprimir)
  {
    Cumplimiento_Imprimir();      // resultado final, automatico, una sola vez
    Blindaje_Si_Cerro();          // cache NVS: identidad + tabla congelada
  }
}

// ============================================================================
//  Marcas
// ============================================================================
void Marcar_Consulta(int Filtro_Contador)
{
  if (Filtro_Contador < 0 || Filtro_Contador >= CUMPLIMIENTO_MAX_METERS)
    return;

  Imprimir_Si_Cerro();            // latido: tabla final si ya cerro (1 sola vez)

  if (Certificacion_Cerrada)      // certificacion concluida: modulo inactivo
    return;

  //  Armado por enlace: la bandera del sistema (en RAM) tambien prueba vida
  //  si sube antes que la primera respuesta de un contador
  if (!Certificacion_Armada &&
      !Variables_globales.Get_Variable_Global(Comunicacion_Maq))
    return;                       // maquina muda: no quemar la unica pasada

  taskENTER_CRITICAL(&Mux_Cumplimiento);
  if (!Certificacion_Armada)
  {
    Certificacion_Armada = true;      // primera senal de vida
    Tragar_Proximo_Fin_Ronda = true;  // la ronda en curso empezo sin marcas
  }
  if (Meters[Filtro_Contador].preguntado == 0)   // primera vez: arranca el timeout
    Meters[Filtro_Contador].t_ask = millis();
  Meters[Filtro_Contador].preguntado++;
  taskEXIT_CRITICAL(&Mux_Cumplimiento);

  Intentar_Cierre();
}

void Marcar_Respuesta(int Filtro_Contador)
{
  if (Filtro_Contador < 0 || Filtro_Contador >= CUMPLIMIENTO_MAX_METERS)
    return;

  //  Fuera del ciclo de vida de la certificacion (corre tambien con el modulo
  //  ya cerrado): el 0x54 alimenta la pantalla/API de runtime, y CUALQUIER
  //  respuesta prueba vida de la maquina
  if (Filtro_Contador == Serial_Maquina_SAS)
    Serial_SAS_Recibido = true;

  Imprimir_Si_Cerro();            // latido: tabla final si ya cerro (1 sola vez)

  if (Certificacion_Cerrada)      // certificacion concluida: modulo inactivo
    return;

  taskENTER_CRITICAL(&Mux_Cumplimiento);
  if (!Certificacion_Armada)
  {
    Certificacion_Armada = true;      // primera respuesta: la maquina esta viva
    Tragar_Proximo_Fin_Ronda = true;  // la ronda en curso empezo sin marcas
  }
  //  Guarda anti-falsos: solo cierra una consulta pendiente. Un write que no
  //  responde a ninguna pregunta (contador acompanante del parser o NULL
  //  writer de los flujos poker) no cuenta como respuesta
  if (Meters[Filtro_Contador].respondido < Meters[Filtro_Contador].preguntado)
    Meters[Filtro_Contador].respondido++;
  taskEXIT_CRITICAL(&Mux_Cumplimiento);

  Intentar_Cierre();
}

void Marcar_Fin_Ronda(void)
{
  Imprimir_Si_Cerro();            // latido: tabla final si ya cerro (1 sola vez)

  if (Certificacion_Cerrada)      // certificacion concluida: modulo inactivo
    return;

  taskENTER_CRITICAL(&Mux_Cumplimiento);
  if (!Certificacion_Armada || Tragar_Proximo_Fin_Ronda)
  {
    //  Sin armar: ronda con la maquina muda, no cuenta. Tragada: la ronda
    //  empezo antes del armado (marcas a medias) - la primera ronda VALIDA
    //  es la siguiente, medida de principio a fin
    Tragar_Proximo_Fin_Ronda = false;
    taskEXIT_CRITICAL(&Mux_Cumplimiento);
    return;
  }
  Rondas_Completas++;
  taskEXIT_CRITICAL(&Mux_Cumplimiento);

  Intentar_Cierre();
}

// ============================================================================
//  Estados derivados
// ============================================================================
static void Leer_Registro(int id, uint32_t &t_ask, uint16_t &preg, uint16_t &resp)
{
  taskENTER_CRITICAL(&Mux_Cumplimiento);
  t_ask = Meters[id].t_ask;
  preg  = Meters[id].preguntado;
  resp  = Meters[id].respondido;
  taskEXIT_CRITICAL(&Mux_Cumplimiento);
}

Estado_Consulta Estado_De(int Filtro_Contador)
{
  if (Filtro_Contador < 0 || Filtro_Contador >= CUMPLIMIENTO_MAX_METERS)
    return SIN_CONSULTAR;

  uint32_t t_ask;
  uint16_t preg, resp;
  Leer_Registro(Filtro_Contador, t_ask, preg, resp);

  if (resp >= 1)
    return CONSULTADO;                              // sticky: respondio, cumple
  if (preg == 0)
    return SIN_CONSULTAR;                           // nunca se le ha preguntado
  if (millis() - t_ask > Timeout_Meter_ms)
    return ERROR_TIMEOUT;                           // vencio el plazo sin respuesta
  return EN_PROGRESO;                               // ventana abierta
}

uint16_t Preguntado_De(int Filtro_Contador)
{
  if (Filtro_Contador < 0 || Filtro_Contador >= CUMPLIMIENTO_MAX_METERS)
    return 0;
  uint32_t t;
  uint16_t preg, resp;
  Leer_Registro(Filtro_Contador, t, preg, resp);
  return preg;
}

uint16_t Respondido_De(int Filtro_Contador)
{
  if (Filtro_Contador < 0 || Filtro_Contador >= CUMPLIMIENTO_MAX_METERS)
    return 0;
  uint32_t t;
  uint16_t preg, resp;
  Leer_Registro(Filtro_Contador, t, preg, resp);
  return resp;
}

const char *Estado_Texto(int Filtro_Contador)
{
  switch (Estado_De(Filtro_Contador))
  {
  case CONSULTADO:   return "CONSULTADO";
  case EN_PROGRESO:  return "EN PROGRESO";
  case ERROR_TIMEOUT:return "ERROR TIMEOUT";
  default:           return "SIN CONSULTAR";
  }
}

// ============================================================================
//  Veredicto de certificacion
// ============================================================================
bool Certificacion_Terminada(void)
{
  return Certificacion_Cerrada;
}

bool Certificacion_Lista(void)
{
  bool hay_ronda;
  taskENTER_CRITICAL(&Mux_Cumplimiento);
  hay_ronda = (Rondas_Completas >= 1);
  taskEXIT_CRITICAL(&Mux_Cumplimiento);

  if (!hay_ronda)
    return false;                     // la cadena no completo ni una vuelta

  //  Lista cuando ningun literal aplicable sigue PENDIENTE (todos los plazos
  //  vencieron o todos respondieron)
  for (uint8_t i = 0; i < NUM_REQUERIDOS; i++)
  {
    if (Estado_Fila_De(REQUERIDOS[i]) == FILA_PENDIENTE)
      return false;                   // todavia hay ventanas abiertas
  }
  return true;
}

// ----------------------------------------------------------------------------
//  Tipo_Mecanico - tipos 9 (2 contadores) y 15 (4 contadores). La resolucion
//  NO acepta contadores mecanicos (pueden ser modificados): estas maquinas
//  quedan FUERA de la medicion (su cadena no lleva marcas, asi que la
//  certificacion jamas cierra). Este veto es la RED DE SEGURIDAD: si alguien
//  llegara a instrumentarlas de nuevo, el veredicto tampoco podria ser CUMPLE
// ----------------------------------------------------------------------------
static bool Es_Tipo_Mecanico(uint16_t t)
{
  return (t == 9 || t == 15);
}

static bool Tipo_Mecanico(void)
{
  return Es_Tipo_Mecanico(Configuracion.Get_Configuracion(Tipo_Maquina, 0));
}

bool Cumplimiento_OK(void)
{
  if (Tipo_Mecanico())
    return false;                     // veto: tipo mecanico nunca cumple

  for (uint8_t i = 0; i < NUM_REQUERIDOS; i++)
  {
    if (Estado_Fila_De(REQUERIDOS[i]) == FILA_ERROR)
      return false;                   // un literal aplicable no respondio
  }
  return true;
}

// ----------------------------------------------------------------------------
//  Totales_Registro - suma preguntado/respondido de TODO el registro (no solo
//  los literales de la resolucion: cualquier contador de la cadena que haya
//  respondido prueba que el enlace con la maquina esta vivo)
// ----------------------------------------------------------------------------
static void Totales_Registro(uint16_t &total_preg, uint16_t &total_resp)
{
  total_preg = 0;
  total_resp = 0;
  taskENTER_CRITICAL(&Mux_Cumplimiento);
  for (uint8_t i = 0; i < CUMPLIMIENTO_MAX_METERS; i++)
  {
    total_preg += Meters[i].preguntado;
    total_resp += Meters[i].respondido;
  }
  taskEXIT_CRITICAL(&Mux_Cumplimiento);
}

// ----------------------------------------------------------------------------
//  Certificacion_Sin_Comunicacion - la certificacion concluyo con timeouts
//  cuyo causa puede ser el enlace y no los contadores. Dos senales:
//    1. Comunicacion_Maq == 0: el firmware mismo reporta que no hay enlace
//       con la maquina (cable desconectado, maquina apagada...)
//    2. 0 respuestas en TODO el registro: se pregunto y no contesto nada
//  En cualquiera de las dos el veredicto correcto es NO VERIFICABLE, no NO
//  CUMPLE. Consultarlo DESPUES que Cumplimiento_OK(): si todo lo requerido
//  respondio, CUMPLE gana aunque el enlace se haya caido despues
// ----------------------------------------------------------------------------
bool Certificacion_Sin_Comunicacion(void)
{
  if (!Certificacion_Terminada())
    return false;                     // sin cerrar no hay veredicto alguno

  if (!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
    return true;                      // bandera del sistema: enlace caido

  uint16_t preg, resp;
  Totales_Registro(preg, resp);
  return (preg > 0 && resp == 0);     // se pregunto y no contesto NADA
}

// ----------------------------------------------------------------------------
//  Tabla de codigos de denominacion SAS (byte [5] del bloque de informacion
//  de maquina) -> valor del credito en centavos. Ej: 0x08 = 1000 centavos
//  = $10.00. Codigos fuera de tabla se muestran tal cual ("cod XX")
// ----------------------------------------------------------------------------
static const struct { uint8_t codigo; uint32_t centavos; } DENOM_SAS[] = {
  {0x01, 1},      {0x02, 5},      {0x03, 10},     {0x04, 25},
  {0x05, 50},     {0x06, 100},    {0x07, 500},    {0x08, 1000},
  {0x09, 2000},   {0x0A, 10000},  {0x0B, 20},     {0x0C, 200},
  {0x0D, 250},    {0x0E, 2500},   {0x0F, 5000},   {0x10, 20000},
  {0x11, 25000},  {0x12, 50000},  {0x13, 100000}, {0x14, 200000},
  {0x15, 250000}, {0x16, 500000}, {0x17, 2},      {0x18, 3},
  {0x19, 15},     {0x1A, 40},     {0x1B, 0},      // 0x1B: 0.5c (fraccionario)
  {0x1C, 0},      // 0.25c                                          |
  {0x1D, 0},      // 0.2c                                           |
  {0x1E, 0},      // 0.1c                                           |
  {0x1F, 0},      // 0.05c   <- fraccionarios van por FRAC_CENTAVOS |
};
#define NUM_DENOM_SAS (sizeof(DENOM_SAS) / sizeof(DENOM_SAS[0]))

//  Fraccionarios (menos de 1 centavo) en centecimas de centavo: valor x 100
static const struct { uint8_t codigo; uint32_t centecimas; } DENOM_SAS_FRAC[] = {
  {0x1B, 50},   // 0.5c
  {0x1C, 25},   // 0.25c
  {0x1D, 20},   // 0.2c
  {0x1E, 10},   // 0.1c
  {0x1F, 5},    // 0.05c
};

//  Denominacion_Texto - convierte el codigo en "$X.XX" (enteros de centavo)
//  o "$0.XXXX" (fraccionarios). Sin tabla: "cod XX"
static void Denominacion_Texto(uint8_t codigo, char *salida, size_t tam)
{
  for (uint8_t i = 0; i < NUM_DENOM_SAS; i++)
  {
    if (DENOM_SAS[i].codigo != codigo || DENOM_SAS[i].centavos == 0)
      continue;
    uint32_t c = DENOM_SAS[i].centavos;
    if (c >= 100)   // dolares con centavos: $10.00, $2.50...
      snprintf(salida, tam, "$%lu.%02lu",
               (unsigned long)(c / 100), (unsigned long)(c % 100));
    else            // centavos enteros < 100: $0.10, $0.03...
      snprintf(salida, tam, "$0.%02lu", (unsigned long)c);
    return;
  }

  for (uint8_t i = 0; i < sizeof(DENOM_SAS_FRAC) / sizeof(DENOM_SAS_FRAC[0]); i++)
  {
    if (DENOM_SAS_FRAC[i].codigo == codigo)
    {               // fraccionarios: $0.0005, $0.0025...
      snprintf(salida, tam, "$0.%04lu",
               (unsigned long)DENOM_SAS_FRAC[i].centecimas);
      return;
    }
  }

  snprintf(salida, tam, "cod %02X", codigo);      // codigo fuera de tabla
}

// ============================================================================
//  Impresion de la certificacion (automatica al concluir; tambien a mano)
// ============================================================================
void Cumplimiento_Imprimir(void)
{
#ifdef Debug_Cumplimiento
  uint8_t n_ok = 0, n_err = 0, n_pend = 0, n_na = 0;

  Serial.println("\r\n──────── CERTIFICACION DE CONTADORES (Res. 20211200034224) ────────");
  Serial.println("Lit.    Contador                                P/R     Valor             Resultado");

  for (uint8_t i = 0; i < NUM_REQUERIDOS; i++)
  {
    int id = -1;
    Estado_Fila f = Estado_Fila_De(REQUERIDOS[i], &id);
    uint16_t p = 0, r = 0;
    const char *res;
    char valor[20] = "-";            // valor respondido ("-" si no ha respondido)

    //  Nombre del contador que esta maquina usa para el literal: el que
    //  respondio (o el que se consulta); si no aplica, el primero del par
    int id_nombre = (id >= 0) ? id : REQUERIDOS[i].ids[0];

    if (id >= 0)                      // id representativa (la que respondio/consulta)
    {
      p = Preguntado_De(id);
      r = Respondido_De(id);
    }

    //  Valor = lo que respondio la maquina (ultimo valor almacenado). Solo se
    //  muestra cuando respondio; sin respuesta el buffer esta en 0 y seria
    //  engañoso. De Informacion_Maquina solo se muestra la DENOMINACION (byte
    //  [5] del bloque, decodificada con la tabla de codigos SAS)
    if (f == FILA_OK && id >= 0)
    {
      char *v = contadores.Get_Contadores_Char(id);
      if (id == Informacion_Maquina)
        Denominacion_Texto((uint8_t)v[5], valor, sizeof(valor));
      else
        snprintf(valor, sizeof(valor), "%s", v);
    }

    switch (f)
    {
    case FILA_OK:       res = "OK";          n_ok++;   break;
    case FILA_ERROR:    res = "NO RESPONDE"; n_err++;  break;
    case FILA_PENDIENTE:res = "...";         n_pend++; break;
    default:            res = "NO APLICA";   n_na++;   break;
    }

    Serial.printf("%-7s %-38s %2u/%-2u  %-17s %s\r\n",
                  REQUERIDOS[i].lit, Nombre_Meter(id_nombre),
                  (unsigned)p, (unsigned)r, valor, res);
  }

  //  Identidad extendida: version SAS + serial real (poll 0x54). No es un
  //  literal de la resolucion: se muestra como evidencia de que maquina
  //  exacta se esta certificando
  {
    char version[6], serial[41];
    Copiar_Serial_SAS(version, serial);
    if (version[0] != '\0')
      Serial.printf("SAS Version : %-4s   Serial Maquina : %s\r\n", version, serial);
    else
      Serial.println("SAS Version / Serial Maquina : NO RESPONDE 0x54");
  }

  Serial.println("─────────────────────────────────────────────────────────────────────");
  if (n_pend > 0 || !Certificacion_Lista())
    Serial.println("RESULTADO: EN PROGRESO (certificacion no concluida)");
  else if (Cumplimiento_OK())
    //  Todo lo requerido respondio: CUMPLE gana aunque el enlace se haya
    //  caido despues de la ultima respuesta
    Serial.printf("RESULTADO: CUMPLE - %u aplican y responden, %u no aplican\r\n",
                  (unsigned)n_ok, (unsigned)n_na);
  else if (Certificacion_Sin_Comunicacion())
  {
    //  Timeouts causados por el enlace, no por los contadores. El "NO
    //    RESPONDE" de las filas NO es un fallo real de la maquina
    uint16_t total_preg, total_resp;
    Totales_Registro(total_preg, total_resp);
    if (total_resp == 0)
      Serial.printf("RESULTADO: NO VERIFICABLE - SIN COMUNICACION con la maquina "
                    "(Comunicacion_Maq=0; %u encuestas enviadas, 0 respuestas). "
                    "NO CUMPLE no aplica: reconectar el enlace y reiniciar el "
                    "ESP32 para repetir la certificacion\r\n",
                    (unsigned)total_preg);
    else
      Serial.printf("RESULTADO: NO VERIFICABLE - el enlace con la maquina cayo "
                    "durante la certificacion (Comunicacion_Maq=0; %u respuestas "
                    "recibidas). Reconectar y reiniciar el ESP32 para repetirla\r\n",
                    (unsigned)total_resp);
  }
  else if (Tipo_Mecanico())
    //  Veto por tipo: sus contadores pueden salir OK en la tabla, pero el
    //  tipo mecanico no entrega los contadores que exige la resolucion
    Serial.println("RESULTADO: NO CUMPLE - TIPO MECANICO: la resolucion exige "
                   "contadores que este tipo de maquina no entrega");
  else
  {
    Serial.print("RESULTADO: NO CUMPLE - no responden: ");
    for (uint8_t i = 0, primeros = 0; i < NUM_REQUERIDOS; i++)
    {
      if (Estado_Fila_De(REQUERIDOS[i]) != FILA_ERROR)
        continue;
      if (primeros++ > 0)
        Serial.print(", ");
      Serial.printf("%s %s", REQUERIDOS[i].lit,
                    Nombre_Meter(REQUERIDOS[i].ids[0]));
    }
    Serial.println();
  }
#else
  /*  Produccion sin Debug_Cumplimiento: la tabla NO se imprime por serial
      (el acta queda disponible por GET /api/Cumplimiento/Acta) */
#endif
}

// ============================================================================
//  Identidad de maquina y cache del acta (blindaje de la certificacion)
// ============================================================================
//  Espejo RAM del snapshot en NVS. Carga perezosa: el primer acceso (normal-
//  mente al cerrar la certificacion) lee el namespace "Certif"
static Snap_Acta Cache_Acta;
static bool Cache_Cargada = false; // ya se leyo NVS en esta sesion
static bool Cache_Valida = false;  // habia un acta guardada

// ----------------------------------------------------------------------------
//  Hash_FNV1a - un byte de la mezcla FNV-1a de 32 bits (identidad compacta)
// ----------------------------------------------------------------------------
static uint32_t Hash_FNV1a(uint8_t byte, uint32_t h)
{
  h ^= byte;
  return h * 16777619u;
}

// ----------------------------------------------------------------------------
//  Copiar_Serial_SAS - extrae del contador 35 (poll 0x54) la version SAS
//  ([0..2] ASCII "601" -> se formatea "6.01": mayor.minor) y el serial de
//  maquina ([3..42] ASCII zero-padded), este ultimo recortado al primer
//  byte 0 (serial de largo variable 0-40)
// ----------------------------------------------------------------------------
static void Copiar_Serial_SAS(char version[6], char serial[41])
{
  version[0] = '\0';
  serial[0] = '\0';
  if (!Serial_SAS_Recibido)
    return;                       // sin respuesta 0x54 aun: campos vacios

  char *b = contadores.Get_Contadores_Char(Serial_Maquina_SAS);
  snprintf(version, 6, "%c.%c%c", b[0], b[1], b[2]);

  uint8_t i = 0;
  while (i < 40 && b[3 + i] != '\0')
  {
    serial[i] = b[3 + i];
    i++;
  }
  serial[i] = '\0';
}

// ----------------------------------------------------------------------------
//  Cumplimiento_Identidad_Maquina - identidad de la maquina conectada AHORA:
//  FNV-1a sobre el bloque 0x1F bytes [0..4] (IDs de juego) + [8..19] (grupo,
//  opciones, tabla de pagos, % base; los bytes [5][6][7] EXCLUIDOS: denom SAS,
//  denom usuario y apuesta max son config de operador - cambiar la denom no
//  es cambiar de maquina) + ROM_Signature + Tipo_Maquina.
//  Retorna 0 si el literal n) nunca respondio: sin bloque 0x1F no hay
//  identidad en la que anclar el acta
// ----------------------------------------------------------------------------
uint32_t Cumplimiento_Identidad_Maquina(void)
{
  if (Estado_De(Informacion_Maquina) != CONSULTADO)
    return 0;                       // sin respuesta 0x1F no hay identidad

  //  Los buffers de id 27/28 NO estan null-terminated: copia por memcpy,
  //  nunca %s directo
  uint8_t bloque[20];
  memcpy(bloque, contadores.Get_Contadores_Char(Informacion_Maquina), 20);
  uint8_t rom[2];
  memcpy(rom, contadores.Get_Contadores_Char(ROM_Signature), 2);

  uint16_t tipo = Configuracion.Get_Configuracion(Tipo_Maquina, 0);

  uint32_t h = 2166136261u;         // offset basis FNV-1a
  for (uint8_t i = 0; i < 5; i++)   // [0..4]: game ID + ID adicional
    h = Hash_FNV1a(bloque[i], h);
  for (uint8_t i = 8; i < 20; i++)  // [8..19]: config del juego
    h = Hash_FNV1a(bloque[i], h);
  h = Hash_FNV1a(rom[0], h);        // firma de ROM (si ningun flujo la lleno
  h = Hash_FNV1a(rom[1], h);        // queda constante: inofensiva)

  //  Serial SAS real (poll 0x54, ultimo paso de la cadena): identidad POR
  //  UNIDAD fisica - dos maquinas identicas en modelo/juego/config ya no
  //  producen el mismo hash. [0..2] version SAS ASCII + [3..42] serial
  //  ASCII zero-padded. Solo mezcla si respondio (maquinas SAS viejas sin
  //  0x54 no cambian su identidad por ello)
  if (Estado_De(Serial_Maquina_SAS) == CONSULTADO)
  {
    uint8_t serial[44];
    memcpy(serial, contadores.Get_Contadores_Char(Serial_Maquina_SAS), 44);
    for (uint8_t i = 0; i < 44; i++)
      h = Hash_FNV1a(serial[i], h);
  }

  h = Hash_FNV1a(tipo & 0xFF, h);   // el tipo define que cadena pregunta y
  h = Hash_FNV1a((tipo >> 8) & 0xFF, h); // que literales aplican
  return h;
}

const char *Cumplimiento_Identidad_Texto(void)
{
  static char hex[9];
  uint32_t id = Cumplimiento_Identidad_Maquina();
  if (id == 0)
    return "N/A";
  snprintf(hex, sizeof(hex), "%08lX", (unsigned long)id);
  return hex;
}

// ----------------------------------------------------------------------------
//  Cache_Leer - carga perezosa del snapshot NVS al espejo RAM (una vez por
//  sesion). Blob de otro tamano (firmware viejo) => cache invalida: se re-
//  registrara en el proximo cierre
// ----------------------------------------------------------------------------
static void Cache_Leer(void)
{
  if (Cache_Cargada)
    return;
  Cache_Cargada = true;
  Cache_Valida = false;
  memset(&Cache_Acta, 0, sizeof(Cache_Acta));

  if (!NVS.begin(CUMPLIMIENTO_NVS_NS, true))     // solo lectura
    return;                       // namespace inexistente: primer registro

  if (NVS.isKey("Acta") && NVS.getBytesLength("Acta") == sizeof(Snap_Acta))
  {
    NVS.getBytes("Acta", &Cache_Acta, sizeof(Cache_Acta));
    Cache_Valida = true;
#ifdef Debug_Cumplimiento
    Serial.printf("[CERTIFICACION] Previa registrada: Ident %08lX %s (%s)\r\n",
                  (unsigned long)Cache_Acta.ident,
                  Cache_Acta.vered ? "CUMPLE" : "NO_CUMPLE",
                  Cache_Acta.fecha);
#endif
  }
  NVS.end();
}

// ----------------------------------------------------------------------------
//  Cache_Guardar - persiste el acta (un solo blob) y refresca el espejo RAM
// ----------------------------------------------------------------------------
static void Cache_Guardar(const Snap_Acta &acta)
{
  if (!NVS.begin(CUMPLIMIENTO_NVS_NS, false))
    return;

  NVS.putBytes("Acta", &acta, sizeof(acta));
  NVS.end();

  Cache_Acta = acta;
  Cache_Valida = true;
}

// ----------------------------------------------------------------------------
//  Llenar_Snapshot - congela la tabla de este cierre en un Snap_Acta. Misma
//  llena que Cumplimiento_Imprimir (Estado_Fila_De / Preguntado_De /
//  Respondido_De / Get_Contadores_Char), salida a struct en vez de serial
// ----------------------------------------------------------------------------
static void Llenar_Snapshot(Snap_Acta &acta, uint32_t ident)
{
  memset(&acta, 0, sizeof(acta));
  acta.ident = ident;
  acta.tipo  = Configuracion.Get_Configuracion(Tipo_Maquina, 0);
  acta.vered = Cumplimiento_OK() ? 1 : 0;
  Copiar_Serial_SAS(acta.version_sas, acta.serial_sas); // evidencia 0x54
  snprintf(acta.fecha, sizeof(acta.fecha), "%u-%u-%u %u:%u:%u",
           (unsigned)RTC.getYear(), (unsigned)(RTC.getMonth() + 1),
           (unsigned)RTC.getDay(), (unsigned)RTC.getHour(true),
           (unsigned)RTC.getMinute(), (unsigned)RTC.getSecond());

  for (uint8_t i = 0; i < NUM_REQUERIDOS; i++)
  {
    int id = -1;
    Estado_Fila f = Estado_Fila_De(REQUERIDOS[i], &id);
    int id_nombre = (id >= 0) ? id : REQUERIDOS[i].ids[0];

    snprintf(acta.lit[i].lit, sizeof(acta.lit[i].lit), "%s", REQUERIDOS[i].lit);
    acta.lit[i].id_repr = (uint8_t)id_nombre;

    if (id >= 0)
    {
      acta.lit[i].preguntado = Preguntado_De(id);
      acta.lit[i].respondido = Respondido_De(id);
    }

    switch (f)
    {
    case FILA_OK:        acta.lit[i].estado = 1; acta.n_ok++;        break;
    case FILA_ERROR:     acta.lit[i].estado = 2; acta.n_error++;     break;
    case FILA_PENDIENTE: acta.lit[i].estado = 3;                     break;
    default:             acta.lit[i].estado = 0; acta.n_no_aplica++; break;
    }

    //  Valor medido al cierre (misma regla que la tabla serial: solo si
    //  respondio; en n) la denominacion decodificada)
    if (f == FILA_OK && id >= 0)
    {
      char *v = contadores.Get_Contadores_Char(id);
      if (id == Informacion_Maquina)
        Denominacion_Texto((uint8_t)v[5], acta.lit[i].valor,
                           sizeof(acta.lit[i].valor));
      else
        snprintf(acta.lit[i].valor, sizeof(acta.lit[i].valor), "%s", v);
    }
    else
      snprintf(acta.lit[i].valor, sizeof(acta.lit[i].valor), "-");
  }
}

// ----------------------------------------------------------------------------
//  Acta_Mecanica - acta sintetica para tipos mecanicos (9/15). La resolucion
//  no acepta contadores mecanicos (modificables): veredicto NO CUMPLE por
//  tipo, SIN medir nada (P/R en 0, sin marcas en la cadena, sin encuestas
//  SAS: estas maquinas no tienen SAS). No se ancla a identidad (ident 0: no
//  hay bloque 0x1F que hashear) ni se persiste en NVS: el veredicto se deriva
//  del tipo, no de una medicion - se re-sintetiza en cada boot
// ----------------------------------------------------------------------------
static void Acta_Mecanica(Snap_Acta &acta)
{
  memset(&acta, 0, sizeof(acta));
  acta.tipo  = Configuracion.Get_Configuracion(Tipo_Maquina, 0);
  acta.vered = 0;                    // veto por tipo: nunca CUMPLE
  snprintf(acta.fecha, sizeof(acta.fecha), "%u-%u-%u %u:%u:%u",
           (unsigned)RTC.getYear(), (unsigned)(RTC.getMonth() + 1),
           (unsigned)RTC.getDay(), (unsigned)RTC.getHour(true),
           (unsigned)RTC.getMinute(), (unsigned)RTC.getSecond());

  for (uint8_t i = 0; i < NUM_REQUERIDOS; i++)
  {
    snprintf(acta.lit[i].lit, sizeof(acta.lit[i].lit), "%s", REQUERIDOS[i].lit);
    acta.lit[i].id_repr = REQUERIDOS[i].ids[0];
    acta.lit[i].estado  = 4;         // MECANICA
    snprintf(acta.lit[i].valor, sizeof(acta.lit[i].valor), "-");
    acta.n_no_aplica++;
  }
}

// ----------------------------------------------------------------------------
//  Imprimir_Acta - tabla de un acta (la guardada en NVS o la sintetica de
//  mecanicas) por serial. Mismo formato que la tabla del cierre; el titulo y
//  la observacion los pone quien llama
// ----------------------------------------------------------------------------
static void Imprimir_Acta(const Snap_Acta &acta, const char *titulo,
                          const char *observacion = nullptr)
{
#ifdef Debug_Cumplimiento
  Serial.println(titulo);
  Serial.printf("Identidad %08lX   Tipo %u   Veredicto %s   Medida %s\r\n",
                (unsigned long)acta.ident, (unsigned)acta.tipo,
                acta.vered ? "CUMPLE" : "NO_CUMPLE", acta.fecha);
  if (acta.version_sas[0] != '\0')
    Serial.printf("SAS Version : %-4s   Serial Maquina : %s\r\n",
                  acta.version_sas, acta.serial_sas);
  Serial.println("Lit.    Contador                                P/R     Valor             Resultado");

  for (uint8_t i = 0; i < CUMPLIMIENTO_NUM_LITERALES; i++)
    Serial.printf("%-7s %-38s %2u/%-2u  %-17s %s\r\n",
                  acta.lit[i].lit,
                  Nombre_Meter(acta.lit[i].id_repr),
                  (unsigned)acta.lit[i].preguntado,
                  (unsigned)acta.lit[i].respondido,
                  acta.lit[i].valor,
                  Estado_Texto_Snap(acta.lit[i].estado));

  if (observacion && observacion[0])
    Serial.printf("Observacion: %s\r\n", observacion);
  Serial.println("─────────────────────────────────────────────────────────────────────");
#else
  (void)acta;      /*  Produccion sin Debug_Cumplimiento: las actas NO se
                      imprimen por serial (solo GET /api/Cumplimiento/Acta) */
  (void)titulo;
  (void)observacion;
#endif
}

// ----------------------------------------------------------------------------
//  Blindaje_Si_Cerro - corre justo despues de la tabla final (una sola vez):
//  decide si el acta de este cierre se registra en NVS (reglas del header).
//  El borrado por cambio de tipo ya lo cubre Cumplimiento_Cache_Boot al
//  boot: la rama de tipo que queda aqui es la red de seguridad. El "no se
//  escribe nada" cuando nada cambio evita desgaste NVS en cada reinicio de
//  una maquina ya certificada
// ----------------------------------------------------------------------------
static void Blindaje_Si_Cerro(void)
{
  Cache_Leer();

  if (!Certificacion_Lista() || Certificacion_Sin_Comunicacion())
    return;                       // NO VERIFICABLE: la cache queda intacta

  uint32_t ident = Cumplimiento_Identidad_Maquina();
  if (ident == 0)
  {
#ifdef Debug_Cumplimiento
    Serial.println("[CERTIFICACION] Sin respuesta 0x1F: no se puede anclar "
                   "identidad (cache intacta)");
#endif
    return;
  }

  uint8_t vered = Cumplimiento_OK() ? 1 : 0;
  uint16_t tipo = Configuracion.Get_Configuracion(Tipo_Maquina, 0);

  if (Cache_Valida)
  {
#ifdef Debug_Cumplimiento
    if (Cache_Acta.tipo != tipo)
      Serial.printf("[CERTIFICACION] Tipo de maquina cambio (%u -> %u): "
                    "cache invalidada\r\n",
                    (unsigned)Cache_Acta.tipo, (unsigned)tipo);
    else if (Cache_Acta.ident != ident)
      Serial.printf("[CERTIFICACION] CAMBIO DE MAQUINA: anterior %08lX, "
                    "actual %08lX - veredicto anterior invalidado\r\n",
                    (unsigned long)Cache_Acta.ident, (unsigned long)ident);
    else if (Cache_Acta.vered != vered)
      Serial.printf("[CERTIFICACION] Identidad %08lX cambia veredicto %s -> %s\r\n",
                    (unsigned long)ident,
                    Cache_Acta.vered ? "CUMPLE" : "NO_CUMPLE",
                    vered ? "CUMPLE" : "NO_CUMPLE");
    else
    {
      Serial.printf("[CERTIFICACION] Identidad %08lX ya certificada %s (%s) "
                    "- sin cambios\r\n",
                    (unsigned long)ident,
                    vered ? "CUMPLE" : "NO_CUMPLE", Cache_Acta.fecha);
      return;                     // nada nuevo: NO se escribe NVS
    }
#else
    /*  Produccion sin serial: MISMO criterio del else de arriba - si nada
        cambio (tipo, identidad y veredicto) NO se reescribe el NVS */
    if (Cache_Acta.tipo == tipo && Cache_Acta.ident == ident &&
        Cache_Acta.vered == vered)
      return;
#endif
  }

  Snap_Acta acta;
  Llenar_Snapshot(acta, ident);
  Cache_Guardar(acta);
#ifdef Debug_Cumplimiento
  Serial.printf("[CERTIFICACION] Identidad %08lX registrada (%s) - %s\r\n",
                (unsigned long)ident, vered ? "CUMPLE" : "NO_CUMPLE",
                acta.fecha);
  Cumplimiento_Imprimir();        // la tabla tal cual quedo guardada en NVS
#else
  (void)vered;                    /* produccion: sin serial, el acta queda */
#endif                             /* servible por GET /api/Cumplimiento/Acta */
}

// ----------------------------------------------------------------------------
//  Consulta publica de la cache (el futuro modulo de envio al sistema leera
//  el acta de aqui: el diagnostico sale del snapshot, no de una re-medicion)
// ----------------------------------------------------------------------------
bool Cumplimiento_Cache_Disponible(void)
{
  Cache_Leer();
  return Cache_Valida;
}

// ----------------------------------------------------------------------------
//  Cumplimiento_Cache_Boot - llamar en setup(): garantiza que el acta en NVS
//  corresponponde al Tipo_Maquina con el que arranca el equipo. Si el tipo
//  cambio, la certificacion previa se BORRA: el conjunto de literales que
//  aplican cambia con la cadena de encuesta, y con un tipo sin cadena
//  instrumentada la certificacion nunca cerraria para invalidarla despues.
//  MECANICAS (9/15): no hay medicion posible, pero el acta existe igual -
//  se sintetiza EN RAM (NO CUMPLE por tipo, observacion NO TIENE SAS) y
//  queda servible por /api/Cumplimiento/Acta; nunca se escribe NVS
// ----------------------------------------------------------------------------
void Cumplimiento_Cache_Boot(void)
{
  Cache_Leer();

  uint16_t tipo = Configuracion.Get_Configuracion(Tipo_Maquina, 0);

  if (Cache_Valida && Cache_Acta.tipo != tipo)
  {
#ifdef Debug_Cumplimiento
    Serial.printf("[CERTIFICACION] Tipo de maquina cambio (%u -> %u): "
                  "certificacion previa borrada\r\n",
                  (unsigned)Cache_Acta.tipo, (unsigned)tipo);
#endif

    if (NVS.begin(CUMPLIMIENTO_NVS_NS, false))
    {
      NVS.clear();
      NVS.end();
    }
    memset(&Cache_Acta, 0, sizeof(Cache_Acta));
    Cache_Valida = false;
  }

  if (Tipo_Mecanico())
  {
    Acta_Mecanica(Cache_Acta);
    Cache_Valida = true;           // acta en RAM: el endpoint la sirve normal
    Imprimir_Acta(Cache_Acta,
                  "\r\n──────── CERTIFICACION MECANICA (Res. 20211200034224) ────────",
                  Cumplimiento_Observacion_Acta(Cache_Acta));
  }
}

bool Cumplimiento_Cache_Leer(Snap_Acta &acta)
{
  Cache_Leer();
  if (!Cache_Valida)
    return false;
  acta = Cache_Acta;
  return true;
}

void Cumplimiento_Cache_Reiniciar(void)
{
  if (NVS.begin(CUMPLIMIENTO_NVS_NS, false))
  {
    NVS.clear();
    NVS.end();
  }
  memset(&Cache_Acta, 0, sizeof(Cache_Acta));
  Cache_Cargada = true;           // estado conocido: vacio
  Cache_Valida = false;
#ifdef Debug_Cumplimiento
  Serial.println("[CERTIFICACION] Cache del acta borrada");
#endif
}

// ----------------------------------------------------------------------------
//  Wrappers publicos para la API: exponen el nombre del contador y el texto
//  del estado de una fila del snapshot (Estado_Texto_Snap se define mas abajo)
// ----------------------------------------------------------------------------
const char *Cumplimiento_Nombre_Contador(int id)
{
  return Nombre_Meter(id);
}

// ----------------------------------------------------------------------------
//  Cumplimiento_Cache_Imprimir - imprime la TABLA GUARDADA en NVS (la
//  evidencia registrada, no una re-medicion). Mismo formato que la tabla
//  final; el nombre del contador se re-deriva de id_repr con Nombre_Meter
// ----------------------------------------------------------------------------
static const char *Estado_Texto_Snap(uint8_t estado)
{
  switch (estado)
  {
  case 1:  return "OK";
  case 2:  return "NO RESPONDE";
  case 3:  return "PENDIENTE";
  case 4:  return "MECANICA";
  default: return "NO APLICA";
  }
}

const char *Cumplimiento_Estado_Texto(uint8_t estado_snap)
{
  return Estado_Texto_Snap(estado_snap);
}

// ----------------------------------------------------------------------------
//  Cumplimiento_Observacion_Acta - nota que acompania el acta servida (serial
//  y API). Vacia para todo acta medida; para tipos mecanicos (9/15) lleva el
//  aviso de que la maquina no tiene SAS y de que el NO CUMPLE es por tipo
// ----------------------------------------------------------------------------
const char *Cumplimiento_Observacion_Acta(const Snap_Acta &acta)
{
  if (Es_Tipo_Mecanico(acta.tipo))
    return "MAQUINA MECANICA - NO TIENE SAS - NO CUMPLE POR TIPO";

  return "";
}

void Cumplimiento_Cache_Imprimir(void)
{
  Cache_Leer();
  if (!Cache_Valida)
  {
#ifdef Debug_Cumplimiento
    Serial.println("[CERTIFICACION] Sin acta guardada en NVS");
#endif
    return;
  }

  Imprimir_Acta(Cache_Acta,
                "\r\n──────── CERTIFICACION GUARDADA (Res. 20211200034224) ────────",
                Cumplimiento_Observacion_Acta(Cache_Acta));
}

// ============================================================================
//  Pruebas de banco
// ============================================================================
void Cumplimiento_Set_Timeout(uint32_t ms)
{
  taskENTER_CRITICAL(&Mux_Cumplimiento);
  Timeout_Meter_ms = ms;
  taskEXIT_CRITICAL(&Mux_Cumplimiento);
}

// ----------------------------------------------------------------------------
//  Cumplimiento_Test_Timeout - autotest de la maquina de estados SIN maquina.
//  Recorre la vida completa de un contador (consulta -> vencimiento ->
//  respuesta tardia) mas la guarda anti-falsos, con timeout de 300 ms para
//  que la prueba tarde <1 s. Usa ids fuera de la tabla REQUERIDOS (28/29) y
//  los devuelve a cero al terminar: no contamina la certificacion real
// ----------------------------------------------------------------------------
bool Cumplimiento_Test_Timeout(void)
{
  const int ID_TEST  = ROM_Signature;   // 28: fuera de los literales, no lo
  const int ID_TEST2 = Billetes_2k;     // 29: pregunta la cadena Simple
  bool ok = true;

  Serial.println("\r\n[CUMPLIMIENTO] Autotest maquina de estados (timeout 300 ms)");

  if (Certificacion_Cerrada)
  {
    Serial.println("[TEST] FAIL: la certificacion ya cerro (reiniciar el ESP)");
    return false;
  }

  //  El autotest simula maquina viva: ARMA las marcas (sin enlace real las
  //  consultas no contarian) y devuelve el armado exactamente como estaba
  bool armada_previa, tragada_previa;
  taskENTER_CRITICAL(&Mux_Cumplimiento);
  armada_previa  = Certificacion_Armada;
  tragada_previa = Tragar_Proximo_Fin_Ronda;
  Certificacion_Armada = true;
  Tragar_Proximo_Fin_Ronda = false;
  taskEXIT_CRITICAL(&Mux_Cumplimiento);

  Cumplimiento_Set_Timeout(300);

  //  1. Consulta simulada: debe quedar EN_PROGRESO con la ventana abierta
  Marcar_Consulta(ID_TEST);
  bool p1 = (Estado_De(ID_TEST) == EN_PROGRESO);
  Serial.printf("  1. tras consulta       -> %-14s %s\r\n",
                Estado_Texto(ID_TEST), p1 ? "OK" : "FAIL");
  ok &= p1;

  //  2. Vencido el timeout: debe pasar a ERROR_TIMEOUT
  delay(400);
  bool p2 = (Estado_De(ID_TEST) == ERROR_TIMEOUT);
  Serial.printf("  2. timeout vencido     -> %-14s %s\r\n",
                Estado_Texto(ID_TEST), p2 ? "OK" : "FAIL");
  ok &= p2;

  //  3. Respuesta tardia: debe SUBIR a CONSULTADO (sticky gana al error)
  Marcar_Respuesta(ID_TEST);
  bool p3 = (Estado_De(ID_TEST) == CONSULTADO);
  Serial.printf("  3. respuesta tardia    -> %-14s %s\r\n",
                Estado_Texto(ID_TEST), p3 ? "OK" : "FAIL");
  ok &= p3;

  //  4. Respuesta SIN consulta previa: NO debe contar (guarda anti-falsos)
  Marcar_Respuesta(ID_TEST2);
  bool p4 = (Respondido_De(ID_TEST2) == 0 && Estado_De(ID_TEST2) == SIN_CONSULTAR);
  Serial.printf("  4. resp. sin consulta  -> %-14s %s\r\n",
                Estado_Texto(ID_TEST2), p4 ? "OK" : "FAIL");
  ok &= p4;

  //  Limpieza: registro de los ids de prueba a cero, timeout de produccion y
  //  armado restaurado (el autotest no abre ni atrasa la certificacion real)
  taskENTER_CRITICAL(&Mux_Cumplimiento);
  Meters[ID_TEST]  = Registro_Meter();
  Meters[ID_TEST2] = Registro_Meter();
  Timeout_Meter_ms = TIMEOUT_METER_MS;
  Certificacion_Armada = armada_previa;
  Tragar_Proximo_Fin_Ronda = tragada_previa;
  taskEXIT_CRITICAL(&Mux_Cumplimiento);

  Serial.println(ok ? "[CUMPLIMIENTO] Autotest PASS"
                    : "[CUMPLIMIENTO] Autotest FAIL");
  return ok;
}

// ----------------------------------------------------------------------------
//  Informacion_Protocolo_SAS - version SAS formateada ("6.01", "0.00" si
//  respondio en cero) o serial de maquina, del contador 35 (poll 0x54).
//  Delega en Copiar_Serial_SAS: una sola fuente del layout 0x54. Buffers
//  estaticos SEPARADOS por tipo: pedir version y serial a la vez y retener
//  ambos punteros es seguro dentro de un mismo task
// ----------------------------------------------------------------------------
const char *Informacion_Protocolo_SAS(TipoDatoSAS tipo)
{
  static char version[6];
  static char serial[41];

  if (!Serial_SAS_Recibido)
    return "Esperando respuesta";      // literal: inmutable, sin aliasing

  Copiar_Serial_SAS(version, serial);

  switch (tipo)
  {
  case SAS_VERSION:
    return version;
  case SAS_SERIAL:
    return serial;
  }

  return "";
}
