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
unsigned long tiempo_inicial, tiempo_final = 0;

void setup()
{
  Init_Config();
  pinMode(2, OUTPUT);
}

void loop()
{
  tiempo_inicial = millis();

  if ((tiempo_inicial - tiempo_final) >= 5000)
  {
    tiempo_final = tiempo_inicial;
    int res = contadores.Get_Contadores(Total_Cancel_Credit);
    Serial.print("Contador Total cancel credit -> ");
    Serial.println(res);
  }
}
