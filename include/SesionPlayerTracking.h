/**
 * @file SesionPlayerTracking.h
 * @brief Sistema de gestión de sesiones sin tarjeta (Player Tracking)
 * @version 2.0 - Refactorización con separación de responsabilidades
 * @author Globus Sistemas SAS
 */

#ifndef SESION_PLAYER_TRACKING_H
#define SESION_PLAYER_TRACKING_H

#include <Arduino.h>
#include "ESP32Time.h"

/* ====================================================================================
   DEFINICIONES Y CONSTANTES
   ==================================================================================== */

#define SPT_TIMEOUT_INACTIVIDAD_BASE    90000UL    // 90 segundos base
#define SPT_TIMEOUT_PAYOUT              10000UL    // 10 segundos para payout
#define SPT_TIMEOUT_VERIFICACION        5000UL     // 5 segundos para verificación
#define SPT_CREDITO_MINIMO              10         // Créditos mínimos
#define SPT_CORTE_HORARIO_SEGUNDOS      3600UL     // 1 hora en segundos
#define SPT_MARGEN_SEGURIDAD            5000UL     // 5 segundos de margen

/* ====================================================================================
   ENUMS - Máquina de Estados
   ==================================================================================== */

/**
 * @brief Estados de la sesión sin tarjeta
 */
enum SPT_EstadoSesion {
    SPT_ESTADO_INACTIVO,           ///< Sin sesión activa
    SPT_ESTADO_SESION_ACTIVA,      ///< Sesión sin tarjeta activa
    SPT_ESTADO_PENDIENTE_CIERRE,   ///< Esperando cierre definitivo
    SPT_ESTADO_BLOQUEADO,          ///< Bloqueado por premio pendiente
    SPT_ESTADO_ERROR               ///< Estado de error
};

/**
 * @brief Eventos que pueden ocurrir en la sesión
 */
enum SPT_Evento {
    SPT_EVENTO_NINGUNO = 0,
    SPT_EVENTO_BILLETE_INSERTADO,     ///< Se insertó un billete
    SPT_EVENTO_MAQUINA_ACTIVA,        ///< Máquina entró en juego
    SPT_EVENTO_TARJETA_INSERTADA,     ///< Se insertó tarjeta RFID
    SPT_EVENTO_TARJETA_RETIRADA,      ///< Se retiró tarjeta RFID
    SPT_EVENTO_INACTIVIDAD,           ///< Timeout de inactividad
    SPT_EVENTO_CORTE_HORARIO,         /// Corte por cambio de hora
    SPT_EVENTO_PREMIO_PENDIENTE,      ///< Hay premio pendiente de pago
    SPT_EVENTO_PREMIO_FINALIZADO,     ///< Premio finalizado
    SPT_EVENTO_PERDIDA_COMUNICACION,  ///< Pérdida de comunicación con MET
    SPT_EVENTO_TIMEOUT_VERIFICACION  ///< Timeout de verificación
};

/**
 * @brief Causas de cierre de sesión
 */
enum SPT_CausaCierre {
    SPT_CIERRE_NINGUNA = 0,
    SPT_CIERRE_INSERCION_TARJETA,      ///< Usuario insertó tarjeta
    SPT_CIERRE_INACTIVIDAD,            ///< Sin actividad por tiempo prolongado
    SPT_CIERRE_CORTE_HORARIO,          /// Corte por cambio de hora
    SPT_CIERRE_PERDIDA_COMUNICACION,    ///< Pérdida de comunicación con MET
    SPT_CIERRE_CREDITOS_INSUFICIENTES,  ///< Créditos por debajo del mínimo
    SPT_CIERRE_MANUAL,                  ///< Cierre manual
    SPT_CIERRE_ERROR                    ///< Cierre por error
};

/**
 * @brief Métodos de apertura de sesión
 */
