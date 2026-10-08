/**
 * @file SesionPlayerTracking.cpp
 * @brief Implementación del sistema de gestión de sesiones sin tarjeta
 * @version 2.0 - Refactorización con separación de responsabilidades
 * @author Globus Sistemas SAS
 */

#include "SesionPlayerTracking.h"
#include "Clase_Variables_Globales.h"
#include "Contadores.h"

/* ====================================================================================
   VARIABLES EXTERNAS (referenciadas desde el código existente)
   ==================================================================================== */

extern Variables_Globales Variables_globales;
extern Contadores_SAS contadores;

/* ====================================================================================
   CONSTRUCTOR Y DESTRUCTOR
   ==================================================================================== */

/**
 * @brief Constructor de la clase
 */
SesionPlayerTracking::SesionPlayerTracking() :
    callback_cambio_estado(nullptr),
    callback_sesion(nullptr),
    premio_pendiente_flag_anterior(false),
    tarjeta_insertada_anterior(Variables_globales.Get_Variable_Global(Flag_Sesion_RFID)),  // Sincronizar con estado actual
    comunicacion_ok_anterior(true),
    timeout_inactividad_ms(SPT_TIMEOUT_INACTIVIDAD_BASE)
{
    // Inicializar estructura de sesión
    sesion_actual = SPT_Sesion();
}

/**
 * @brief Destructor de la clase
 */
SesionPlayerTracking::~SesionPlayerTracking()
{
    // Limpieza si es necesaria
}

/* ====================================================================================
   MÉTODOS DE CONFIGURACIÓN
   ==================================================================================== */

void SesionPlayerTracking::setCallback(SPT_Callback callback)
{
    callback_cambio_estado = callback;
}

void SesionPlayerTracking::setSesionCallback(SPT_SesionCallback callback)
{
    callback_sesion = callback;
}

void SesionPlayerTracking::setTimeoutInactividad(unsigned long timeout_ms)
{
    timeout_inactividad_ms = timeout_ms;
}

/* ====================================================================================
   MÉTODOS DE CONSULTA
   ==================================================================================== */

SPT_EstadoSesion SesionPlayerTracking::getEstado() const
{
    return sesion_actual.estado;
}

bool SesionPlayerTracking::isSesionActiva() const
{
    return (sesion_actual.estado == SPT_ESTADO_SESION_ACTIVA);
}

bool SesionPlayerTracking::isCierrePendiente() const
{
    return (sesion_actual.estado == SPT_ESTADO_PENDIENTE_CIERRE);
}

uint16_t SesionPlayerTracking::getSesionesAcumuladas() const
{
    return sesion_actual.sesiones_acumuladas;
}

const SPT_Sesion& SesionPlayerTracking::getInfoSesion() const
{
    return sesion_actual;
}

SPT_MetodoApertura SesionPlayerTracking::getMetodoApertura() const
{
    return sesion_actual.metodo_apertura;
}

/* ====================================================================================
   MÉTODOS PRIVADOS
   ==================================================================================== */

