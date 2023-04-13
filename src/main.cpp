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
extern bool Enable_Status;
extern int Intento_Connect_SD; // Variable Contadora de Intentos de Conexión SD.
extern int Contador_Escrituras;

unsigned long Timer_SD_CHECK=0;
unsigned long Timer_SD_Previous=0;
unsigned long SD_CHECK_Timer=10000;
bool Handle_SD=false;
bool extern Datos_OK;
bool Carga_Datos_Iniciales=false;
extern bool Counter_Final;
int Conta_Ejecuta=0;
extern bool Ultimo_Counter_;
bool Ejecuta_Instruccions_=false;


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
    check_SD(); /*Ejecuta cada 1000ms*/
}

/*Verifica Comunicación Maquina cada Timeout cuando llega un dato por el puerto COM1
el Timeout es  reseteado.*/
void Check_Comunicacion_Maq(void)
{
  Bandera_RS232 = millis();
  Msg_RS232 = millis();

  if (Bandera_RS232 - Bandera_RS232_F >= Timeout_RS232)
  {
    Variables_globales.Set_Variable_Global(Comunicacion_Maq, false);
    if (Msg_RS232 - Msg_RS232_F >= Timeout_Msg)
    {
      #ifdef Debug_Comunicacion_MQ
      Serial.println("No  Hay Comunicación con la maquina");
      #endif
      Msg_RS232_F = Msg_RS232;
      Contador_Transmision_Contadores = 0;
      Contador_Maquina_En_Juego = 0;
      Fallo_Comunicacion = true; /*SI FALLO LA CONMUNICACIÓN*/
      Datos_OK = false;
      Conta_Ejecuta = 0;
      Counter_Final = false;
      Ultimo_Counter_ = false;
      Ejecuta_Instruccions_ = false;
    }
  }
  else
  {
    /*-----------------------------> Falla la comunicación <---------------------------*/
    if (Fallo_Comunicacion == true)
    {
      if (Ejecuta_Instruccions_ == false)
      {
        Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 1);
        Ejecuta_Instruccions_ = true;
        Encuesta_Creditos_Premio();
      }

      else if (Ejecuta_Instruccions_ == true)
      {
        if (Counter_Final)
        {
          Fallo_Comunicacion = false;
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
          // Conta_Ejecuta=0;
          Counter_Final = false;
          Ultimo_Counter_ = false;
          Ejecuta_Instruccions_ = false;
          Carga_Datos_Iniciales = true;
          Serial.println("Comunicación con la maquina OK");
        }
      }
    }
    /*----------------------------------------------------------------------------*/

    /*----------------------------> Primera conexión <----------------------------*/
    else if (Datos_OK == true) /*Primera Conexión*/
    {
      if (Datos_OK == true && Carga_Datos_Iniciales == false)
      {
        // Conta_Ejecuta++;
        if (!Ejecuta_Instruccions_)
        {
          Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 1);
          Ejecuta_Instruccions_ = true;
          Encuesta_Creditos_Premio();
        }

        if (Ejecuta_Instruccions_)
        {
          if (Counter_Final == true) /*Ultimo_Counter_*/
          {
            Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
            Counter_Final = false;
            Carga_Datos_Iniciales = true;
            Conta_Ejecuta = 0;
            Ultimo_Counter_ = false;
            Ejecuta_Instruccions_ = false;
            Serial.println("Comunicación con la maquina OK");
          }
        }
      }

      if (Msg_RS232 - Msg_RS232_F >= Timeout_Msg)
      {
#ifdef Debug_Comunicacion_MQ
        Serial.println("Comunicación con la maquina OK");
#endif
        Msg_RS232_F = Msg_RS232;
      }
    }
    /*--------------------------------------------------------------------------------*/
  }
}

void check_SD(void)
{
  Timer_SD_CHECK = millis();
  
  if (Variables_globales.Get_Variable_Global(Ftp_Mode))
  {
    Rum_FTP_Server();
  }
  else
  {
    if ((Timer_SD_CHECK - Timer_SD_Previous) >= SD_CHECK_Timer)
    {
      if (Handle_SD == false)
      {
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
          Variables_globales.Set_Variable_Global(SD_INSERT, true);
          if (Contador_Escrituras > 0 && Variables_globales.Get_Variable_Global(Enable_Storage) == true)
          {
            Variables_globales.Set_Variable_Global(Estado_Escritura, true);
          }
          else
          {
            Variables_globales.Set_Variable_Global(Estado_Escritura, false);
          }
          if (Variables_globales.Get_Variable_Global(Ftp_Mode) == false)
          {

            if (Variables_globales.Get_Variable_Global(Comunicacion_Maq) && Variables_globales.Get_Variable_Global(Sincronizacion_RTC) && Variables_globales.Get_Variable_Global(Flag_Crea_Archivos) && !Variables_globales.Get_Variable_Global(Ftp_Mode) && Variables_globales.Get_Variable_Global(SD_INSERT))
            {
              Serial.println("Creado Archivos......");
              /*-----------------------> Crea Archivos fecha actual<----------------------------------------*/
              Create_ARCHIVE_Excel(Archivo_CSV_Contadores, Variables_globales.Get_Encabezado_Maquina(Encabezado_Maquina_Generica));
              delay(100);
              Create_ARCHIVE_Excel_Eventos(Archivo_CSV_Eventos, Variables_globales.Get_Encabezado_Maquina(Encabezado_Maquina_Eventos));
              delay(100);
              Create_ARCHIVE_Txt(Archivo_LOG);
              delay(100);
              Serial.println("OK Archivos Listos..");
              Variables_globales.Set_Variable_Global(Flag_Crea_Archivos, false);
              Variables_globales.Set_Variable_Global(Flag_Archivos_OK, true);
              /*--------------------------------------------------------------------------------------------*/
            }
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
}