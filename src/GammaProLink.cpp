#include "GammaProLink.h"
#include "Contadores.h"
#include "Preferences.h"
#include <string.h>
#include <eventos.h>


#include "Clase_Variables_Globales.h"
extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales

//#define Debug_Gamma

// ============================================================================
//  >>>>>> DATOS DE PRUEBA maq. 3 - BORRAR AL TERMINAR LAS PRUEBAS <<<<<<
//  Comentar este #define desactiva TODO el modo prueba de una sola vez:
//  con una maquina real conectada (p.ej. la maq. 1), Gamma_DespacharTrama
//  SUSTITUYE los valores de los contadores por los de la maq. 3 (captura
//  04-09-2026) - el enlace, SEQ, CRC y ciclo de encuestas son los reales.
//  Bloques afectados: Gamma_Test_Sustituir_MQ3 (despachador) y el aviso en
//  Gamma_Calibrar_N_Desde_Pantalla.
// ============================================================================
//#define GAMMA_TEST_MQ3

//  Objeto global de contadores (definido en main.cpp) - usado para guardar
//  el valor de los meters igual que lo hace el receptor SAS
extern Contadores_SAS contadores;

//  NVS global del firmware (definido en main.cpp) - persiste la tabla N de
//  la fase SPC (namespace "Gamma", clave "N_Vueltas")
extern Preferences NVS;

extern Eventos_SAS eventos; // Objeto contiene eventos maquina

//  Timeout de comunicacion (definidos en main.cpp / RS232.h):
//  Datos_OK          -> izada al recibir el primer dato valido de la maquina
//  Bandera_RS232_F   -> "ultima vez que se recibio trama valida"; la task
//                       Check_Comunicacion_Maq la compara contra Bandera_RS232
//                       para declarar la caida del enlace (Timeout_RS232)
//  Se declaran extern (sin incluir RS232.h) porque ese header define
//  variables globales y no tiene include guard.
extern bool Datos_OK;
extern unsigned long Bandera_RS232;
extern unsigned long Bandera_RS232_F;
extern bool  flag_ultimo_contador_Ok;

//  "Se completo la secuencia completa de consultas de la maquina". El receptor
//  SAS la iza tras su bloque de informacion 0x1F (ultima respuesta de su
//  secuencia); Check_Comunicacion_Maq (main.cpp) la consume para declarar la
//  primera conexion OK y la vuelve a false. Gamma la iza al completar su ciclo
//  (el bloque identidad 03 01 00 es la ultima consulta de la tabla).
extern bool Ultimo_Counter_;

//  LA puerta de la reconexion: tras una caida (Fallo_Comunicacion=true),
//  Check_Comunicacion_Maq espera Counter_Final==true para salir del modo
//  fallo y devolver Comunicacion_Maq a true. El SAS la iza al completar cada
//  secuencia de contadores; Gamma hace lo propio al completar su ciclo.
extern bool Counter_Final;

// ============================================================================
//  Tabla de contadores del ciclo de encuestas (objetos verificados contra la
//  maquina real - Manual del protocolo, Anexo F)
// ============================================================================
static const uint8_t TABLA_METERS_GAMMA[][3] = {
    {0x02, 0x04, 0x04},   // Total Games Played
    {0x02, 0x04, 0x05},   // Total Credits Bet
    {0x02, 0x04, 0x06},   // Total Credits Won
    {0x02, 0x05, 0x03},   // Cashout Handpays
    {0x04, 0x09, 0x01},   // True In (Coins In)
    {0x04, 0x04, 0x01},   // Coin Drop / Gross Drop
    {0x02, 0x03, 0x01},   // Current Credits
    {0x06, 0x02, 0x01},   // PUERTA: estado en vivo (bit) + aperturas - EGM Doors P1
    {0x02, 0x03, 0x03},   // BLOQUE MULTI-METER - 24 contadores (§7.6)
    {0x02, 0x01, 0x05},   // Denominacion: value of 1 credit en centavos (byte [5])
    {0x04, 0x01, 0x01},   // Value of 1 coin en centavos - solo referencia/log
    {0x04, 0x01, 0x16},   // Apuesta maxima (candidato 20.000) - Anexo G.5
    {0x03, 0x01, 0x00},   // Bloque identidad: juego / % / paytable - G.3
};
#define NUM_METERS_GAMMA (sizeof(TABLA_METERS_GAMMA) / sizeof(TABLA_METERS_GAMMA[0]))

static const char *NOMBRES_METERS_GAMMA[] = {
    "Total Games Played",
    "Total Credits Bet",
    "Total Credits Won",
    "Cashout Handpays",
    "True In (Coins In)",
    "Coin Drop / Gross Drop",
    "Current Credits",
    "Puerta (estado + aperturas)",
    "Bloque Multi-Meter 02 03 03 (24 contadores)",
    "Denominacion (value of 1 credit)",
    "Value of 1 coin (referencia)",
    "Apuesta Maxima (candidato)",
    "Identidad de Maquina",
};

//  Fases del ciclo de encuestas
enum { GAMMA_FASE_QUERY = 0, GAMMA_FASE_ESPERA, GAMMA_FASE_RESYNC };

//  Nombres de la tabla N (fase SPC) - para los avisos del vigilante
static const char *NOMBRES_N_VUELTAS[GAMMA_N_TOTAL] = {
    "Coin In",      // GAMMA_N_COIN_IN
    "Coin Out",     // GAMMA_N_COIN_OUT
    "Drop",         // GAMMA_N_DROP
    "Cancelled",    // GAMMA_N_CANCEL
    "Jackpot",      // GAMMA_N_JACKPOT
    "Coin Drop",    // GAMMA_N_COIN_DROP
    "Games",        // GAMMA_N_GAMES
};

//  Vigilante de vuelta de 2^32: entre dos lecturas del mismo meter (segundos)
//  el contador no puede crecer 4.294.967.296 enteros, asi que una vuelta REAL
//  solo puede verse como caida desde un valor pegado al tope. Una caida desde
//  rango medio NO es vuelta: es RAM clear o el box cambiado de maquina -> el
//  N queda intacto y se avisa para recargar la calibracion
#define GAMMA_UMBRAL_VUELTA_32  3900000000UL

//  Validacion de la calibracion por pantalla: maxima desviacion tolerada
//  entre el R implicito del valor tipeado y la ultima lectura R real, en
//  centavos ($100.000 de juego entre la lectura de la pantalla y el comando).
//  Un typo que cruce el corchete de 2^32 desvia ~4.294.967.296 enteros (mas
//  de 400 veces esta tolerancia) y siempre se rechaza; corner documentado: si
//  el contador esta a <tolerancia del tope de 2^32 y la maquina juega ese
//  resto entre la pantalla y el comando, el N podria dar off-by-one
#define GAMMA_CALIB_TOLERANCIA_C  10000000ULL

//  Marcador "este contador no viene en el comando de calibracion" (un valor
//  de pantalla real jamas llega a $4.294.967.295)
#define GAMMA_CALIB_SIN_DATO     0xFFFFFFFFUL

//  Ventanas de tiempo del ciclo
#define GAMMA_VENTANA_ESPERA_MS 200    // sin DATA -> reenviar la QUERY del mismo meter
#define GAMMA_REINTENTOS_MAX    3      // reintentos agotados -> re-enganchar el enlace
#define GAMMA_VENTANA_RESYNC_MS 1000   // duracion del re-enganche con polls del slot 0x40
#define GAMMA_GAP_TX_AVISO_MS   250    // hueco entre transmisiones mayor = avisar (diagnostico caidas)

//  Fuentes posibles del drop (Total Drop / True In 04 09 01). Poblaciones
//  de maquinas observadas: (a) valor solo en el objeto individual, (b)
//  individual en 0 y valor solo en la entrada [09] del bloque 02 03 03,
//  (c) valor por ambos lados (puede diferir -> gana el individual)
#define GAMMA_DROP_SIN_DECIDIR 0       // ambas lecturas en 0: no setear nada
#define GAMMA_DROP_INDIVIDUAL  1       // objeto 04 09 01 (prioridad si vive)
#define GAMMA_DROP_BLOQUE      2       // entrada [09] del bloque 02 03 03

//  Fuentes que alimentan Informacion_Maquina: se setea SOLO cuando todas llegaron
#define GAMMA_INFO_DENOM    0x01       // 02 01 05 - value of 1 credit (byte [5])
#define GAMMA_INFO_IDENT    0x02       // 03 01 00 - tabla de pagos + porcentaje base
#define GAMMA_INFO_COMPLETA (GAMMA_INFO_DENOM | GAMMA_INFO_IDENT)

//  Instancia global del enlace Gamma
GammaProlink Gamma;

// ============================================================================
//  Constructor: la rotacion arranca en 0x40 (slot de arranque / re-enganche
//  del enlace - Manual §6.3) y el ciclo de encuestas en el primer meter
// ============================================================================
GammaProlink::GammaProlink()
    : indice_slot(0),
      flag_meter_recibido(false),
      flag_ciclo_completo(false),
      fase_encuesta(GAMMA_FASE_QUERY),
      conta_meters(0),
      tiempo_query(0),
      reintentos_query(0),
      hay_objeto_pendiente(false),
      ultimo_seq_enviado(0),
      hay_rx_contador(false),
      rx_long(0)
{
    objeto_pendiente[0] = objeto_pendiente[1] = objeto_pendiente[2] = 0;
    rx_objeto[0] = rx_objeto[1] = rx_objeto[2] = 0;
    rx_contador[0] = '\0';

    //  Informacion_Maquina en 0 (formato SAS de 20 chars): lo que no se
    //  identifica del protocolo Gamma queda en 0 por decision de diseno
    static const char PLANTILLA_INFO[20] = {
        '0', '0',                          // [0..1]  ID de juego
        '0', '0', '0',                     // [2..4]  ID adicional
        0, 0, 0, 0, 0,                     // [5..9]  bytes BCD (denom etc.)
        '0', '0', '0', '0', '0', '0',      // [10..15] tabla de pagos
        '0', '0', '0', '0'                 // [16..19] porcentaje base
    };
    memcpy(info_gamma, PLANTILLA_INFO, sizeof(info_gamma));
    info_flags = 0;                    // ninguna fuente de info recibida aun
    bloque_pendiente = false;          // sin sesion de descubrimiento activa
    tiempo_bloque = 0;
    gamma_reenganche_ok = false;       // sin eco de re-enganche por consumir
    tiempo_ultimo_tx = 0;              // diagnostico de huecos de TX aun sin marcar
    drop_individual   = 0;             // fuente del drop sin decidir aun:
    drop_bloque       = 0;             // la primera lectura no-cero decide
    fuente_drop       = GAMMA_DROP_SIN_DECIDIR;
    drop_individual_leido = false;     // el individual aun no respondio nunca
    puerta_conocida   = false;         // detector de puerta sin estado inicial aun
    puerta_abierta    = false;
    aperturas_puerta  = 0;

    //  Fase SPC: defaults 0 en RAM; la tabla N llega de NVS en la primera
    //  lectura de meter (carga perezosa - el orden de construccion de los
    //  globales entre unidades es indefinido y NVS vive en main.cpp)
    memset(N_vueltas, 0, sizeof(N_vueltas));
    memset(R_previa,  0, sizeof(R_previa));
    memset(R_valida,  0, sizeof(R_valida));
    N_cargado = false;
}

// ============================================================================
//  FASE SPC - tabla N (vueltas de 2^32 por contador) en NVS
//
//  El meter que entrega Gamma es el total de vida del contador MODULO 2^32;
//  el servidor reconstruye el total absoluto acumulando los modulos de 10^8
//  (sus "Contadores rollover") y espera la MISMA fase que enviaba la SPC:
//  salida = (lectura + N x 2^32) mod 10^8.
//
//  - N=0 (default): maquina nueva / RAM clear - sale exacto sin calibrar
//  - Retrofit: el comando TCP carga los N una sola vez en la instalacion
//    (N = pantalla_MAIN1_en_dolares x 100 - lectura) / 2^32; verificados en
//    maq. 3: Coin In 13, Coin Out 11, Billetero 5, Cancel 3, Jackpot 0)
//  - Vigilante: cuando el contador complete su proxima vuelta de 2^32
//    (~$43M de juego despues), N++ solo y se persiste (1 escritura NVS)
//  - Un N equivocado SOLO deja la fase corrida: los deltas jamas se rompen
// ============================================================================

//  Carga perezosa de la tabla: lee NVS y, si la clave no existe (primer
//  arranque), CREA los defaults (todo 0) para que queden persistidos
void GammaProlink::Gamma_Cargar_N_Vueltas()
{
    memset(N_vueltas, 0, sizeof(N_vueltas));          // default: sin vueltas
    NVS.begin("Gamma", false);
    if (NVS.isKey("N_Vueltas"))
        NVS.getBytes("N_Vueltas", N_vueltas, sizeof(N_vueltas));
    else
        NVS.putBytes("N_Vueltas", N_vueltas, sizeof(N_vueltas));   // crea defaults
    NVS.end();
    N_cargado = true;
}

//  Persiste la tabla completa (write-through: solo cambia en la calibracion
//  del comando TCP o en una vuelta de 2^32 - rarisimo)
bool GammaProlink::Gamma_Guardar_N_Vueltas()
{
    NVS.begin("Gamma", false);
    size_t escritos = NVS.putBytes("N_Vueltas", N_vueltas, sizeof(N_vueltas));
    NVS.end();
    return escritos == sizeof(N_vueltas);
}

