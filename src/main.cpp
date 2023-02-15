#include <Arduino.h>
#include "Errores.h"
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
Buffers Buffer;            // Objeto de buffer de mensajes servidor
Contadores_SAS contadores; // Objeto contiene contadores maquina

#include "Buffer_Cashless.h"
Buffer_RX_AFT Buffer_Cashless;
//#include "Clase_Variables_Globales.h"
Variables_Globales Variables_globales; // Objeto contiene Variables Globales

Eventos_SAS eventos; // Objeto contiene eventos maquina
#include "Tabla_Eventos.h"
Tabla_Eventos Tabla_Evento;
/* Definir los metodos que haran uso de las clases */

#include "RS232.h"
#include "Comunicaciones.h"

#include <stdio.h>
#include <string.h>
#include "Config_Perifericos.h"

/*--------------------------------------->Debug Comunicación Maquina <------------------------------*/
#define Debug_Comunicacion_MQ
//#define Debug_Memoria_SD
/*--------------------------------------------------------------------------------------------------*/

unsigned long tiempo_inicial, tiempo_final = 0;
int bandera2 = 0;

/*Variables Para Task Comunicación Maquina*/
unsigned long Bandera_RS232=0;
unsigned long Bandera_RS232_F=0;

unsigned long Msg_RS232=0;
unsigned long Msg_RS232_F=0;
unsigned long Timeout_Msg=8000;

unsigned long Timeout_RS232=8000;
unsigned long Excepcion=0;
bool Fallo_Comunicacion=false;
void Check_Comunicacion_Maq(void);
bool Ftp_Out=false;

void setup()
{
  Variables_globales.Init_Variables_Globales();
  Tabla_Evento.Init_Tabla_Eventos();
  Init_Config(); // Config Perifericos
}

void loop()
{
  Check_Comunicacion_Maq();
}

/*Verifica Comunicación Maquina cada Timeout cuando llega un dato por el puerto COM1
el Timeout es  reseteado.*/
void Check_Comunicacion_Maq(void)
{
  Bandera_RS232 = millis();
  Msg_RS232 = millis();

  if (Excepcion == 0)
  {
    Excepcion = 1;
    Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
  }

  if (Bandera_RS232 - Bandera_RS232_F >= Timeout_RS232)
  {
    Variables_globales.Set_Variable_Global(Comunicacion_Maq, false);
    Fallo_Comunicacion = true; /*SI FALLO LA CONMUNICACIÓN*/
    if (Msg_RS232 - Msg_RS232_F >= Timeout_Msg)
    {
      #ifdef Debug_Comunicacion_MQ
      Serial.println("No  Hay Comunicación con la maquina");
      #endif
      Msg_RS232_F = Msg_RS232;
    }
  }
  else
  {
    if (Fallo_Comunicacion == true)
    {
      if (flag_ultimo_contador_Ok)
      {
        Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
        if (Msg_RS232 - Msg_RS232_F >= Timeout_Msg)
        {
          #ifdef Debug_Comunicacion_MQ
          Serial.println("Comunicación con la maquina OK");
          #endif
          Msg_RS232_F = Msg_RS232;
        }
        Fallo_Comunicacion = false;
      }
    }
    else
    {
      Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
      if (Msg_RS232 - Msg_RS232_F >= Timeout_Msg)
      {
        #ifdef Debug_Comunicacion_MQ
        Serial.println("Comunicación con la maquina OK");
        #endif
        Msg_RS232_F = Msg_RS232;
      }
    }
  }
}