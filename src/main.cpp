#include <Arduino.h>
#include "Bootloader.h"
#include "Internet.h"
#include "RS232.h"
#include <stdio.h>
#include <string.h>
#include "Config_Perifericos.h"

void setup() {
  Init_Config();
}

void loop() {
  Rum_Bootloader();
  delay(1);
}




