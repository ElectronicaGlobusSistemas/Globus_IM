#include "ArduinoOTA.h"
#include <Arduino.h>
#include "Memory_SD.h"
#include "Preferences.h"
#include "ESP32Time.h"
#include "Clase_Variables_Globales.h"

#include "Pantalla_TFT.h"
#include "SD.h"
#include <ESPAsyncWebServer.h>

#include "Buffer_Cashless.h"

extern Transsaccion_Cashless Cashless;

#define Mode_Bootloder_A 1
#define Mode_Program 0
#define WIFI_Status 15
TaskHandle_t Modo_Bootloader;
extern Preferences NVS;
extern ESP32Time RTC;

extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales
extern Pantalla_TFT DisplayTFT;
void Setup_Bootloader(void);
void  Init_Bootloader();
static void Rum_Bootloader(void*parameter);

void Inicializa_Display_TFT(void);
void Update_Date(void);
//Variables_Globales Variables_globales;
bool Bootloader_Enable;
bool Termina_Bootlader_Timeout=false;

extern bool Update_Enable_DTime;
extern void Fecha_Update(bool Enable);


bool Last_Bootloader_Status=false;


unsigned long Timeout_Mantenimiento_Inicial=0;
unsigned long Timeout_Mantenimiento_Final=0;
int Time_Mantenimiento=60000;
unsigned long ultimo_progreso_ota = 0;


extern AsyncWebServer Server_API;

