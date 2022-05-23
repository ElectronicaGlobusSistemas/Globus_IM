#include <Arduino.h>

#define RXD1 33
#define TXD1 32
#define Clock_frequency 240

TaskHandle_t Task1;
void loop2(void *parameter);
void Init_Tasks();

void Init_Config(void)
{
    setCpuFrequencyMhz(Clock_frequency); // Maxima Frecuencia.
    Serial.begin(115200);
    Init_UART2();
    CONNECT_WIFI();
    CONNECT_SERVER_TCP();
    Init_Tasks();
    Bootloader();
}

void Init_Tasks()
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
        "WIFI-Connect",
        10000,
        NULL,
        configMAX_PRIORITIES - 10,
        NULL,
        0); // Core donde se ejecutara la tarea

    xTaskCreatePinnedToCore(
        Task_Verifica_Conexion_Servidor_TCP,
        "Server-Connect",
        5000,
        NULL,
        configMAX_PRIORITIES - 20,
        NULL,
        0); // Core donde se ejecutara la tarea

    xTaskCreatePinnedToCore(
        Task_Verifica_Mensajes_Servidor_TCP,
        "Server-messages",
        5000,
        NULL,
        configMAX_PRIORITIES - 5,
        NULL,
        0); // Core donde se ejecutara la tarea
}