uint8_t GammaProlink::Gamma_Get_N_Vueltas(uint8_t idx)
{
    Gamma_N_Listos();
    if (idx >= GAMMA_N_TOTAL) return 0;
    return N_vueltas[idx];
}

//  Carga la tabla completa (orden del enum GAMMA_N_*, cantidad = GAMMA_N_TOTAL;
//  si viene menos, el resto queda como esta). Entrada del comando TCP
void GammaProlink::Gamma_Set_N_Vueltas(const uint8_t *valores, uint8_t cantidad)
{
    Gamma_N_Listos();
    if (cantidad > GAMMA_N_TOTAL) cantidad = GAMMA_N_TOTAL;
    for (uint8_t i = 0; i < cantidad; i++)
        N_vueltas[i] = valores[i];

    Gamma_Guardar_N_Vueltas();

    Serial.print("[GAMMA] Calibracion N cargada (");
    Serial.print(cantidad);
    Serial.print("/"); Serial.print(GAMMA_N_TOTAL);
    Serial.print("): ");
    for (uint8_t i = 0; i < cantidad; i++)
    {
        Serial.print(NOMBRES_N_VUELTAS[i]); Serial.print('=');
        Serial.print((unsigned long)N_vueltas[i]);
        if (i + 1 < cantidad) Serial.print(' ');
    }
    Serial.println();
}

//  Carga un solo N (idx del enum GAMMA_N_*). Entrada del comando TCP
void GammaProlink::Gamma_Set_N_Vuelta(uint8_t idx, uint8_t n)
{
    Gamma_N_Listos();
    if (idx >= GAMMA_N_TOTAL) return;
    N_vueltas[idx] = n;
    Gamma_Guardar_N_Vueltas();

    Serial.print("[GAMMA] Calibracion N: ");
    Serial.print(NOMBRES_N_VUELTAS[idx]); Serial.print(" = ");
    Serial.println((unsigned long)n);
}

//  Consulta individual del estado de un contador (N de NVS + ultima lectura
//  cruda + total verdadero implicito). Entrada del comando TCP de consulta
bool GammaProlink::Gamma_Consultar_Vueltas(uint8_t idx, uint8_t *n,
                                           uint32_t *r_ultima, uint64_t *t_total)
{
    Gamma_N_Listos();
    if (idx >= GAMMA_N_TOTAL) return false;

    if (n)        *n        = N_vueltas[idx];
    if (r_ultima) *r_ultima = R_valida[idx] ? R_previa[idx] : 0;
    if (t_total)  *t_total  = R_valida[idx]
                              ? (uint64_t)R_previa[idx] +
                                (uint64_t)N_vueltas[idx] * 4294967296ULL
                              : 0;
    return true;
}

// ----------------------------------------------------------------------------
//  Calibracion por PANTALLA: los 5 totales de vida de MAIN1 en dolares ->
//  N = (pantalla x 100) / 2^32, validado contra la ultima R y persistido.
//  ATOMICA: primero se validan los 5; si uno falla no se guarda nada.
//  El R implicito del valor tipeado es (pantalla x 100) mod 2^32 y tiene que
//  cerrar contra la R real dentro de la tolerancia (en centavos):
//    R_implicito > R + tol  -> la maquina "retrocedio": typo u otra maquina
//    R > R_implicito + tol  -> la pantalla quedo vieja: releer y reenviar
// ----------------------------------------------------------------------------
Gamma_Calib_Res GammaProlink::Gamma_Calibrar_N_Desde_Pantalla(uint32_t coin_in,
                                                              uint32_t coin_out,
                                                              uint32_t bill_in,
                                                              uint32_t cancel,
                                                              uint32_t jackpot,
                                                              uint8_t *idx_rechazado,
                                                              bool *en_creditos)
{
    //  En orden del enum GAMMA_N_*; Coin Drop y Games no vienen en el comando
    //  (conservan su N anterior)
    const uint32_t pantalla[GAMMA_N_TOTAL] = {
        coin_in,                  // GAMMA_N_COIN_IN
        coin_out,                 // GAMMA_N_COIN_OUT
        bill_in,                  // GAMMA_N_DROP
        cancel,                   // GAMMA_N_CANCEL
        jackpot,                  // GAMMA_N_JACKPOT
        GAMMA_CALIB_SIN_DATO,     // GAMMA_N_COIN_DROP
        GAMMA_CALIB_SIN_DATO,     // GAMMA_N_GAMES
    };

    Gamma_N_Listos();
    if (idx_rechazado) *idx_rechazado = 0xFF;
    if (en_creditos)   *en_creditos   = false;

#ifdef GAMMA_TEST_MQ3
    Serial.println("[GAMMA] *** MODO PRUEBA maq. 3: valores sustituidos (BORRAR #define tras validar) ***");
#endif

    // ── Fase 1: lectura R vigente en TODOS los contadores pedidos ──────────
    for (uint8_t i = 0; i < GAMMA_N_TOTAL; i++)
    {
        if (pantalla[i] == GAMMA_CALIB_SIN_DATO)
            continue;

        if (!R_valida[i])          // sin lectura vigente: nada que validar
        {
            if (idx_rechazado) *idx_rechazado = i;
            Serial.print("[GAMMA] CALIBRACION RECHAZADA: ");
            Serial.print(NOMBRES_N_VUELTAS[i]);
            Serial.println(" sin lectura vigente de la maquina (esperar 1 ciclo)");
            return GAMMA_CALIB_ERR_SIN_LECTURA;
        }
    }

    // ── Fase 2: detectar la UNIDAD del numero tipeado ───────────────────────
    //  El operario ingresa el numero de la pantalla tal cual, sin saber si su
    //  maquina la muestra en dolares (Mk6/MAIN1) o en creditos (Mk5). Llevado
    //  a creditos (la unidad del meter que lee el firmware):
    //    CREDITOS: T = pantalla (el numero del meter directamente)
    //    DOLARES : T = pantalla x 100 / valor_del_credito_en_¢ (el 02 01 05,
    //              guardado BCD en info_gamma[5]; 1¢ en toda la flota actual)
    //  Gana la interpretacion que cierre contra las lecturas R de TODOS los
    //  contadores. Se prueba CREDITOS primero: un credito tipeado pegado al
    //  tope de 2^32 podria cerrar por casualidad como dolares (N equivocado);
    //  el falso positivo inverso (dolares leidos como creditos) solo ocurre
    //  con numeros chicos, donde ambas interpretaciones dan el mismo N=0
    uint8_t  bcd        = (uint8_t)info_gamma[5];
    uint32_t valor_cent = (((uint32_t)bcd >> 4) & 0x0F) * 10 + ((uint32_t)bcd & 0x0F);
    if (valor_cent == 0)
        valor_cent = 1;            // denominacion sin llegar aun: la mas comun

    uint8_t unidad = 0xFF;         // 0 = creditos (se prueba primero), 1 = dolares
    for (uint8_t u = 0; u < 2 && unidad == 0xFF; u++)
    {
        bool cierra = true;

        for (uint8_t i = 0; i < GAMMA_N_TOTAL && cierra; i++)
        {
            if (pantalla[i] == GAMMA_CALIB_SIN_DATO)
                continue;

            uint64_t t = (u == 0) ?  (uint64_t)pantalla[i]
                                  : ((uint64_t)pantalla[i] * 100ULL) / valor_cent;
            uint32_t r_implicito = (uint32_t)(t % 4294967296ULL);
            uint64_t a = r_implicito, r = R_previa[i];

            if (a > r + GAMMA_CALIB_TOLERANCIA_C || r > a + GAMMA_CALIB_TOLERANCIA_C)
                cierra = false;
        }
        if (cierra)
            unidad = u;
    }

    if (unidad == 0xFF)            // ninguna unidad cierra: rechazo informado
    {
        for (uint8_t i = 0; i < GAMMA_N_TOTAL; i++)
        {
            if (pantalla[i] == GAMMA_CALIB_SIN_DATO)
                continue;

            uint64_t t = ((uint64_t)pantalla[i] * 100ULL) / valor_cent;
            uint32_t r_implicito = (uint32_t)(t % 4294967296ULL);
            uint64_t a = r_implicito, r = R_previa[i];

            if (a <= r + GAMMA_CALIB_TOLERANCIA_C && r <= a + GAMMA_CALIB_TOLERANCIA_C)
                continue;          // este si cierra en dolares: seguir buscando

            if (idx_rechazado) *idx_rechazado = i;

            if (a > r + GAMMA_CALIB_TOLERANCIA_C)
            {
                Serial.print("[GAMMA] CALIBRACION RECHAZADA: ");
                Serial.print(NOMBRES_N_VUELTAS[i]);
                Serial.print(" no cierra en dolares ni en creditos (R implicito ");
                Serial.print((unsigned long)r_implicito);
                Serial.print(" / lectura ");
                Serial.print((unsigned long)R_previa[i]);
                Serial.println(") - typo u otra maquina?");
                return GAMMA_CALIB_ERR_ATRAS;
            }
            else
            {
                Serial.print("[GAMMA] CALIBRACION RECHAZADA: ");
                Serial.print(NOMBRES_N_VUELTAS[i]);
                Serial.print(" pantalla vieja (lectura ");
                Serial.print((unsigned long)R_previa[i]);
                Serial.print(" > R implicito ");
                Serial.print((unsigned long)r_implicito);
                Serial.println(") - releer la pantalla y reenviar");
                return GAMMA_CALIB_ERR_DESACTUALIZADA;
            }
        }
    }

    // ── Fase 3: N por contador con la unidad detectada ──────────────────────
    uint8_t nuevo_n[GAMMA_N_TOTAL];
    memcpy(nuevo_n, N_vueltas, sizeof(nuevo_n));

    for (uint8_t i = 0; i < GAMMA_N_TOTAL; i++)
    {
        if (pantalla[i] == GAMMA_CALIB_SIN_DATO)
            continue;

        uint64_t t = (unidad == 0) ?  (uint64_t)pantalla[i]
                                   : ((uint64_t)pantalla[i] * 100ULL) / valor_cent;
        nuevo_n[i] = (uint8_t)(t / 4294967296ULL);
    }

    if (en_creditos)
        *en_creditos = (unidad == 0);

    // ── Fase 4: aplicar y persistir (rollback si NVS falla) ─────────────────
    uint8_t backup[GAMMA_N_TOTAL];
    memcpy(backup, N_vueltas, sizeof(backup));
    memcpy(N_vueltas, nuevo_n, sizeof(nuevo_n));

    if (!Gamma_Guardar_N_Vueltas())
    {
        memcpy(N_vueltas, backup, sizeof(backup));   // estado intacto
        Serial.println("[GAMMA] CALIBRACION RECHAZADA: fallo el guardado en NVS");
        return GAMMA_CALIB_ERR_NVS;
    }

    Serial.print("[GAMMA] CALIBRACION POR PANTALLA OK (pantalla en ");
    Serial.print(unidad == 0 ? "CREDITOS" : "DOLARES");
    Serial.println("):");
    for (uint8_t i = 0; i < GAMMA_N_TOTAL; i++)
    {
        if (pantalla[i] == GAMMA_CALIB_SIN_DATO)
            continue;
        Serial.print("  ");
        Serial.print(NOMBRES_N_VUELTAS[i]);
        Serial.print(": "); Serial.print((unsigned long)pantalla[i]);
        Serial.print(unidad == 0 ? " creditos" : " dolares");
        Serial.print(" -> N = "); Serial.println((unsigned long)N_vueltas[i]);
    }
    return GAMMA_CALIB_OK;
}

//  Texto del resultado para la respuesta al cliente TCP
const char *Gamma_Calibrar_Texto(Gamma_Calib_Res res)
{
    switch (res)
    {
    case GAMMA_CALIB_OK:              return "OK: N calculados, validados y guardados";
    case GAMMA_CALIB_ERR_SIN_LECTURA: return "ERROR: contador sin lectura vigente de la maquina (esperar 1 ciclo de encuestas)";
    case GAMMA_CALIB_ERR_ATRAS:       return "ERROR: el valor no cierra contra la lectura de la maquina (typo u otra maquina?)";
    case GAMMA_CALIB_ERR_DESACTUALIZADA: return "ERROR: pantalla desactualizada respecto de la maquina (releer MAIN1 y reenviar)";
    case GAMMA_CALIB_ERR_NVS:         return "ERROR: validacion OK pero fallo el guardado en NVS";
    }
    return "ERROR: codigo de calibracion desconocido";
}

//  Entrada del bloque multi-meter 02 03 03 -> indice de la tabla N.
//  OJO (verificado en maq. 1, 07-09): las entradas [2]/[4]/[5] del bloque
//  pueden traer una EPOCA DISTINTA del meter individual (mismo contador con
//  offset) y el sistema se alimenta de los INDIVIDUALES (02 04 04/05/06 -
//  los cases del bloque estan sin activar): el vigilante/N sigue SOLO a la
//  via individual; mezclar dos fuentes con offset disparaba el aviso de
//  caida en cada ciclo. [9] drop: lo decide la fuente adaptable en el loop.
//  [13]: el cancel del servidor (02 05 03 es Split Voucher, ticket-out).
//  [17]: jackpot (solo existe en el bloque)
uint8_t GammaProlink::Gamma_Idx_N_Bloque(uint8_t entrada) const
{
    switch (entrada)
    {
    case 9:  return GAMMA_N_DROP;        // [09] Drop (fuente adaptable)
    case 13: return GAMMA_N_CANCEL;      // [13] Cashout Handpays
    case 17: return GAMMA_N_JACKPOT;     // [17] Jackpot
    default: return GAMMA_IDX_SIN_N;     // [2]/[4]/[5]: vigilante en la via individual
    }
}