void Setup_Bootloader(void)
{
  ArduinoOTA.setPassword("FEFE99098F1EAC0361BE7BBCC3342204");

  ArduinoOTA.setTimeout(30000);
  ArduinoOTA.onStart([]()
                     {
                       String type;
                       if (ArduinoOTA.getCommand() == U_FLASH)
                         type = "sketch";
                       else // U_SPIFFS
                         type = "filesystem";

                       // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                       Serial.println("Start updating " + type);
                       Variables_globales.Set_Variable_Global(Flag_Update_OTA, true);
                       if (Variables_globales.Get_Variable_Global(SD_INSERT) && Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
                       {
                         Variables_globales.Set_Variable_Global(SD_INSERT, false);

                         Menssage_TFT("Actualizando interfaz Globus IM", 500, false);
                       }

                       ultimo_progreso_ota = millis(); })

      .onEnd([]()
             {
               Serial.println("\nEnd");
               Variables_globales.Set_Variable_Global(Flag_Update_OTA, false);
               // DisplayTFT.Notify_Now(EVENT_ON_END);
             })
      .onProgress([](unsigned int progress, unsigned int total)
                  {
                    ultimo_progreso_ota = millis(); // Reinicia timeout

                    int pct = (progress * 100) / total;

                    // Variable estática para recordar el último porcentaje que enviamos
                    static int last_pct = -1;

                    // Si es múltiplo de 5 Y no lo hemos enviado antes
                    if (pct % 5 == 0 && pct != last_pct)
                    {
                      last_pct = pct; // Lo marcamos como enviado

                      // Creamos el mensaje en un buffer fijo (NUNCA uses String aquí)
                      char msg[45];
                      snprintf(msg, sizeof(msg), "Actualizando interfaz Globus IM %d%%", pct);

                      // Enviamos a la pantalla
                      Menssage_TFT(msg, 500, false);
                    }

                    // El serial sí puede imprimir todo siempre, es rápido y no usa red
                    // Serial.printf("Sin progreso OTA: %lu ms\n",
                    // millis() - ultimo_progreso_ota);
                  })

      .onError([](ota_error_t error)
               {
                 Serial.printf("Error[%u]: ", error);
                 Variables_globales.Set_Variable_Global(Flag_Update_OTA, false);
                 // ArduinoOTA.end();

                 if (error == OTA_AUTH_ERROR)
                   Serial.println("Auth Failed");
                 else if (error == OTA_BEGIN_ERROR)
                   Serial.println("Begin Failed");
                 else if (error == OTA_CONNECT_ERROR)
                   Serial.println("Connect Failed");
                 else if (error == OTA_RECEIVE_ERROR)
                   Serial.println("Receive Failed");
                 else if (error == OTA_END_ERROR)
                   Serial.println("End Failed"); 
                  
                  //DisplayTFT.Notify_Now(EVENT_ON_ERROR);

                  Menssage_TFT("Actualización fallida", 500, true); });

  ArduinoOTA.begin();
  Serial.println("Inicializa Servicio OTA");
}

bool Modo_Mantenimiento(bool Modo_OTA)
{
  if (Modo_OTA)
  {



    Cashless.Init_API_Server(); 

    Server_API.begin();

    

    if (Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
    {
      Inicializa_Display_TFT();

      if (!Variables_globales.Get_Variable_Global(Flag_Update_OTA))
      {
        Menssage_TFT("Modo de actualizacion activo\n Esperando actualizacion", 500, false);
      }
    }

    

    

    NVS.begin("Config_ESP32", false);
    NVS.putBool("Modo_OTA", false);
    NVS.end();

    Setup_Bootloader();

    // unsigned long tiempo_inicio = millis();
    int wifi_fail_count = 0; // Contador de fallos
    ultimo_progreso_ota = millis();

    while (millis() - ultimo_progreso_ota < 60000)
    {

      ArduinoOTA.handle();

      // ---> COMPROBACIÓN DE RED <---
      if (WiFi.status() != WL_CONNECTED)
      {
        wifi_fail_count++;

        if ((wifi_fail_count % 50) == 0)
        {
          WiFi.reconnect();
        }

        if (wifi_fail_count > 100)
        {
          break;
        }
      }
      else
      {
        wifi_fail_count = 0; // Si hay red, reiniciamos el contador
      }
      // --------------------------------

      delay(10);
    }

    // Si sale por el break del WiFi o por el timeout de 60s, reinicia
    delay(100);
    ESP.restart();
  }

  return false;
}

void  Init_Bootloader()
{
  Setup_Bootloader(); // Setup
  // xTaskCreatePinnedToCore(
  //     Rum_Bootloader,           //  Funcion a implementar la tarea
  //     "Modo_Bootlader",         //  Nombre de la tarea
  //     5000,                    //  Tamaño de stack en palabras (memoria) //10000
  //     NULL,                     //  Entrada de parametros
  //     configMAX_PRIORITIES - 5, //  Prioridad de la tarea
  //     &Modo_Bootloader,         //  Manejador de la tarea
  //     0);                       //  Core donde se ejecutara la tarea
}

bool Handle_Bootloader()
{
  if (WiFi.status() == WL_CONNECTED &&
      Variables_globales.Get_Variable_Global(Bootloader_Mode))
  {
    if (!Update_Enable_DTime)
    {
      Fecha_Update(true);
      Update_Enable_DTime = true;
    }

    

    ArduinoOTA.handle();

    if (Variables_globales.Get_Variable_Global(Flag_Update_OTA))
    {
      vTaskDelay(1);
      return true;
    }

  }

  return false;
}

// static void Rum_Bootloader(void*parameter)
// {
 
//   vTaskSuspend(Modo_Bootloader);//  bootloader En Pausa.
  
//   int Conteo = 0;
//   bool Captura_Fecha=true;
//   unsigned long Tiempo_Actual_Bootloader= 0;
//   unsigned long Tiempo_Previo_Bootloader=0;

//   for (;;)
//   {
//     Tiempo_Actual_Bootloader = millis();

//     if (Captura_Fecha == true)
//     {
//       Update_Date();
//       Captura_Fecha = false;
//     }

//     if ((Tiempo_Actual_Bootloader - Tiempo_Previo_Bootloader) >= 500000) // 5 minutos
//     {
//       Termina_Bootlader_Timeout=true;
//       Tiempo_Previo_Bootloader = Tiempo_Actual_Bootloader;
//     }

//     Conteo++;
//     ArduinoOTA.handle();
//    // delay(100);
//    delay(10);
   
//   }
//   vTaskDelay(10);
//   vTaskDelete(NULL);
// }

// void Update_Date(void)
// {
//   Serial.println("Bootloader Activado..");
//   /*Captura información inicio Bootloader*/
//   String Hora = RTC.getTime();
//   String Fecha = RTC.getDate();
//   String Mes;
//   int month = RTC.getMonth();
//   switch (month)
//   {
//   case 0:
//    Mes = "01";
//    break;
//   case 1:
//    Mes = "02";
//    break;
//   case 2:
//    Mes = "03";
//    break;
//   case 3:
//    Mes = "04";
//    break;
//   case 4:
//    Mes = "05";
//    break;
//   case 5:
//    Mes = "06";
//    break;
//   case 6:
//    Mes = "07";
//    break;
//   case 7:
//    Mes = "08";
//    break;
//   case 8:
//    Mes = "09";
//    break;
//   case 9:
//    Mes = "10";
//    break;
//   case 10:
//    Mes = "11";
//    break;
//   case 11:
//    Mes = "12";
//    break;
//   default:
//    break;
//   }
//   Serial.println("Guardando Fecha Bootloader.....");
//   NVS.begin("Config_ESP32", false);
//   uint8_t Fecha_Modo_Bootloader[] = {Hora[0], Hora[1], Hora[3], Hora[4], Hora[6], Hora[7], Fecha[9], Fecha[10], Mes[0], Mes[1], Fecha[14], Fecha[15]};
//   NVS.putBytes("Fecha_Boot", Fecha_Modo_Bootloader, sizeof(Fecha_Modo_Bootloader));

//   size_t Fecha_len = NVS.getBytesLength("Fecha_Boot");
//   uint8_t Datos_Fecha_B[Fecha_len];
//   NVS.getBytes("Fecha_Boot", Datos_Fecha_B, sizeof(Datos_Fecha_B));
//   NVS.end();
// }