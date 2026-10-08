#ifndef GAMMAPROLINK_H
#define GAMMAPROLINK_H

#include <Arduino.h>
#include "driver/uart.h"

// ============================================================================
//  GammaProLink - Enlace con maquinas Aristocrat por protocolo Gamma
//  (el ESP32 hace de SPC maestro)
//
//  Ver: ProtocoloGamma/docs/Manual_Protocolo_Gamma.md
//  CRC validado contra maquina real 27-08-2026 (1561/1561 tramas exactas)
// ============================================================================

//  Mismo puerto fisico que usa RS232.h (NUMERO_PORTA_SERIALE = UART_NUM_2).
//  No se incluye RS232.h porque ese header define variables globales y no
//  tiene include guard -> incluirlo dos veces rompe el linker.
//  Si algun dia cambia el puerto en RS232.h, cambiarlo aqui tambien.
#define GAMMA_PORTA_SERIALE UART_NUM_2

//#define Debug_Gamma   // descomentar para depurar el ciclo de encuestas por serial

// ----------------------------------------------------------------------------
//  FASE SPC - vueltas de 2^32 (N) por contador identificado.
//  El meter que llega por Gamma es el total de vida MODULO 2^32 (la maquina
//  descarto las vueltas); el sistema del servidor acumula los modulos de 10^8
//  con sus "Contadores rollover" y espera la MISMA fase que enviaba la SPC.
//  El firmware publica entonces:  salida = (lectura + N x 2^32) mod 10^8.
//  N = 0 (default) = maquina nueva / RAM clear: sale exacto sin calibrar.
//  Un N equivocado solo deja la fase corrida: los deltas JAMAS se rompen.
//  Ver ProtocoloGamma/docs/Analisis_SPC_Contadores.md (secciones 3 y 8).
// ----------------------------------------------------------------------------
enum {
    GAMMA_N_COIN_IN   = 0,   // [0] Coin In / Total Credits Bet (02 04 05 / bloque [04])
    GAMMA_N_COIN_OUT  = 1,   // [1] Coin Out / Total Credits Won (02 04 06 / bloque [05])
    GAMMA_N_DROP      = 2,   // [2] Drop / Billetero (04 09 01 / bloque [09])
    GAMMA_N_CANCEL    = 3,   // [3] Cancelled credits (02 05 03 / bloque [13])
    GAMMA_N_JACKPOT   = 4,   // [4] Jackpot (bloque [17])
    GAMMA_N_COIN_DROP = 5,   // [5] Coin Drop / Gross Drop (04 04 01)
    GAMMA_N_GAMES     = 6,   // [6] Total Games Played (02 04 04 / bloque [02])
    GAMMA_N_TOTAL     = 7
};
#define GAMMA_IDX_SIN_N   0xFF   // meter sin N asociado: solo formateo %10^8

// ----------------------------------------------------------------------------
//  Resultado de la calibracion por pantalla (Gamma_Calibrar_N_Desde_Pantalla).
//  El handler TCP responde al cliente con Gamma_Calibrar_Texto(res) + el
//  nombre del contador rechazado (NOMBRES_N_VUELTAS en el .cpp, o el eco de
//  Gamma_Get_N_Vueltas si fue OK)
// ----------------------------------------------------------------------------
enum Gamma_Calib_Res {
    GAMMA_CALIB_OK = 0,              // N calculados, validados y guardados en NVS
    GAMMA_CALIB_ERR_SIN_LECTURA,     // contador sin lectura R vigente (esperar 1 ciclo)
    GAMMA_CALIB_ERR_ATRAS,           // R implicito > R actual + tolerancia (typo / otra maquina)
    GAMMA_CALIB_ERR_DESACTUALIZADA,  // R actual - R implicito > tolerancia (pantalla vieja)
    GAMMA_CALIB_ERR_NVS              // validacion OK pero fallo la persistencia
};

