#include <Arduino.h>
#include "Bootloader.h"
#include "Internet.h"
#include <stdio.h>
#include <string.h>
#include "RS232.h"
#include "Config_Perifericos.h"

unsigned long tiempo_inicial, tiempo_final = 0;

void setup()
{
  Init_Config();
  //  Init_Tasks();
  pinMode(2, OUTPUT);
}

void loop()
{
  Run_Bootloader();

  tiempo_inicial = millis();

  char buffer_envio[256] = {'H', 'o', 'l', 'a'};
  unsigned char val1, val2;

  if ((tiempo_inicial - tiempo_final) >= 5000)
  {
    tiempo_final = tiempo_inicial;
    string res = contadores.Get_Contadores_String(Total_Cancel_Credit);
    Serial.print("Contador Total cancel credit -> ");
//    Serial.println(res);
    

    client.write(res.c_str());
  }
}

void loop2(void *parameter)
{
  for (;;)
  {
    digitalWrite(2, HIGH); // turn the LED on (HIGH is the voltage level)
    delay(500);            // wait for a second
    digitalWrite(2, LOW);  // turn the LED off by making the voltage LOW
    delay(500);
  }
  vTaskDelay(10);
}
