
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include "cert.h"
#include "AutoUpdate.h"
#include <esp_task_wdt.h>
#include "Clase_Variables_Globales.h"
#include "ESP32Time.h"
#include "time.h"
#include "Preferences.h"

extern Preferences NVS;   
extern ESP32Time RTC; // Objeto contiene hora y fecha
//#define  Debug_HTTPS 

extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales
/* Verifica  estado de la maquina  para definir  si puede lanzar actualización */
void AutoUpdate::Auto_Update(bool Flag_Maquina_en_Juego_, bool Hopper_Poker_, bool Billete_Insert__, bool Flag_Premio_pagado_, bool Flag_Sesion_Player_Tracking, int Creditos_Actuales)
{
  if(!Flag_Maquina_en_Juego_ &&  !Hopper_Poker_&&!Billete_Insert__&&!Flag_Premio_pagado_ &&!Flag_Sesion_Player_Tracking && Creditos_Actuales<10)
  {
     if(FirmwareVersionCheck())
       firmwareUpdate();
  }else{

    Variables_globales.Set_Variable_Global(Updating_System,false);
    Confirmacion_ACK_HTTPS(MACHINE_IN_GAME,DES_NO);
    #ifdef Debug_HTTPS 
    Serial.println("Maquina en juego ");
    #endif
    /* Transmite_ACK MAQUINA EN JUEGO ABORTA ACTUALIZACION  */
  }
}

/* Inicializa Parametros de actualizacion
URL_Version  Direccion que contiene el archivo para verificar la version nueva del firmware
URL_Bin Direccion que  contiene el archivo de actualización 
Token  Codigo de seguridad por la API */
void AutoUpdate::Init_AutoUpdate(String Version_Firmware_, String URL_Generic_,String Api_ver ,String URL_Bin_,String Respuest ,String Token_ ,uint8_t Buffer[])
{

  VERSION_FIR.replace("V_",""); /* Elimina V_ de V_2.0.0.0*/
  VERSION_FIR.trim();
  //Serial.println(VERSION_FIR);
  VERSION_FIR=Version_Firmware_; /* Version de firmware desde Api*/
  URL_GENERIC=URL_Generic_; /* URL Generica */
  API_BIN=URL_Bin_; /* URL Archivo de actualizacion */
  API_RES=Respuest; /**/
  TOKEN_VALI=Token_;
  VERSION_FIR_LOCAL;
  API_VER=Api_ver;
  String Temp_Ver_Fir_Local;
  Millis_R = 0;
  Millis_P = 0;
  Invert_Update = 30000; 
  

/* Verifica version de programa  en Codigo */
  for (int i = 0; i < sizeof(Buffer) / sizeof(Buffer[0]); ++i)
  {
    Temp_Ver_Fir_Local += String(Buffer[i]);
    if (i < sizeof(Buffer) / sizeof(Buffer[0]) - 1)
    {
      Temp_Ver_Fir_Local+= ".";
    }
  }

  /* Version Firmware Local  String */
  VERSION_FIR_LOCAL=Temp_Ver_Fir_Local;
 // Serial.println(VERSION_FIR_LOCAL);
}


/* Ejecuta actualización de firmware */
void AutoUpdate::firmwareUpdate(void)
{
  //  http://192.168.1.55:9090/api/Tarjeta/Descargar?Mac=acb&Ver=2.5.0.0  
  WiFiClient client;
  //client.setCACert(rootCACertificate);
  httpUpdate.setLedPin(4,HIGH); /* LED Status SD*/
  
  esp_task_wdt_init(1000000, true);
  esp_task_wdt_reset();
  HTTPClient http;

  String URL_Complet=URL_GENERIC+API_BIN+"?"+"Mac="+WiFi.macAddress()+"&Ver="+VERSION_FIR_LOCAL;
 // #ifdef Debug_HTTPS
 // Serial.println(URL_Complet);
 // #endif
  http.addHeader("Authorization", "Bearer " + String(TOKEN_VALI));
  t_httpUpdate_return ret = httpUpdate.update(client, URL_Complet,"",TOKEN_VALI);
 
  esp_task_wdt_reset();
  switch (ret) {
  case HTTP_UPDATE_FAILED:
    #ifdef Debug_HTTPS 
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    #endif
    Variables_globales.Set_Variable_Global(Updating_System,false);
    Confirmacion_ACK_HTTPS(ERROR_DES,DES_NO);
    
    break;

  case HTTP_UPDATE_NO_UPDATES:
    #ifdef Debug_HTTPS
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    #endif
    Variables_globales.Set_Variable_Global(Updating_System,false);
    Confirmacion_ACK_HTTPS(ERROR_DES,DES_NO);
    
    break;

  case HTTP_UPDATE_OK:
    #ifdef Debug_HTTPS
    Serial.println("HTTP_UPDATE_OK");
    #endif
    break;
  }
 
}

