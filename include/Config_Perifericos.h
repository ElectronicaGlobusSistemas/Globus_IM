#include <Arduino.h>
#include "Memory_SD.h"
#include "RTC.h"

#define Clock_frequency 240   
#define MCU_Status      2
#define WIFI_Status     15


void Indicadores(void);
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
  Indicadores();                  // Reset Indicadores LED'S LOW.
  //---------------------------------------------------------------

  //-----------------> Config Comunicación Maquina <---------------
  Init_UART2(); // Inicializa Comunicación Maquina Puerto #2
  Serial.begin(19200); // Inicializa Monitor Serial Debug
  //---------------------------------------------------------------

  //--------------------> Config  WIFI <---------------------------
  // CONNECT_WIFI(); // Conecta  a red Wifi con ssdi y password

  Sincroniza_Hora(0,41,13,2,6,2022);
  //---------------------------------------------------------------


  //-------------------->  Módulos <-------------------------------
  Init_SD(); // Inicializa Memoria SD.
  //---------------------------------------------------------------

  //-------------------->  Update  <-------------------------------
  //  Bootloader(); // Config Bootloader metodo  spiffs
  //---------------------------------------------------------------

  
}

void Indicadores(void)
{
  digitalWrite(SD_ChipSelect,LOW);
  digitalWrite(SD_Status,LOW);
  digitalWrite(MCU_Status,LOW);
}