//  Objeto de la via individual -> indice de la tabla N. OJO con las
//  exclusiones (verificado contra pantalla, doc §9):
//   - 02 05 03 NO es cancel: es SPLIT VOUCHER PAYS (ticket-out) - el cancel
//     del sistema/servidor es el [13] del bloque. Compartir el N entre ambos
//     mezclaba meters distintos (0 vs 2.303.708.112) y disparaba el aviso
//     de caida en cada ciclo
//   - 02 03 01 (Current Credits) y las referencias de configuracion: no son
//     meters monotonicos de por vida o jamas alcanzan 2^32
uint8_t GammaProlink::Gamma_Idx_N_Objeto(const uint8_t obj[3]) const
{
    if (obj[0] == 0x02 && obj[1] == 0x04)
    {
        if (obj[2] == 0x04) return GAMMA_N_GAMES;      // Total Games Played
        if (obj[2] == 0x05) return GAMMA_N_COIN_IN;    // Total Credits Bet
        if (obj[2] == 0x06) return GAMMA_N_COIN_OUT;   // Total Credits Won
    }
    if (obj[0] == 0x04 && obj[1] == 0x09 && obj[2] == 0x01) return GAMMA_N_DROP;      // True In (drop)
    if (obj[0] == 0x04 && obj[1] == 0x04 && obj[2] == 0x01) return GAMMA_N_COIN_DROP; // Coin Drop / Gross Drop
    return GAMMA_IDX_SIN_N;
}

// ----------------------------------------------------------------------------
//  Gamma_ProcesarMeter - punto unico de salida de todo meter hacia el sistema:
//    1. Vigilante de vuelta de 2^32: mantiene N vigente (N++ y persiste)
//    2. Salida en fase SPC: (lectura + N x 2^32) mod 10^8 -> 8 digitos, el
//       mismo numero que enviaba la SPC (uint64 obligatorio: N x 2^32 no
//       cabe en 32 bits y el modulo saldria corrupto)
//  idx_n = indice de la tabla N o GAMMA_IDX_SIN_N (solo formateo %10^8).
//  Lo llaman las DOS vias de cada contador (individual y bloque): ambas
//  muestrean el mismo meter monotono, asi que la ultima lectura previa es
//  siempre coherente entre ellas
// ----------------------------------------------------------------------------
uint32_t GammaProlink::Gamma_ProcesarMeter(uint8_t idx_n, uint32_t r)
{
    Gamma_N_Listos();

    if (idx_n == GAMMA_IDX_SIN_N)
        return r % 100000000UL;

    if (R_valida[idx_n] && r < R_previa[idx_n])
    {
        if (R_previa[idx_n] > GAMMA_UMBRAL_VUELTA_32)      // vuelta REAL de 2^32
        {
            N_vueltas[idx_n]++;
            Gamma_Guardar_N_Vueltas();

            Serial.print("[GAMMA] >>> VUELTA 2^32: ");
            Serial.print(NOMBRES_N_VUELTAS[idx_n]);
            Serial.print(" -> N = ");
            Serial.println((unsigned long)N_vueltas[idx_n]);
        }
        else   // caida desde rango medio: RAM clear o box cambiado de maquina
        {
            Serial.print("[GAMMA] !!! ");
            Serial.print(NOMBRES_N_VUELTAS[idx_n]);
            Serial.println(": la lectura cayo sin vuelta posible (RAM clear / cambio de maquina?) - N sin tocar, recargar calibracion");
        }
    }
    R_previa[idx_n] = r;
    R_valida[idx_n] = true;

    return (uint32_t)(((uint64_t)r +
                       (uint64_t)N_vueltas[idx_n] * 4294967296ULL) % 100000000ULL);
}

// ----------------------------------------------------------------------------
//  Gamma_Verificar_RAMClear - auto-recalibracion ante RAM clear (idea del
//  usuario, 07-09). Al completar un ciclo de encuestas con comunicacion
//  viva: si TODOS los contadores de dinero estan en 0 (con lecturas
//  validas), la unica explicacion fisica es un RAM clear (o maquina nueva)
//  - los meters de vida jamas vuelven a 0, y menos los 5 a la vez llegando
//  cada uno por su fuente. En ese estado el total de vida real ES 0, asi
//  que N=0 para todos: se pisa la calibracion vieja (que quedo sin
//  sentido) y se avisa. One-shot: con los N ya en 0 no vuelve a disparar.
//  El aviso de caida del vigilante ("RAM clear / cambio de maquina?") se
//  dispara igual en la primera lectura en 0 - esta funcion es la que
//  RESTAURA la calibracion correcta sin intervencion humana.
// ----------------------------------------------------------------------------
void GammaProlink::Gamma_Verificar_RAMClear()
{
    Gamma_N_Listos();

    //  ¿Hay algo que resetear? (evita disparar en maquinas nuevas y repetir)
    bool hay_n = false;
    for (uint8_t i = 0; i < GAMMA_N_TOTAL && !hay_n; i++)
        if (N_vueltas[i] != 0) hay_n = true;
    if (!hay_n)
        return;

    //  Los 5 contadores de dinero del comando de calibracion: todos con
    //  lectura valida y todos exactamente en 0
    static const uint8_t IDX_DINERO[] = {
        GAMMA_N_COIN_IN, GAMMA_N_COIN_OUT, GAMMA_N_DROP,
        GAMMA_N_CANCEL,  GAMMA_N_JACKPOT
    };
    for (uint8_t i = 0; i < sizeof(IDX_DINERO); i++)
    {
        if (!R_valida[IDX_DINERO[i]] || R_previa[IDX_DINERO[i]] != 0)
            return;          // no es un RAM clear: no tocar nada
    }

    //  RAM clear confirmado: el total de vida volvio a 0 -> N=0 para todos
    memset(N_vueltas, 0, sizeof(N_vueltas));
    Gamma_Guardar_N_Vueltas();

    Serial.println("[GAMMA] >>> RAM CLEAR DETECTADO: todos los contadores de dinero en 0");
    Serial.println("[GAMMA]     Calibracion N devuelta a 0 (total de vida real = 0)");
    Serial.println("[GAMMA]     Reinicio: re-siembra de las bases de deteccion de cambios");
    delay(1000);
    ESP.restart();
}

// ============================================================================
//  CRC-16/XMODEM - algoritmo verificado contra la maquina real
//  (reproduce el 100% de las 1561 tramas de la captura del 27-08-2026)
// ============================================================================
uint16_t GammaProlink::Gamma_CRC16(const uint8_t *datos, size_t longitud)
{
    uint16_t crc = 0x0000;                  // init

    for (size_t i = 0; i < longitud; i++)
    {
        crc ^= (uint16_t)datos[i] << 8;     // MSB first, sin reflexion

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x8000)
                crc = (uint16_t)((crc << 1) ^ 0x1021);   // poly
            else
                crc = (uint16_t)(crc << 1);
        }
    }
    return crc;                              // xorout = 0x0000
}

// ============================================================================
//  Devuelve el slot actual (0x40..0x78) y avanza la rotacion.
//  Cada transmision (poll, query o lo que sea) consume un slot (Manual §5).
// ============================================================================
uint8_t GammaProlink::Gamma_TomarSlot()
{
    uint8_t slot = 0x40 + 8 * indice_slot;
    indice_slot = (indice_slot + 1) % 8;
    return slot;
}

// ============================================================================
//  Patron de envio por UART - el mismo de RS232.h: esperar que el TX anterior
//  termino, escribir, y esperar que este termine (la trama sale "de corrido").
//
//  OJO con el timeout de uart_wait_tx_done: esta en TICKS de FreeRTOS
//  (~1 ms cada uno), no en milisegundos. Con UART2 a 19200 baudios la QUERY
//  de 10 bytes tarda ~5.2 ms en salir: un timeout de 3 ticks abortaba el
//  firmware con ESP_ERR_TIMEOUT. Se usa pdMS_TO_TICKS(100) (margen >10x) y
//  fallo NO fatal: se avisa por serial y se sigue (la trama igual termina de
//  salir sola del FIFO).
// ============================================================================

bool GammaProlink::Maquina_Aristocrat_Gamma(uint16_t Tipo_Maq)
{
    switch (Tipo_Maq)
    {
    case 19:
        return true;
        break;

    default:
        break;
    }
    return false;
}



void GammaProlink::Gamma_EnviarTrama(const uint8_t *trama, size_t tamano)
{
    //  DIAGNOSTICO de caidas: la maquina declara "link Down" cuando deja de
    //  recibir transmisiones (Manual §6.3). Con la task a 100 ms el hueco
    //  entre transmisiones nunca deberia pasar de ese orden. Si este aviso
    //  aparece justo antes de un "Enlace caido", la task de encuestas estuvo
    //  detenida ese tiempo (SD, WiFi, TFT...) y ESA es la causa de la caida.
    //  Si no aparece, la caida ocurrio con cadencia perfecta -> lado maquina
    uint32_t ahora = millis();
    if (tiempo_ultimo_tx != 0 &&
        ahora - tiempo_ultimo_tx > GAMMA_GAP_TX_AVISO_MS)
    {
        #ifdef Debug_Gamma
        Serial.print("[GAMMA] AVISO: hueco de ");
        Serial.print(ahora - tiempo_ultimo_tx);
        Serial.println(" ms entre transmisiones (task de encuestas detenida?)");
        #endif
    }
    tiempo_ultimo_tx = ahora;

    uart_wait_tx_done(GAMMA_PORTA_SERIALE, pdMS_TO_TICKS(100));            // espera envio anterior
    uart_write_bytes(GAMMA_PORTA_SERIALE, (const char *)trama, tamano);    // envia la trama
    ultimo_seq_enviado = trama[0];                                         // para validar la respuesta
    if (uart_wait_tx_done(GAMMA_PORTA_SERIALE, pdMS_TO_TICKS(100)) != ESP_OK)
    {
        #ifdef Debug_Gamma
        Serial.println("[GAMMA] AVISO: wait_tx_done vencio (TX lenta o atascada)");
        #endif
    }
}

// ============================================================================
//  (Solo con Debug_Gamma) Imprime por serial una trama de TX en hexadecimal,
//  etiquetada segun su tipo (POLL / QUERY / RE-ENGANCHE)
// ============================================================================
#ifdef Debug_Gamma
static void Gamma_ImprimirTx(const char *etiqueta, const uint8_t *trama, size_t tamano)
{
    Serial.print("[GAMMA] TX ");
    Serial.print(etiqueta);
    Serial.print(": ");
    for (size_t i = 0; i < tamano; i++)
    {
        if (trama[i] < 0x10) Serial.print('0');
        Serial.print(trama[i], HEX);
        Serial.print(' ');
    }
    Serial.println();
}
#endif

// ============================================================================
//  POLL vacio del slot actual: [SEQ][00][CRC MSB][CRC LSB]
//  Es el keep-alive del enlace y el "¿ya?" que hace entregar la DATA pendiente.
// ============================================================================
uint8_t GammaProlink::Gamma_EnviarPoll()
{
    uint8_t seq = Gamma_TomarSlot();

    uint8_t trama[4] = { seq, 0x00, 0x00, 0x00 };
    uint16_t crc = Gamma_CRC16(trama, 2);          // CRC sobre SEQ+LEN
    trama[2] = (uint8_t)(crc >> 8);                // MSB primero
    trama[3] = crc & 0xFF;

    #ifdef Debug_Gamma
    Gamma_ImprimirTx("POLL", trama, sizeof(trama));
    #endif
    Gamma_EnviarTrama(trama, sizeof(trama));
    return seq;
}


bool GammaProlink::Flag_handle_Query()
{
    return Flag_Query_Gamma;
}

// ============================================================================
//  Consulta BAJO DEMANDA del bloque multi-meter 02 03 03 (24 valores - §7.6).
//  Gamma_Activar_Query_Bloque: la enciende desde donde sea (Web_Debug, etc.).
// ============================================================================
void GammaProlink::Gamma_Activar_Query_Bloque()
{
    Flag_Query_Gamma = true;
}

// ============================================================================
//  Gamma_ConsultarBloque - la llama la task de encuestas en cada tick
//  mientras Flag_handle_Query() este en true (su rama del if en RS232.h):
//    tick 1 -> envia la QUERY del bloque (una sola vez)
//    resto -> envia polls ("¿ya?") hasta que la DATA llegue (el despachador
//             la imprime indexada y cierra la sesion) o venza la ventana.
//  No toca el avance del ciclo normal: el objeto no queda pendiente.
// ============================================================================
void GammaProlink::Gamma_ConsultarBloque()
{
    if (!bloque_pendiente)
    {
        bloque_pendiente = true;
        tiempo_bloque    = millis();
        Gamma_EnviarQuery(0x02, 0x03, 0x03);   // QUERY del bloque
        hay_objeto_pendiente = false;          // que no avance el ciclo normal
        #ifdef Debug_Gamma
        Serial.println("[GAMMA] Consulta de bloque 02 03 03 iniciada");
        #endif
    }
    else
    {
        Gamma_EnviarPoll();                    // la DATA llega con este poll
        if (millis() - tiempo_bloque > 3000)   // ventana vencida: rendirse
        {
            bloque_pendiente = false;
            Flag_Query_Gamma  = false;
            #ifdef Debug_Gamma
            Serial.println("[GAMMA] Consulta de bloque: sin respuesta (venzio)");
            #endif
        }
    }
}