class GammaProlink {
public:

    GammaProlink();

    // -----------------------------------------------------------------------
    //  CRC-16/XMODEM del protocolo Gamma
    //  poly=0x1021, init=0x0000, sin reflexion, xorout=0x0000
    //  Se calcula sobre SEQ+LEN+PAYLOAD (toda la trama menos sus 2 bytes de
    //  CRC) y se transmite MSB primero.
    //
    //  Vectores de verificacion (Anexo C del manual):
    //    {40 00}                                    -> 0x0DCC (poll slot 0x40)
    //    {78 06 05 30 02 02 04 05}                  -> 0x5F75 (QUERY Credits Bet)
    //    {48 06 05 30 02 02 03 01}                  -> 0xC0BA (QUERY Current Credits)
    //    {40 0C 0A 31 82 02 04 05 00 02 21 1E FF 00}-> 0xCBF3 (DATA Credits Bet)
    //
    //  Propiedad util: si se le pasa la trama COMPLETA (incluido su CRC),
    //  devuelve 0x0000 -> valida lo recibido sin extraer campos.
    // -----------------------------------------------------------------------
    static uint16_t Gamma_CRC16(const uint8_t *datos, size_t longitud);

    // -----------------------------------------------------------------------
    //  Envia por UART2 el POLL vacio del slot actual: [SEQ][00][CRC MSB][CRC LSB]
    //  (4 bytes). Avanza la rotacion de slots. Devuelve el SEQ usado para que
    //  el llamador pueda validar el eco/ACK de la maquina (responde con el
    //  mismo SEQ).
    // -----------------------------------------------------------------------
    uint8_t Gamma_EnviarPoll();

    // -----------------------------------------------------------------------
    //  Envia por UART2 la QUERY de lectura del objeto (clase/subgrupo/meter):
    //  [SEQ][06][05][30][02][clase][sub][meter][CRC MSB][CRC LSB]  (10 bytes).
    //  Avanza la rotacion de slots. Devuelve el SEQ usado.
    //  La maquina responde primero un ACK vacio con ese SEQ, y la DATA llega
    //  como respuesta al POLL del slot SIGUIENTE (modelo diferido, Manual §6).
    // -----------------------------------------------------------------------
    uint8_t Gamma_EnviarQuery(uint8_t clase, uint8_t subgrupo, uint8_t meter);

    // -----------------------------------------------------------------------
    //  Poll del slot 0x40 (slot de arranque / re-enganche del enlace, Manual §6.3).
    //  Reinicia la rotacion en 0x40; el siguiente envio sera 0x48.
    // -----------------------------------------------------------------------
    uint8_t Gamma_EnviarPollArranque();

    // -----------------------------------------------------------------------
    //  Gamma_SimularQuery - prepara el estado del receptor SIN transmitir:
    //  queda igual que si se hubiera enviado la QUERY del objeto (consume un
    //  slot de la rotacion, deja objeto_pendiente y ultimo_seq_enviado).
    //  Para autotests de banco: no toca el UART, asi que no requiere el
    //  driver instalado ni maquina conectada. Devuelve el SEQ que debe
    //  traer la trama DATA simulada para pasar el filtro del despachador.
    // -----------------------------------------------------------------------
    uint8_t Gamma_SimularQuery(uint8_t clase, uint8_t subgrupo, uint8_t meter);

