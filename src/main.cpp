#include <Arduino.h>
//#include "Bootloader.h"
//#include "Internet.h"
#include "RS232.h"
//#include <stdio.h>
//#include <string.h>
#include "Config_Perifericos.h"


void setup() {
  Init_Config();
}




void loop() {
  
    tiempo_actual=millis();
    tiempo_actual2=millis();


  

      if(tiempo_actual-tiempo_previo>=100){
        if(bandera==1){
        tiempo_previo=tiempo_actual;
        sendDataa(dat,sizeof(dat)); // transmite sincronización
        //Transmite_Poll(0x00); // general poll
        bandera=0;
        }
        else{
            
            Transmite_Poll(0x00);
            bandera=1;
        }
      }
}