// ============================================================================
//  QUERY de lectura de un objeto (contador):
//  [SEQ][06][05][30][02][clase][subgrupo][meter][CRC MSB][CRC LSB]
//  La maquina responde un ACK vacio con este SEQ; la DATA llega con el POLL
//  del slot siguiente (modelo diferido - Manual §6).
// ============================================================================
uint8_t GammaProlink::Gamma_EnviarQuery(uint8_t clase, uint8_t subgrupo, uint8_t meter)
{
    uint8_t seq = Gamma_TomarSlot();

    //  Recordar el objeto pedido: la DATA que lo eche (rx[5..7]) confirmara
    //  la respuesta y hara avanzar el ciclo de encuestas
    objeto_pendiente[0] = clase;
    objeto_pendiente[1] = subgrupo;
    objeto_pendiente[2] = meter;
    hay_objeto_pendiente = true;

    uint8_t trama[10] = { seq, 0x06, 0x05, 0x30, 0x02, clase, subgrupo, meter, 0x00, 0x00 };
    uint16_t crc = Gamma_CRC16(trama, 8);          // CRC sobre SEQ+LEN+PAYLOAD
    trama[8] = (uint8_t)(crc >> 8);                // MSB primero
    trama[9] = crc & 0xFF;

    #ifdef Debug_Gamma
    Gamma_ImprimirTx("QUERY", trama, sizeof(trama));
    #endif
    Gamma_EnviarTrama(trama, sizeof(trama));
    return seq;
}

// ============================================================================
//  Poll del slot 0x40: arranque del enlace o re-enganche tras una caida.
//  La maquina readquiere la comunicacion SOLO al ver este slot (Manual §6.3);
//  despues del eco espera la secuencia normal (siguiente = 0x48).
// ============================================================================
uint8_t GammaProlink::Gamma_EnviarPollArranque()
{
    indice_slot = 0;                   // reinicia la rotacion en 0x40
    uint8_t seq = Gamma_TomarSlot();   // siempre 0x40

    uint8_t trama[4] = { seq, 0x00, 0x00, 0x00 };
    uint16_t crc = Gamma_CRC16(trama, 2);
    trama[2] = (uint8_t)(crc >> 8);
    trama[3] = crc & 0xFF;

    #ifdef Debug_Gamma
    Gamma_ImprimirTx("RE-ENGANCHE (slot arranque)", trama, sizeof(trama));
    #endif
    Gamma_EnviarTrama(trama, sizeof(trama));
    return seq;
}

// ============================================================================
//  Gamma_SimularQuery - version "sin cable" de Gamma_EnviarQuery:
//  deja el mismo estado (slot consumido, objeto pendiente, ultimo SEQ) pero
//  no escribe nada en el UART. Es la que usan los autotests de banco, que
//  asi no dependen del driver instalado ni transmiten al aire.
// ============================================================================
uint8_t GammaProlink::Gamma_SimularQuery(uint8_t clase, uint8_t subgrupo, uint8_t meter)
{
    uint8_t seq = Gamma_TomarSlot();       // misma rotacion que la QUERY real

    objeto_pendiente[0] = clase;
    objeto_pendiente[1] = subgrupo;
    objeto_pendiente[2] = meter;
    hay_objeto_pendiente = true;
    ultimo_seq_enviado  = seq;             // la DATA simulada debe traer este SEQ
    return seq;
}

// ============================================================================
//  Ciclo de encuestas Gamma (tipo de maquina 19).
//  Una llamada = una transmision. El avance de meter lo decide el receptor:
//  solo cuando llega la DATA validada (CRC + SEQ + eco del objeto) se pasa al
//  siguiente contador. Si vence la ventana se reintenta el MISMO meter, y tras
//  agotar los reintentos se re-engancha el enlace con el slot 0x40.
// ============================================================================
void GammaProlink::Encuestas_Maquinas_Gamma()
{
    // ── 1. El RX manda: si llego la DATA del meter esperado, avanzar ──────
    if (flag_meter_recibido)
    {
        flag_meter_recibido = false;
        reintentos_query = 0;

        conta_meters++;
        if (conta_meters >= NUM_METERS_GAMMA)
        {
            conta_meters = 0;
            flag_ciclo_completo = true;        // la task lo pasa a flag_ultimo_contador_Ok
            Ultimo_Counter_ = true;            // igual que el SAS tras su bloque 0x1F:
                                               // Check_Comunicacion_Maq la consume para
                                               // declarar la primera conexion OK
            Counter_Final = true;              // LA puerta de reconexion: sin esta,
                                               // tras una caida Check_Comunicacion_Maq
                                               // queda esperando en Fallo_Comunicacion
                                               // y Comunicacion_Maq nunca vuelve a true
                                               flag_ultimo_contador_Ok=true;
            Gamma_Verificar_RAMClear();        // ¿todos los meters de dinero en 0? -> N=0
            #ifdef Debug_Gamma
            Serial.println("[GAMMA] == Ciclo de contadores completo ==");
            #endif
        }
        fase_encuesta = GAMMA_FASE_QUERY;      // siguiente meter
    }

    // ── 2. Transmitir segun la fase ────────────────────────────────────────
    switch (fase_encuesta)
    {
    case GAMMA_FASE_QUERY:
        #ifdef Debug_Gamma
        Serial.print("[GAMMA] QUERY ");
        Serial.println(NOMBRES_METERS_GAMMA[conta_meters]);
        #endif
        Gamma_EnviarQuery(TABLA_METERS_GAMMA[conta_meters][0],
                          TABLA_METERS_GAMMA[conta_meters][1],
                          TABLA_METERS_GAMMA[conta_meters][2]);
        tiempo_query   = millis();             // marca el inicio de la ventana
        fase_encuesta  = GAMMA_FASE_ESPERA;
        break;

    case GAMMA_FASE_ESPERA:
        Gamma_EnviarPoll();                    // el "¿ya?" - la DATA llega como respuesta
        if (millis() - tiempo_query > GAMMA_VENTANA_ESPERA_MS)
        {
            reintentos_query++;
            if (reintentos_query >= GAMMA_REINTENTOS_MAX)
            {
                //  Objeto sin DATA tras los reintentos: saltarlo y seguir con
                //  el siguiente. Los objetos raros responden ACK vacio sin
                //  DATA (Manual §9.3) y quedarse esperandolo colgaria el
                //  ciclo para siempre. Imprescindible para el barrido de
                //  objetos en exploracion
                //
                //  Si el saltado era el individual del drop, cuenta como
                //  "respondio" (tuvo su chance y no trajo nada): en las
                //  maquinas con el 04 09 01 muerto esto habilita al bloque
                //  [09] como fuente del drop
                if (hay_objeto_pendiente &&
                    objeto_pendiente[0] == 0x04 &&
                    objeto_pendiente[1] == 0x09 &&
                    objeto_pendiente[2] == 0x01)
                {
                    drop_individual_leido = true;
                }
                conta_meters = (conta_meters + 1) % NUM_METERS_GAMMA;
                if (conta_meters == 0)         // el salto cerro el ciclo
                {
                    flag_ciclo_completo = true;
                    Ultimo_Counter_     = true;
                    Counter_Final       = true;
                    Gamma_Verificar_RAMClear();   // ¿todos los meters de dinero en 0? -> N=0
                }
                //  Enlace caido: re-enganchar con polls del slot 0x40 (Manual §6.3)
                reintentos_query   = 0;
                gamma_reenganche_ok = false;   // eco de un RESYNC anterior, fuera
                fase_encuesta     = GAMMA_FASE_RESYNC;
                tiempo_query      = millis();
                #ifdef Debug_Gamma
                Serial.print("[GAMMA] Enlace caido t=");
                Serial.print(millis());
                Serial.println(" -> re-enganche con slot 0x40");
                #endif
            }
            else
            {
                fase_encuesta = GAMMA_FASE_QUERY;   // reintentar el MISMO meter
            }
        }
        break;

    case GAMMA_FASE_RESYNC:
        //  El re-enganche se VALIDA con el eco: si la maquina contesto el
        //  poll de arranque (bandera izada por el receptor al ver el eco
        //  0x40), retomar el ciclo de inmediato. La maquina ya readquirio
        //  el enlace y espera la secuencia normal (siguiente slot 0x48 -
        //  Manual §6.3), que es justo el que toma la siguiente QUERY
        if (gamma_reenganche_ok)
        {
            gamma_reenganche_ok = false;
            fase_encuesta = GAMMA_FASE_QUERY;
            #ifdef Debug_Gamma
            Serial.print("[GAMMA] Re-enganche confirmado (eco del 0x40) tras ");
            Serial.print(millis() - tiempo_query);
            Serial.println(" ms de re-enganche -> retomar ciclo");
            #endif
            break;
        }
        Gamma_EnviarPollArranque();            // poll del slot 0x40 hasta re-enganchar
        if (millis() - tiempo_query > GAMMA_VENTANA_RESYNC_MS)
        {
            fase_encuesta = GAMMA_FASE_QUERY;  // volver a intentar el meter pendiente
        }
        break;
    }
}

// ============================================================================
//  ¿Es uno de los 8 SEQ validos del protocolo? (0x40..0x78 paso 8)
//  Filtra la basura del arranque y residuos de otros protocolos.
// ============================================================================
bool GammaProlink::Gamma_EsSlotValido(uint8_t byte) const
{
    return (byte >= 0x40 && byte <= 0x78 && (byte & 0x07) == 0x00);
}

// ============================================================================
//  Gamma_ProcesarRx - receptor del protocolo Gamma.
//  Se alimenta con los bytes crudos que llegan del UART2 (Task UART) y va
//  extrayendo tramas completas por ESTRUCTURA (no por tiempo):
//    1. El primer byte debe ser un SEQ valido (0x40..0x78)
//    2. El segundo byte es LEN: la trama completa mide LEN+4
//    3. Con los LEN+4 bytes, el CRC debe dar residuo 0
//  Si algo no cuadra se descarta UN byte y se reintenta (resincronizacion).
// ============================================================================
void GammaProlink::Gamma_ProcesarRx(const uint8_t *datos, size_t longitud)
{
    // ── 1. Acumular los bytes que llegaron ──────────────────────────────────
    for (size_t i = 0; i < longitud; i++)
    {
        if (rx_long >= sizeof(rx_acum))       // acumulador lleno: descartar todo
            rx_long = 0;
        rx_acum[rx_long++] = datos[i];
    }

    //  NOTA de depuracion: el dump crudo de entrada se retiro. La visibilidad
    //  ahora es selectiva y vive en Gamma_DespacharTrama: lo CONOCIDO (meters,
    //  ACK, identidad) solo imprime con Debug_Gamma; lo DESCONOCIDO imprime
    //  SIEMPRE -> abriendo la puerta de la maquina, cualquier trama nueva
    //  (evento) resalta en un log silencioso.
    // ── 2. Extraer todas las tramas completas que haya ──────────────────────
    while (rx_long >= 4)
    {
        if (!Gamma_EsSlotValido(rx_acum[0]))            // byte de arranque invalido
        {
            memmove(rx_acum, rx_acum + 1, rx_long - 1); // descartar 1 byte y seguir
            rx_long--;
            continue;
        }

        uint16_t total = (uint16_t)rx_acum[1] + 4;      // longitud total de la trama
        if (total > sizeof(rx_acum))                    // LEN absurdo: descartar el SEQ
        {
            memmove(rx_acum, rx_acum + 1, rx_long - 1);
            rx_long--;
            continue;
        }
        if (rx_long < total)                            // trama incompleta: esperar mas bytes
            break;

        if (Gamma_CRC16(rx_acum, total) != 0)           // CRC invalido: descartar 1 byte
        {
            memmove(rx_acum, rx_acum + 1, rx_long - 1);
            rx_long--;
            continue;
        }

        //  Trama completa y con CRC valida
        uint8_t trama[sizeof(rx_acum)];
        memcpy(trama, rx_acum, total);
        memmove(rx_acum, rx_acum + total, rx_long - total);
        rx_long -= total;

        Gamma_DespacharTrama(trama);
    }
}

// ============================================================================
//  Gamma_ExtraerPaytable - copia al destino (6 chars fijos, sin terminador)
//  la primera cadena imprimible (>= 4 chars) del bloque de identidad: es la
//  tabla de pagos ("G1607" en la captura del 27-08). Rellena con espacios.
// ============================================================================
static void Gamma_ExtraerPaytable(const uint8_t *payload, size_t largo, char *destino)
{
    size_t i = 7;                            // salta SIZE/31/82/eco/pad/count
    while (i < largo)
    {
        if (payload[i] >= 0x20 && payload[i] <= 0x7E)
        {
            size_t fin = i;
            while (fin < largo && payload[fin] >= 0x20 && payload[fin] <= 0x7E)
                fin++;
            if (fin - i >= 4)                // candidata a tabla de pagos
            {
                size_t n = 0;
                for (; i < fin && n < 6; i++, n++)
                    destino[n] = (char)payload[i];
                for (; n < 6; n++)
                    destino[n] = ' ';        // relleno a 6 chars
                return;
            }
            i = fin;
        }
        else
            i++;
    }
}

