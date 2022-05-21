#include <Arduino.h>

#define RXD1 33
#define TXD1 32
#define Clock_frequency 240

void Init_Config(void)
{
  Init_UART2();

  Serial.begin(19200);                 // Inicializa Monitor Serial
  setCpuFrequencyMhz(Clock_frequency); // Frecuencia de Nucleos 1 y 0.
                                       // CONNECT_WIFI(); // Conecta  a red Wifi con ssdi y password

  //  Bootloader(); // Config Bootloader metodo  spiffs
}