#include <Arduino.h>

#include "Bootloader.h"

/* Definir las clases que haran uso de los metodos */
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

void setup()
{
  Init_Config();

  pinMode(2, OUTPUT);
}

void loop()
{
  Run_Bootloader();

  tiempo_inicial = millis();

  // unsigned char val1, val2;

  if ((tiempo_inicial - tiempo_final) >= 10000)
  {
    tiempo_final = tiempo_inicial;

    //    Transmite_Eco_Broadcast();
    // char res[8] = {};
    // bzero(res, 8);
    // memcpy(res, contadores.Get_Contadores_Char(Total_Cancel_Credit), sizeof(res) / sizeof(res[0]));
    // Serial.println(contadores.Get_Contadores_Char(Total_Cancel_Credit));
    // Serial.println("Entero...");
    // Serial.println(contadores.Get_Contadores_Int(Total_Cancel_Credit));
    // Serial.println("Set buffer...");
    // //    Buffer.Set_buffer_contadores(contadores);
    // Serial.println("----------------------------------------");

    if (bandera2 == 0)
    {
//      Transmite_Contadores();
      bandera2 = 1;
    }
    else
    {
//      Transmite_Confirmacion('A', '3'); // Transmite ACK a Server
      bandera2 = 0;
    }
  }
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