enum SPT_MetodoApertura {
    SPT_APERTURA_NINGUNO = 0,
    SPT_APERTURA_BILLETE,        ///< Por inserción de billete
    SPT_APERTURA_COIN_IN,        ///< Por coin in (monedas)
    SPT_APERTURA_REINTENTO,      ///< Por reintento después de corte
    SPT_APERTURA_TARJETA_RETIRADA ///< Por retirar tarjeta con créditos
};

/* ====================================================================================
   ESTRUCTURAS DE DATOS
   ==================================================================================== */

/**
 * @brief Estructura con la información de una sesión
 */
struct SPT_Sesion {
    SPT_EstadoSesion estado;                  ///< Estado actual de la sesión
    SPT_MetodoApertura metodo_apertura;      ///< Método por el que se abrió la sesión
    time_t epoch_inicio;                     ///< Epoch completo cuando inició la sesión
    time_t epoch_cierre;                     ///< Epoch cuando se cerró la sesión
    unsigned long millis_inicio;             ///< Millis cuando inició (para timeouts locales)
    unsigned long millis_ultima_actividad;    ///< Millis de última actividad
    uint8_t hora_inicio;                     ///< Hora del día (0-23) para control de corte
    int creditos_al_inicio;                  ///< Créditos al inicio de sesión
    int creditos_actuales;                   ///< Créditos actuales
    uint16_t sesiones_acumuladas;            ///< Contador de sesiones acumuladas
    bool hay_premio_pendiente;               ///< Flag de premio pendiente
    bool maquina_en_juego;                   ///< Flag de máquina en juego
    bool comunicacion_met_ok;                ///< Flag de comunicación con MET

    // Ventana de seguridad después de cierre de sesión con tarjeta
    unsigned long millis_ultimo_cierre_tarjeta;  ///< Timestamp del último cierre de sesión con tarjeta
    bool cierre_por_tarjeta_detectado;          ///< Flag que indica cierre reciente de sesión con tarjeta

    // Constructor por defecto
    SPT_Sesion() :
        estado(SPT_ESTADO_INACTIVO),
        metodo_apertura(SPT_APERTURA_NINGUNO),
        epoch_inicio(0),
        epoch_cierre(0),
        millis_inicio(0),
        millis_ultima_actividad(0),
        hora_inicio(255),
        creditos_al_inicio(0),
        creditos_actuales(0),
        sesiones_acumuladas(0),
        hay_premio_pendiente(false),
        maquina_en_juego(false),
        comunicacion_met_ok(false),
        millis_ultimo_cierre_tarjeta(0),
        cierre_por_tarjeta_detectado(false) {}
};

/**
 * @brief Estructura con el resultado de una operación
 */
struct SPT_Resultado {
    bool exito;                              ///< true si la operación fue exitosa
    SPT_EstadoSesion estado_resultante;     ///< Estado después de la operación
    SPT_CausaCierre causa_cierre;            ///< Causa del cierre (si aplica)
    String mensaje;                          ///< Mensaje descriptivo

    SPT_Resultado() : exito(false), estado_resultante(SPT_ESTADO_INACTIVO),
                      causa_cierre(SPT_CIERRE_NINGUNA), mensaje("") {}
};

/**
 * @brief Callback para eventos de sesión
 */
typedef void (*SPT_Callback)(SPT_EstadoSesion estado_anterior,
                             SPT_EstadoSesion estado_nuevo,
                             SPT_CausaCierre causa);

/**
 * @brief Callback para registrar sesiones (evita dependencia circular)
 */
typedef void (*SPT_SesionCallback)(bool abrir, String json_sesion);

/* ====================================================================================
   CLASE PRINCIPAL - Gestor de Sesiones Player Tracking
   ==================================================================================== */

class SesionPlayerTracking {
private:
    SPT_Sesion sesion_actual;              ///< Datos de la sesión actual
    SPT_Callback callback_cambio_estado;   ///< Callback para cambios de estado
    SPT_SesionCallback callback_sesion;    ///< Callback para registrar sesiones