/* Compara version instalada y version de actualización */
int AutoUpdate::FirmwareVersionCheck(void)
{
 

  if(VERSION_FIR_LOCAL==VERSION_FIR)
  {
    #ifdef Debug_HTTPS
    Serial.println("Version  de firmware igual a version   instalada ");
    #endif
    Variables_globales.Set_Variable_Global(Updating_System,false);
    Confirmacion_ACK_HTTPS(INSTALL_VERSION_CURRENT,INS_OK);
    return false;
  }
  else{
    #ifdef Debug_HTTPS
    Serial.println("Version  Nueva dectada!");
    #endif
    Confirmacion_ACK_HTTPS(NEW_VERSION,VER_OK);
    return true;
  }




// //http://192.168.1.55:9090/api/Tarjeta/Verificar?Mac=acb&Ver=2.5.0.0

//   String payload;
//   int httpCode;
//   String fwurl = URL_GENERIC+"?"+"Mac="+WiFi.macAddress()+"&Ver="+VERSION_FIR_LOCAL;
//  // #ifdef Debug_HTTPS
//   Serial.println(fwurl);
//  // #endif
//   WiFiClient *client = new WiFiClient;

//   if (client)
//   {
//   //  client->setCACert(rootCACertificate);

//     HTTPClient https;

//     if (https.begin(*client, fwurl))
//     {
//       https.addHeader("Authorization", "Bearer " + String(TOKEN_VALI)); // Agrega el token de autorización
//       #ifdef Debug_HTTPS
//       Serial.print("[HTTPS] GET...\n");
//       #endif
//       delay(100);
//       httpCode = https.GET();
//       delay(100);
//       Serial.println(httpCode);
//       if (httpCode == HTTP_CODE_OK)
//       {
//         payload = https.getString();
//       }
//       else
//       {
//         #ifdef Debug_HTTPS
//         Serial.print("error in downloading version file:");
//         Serial.println(httpCode);
//         #endif
//       }
//       https.end();
//     }
//     delete client;
//     client->stop();
//   }

//   if (httpCode == HTTP_CODE_OK)
//   {
//     payload.trim();
//     if (payload.equals(VERSION_FIR_LOCAL))
//     {
//       #ifdef Debug_HTTPS
//       Serial.printf("\nDevice already on the latest firmware version:%s\n", VERSION_FIR_LOCAL);
//       #endif 
//       return 0;
//     }
//     else
//     {
//       #ifdef Debug_HTTPS
//       Serial.println(payload);
//       Serial.println("New firmware detected");
//       #endif
//       return 1;
//     }
//   }
//   return 0;
}

/* Metodo para envio de mensajes de  confirmacion de procesos por API  */
bool AutoUpdate::Confirmacion_ACK_HTTPS(String Ack, String Code)
{

  
//http://192.168.1.55:9090/api/Tarjeta/Respuesta?Mac=abc&Ver=2.5.0.0&Tipo=DES_OK&Msj=10
  int httpCode;
  String fwurl;
   
  
  if(Code==INS_OK)
    fwurl = URL_GENERIC+API_RES+"?"+"Mac="+WiFi.macAddress()+"&Ver="+VERSION_FIR+"&Tipo="+Code+"&Msj="+Ack;
  else
    fwurl = URL_GENERIC+API_RES+"?"+"Mac="+WiFi.macAddress()+"&Ver="+VERSION_FIR_LOCAL+"&Tipo="+Code+"&Msj="+Ack;
    
  /* 
  Agregar URL Datos confirmacion */
 // #ifdef Debug_HTTPS
//  Serial.println(fwurl);
 // #endif
  WiFiClient *client = new WiFiClient;
  
  if (client) {

    HTTPClient https;

    if (https.begin(*client, fwurl)) {
      https.addHeader("Authorization", "Bearer " + String(TOKEN_VALI)); // Agrega el token de autorización
      #ifdef Debug_HTTPS
      Serial.print("[HTTPS] GET...\n");
      #endif
      httpCode = https.GET();
      if (httpCode == HTTP_CODE_OK) {
        #ifdef Debug_HTTPS
        Serial.println("Ack enviado");
        #endif
        return true;
      } else {
        #ifdef Debug_HTTPS
        Serial.print("Ack no enviado");
        Serial.println(httpCode);
        #endif
         return false;
      }
      https.end();
    }
    client->stop();
    delete client; 
   
  }
  return false;
}

/* Guarda fecha  de actualización en  memoria  para consulta por comando info tarjeta */
bool AutoUpdate::DateTime_Update(bool Enable)
{
  if (Enable)
  {
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
    #ifdef Debug_HTTPS
    Serial.println("Guarda fecha de actualización");
    #endif
    NVS.begin("Config_ESP32", false);
    uint8_t Fecha_Modo_Bootloader[] = {Hora[0], Hora[1], Hora[3], Hora[4], Hora[6], Hora[7], Fecha[9], Fecha[10], Mes[0], Mes[1], Fecha[14], Fecha[15]};
    NVS.putBytes("Fecha_Boot", Fecha_Modo_Bootloader, sizeof(Fecha_Modo_Bootloader));

    size_t Fecha_len = NVS.getBytesLength("Fecha_Boot");
    uint8_t Datos_Fecha_B[Fecha_len];
    NVS.getBytes("Fecha_Boot", Datos_Fecha_B, sizeof(Datos_Fecha_B));
    NVS.end();
  }
}