bool SesionPlayerTracking::puedeAbrirSesion(SPT_MetodoApertura metodo_intentado, bool maquina_en_juego, int creditos, bool contadores_ok)
{
    #ifdef DEBUG_SESIONES_A
    Serial.printf("[SPT-DEBUG] puedeAbrirSesion | metodo=%d, estado=%d, maquina=%d, creditos=%d, contadores_ok=%d\n",
                  metodo_intentado, sesion_actual.estado, maquina_en_juego, creditos, contadores_ok);
    #endif

    // No abrir si ya hay sesión activa
    if (sesion_actual.estado == SPT_ESTADO_SESION_ACTIVA ||
        sesion_actual.estado == SPT_ESTADO_PENDIENTE_CIERRE) {
        #ifdef DEBUG_SESIONES_A
        Serial.println("[SPT-DEBUG] ❌ FAIL: Ya hay sesión activa");
        #endif
        return false;
    }

    // No abrir sesión sin tarjeta si hay una tarjeta insertada
    bool tarjeta_insertada = Variables_globales.Get_Variable_Global(Flag_Sesion_RFID);
    if (tarjeta_insertada) {
        #ifdef DEBUG_SESIONES_A
        Serial.println("[SPT-DEBUG] ❌ FAIL: Tarjeta insertada detectada");
        #endif
        return false;
    }

    // Verificar que los contadores no estén en cero
    if (!contadores_ok) {
        #ifdef DEBUG_SESIONES_A
        Serial.println("[SPT-DEBUG] ❌ FAIL: Contadores en cero");
        #endif
        return false;
    }

    // ========================================================================
    // VENTANA DE SEGURIDAD: Bloquear coin-in después de cierre de sesión con tarjeta
    // ========================================================================
    if (sesion_actual.cierre_por_tarjeta_detectado) {
        unsigned long tiempo_desde_cierre = millis() - sesion_actual.millis_ultimo_cierre_tarjeta;

        if (tiempo_desde_cierre < SPT_MARGEN_SEGURIDAD) {
            // Solo bloquear si es apertura por coin-in
            if (metodo_intentado == SPT_APERTURA_COIN_IN) {
                #ifdef DEBUG_SESIONES_A
                Serial.printf("[SPT-DEBUG] ❌ FAIL: Ventana de seguridad activa (%lu ms restantes) - Solo billete permitido\n",
                              SPT_MARGEN_SEGURIDAD - tiempo_desde_cierre);
                #endif
                return false;
            }
            // Permitir apertura por billete incluso en ventana de seguridad
            #ifdef DEBUG_SESIONES_A
            Serial.printf("[SPT-DEBUG] ⚠️ WARN: Ventana de seguridad activa pero permite BILLETE\n");
            #endif
        } else {
            // Ventana de seguridad expirada
            sesion_actual.cierre_por_tarjeta_detectado = false;
            #ifdef DEBUG_SESIONES_A
            Serial.println("[SPT-DEBUG] ✅ Ventana de seguridad expirada");
            #endif
        }
    }

    // Verificar créditos mínimos Y máquina en juego (ambas requeridas)
    if (creditos <= SPT_CREDITO_MINIMO || !maquina_en_juego) {
        #ifdef DEBUG_SESIONES_A
        Serial.printf("[SPT-DEBUG] ❌ FAIL: Créditos=%d (min:%d) o maquina=%d\n",
                      creditos, SPT_CREDITO_MINIMO, maquina_en_juego);
        #endif
        return false;
    }

    #ifdef DEBUG_SESIONES_A
    Serial.println("[SPT-DEBUG] ✅ PASS: Todas las condiciones cumplidas");
    #endif
    return true;
}

void SesionPlayerTracking::ejecutarCierre(SPT_CausaCierre causa, ESP32Time& rtc)
{
    SPT_EstadoSesion estado_anterior = sesion_actual.estado;

    // Actualizar datos de cierre
    sesion_actual.epoch_cierre = rtc.getEpoch();
    sesion_actual.estado = SPT_ESTADO_INACTIVO;

    // Incrementar contador de sesiones acumuladas
    sesion_actual.sesiones_acumuladas++;

    // Registrar en el sistema existente vía callback
    if (callback_sesion != nullptr)
    {
        callback_sesion(false, ""); // false = cierre
    }

    // Actualizar variable global
    Variables_globales.Set_Variable_Global(Flag_Sesion_Sin_Tarjeta, false);

    // Notificar cambio de estado
    notificarCambioEstado(estado_anterior, causa);
}

void SesionPlayerTracking::notificarCambioEstado(SPT_EstadoSesion estado_anterior, SPT_CausaCierre causa)
{
    // Llamar al callback si está configurado
    if (callback_cambio_estado != nullptr) {
        callback_cambio_estado(estado_anterior, sesion_actual.estado, causa);
    }

    // Logging del cambio de estado
    #ifdef DEBUG_SESIONES_A
    Serial.printf("[SPT] Estado: %s -> %s | Causa: %s | Acumuladas: %u\n",
                  estadoToString(estado_anterior).c_str(),
                  estadoToString(sesion_actual.estado).c_str(),
                  causaToString(causa).c_str(),
                  sesion_actual.sesiones_acumuladas);
    #endif
}

