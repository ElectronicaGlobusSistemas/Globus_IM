#include <Arduino.h>
#include "Bootloader.h"
#include "Internet.h"
#include "RS232.h"
#include <stdio.h>
#include <string.h>
#include "Config_Perifericos.h"

void setup() {
  Init_Config();
  Serial.begin(115200);
}

void loop() {
  delay(1000);
}




