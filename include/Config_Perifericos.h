#include <Arduino.h>

#define RXD1 33
#define TXD1 32
#define Clock_frequency 240

TaskHandle_t Task1;
void loop2(void *parameter);

void Init_Config(void)
{
    Init_UART2();
    Serial.begin(115200);
    setCpuFrequencyMhz(Clock_frequency); // Maxima Frecuencia.
    CONNECT_WIFI();
    CONNECT_SERVER_TCP();
    Bootloader();
}

void Init_Tasks(void)
{
    xTaskCreatePinnedToCore(
        loop2,    // Funcion a implementar la tarea
        "Task_1", // Nombre de la tarea
        10000,    // Tamaño de stack en palabras (memoria)
        NULL,     // Entrada de parametros
        1,        // Prioridad de la tarea
        &Task1,   // Manejador de la tarea
        0);       // Core donde se ejecutara la tarea

    xTaskCreatePinnedToCore(
        UART_ISR_ROUTINE,
        "UART_ISR_ROUTINE", // Rutina de interrupcion por entrada de mensajes en UART
        2048,
        NULL,
        configMAX_PRIORITIES, // Máx Priority principal
        NULL,
        1); // Core donde se ejecutara la tarea

    xTaskCreatePinnedToCore(
        Encuestas_Maquina,
        "Encuestas",
        2048,
        NULL,
        configMAX_PRIORITIES - 5,
        NULL,
        1); // Core donde se ejecutara la tarea

    xTaskCreatePinnedToCore(
        Task_Verifica_Conexion_Wifi,
        "Tarea para verificar conexion WIFI",
        5000,
        NULL,
        configMAX_PRIORITIES - 20,
        NULL,
        0); // Core donde se ejecutara la tarea

    xTaskCreatePinnedToCore(
        Task_Verifica_Conexion_Servidor_TCP,
        "Tarea para verificar conexion SERVER",
        5000,
        NULL,
        configMAX_PRIORITIES - 20,
        NULL,
        0); // Core donde se ejecutara la tarea

    xTaskCreatePinnedToCore(
        Task_Verifica_Mensajes_Servidor_TCP,
        "Tarea para verificar mensajes SERVER",
        5000,
        NULL,
        configMAX_PRIORITIES - 5,
        NULL,
        0); // Core donde se ejecutara la tarea
}