/* ====================================================================================
   MÉTODOS DE CONTROL - MÁQUINA DE ESTADOS
   ==================================================================================== */

/**
 * @brief Función principal de procesamiento de la máquina de estados
 * @param rtc Referencia al objeto RTC
 * @return Resultado de la operación
 */
SPT_Resultado SesionPlayerTracking::procesar(ESP32Time& rtc)
{
    SPT_Resultado resultado;
    resultado.estado_resultante = sesion_actual.estado;

    // Obtener estado actual de las variables externas
    bool tarjeta_insertada = Variables_globales.Get_Variable_Global(Flag_Sesion_RFID);
    bool maquina_en_juego = Variables_globales.Get_Variable_Global(Flag_Maquina_En_Juego);
    bool comunicacion_ok = Variables_globales.Get_Variable_Global(Comunicacion_Maq);
    int creditos = contadores.Get_Contadores_Int(24);
    bool contadores_ok = !verificarContadoresEnCero();

    unsigned long millis_actual = millis();
    time_t epoch_actual = rtc.getEpoch();
    uint8_t hora_actual = rtc.getHour(true);

    // ==========================================================================
    // ESTADO INACTIVO - Esperando condiciones para abrir sesión
    // ==========================================================================
    if (sesion_actual.estado == SPT_ESTADO_INACTIVO)
    {
        // Detectar método de apertura antes de validar
        SPT_MetodoApertura metodo = SPT_APERTURA_COIN_IN;
        bool maquina_activa = Variables_globales.Get_Variable_Global(Flag_Maquina_En_Juego);

        if (maquina_activa) {
            metodo = SPT_APERTURA_COIN_IN;
        }
        // Nota: El método de billete se detecta vía evento externo (SPT_EVENTO_BILLETE_INSERTADO)

        // Verificar si se puede abrir sesión con el método detectado
        if (puedeAbrirSesion(metodo, maquina_en_juego, creditos, contadores_ok))
        {
            return abrirSesion(metodo, rtc, creditos, maquina_en_juego);
        }

        // ❌ ELIMINADO: Apertura automática al retirar tarjeta con créditos
        // Ya no se abre sesión sin tarjeta automáticamente cuando se retira la tarjeta
        // if (!tarjeta_insertada && tarjeta_insertada_anterior &&
        //     creditos > SPT_CREDITO_MINIMO && maquina_en_juego &&
        //     contadores_ok)
        // {
        //     return abrirSesion(SPT_APERTURA_TARJETA_RETIRADA, rtc, creditos, maquina_en_juego);
        // }
    }

    // ==========================================================================
    // ESTADO SESION_ACTIVA - Procesando eventos de sesión activa
    // ==========================================================================
    else if (sesion_actual.estado == SPT_ESTADO_SESION_ACTIVA)
    {
        // DEBUG: Mostrar estado de variables relevantes
        static unsigned long last_debug_time = 0;
        if (millis_actual - last_debug_time >= 2000) {  // Cada 2 segundos
            // Serial.printf("[SPT-STATUS] tarjeta_insertada=%d, tarjeta_anterior=%d\n",
            //              tarjeta_insertada, tarjeta_insertada_anterior);
            last_debug_time = millis_actual;
        }
        // --------------------------------------------------------------------
        // VERIFICACIÓN 1: Corte horario (prioridad alta)
        // --------------------------------------------------------------------
        if (verificarCorteHorario(rtc))
        {
            #ifdef DEBUG_SESIONES_A
            Serial.printf("[SPT] 🔔 Corte horario detectado: %02d:00\n", hora_actual);
            #endif

            // Cerrar sesión actual
            ejecutarCierre(SPT_CIERRE_CORTE_HORARIO, rtc);

            // Inmediatamente abrir nueva sesión
            resultado = abrirSesion(SPT_APERTURA_REINTENTO, rtc, creditos, maquina_en_juego);

            resultado.mensaje = "Corte horario - Nueva sesión iniciada";
            return resultado;
        }

        // --------------------------------------------------------------------
        // VERIFICACIÓN 2: Inserción de tarjeta (prioridad alta)
        // --------------------------------------------------------------------
        if (tarjeta_insertada && !tarjeta_insertada_anterior)
        {
            Serial.println("[SPT] 💳 Tarjeta insertada - Cerrando sesión sin tarjeta");
            Serial.printf("[SPT-DEBUG] tarjeta_insertada=%d, tarjeta_insertada_anterior=%d\n",
                         tarjeta_insertada, tarjeta_insertada_anterior);

            ejecutarCierre(SPT_CIERRE_INSERCION_TARJETA, rtc);
            resultado.exito = true;
            resultado.mensaje = "Sesión cerrada por inserción de tarjeta";

            // IMPORTANTE: Actualizar flag anterior para evitar bucle infinito
            tarjeta_insertada_anterior = tarjeta_insertada;

            return resultado;
        }

        // --------------------------------------------------------------------
        // VERIFICACIÓN 3: Pérdida de comunicación con MET
        // --------------------------------------------------------------------
        if (!comunicacion_ok && comunicacion_ok_anterior)
        {
            #ifdef DEBUG_SESIONES_A
            Serial.println("[SPT] 🔌 Pérdida de comunicación con MET");
            #endif

            ejecutarCierre(SPT_CIERRE_PERDIDA_COMUNICACION, rtc);
            resultado.exito = true;
            resultado.mensaje = "Sesión cerrada por pérdida de comunicación";
            return resultado;
        }

        // --------------------------------------------------------------------
        // VERIFICACIÓN 4: Inactividad y créditos bajos
        // --------------------------------------------------------------------
        if (!maquina_en_juego && creditos < SPT_CREDITO_MINIMO)
        {
            if (verificarInactividad())
            {
                // Transición a estado pendiente de cierre
                SPT_EstadoSesion estado_anterior = sesion_actual.estado;
                sesion_actual.estado = SPT_ESTADO_PENDIENTE_CIERRE;

                #ifdef DEBUG_SESIONES_A
                Serial.println("[SPT] ⏳ Cierre de sesión pendiente (inactividad)");
                #endif

                // Actualizar timestamp de solicitud de cierre
                sesion_actual.millis_ultima_actividad = millis_actual;

                resultado.exito = true;
                resultado.estado_resultante = SPT_ESTADO_PENDIENTE_CIERRE;
                resultado.mensaje = "Sesión en cierre pendiente";

                notificarCambioEstado(estado_anterior, SPT_CIERRE_INACTIVIDAD);
                return resultado;
            }
        }

        // Actualizar timestamp de actividad si hay actividad
        if (maquina_en_juego || creditos >= SPT_CREDITO_MINIMO)
        {
            sesion_actual.millis_ultima_actividad = millis_actual;
        }
    }

    // ==========================================================================
    // ESTADO PENDIENTE_CIERRE - Esperando condiciones finales para cierre
    // ==========================================================================
    else if (sesion_actual.estado == SPT_ESTADO_PENDIENTE_CIERRE)
    {
        // Calcular tiempo desde solicitud de cierre
        unsigned long tiempo_pendiente = millis_actual - sesion_actual.millis_ultima_actividad;

        // Verificar condiciones para cierre definitivo
        bool timeout_cumplido = (tiempo_pendiente >= SPT_TIMEOUT_PAYOUT);
        bool creditos_bajos = (creditos < SPT_CREDITO_MINIMO);
        bool premio_finalizado = (premio_pendiente_flag_anterior && !sesion_actual.hay_premio_pendiente);

        // Actualizar flag anterior para próximo ciclo
        premio_pendiente_flag_anterior = sesion_actual.hay_premio_pendiente;

        // Ejecutar cierre si se cumplen las condiciones
        if (premio_finalizado || timeout_cumplido || creditos_bajos)
        {
            #ifdef DEBUG_SESIONES_A
            Serial.printf("[SPT] ✅ Cierre definitivo | Timeout: %u | Créditos: %d | Premio: %s\n",
                          tiempo_pendiente, creditos,
                          premio_finalizado ? "Finalizado" : "Pendiente");
            #endif

            ejecutarCierre(SPT_CIERRE_INACTIVIDAD, rtc);
            resultado.exito = true;
            resultado.mensaje = "Sesión cerrada por inactividad definitiva";
            return resultado;
        }

        // Si hay premio pendiente, esperar
        if (sesion_actual.hay_premio_pendiente)
        {
            #ifdef DEBUG_SESIONES_A
            Serial.println("[SPT] 💰 Esperando finalización de premio...");
            #endif
            resultado.mensaje = "Esperando finalización de premio";
            return resultado;
        }
    }

    // ==========================================================================
    // ACTUALIZACIÓN DE FLAGS ANTERIORES
    // ==========================================================================
    tarjeta_insertada_anterior = tarjeta_insertada;
    comunicacion_ok_anterior = comunicacion_ok;

    // Actualizar datos de sesión
    sesion_actual.maquina_en_juego = maquina_en_juego;
    sesion_actual.comunicacion_met_ok = comunicacion_ok;
    sesion_actual.creditos_actuales = creditos;

    return resultado;
}

