#include <Arduino.h>
#include "Bootloader.h"
#include "Internet.h"
#include <stdio.h>
#include <string.h>
#include "RS232.h"
#include "Config_Perifericos.h"

char buffer_envio[258] = {};

unsigned long tiempo_inicial, tiempo_final = 0;

void setup()
{
  Init_Config();
  //  Init_Tasks();
  pinMode(2, OUTPUT);
  bzero(buffer_envio, 258); // Pone el buffer en 0
}

void loop()
{
  Run_Bootloader();

  tiempo_inicial = millis();

  unsigned char val1, val2;

  if ((tiempo_inicial - tiempo_final) >= 5000)
  {
    tiempo_final = tiempo_inicial;
    string res = contadores.Get_Contadores_String(Total_Cancel_Credit);
    Serial.println("--------------------------------------------------------------------");
    //    Serial.println(res);
    int32_t Aux1;
    int Com = 5;
    Aux1 = Com;
    Aux1 = (Aux1 & 0x000000FF);
    buffer_envio[0] = Aux1;
    //------------------------------------------------------------------------------
    // Guarda el segundo byte
    Aux1 = Com;
    Aux1 = ((Aux1 & 0x0000FF00) >> 8);
    buffer_envio[1] = Aux1;
    //------------------------------------------------------------------------------
    // Guarda el tercer byte
    Aux1 = Com;
    Aux1 = ((Aux1 & 0x00FF0000) >> 16);
    buffer_envio[2] = Aux1;
    //------------------------------------------------------------------------------
    // Guarda el cuarto byte
    Aux1 = Com;
    Aux1 = ((Aux1 & 0xFF000000) >> 24);
    buffer_envio[3] = Aux1;

    // buffer_envio[0] = ({5});
    // buffer_envio[1] = {0};
    // buffer_envio[2] = {0};
    // buffer_envio[3] = {0};
    buffer_envio[4] = {'B'};
    buffer_envio[5] = {'C'};
    buffer_envio[6] = {'|'};

    Encripta_Mensaje_Servidor();
    Calcula_CRC_Wifi();
    int len = sizeof(buffer_envio);
    Serial.println(len);
    Serial.println(buffer_envio);
    int length_ = client.write(buffer_envio, len);
    Serial.println(length_);
    Serial.println("--------------------------------------------------------------------");
    bzero(buffer_envio, 258); // Pone el buffer en 0
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
