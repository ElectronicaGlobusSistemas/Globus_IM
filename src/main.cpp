#include <Arduino.h>

#include "Bootloader.h"

/* Definir las clases que haran uso de los metodos */
#include "ESP32Time.h"
#include "time.h"
ESP32Time RTC; // Objeto contiene hora(3600 ssegundots) y fecha

#include "Buffers.h"
Buffers Buffer; // Objeto de buffer de confirmacion Servidor

//#include "Contadores.h"
Contadores_SAS contadores; // Objeto contiene contadores maquina

#include "Eventos.h"
Eventos_SAS eventos; // Objeto contiene eventos maquina

/* Definir los metodos que haran uso de las clases */
#include "RS232.h"
#include "Comunicaciones.h"

#include <stdio.h>
#include <string.h>
#include "Config_Perifericos.h"
unsigned long tiempo_inicial, tiempo_final = 0;
int bandera2 = 0;

/* Variables globales */
bool flag_dato_valido_recibido = false;
bool flag_dato_no_valido_recibido = false;
bool flag_serie_trama_contadores = false;

void setup()
{
  Init_Config();

  pinMode(2, OUTPUT);

  
}

void loop()
{
  

}

void loop2(void *parameter)
{
  vTaskSuspend(Task1);
  for (;;)
  {
    digitalWrite(2, HIGH); // turn the LED on (HIGH is the voltage level)
    delay(100);            // wait for a second
    digitalWrite(2, LOW);  // turn the LED off by making the voltage LOW
    delay(100);
    
  }
  vTaskDelay(10);
}