/**
 * @brief Abre una nueva sesión sin tarjeta
 */
SPT_Resultado SesionPlayerTracking::abrirSesion(SPT_MetodoApertura metodo,
                                                  ESP32Time& rtc,
                                                  int creditos,
                                                  bool maquina_en_juego)
{
    SPT_Resultado resultado;
    SPT_EstadoSesion estado_anterior = sesion_actual.estado;

    // Verificar que se pueda abrir sesión (pasar el método para validación de ventana de seguridad)
    if (!puedeAbrirSesion(metodo, maquina_en_juego, creditos, !verificarContadoresEnCero()))
    {
        resultado.mensaje = "No se puede abrir sesión en este momento";
        return resultado;
    }

    // Inicializar datos de la nueva sesión
    unsigned long millis_actual = millis();
    time_t epoch_actual = rtc.getEpoch();
    uint8_t hora_actual = rtc.getHour(true);

    sesion_actual.estado = SPT_ESTADO_SESION_ACTIVA;
    sesion_actual.metodo_apertura = metodo;
    sesion_actual.epoch_inicio = epoch_actual;
    sesion_actual.epoch_cierre = 0;
    sesion_actual.millis_inicio = millis_actual;
    sesion_actual.millis_ultima_actividad = millis_actual;
    sesion_actual.hora_inicio = hora_actual;
    sesion_actual.creditos_al_inicio = creditos;
    sesion_actual.creditos_actuales = creditos;
    sesion_actual.hay_premio_pendiente = false;
    sesion_actual.maquina_en_juego = maquina_en_juego;

    // Actualizar variable global
    Variables_globales.Set_Variable_Global(Flag_Sesion_Sin_Tarjeta, true);

    // Registrar en el sistema existente vía callback
    if (callback_sesion != nullptr)
    {
        callback_sesion(true, ""); // true = apertura
    }

    // Logging del método de apertura
    #ifdef DEBUG_SESIONES_A
    const char* metodo_str = "";
    switch (metodo) {
        case SPT_APERTURA_BILLETE: metodo_str = "Billete"; break;
        case SPT_APERTURA_COIN_IN: metodo_str = "Coin In"; break;
        case SPT_APERTURA_REINTENTO: metodo_str = "Reintento"; break;
        case SPT_APERTURA_TARJETA_RETIRADA: metodo_str = "Tarjeta Retirada"; break;
        default: metodo_str = "Desconocido"; break;
    }
    Serial.printf("[SPT] 🟢 Sesión abierta | Método: %s | Hora: %02d:00 | Créditos: %d\n",
                  metodo_str, hora_actual, creditos);
    #endif

    resultado.exito = true;
    resultado.estado_resultante = SPT_ESTADO_SESION_ACTIVA;
    resultado.mensaje = "Sesión iniciada correctamente";

    notificarCambioEstado(estado_anterior, SPT_CIERRE_NINGUNA);

    return resultado;
}