// ============================================================================
//  Gamma_ExtraerPorcentaje - busca "NN.NN%" en el bloque de identidad y
//  escribe 4 digitos (2 enteros + 2 decimales redondeados) al destino:
//  "87.928%" -> "8793" (87,93). Si no hay '%', el destino queda como estaba.
// ============================================================================
static void Gamma_ExtraerPorcentaje(const uint8_t *payload, size_t largo, char *destino)
{
    size_t pos = 0;                          // 1. localizar el '%'
    while (pos < largo && payload[pos] != '%')
        pos++;
    if (pos == 0 || pos >= largo)
        return;

    char numero[12];                         // 2. recoger el numero previo
    uint8_t n = 0;
    size_t i = pos;
    while (i > 0 && n < sizeof(numero) - 1)
    {
        char c = (char)payload[i - 1];
        if ((c >= '0' && c <= '9') || c == '.')
            numero[n++] = c;
        else
            break;
        i--;
    }
    if (n == 0)
        return;
    numero[n] = '\0';
    for (uint8_t a = 0; a < n / 2; a++)      // quedo al reves: invertir
    {
        char t = numero[a];
        numero[a] = numero[n - 1 - a];
        numero[n - 1 - a] = t;
    }

    uint32_t ent = 0, dec = 0;               // 3. partir y redondear a 2 dec
    uint8_t  ndec = 0;
    bool     en_dec = false;
    for (uint8_t k = 0; k < n; k++)
    {
        char c = numero[k];
        if (c == '.')
            en_dec = true;
        else if (!en_dec)
            ent = ent * 10 + (c - '0');
        else if (ndec < 2)
        {
            dec = dec * 10 + (c - '0');
            ndec++;
        }
        else if ((c - '0') >= 5)             // 3er decimal: redondear
        {
            dec++;
            if (dec >= 100) { dec = 0; ent++; }
            break;
        }
    }
    while (ndec < 2) { dec *= 10; ndec++; }
    if (ent > 99) ent = 99;                  // el campo son 2+2 digitos

    destino[0] = (char)('0' + (ent / 10));
    destino[1] = (char)('0' + (ent % 10));
    destino[2] = (char)('0' + (dec / 10));
    destino[3] = (char)('0' + (dec % 10));
}

// ============================================================================
//  Gamma_PublicarInfoSiCompleta - setea Informacion_Maquina SOLO cuando todas
//  las fuentes individuales (denominacion 02 01 05 + bloque identidad
//  03 01 00) llegaron en algun momento. Asi nunca se publica un buffer a
//  medias: hasta completar, el campo queda en su valor por defecto (todo 0).
//  La info de maquina es configuracion estatica: las banderas NO se bajan
//  ante caidas de enlace; se re-aprenden en el primer ciclo tras reinicio.
// ============================================================================
void GammaProlink::Gamma_PublicarInfoSiCompleta()
{
    if ((info_flags & GAMMA_INFO_COMPLETA) != GAMMA_INFO_COMPLETA)
        return;                        // falta alguna fuente: no setear parcial

    contadores.Set_Contadores(Informacion_Maquina, info_gamma);

    #ifdef Debug_Gamma
    Serial.print("[GAMMA] Informacion_Maquina COMPLETA y seteada: '");
    for (uint8_t i = 0; i < 20; i++)
        Serial.write(info_gamma[i] < 0x20 ? '.' : info_gamma[i]);
    Serial.println("'");
    #endif
}

// ============================================================================
//  Gamma_DecidirFuenteDrop - regla de compatibilidad del drop (Total Drop /
//  True In) entre poblaciones de maquinas. Se evalua tras cada lectura del
//  objeto 04 09 01 y de la entrada [09] del bloque 02 03 03:
//
//    individual > 0              -> fuente = INDIVIDUAL (gana siempre, aun
//                                   si el bloque trae un valor distinto)
//    individual == 0 y bloque >0 -> fuente = BLOQUE, PERO SOLO si el
//                                   individual ya respondio al menos una vez
//                                   (drop_individual_leido, aunque haya
//                                   respondido 0) o fue saltado por no
//                                   responder: el bloque NO decide solo
//                                   porque se haya leido primero (evita el
//                                   transitorio bloque->individual en
//                                   maquinas con ambos lados vivos)
//    ambos en 0                  -> se mantiene como este: SIN_DECIDIR no
//                                   setea nada, y la primera lectura no-cero
//                                   decide sola (cubre la maquina recien
//                                   RAM-cleared que un scanner de arranque
//                                   clasificaria mal)
//
//  La decision es sticky: exactamente UNA via setea por ciclo, sin el
//  0/valor alternado de setear por ambos lados. Si la maquina cambia de
//  comportamiento (p.ej. RAM clear a mitad de vida), la regla re-decide
//  sola en cuanto vuelve una lectura no-cero.
// ============================================================================
void GammaProlink::Gamma_DecidirFuenteDrop()
{
    uint8_t fuente_anterior = fuente_drop;

    if (drop_individual > 0)
        fuente_drop = GAMMA_DROP_INDIVIDUAL;
    else if (drop_bloque > 0 && drop_individual_leido)
        fuente_drop = GAMMA_DROP_BLOQUE;
    //  bloque > 0 con el individual sin responder aun, o ambos en 0:
    //  no cambiar la decision actual

    #ifdef Debug_Gamma
    if (fuente_drop != fuente_anterior)
    {
        Serial.print("[GAMMA] Fuente del drop -> ");
        Serial.println(fuente_drop == GAMMA_DROP_INDIVIDUAL
                           ? "individual (04 09 01)"
                           : "bloque 02 03 03 [09]");
    }
    #endif
}

// ============================================================================
//  Gamma_DespacharTrama - procesa una trama ya validada por CRC.
//    [0]=SEQ  [1]=LEN  [2..]=payload  [ultimos 2]=CRC
//    ACK/poll eco: LEN==0 -> no hay nada que procesar
//    DATA: payload = [SIZE][31][82][clase][sub][meter][00][VALOR BE32][00]
//
//  La funcion es PERMISIVA: cualquier trama con CRC valido refresca el
//  timeout de comunicacion (Datos_OK + Bandera_RS232_F) antes de filtrar,
//  y las estructuras que todavia no sabemos decodificar simplemente se
//  descartan (o se dumpean con Debug_Gamma) sin bloquear el flujo.
// ============================================================================
#ifdef GAMMA_TEST_MQ3
// ----------------------------------------------------------------------------
//  Gamma_Test_Sustituir_MQ3 - PRUEBA maq. 3 (se compila solo con el #define;
//  BORRAR al terminar). La maquina real conectada genera TODO el trafico
//  (enlace, SEQ, CRC, timing del ciclo) y esta funcion parchea los VALORES
//  de la DATA ya validada con los contadores de la maq. 3 (captura del
//  04-09-2026), justo despues de la validacion de CRC del receptor: el
//  resto del pipeline (vigilante, fase SPC, string de 8 digitos,
//  Set_Contadores) corre exactamente como con la maq. 3 al otro lado.
//  Devuelve la trama parcheada (buffer interno) o la original si el objeto
//  no esta en la tabla (denominacion, puerta, identidad y referencias pasan
//  tal cual llegan).
// ----------------------------------------------------------------------------
static const uint8_t *Gamma_Test_Sustituir_MQ3(const uint8_t *trama, uint8_t len)
{
    static uint8_t trama_test[128];     // solo la toca la task UART (RX)

    const uint8_t c = trama[5], s = trama[6], m = trama[7];

    //  Meters simples (LEN 0C): [SEQ][0C][0A][31][82][c][s][m][00][BE32][00][CRC]
    if (len == 0x0C)
    {
        static const struct { uint8_t c, s, m; uint32_t v; } METERS_MQ3[] = {
            { 0x02, 0x04, 0x04,   3381566 },   // Total Games Played
            { 0x02, 0x04, 0x05, 843949152 },   // Coin In   ($566.785.240, 13 vueltas)
            { 0x02, 0x04, 0x06, 2287693744 },  // Coin Out  ($495.323.340, 11 vueltas)
            { 0x02, 0x05, 0x03,          0 },  // Split Voucher Pays ($0 en maq. 3)
            { 0x04, 0x09, 0x01,          0 },  // True In individual (muerto en maq. 3)
            { 0x04, 0x04, 0x01,          0 },  // Coin Drop / monedas ($0)
            { 0x02, 0x03, 0x01,          0 },  // Current Credits
        };
        for (uint8_t i = 0; i < sizeof(METERS_MQ3) / sizeof(METERS_MQ3[0]); i++)
        {
            if (METERS_MQ3[i].c != c || METERS_MQ3[i].s != s || METERS_MQ3[i].m != m)
                continue;

            memcpy(trama_test, trama, (size_t)len + 4);
            trama_test[9]  = (uint8_t)(METERS_MQ3[i].v >> 24);
            trama_test[10] = (uint8_t)(METERS_MQ3[i].v >> 16);
            trama_test[11] = (uint8_t)(METERS_MQ3[i].v >> 8);
            trama_test[12] = (uint8_t)METERS_MQ3[i].v;
            return trama_test;
        }
        return trama;
    }

    //  BLOQUE MULTI-METER 02 03 03 (LEN 68): parchea las entradas
    //  identificadas con los valores de la maq. 3 (el resto, sin asignar,
    //  queda como lo mande la maquina real)
    if (len == 0x68 && c == 0x02 && s == 0x03 && m == 0x03)
    {
        memcpy(trama_test, trama, (size_t)len + 4);
        static const struct { uint8_t e; uint32_t v; } BLOQUE_MQ3[] = {
            {  1,          0 },     // [01] Current Credits
            {  2,    3381566 },     // [02] Total Games Played
            {  4,  843949152 },     // [04] Coin In
            {  5, 2287693744 },     // [05] Coin Out
            {  9,  859963520 },     // [09] Drop / Bill In ($223.348.000, 5 v.)
            { 13, 2303708112 },     // [13] Cancel credits ($151.886.100, 3 v.)
            { 17,  101050000 },     // [17] Jackpot ($1.010.500)
        };
        for (uint8_t i = 0; i < sizeof(BLOQUE_MQ3) / sizeof(BLOQUE_MQ3[0]); i++)
        {
            trama_test[9  + 4 * BLOQUE_MQ3[i].e] = (uint8_t)(BLOQUE_MQ3[i].v >> 24);
            trama_test[10 + 4 * BLOQUE_MQ3[i].e] = (uint8_t)(BLOQUE_MQ3[i].v >> 16);
            trama_test[11 + 4 * BLOQUE_MQ3[i].e] = (uint8_t)(BLOQUE_MQ3[i].v >> 8);
            trama_test[12 + 4 * BLOQUE_MQ3[i].e] = (uint8_t)BLOQUE_MQ3[i].v;
        }
        return trama_test;
    }

    return trama;
}
#endif

