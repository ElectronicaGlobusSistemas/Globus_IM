#include <Arduino.h>

#define RXD1 33
#define TXD1 32
#define Clock_frequency 240

void Init_Config(void)
{
    Serial.begin(115200);
    setCpuFrequencyMhz(Clock_frequency); // Maxima Frecuencia.
    CONNECT_WIFI();
    Bootloader();
}
