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


void setup()
{
  Init_Config();
  pinMode(2, OUTPUT);
}

void loop()
{
  
}
