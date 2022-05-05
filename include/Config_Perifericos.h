#include <Arduino.h>

#define RXD1            33 
#define TXD1            32
#define Clock_frequency 240 


void Init_Config(void){
   
    setCpuFrequencyMhz(Clock_frequency); // Maxima Frecuencia.
    //Serial2.begin(19200,SERIAL_8N1,TXD2,RXD2); // UART2 por defecto
    //Serial1.begin(19200,SERIAL_8N1,TXD1,RXD1); // UART1 por Software
    Init_UART2();
    
}