    // -----------------------------------------------------------------------
    //  Gamma_ProcesarRx - receptor del protocolo Gamma.
    //  Llamar desde la Task UART (UART_ISR_ROUTINE) cuando el tipo de maquina
    //  sea 19, pasandole los bytes crudos recien leidos del UART2:
    //      Gamma.Gamma_ProcesarRx((const uint8_t *)UART2_data, UART2_data_length);
    //  Acumula los bytes, extrae tramas completas por estructura (SEQ valido
    //  + LEN + bytes), valida el CRC (residuo 0) y el SEQ, y despacha:
    //    - ACK/poll eco (4 B)      -> se descarta
    //    - DATA (16 B)             -> valida el eco del objeto (rx[5..7]):
    //        * guarda el valor solo en Games_Played (los demas quedan listos
    //          para activar su Set_Contadores en Gamma_DespacharTrama)
    //        * si el objeto es el de la ultima QUERY enviada, iza la bandera
    //          flag_meter_recibido que hace avanzar el ciclo de encuestas
    // -----------------------------------------------------------------------
    void Gamma_ProcesarRx(const uint8_t *datos, size_t longitud);

    // -----------------------------------------------------------------------
    //  Ciclo de encuestas Gamma (tipo de maquina 19).
    //  Llamar desde la task de encuestas de maquina: cada llamada transmite
    //  UNA transaccion y avanza la maquina de estados:
    //    FASE_QUERY  -> envia la QUERY del meter actual de la tabla
    //    FASE_ESPERA -> envia POLL ("¿ya?"); si llego la DATA avanza de meter;
    //                   si vence la ventana reintenta la QUERY del MISMO meter
    //    FASE_RESYNC -> tras agotar los reintentos, re-engancha el enlace con
    //                   polls del slot 0x40 (la maquina deja de responder si
    //                   deja de recibir transmisiones - Manual §6.3)
    //  El avance de meter SOLO ocurre cuando el receptor confirma la DATA
    //  (bandera Hay_Meter_Recibido), no por tiempo.
    // -----------------------------------------------------------------------
    void Encuestas_Maquinas_Gamma();

    bool Maquina_Aristocrat_Gamma(uint16_t Tipo_Maq);
    bool Flag_handle_Query();

    // -----------------------------------------------------------------------
    //  Consulta BAJO DEMANDA del bloque multi-meter 02 03 03 (24 valores -
    //  Manual §7.6). El bloque TAMBIEN viaja en el ciclo de encuestas
    //  (TABLA_METERS_GAMMA); esta via extra es herramienta de descubrimiento:
    //  activar Gamma_Activar_Query_Bloque() desde donde se quiera (Web_Debug,
    //  etc.) -> la task de encuestas, al ver el flag (lo consulta con
    //  Flag_handle_Query()), llama Gamma_ConsultarBloque() en cada tick:
    //  esta envia la QUERY una vez y luego polls hasta que llegue la DATA
    //  (el despachador la imprime indexada bajo Debug_Gamma y cierra la
    //  sesion) o venza la ventana. Mientras el flag este alto, esta rama
    //  reemplaza el ciclo normal.
    // -----------------------------------------------------------------------
    void Gamma_Activar_Query_Bloque();
    void Gamma_ConsultarBloque();

    //  El receptor (ProcesarRx, Task UART) iza esta bandera al recibir y
    //  validar (CRC + SEQ + eco del objeto) la DATA del meter esperado.
    bool Hay_Meter_Recibido() const   { return flag_meter_recibido; }
    void Limpiar_Flag_Meter()         { flag_meter_recibido = false; }

    //  Cierto una vez por cada ciclo completo de la tabla de meters.
    //  Equivalente al flag_ultimo_contador_Ok de las Encuestas SAS.
    bool Ciclo_Completo() const       { return flag_ciclo_completo; }
    void Limpiar_Ciclo_Completo()     { flag_ciclo_completo = false; }

