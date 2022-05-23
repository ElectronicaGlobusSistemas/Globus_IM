#include <Arduino.h>
#include "time.h"
#include <ESP32Time.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
//#include "Bootloader.h"
//#include "Internet.h"
#include "RS232.h"
#include <stdio.h>
//#include <string.h>
#include "Config_Perifericos.h"
#include "Memoria_SD.h"
#include <iostream>
#include <bitset>

using namespace std;
using std::bitset;
unsigned long tiempo_inicial, tiempo_final = 0;

void setup()
{
  Init_Config();
  pinMode(2, OUTPUT);
//  Init_SD();
}

void loop()
{
  // tiempo_inicial = millis();

  // if ((tiempo_inicial - tiempo_final) >= 5000)
  // {
  //   tiempo_final = tiempo_inicial;
  //   int res = contadores.Get_Contadores(Total_Cancel_Credit);
  //   Serial.print("Contador Total cancel credit -> ");
  //   Serial.println(res);
  // }
}
