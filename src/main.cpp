#include <Arduino.h>
#include "Bootloader.h"
#include "Internet.h"
#include "RS232.h"
#include <stdio.h>
#include <string.h>
#include "Config_Perifericos.h"

void setup() {
  Init_Config();
  pinMode(2, OUTPUT);
}

void loop() {
  Rum_Bootloader();
  digitalWrite(2, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);                       // wait for a second
  digitalWrite(2, LOW);    // turn the LED off by making the voltage LOW
  delay(500); 
}