    // -----------------------------------------------------------------------
    //  PUERTA (objeto 06 02 01 - EGM Doors P1, en el ciclo de encuestas).
    //  El receptor detecta la TRANSICION del bit de estado e imprime
    //  "[GAMMA] >>> PUERTA ABIERTA/CERRADA <<<" en cada apertura/cierre
    //  (la primera lectura tras arranque solo fija el estado inicial).
    //  OJO con los nombres: los eventos del sistema se llaman igual en
    //  Tabla_Eventos.h (Puerta_Abierta=0x11 / Puerta_Cerrada=0x12, macros) -
    //  por eso los getters llevan el prefijo Get_.
    //    Get_Puerta_Abierta()    -> ultimo estado conocido (true = abierta)
    //    Get_Aperturas_Puerta()  -> contador de aperturas de la maquina (+1
    //                               por apertura; util para detectar aperturas
    //                               ocurridas entre polls cuando se necesite)
    // -----------------------------------------------------------------------
    bool     Get_Puerta_Abierta() const   { return puerta_abierta; }
    uint16_t Get_Aperturas_Puerta() const { return aperturas_puerta; }

    // -----------------------------------------------------------------------
    //  Ultima DATA valida decodificada por el receptor (de cualquier objeto):
    //    Get_Contador_Recibido -> valor en string decimal, LISTO para un
    //                              Set_Contadores (por ahora el firmware solo
    //                              lo imprime; los autotests lo verifican)
    //    Get_Objeto_Recibido   -> eco del objeto {clase, subgrupo, meter}
    // -----------------------------------------------------------------------
    bool             Hay_Contador_Recibido() const { return hay_rx_contador; }
    const char      *Get_Contador_Recibido() const { return rx_contador; }
    const uint8_t   *Get_Objeto_Recibido() const   { return rx_objeto; }
    void             Limpiar_Contador_Recibido()   { hay_rx_contador = false; }

    // -----------------------------------------------------------------------
    //  CALIBRACION DE FASE SPC: N (vueltas de 2^32) por contador identificado,
    //  persistidos en NVS (namespace "Gamma", clave "N_Vueltas"; defaults 0
    //  creados en el primer arranque). Entrada prevista: comando del servidor
    //  por AsyncTCP que llame a:
    //    Gamma_Set_N_Vueltas(valores, cantidad) -> tabla completa (orden del
    //        enum GAMMA_N_*, cantidad = GAMMA_N_TOTAL)
    //    Gamma_Set_N_Vuelta(idx, n)             -> un solo contador
    //  El N de cada contador se calcula UNA vez en la instalacion con el
    //  total de la pantalla de meters de la maquina (MAIN1, en dolares):
    //        N = (pantalla x 100 - lectura_actual) / 2^32
    //  El vigilante interno mantiene N vigente cuando el contador de la
    //  maquina completa su proxima vuelta de 2^32 (escritura NVS cada ~$43M
    //  de juego). Se puede llamar desde otra task (TCP): escrituras de 1 byte
    //  atomicas y NVS con candado propio.
    // -----------------------------------------------------------------------
    uint8_t Gamma_Get_N_Vueltas(uint8_t idx);
    void    Gamma_Set_N_Vueltas(const uint8_t *valores, uint8_t cantidad);
    void    Gamma_Set_N_Vuelta(uint8_t idx, uint8_t n);

    //  Consulta INDIVIDUAL del estado de un contador: devuelve true si idx es
    //  valido y entrega (todo opcional, pasar nullptr en lo que no se use):
    //    n        -> vueltas de 2^32 cargadas (N, de NVS)
    //    r_ultima -> ultima lectura cruda de la maquina (0 si aun no llego)
    //    t_total  -> total verdadero implicito T = R + N x 2^32 (en centavos)
    //  Para responder por TCP p.ej.: "Coin In: N=13 R=843949152 T=56678524000"
    bool Gamma_Consultar_Vueltas(uint8_t idx, uint8_t *n = nullptr,
                                 uint32_t *r_ultima = nullptr,
                                 uint64_t *t_total = nullptr);

