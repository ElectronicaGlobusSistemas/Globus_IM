#include <Arduino.h>
//#include "Bootloader.h"
//#include "Internet.h"
#include "RS232.h"
#include <stdio.h>
//#include <string.h>
#include "Config_Perifericos.h"
#include <iostream>
#include <bitset>

using namespace std;
using std::bitset;

int bandera, conta_poll = 0;

void setup()
{
  Init_Config();
  pinMode(2, OUTPUT);
}

void loop()
{
  tiempo_actual = millis();
  tiempo_actual2 = millis();

  if (tiempo_actual - tiempo_previo >= 200)
  {
    conta_poll++;
    tiempo_previo = tiempo_actual;
    if (bandera == 0 && conta_poll < 50)
    {
      const char dato_sync[1] = {128};
      sendDataa(dato_sync, sizeof(dato_sync)); // transmite sincronización
      bandera = 1;
    }
    else if (bandera == 1 && conta_poll < 50)
    {
      const char dato_poll_1[1] = {1};
      const char dato_poll_2[1] = {129};

      sendDataa(dato_poll_1, sizeof(dato_poll_1));
      delay(3);
      sendDataa(dato_poll_2, sizeof(dato_poll_2));
//      Transmite_Poll(0x00); // transmite general poll
      bandera = 0;
    }
    else if (conta_poll >= 50)
    {
      conta_poll = 0;
      digitalWrite(2, HIGH);

      // TX>= 01 2F 03 00 00 2F 7E F2

      const char dato_1[1] = {0x01};
      const char dato_2[1] = {0x15};
      const char dato_3[1] = {0x00};
      const char dato_4[1] = {0x00};
      const char dato_5[1] = {0xFF};
      const char dato_6[1] = {0xE0};
      const char dato_7[1] = {0x7E};
      const char dato_8[1] = {0xF2};
      //      int len = sizeof(dato_1);
      // for (int i = 0; i < len; i++)
      // {
      //   Serial.print(dato_1[i], HEX);
      // }
      Serial.print(" - ");
      
//      delay(100);
      sendDataa(dato_1, sizeof(dato_1));
      delay(3);
      uart_set_parity(UART_NUM_2, UART_PARITY_ODD);
      sendDataa(dato_2, sizeof(dato_2));
      delay(3);
      // sendDataa(dato_3, sizeof(dato_3));
      // delay(3);
      // sendDataa(dato_4, sizeof(dato_4));
      // delay(3);
      // sendDataa(dato_5, sizeof(dato_5));
      // delay(3);
      // sendDataa(dato_6, sizeof(dato_6));
      // delay(3);
      // sendDataa(dato_7, sizeof(dato_7));
      // delay(3);
      // sendDataa(dato_8, sizeof(dato_8));
      delay(100);
      digitalWrite(2, LOW);
      uart_set_parity(UART_NUM_2, UART_PARITY_DISABLE);
    }
  }
}