    // Variables internas para control de timeouts
    bool premio_pendiente_flag_anterior;  ///< Flag anterior para detectar flancos
    bool tarjeta_insertada_anterior;       ///< Flag anterior para detectar inserción
    bool comunicacion_ok_anterior;         ///< Flag anterior para detectar pérdida de comunicación

    // Configuración
    unsigned long timeout_inactividad_ms;  ///< Timeout configurable de inactividad

    /**
     * @brief Valida si se puede abrir una nueva sesión
     * @param metodo_intentado Método de apertura que se intenta (billete o coin-in)
     * @param maquina_en_juego true si la máquina está en juego
     * @param creditos Créditos actuales
     * @param contadores_ok true si los contadores no están en cero
     * @return true si se puede abrir sesión
     */
    bool puedeAbrirSesion(SPT_MetodoApertura metodo_intentado, bool maquina_en_juego, int creditos, bool contadores_ok);

    /**
     * @brief Ejecuta el cierre de sesión con la causa especificada
     */
    void ejecutarCierre(SPT_CausaCierre causa, ESP32Time& rtc);

    /**
     * @brief Registra el evento de cambio de estado
     */
    void notificarCambioEstado(SPT_EstadoSesion estado_anterior, SPT_CausaCierre causa);

public:
    /**
     * @brief Constructor
     */
    SesionPlayerTracking();

    /**
     * @brief Destructor
     */
    ~SesionPlayerTracking();

    /* ------------------------------------------------------------------------ */
    /* MÉTODOS DE CONFIGURACIÓN                                                 */
    /* ------------------------------------------------------------------------ */

    /**
     * @brief Establece el callback para cambios de estado
     * @param callback Función a llamar cuando cambie el estado
     */
    void setCallback(SPT_Callback callback);

    /**
     * @brief Establece el callback para registrar sesiones
     * @param callback Función a llamar para registrar sesiones (evita dependencia circular)
     */
    void setSesionCallback(SPT_SesionCallback callback);

    /**
     * @brief Configura el timeout de inactividad
     * @param timeout_ms Timeout en milisegundos
     */
    void setTimeoutInactividad(unsigned long timeout_ms);

    /* ------------------------------------------------------------------------ */
    /* MÉTODOS DE CONSULTA                                                      */
    /* ------------------------------------------------------------------------ */

    /**
     * @brief Obtiene el estado actual de la sesión
     * @return Estado actual
     */
    SPT_EstadoSesion getEstado() const;

    /**
     * @brief Obtiene si hay una sesión activa
     * @return true si hay sesión activa
     */
    bool isSesionActiva() const;

    /**
     * @brief Obtiene si hay un cierre pendiente
     * @return true si hay cierre pendiente
     */
    bool isCierrePendiente() const;

    /**
     * @brief Obtiene el número de sesiones acumuladas
     * @return Número de sesiones acumuladas
     */
    uint16_t getSesionesAcumuladas() const;

    /**
     * @brief Obtiene información de la sesión actual
     * @return Estructura con la información de la sesión
     */
    const SPT_Sesion& getInfoSesion() const;

    /**
     * @brief Obtiene el método de apertura de la sesión actual
     * @return Método de apertura
     */
    SPT_MetodoApertura getMetodoApertura() const;

    /* ------------------------------------------------------------------------ */
    /* MÉTODOS DE CONTROL - MÁQUINA DE ESTADOS                                  */
    /* ------------------------------------------------------------------------ */

    /**
     * @brief Procesa el ciclo de la máquina de estados
     * @param rtc Referencia al objeto RTC
     * @return Resultado de la operación
     */
    SPT_Resultado procesar(ESP32Time& rtc);

    /**
     * @brief Intenta abrir una nueva sesión
     * @param metodo Método de apertura
     * @param rtc Referencia al objeto RTC
     * @param creditos Créditos actuales
     * @param maquina_en_juego true si la máquina está en juego
     * @return Resultado de la operación
     */
    SPT_Resultado abrirSesion(SPT_MetodoApertura metodo, ESP32Time& rtc,
                              int creditos, bool maquina_en_juego);

