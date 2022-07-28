#include "ArduinoOTA.h"
#include <Arduino.h>
#include "Memory_SD.h"
#define Mode_Bootloder_A 1
#define Mode_Program 0
#define WIFI_Status 15
TaskHandle_t Modo_Bootloader;



void Setup_Bootloader(void);
void  Init_Bootloader();
static void Rum_Bootloader(void*parameter);

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
              { 
                Serial.println("\nEnd"); 
              })
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
    ArduinoOTA.setPassword("admin");
}

void  Init_Bootloader()
{
  
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
  Setup_Bootloader(); // Setup
  vTaskSuspend(Modo_Bootloader);//  bootloader En Pausa.
  
  int Conteo = 0;
  int Mensaje=1;
  Serial.println("Bootloader Activado..");
  for (;;)
  {
    if(Mensaje==1)
    {
       Serial.println("Bootloader Activado..");
    }
    Mensaje=0;
   
    Conteo++;
    ArduinoOTA.handle();
   // delay(100);
   delay(10);
   
  }
  vTaskDelay(10);
  vTaskDelete(NULL);
}