void GammaProlink::Gamma_DespacharTrama(const uint8_t *trama)
{
    //  Mismo papel que "Datos_OK=true; Bandera_RS232_F=Bandera_RS232;" en el
    //  receptor SAS (Verifica_buffer_Maq). Va ANTES de todo filtro para que
    //  tambien cuenten los ACK/ecos del poll, que son el keep-alive del enlace
    Datos_OK = true;
    Bandera_RS232_F = Bandera_RS232;

    uint8_t len = trama[1];

    if (len == 0)                       // eco del poll / ACK vacio
    {
        //  VALIDACION del re-enganche: si estamos en RESYNC y la maquina
        //  contesto el eco del poll de arranque (0x40), el enlace volvio a
        //  la vida. Se iza la bandera y la fase RESYNC corta su ventana en
        //  cuanto la ve, en lugar de quemar el segundo completo
        if (fase_encuesta == GAMMA_FASE_RESYNC && trama[0] == 0x40)
            gamma_reenganche_ok = true;
        return;                         // no hay nada mas que procesar
    }

    const uint8_t *p = trama + 2;       // inicio del payload

#ifdef GAMMA_TEST_MQ3
    //  PRUEBA maq. 3: la trama ya paso CRC/SEQ/estructura; a partir de aca
    //  sus VALORES son los de la maq. 3 (borrar con el #define)
    trama = Gamma_Test_Sustituir_MQ3(trama, len);
    p = trama + 2;
#endif

    if (len < 2 || p[1] != 0x31)        // no es bloque de datos (0x31)
    {
        #ifdef Debug_Gamma               // estructura no-0x31: solo con debug
        Serial.print("[GAMMA] TRAMA DESCONOCIDA no-0x31 (len="); Serial.print(len);
        Serial.print("): ");
        for (uint8_t i = 0; i < len + 4 && i < sizeof(rx_acum); i++)
        {
            if (trama[i] < 0x10) Serial.print('0');
            Serial.print(trama[i], HEX); Serial.print(' ');
        }
        Serial.println();
        #endif
        return;
    }

    // ── BLOQUE MULTI-METER 02 03 03 (§7.6): 24 contadores BE32 seguidos ────
    //  Objeto del ciclo de encuestas (TABLA_METERS_GAMMA) y tambien de la
    //  consulta bajo demanda. Estructura:
    //    [SEQ][LEN=0x68][SIZE][31][82][02][03][03][00][24 x BE32][00][CRC]
    //  trama completa = LEN+4 = 108 bytes, la entrada i esta en pos 9+4i
    //  (entradas = (LEN-8)/4). Mismas validaciones que un meter simple
    //  (SEQ + eco del objeto) para izar flag_meter_recibido y avanzar el
    //  ciclo. Si la DATA nunca llegara, Encuestas_Maquinas_Gamma SALTA el
    //  objeto tras GAMMA_REINTENTOS_MAX (rama de re-enganche): el bloque no
    //  puede colgar las consultas. Por ahora solo se lista (Debug_Gamma):
    //  los contadores que alimenta el firmware siguen llegando por los
    //  meters individuales, que son los verificados contra la maquina real
    if (p[3] == 0x02 && p[4] == 0x03 && p[5] == 0x03 && len >= 0x0C &&
        ((len - 8) & 0x03) == 0)
    {
        //  Filtro SEQ: la DATA debe traer el SEQ de nuestra ultima TX
        if (trama[0] != ultimo_seq_enviado)
        {
            #ifdef Debug_Gamma
            Serial.print("[GAMMA] BLOQUE 02 03 03 descartado: SEQ 0x");
            Serial.print(trama[0], HEX);
            Serial.print(" != esperado 0x");
            Serial.println(ultimo_seq_enviado, HEX);
            #endif
            return;
        }

        //  ¿DATA del objeto pendiente del ciclo? -> consumar y avanzar
        if (hay_objeto_pendiente &&
            objeto_pendiente[0] == 0x02 &&
            objeto_pendiente[1] == 0x03 &&
            objeto_pendiente[2] == 0x03)
        {
            hay_objeto_pendiente = false;
            flag_meter_recibido = true;     // avanza al siguiente meter
        }

        //  Sesion bajo demanda (Gamma_ConsultarBloque): cerrarla tambien
        if (bloque_pendiente)
        {
            bloque_pendiente = false;
            Flag_Query_Gamma  = false;
        }

        // ------------------------------------------------------------
        //  Extraccion y seteo de las 24 entradas (mapeo verificado contra
        //  la maquina real). El SETEO corre SIEMPRE, aunque Debug_Gamma
        //  este apagado; el listado por serial solo con Debug_Gamma
        // ------------------------------------------------------------
        uint8_t n = (len - 8) / 4;              // entradas BE32 del bloque
        if (n > 24) n = 24;

        #ifdef Debug_Gamma
        Serial.print("[GAMMA] BLOQUE 02 03 03 - ");
        Serial.print(n);
        Serial.println(" entradas:");
        #endif

        for (uint8_t e = 0; e < n; e++)
        {
            uint32_t v = ((uint32_t)trama[9 + 4 * e] << 24)     |
                         ((uint32_t)trama[10 + 4 * e] << 16)    |
                         ((uint32_t)trama[11 + 4 * e] << 8)     |
                         (uint32_t)trama[12 + 4 * e];

            /* Tests */


            //  Salida en FASE SPC: (valor + N x 2^32) mod 10^8 por contador
            //  identificado (vigilante de vuelta incluido); el resto solo
            //  %10^8. Mismo formato que la via individual: 8 digitos ASCII
            //  con ceros a la izquierda + terminador (byte-compatible con
            //  los char[9] de Contadores_SAS y su memcpy de 9 bytes). Asi
            //  el numero coincide con el que enviaba la SPC y el servidor
            //  lo sigue acumulando con sus "Contadores rollover"; ademas el
            //  %10^8 evita que un valor de 9+ digitos quede truncado en
            //  sus primeros 8 digitos por el %08lu
            char contador[9];
            uint8_t idx_n_e = Gamma_Idx_N_Bloque(e);

            //  DROP (entrada [9]): espejo del gateo de la via individual -
            //  solo la fuente ACTIVA alimenta el vigilante/N (ver comentario
            //  en Gamma_Idx_N_Objeto)
            if (e == 9 && fuente_drop == GAMMA_DROP_INDIVIDUAL)
                idx_n_e = GAMMA_IDX_SIN_N;

            snprintf(contador, sizeof(contador), "%08lu",
                     (unsigned long)Gamma_ProcesarMeter(idx_n_e, v));

            // ========================================================
            //  SETEO DE LAS 24 ENTRADAS: cada case trae su linea
            //  Set_Contadores lista - descomentar la del contador a
            //  setear (Filtro_Contador segun Contadores.h). En las
            //  entradas confirmadas se anota el contador que hoy
            //  alimenta el objeto individual equivalente; activarlas
            //  hace que bloque y meter individual escriban el MISMO
            //  contador del sistema
            // ========================================================
            switch (e)
            {
            case 0:                                   // [00] Meter grande
                #ifdef Debug_Gamma
                Serial.println("  [00] Meter grande (02 02 04) = " + String((unsigned long)v));
                #endif
                // contadores.Set_Contadores(____, contador);   // <- elegir de Contadores.h
                break;

            case 1:                                   // [01] Current Credits
                #ifdef Debug_Gamma
                Serial.println("  [01] Current Credits (02 03 01) = " + String((unsigned long)v));
                #endif
                //  Tambien lo alimenta el meter individual 02 03 01
                // contadores.Set_Contadores(Current_Credits, contador);
                break;

            case 2:                                   // [02] Total Games Played
                #ifdef Debug_Gamma
                Serial.println("  [02] Total Games Played (02 04 04) = " + String((unsigned long)v));
                #endif
                //  Tambien lo alimenta el meter individual 02 04 04
                // contadores.Set_Contadores(Games_Played, contador);
                break;

            case 3:                                   // [03] Sin asignar
                #ifdef Debug_Gamma
                Serial.println("  [03] Sin asignar (meter vivo; tambien en 03 01 00) = " + String((unsigned long)v));
                #endif
                // contadores.Set_Contadores(____, contador);   // <- elegir de Contadores.h
                break;

            case 4:                                   // [04] Total Credits Bet
                #ifdef Debug_Gamma
                Serial.println("  [04] Total Credits Bet (02 04 05) = " + String((unsigned long)v));
                #endif
                //  Tambien lo alimenta el meter individual 02 04 05
                // contadores.Set_Contadores(Coin_In, contador);
                break;

            case 5:                                   // [05] Total Credits Won
                #ifdef Debug_Gamma
                Serial.println("  [05] Total Credits Won (02 04 06) = " + String((unsigned long)v));
                #endif
                //  Tambien lo alimenta el meter individual 02 04 06
                // contadores.Set_Contadores(Coin_Out, contador);
                break;

            case 6:                                   // [06] Sin asignar
                #ifdef Debug_Gamma
                Serial.println("  [06] Sin asignar = " + String((unsigned long)v));
                #endif
                // contadores.Set_Contadores(____, contador);   // <- elegir de Contadores.h
                break;

            case 7:                                   // [07] Sin asignar
                #ifdef Debug_Gamma
                Serial.println("  [07] Sin asignar = " + String((unsigned long)v));
                #endif
                // contadores.Set_Contadores(____, contador);   // <- elegir de Contadores.h
                break;

            case 8:                                   // [08] Sin asignar
                #ifdef Debug_Gamma
                Serial.println("  [08] Sin asignar (familia drop) = " + String((unsigned long)v));
                #endif
                // contadores.Set_Contadores(____, contador);   // <- elegir de Contadores.h
                break;

            case 9:                                   // [09] Drop (fuente adaptable)
                #ifdef Debug_Gamma
                Serial.println("  [09] Drop (fuente adaptable) = " + String((unsigned long)v));
                #endif
                //  En las maquinas donde el 04 09 01 individual viene en 0,
                //  el drop vive AQUI. Setea solo cuando la fuente activa es
                //  el bloque (el individual tiene prioridad siempre que
                //  traiga valor). OJO: en otras maquinas la zona drop del
                //  bloque son [08]/[15] con [09] en 0 - en esas el
                //  individual esta vivo y la regla lo toma a el
                drop_bloque = v;
                Gamma_DecidirFuenteDrop();

                if (fuente_drop == GAMMA_DROP_BLOQUE)
                {
                    contadores.Set_Contadores(
                        Total_Drop,
                        contador);

                    contadores.Set_Contadores(
                        Bill_Amount,
                        contador);

                    contadores.Change_Counters_Bill_In(contador);   // detecta subida -> Flag_Bill_In
                    contadores.Ajusta_Base_Vuelta_Bill_In(contador);   // vuelta 10^8: re-siembra base
                }
                break;

            case 10:                                  // [10] Sin asignar
                #ifdef Debug_Gamma
                Serial.println("  [10] Sin asignar = " + String((unsigned long)v));
                #endif
                // contadores.Set_Contadores(____, contador);   // <- elegir de Contadores.h
                break;

            case 11:                                  // [11] Sin asignar
                #ifdef Debug_Gamma
                Serial.println("  [11] Sin asignar = " + String((unsigned long)v));
                #endif
                // contadores.Set_Contadores(____, contador);   // <- elegir de Contadores.h
                break;

            case 12:                                  // [12] Sin asignar
                #ifdef Debug_Gamma
                Serial.println("  [12] Sin asignar = " + String((unsigned long)v));
                #endif
                // contadores.Set_Contadores(____, contador);   // <- elegir de Contadores.h
                break;

            case 13:                                  // [13] Cashout Handpays
                #ifdef Debug_Gamma
                Serial.println("  [13] Cashout Handpays (02 05 03) = " + String((unsigned long)v));
                #endif
                //  OJO: este valor DIFIERE del meter individual 02 05 03
                //  (delta constante): activar ambos haria oscilar el
                //  contador. Elegir UNA sola fuente
                // contadores.Set_Contadores(Cancel_Credit_Hand_Pay, contador);
                // contadores.Set_Contadores(Total_Cancel_Credit, contador);
                // contadores.Change_Counters(contador);         // detecta subida -> Flag_Premio
                // contadores.Ajusta_Base_Vuelta_Counters(contador);   // vuelta 10^8: re-siembra

                //Serial.println("[GAMMA] [13] Cashout Handpays (02 05 03) = " + String((unsigned long)v));
                break;

            case 14:                                  // [14] Sin asignar
                #ifdef Debug_Gamma
                Serial.println("  [14] Sin asignar = " + String((unsigned long)v));
                #endif
                // contadores.Set_Contadores(____, contador);   // <- elegir de Contadores.h
                break;

            case 15:                                  // [15] Sin asignar
                #ifdef Debug_Gamma
                Serial.println("  [15] Sin asignar (familia drop; NO es True In) = " + String((unsigned long)v));
                #endif
                // contadores.Set_Contadores(____, contador);   // <- elegir de Contadores.h
                break;

            case 16:                                  // [16] Sin asignar
                #ifdef Debug_Gamma
                Serial.println("  [16] Sin asignar = " + String((unsigned long)v));
                #endif
                // contadores.Set_Contadores(____, contador);   // <- elegir de Contadores.h
                break;

            case 17:                                  // [17] Sin asignar
                #ifdef Debug_Gamma
                Serial.println("  [17] Sin asignar = " + String((unsigned long)v));
                #endif
                // contadores.Set_Contadores(____, contador);   // <- elegir de Contadores.h
                contadores.Set_Contadores(Jackpot, contador);   // <- elegir de Contadores.h
                break;

            case 18:                                  // [18] Sin asignar
                #ifdef Debug_Gamma
                Serial.println("  [18] Sin asignar = " + String((unsigned long)v));
                #endif
                // contadores.Set_Contadores(____, contador);   // <- elegir de Contadores.h
                break;

            case 19:                                  // [19] Sin asignar
                #ifdef Debug_Gamma
                Serial.println("  [19] Sin asignar = " + String((unsigned long)v));
                #endif
                // contadores.Set_Contadores(____, contador);   // <- elegir de Contadores.h
                break;

            case 20:                                  // [20] Sin asignar
                #ifdef Debug_Gamma
                Serial.println("  [20] Sin asignar = " + String((unsigned long)v));
                #endif
                // contadores.Set_Contadores(____, contador);   // <- elegir de Contadores.h
                break;

            case 21:                                  // [21] Sin asignar
                #ifdef Debug_Gamma
                Serial.println("  [21] Sin asignar = " + String((unsigned long)v));
                #endif
                // contadores.Set_Contadores(____, contador);   // <- elegir de Contadores.h
                break;

            case 22:                                  // [22] Sin asignar
                #ifdef Debug_Gamma
                Serial.println("  [22] Sin asignar = " + String((unsigned long)v));
                #endif
                // contadores.Set_Contadores(____, contador);   // <- elegir de Contadores.h
                break;

            case 23:                                  // [23] Sin asignar
                #ifdef Debug_Gamma
                Serial.println("  [23] Sin asignar = " + String((unsigned long)v));
                #endif
                // contadores.Set_Contadores(____, contador);   // <- elegir de Contadores.h
                break;
            }
        }
        return;
    }

    //  Se aceptan meters simples (16 B) y el bloque de identidad 03 01 00
    //  (98 B); el bloque multi-meter 02 03 03 ya se manejo arriba. Lo demas,
    //  solo con Debug_Gamma
    if (len != 0x0C && !(p[3] == 0x03 && p[4] == 0x01 && p[5] == 0x00 && len == 0x62)
                   && !(p[3] == 0x02 && p[4] == 0x01 && p[5] == 0x05 && len == 0x08)
                   && !(p[3] == 0x06 && p[4] == 0x02 && p[5] == 0x01 && len == 0x0A))
    {
        #ifdef Debug_Gamma               // cualquier otra estructura rara
        Serial.print("[GAMMA] TRAMA DESCONOCIDA 0x31 (len="); Serial.print(len);
        Serial.print("): ");
        for (uint8_t i = 0; i < len + 4 && i < sizeof(rx_acum); i++)
        {
            if (trama[i] < 0x10) Serial.print('0');
            Serial.print(trama[i], HEX); Serial.print(' ');
        }
        Serial.println();
        #endif
        return;
    }

    //  Filtro SEQ: la respuesta debe traer el SEQ de nuestra ultima transmision
    if (trama[0] != ultimo_seq_enviado)
        return;

    // ── Identificacion del contador por su eco (rx[5..7]) ──────────────────
    uint8_t obj[3] = { p[3], p[4], p[5] };

    // ── VALIDACION para todos: ¿es la DATA del objeto que se pidio? ────────
    //  Hace avanzar el ciclo de encuestas (meters de 16 B Y bloque identidad)
    if (hay_objeto_pendiente &&
        obj[0] == objeto_pendiente[0] && obj[1] == objeto_pendiente[1] &&
        obj[2] == objeto_pendiente[2])
    {
        hay_objeto_pendiente = false;
        flag_meter_recibido = true;     // avanza al siguiente meter
    }

    // ── PUERTA (06 02 01 - EGM Doors P1) ────────────────────────────────────
    //  ANTES de la bifurcacion de LEN: la maquina Mk5 manda la DATA con LEN
    //  0x0A (T09) y la Mk6 con LEN 0x0C (T0A) - las POSICIONES del dato son
    //  identicas en ambos formatos (verificado en campo 04-09-2026), asi que
    //  un solo decode sirve para todos:
    //    trama[9]  bit 0 = puerta ABIERTA en vivo (1 = abierta, 0 = cerrada)
    //    trama[10..11] BE16 = contador de APERTURAS (+1 al abrir; el cierre
    //              NO suma; un ciclo completo suma +1 exacto)
    //  Se registra la TRANSICION del bit: la primera lectura tras arranque
    //  solo fija el estado inicial (sin evento); y las aperturas alimentan el
    //  campo "Numero de aperturas de puerta" de la trama al servidor
    if (obj[0] == 0x06 && obj[1] == 0x02 && obj[2] == 0x01)
    {
        bool     abierta    = trama[9] & 0x01;
        uint16_t aperturas  = ((uint16_t)trama[10] << 8) | (uint16_t)trama[11];

        if (!puerta_conocida)              // primera lectura: estado inicial
        {
            puerta_conocida = true;
            puerta_abierta  = abierta;
            //  Solo fija el estado de referencia: SIN evento (decision de
            //  diseno - no reportar el estado inicial al servidor, unicamente
            //  las transiciones que pasen estando el enlace vivo)
        }
        else if (abierta != puerta_abierta)    // TRANSICION -> evento
        {
            puerta_abierta = abierta;

            //  0x11 abierta / 0x12 cerrada segun la NUEVA posicion
            if (eventos.Set_evento(abierta ? 0x11 : 0x12))
            {
                Variables_globales.Set_Variable_Global(Dato_Evento_Valido, true);
            }
        }

        //  Aperturas -> campo de la trama (mismo formato string de 8 digitos)
        char aperturas_txt[9];
        snprintf(aperturas_txt, sizeof(aperturas_txt), "%08lu", (unsigned long)aperturas);
        contadores.Set_Contadores(Door_Open, aperturas_txt);

        aperturas_puerta = aperturas;      // disponible para el futuro
        return;
    }

    if (len != 0x0C)
    {
        // ── DENOMINACION (02 01 05, LEN 08 - EGM Configuration P5) ─────────
        //  Value of 1 credit en centavos de la moneda interna: 1 = 0,01 $.
        //  Trama T=07: [slot][08][07][31 82][eco][BE16][CRC] -> valor en
        //  trama[8..9]. Alimenta el byte BCD [5] de la info (campo
        //  Denomination del SAS 1F). Ojo: 04 01 01 es el value of 1 COIN
        //  (100 vs 1 en esta maquina) - valores distintos, no mezclar
        if (obj[0] == 0x02 && obj[1] == 0x01 && obj[2] == 0x05)
        {
            uint32_t denom = ((uint32_t)trama[8] << 8) | (uint32_t)trama[9];
            if (denom >= 1 && denom <= 99)     // el campo es un byte BCD (2 dig)
            {
                info_gamma[5] = (char)((denom / 10) * 16 + (denom % 10));
                info_flags |= GAMMA_INFO_DENOM;   // ya tenemos la denominacion
                Gamma_PublicarInfoSiCompleta();   // setea solo si la identidad llego
                #ifdef Debug_Gamma
                Serial.print("[GAMMA] Denominacion (02 01 05) = ");
                Serial.print((unsigned long)denom);
                Serial.println(" centavos -> byte [5] BCD");
                #endif
            }
            #ifdef Debug_Gamma
            else Serial.println("[GAMMA] Denominacion (02 01 05) fuera de rango BCD 1..99");
            #endif
            return;
        }

        // ── BLOQUE IDENTIDAD (03 01 00, 98 B - Anexo G.3) ──────────────────
        //  ASCII: "G1607" / "AH" / "Dolphin Treasure" / "87.928%" + meters.
        //  De el tomamos tabla de pagos y porcentaje base. La apuesta maxima
        //  TAMBIEN vive aqui (BE16 '00 C8' = 200 creditos: ultima entrada
        //  no-cero de la cola), pero NO se setea: el formato del servidor
        //  (Set_buffer_id_maq) solo admite 2 digitos decimales por byte y
        //  200 no cabe - el byte [6] queda en 0 por decision de diseno
        Gamma_ExtraerPaytable(trama + 2, len, info_gamma + 10);
        Gamma_ExtraerPorcentaje(trama + 2, len, info_gamma + 16);
        info_flags |= GAMMA_INFO_IDENT;         // ya tenemos tabla + porcentaje
        Gamma_PublicarInfoSiCompleta();         // setea solo si la denom tambien llego

        #ifdef Debug_Gamma
        Serial.print("[GAMMA] Identidad: tabla de pagos='");
        Serial.write(info_gamma + 10, 6);
        Serial.print("'  porcentaje base=");
        Serial.write(info_gamma + 16, 4);
        Serial.println();
        #endif

        #ifdef Debug_Gamma
        //  EXPLORACION: meters embebidos tras el texto del bloque (G.3 trae
        //  Bet/Won/Games y varios "sin asignar")
        Serial.print("[GAMMA] Identidad meters embebidos: ");
        for (uint16_t off = 30; off + 4 <= (uint16_t)len + 2; off += 4)
        {
            uint32_t v = ((uint32_t)trama[off] << 24)     | ((uint32_t)trama[off + 1] << 16) |
                         ((uint32_t)trama[off + 2] << 8)  | (uint32_t)trama[off + 3];
            Serial.print('@'); Serial.print(off); Serial.print(':');
            Serial.print((unsigned long)v); Serial.print(' ');
        }
        Serial.println();
        #endif
        return;
    }

    //  Valor del contador: 4 bytes Big-Endian en las posiciones 9..12
    uint32_t valor = ((uint32_t)trama[9] << 24) | ((uint32_t)trama[10] << 16) |
                     ((uint32_t)trama[11] << 8)  | (uint32_t)trama[12];

    //  Salida en FASE SPC: (valor + N x 2^32) mod 10^8 (vigilante de vuelta
    //  incluido) - 8 digitos ASCII con ceros a la izquierda + terminador ->
    //  byte-compatible con los char[9] de Contadores_SAS y su memcpy de 9
    //  bytes. Es el mismo numero que enviaba la SPC; el servidor reconstruye
    //  el total absoluto con sus "Contadores rollover". Sin el modulo, un
    //  meter de 9 digitos (maq. 3: Coin Out 2.287.693.744) quedaria truncado
    //  en sus primeros 8 digitos por el %08lu
    uint8_t idx_n = Gamma_Idx_N_Objeto(obj);

    //  DROP: individual y bloque son meters distintos segun la poblacion de
    //  maquina (individual muerto en 0 con el drop vivo en [9], o al reves).
    //  Solo la fuente ACTIVA de la regla adaptable alimenta el vigilante/N;
    //  la inactiva sale con %10^8 simple (su Set_Contadores ya esta gateado
    //  por la misma regla mas abajo)
    if (obj[0] == 0x04 && obj[1] == 0x09 && obj[2] == 0x01 &&
        fuente_drop == GAMMA_DROP_BLOQUE)
        idx_n = GAMMA_IDX_SIN_N;

    char contador[9];
    snprintf(contador, sizeof(contador), "%08lu",
             (unsigned long)Gamma_ProcesarMeter(idx_n, valor));

    //  Publicar el contador decodificado: queda disponible (Get_Contador_
    //  Recibido) como string decimal listo para un Set_Contadores
    rx_objeto[0] = obj[0];
    rx_objeto[1] = obj[1];
    rx_objeto[2] = obj[2];
    strncpy(rx_contador, contador, sizeof(rx_contador) - 1);
    rx_contador[sizeof(rx_contador) - 1] = '\0';
    hay_rx_contador = true;

    #ifdef Debug_Gamma                  // lo CONOCIDO solo con Debug_Gamma:
                                        // en modo silencioso, unicamente lo
                                        // desconocido llena el log
    Serial.print("[GAMMA] DATA ");
    Serial.print(obj[0], HEX); Serial.print(' ');
    Serial.print(obj[1], HEX); Serial.print(' ');
    Serial.print(obj[2], HEX); Serial.print(" = ");
    Serial.println(contador);
    #endif

    // ── GUARDADO en contadores: CADENA UNICA de objetos mapeados ──────────
    //  (mismo patron y mismo formato de string que el receptor SAS; lo que
    //   no este aqui cae al else final y se dumpea siempre)
    if (obj[0] == 0x02 && obj[1] == 0x04 && obj[2] == 0x04)   // 02 04 04
    {
        contadores.Set_Contadores(Games_Played, contador);
    }

    //  04 01 01 (value of 1 coin, centavos): queda como referencia en el log,
    //  NO alimenta el byte [5]. El viejo valor/100 solo coincidia por azar con
    //  el ratio coin/credit (100) de esta maquina; en otras es 5 y daria 00

    //  04 01 16 (apuesta maxima, candidato 20.000): se consulta para observar
    //  su valor en el log, pero no se codifica todavia: no cabe en el byte
    //  BCD de 2 digitos del formato SAS. Queda en 0 hasta definir el escalado


    

    // ── Los demas contadores: activa aqui tu Set_Contadores ────────────────
    //  (ya llegan validados: CRC ok + SEQ ok + eco identificado + valor en
    //   la variable 'contador' como string decimal)
    //
    else if (obj[0] == 0x02 && obj[1] == 0x04 && obj[2] == 0x05)      // Total Credits Bet
        contadores.Set_Contadores(Coin_In, contador);
    else if (obj[0] == 0x02 && obj[1] == 0x04 && obj[2] == 0x06)      // Total Credits Won
        contadores.Set_Contadores(Coin_Out, contador);
    //  02 05 03 ("Cashout Handpays" segun el manual): DATO DE CAMPO 07-09 —
    //  es el MISMO meter que la entrada [13] del bloque (valores identicos
    //  en cada maquina), pero su identidad cambia por maquina: en la maq. 1
    //  (Mk5) es Split Voucher Pays (~8M) y en la maq. 3 (Mk6) es el Attendant
    //  Paid Cancel (2,3B). El sistema se alimenta del [13] (via con fase N
    //  cargada); este objeto queda como referencia/log igual que 04 01 01
    else if (obj[0] == 0x02 && obj[1] == 0x05 && obj[2] == 0x03)
    {
        contadores.Set_Contadores(Cancel_Credit_Hand_Pay, contador);
        contadores.Set_Contadores(Total_Cancel_Credit, contador);
        contadores.Change_Counters(contador);                  // subida -> Flag_Premio
        contadores.Ajusta_Base_Vuelta_Counters(contador);      // vuelta 10^8
    }

    //  True In (Coins In) - FUENTE ADAPTABLE del drop: solo setea cuando la
    //  fuente activa es el individual (gana siempre que traiga valor; si
    //  esta maquina lo reporta por el bloque [09], este camino no escribe
    //  nada y evita el 0/valor alternado de cada ciclo)
    else if (obj[0] == 0x04 && obj[1] == 0x09 && obj[2] == 0x01)
    {
        drop_individual = valor;
        drop_individual_leido = true;   // ya respondio (aunque sea con 0):
                                        // habilita al bloque como fuente
        Gamma_DecidirFuenteDrop();

        if (fuente_drop == GAMMA_DROP_INDIVIDUAL)
        {
            contadores.Set_Contadores(Total_Drop, contador);
            contadores.Set_Contadores(Bill_Amount, contador);
            contadores.Change_Counters_Bill_In(contador);   // detecta subida -> Flag_Bill_In
            contadores.Ajusta_Base_Vuelta_Bill_In(contador);   // vuelta 10^8: re-siembra base
        }
    }
        
    else if (obj[0] == 0x04 && obj[1] == 0x04 && obj[2] == 0x01)      // Coin Drop / Gross Drop
        contadores.Set_Contadores(Total_Coin_Drop, contador);
    else if (obj[0] == 0x02 && obj[1] == 0x03 && obj[2] == 0x01)      // Current Credits
        contadores.Set_Contadores(Current_Credits, contador);
    #ifdef Debug_Gamma                  // objeto no mapeado: solo con debug
    else
    {
        Serial.print("[GAMMA] DATA OBJETO NO MAPEADO (");
        Serial.print(obj[0], HEX); Serial.print(' ');
        Serial.print(obj[1], HEX); Serial.print(' ');
        Serial.print(obj[2], HEX);
        Serial.print("): ");
        for (uint8_t i = 0; i < len + 4 && i < sizeof(rx_acum); i++)
        {
            if (trama[i] < 0x10) Serial.print('0');
            Serial.print(trama[i], HEX); Serial.print(' ');
        }
        Serial.println();
    }
    #endif
}

