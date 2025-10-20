
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
#include "API_Gmaster.h"
#include "ArduinoJson.h"
#include "Configuracion.h"
#include "RFID.h"

extern Preferences NVS;   
extern ESP32Time RTC; // Objeto contiene hora y fecha
//#define  Debug_HTTPS 
extern AutoUpdate UpdateOTA;

extern TaskHandle_t Check_Comunication_Maq;
extern TaskHandle_t CommandProcess;
extern TaskHandle_t ManagerTask;
extern TaskHandle_t Mensajes_Server;

extern void Recovery_Task_Hopper(void);
extern API_Gmaster Api_G;
extern Configuracion_ESP32 Configuracion;
extern uint8_t Version_Firmware_[];
extern TaskHandle_t Task_Poker_Hopper;
// Declaración del timer
esp_timer_handle_t UpdateObj;


extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales



std::string IP_toString_Ip(char IP_Char[])
{
    std::stringstream ss;

    // Agregar cada octeto al stringstream
    for (int i = 0; i < 4; ++i) {
        ss << static_cast<int>(IP_Char[i]); // Convertir char a int para imprimir el valor numérico
        if (i < 3) {
            ss << '.'; // Agregar puntos entre los octetos
        }
    }

    // Obtener el string resultante
    std::string ipString = ss.str();

    return ipString;
}


