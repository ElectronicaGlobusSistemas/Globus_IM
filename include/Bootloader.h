#include "ArduinoOTA.h"
#include <Arduino.h>
#include "Memory_SD.h"
#include "Preferences.h"
#include "ESP32Time.h"

#define Mode_Bootloder_A 1
#define Mode_Program 0
#define WIFI_Status 15
TaskHandle_t Modo_Bootloader;
extern Preferences NVS;
extern ESP32Time RTC;

void Setup_Bootloader(void);
void  Init_Bootloader();
static void Rum_Bootloader(void*parameter);
void Update_Date(void);
//Variables_Globales Variables_globales;
bool Bootloader_Enable;
bool Termina_Bootlader_Timeout=false;

void Setup_Bootloader(void)
{
  ArduinoOTA.setPassword("FEFE99098F1EAC0361BE7BBCC3342204");
  ArduinoOTA.onStart([]()
                     {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type); })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed"); });

  ArduinoOTA.begin();
}

void  Init_Bootloader()
{
  Setup_Bootloader(); // Setup
  xTaskCreatePinnedToCore(
      Rum_Bootloader,           //  Funcion a implementar la tarea
      "Modo_Bootlader",         //  Nombre de la tarea
      10000,                    //  Tamaño de stack en palabras (memoria)
      NULL,                     //  Entrada de parametros
      configMAX_PRIORITIES - 5, //  Prioridad de la tarea
      &Modo_Bootloader,         //  Manejador de la tarea
      0);                       //  Core donde se ejecutara la tarea
  
}
static void Rum_Bootloader(void*parameter)
{
 
  vTaskSuspend(Modo_Bootloader);//  bootloader En Pausa.
  
  int Conteo = 0;
  bool Captura_Fecha=true;
  unsigned long Tiempo_Actual_Bootloader= 0;
  unsigned long Tiempo_Previo_Bootloader=0;

  for (;;)
  {
    Tiempo_Actual_Bootloader = millis();

    if (Captura_Fecha == true)
    {
      Update_Date();
      Captura_Fecha = false;
    }

    if ((Tiempo_Actual_Bootloader - Tiempo_Previo_Bootloader) >= 500000) // 5 minutos
    {
      Termina_Bootlader_Timeout=true;
      Tiempo_Previo_Bootloader = Tiempo_Actual_Bootloader;
    }

    Conteo++;
    ArduinoOTA.handle();
   // delay(100);
   delay(10);
   
  }
  vTaskDelay(10);
  vTaskDelete(NULL);
}

void Update_Date(void)
{
  Serial.println("Bootloader Activado..");
  /*Captura información inicio Bootloader*/
  String Hora = RTC.getTime();
  String Fecha = RTC.getDate();
  String Mes;
  int month = RTC.getMonth();
  switch (month)
  {
  case 0:
   Mes = "01";
   break;
  case 1:
   Mes = "02";
   break;
  case 2:
   Mes = "03";
   break;
  case 3:
   Mes = "04";
   break;
  case 4:
   Mes = "05";
   break;
  case 5:
   Mes = "06";
   break;
  case 6:
   Mes = "07";
   break;
  case 7:
   Mes = "08";
   break;
  case 8:
   Mes = "09";
   break;
  case 9:
   Mes = "10";
   break;
  case 10:
   Mes = "11";
   break;
  case 11:
   Mes = "12";
   break;
  default:
   break;
  }
  Serial.println("Guardando Fecha Bootloader.....");
  NVS.begin("Config_ESP32", false);
  uint8_t Fecha_Modo_Bootloader[] = {Hora[0], Hora[1], Hora[3], Hora[4], Hora[6], Hora[7], Fecha[9], Fecha[10], Mes[0], Mes[1], Fecha[14], Fecha[15]};
  NVS.putBytes("Fecha_Boot", Fecha_Modo_Bootloader, sizeof(Fecha_Modo_Bootloader));

  size_t Fecha_len = NVS.getBytesLength("Fecha_Boot");
  uint8_t Datos_Fecha_B[Fecha_len];
  NVS.getBytes("Fecha_Boot", Datos_Fecha_B, sizeof(Datos_Fecha_B));
  NVS.end();
}