// ============================================================================
//  Gamma_Test_Rx_Meter - autotest de banco del receptor Gamma, para CUALQUIER
//  contador. Simula la recepcion de la DATA del objeto {clase,subgrupo,meter}
//  con la misma estructura de las tramas de la captura del 27-08-2026:
//    [SEQ][0C][0A][31][82][clase][sub][meter][00][VALOR BE32][00][CRC16]
//  y verifica que Gamma_ProcesarRx GENERA el contador (string decimal listo
//  para Set_Contadores). Por ahora solo se imprime: no se escribe en
//  Contadores.
//
//  Ejercita el camino completo usando SOLO la API publica:
//    1. Gamma_SimularQuery(clase,sub,meter) -> arma objeto_pendiente y ultimo
//       SEQ SIN transmitir nada (no requiere driver UART ni maquina)
//    2. Inyecta basura previa (A5 00)       -> prueba la resincronizacion
//    3. Inyecta la trama PARTIDA en 2       -> prueba el acumulador rx_acum
//       rafagas (7 B + 9 B)                    entre eventos UART
//    4. Verifica: banderas del ciclo, Datos_OK, el objeto eco recibido y el
//       VALOR del contador generado vs el inyectado
//
//  COMO USARLA: llamarla donde sea (setup, Web_Debug...) SIN la task de
//  encuestas Gamma corriendo, para que ninguna transmision real cambie
//  ultimo_seq_enviado entre la simulacion y la inyeccion. Ejemplos:
//      Gamma_Test_Rx_Meter(0x02, 0x04, 0x04, 12345678);  // Games Played
//      Gamma_Test_Rx_Meter(0x02, 0x03, 0x01, 250);       // Current Credits
//  valor_prueba: maximo 8 digitos (asi se validan los meters reales).
//  NOTA: si algun dia se quiere probar TAMBIEN la via de TX real (p.ej. con
//  un lazo fisico TX->RX), cambiar Gamma_SimularQuery por Gamma_EnviarQuery.
// ============================================================================
bool Gamma_Test_Rx_Meter(uint8_t clase, uint8_t subgrupo, uint8_t meter,
                         uint32_t valor_prueba)
{
    Serial.println();
    Serial.println("[GAMMA-TEST] ===== Autotest Rx Meter =====");

    bool ok = true;

    // ── 1. QUERY simulada del objeto: deja SEQ + objeto pendientes ─────────
    Gamma.Limpiar_Flag_Meter();
    Gamma.Limpiar_Contador_Recibido();
    uint8_t seq = Gamma.Gamma_SimularQuery(clase, subgrupo, meter);
    Serial.print("[GAMMA-TEST] QUERY simulada (objeto ");
    Serial.print(clase, HEX);  Serial.print(' ');
    Serial.print(subgrupo, HEX); Serial.print(' ');
    Serial.print(meter, HEX);
    Serial.print(") con SEQ 0x");
    Serial.println(seq, HEX);

    // ── 2. Armar la trama DATA como las de la captura, con CRC real ────────
    uint8_t trama[16];
    trama[0]  = seq;                             // eco de nuestra ultima TX
    trama[1]  = 0x0C;                            // LEN: 12 bytes de payload
    trama[2]  = 0x0A;                            // SIZE
    trama[3]  = 0x31;                            // bloque de datos
    trama[4]  = 0x82;
    trama[5]  = clase;                           // ┐
    trama[6]  = subgrupo;                        // ├ eco del objeto consultado
    trama[7]  = meter;                           // ┘
    trama[8]  = 0x00;
    trama[9]  = (uint8_t)(valor_prueba >> 24);   // valor Big-Endian de 32 bits
    trama[10] = (uint8_t)(valor_prueba >> 16);
    trama[11] = (uint8_t)(valor_prueba >> 8);
    trama[12] = (uint8_t)(valor_prueba);
    trama[13] = 0x00;
    uint16_t crc = GammaProlink::Gamma_CRC16(trama, 14);
    trama[14] = (uint8_t)(crc >> 8);             // CRC MSB primero
    trama[15] = (uint8_t)(crc);

    // ── 3. Inyeccion: basura + trama partida en dos rafagas ───────────────
    const uint8_t basura[2] = { 0xA5, 0x00 };    // slots invalidos: resincroniza
    Gamma.Gamma_ProcesarRx(basura, sizeof(basura));
    Gamma.Gamma_ProcesarRx(trama, 7);            // rafaga 1: trama incompleta

    if (Gamma.Hay_Meter_Recibido() || Gamma.Hay_Contador_Recibido())
    {
        Serial.println("[GAMMA-TEST] FAIL: izo flags con la trama incompleta");
        ok = false;
    }
    else
        Serial.println("[GAMMA-TEST] OK   : trama incompleta no izo flags");

    Gamma.Gamma_ProcesarRx(trama + 7, 9);        // rafaga 2: completa la trama

    // ── 4. Verificaciones ──────────────────────────────────────────────────
    if (!Gamma.Hay_Meter_Recibido())
    {
        Serial.println("[GAMMA-TEST] FAIL: flag de meter NO izado tras la DATA");
        ok = false;
    }
    else
        Serial.println("[GAMMA-TEST] OK   : Hay_Meter_Recibido() en true");

    if (!Datos_OK)
    {
        Serial.println("[GAMMA-TEST] FAIL: Datos_OK no quedo en true");
        ok = false;
    }
    else
        Serial.println("[GAMMA-TEST] OK   : Datos_OK en true (timeout refrescado)");

    // ── El contador que GENERO el receptor: listo para Set_Contadores ──────
    if (!Gamma.Hay_Contador_Recibido())
    {
        Serial.println("[GAMMA-TEST] FAIL: el receptor no genero contador");
        ok = false;
    }
    else
    {
        const uint8_t  *eco      = Gamma.Get_Objeto_Recibido();
        const char     *contador = Gamma.Get_Contador_Recibido();   // string decimal
        char esperado[16];
        snprintf(esperado, sizeof(esperado), "%lu", (unsigned long)valor_prueba);

        Serial.print("[GAMMA-TEST] Contador generado por Gamma_ProcesarRx: \"");
        Serial.print(contador);
        Serial.print("\"  (objeto eco ");
        Serial.print(eco[0], HEX); Serial.print(' ');
        Serial.print(eco[1], HEX); Serial.print(' ');
        Serial.print(eco[2], HEX);
        Serial.println(")  <- listo para Set_Contadores (solo impresion por ahora)");

        if (strcmp(contador, esperado) != 0)
        {
            Serial.print("[GAMMA-TEST] FAIL: valor generado \"");
            Serial.print(contador);
            Serial.print("\" != esperado \"");
            Serial.print(esperado);
            Serial.println("\"");
            ok = false;
        }
        else
            Serial.println("[GAMMA-TEST] OK   : valor generado == valor inyectado");

        if (eco[0] != clase || eco[1] != subgrupo || eco[2] != meter)
        {
            Serial.println("[GAMMA-TEST] FAIL: el eco del objeto no coincide");
            ok = false;
        }
        else
            Serial.println("[GAMMA-TEST] OK   : eco del objeto correcto");
    }

    Serial.println(ok ? "[GAMMA-TEST] ===== RESULTADO: PASS ====="
                      : "[GAMMA-TEST] ===== RESULTADO: FAIL =====");
    return ok;
}