/* Verifica  estado de la maquina  para definir  si puede lanzar actualización */
void AutoUpdate::Auto_Update(bool Flag_Maquina_en_Juego_, bool Hopper_Poker_, bool Billete_Insert__, bool Flag_Premio_pagado_, bool Flag_Sesion_Player_Tracking, int Creditos_Actuales, bool Mode_AP)
{

  if(!Flag_Maquina_en_Juego_ &&  !Hopper_Poker_&&!Billete_Insert__&&!Flag_Premio_pagado_ &&!Flag_Sesion_Player_Tracking && Creditos_Actuales<10 && !Mode_AP)
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

bool AutoUpdate::Update_Api_Mode(bool Token_Generado)
{
  if (Token_Generado)
  {
    int httpCode;
    String fwurl = "http://192.168.5.100:5364/Api/Tarjeta/ProcesarEventos?idMaquina=36087"; /* URL */

    WiFiClient client;

    if (client)
    {
      HTTPClient https;

      if (https.begin(client, fwurl))
      {
        /* -----------------------> Configuración de la solicitud <-------------------------------- */
        https.addHeader("Authorization", "Bearer " + String(Api_G.Get_Access_Token_String()));
        https.addHeader("Content-Type", "application/json");
        /*------------------------------------------------------------------------------------------*/
        httpCode = https.GET();

        if (httpCode == HTTP_CODE_OK)
        {

          /* -----------------------> Respuesta <-------------------------------------------------*/
          String payload = https.getString();
          /*--------------------------------------------------------------------------------------*/

          /*--------------------------------> Crea Archivo json <---------------------------------*/
          StaticJsonDocument<500>
              doc,
              filter;
          DeserializationError error = deserializeJson(doc, payload);
          /*--------------------------------------------------------------------------------------*/
          if (error)
          {
#ifdef Debug_HTTPS
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
#endif
          }
          else
          {
            bool IsSuccess = doc["IsSuccess"];
            
            
            if (IsSuccess)
            {

              String Version_Update = doc["Version"];
              String Url_Descarga = doc["Url_des"];
              String Api_Version_Update=doc["Api_Version"];
              String Url_Request=doc["Api_Request"];

              Init_AutoUpdate(Version_Update,Api_G.Get_Controlador_Api(),Api_Version_Update,Url_Descarga,Url_Request,Api_G.Get_Access_Token_String(),Version_Firmware_);

              doc.clear();
              https.end();
            
              #ifdef Debug_HTTPS
              Serial.println(" Solicitud de actualización recibida! ");
              Serial.println("---------> Parametros de actualización <----------");
              Serial.print("Version: ");
              Serial.println(Version_Update);
              Serial.print("Url Descarga: ");
              Serial.println(Url_Descarga);
              Serial.print("Url Version: ");
              Serial.println(Api_Version_Update);
              Serial.print("Url Respuestas: ");
              Serial.println(Url_Request);
              Serial.print("Controlador: ");
              Serial.println(Api_G.Get_Controlador_Api());
              Serial.print("Token Acceso: ");
              Serial.println(Api_G.Get_Access_Token_String());
              Serial.println("--------------------------------------------------");
              #endif
              return true;
            }
            doc.clear();
          }
        }
        else
        {

#ifdef Debug_HTTPS
          Serial.print("No se pudo enviar");
          Serial.println(httpCode);
#endif
        }
        https.end();
      }
      // client->stop();
      // delete client;
    }
  }
  return false;
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
void Stop_Update_Status(void *arg)
{
  if (Variables_globales.Get_Variable_Global(Updating_System))
  {
    Variables_globales.Set_Variable_Global(Updating_System, false);
    //Serial.println("Tiempo de actualización Agotado.");

    /* Apaga lectura de Hopper Poker */
    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 14)
    {
      ESP.restart();
    }
  }
  else
  {
    esp_timer_stop(UpdateObj);
  }
}

void AutoUpdate::Timer_Update(unsigned long timeout_ms)
{
  // Verificar si el temporizador ya existe
  if (UpdateObj == NULL)
  {
    esp_timer_create_args_t timer_args = {
        .callback = &Stop_Update_Status,
        .arg = NULL,                       // Argumentos opcionales (no utilizados)
        .dispatch_method = ESP_TIMER_TASK, // Ejecutar callback en una tarea
        .name = "Update"};

    // Crear el temporizador
    if (esp_timer_create(&timer_args, &UpdateObj) != ESP_OK)
    {
      //Serial.println("Error al crear el temporizador.");
      return;
    }
  }
  esp_timer_stop(UpdateObj);
  // Iniciar el temporizador con el tiempo de espera especificado
  esp_timer_start_once(UpdateObj, timeout_ms * 1000); // Convertir ms a us
 // Serial.println("Temporizador iniciado.");
}

void AutoUpdate::firmwareUpdate(void)
{
  
  WiFiClient client;
  //client.setCACert(rootCACertificate);
  httpUpdate.setLedPin(4,HIGH); /* LED Status SD*/
  HTTPClient http;
  char Current_IP[4];
  memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));
  IP_toString_Ip(Current_IP);

  std::string Ip=IP_toString_Ip(Current_IP);
  String Ip_Local=String(Ip.c_str());

  String URL_Complet=URL_GENERIC+API_BIN+"?"+"Mac="+WiFi.macAddress()+"&Ver="+VERSION_FIR_LOCAL+"&Ip="+Ip_Local;

  httpUpdate.onStart([]() {
    
    Variables_globales.Set_Variable_Global(Updating_System,true);
    Serial.println("🔄 Iniciando actualización de firmware...");
    vTaskDelete(Check_Comunication_Maq);
    vTaskDelete(CommandProcess);
    //vTaskDelete(ManagerTask);
    vTaskDelete(Mensajes_Server);
    Variables_globales.Set_Variable_Global(Comunicacion_Maq,false);
  });

  httpUpdate.onEnd([]() {

    Serial.println("✅ Actualizacion  finalizada con exito!");
    UpdateOTA.Confirmacion_ACK_HTTPS(INSTALL_OK,INS_OK);
    UpdateOTA.DateTime_Update(true);
  });

  httpUpdate.onProgress([](int cur, int total) {
    int percent = (cur * 100) / total;
    Serial.printf("Progreso: %d%%\r", percent);
    UpdateOTA.Confirmacion_ACK_HTTPS(String(percent),DES_OK);

    if (Variables_globales.Get_Variable_Global(Updating_System) && !Variables_globales.Get_Variable_Global(Access_Point_Mode))
    {
      Status_Barra(UPDATING_SYS);
    }
  });


  client.setTimeout(8000); // 30 segundos
  http.setTimeout(8000); // 30 segundos
  http.addHeader("Authorization", "Bearer " + String(TOKEN_VALI));
  t_httpUpdate_return ret = httpUpdate.update(client, URL_Complet,"",TOKEN_VALI);
  
  
  switch (ret) {

  case HTTP_UPDATE_FAILED:
    #ifdef Debug_HTTPS 
    Serial.printf(" ❌ HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    #endif
    
    esp_task_wdt_reset();
    Variables_globales.Set_Variable_Global(Updating_System,false);
    Confirmacion_ACK_HTTPS(ERROR_DES,DES_NO);

    delay(500);
    ESP.restart();
    
  break;

  case HTTP_UPDATE_NO_UPDATES:
    esp_task_wdt_reset();
    #ifdef Debug_HTTPS
    Serial.println("❌ HTTP_UPDATE_NO_UPDATES");
    #endif
    Variables_globales.Set_Variable_Global(Updating_System,false);
    Confirmacion_ACK_HTTPS(ERROR_DES,DES_NO);
   
    delay(500);
    ESP.restart();
    break;

  case HTTP_UPDATE_OK:
    esp_task_wdt_reset();
    #ifdef Debug_HTTPS
    Serial.println("✅ HTTP_UPDATE_OK");
    #endif
    break;

  default:
    delay(500);
    ESP.restart();
  break;
  }
}




// bool TurnOn_WiFi_TFT(String SSID_Temp, String PASSWORD_T)
// {

//   WiFi.mode(WIFI_MODE_STA);

//   IPAddress Local_IP(192, 168, 5, 250);
//   IPAddress Gateway(192, 168, 5, 1);
//   IPAddress SubnetMask(255, 255, 255, 0);
//   IPAddress primaryDNS(8, 8, 8, 8);   // optional
//   IPAddress secondaryDNS(8, 8, 4, 4); //

//   if (!WiFi.config(Local_IP, Gateway, SubnetMask, primaryDNS, secondaryDNS))
//     return false;

