#include "ArduinoOTA.h"
#include <Arduino.h>
#include "Memory_SD.h"
#define Mode_Bootloder_A 1
#define Mode_Program 0
#define WIFI_Status 15
TaskHandle_t Modo_Bootloader;
extern bool Enable_Status;
extern WiFiClient client; // Declara un objeto cliente para conectarse al servidor

void Setup_Bootloader(void);
void  Init_Bootloader();
static void Rum_Bootloader(void*parameter);

void Setup_Bootloader(void)
{
  if(WiFi.status() == WL_CONNECTED){
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
  }
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
  vTaskSuspend(Modo_Bootloader);//  bootloader En Pausa.
}
static void Rum_Bootloader(void*parameter)
{
  Setup_Bootloader(); // Setup
  
  unsigned long InicialTime = 0;
  unsigned long FinalTime = 0;
  int Conteo = 0;
  bool SD_State = LOW;
  bool WIFI_State=LOW;
  Serial.println("Bootloader Activado..");
  for (;;)
  {
    if ((InicialTime - FinalTime) > 100)
    {
        SD_State = !SD_State;
        WIFI_State=!WIFI_State;
        digitalWrite(SD_Status, !SD_State);
        digitalWrite(WIFI_Status, ! WIFI_State);
        FinalTime = InicialTime;
    }
    delay(10);
    ArduinoOTA.handle();
  }
  vTaskDelay(10);
  vTaskDelete(NULL);
}