
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "cert.h"
#include "AutoUpdate.h"
#include <esp_task_wdt.h>



/* Verifica  estado de la maquina  para definir  si puede lanzar actualización */
void AutoUpdate::Auto_Update(bool Flag_Maquina_en_Juego_, bool Hopper_Poker_, bool Billete_Insert__, bool Flag_Premio_pagado_, bool Flag_Sesion_Player_Tracking)
{
  
  if(!Flag_Maquina_en_Juego_ &&  !Hopper_Poker_&&!Billete_Insert__&&!Flag_Premio_pagado_ &&!Flag_Sesion_Player_Tracking)
  {
    if(FirmwareVersionCheck())
      firmwareUpdate();
  }else{
    Serial.println("Maquina en juego! o sesion de usuario de juego abierta");
    /* Transmite_ACK MAQUINA EN JUEGO ABORTA ACTUALIZACION  */
  }
}

/* Inicializa Parametros de actualizacion
URL_Version  Direccion que contiene el archivo para verificar la version nueva del firmware
URL_Bin Direccion que  contiene el archivo de actualización 
Token  Codigo de seguridad por la API */
void AutoUpdate::Init_AutoUpdate(String URL_Version_, String URL_Bin_, String Token_ ,uint8_t Buffer[])
{
  URL_Version_Firmware = URL_Version_;
  Token = Token_;
  URL_Bin = URL_Bin_;

  Millis_R = 0;
  Millis_P = 0;
  Invert_Update = 30000;
/* Verifica version de programa  en Codigo */
  for (int i = 0; i < sizeof(Buffer) / sizeof(Buffer[0]); ++i)
  {
    FirmwareVer += String(Buffer[i]);
    if (i < sizeof(Buffer) / sizeof(Buffer[0]) - 1)
    {
      FirmwareVer += ".";
    }
  }
}

/* Ejecuta actualización de firmware */
void AutoUpdate::firmwareUpdate(void)
{
    
  WiFiClientSecure client;
  client.setCACert(rootCACertificate);
  httpUpdate.setLedPin(4,HIGH); /* LED Status SD*/
  
  esp_task_wdt_init(1000000, true);
  esp_task_wdt_reset();
  HTTPClient http;
  http.addHeader("Authorization", "token " + String(Token));
  t_httpUpdate_return ret = httpUpdate.update(client, URL_Bin,"",Token);
 
  esp_task_wdt_reset();
  switch (ret) {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    break;

  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    break;

  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
    break;
  }
 
}

/* Función  para consultar si existe actualización disponible */
int AutoUpdate::FirmwareVersionCheck(void)
{

 
  String payload;
  int httpCode;
  String fwurl = URL_Version_Firmware;
  Serial.println(fwurl);
  WiFiClientSecure *client = new WiFiClientSecure;
 
  if (client)
  {
    client->setCACert(rootCACertificate);

    HTTPClient https;

    if (https.begin(*client, fwurl))
    {
            https.addHeader("Authorization", "token " + String(Token)); // Agrega el token de autorización
            Serial.print("[HTTPS] GET...\n");
            delay(100);
            httpCode = https.GET();
            delay(100);
            Serial.println(httpCode);
            if (httpCode == HTTP_CODE_OK)
            {
                payload = https.getString();
            }
            else
            {

                Serial.print("error in downloading version file:");
                Serial.println(httpCode);
            }
            https.end();
    }
    delete client;
    client->stop();
  }

  if (httpCode == HTTP_CODE_OK) {
    payload.trim();
    if (payload.equals(FirmwareVer)) {
      Serial.printf("\nDevice already on the latest firmware version:%s\n", FirmwareVer);
      return 0;
    } else {

      
      Serial.println(payload);
      Serial.println("New firmware detected");
      return 1;
    }
  }
  return 0;
}