// ============================================================================
//  Gamma_Test_Rx_Bloque - autotest de banco del BLOQUE MULTI-METER 02 03 03,
//  ahora integrado al ciclo de encuestas (TABLA_METERS_GAMMA). Inyecta la
//  trama de 108 B (9 de cabecera + 24 contadores BE32 + byte final + CRC16)
//  con la misma estructura de la captura del 27-08-2026 y verifica:
//    1. Con SEQ equivocado NO iza flags (una trama "de rebote" de otro slot
//       no puede hacer avanzar el ciclo)
//    2. Llegando partida en dos rafagas no iza flags (acumulador rx_acum)
//    3. Completa y con el SEQ esperado iza flag_meter_recibido -> el ciclo
//       AVANZA al siguiente meter. Esto, junto con la rama de salto tras
//       GAMMA_REINTENTOS_MAX de Encuestas_Maquinas_Gamma (objeto sin DATA:
//       saltarlo y seguir), garantiza que el bloque NO BLOQUEA las consultas
//    4. NO publica contador simple (hay_rx_contador queda en false): el
//       bloque no interfiere con la via de meters individuales de 16 B
//
//  Simulacion pura como Gamma_Test_Rx_Meter: no transmite nada ni requiere
//  driver UART ni maquina. Llamarla sin la task de encuestas corriendo.
// ============================================================================
bool Gamma_Test_Rx_Bloque()
{
    Serial.println();
    Serial.println("[GAMMA-TEST] ===== Autotest Rx Bloque 02 03 03 =====");

    bool ok = true;

    // ── 1. QUERY simulada del bloque: deja SEQ + objeto pendientes ─────────
    Gamma.Limpiar_Flag_Meter();
    Gamma.Limpiar_Contador_Recibido();
    uint8_t seq = Gamma.Gamma_SimularQuery(0x02, 0x03, 0x03);
    Serial.print("[GAMMA-TEST] QUERY simulada del bloque con SEQ 0x");
    Serial.println(seq, HEX);

    // ── 2. Armar la trama DATA del bloque como las de la captura ───────────
    //    Entrada e = 1.000.000 + e (9 digitos): valores grandes a proposito
    //    para dejar claro en el log que el listado no trunca
    uint8_t trama[108];
    trama[0] = seq;                             // eco de nuestra ultima TX
    trama[1] = 0x68;                            // LEN: trama completa = 108 B
    trama[2] = 0x66;                            // SIZE
    trama[3] = 0x31;                            // bloque de datos
    trama[4] = 0x82;
    trama[5] = 0x02;                            // ┐
    trama[6] = 0x03;                            // ├ eco del objeto consultado
    trama[7] = 0x03;                            // ┘
    trama[8] = 0x00;
    for (uint8_t e = 0; e < 24; e++)            // 24 contadores BE32
    {
        uint32_t v = 1000000UL + e;
        trama[9  + 4 * e] = (uint8_t)(v >> 24);
        trama[10 + 4 * e] = (uint8_t)(v >> 16);
        trama[11 + 4 * e] = (uint8_t)(v >> 8);
        trama[12 + 4 * e] = (uint8_t)v;
    }
    trama[105] = 0x00;                          // byte final del payload

    // ── 3. SEQ equivocado (slot vecino, tambien valido): no debe izar nada ─
    trama[0] = seq ^ 0x08;
    uint16_t crc = GammaProlink::Gamma_CRC16(trama, 106);
    trama[106] = (uint8_t)(crc >> 8);
    trama[107] = (uint8_t)crc;
    Gamma.Gamma_ProcesarRx(trama, sizeof(trama));

    if (Gamma.Hay_Meter_Recibido() || Gamma.Hay_Contador_Recibido())
    {
        Serial.println("[GAMMA-TEST] FAIL:izo flags con SEQ equivocado");
        ok = false;
    }
    else
        Serial.println("[GAMMA-TEST] OK   : SEQ equivocado no izo flags");

    // ── 4. Trama buena (SEQ correcto), partida en dos rafagas ──────────────
    Gamma.Limpiar_Flag_Meter();
    trama[0] = seq;
    crc = GammaProlink::Gamma_CRC16(trama, 106);
    trama[106] = (uint8_t)(crc >> 8);
    trama[107] = (uint8_t)crc;

    const uint8_t basura[2] = { 0xA5, 0x00 };   // slots invalidos: resincroniza
    Gamma.Gamma_ProcesarRx(basura, sizeof(basura));
    Gamma.Gamma_ProcesarRx(trama, 60);          // rafaga 1: trama incompleta

    if (Gamma.Hay_Meter_Recibido() || Gamma.Hay_Contador_Recibido())
    {
        Serial.println("[GAMMA-TEST] FAIL: izo flags con la trama incompleta");
        ok = false;
    }
    else
        Serial.println("[GAMMA-TEST] OK   : trama incompleta no izo flags");

    Gamma.Gamma_ProcesarRx(trama + 60, 48);     // rafaga 2: completa la trama

    // ── 5. Verificaciones ──────────────────────────────────────────────────
    if (!Gamma.Hay_Meter_Recibido())
    {
        Serial.println("[GAMMA-TEST] FAIL: flag de meter NO izado tras el bloque");
        ok = false;
    }
    else
        Serial.println("[GAMMA-TEST] OK   : Hay_Meter_Recibido() en true (el ciclo avanza)");

    if (!Datos_OK)
    {
        Serial.println("[GAMMA-TEST] FAIL: Datos_OK no quedo en true");
        ok = false;
    }
    else
        Serial.println("[GAMMA-TEST] OK   : Datos_OK en true (timeout refrescado)");

    if (Gamma.Hay_Contador_Recibido())
    {
        Serial.println("[GAMMA-TEST] FAIL: el bloque publico un contador simple");
        ok = false;
    }
    else
        Serial.println("[GAMMA-TEST] OK   : sin contador simple (no interfiere con meters 16 B)");

    Serial.println(ok ? "[GAMMA-TEST] ===== RESULTADO: PASS ====="
                      : "[GAMMA-TEST] ===== RESULTADO: FAIL =====");
    return ok;
}
