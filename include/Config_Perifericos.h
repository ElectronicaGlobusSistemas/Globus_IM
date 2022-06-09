#include <Arduino.h>
#include "Memory_SD.h"
#include "RTC.h"

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
    // vTaskSuspendAll(); // Suspende Todas Las Tareas.
    RTC.setTime(0, 0, 0, 1, 1, 2022);
    //-------------------->  Módulos <-------------------------------
    Init_SD(); // Inicializa Memoria SD.
    Archivo_Format = "17062022.csv";
    Create_ARCHIVE_Excel(Archivo_Format, Encabezado_Contadores);
    //---------------------------------------------------------------

    //-----------------> Config Comunicación Maquina <---------------
    Init_UART2();         // Inicializa Comunicación Maquina Puerto #2
    Serial.begin(115200); //  Inicializa Monitor Serial Debug
    //---------------------------------------------------------------

    init_Comunicaciones();
    //--------------------> Config  WIFI <---------------------------
    CONNECT_WIFI();
    CONNECT_SERVER_TCP();
    Init_Wifi();

    //---------------------------------------------------------------

    //--------------------> Run Tareas <-----------------------------
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
}

void Init_Indicadores_LED(void)
{
    digitalWrite(SD_ChipSelect, LOW);
    digitalWrite(SD_Status, LOW);
    digitalWrite(MCU_Status, LOW);
    digitalWrite(WIFI_Status, LOW);
}