    /**
     * @brief Solicita el cierre de la sesión
     * @param causa Causa del cierre
     * @param rtc Referencia al objeto RTC
     * @return Resultado de la operación
     */
    SPT_Resultado cerrarSesion(SPT_CausaCierre causa, ESP32Time& rtc);

    /**
     * @brief Procesa un evento externo
     * @param evento Evento a procesar
     * @param rtc Referencia al objeto RTC
     * @param creditos Créditos actuales
     * @return Resultado de la operación
     */
    SPT_Resultado procesarEvento(SPT_Evento evento, ESP32Time& rtc, int creditos);

    /* ------------------------------------------------------------------------ */
    /* MÉTODOS DE ACTUALIZACIÓN DE ESTADO                                        */
    /* ------------------------------------------------------------------------ */

    /**
     * @brief Actualiza el estado de la máquina en juego
     * @param en_juego true si la máquina está en juego
     */
    void actualizarMaquinaEnJuego(bool en_juego);

    /**
     * @brief Actualiza el estado del premio pendiente
     * @param premio_pendiente true si hay premio pendiente
     */
    void actualizarPremioPendiente(bool premio_pendiente);

    /**
     * @brief Actualiza el estado de comunicación con MET
     * @param comunicacion_ok true si hay comunicación
     */
    void actualizarComunicacionMET(bool comunicacion_ok);

    /**
     * @brief Actualiza los créditos actuales
     * @param creditos Créditos actuales
     */
    void actualizarCreditos(int creditos);

    /**
     * @brief Actualiza el estado de la tarjeta insertada
     * @param tarjeta_insertada true si hay tarjeta insertada
     */
    void actualizarTarjetaInsertada(bool tarjeta_insertada);

    /**
     * @brief Registra el cierre de una sesión con tarjeta (activa ventana de seguridad)
     * Debe llamarse cuando se cierra una sesión con tarjeta para evitar que se abra
     * inmediatamente una sesión sin tarjeta por coin-in
     */
    void registrarCierreSesionConTarjeta();

    /* ------------------------------------------------------------------------ */
    /* MÉTODOS DE VERIFICACIÓN                                                  */
    /* ------------------------------------------------------------------------ */

    /**
     * @brief Verifica si debe ocurrir el corte horario
     * @param rtc Referencia al objeto RTC
     * @return true si debe ocurrir el corte
     */
    bool verificarCorteHorario(ESP32Time& rtc);

    /**
     * @brief Verifica si debe ocurrir el cierre por inactividad
     * @return true si debe ocurrir el cierre
     */
    bool verificarInactividad();

    /**
     * @brief Verifica si hay contadores en cero
     * @return true si todos los contadores están en cero
     */
    bool verificarContadoresEnCero();

    /* ------------------------------------------------------------------------ */
    /* MÉTODOS DE UTILIDAD                                                      */
    /* ------------------------------------------------------------------------ */

    /**
     * @brief Reinicia el contador de sesiones acumuladas
     */
    void reiniciarSesionesAcumuladas();

    /**
     * @brief Obtiene el tiempo transcurrido desde el inicio de la sesión
     * @return Tiempo transcurrido en milisegundos
     */
    unsigned long getTiempoTranscurrido() const;

    /**
     * @brief Obtiene el tiempo transcurrido desde la última actividad
     * @return Tiempo transcurrido en milisegundos
     */
    unsigned long getTiempoInactividad() const;

    /**
     * @brief Convierte el estado a string para logging
     * @param estado Estado a convertir
     * @return String con el nombre del estado
     */
    static String estadoToString(SPT_EstadoSesion estado);

    /**
     * @brief Convierte el evento a string para logging
     * @param evento Evento a convertir
     * @return String con el nombre del evento
     */
    static String eventoToString(SPT_Evento evento);

    /**
     * @brief Convierte la causa de cierre a string para logging
     * @param causa Causa a convertir
     * @return String con el nombre de la causa
     */
    static String causaToString(SPT_CausaCierre causa);
};

#endif // SESION_PLAYER_TRACKING_H