    // -----------------------------------------------------------------------
    //  CALIBRACION POR PANTALLA (entrada natural del comando TCP): recibe los
    //  5 totales de vida del menu de meters de la maquina, EL NUMERO TAL CUAL
    //  LO MUESTRA LA PANTALLA (el operario no necesita saber si su maquina
    //  la muestra en dolares -Mk6/MAIN1- o en creditos -Mk5-): el firmware
    //  prueba ambas interpretaciones contra las lecturas R reales y gana la
    //  que cierra en TODOS los contadores:
    //    DOLARES : T = pantalla x 100 / valor_del_credito_en_¢ (el 02 01 05)
    //    CREDITOS: T = pantalla (el numero del meter directamente)
    //  VALIDA contra la ultima lectura R vigente y guarda en NVS. ATOMICA:
    //  si un contador falla la validacion no se guarda NADA. idx_rechazado
    //  (si se pasa) recibe el indice GAMMA_N_* del contador que motivo el
    //  rechazo (0xFF si todo OK); en_creditos (si se pasa) recibe la unidad
    //  detectada - combinar con Gamma_Calibrar_Texto() para armar la
    //  respuesta del cliente TCP.
    //  Propiedad de la validacion: un typo chico (que no cruza el corchete de
    //  2^32) no cambia N y se absorbe en silencio - el N guardado es siempre
    //  el correcto; un typo que cruzaria el corchete produce una desviacion
    //  de miles de veces la tolerancia y se rechaza.
    // -----------------------------------------------------------------------
    Gamma_Calib_Res Gamma_Calibrar_N_Desde_Pantalla(uint32_t coin_in, uint32_t coin_out,
                                                    uint32_t bill_in, uint32_t cancel,
                                                    uint32_t jackpot,
                                                    uint8_t *idx_rechazado = nullptr,
                                                    bool *en_creditos = nullptr);

private:

    //  Patron de envio por UART (igual al de RS232.h)
    void Gamma_EnviarTrama(const uint8_t *trama, size_t tamano);

    //  Devuelve el slot actual (0x40..0x78) y avanza la rotacion (+8 por transmision)
    uint8_t Gamma_TomarSlot();

    //  ¿Es uno de los 8 SEQ validos del protocolo? (filtra basura del arranque)
    bool Gamma_EsSlotValido(uint8_t byte) const;

    //  Procesa una trama completa ya validada por CRC (despacha ACK / DATA).
    //  TODA trama valida (incluidos ACK/ecos del poll) refresca el timeout de
    //  comunicacion: Datos_OK=true y Bandera_RS232_F=Bandera_RS232 (mismas
    //  banderas que usa Check_Comunicacion_Maq para declarar la caida del
    //  enlace). Las estructuras aun no decodificadas no bloquean nada: solo
    //  los meters conocidos de la tabla se guardan en contadores.
    void Gamma_DespacharTrama(const uint8_t *trama);

    //  Setea Informacion_Maquina SOLO cuando todas las fuentes individuales
    //  (denominacion 04 01 01 + bloque identidad 03 01 00) han llegado: nunca
    //  se publica un buffer a medias
    void Gamma_PublicarInfoSiCompleta();

    // ── Fase SPC (N en NVS) ─────────────────────────────────────────────────
    //  Carga perezosa desde el constructor: NVS es un global de main.cpp y el
    //  orden de construccion entre unidades es indefinido, asi que la primera
    //  lectura de meter (o el primer Set del comando TCP) trae la tabla. La
    //  carrera benigna si dos tasks entran a la vez solo repite la misma
    //  lectura/escritura de defaults.
    void     Gamma_N_Listos()          { if (!N_cargado) Gamma_Cargar_N_Vueltas(); }
    void     Gamma_Cargar_N_Vueltas();          // RAM <- NVS (crea defaults 0)
    bool     Gamma_Guardar_N_Vueltas();         // RAM -> NVS
    uint8_t  Gamma_Idx_N_Bloque(uint8_t entrada) const;
    uint8_t  Gamma_Idx_N_Objeto(const uint8_t obj[3]) const;

