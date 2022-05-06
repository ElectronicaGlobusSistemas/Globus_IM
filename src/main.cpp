#include <Arduino.h>
#include "Bootloader.h"
#include "Internet.h"
#include <stdio.h>
#include <string.h>
#include "Config_Perifericos.h"

String buffer;

void setup()
{
  Init_Config();
  Init_Tasks();
  pinMode(2, OUTPUT);
}

void loop()
{
  Run_Bootloader();
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

