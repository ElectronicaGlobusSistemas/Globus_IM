#include <Arduino.h>
#include "Bootloader.h"

/* Definir las clases que haran uso de los metodos */
#include "Preferences.h"
Preferences NVS;

#include "Configuracion.h"
Configuracion_ESP32 Configuracion;

#include "ESP32Time.h"
#include "time.h"
ESP32Time RTC; // Objeto contiene hora y fecha

#include "Buffers.h"
Buffers Buffer; // Objeto de buffer de mensajes servidor
Contadores_SAS contadores; // Objeto contiene contadores maquina

#include "Clase_Variables_Globales.h"
Variables_Globales Variables_globales; // Objeto contiene Variables Globales

Eventos_SAS eventos; // Objeto contiene eventos maquina
/* Definir los metodos que haran uso de las clases */




#include "RS232.h"
#include "Comunicaciones.h"

#include <stdio.h>
#include <string.h>
#include "Config_Perifericos.h"
unsigned long tiempo_inicial, tiempo_final = 0;
int bandera2 = 0;


void setup()
{
  Variables_globales.Init_Variables_Globales();
  Init_Config(); // Config Perifericos
}

void loop()
{
}