    //  Punto unico de salida de todo meter: vigilante de vuelta de 2^32 +
    //  salida en fase SPC (lectura + N x 2^32) mod 10^8
    uint32_t Gamma_ProcesarMeter(uint8_t idx_n, uint32_t r);

    //  Auto-recalibracion ante RAM clear: al completar un ciclo con
    //  comunicacion viva y TODOS los contadores de dinero en 0 (lecturas
    //  validas), la unica explicacion es RAM clear / maquina nueva -> el
    //  total de vida real es 0 y N=0 es lo correcto. Pisa la calibracion
    //  vieja y avisa por serial. One-shot (con N ya en 0 no dispara).
    //  La llaman los dos puntos de cierre de ciclo de Encuestas_
    //  Maquinas_Gamma
    void Gamma_Verificar_RAMClear();

    //  Regla de compatibilidad del drop entre maquinas: decide si el valor
    //  se toma del objeto individual 04 09 01 o de la entrada [09] del
    //  bloque 02 03 03 (ver Gamma_DecidirFuenteDrop en el .cpp)
    void Gamma_DecidirFuenteDrop();
    

    uint8_t  indice_slot;                 // 0..7  ->  slot = 0x40 + 8 * indice_slot

    //  Estado del ciclo de encuestas
    volatile bool flag_meter_recibido;    // la iza el receptor (otra task)
    volatile bool flag_ciclo_completo;
    uint8_t  fase_encuesta;               // GAMMA_FASE_QUERY / ESPERA / RESYNC
    uint8_t  conta_meters;                // meter actual de la tabla (0..N-1)
    uint32_t tiempo_query;                // marca de tiempo de la ventana
    uint8_t  reintentos_query;            // reintentos fallidos del meter actual

    //  Estado del receptor
    uint8_t  objeto_pendiente[3];         // objeto de la ultima QUERY enviada
    bool     hay_objeto_pendiente;        // hay QUERY cuya DATA se espera
    volatile uint8_t ultimo_seq_enviado;  // SEQ de la ultima trama transmitida
    char     rx_contador[16];             // ultima DATA: valor decimal listo p/ Set
    uint8_t  rx_objeto[3];                // ultima DATA: eco {clase,sub,meter}
    volatile bool hay_rx_contador;        // hay DATA decodificada sin leer
    char     info_gamma[20];              // Informacion_Maquina formato SAS (20 B):
                                           // [0..1] ID juego | [2..4] ID adicional
                                           // [5..9] bytes BCD: denom SAS/usuario/
                                           //        apuesta max/grupo/opciones
                                           // [10..15] tabla de pagos
                                           // [16..19] porcentaje base
    uint8_t  info_flags;                  // fuentes de info ya recibidas (denom/identidad)
    uint8_t  rx_acum[128];                // acumulador de bytes recibidos
                                           // (>= una rafaga tipica del UART: 8
                                           //  tramas DATA de 16 B por evento)
    uint16_t rx_long;                     // bytes pendientes en rx_acum

    bool Flag_Query_Gamma=false;
    bool     bloque_pendiente;             // sesion de descubrimiento del 02 03 03
    uint32_t tiempo_bloque;                // ventana de la sesion

    //  Fuente adaptable del drop (Total Drop / True In): maquinas que lo
    //  traen solo en 04 09 01, solo en el bloque [09], o por ambos lados
    //  (con valores que pueden diferir -> gana el individual). La decision
    //  es sticky y se re-evalua en cada lectura; con ambas en 0 no se
    //  setea nada hasta la primera lectura no-cero
    uint32_t drop_individual;              // ultimo valor del objeto 04 09 01
    uint32_t drop_bloque;                  // ultimo valor del bloque [09]
    uint8_t  fuente_drop;                  // GAMMA_DROP_SIN_DECIDIR/INDIVIDUAL/BLOQUE
    volatile bool drop_individual_leido;   // true cuando el 04 09 01 respondio
                                           // (aunque sea 0) o el ciclo lo salto:
                                           // el bloque NO decide la fuente solo
                                           // por haberse leido primero. La
                                           // escriben la task UART (DATA) y la
                                           // task de encuestas (salto)
    volatile bool gamma_reenganche_ok; // la iza el receptor (Task UART) al ver el
                                       // eco del poll de arranque 0x40 durante el
                                       // RESYNC; la consume la task de encuestas
                                       // para cortar la ventana de re-enganche
    uint32_t tiempo_ultimo_poll;

