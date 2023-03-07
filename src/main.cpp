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

#define Debug_FTP
#define Debug_Status_SD
#define Debug_Escritura
#define Info_SD
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
extern bool Enable_Status;
extern int Intento_Connect_SD; // Variable Contadora de Intentos de Conexión SD.
extern int Contador_Escrituras;

unsigned long Timer_SD_CHECK=0;
unsigned long Timer_SD_Previous=0;
unsigned long SD_CHECK_Timer=10000;
bool Handle_SD=false;

void check_SD(void);
void setup()
{
  Variables_globales.Init_Variables_Globales();
  Tabla_Evento.Init_Tabla_Eventos();
  Init_Config(); // Config Perifericos
}

void loop()
{

  

  if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9)
  {
    Check_Comunicacion_Maq(); /* Verifica contador */
  }
  if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6)
  {
    check_SD(); /*Ejecuta cada 1000ms*/
  }
  
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
   // Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
   Fallo_Comunicacion=true;
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

void check_SD(void)
{

  Timer_SD_CHECK = millis();

  if ((Timer_SD_CHECK - Timer_SD_Previous) >= SD_CHECK_Timer)
  {
    if (Handle_SD == false)
    {
      /* Ejecuta cada 1000ms*/
      if (Ftp_Out == true)
      {
        Variables_globales.Set_Variable_Global(Ftp_Mode, false);
        Ftp_Out = false;
        if (Variables_globales.Get_Variable_Global(Ftp_Mode) == false)
        {
          Ftp_Out = false;
          Serial.println("------------------>>>FTP OUT");
          vTaskSuspend(Ftp_SERVER);
        }
      }
      uint8_t Temperatura_Procesador_GPU = temperatureRead();
      Variables_globales.Set_Variable_Global_String(Temperatura_procesador, String(Temperatura_Procesador_GPU));
      if (!SD.begin(SD_ChipSelect)) // SD Desconectada...
      {
#ifdef Info_SD
        Serial.println("Memoria SD Desconectada..");
#endif
        Variables_globales.Set_Variable_Global(SD_INSERT, false);
        if (!SD.begin(SD_ChipSelect))
        {
          // Apaga  Indicador LED SD Status.
          Enable_Status = false; // Desactiva Parpadeo de LED SD Status en Modo FTP Server

// Intenta Conectar Despues de Fallo.
#ifdef Info_SD
          Serial.print("Fallo en Conexión SD"); // Mensaje de Fallo.
          Serial.print(" Intento #: ");         // Mensaje de Fallo.
          Serial.println(Intento_Connect_SD);   // Imprime conteo de Fallos.
#endif
          Intento_Connect_SD++;

          if (Intento_Connect_SD >= 10)
          {
            Intento_Connect_SD = 0;
          }
          digitalWrite(SD_Status, LOW); // Apaga Indicador LED SD Status.
          Variables_globales.Set_Variable_Global(Enable_SD, false);
          Variables_globales.Set_Variable_Global_String(Espacio_Libre_SD, "0000");
          Variables_globales.Set_Variable_Global_String(Espacio_Usado_SD, "0000");
          Variables_globales.Set_Variable_Global_String(Size_SD, "0000");
          // Sd_Mont = false;
          Variables_globales.Set_Variable_Global(SD_INSERT, false);
        }
      }
      else
      {

#ifdef Info_SD
        Serial.println("SD OK"); // Mensaje de Conexión SD.
#endif
        FreeSpace_SD();
        Variables_globales.Set_Variable_Global(Enable_SD, true);
        digitalWrite(SD_Status, HIGH); // Enciende Indicador LED SD Status.
        Enable_Status = true;          // Habilita  El Parpadeo de LED SD Status en Modo FTP Server
        // Sd_Mont = true;
        Variables_globales.Set_Variable_Global(SD_INSERT, true);
        // log_e("Error Inicializando SD: ", ERROR_INICIALIZANDO_SD);
        // LOG_ESP(Archivo_LOG,Variables_globales.Get_Variable_Global(Enable_Storage));
        if (Contador_Escrituras > 0 && Variables_globales.Get_Variable_Global(Enable_Storage) == true)
        {
          Variables_globales.Set_Variable_Global(Estado_Escritura, true);
        }
        else
        {
          Variables_globales.Set_Variable_Global(Estado_Escritura, false);
        }
        // Serial.println("Memoria SD Conectada");
        if (Variables_globales.Get_Variable_Global(Ftp_Mode) == false)
        {

          int Total_SD = SD.totalBytes() / (1024 * 1024);
          int Usado_SD = SD.usedBytes() / (1024 * 1024);
          int Libre_SD = Total_SD - Usado_SD;
#ifdef Info_SD
          Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
          Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
          Serial.println("Free Space: " + String(Libre_SD)) + "MB";
#endif
          Variables_globales.Set_Variable_Global_String(Espacio_Libre_SD, String(Libre_SD)); // Guarda  espacio libre de memoria
          Variables_globales.Set_Variable_Global_String(Espacio_Usado_SD, String(Usado_SD));
          Variables_globales.Set_Variable_Global_String(Size_SD, String(Total_SD));

          if (Libera_Memoria(Total_SD, Usado_SD))
          {
            Handle_SD = true; /* Deshabilita Verificación SD */
#ifdef Debug_Escritura
            Serial.println("Alerta Memoria llena Borrando Datos...");
#endif
            /*Memoria llena*/
            Variables_globales.Set_Variable_Global(Flag_Memoria_SD_Full, true);
          }
          else
          {
            /*Memoria libre Espacio usado <80% de memoria total*/
            Variables_globales.Set_Variable_Global(Flag_Memoria_SD_Full, false);
            Handle_SD = false;
          }
        }
      }
    }

    else if (Handle_SD == true)
    {
          int Total_SD = SD.totalBytes() / (1024 * 1024);
          int Usado_SD = SD.usedBytes() / (1024 * 1024);
          int Libre_SD = Total_SD - Usado_SD;
#ifdef Info_SD
          Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
          Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
          Serial.println("Free Space: " + String(Libre_SD)) + "MB";
#endif
          Variables_globales.Set_Variable_Global_String(Espacio_Libre_SD, String(Libre_SD)); // Guarda  espacio libre de memoria
          Variables_globales.Set_Variable_Global_String(Espacio_Usado_SD, String(Usado_SD));
          Variables_globales.Set_Variable_Global_String(Size_SD, String(Total_SD));
      if (Libera_Memoria(Total_SD, Usado_SD))
      {
#ifdef Debug_Escritura
        Serial.println("Buscando archivos...");
        Serial.println("Alerta Memoria llena Borrando Datos...");
#endif
        /*Memoria llena*/
        Variables_globales.Set_Variable_Global(Flag_Memoria_SD_Full, true);
        
      }
      else
      {
        /*Memoria libre Espacio usado <80% de memoria total*/
        Variables_globales.Set_Variable_Global(Flag_Memoria_SD_Full, false);
        Handle_SD = false;
      }
    }
    Timer_SD_Previous = millis();
  }
}