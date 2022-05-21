#include <Arduino.h>

#define RXD1 33
#define TXD1 32
#define Clock_frequency 240

void Inicializa_SPI();

void Init_Config(void)
{
  Init_UART2();
  Inicializa_SPI();
  Serial.begin(19200);                 // Inicializa Monitor Serial
  setCpuFrequencyMhz(Clock_frequency); // Frecuencia de Nucleos 1 y 0.
                                       // CONNECT_WIFI(); // Conecta  a red Wifi con ssdi y password

  //  Bootloader(); // Config Bootloader metodo  spiffs
}

void Inicializa_SPI()
{

#define HSPI_MISO 19
#define HSPI_MOSI 23
#define HSPI_SCLK 18
#define HSPI_CS 5

  //  SPIClass *hspi = NULL;
  //  hspi = new SPIClass(VSPI);
  //  hspi->begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_CS);
  pinMode(HSPI_CS, OUTPUT); // HSPI SS
  digitalWrite(HSPI_CS, HIGH);
}