//   WiFi.setSleep(false); // Desactiva la suspensión de wifi en modo STA para mejorar la velocidad de
//   WiFi.begin(SSID_Temp.c_str(), PASSWORD_T.c_str());

//   unsigned long startAttemptTime = millis();

//   // Esperar hasta 10 segundos la conexión
//   while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000)
//   {
//     delay(200);
//   }

//   switch (WiFi.status())
//   {
//   case WL_CONNECTED:
//     return true;
//     break;

//   default:
//     return false;
//     break;
//   }
// }

// bool CheckFirmware_TFT(String Version_Actualizacion, String Version_Instalada)
// {
//   if (Version_Actualizacion == Version_Instalada)
//     return false;
//   else
//     return true;
// }

// bool Get_Token(String URL,String API_TOKEN,String API_RES)
// {

//   String Url_Completa = URL + API_TOKEN + "?" + "Mac=" + WiFi.macAddress() + "&Ip=" + WiFi.localIP();
  
//   String Token_Valido = Token_Generator_Update(Url_Completa);

// }

// void Run_Update_TFT()
// {
//   WiFiClient client;
  
//   HTTPClient http;
//   char Current_IP[4];
//   memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));
//   IP_toString_Ip(Current_IP);

//   std::string Ip = IP_toString_Ip(Current_IP);
//   String Ip_Local = String(Ip.c_str());

//   String URL_Complet = URL_GENERIC + API_BIN + "?" + "Mac=" + WiFi.macAddress() + "&Ver=" + VERSION_FIR_LOCAL + "&Ip=" + Ip_Local;
//   // #ifdef Debug_HTTPS
//   // Serial.println(URL_Complet);
//   // #endif

//   // client.setTimeout(30000); // 30 segundos
//   http.addHeader("Authorization", "Bearer " + String(TOKEN_VALI));
//   t_httpUpdate_return ret = httpUpdate.update(client, URL_Complet, "", TOKEN_VALI);

//   switch (ret)
//   {

//   case HTTP_UPDATE_FAILED:

//     Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
//   break;

//   case HTTP_UPDATE_NO_UPDATES:
//     break;

//   case HTTP_UPDATE_OK:
//     break;

//   default:
//     break;
//   }
// }

/* Compara version instalada y version de actualización */
bool AutoUpdate::FirmwareVersionCheck(void)
{
 
 // Serial.println(VERSION_FIR_LOCAL);
 // Serial.println(VERSION_FIR);
  if(VERSION_FIR_LOCAL==VERSION_FIR)
  {
    #ifdef Debug_HTTPS
    Serial.println("✅ Version  de firmware igual a version   instalada");
    #endif
    Variables_globales.Set_Variable_Global(Updating_System,false);
    Confirmacion_ACK_HTTPS(INSTALL_VERSION_CURRENT,INS_OK);
    return false;
  }
  else{
    /* Apaga lectura de Hopper Poker */
    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 14)
    {
      vTaskDelete(Task_Poker_Hopper);
    }
    #ifdef Debug_HTTPS
    Serial.println("🔄 Version  Nueva dectada!");
    #endif
    Confirmacion_ACK_HTTPS(NEW_VERSION,VER_OK);
    return true;
  }

  return false;
}

/* Metodo para envio de mensajes de  confirmacion de procesos por API  */
bool AutoUpdate::Confirmacion_ACK_HTTPS(String Ack, String Code)
{

  int httpCode;
  String fwurl;
  bool Output=false;
  char Current_IP[4];
  memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));
  IP_toString_Ip(Current_IP);

  std::string Ip = IP_toString_Ip(Current_IP);
  String Ip_Local = String(Ip.c_str());

  if (Code == INS_OK)
    fwurl = URL_GENERIC + API_RES + "?" + "Mac=" + WiFi.macAddress() + "&Ver=" + VERSION_FIR + "&Tipo=" + Code + "&Msj=" + Ack + "&Ip=" + Ip_Local;
  else
    fwurl = URL_GENERIC + API_RES + "?" + "Mac=" + WiFi.macAddress() + "&Ver=" + VERSION_FIR_LOCAL + "&Tipo=" + Code + "&Msj=" + Ack + "&Ip=" + Ip_Local;

  /*
  Agregar URL Datos confirmacion */
  // #ifdef Debug_HTTPS
  //  Serial.println(fwurl);
  // #endif
  WiFiClient client;

  HTTPClient https;

  if (https.begin(client, fwurl))
  {
    https.addHeader("Authorization", "Bearer " + String(TOKEN_VALI)); // Agrega el token de autorización
#ifdef Debug_HTTPS
    Serial.print("[HTTPS] GET...\n");
#endif
    httpCode = https.GET();
    if (httpCode == HTTP_CODE_OK)
    {
#ifdef Debug_HTTPS
      Serial.println("Ack enviado");
#endif
      Output=true;
    }
    else
    {
#ifdef Debug_HTTPS
      Serial.print("Ack no enviado");
      Serial.println(httpCode);
#endif
      Output=false;
    }
    https.end();
  }
  return Output;
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
    return true;
  }else{
    return false;
  }
}


