/**
 * @brief Cierra la sesión actual
 */
SPT_Resultado SesionPlayerTracking::cerrarSesion(SPT_CausaCierre causa, ESP32Time& rtc)
{
    SPT_Resultado resultado;

    if (sesion_actual.estado == SPT_ESTADO_INACTIVO)
    {
        resultado.mensaje = "No hay sesión activa para cerrar";
        return resultado;
    }

    ejecutarCierre(causa, rtc);

    resultado.exito = true;
    resultado.estado_resultante = SPT_ESTADO_INACTIVO;
    resultado.causa_cierre = causa;
    resultado.mensaje = "Sesión cerrada - Causa: " + causaToString(causa);

    return resultado;
}

/**
 * @brief Procesa un evento externo específico
 */
SPT_Resultado SesionPlayerTracking::procesarEvento(SPT_Evento evento, ESP32Time& rtc, int creditos)
{
    SPT_Resultado resultado;

    switch (evento)
    {
        case SPT_EVENTO_BILLETE_INSERTADO:
            if (sesion_actual.estado == SPT_ESTADO_INACTIVO)
            {
                resultado = abrirSesion(SPT_APERTURA_BILLETE, rtc, creditos, true);
                #ifdef DEBUG_SESIONES_A
                Serial.println("[SPT] 💸 Sesión abierta por billete insertado");
                #endif
            }
            else if (sesion_actual.estado == SPT_ESTADO_SESION_ACTIVA)
            {
                // Actualizar actividad
                sesion_actual.millis_ultima_actividad = millis();
                resultado.exito = true;
                resultado.mensaje = "Actividad registrada - Billete insertado";
            }
            break;

        case SPT_EVENTO_MAQUINA_ACTIVA:
            if (sesion_actual.estado == SPT_ESTADO_SESION_ACTIVA)
            {
                // Actualizar actividad y estado
                sesion_actual.maquina_en_juego = true;
                sesion_actual.millis_ultima_actividad = millis();
                resultado.exito = true;
                resultado.mensaje = "Máquina activa - Actividad registrada";
            }
            break;

        case SPT_EVENTO_TARJETA_INSERTADA:
            if (sesion_actual.estado == SPT_ESTADO_SESION_ACTIVA)
            {
                resultado = cerrarSesion(SPT_CIERRE_INSERCION_TARJETA, rtc);
            }
            break;

        case SPT_EVENTO_PERDIDA_COMUNICACION:
            if (sesion_actual.estado == SPT_ESTADO_SESION_ACTIVA)
            {
                resultado = cerrarSesion(SPT_CIERRE_PERDIDA_COMUNICACION, rtc);
            }
            break;

        default:
            resultado.mensaje = "Evento no reconocido";
            break;
    }

    return resultado;
}

