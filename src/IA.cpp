// ==========================================
// EDGE AI: MODELO RAGE QUIT V2 (PRODUCCIÓN)
// ==========================================
#include <cmath>
#include <Arduino.h>
#include "IA.h"
// --- Estructura de datos para la IA ---
#include <WiFi.h>
#include <PubSubClient.h>

// --- Variables de Estado de la IA ---
float initial_credit_session = 0.0;
unsigned long prev_coin_out = 0;
unsigned long session_start_ms = 0;
unsigned long last_win_time_ms = 0;

#define WIN_HISTORY_SIZE 10
float win_history[WIN_HISTORY_SIZE] = {0};
int win_index = 0;

bool session_active = false;
bool alert_triggered = false; 


// ==========================================
// FUNCIÓN 1: SE LLAMA AL INICIAR EL JUEGO
// ==========================================
// Pásale el current_credit en el momento que el jugador 
// inserta dinero o inicia la primera jugada.
void IA_IniciarSesion(float current_credit) {
    if (session_active) return; // Evitar reinicios duplicados

    initial_credit_session = current_credit;
    session_start_ms = millis();
    last_win_time_ms = millis();
    session_active = true;
    alert_triggered = false;
    win_index = 0;
    
    // Reiniciamos historial
    for(int i=0; i<WIN_HISTORY_SIZE; i++) win_history[i] = 0.0;
    
    Serial.printf("[IA] Sesión iniciada. Crédito inicial: %.2f\n", current_credit);
}

// ==========================================
// FUNCIÓN 2: SE LLAMA AL TERMINAR EL JUEGO
// ==========================================
// Se ejecuta cuando el jugador se retira, se queda en 0, 
// o la máquina entra en modo idle.
void IA_TerminarSesion() {
    if (!session_active) return;

    session_active = false;
    prev_coin_out = 0; // Reseteamos el coin out para el próximo jugador
    Serial.println("[IA] Sesión terminada. Modelo en reposo.");
}


// ==========================================
// FUNCIÓN 3: SE LLAMA CADA VEZ QUE HAY UN TIRO
// ==========================================
// Pásale las variables REALES que ya tienes de tu lectura SAS.
// current_credit: El contador de créditos actuales.
// coin_out: El contador que suma cuando gana.
void IA_ActualizarDatos(float current_credit, unsigned long coin_out) {
    
    // Si no hay sesión activa, no hacemos nada
    if (!session_active || alert_triggered) return;

    // 1. Detectar PREMIO usando tu regla del Coin Out
    // Ojo: Usamos 'long' por si el contador da la vuelta y es menor que el previo
    long delta_coin_out = (long)coin_out - (long)prev_coin_out;
    
    if (delta_coin_out > 0) {
        float win_amount = (float)delta_coin_out;
        
        // Guardar en array circular
        win_history[win_index] = win_amount;
        win_index = (win_index + 1) % WIN_HISTORY_SIZE;
        
        // Actualizar tiempo del último premio
        last_win_time_ms = millis();
        
        // Serial.printf("[IA DEBUG] Premio detectado: %.2f\n", win_amount); // Descomenta para debuggear
    }
    
    // 2. Evaluar si el jugador se quedó sin dinero (Crédito = 0)
    if (current_credit == 0.0) {
        Serial.println("[IA] Crédito en 0. Session terminada por pérdida total.");
        IA_TerminarSesion();
        return;
    }

    // 3. Actualizar el previo para el próximo tiro
    prev_coin_out = coin_out;

    // 4. Calcular tiempo de juego (Damos 30 segundos de gracia al inicio para estabilizar)
    float played_minutes = (millis() - session_start_ms) / 60000.0;
    if (played_minutes < 0.5) return; // Menos de 30 seg, la IA aún no tiene contexto

    // 5. EXTRAER FEATURES (Ingeniería de variables)
    float loss_rate = (initial_credit_session - current_credit) / played_minutes;
    float variance = calculate_variance();
    float time_no_win = (millis() - last_win_time_ms) / 60000.0;

    // 6. EJECUTAR INFERENCIA
    GameFeatures features = {loss_rate, variance, time_no_win};
    int prediction = predict_rage_quit(features);

    // 7. ACTUAR
    if (prediction == 1) {
        Serial.printf("🚨 [ALERTA!] RAGE QUIT -> Loss: %.2f | Var: %.2f | SinWin: %.2f\n", loss_rate, variance, time_no_win);
        
        // >>> AQUÍ VA TU PUBLICACIÓN MQTT <<<
        // mqttClient.publish("casino/floor/alerts", "{\"alert\":\"rage_quit\"}");
        
        alert_triggered = true; // Congela la IA para no hacer spam en esta sesión
    }
}


// ==========================================
// CÁLCULO DE VARIANZA (Feature Engineering)
// ==========================================
float calculate_variance() {
    float sum = 0.0;
    float mean = 0.0;
    
    for(int i=0; i<WIN_HISTORY_SIZE; i++) sum += win_history[i];
    mean = sum / WIN_HISTORY_SIZE;
    
    float variance = 0.0;
    for(int i=0; i<WIN_HISTORY_SIZE; i++) {
        variance += pow(win_history[i] - mean, 2);
    }
    
    return variance / WIN_HISTORY_SIZE;
}


// ==========================================
// EL CEREBRO DE LA IA (El Árbol de Decisión)
// ==========================================
int predict_rage_quit(GameFeatures f) {
    if (f.win_variance <= 8.33f) {
        if (f.time_since_last_win <= 1.99f) {
            return 0;
        } else {
            if (f.loss_rate <= 0.14f) {
                return 0;
            } else {
                return 1; // RAGE QUIT
            }
        }
    } else {
        if (f.win_variance <= 10.06f) {
            if (f.time_since_last_win <= 2.37f) {
                return 0;
            } else {
                if (f.loss_rate <= 3.16f) {
                    if (f.time_since_last_win <= 5.04f) {
                        return 0;
                    } else {
                        return 1; // RAGE QUIT
                    }
                } else {
                    return 1; // RAGE QUIT
                }
            }
        } else {
            if (f.time_since_last_win <= 6.37f) {
                return 0;
            } else {
                return 1; // RAGE QUIT
            }
        }
    }
}




// const char* ssid = "TU_WIFI";
// const char* password = "TU_CLAVE";

// const char* mqtt_server = "broker.emqx.io";

// WiFiClient espClient;
// PubSubClient client(espClient);

// void conectarMQTT()
// {
//     while (!client.connected())
//     {
//         Serial.println("Conectando MQTT...");

//         if (client.connect("ESP32_TEST"))
//         {
//             Serial.println("MQTT conectado");
//         }
//         else
//         {
//             delay(2000);
//         }
//     }
// }

// void Init()
// {
   
//     client.setServer(mqtt_server, 1883);

//     conectarMQTT();

//     client.publish(
//         "prueba/hola",
//         "Hola Mundo"
//     );

//     Serial.println("Mensaje enviado");
// }


// void Lopper()
// {
//     client.loop();
// }

