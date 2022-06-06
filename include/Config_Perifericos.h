#include <Arduino.h>
#include "Memory_SD.h"

//-------------------> Parametros <-------------------------------
#define Clock_frequency 240
#define MCU_Status 2
#define WIFI_Status 15
//-----------------------------------------------------------------

TaskHandle_t Task1;
void loop2(void *parameter);
void Init_Tasks();
void Init_Indicadores_LED(void);

void Init_Config(void)
{
    //------------------------> Config MCU <-------------------------
    setCpuFrequencyMhz(Clock_frequency); // Frecuencia de Nucleos 1 y 0.
    //---------------------------------------------------------------

    //---------------------> Inicializa Indicadores <----------------
    pinMode(SD_ChipSelect, OUTPUT); // Selector de Esclavo SPI.
    pinMode(SD_Status, OUTPUT);     // SD Status Como Salida.
    pinMode(MCU_Status, OUTPUT);    // MCU_Status Como Salida.
    pinMode(WIFI_Status, OUTPUT);   // Wifi_Status como Salida.
    Init_Indicadores_LED();         // Reset Indicadores LED'S LOW.
    //---------------------------------------------------------------

    //-----------------> Config Comunicación Maquina <---------------
    Init_UART2();         // Inicializa Comunicación Maquina Puerto #2
    Serial.begin(115200); //  Inicializa Monitor Serial Debug
    //---------------------------------------------------------------

    //-------------------->  Módulos <-------------------------------
    Init_SD(); // Inicializa Memoria SD.
    //---------------------------------------------------------------

    //--------------------> Config  WIFI <---------------------------
    CONNECT_WIFI();
    CONNECT_SERVER_TCP();

    // Segundos, Minutos, Hora, Dia, Mes, Año
    RTC.setTime(0, 0, 0, 1, 1, 2022);

    // Archivo_Format= Hora Actualizada..
    Archivo_Format = "03062022";
    Create_ARCHIVE_Excel(Archivo_Format, Encabezado_Contadores);
    //---------------------------------------------------------------

    //--------------------> Rum Tareas <-----------------------------
    Init_Tasks();
    //---------------------------------------------------------------

    //-------------------->  Update  <-------------------------------
    Bootloader(); // Inicializa  Bootloader
    //---------------------------------------------------------------
}

void Init_Tasks(void)
{
    xTaskCreatePinnedToCore(
        loop2,    //  Funcion a implementar la tarea
        "Task_1", //  Nombre de la tarea
        10000,    //  Tamaño de stack en palabras (memoria)
        NULL,     //  Entrada de parametros
        1,        //  Prioridad de la tarea
        &Task1,   //  Manejador de la tarea
        0);       //  Core donde se ejecutara la tarea

    xTaskCreatePinnedToCore(
        UART_ISR_ROUTINE,     //  Funcion a implementar la tarea
        "UART_ISR_ROUTINE",   //  Nombre de la tarea
        2048,                 //  Tamaño de stack en palabras (memoria)
        NULL,                 //  Entrada de parametros
        configMAX_PRIORITIES, //  Prioridad de la tarea
        NULL,                 //  Manejador de la tarea
        1);                   //  Core donde se ejecutara la tarea

    xTaskCreatePinnedToCore(
        Encuestas_Maquina,        //  Funcion a implementar la tarea
        "Encuestas",              //  Nombre de la tarea
        2048,                     //  Tamaño de stack en palabras (memoria)
        NULL,                     //  Entrada de parametros
        configMAX_PRIORITIES - 5, //  Prioridad de la tarea
        NULL,                     //  Manejador de la tarea
        1);                       //  Core donde se ejecutara la tarea

    xTaskCreatePinnedToCore(
        Task_Verifica_Conexion_Wifi, //  Funcion a implementar la tarea
        "Verifica conewxion wifi",   //  Nombre de la tarea
        10000,                       //  Tamaño de stack en palabras (memoria)
        NULL,                        //  Entrada de parametros
        configMAX_PRIORITIES - 10,   //  Prioridad de la tarea
        NULL,                        //  Manejador de la tarea
        0);                          //  Core donde se ejecutara la tarea

    xTaskCreatePinnedToCore(
        Task_Verifica_Conexion_Servidor_TCP,
        "Verifica conexion server",
        5000,
        NULL,
        configMAX_PRIORITIES - 20,
        NULL,
        0); // Core donde se ejecutara la tarea

    xTaskCreatePinnedToCore(
        Task_Verifica_Mensajes_Servidor_TCP,
        "Verifica mensajes server",
        5000,
        NULL,
        configMAX_PRIORITIES,
        NULL,
        0); // Core donde se ejecutara la tarea
    xTaskCreatePinnedToCore(
        Task_Procesa_Comandos,
        "Procesa comandos server",
        5000,
        NULL,
        configMAX_PRIORITIES - 5,
        NULL,
        0); // Core donde se ejecutara la tarea
}

void Init_Indicadores_LED(void)
{
    digitalWrite(SD_ChipSelect, LOW);
    digitalWrite(SD_Status, LOW);
    digitalWrite(MCU_Status, LOW);
}