/* ====================================================================================
   MÉTODOS DE ACTUALIZACIÓN DE ESTADO
   ==================================================================================== */

void SesionPlayerTracking::actualizarMaquinaEnJuego(bool en_juego)
{
    sesion_actual.maquina_en_juego = en_juego;

    if (en_juego && sesion_actual.estado == SPT_ESTADO_SESION_ACTIVA)
    {
        sesion_actual.millis_ultima_actividad = millis();
    }
}

void SesionPlayerTracking::actualizarPremioPendiente(bool premio_pendiente)
{
    // Detectar flanco de bajada (premio finalizado)
    if (premio_pendiente_flag_anterior && !premio_pendiente)
    {
        #ifdef DEBUG_SESIONES_A
        Serial.println("[SPT] 💰 Premio finalizado detectado");
        #endif
    }

    sesion_actual.hay_premio_pendiente = premio_pendiente;
    premio_pendiente_flag_anterior = premio_pendiente;
}

void SesionPlayerTracking::actualizarComunicacionMET(bool comunicacion_ok)
{
    sesion_actual.comunicacion_met_ok = comunicacion_ok;

    // Detectar pérdida de comunicación
    if (comunicacion_ok_anterior && !comunicacion_ok)
    {
        if (sesion_actual.estado == SPT_ESTADO_SESION_ACTIVA)
        {
            #ifdef DEBUG_SESIONES_A
            Serial.println("[SPT] 🔌 Pérdida de comunicación detectada");
            #endif
        }
    }

    comunicacion_ok_anterior = comunicacion_ok;
}

