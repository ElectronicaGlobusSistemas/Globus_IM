#include <Arduino.h>

#define RXD1 33
#define TXD1 32
#define Clock_frequency 240

TaskHandle_t Task1;
void loop2(void *parameter);

void Init_Config(void)
{
    Serial.begin(115200);
    setCpuFrequencyMhz(Clock_frequency); // Maxima Frecuencia.
    CONNECT_WIFI();
    CONNECT_SERVER_TCP();
    Bootloader();
}

void Init_Tasks(void)
{
    xTaskCreatePinnedToCore(
        loop2,
        "Task_1",
        10000,
        NULL,
        1,
        &Task1,
        0);

    xTaskCreatePinnedToCore(
        Task_Verifica_Conexion_Wifi,
        "Tarea para verificar conexion WIFI",
        5000,
        NULL,
        2,
        NULL,
        0);

    xTaskCreatePinnedToCore(
        Task_Verifica_Conexion_Servidor_TCP,
        "Tarea para verificar conexion SERVER",
        5000,
        NULL,
        2,
        NULL,
        0);

    xTaskCreatePinnedToCore(
        Task_Verifica_Mensajes_Servidor_TCP,
        "Tarea para verificar mensajes SERVER",
        5000,
        NULL,
        1,
        NULL,
        0);
}