    //  Detector de puerta (06 02 01 - EGM Doors P1): el bit de estado llega
    //  en cada ciclo de encuestas; la primera lectura fija el estado inicial
    //  y cada transicion imprime el evento. La escribe la Task UART (despacha
    //  la DATA) y la leen el resto de las tasks -> volatile
    bool     puerta_conocida;         // ya llego la primera lectura del estado
    volatile bool     puerta_abierta; // ultimo estado conocido (true = abierta)
    volatile uint16_t aperturas_puerta;   // contador de aperturas de la maquina

    uint32_t tiempo_ultimo_tx;        // diagnostico de caidas: marca de la ultima TX
                                       // (hueco > GAMMA_GAP_TX_AVISO_MS = task detenida)
    uint32_t tiempo_ultimo_reenganche;

    //  Fase SPC: N (vueltas de 2^32) por contador identificado. R_previa es
    //  el estado del vigilante (ultima lectura cruda de cada contador); la
    //  escriben la task UART (DATA de bloque y via individual) y la leen las
    //  siguientes lecturas del mismo contador
    uint8_t  N_vueltas[GAMMA_N_TOTAL];    // persistido en NVS ("Gamma"/"N_Vueltas")
    uint32_t R_previa[GAMMA_N_TOTAL];     // ultima lectura cruda por contador
    bool     R_valida[GAMMA_N_TOTAL];     // ya hay lectura previa vigente
    bool     N_cargado;                   // NVS ya leido (carga perezosa)


    
    uint32_t tiempo_ultimo_rx_gamma;
};

//  Instancia global del enlace Gamma (definida en GammaProLink.cpp)
extern GammaProlink Gamma;

// ----------------------------------------------------------------------------
//  Texto del resultado de la calibracion por pantalla (para la respuesta al
//  cliente TCP: "OK: ..." / "ERROR: ..."). Texto fijo, seguro para strcpy.
// ----------------------------------------------------------------------------
const char *Gamma_Calibrar_Texto(Gamma_Calib_Res res);

// ----------------------------------------------------------------------------
//  Autotest de banco del receptor para CUALQUIER contador: simula la DATA
//  del objeto {clase,subgrupo,meter} identica a las de la captura (partida
//  en dos rafagas, con basura previa) y verifica que Gamma_ProcesarRx genera
//  el contador (string decimal listo para Set_Contadores), imprimiendolo.
//  NO escribe en Contadores. Simulacion pura: no transmite nada ni requiere
//  el driver UART; llamarla sin la task de encuestas corriendo.
// ----------------------------------------------------------------------------
bool Gamma_Test_Rx_Meter(uint8_t clase, uint8_t subgrupo, uint8_t meter,
                         uint32_t valor_prueba);

// ----------------------------------------------------------------------------
//  Autotest de banco del BLOQUE MULTI-METER 02 03 03 (objeto del ciclo de
//  encuestas): inyecta la trama de 108 B (24 entradas BE32) con SEQ
//  equivocado y luego la correcta partida en dos rafagas, y verifica que el
//  bloque iza flag_meter_recibido (el ciclo AVANZA: no bloquea las
//  consultas) sin publicar contador simple. Simulacion pura: no transmite
//  nada ni requiere el driver UART; llamarla sin la task de encuestas.
// ----------------------------------------------------------------------------
bool Gamma_Test_Rx_Bloque();

#endif // GAMMAPROLINK_H