void SesionPlayerTracking::actualizarCreditos(int creditos)
{
    sesion_actual.creditos_actuales = creditos;
}

void SesionPlayerTracking::actualizarTarjetaInsertada(bool tarjeta_insertada)
{
    tarjeta_insertada_anterior = tarjeta_insertada;
}

void SesionPlayerTracking::registrarCierreSesionConTarjeta()
{
    sesion_actual.millis_ultimo_cierre_tarjeta = millis();
    sesion_actual.cierre_por_tarjeta_detectado = true;

    #ifdef DEBUG_SESIONES_A
    Serial.println("[SPT] 🔒 Ventana de seguridad activada por cierre de sesión con tarjeta");
    Serial.printf("[SPT] ⏱️ Ventana de %lu ms activada\n", SPT_MARGEN_SEGURIDAD);
    #endif
}

/* ====================================================================================
   MÉTODOS DE VERIFICACIÓN
   ==================================================================================== */

bool SesionPlayerTracking::verificarCorteHorario(ESP32Time& rtc)
{
    if (sesion_actual.estado != SPT_ESTADO_SESION_ACTIVA)
    {
        return false;
    }

    time_t epoch_actual = rtc.getEpoch();
    uint8_t hora_actual = rtc.getHour(true);

    // Verificar que haya una hora de inicio válida
    if (sesion_actual.hora_inicio == 255)
    {
        return false;
    }

    // Calcular tiempo transcurrido en segundos
    time_t tiempo_transcurrido = epoch_actual - sesion_actual.epoch_inicio;

    // Verificar si pasó al menos 1 hora (con margen de seguridad)
    if (tiempo_transcurrido >= (SPT_CORTE_HORARIO_SEGUNDOS - SPT_MARGEN_SEGURIDAD/1000))
    {
        return true;
    }

    // Verificación adicional por cambio de hora (para robustez)
    if (hora_actual != sesion_actual.hora_inicio && tiempo_transcurrido > 0)
    {
        // Verificar que no sea un cambio de día (23 -> 00)
        // Solo si el tiempo transcurrido es razonable para un cambio de hora
        if (tiempo_transcurrido < 86400) // Menos de 1 día
        {
            return true;
        }
    }

    return false;
}

bool SesionPlayerTracking::verificarInactividad()
{
    if (sesion_actual.estado != SPT_ESTADO_SESION_ACTIVA &&
        sesion_actual.estado != SPT_ESTADO_PENDIENTE_CIERRE)
    {
        return false;
    }

    unsigned long millis_actual = millis();
    unsigned long tiempo_inactividad = millis_actual - sesion_actual.millis_ultima_actividad;

    return (tiempo_inactividad >= timeout_inactividad_ms);
}

bool SesionPlayerTracking::verificarContadoresEnCero()
{
    // Verificar solo los 4 contadores principales específicos
    if (contadores.Get_Contadores_Int(Total_Drop) != 0)
        return false;
    if (contadores.Get_Contadores_Int(Total_Cancel_Credit) != 0)
        return false;
    if (contadores.Get_Contadores_Int(Bill_Amount) != 0)
        return false;
    if (contadores.Get_Contadores_Int(Cancel_Credit_Hand_Pay) != 0)
        return false;

    return true;  // Todos los contadores verificados están en cero
}

/* ====================================================================================
   MÉTODOS DE UTILIDAD
   ==================================================================================== */

void SesionPlayerTracking::reiniciarSesionesAcumuladas()
{
    sesion_actual.sesiones_acumuladas = 0;
}

unsigned long SesionPlayerTracking::getTiempoTranscurrido() const
{
    if (sesion_actual.estado == SPT_ESTADO_INACTIVO && sesion_actual.epoch_cierre > 0)
    {
        // Sesión cerrada: retornar duración total
        return (sesion_actual.epoch_cierre - sesion_actual.epoch_inicio) * 1000;
    }
    else if (sesion_actual.estado != SPT_ESTADO_INACTIVO)
    {
        // Sesión activa: retornar tiempo hasta ahora
        return millis() - sesion_actual.millis_inicio;
    }
    return 0;
}

unsigned long SesionPlayerTracking::getTiempoInactividad() const
{
    if (sesion_actual.estado == SPT_ESTADO_INACTIVO)
    {
        return 0;
    }
    return millis() - sesion_actual.millis_ultima_actividad;
}

/* ====================================================================================
   MÉTODOS ESTÁTICOS - Conversión a String
   ==================================================================================== */

String SesionPlayerTracking::estadoToString(SPT_EstadoSesion estado)
{
    switch (estado)
    {
        case SPT_ESTADO_INACTIVO:           return "INACTIVO";
        case SPT_ESTADO_SESION_ACTIVA:     return "ACTIVA";
        case SPT_ESTADO_PENDIENTE_CIERRE:  return "PENDIENTE_CIERRE";
        case SPT_ESTADO_BLOQUEADO:          return "BLOQUEADO";
        case SPT_ESTADO_ERROR:             return "ERROR";
        default:                            return "DESCONOCIDO";
    }
}

String SesionPlayerTracking::eventoToString(SPT_Evento evento)
{
    switch (evento)
    {
        case SPT_EVENTO_NINGUNO:              return "NINGUNO";
        case SPT_EVENTO_BILLETE_INSERTADO:    return "BILLETE_INSERTADO";
        case SPT_EVENTO_MAQUINA_ACTIVA:       return "MAQUINA_ACTIVA";
        case SPT_EVENTO_TARJETA_INSERTADA:    return "TARJETA_INSERTADA";
        case SPT_EVENTO_TARJETA_RETIRADA:     return "TARJETA_RETIRADA";
        case SPT_EVENTO_INACTIVIDAD:          return "INACTIVIDAD";
        case SPT_EVENTO_CORTE_HORARIO:        return "CORTE_HORARIO";
        case SPT_EVENTO_PREMIO_PENDIENTE:     return "PREMIO_PENDIENTE";
        case SPT_EVENTO_PREMIO_FINALIZADO:    return "PREMIO_FINALIZADO";
        case SPT_EVENTO_PERDIDA_COMUNICACION: return "PERDIDA_COMUNICACION";
        case SPT_EVENTO_TIMEOUT_VERIFICACION: return "TIMEOUT_VERIFICACION";
        default:                              return "DESCONOCIDO";
    }
}

String SesionPlayerTracking::causaToString(SPT_CausaCierre causa)
{
    switch (causa)
    {
        case SPT_CIERRE_NINGUNA:               return "NINGUNA";
        case SPT_CIERRE_INSERCION_TARJETA:     return "INSERCION_TARJETA";
        case SPT_CIERRE_INACTIVIDAD:           return "INACTIVIDAD";
        case SPT_CIERRE_CORTE_HORARIO:         return "CORTE_HORARIO";
        case SPT_CIERRE_PERDIDA_COMUNICACION:  return "PERDIDA_COMUNICACION";
        case SPT_CIERRE_CREDITOS_INSUFICIENTES: return "CREDITOS_INSUFICIENTES";
        case SPT_CIERRE_MANUAL:                 return "MANUAL";
        case SPT_CIERRE_ERROR:                  return "ERROR";
        default:                                return "DESCONOCIDA";
    }
}
