#include <iostream>
#include "Buffer_Cashless.h"
#include "Arduino.h"
#include "WiFi.h"

#include "ESP32Time.h"
#include "time.h"
#include "nvs_flash.h"
#include "Preferences.h"

#include "Clase_Variables_Globales.h"

#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include "Configuracion.h"
#include "Contadores.h"
#include "RFID.h"
#include <esp_task_wdt.h>

#include "SD.h"
extern Configuracion_ESP32 Configuracion;
extern Contadores_SAS contadores; // Objeto contiene contadores maquina

extern Preferences NVS;

using namespace std;
extern ESP32Time RTC; // Objeto contiene hora y fecha

unsigned long TimeOut_Reg=0;
unsigned long TimeOut_Regf=0;
unsigned long Timout=30000;

extern const char* LogError;

extern const char* archivo;
extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales
extern Transsaccion_Cashless Cashless;
extern Buffer_RX_AFT Buffer_Cashless;
extern unsigned char Registra_Machine(void);
extern unsigned char Delete_Registro_Machine(void);
extern bool Consulta_Info_Cashless(void);
const unsigned char Tabla_Ascii_Data[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};




char Conve_Ascii_To_Hex_LL(char *Val_Ascii)
{
  char Val1, Val2;

  Val1 = *Val_Ascii;
  Val2 = Val1 - 0x30;
  return (Val2);
}

/******************************************************************************/

char Conve_Ascii_To_Hex_HH(char *Val_Ascii)
{
  char Val1, Val2;

  Val1 = *Val_Ascii;
  Val2 = Val1 - 0x30;
  Val1 = (Val2 << 4);
  return (Val1);
}


char Hex_Ascci_HIGH(char Hex_Val) {
    char Val1, Val2;

    Val1 = Hex_Val; //Paso el valor del registro
    Val2 = (Val1 & 0b11110000); //Enmascaro  el nibble superior
    Val2 = Val2 >> 4;
    Val1 = Tabla_Ascii_Data[ Val2 ];
    return ( Val1);
}

char Hex_Ascci_LOW(char Hex_Val) {
    char Val1, Val2;

    Val1 = Hex_Val; //Paso el valor del registro
    Val2 = (Val1 & 0b00001111); //Enmascaro  el nibble superior
    Val1 = Tabla_Ascii_Data[ Val2 ];
    return ( Val1);
}


bool Buffer_RX_AFT::Set_RX_AFT(int Filtro_buffer, char Buffer[])
{
 
  switch (Filtro_buffer) // Selecciona Contador Especifico.
  {
  case 1: // Bloque de instrucciones 1;
    memcpy(Info_MQ_AFT_,Buffer, sizeof(Info_MQ_AFT_) / sizeof(Info_MQ_AFT_[0]));
    return true;
    break;

  case 2:
    memcpy(Interrog_registro_, Buffer, sizeof(Interrog_registro_) / sizeof(Interrog_registro_[0]));
    return true;
    break;

  case 3: 
    memcpy(Buffer_registro_Mq, Buffer, sizeof(Buffer_registro_Mq) / sizeof(Buffer_registro_Mq[0]));
      return true;
      break;

  case 4:
    Asset_Pos_Id[0]=Buffer[4];
    Asset_Pos_Id[1]=Buffer[5];
    Asset_Pos_Id[2]=Buffer[6];
    Asset_Pos_Id[3]=Buffer[7];
    Asset_Pos_Id[4]=Buffer[28];
    Asset_Pos_Id[5]=Buffer[29];
    Asset_Pos_Id[6]=Buffer[30];
    Asset_Pos_Id[7]=Buffer[31];
    return true;
    break;

  case 5:
    memcpy(Buffer_RX_AFT_MQ, Buffer, sizeof(Buffer_RX_AFT_MQ) / sizeof(Buffer_RX_AFT_MQ[0]));
    return true;
    break;

  case 6:
    memcpy(Buffer_RX_Credit_Cashless, Buffer, sizeof(Buffer_RX_Credit_Cashless) / sizeof(Buffer_RX_Credit_Cashless[0]));
    return true;
    break;
  
  }
  return false;
}

char* Buffer_RX_AFT::Get_RX_AFT(int Filtro_buffer)
{
  switch (Filtro_buffer) // Selecciona Contador Especifico.
  {
  case 1: // Bloque de instrucciones 1;
    return Info_MQ_AFT_;
    break;

  case 2:
    return Interrog_registro_;
    break;

  case 3: 
    return Buffer_registro_Mq_AFT;
    break;

  case 4:
    return Asset_Pos_Id;
    break;

  case 5:
    return Buffer_RX_AFT_MQ;
    break;

  case 6:
    return Buffer_RX_Credit_Cashless;
    break;
  }

 // return nullptr;
}

bool Buffer_RX_AFT::Clear_Buffer(int Filtro_buffer)
{

  switch (Filtro_buffer) // Selecciona Contador Especifico.
  {
 

  case 3: 
    for (int i=0; i<128; i++)
    {
      Buffer_registro_Mq[i]=0xAA;
    }
    return true;
    break;

  case 6:
    for (int i = 0; i < 128; i++)
    {
      Buffer_RX_Credit_Cashless[i] = 0x00;
    }
    return true;
    break;

  default:
  return false;
  break;
  }

  return false;
}

void macStringToByteArray(String mac, char* bytes) {
  for (int i = 0; i < 6; i++) {
    bytes[i] = strtoul(mac.substring(i*3, i*3+2).c_str(), NULL, 16);
  }
}



/***************************************************************************************************
 * *************************************************************************************************
 * ******************************************REGISTRO MAQUINA **************************************
 * *************************************************************************************************
*/
/* Guarda Clave de Registro en memoria RAM */
bool Buffer_RX_AFT::Set_Key_Register_AFT(char Buffer[],bool Reset)
{

  if (Reset)
  {
    for (int i = 0; i < 20; i++)
    {
      Buffer_registro_Mq_AFT[i] = 0x00;
    }

    for (int i = 0; i < 20; i++)
    {
      if (Buffer_registro_Mq_AFT[i] == 0x00)
      {
      }
      else
        return false;
    }
    return true;
  }
  else
  {

    for (int i = 0; i < 20; i++)
    {
      Buffer_registro_Mq_AFT[i] = Buffer[i];
    }

    for (int i = 0; i < 20; i++)
    {
      if (Buffer_registro_Mq_AFT[i] == Buffer[i])
      {
      }
      else
        return false;
    }
  }

  return true;
}

bool Buffer_RX_AFT::Delete_Key_Register_AFT(bool Reset)
{

  if (Reset)
  {
    for (int i = 0; i < 20; i++)
    {
      Buffer_registro_Mq_AFT[i] = 0x00;
    }
    NVS.begin("Config_ESP32", false);
    NVS.putBytes("Reg_AFT", Buffer_registro_Mq_AFT, sizeof(Buffer_registro_Mq_AFT));
    NVS.end();

    if(Buffer_registro_Mq_AFT[0]==0x00 && Buffer_registro_Mq_AFT[19]==0x00)
      return true;
    else
      return false;
  }
 return false;
}
/* Retorna Clave de registro AFT */
char* Buffer_RX_AFT::Get_Key_Register_AFT(void)
{
  return Buffer_registro_Mq_AFT;
}

String Buffer_RX_AFT::Get_Key_Register_AFT_String(void)
{
  String Key;

  for (int i = 0; i < 20; i++)
  {
    if (i < 14)
    {
      Key += String(Buffer_Cashless.Get_Key_Register_AFT()[i], HEX);
    }
    else
    {
      Key += String(Buffer_Cashless.Get_Key_Register_AFT()[i], DEC);
    }
  }

  return Key;
}

/***************************************************************************************************
 * *************************************************************************************************
 * ******************************************REGISTRO MAQUINA **************************************
 * *************************************************************************************************
*/

/* Reset de Transaccion ID por cancelacion de registro */
bool Transsaccion_Cashless::Delete_Trans_ID(bool Status)
{
  if (Status)
  {
    /* --------------> Borra ID cliente <---------*/
    Num_Trans_ID = 0;
    sprintf(Counter_Trans_ID,"%07d",Num_Trans_ID);
    /*--------------------------------------------*/

    /* ---------> Guarda en memoria ID <-------- */
    NVS.begin("Config_ESP32", false);
    NVS.putUInt("Trans_ID", Num_Trans_ID);
    NVS.end();
    /*-------------------------------------------*/

    if (Num_Trans_ID == 0 && atoi(Counter_Trans_ID)==0)
      return true;
    else
      return false;
  }
  else
    return false;
}

/* Recupera numero id de transaccion  de memoria FLASH y lo retiene en RAM */
bool Transsaccion_Cashless::Set_Inicial_Trans_ID(uint32_t Trans_ID_Memory)
{
  Num_Trans_ID=Trans_ID_Memory;
  sprintf(Counter_Trans_ID,"%07d",Num_Trans_ID);

  if(Num_Trans_ID==atoi(Counter_Trans_ID))
    return true;
  else
    return false;
}

/* Incrementa ID de transacciones Maquina AFT */
bool Transsaccion_Cashless::Increase_Transaction_Number_ID(void)
{
  Num_Trans_ID++;

  if(Num_Trans_ID>9999999)
  {
    Num_Trans_ID=0;
  }
  sprintf(Counter_Trans_ID,"%07d",Num_Trans_ID);

  /* ---------> Guarda en memoria ID <-------- */
  NVS.begin("Config_ESP32", false);
  NVS.putUInt("Trans_ID",Num_Trans_ID);
  NVS.end();
  /*-------------------------------------------*/
  return true;
}



bool Transsaccion_Cashless::Sincroniza_Transaction_Number_ID(uint32_t Trans_ID_Maq)
{
  Num_Trans_ID=Trans_ID_Maq;
  sprintf(Counter_Trans_ID,"%07d",Num_Trans_ID);
  /* ---------> Guarda en memoria ID <-------- */
  NVS.begin("Config_ESP32", false);
  NVS.putUInt("Trans_ID",Num_Trans_ID);
  NVS.end();
  /*-------------------------------------------*/
  if(Num_Trans_ID==atoi(Counter_Trans_ID))
    return true;
  else
    return false;
}

uint32_t Transsaccion_Cashless::Get_Trans_ID_Int(void)
{
  return Num_Trans_ID;
}

unsigned char Transsaccion_Cashless::Get_Trans_ID_EFT(void)
{
  unsigned char Number;
  
  Number = static_cast<unsigned char>(Num_Trans_ID);
  return Number;
}

bool Transsaccion_Cashless::Increase_Transaction_Number_ID_EFT(void)
{
  Num_Trans_ID++;

  if(Num_Trans_ID>=256)
  {
    Num_Trans_ID=0;
  }
  sprintf(Counter_Trans_ID,"%07d",Num_Trans_ID);

  /* ---------> Guarda en memoria ID <-------- */
  NVS.begin("Config_ESP32", false);
  NVS.putUInt("Trans_ID",Num_Trans_ID);
  NVS.end();
  /*-------------------------------------------*/
  return true;
}

/* Retorna ID de transaccion */
char* Transsaccion_Cashless::Get_Trans_ID(void)
{
  return Counter_Trans_ID;
}


AsyncWebServer Server_API(9595);


String Transsaccion_Cashless::Get_Parameter_Print_Ticket(int Filtro)
{
   switch (Filtro)
   {
   case Location_Tick:
    return Location;
    break;
  case Adress_1_Tick:
    return Adress_1;
    break;
  case Adress_2_Tick:
    return Adress_2;
    break;
  case Restricted_Ticket_Title_Tick:
    return Restricted_Ticket_Title;
    break;
  case Debit_Ticket_Title_Tick:
    return  Debit_Ticket_Title;
    break;
   default:
     return "";
    break;
   }
   return "";
}


bool Transsaccion_Cashless::Set_Parameter_Print_Ticket(String Location_S, String Adr1_S,String Adr2_S,String Restricted_Ticket_S, String Debit_Title_S)
{
  Location=Location_S;
  Adress_1=Adr1_S; 
  Adress_2=Adr2_S; 
  Restricted_Ticket_Title=Restricted_Ticket_S; 
  Debit_Ticket_Title=Debit_Title_S;

  if(Location==Location_S&&Adress_1==Adr1_S&&Adress_2==Adr2_S&&Restricted_Ticket_Title==Restricted_Ticket_S&&Debit_Ticket_Title==Debit_Title_S)
    return true;
  else
    return false;
}




String New_Token(void)
{
 
  String MAC=WiFi.macAddress();
  String Base_JWT= "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiYWRtaW4iOnRydWUsImV4cCI6MTcxNzM1NjgwMH0.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c";
  

  String JWT=MAC+Base_JWT;
  const char* charset = JWT.c_str(); // Convertir a const char*

  String Output;


    for (int i = 0; i < JWT.length(); i++)
    {
      int index=random(0,JWT.length());
      Output += charset[index];
    }
  
 // Serial.println(Output);
  return Output;
}


bool Transsaccion_Cashless::Init_API_Server(void)
{
  
  String URL;

  /**********************************************************************************************************/
  /**********************************************CONFIG CASHLESS*********************************************/
  /*********************************************************************************************************/
  Server_API.on("/Evento_6A_Cashless_Deshabilitado",HTTP_GET, [](AsyncWebServerRequest *request){
    

    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());


    NVS.begin("Config_ESP32", false);
    NVS.putBool("Only_Cashless",false);
    bool Test=NVS.getBool("Only_Cashless",false);

    if(Test)
      Variables_globales.Set_Variable_Global(Descarga_Solo_Cashelss, true);
    else
      Variables_globales.Set_Variable_Global(Descarga_Solo_Cashelss, false);


    if(!Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss))
      jsonDocument["IsSuccess"] = true;
    else
      jsonDocument["IsSuccess"] = false;

    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
    jsonDocument["Desc"] = "Evento 6A cashless Deshabilitado";
    jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 0:
      jsonDocument["Tipo_Maq"] = "AFT";
      break;
    case 1:
      jsonDocument["Tipo_Maq"] = "AFT";
      break;
    case 2:
      jsonDocument["Tipo_Maq"] = "EFT";
      break;
    case 3:
      jsonDocument["Tipo_Maq"] = "AFT";
      break;

    default:
      jsonDocument["Tipo_Maq"] = "";
      break;
    }
    jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);

    NVS.end();

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
   // Serial.println(Json);
    request->send(200, "application/json", Json);
  });

  Server_API.on("/Evento_6A_Cashless_Habilitado",HTTP_GET, [](AsyncWebServerRequest *request){
    
    

    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)<4)
    {
      StaticJsonDocument<500> jsonDocument;
      jsonDocument.clear();
      String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());


      NVS.begin("Config_ESP32", false);
      NVS.putBool("Only_Cashless",true);
      bool Test=NVS.getBool("Only_Cashless",false);

      if(Test)
        Variables_globales.Set_Variable_Global(Descarga_Solo_Cashelss, true);
      else
        Variables_globales.Set_Variable_Global(Descarga_Solo_Cashelss, false);


      if(Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss))
        jsonDocument["IsSuccess"] = true;
      else
        jsonDocument["IsSuccess"] = false;

      jsonDocument["Fecha_Hora"] = DataTime;
      jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
      jsonDocument["Desc"] = "Evento 6A Cashless habilitado";
      jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
      switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
      {
      case 0:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
      case 1:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
      case 2:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;
      case 3:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;

      default:
        jsonDocument["Tipo_Maq"] = "";
        break;
      }
      jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
      NVS.end();

      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
    // Serial.println(Json);
      request->send(200, "application/json", Json);
    }else{

      StaticJsonDocument<500> jsonDocument;
      jsonDocument.clear();
      String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());

      Variables_globales.Set_Variable_Global(Descarga_Solo_Cashelss, false);


      if(Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss))
        jsonDocument["IsSuccess"] = true;
      else
        jsonDocument["IsSuccess"] = false;

      jsonDocument["Fecha_Hora"] = DataTime;
      jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
      jsonDocument["Desc"] = "Tipo de maquina no compatible";
      jsonDocument["Key"] = nullptr;
      switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
      {
      case 0:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
      case 1:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
      case 2:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;
      case 3:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;

      default:
        jsonDocument["Tipo_Maq"] = "";
        break;
      }
      jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
    // Serial.println(Json);
      request->send(200, "application/json", Json);

      }

   
  });

  Server_API.on("/Habilitar_Cashless",HTTP_GET, [](AsyncWebServerRequest *request){
    
    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)<4)
    {
      NVS.begin("Config_ESP32", false);
      NVS.putBool("Enable_Cashless",true);
      bool Test=NVS.getBool("Enable_Cashless",false);
      Variables_globales.Set_Variable_Global(Enable_Cashless,Test);
      NVS.end();

      if(Test && Variables_globales.Get_Variable_Global(Enable_Cashless))
      {
        int Status;
        Status = Registra_Machine();
        delay(300); /* Espera Por respuesta de la maquina */

        String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());

        StaticJsonDocument<500> jsonDocument;
        jsonDocument.clear();
        jsonDocument["IsSuccess"] = true;
        jsonDocument["Fecha_Hora"] = DataTime;
        jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
        jsonDocument["Desc"] = "Cashless Habilitado con Exito!";
        jsonDocument["Key"]=Buffer_Cashless.Get_Key_Register_AFT_String();
        
        
        switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
        {
        case 0:
            jsonDocument["Tipo_Maq"] = "AFT";
            break;
        case 1:
            jsonDocument["Tipo_Maq"] = "AFT";
            break;
        case 2:
            jsonDocument["Tipo_Maq"] = "EFT";
            break;
        case 3:
            jsonDocument["Tipo_Maq"] = "AFT";
            break;

        default:
            jsonDocument["Tipo_Maq"] = "";
            break;
        }
        jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
        String Json;
        serializeJson(jsonDocument, Json); /* Serializa Data */
       // Serial.println(Json);
        request->send(200, "application/json", Json);
      }
      else
      {
        String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());

        StaticJsonDocument<200> jsonDocument;
        jsonDocument.clear();
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Fecha_Hora"] = DataTime;
        jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
        jsonDocument["Desc"] = "No fue posible habilitar Cashless";
        jsonDocument["Key"]=Buffer_Cashless.Get_Key_Register_AFT_String();
        switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
        {
        case 0:
          jsonDocument["Tipo_Maq"] = "AFT";
          break;
        case 1:
          jsonDocument["Tipo_Maq"] = "AFT";
          break;
        case 2:
          jsonDocument["Tipo_Maq"] = "EFT";
          break;
        case 3:
          jsonDocument["Tipo_Maq"] = "AFT";
          break;
        default:
          jsonDocument["Tipo_Maq"] = "";
          break;
        }
        jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
        String Json;
        serializeJson(jsonDocument, Json); /* Serializa Data */
       // Serial.println(Json);
        request->send(200, "application/json", Json);
      }
    }
    else
    {
      String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

      StaticJsonDocument<200> jsonDocument;
      jsonDocument.clear();
      jsonDocument["IsSuccess"] = false;
      jsonDocument["Fecha_Hora"] = DataTime;
      jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
      jsonDocument["Desc"] = "No compatible con el tipo de maquina";
      jsonDocument["Key"] = nullptr;
      switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
      {
      case 0:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
      case 1:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
      case 2:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;
      case 3:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;

      default:
        jsonDocument["Tipo_Maq"] = "";
        break;
      }
      jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
     // Serial.println(Json);
      request->send(200, "application/json", Json);
    }


  });

  Server_API.on("/Inhabilitar_Cashless",HTTP_GET, [](AsyncWebServerRequest *request){
    


  if(!Variables_globales.Get_Variable_Global(Enable_Cashless))
  {
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());

    StaticJsonDocument<200> jsonDocument;
    jsonDocument.clear();

    jsonDocument["IsSuccess"] = true;
    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
    jsonDocument["Desc"] = "Cashless Inhabilitado con Exito";
    jsonDocument["Key"]=Buffer_Cashless.Get_Key_Register_AFT_String();
    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 0:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 1:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 2:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;
    case 3:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    
    default:
        jsonDocument["Tipo_Maq"] = "";
        break;
    }
    jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);

  }else{
    NVS.begin("Config_ESP32", false);
    NVS.putBool("Enable_Cashless", false);
    bool Test = NVS.getBool("Enable_Cashless", false);
    Variables_globales.Set_Variable_Global(Enable_Cashless, Test);
    NVS.end();
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());

   if(!Test && !Variables_globales.Get_Variable_Global(Enable_Cashless))
   {

    StaticJsonDocument<200> jsonDocument;
    jsonDocument.clear();

    jsonDocument["IsSuccess"] = true;
    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
    jsonDocument["Desc"] = "Cashless Inhabilitado con Exito";
    jsonDocument["Key"]=Buffer_Cashless.Get_Key_Register_AFT_String();
    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 0:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 1:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 2:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;
    case 3:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;

    case 5:
        jsonDocument["Tipo_Maq"] = "AFT"; /* Eliminar*/
        break;
    
    default:
        jsonDocument["Tipo_Maq"] = "";
        break;
    }
    jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);
   }else{
     StaticJsonDocument<200> jsonDocument;
     jsonDocument.clear();

    jsonDocument["IsSuccess"] = false;
    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
    jsonDocument["Desc"] = "No fue posible Inhabilitar Cashless";
    jsonDocument["Key"]=Buffer_Cashless.Get_Key_Register_AFT_String();
    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 0:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 1:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 2:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;
    case 3:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;

    // case 5:
    //     jsonDocument["Tipo_Maq"] = "AFT"; /* Eliminar*/
    //     break;
    
    default:
        jsonDocument["Tipo_Maq"] = "";
        break;
    }
    jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);
   }
  }  
  });

  Server_API.on("/Registra_Maquina",HTTP_GET, [](AsyncWebServerRequest *request){
  
  String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
  StaticJsonDocument<200> jsonDocument;
  jsonDocument.clear();
  switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
  {
  case 0:
    jsonDocument["Tipo_Maq"] = "AFT";
    break;
  case 1:
    jsonDocument["Tipo_Maq"] = "AFT";
    break;
  case 2:
    jsonDocument["Tipo_Maq"] = "EFT";
    break;
  case 3:
    jsonDocument["Tipo_Maq"] = "AFT";
    break;
  default:
    jsonDocument["Tipo_Maq"] = "";
    break;
  }
  if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)<4)
  {
    
   
    int Status;
    
    Status = Registra_Machine();
    delay(300); /* Espera Por respuesta de la maquina */

    switch (Status)
    {
    case 1:
      if(Cashless.Get_Trans_ID_Int()>0)
        Cashless.Delete_Trans_ID();
      Variables_globales.Set_Variable_Global(Status_AFT_Machine,true);
      jsonDocument["IsSuccess"] = true;
      jsonDocument["Fecha_Hora"] = DataTime;
      jsonDocument["Key"]=Buffer_Cashless.Get_Key_Register_AFT_String();
      jsonDocument["Trans_ID"]=Cashless.Get_Trans_ID_Int();
      jsonDocument["Desc"] = "Maquina Registrada con Exito";
      jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);

      //Serial.println("Maquina Registrada con exito!");
      break;
    case 2:
      
      //Serial.println("Maquina Registrada anteriormente");
      jsonDocument["IsSuccess"] = true;
      jsonDocument["Fecha_Hora"] = DataTime;
      jsonDocument["Key"]=Buffer_Cashless.Get_Key_Register_AFT_String();
      jsonDocument["Trans_ID"]=Cashless.Get_Trans_ID_Int();
      jsonDocument["Desc"] = "Maquina ya se encontraba registrada";
      jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
      break;
    
    default:
      //Serial.println("Maquina no responde ");

      if (!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
      {
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Fecha_Hora"] = DataTime;
        jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
        jsonDocument["Trans_ID"]=Cashless.Get_Trans_ID_Int();
        jsonDocument["Desc"] = "No hay comunicacion con la maquina";
        jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
      }else{
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Fecha_Hora"] = DataTime;
        jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
        jsonDocument["Trans_ID"]=Cashless.Get_Trans_ID_Int();
        jsonDocument["Desc"] = "No hubo respuesta de la maquina";
        jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
      }
      break;
    }

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
   // Serial.println(Json);
    request->send(200, "application/json", Json);
  }else{
    
    jsonDocument["IsSuccess"] = false;
    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Key"]=nullptr;
    jsonDocument["Trans_ID"]=Cashless.Get_Trans_ID_Int();
    jsonDocument["Desc"] = "No compatible con el tipo de maquina";
    jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
    jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
   // Serial.println(Json);
    request->send(200, "application/json", Json);
  }
  
    
  });

  Server_API.on("/Cancela_Registro_Maquina",HTTP_GET, [](AsyncWebServerRequest *request){

  StaticJsonDocument<200> jsonDocument;
  jsonDocument.clear();
  String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
  int Status;

  if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)<4)
  {
     
    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 0:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 1:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 2:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;
    case 3:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    
    default:
        jsonDocument["Tipo_Maq"] = "";
        break;
    }
    Status = Delete_Registro_Machine();
    delay(100); /* Espera Por respuesta de la maquina */

    switch (Status)
    {
    case 1:

      if (Buffer_Cashless.Delete_Key_Register_AFT(true))
        Variables_globales.Set_Variable_Global(Status_AFT_Machine, false);
      Cashless.Delete_Trans_ID();
      jsonDocument["IsSuccess"] = true;
      jsonDocument["Fecha_Hora"] = DataTime;
      if (Variables_globales.Get_Variable_Global(Status_AFT_Machine))
        jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
      else
        jsonDocument["Key"] = nullptr;
      jsonDocument["Trans_ID"]=Cashless.Get_Trans_ID_Int();
      jsonDocument["Desc"] = "Registro Maquina AFT Cancelado con Exito!";
      jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
      jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
      //Serial.println("Se elimino el registro de la maquina con exito");
      Cashless.Set_Reintento_Registro(false);
      break;
    case 2:
      
      //Serial.println("Maquina Registrada anteriormente");
      jsonDocument["IsSuccess"] = false;
      jsonDocument["Fecha_Hora"] = DataTime;
      if (Variables_globales.Get_Variable_Global(Status_AFT_Machine))
        jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
      else
        jsonDocument["Key"] = nullptr;
      jsonDocument["Trans_ID"]=Cashless.Get_Trans_ID_Int();
      jsonDocument["Desc"] = "Registro no eliminado";
      jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
      jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
      break;
    
    default:
      //Serial.println("Maquina no responde ");

      if (!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
      {
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Fecha_Hora"] = DataTime;
        if (Variables_globales.Get_Variable_Global(Status_AFT_Machine))
          jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
        else
          jsonDocument["Key"] = nullptr;
        jsonDocument["Trans_ID"]=Cashless.Get_Trans_ID_Int();
        jsonDocument["Desc"] = "No hay comunicacion con la maquina";
        jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
        jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
      }else{
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Fecha_Hora"] = DataTime;
        if (Variables_globales.Get_Variable_Global(Status_AFT_Machine))
          jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
        else
          jsonDocument["Key"] = nullptr;
        jsonDocument["Trans_ID"]=Cashless.Get_Trans_ID_Int();
        jsonDocument["Desc"] = "No hubo respuesta de la maquina";
        jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
        jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
      }
      break;
    }

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    //Serial.println(Json);
    request->send(200, "application/json", Json);
  }else{

    jsonDocument["IsSuccess"] = false;
    jsonDocument["Fecha_Hora"] = DataTime;
    if(Variables_globales.Get_Variable_Global(Status_AFT_Machine))
      jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
    else
      jsonDocument["Key"] = nullptr;
    jsonDocument["Trans_ID"] = Cashless.Get_Trans_ID_Int();
    jsonDocument["Desc"] = "No compatible con el tipo de maquina";
    jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
    jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    //Serial.println(Json);
    request->send(200, "application/json", Json);
  }

  });

  Server_API.on("/Info_Cashless",HTTP_GET, [](AsyncWebServerRequest *request){

  StaticJsonDocument<800> jsonDocument;
  jsonDocument.clear();
  
  String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
  String Output;
  String AFT_Game_Lock = "00";
  String Asset_Number = "0000";
  String Available_transfer = "00";
  String Host_Cashout_Status = "00";
  String AFT_Status = "00";
  String Max_History_index = "00";
  String Current_Casheable = "0000000000";
  String Current_Restricted = "0000000000";
  String Current_Nonrestricted = "0000000000";
  String Transfer_Limit = "0000000000";
  String Restricted_Expiration = "00000000";
  String Restricted_Pool_ID = "0000";
  if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)<4)
  {

    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 0:
      jsonDocument["Tipo_Maq"] = "AFT";
      break;
    case 1:
      jsonDocument["Tipo_Maq"] = "AFT";
      break;
    case 2:
      jsonDocument["Tipo_Maq"] = "EFT";
      break;
    case 3:
      jsonDocument["Tipo_Maq"] = "AFT";
      break;

    default:
      jsonDocument["Tipo_Maq"] = "";
      break;
    }
    Consulta_Info_Cashless();
    delay(350);
    

    AFT_Game_Lock = String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[7])) + String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[7]));
    Asset_Number = String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[3])) +
                   String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[3])) +
                   String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[4])) +
                   String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[4])) +
                   String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[5])) +
                   String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[5])) +
                   String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[6])) +
                   String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[6]));

    // Available Transfers
    Available_transfer =
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[8])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[8]));
    // Host Cashout Status
    Host_Cashout_Status =
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[9])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[9]));
    // AFT Status
    AFT_Status =
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[10])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[10]));
    // Max History Index
    Max_History_index =
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[11])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[11]));
    // Current Cashable
    Current_Casheable =
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[12])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[12])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[13])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[13])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[14])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[14])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[15])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[15])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[16])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[16]));
    // Current Restricted
    Current_Restricted =
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[17])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[17])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[18])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[18])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[19])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[19])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[20])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[20])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[21])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[21]));

    // Current Nonrestricted
    Current_Nonrestricted =
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[22])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[22])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[23])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[23])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[24])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[24])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[25])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[25])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[26])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[26]));
    // Transfer limit
    Transfer_Limit =
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[27])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[27])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[28])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[28])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[29])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[29])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[30])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[30])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[31])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[31]));
    // Restricted Expiration
    Restricted_Expiration =
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[32])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[32])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[33])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[33])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[34])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[34])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[35])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[35]));
    // Restricted Pool_ID
    Restricted_Pool_ID =
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[36])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[36])) +
        String(Hex_Ascci_HIGH(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[37])) +
        String(Hex_Ascci_LOW(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[37]));

    if (!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
    {
      jsonDocument["IsSuccess"] = false;
      jsonDocument["AFT_Game_Lock"] = AFT_Game_Lock;
      jsonDocument["Asset_Number"] = Asset_Number;
      jsonDocument["Available_Trans"] = Available_transfer;
      jsonDocument["Host_Cashout"] = Host_Cashout_Status;
      jsonDocument["AFT_Status"] = AFT_Status;
      jsonDocument["Max_History_Index"] = Max_History_index;
      jsonDocument["Current_Casheable"] = Current_Casheable;
      jsonDocument["Current_Restricted"] = Current_Restricted;
      jsonDocument["Current_Nonrestricted"] = Current_Nonrestricted;
      jsonDocument["Transfer_Limit"] = Transfer_Limit;
      jsonDocument["Restricted_Expiration"] = Restricted_Expiration;
      jsonDocument["Restricted_Pool_ID"] = Restricted_Pool_ID;
      if (Variables_globales.Get_Variable_Global(Status_AFT_Machine))
        jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
      else
        jsonDocument["Key"] = nullptr;
      jsonDocument["Trans_ID"] = Cashless.Get_Trans_ID_Int();
      jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
      jsonDocument["Desc"] = "No Hay Comunicacion con la maquina";
      jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
      }
      else
      {
        jsonDocument["IsSuccess"] = true;
        jsonDocument["AFT_Game_Lock"] = AFT_Game_Lock;
        jsonDocument["Asset_Number"] = Asset_Number;
        jsonDocument["Available_Trans"] = Available_transfer;
        jsonDocument["Host_Cashout"] = Host_Cashout_Status;
        jsonDocument["AFT_Status"] = AFT_Status;
        jsonDocument["Max_History_Index"] = Max_History_index;
        jsonDocument["Current_Casheable"] = Current_Casheable;
        jsonDocument["Current_Restricted"] = Current_Restricted;
        jsonDocument["Current_Nonrestricted"] = Current_Nonrestricted;
        jsonDocument["Transfer_Limit"] = Transfer_Limit;
        jsonDocument["Restricted_Expiration"] = Restricted_Expiration;
        jsonDocument["Restricted_Pool_ID"] = Restricted_Pool_ID;
        if (Variables_globales.Get_Variable_Global(Status_AFT_Machine))
          jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
        else
          jsonDocument["Key"] = nullptr;
        jsonDocument["Trans_ID"]=Cashless.Get_Trans_ID_Int();
        jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
        jsonDocument["Desc"] = "Informacion Generada con exito!";
        jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
      }

  serializeJson(jsonDocument, Output); /* Serializa Data */
  //Serial.println(Output);
  request->send(200, "application/json", Output);
  }
  else
  {

    jsonDocument["IsSuccess"] = false;
    jsonDocument["AFT_Game_Lock"] = AFT_Game_Lock;
    jsonDocument["Asset_Number"] = Asset_Number;
    jsonDocument["Available_Trans"] = Available_transfer;
    jsonDocument["Host_Cashout"] = Host_Cashout_Status;
    jsonDocument["AFT_Status"] = AFT_Status;
    jsonDocument["Max_History_Index"] = Max_History_index;
    jsonDocument["Current_Casheable"] = Current_Casheable;
    jsonDocument["Current_Restricted"] = Current_Restricted;
    jsonDocument["Current_Nonrestricted"] = Current_Nonrestricted;
    jsonDocument["Transfer_Limit"] = Transfer_Limit;
    jsonDocument["Restricted_Expiration"] = Restricted_Expiration;
    jsonDocument["Restricted_Pool_ID"] = Restricted_Pool_ID;

    if(Variables_globales.Get_Variable_Global(Status_AFT_Machine))
      jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
    else
      jsonDocument["Key"] = nullptr;

    jsonDocument["Trans_ID"] = Cashless.Get_Trans_ID_Int();
    jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
    jsonDocument["Desc"] = "Tipo de maquina no compatible";
    jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);

    serializeJson(jsonDocument, Output); /* Serializa Data */
   // Serial.println(Output);
    request->send(200, "application/json", Output);
  }
    
  });

  Server_API.on("/Transferencias_Pendientes", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/transacciones.txt", "text/plain", true);
    response->addHeader("Txt", "Transferencias Pendientes");
    request->send(response);
  });

   Server_API.on("/Solicitud_Contadores_Cashless",HTTP_GET, [](AsyncWebServerRequest *request)
  {

    
    DynamicJsonDocument jsonDocument(800);
    jsonDocument.clear();
    char Current_IP[4];
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 0:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 1:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 2:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;
    case 3:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    
    default:
        jsonDocument["Tipo_Maq"] = "";
        break;
    }
    if(!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
      jsonDocument["IsSuccess"] = false;
    else
      jsonDocument["IsSuccess"] = true;

    jsonDocument["Entrada_Canjeable"] = contadores.Get_Contadores_Int(Casheable_In);
    jsonDocument["Entrada_Restringida"] = contadores.Get_Contadores_Int(Casheable_Restricted_In);
    jsonDocument["Entrada_No_Restringida"] = contadores.Get_Contadores_Int(Casheable_NONrestricted_In);
    

    jsonDocument["Salida_Canjeable"] = contadores.Get_Contadores_Int(Casheable_Out);
    jsonDocument["Salida_Restringida"] = contadores.Get_Contadores_Int(Casheable_Restricted_Out);
    jsonDocument["Salida_No_Restringida"] = contadores.Get_Contadores_Int(Casheable_NONrestricted_Out);

    jsonDocument["Ip"] = IP_toString_(Current_IP);
    jsonDocument["MAC"] = WiFi.macAddress();
    jsonDocument["Id_Maquina"] = 0;
    jsonDocument["Fecha_Hora"] = DataTime;
    
    if(Variables_globales.Get_Variable_Global(Status_AFT_Machine))
      jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
    else
      jsonDocument["Key"] = nullptr;

    if(jsonDocument["IsSuccess"]==true)
      jsonDocument[ "Message"] = "Contadores actualizados correctamente";
    else
      jsonDocument[ "Message"] = "No hay comunicación con la MET";

    String Output;
    serializeJson(jsonDocument, Output); /* Serializa Data */
    //Serial.println(Output);
    request->send(200, "application/json", Output);
  });

  // Server_API.on("/log", HTTP_GET, [](AsyncWebServerRequest *request){
  
  // if (SD.exists(archivo))
  // {
  //   AsyncWebServerResponse *response = request->beginResponse(SD, archivo, "text/plain", true);
  //   response->addHeader("Server", "ESP Async Web Server");
  //   request->send(response);
  // }else{
  //   request->send(404, "text/plain", "No existe el archivo");
  // }
  
  // });


  // Server_API.on("/Token", HTTP_GET, [](AsyncWebServerRequest *request){

  //   StaticJsonDocument<500> jsonDocument;
  //   jsonDocument.clear();
  //   String Token =New_Token();

    
  //   int Http_Code;
  //   String User = request->getParam("Gmaster")->value();
  //   String Passw = request->getParam("Password")->value();

  //   if (User == "jose" && Passw == "123")
  //   {
  //     if (Token != "")
  //     {
  //       jsonDocument["IsSuccess"] = true;
  //       jsonDocument["Access_Token"] = Token;
  //       jsonDocument["Type"] = "JWT Random";
  //     }
  //     else
  //     {
  //       jsonDocument["IsSuccess"] = false;
  //       jsonDocument["Access_Token"] = nullptr;
  //       jsonDocument["Type"] = nullptr;
  //     }
  //   }else{
  //     jsonDocument["IsSuccess"] = false;
  //     jsonDocument["Access_Token"] = nullptr;
  //     jsonDocument["Type"] = nullptr;
  //   }

  //   String Output;
  //   serializeJson(jsonDocument, Output); /* Serializa Data */
  //   request->send(200, "application/json", Output);
  
  // });




  // Server_API.on("/log", HTTP_GET, [](AsyncWebServerRequest *request)
  //           {
  //   if (request->hasParam("file")) {
  //     String fileName = request->getParam("file")->value();
  //     if (SD.exists("/" + fileName)) {
  //       AsyncWebServerResponse *response = request->beginResponse(SD, "/" + fileName, "text/plain", true);
  //       response->addHeader("Server", "ESP Async Web Server");
  //       request->send(response);
  //     } else {
  //       request->send(404, "text/plain", "File Not Found");
  //     }
  //   } else {
  //     request->send(400, "text/plain", "Bad Request");
  //   } });

  // Server_API.on("/delete-log", HTTP_GET, [](AsyncWebServerRequest *request) {

  //     Serial.println(archivo);
  //   if (SPIFFS.exists(archivo)) {
  //     SPIFFS.remove(archivo);
  //     request->send(200, "text/plain", "LOG ELIMINADO CON EXITO");

  //     File file = SPIFFS.open(archivo, "w");
  //     if (file)
  //     {
  //       file.close();
  //     }
  //   } else {
  //     request->send(200, "text/plain", "No existe el archivo");
  //   }
  // });

 

  /*********************************************************************************************************/
  /**********************************************CONFIG TITO************************************************/
  /*********************************************************************************************************/
  Server_API.addHandler(new AsyncCallbackJsonWebHandler("/Configura_Ticket", [](AsyncWebServerRequest* request, JsonVariant& json) {
    


    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();


    if (!json.is<JsonObject>()) {


      jsonDocument["IsSuccess"] = false;

      jsonDocument["Location"] = nullptr;
      jsonDocument["Adress_1"] = nullptr;
      jsonDocument["Adress_2"] = nullptr;
      jsonDocument["Restricted_Ticket"] = nullptr;
      jsonDocument["Debit_Ticket_Title"] = nullptr;
      jsonDocument[ "Message"]="json no identificado";

      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);
      return;
    }
    auto&& data = json.as<JsonObject>();

    String  Location= data["Location"].as<String>();
    String  Adress_1= data["Adress_1"].as<String>();
    String  Adress_2= data["Adress_2"].as<String>();
    String  Restricted_Ticket_Title= data["Restricted_Ticket"].as<String>();
    String  Debit_Ticket_Title= data["Debit_Ticket_Title"].as<String>();


    if(Location.length()>40||Adress_1.length()>40||Adress_2.length()>40||Restricted_Ticket_Title.length()>16||Debit_Ticket_Title.length()>16)
    {
      jsonDocument["IsSuccess"] = false;

      jsonDocument["Location"] = Location;
      jsonDocument["Adress_1"] = Adress_1;
      jsonDocument["Adress_2"] = Adress_2;
      jsonDocument["Restricted_Ticket"] = Restricted_Ticket_Title;
      jsonDocument["Debit_Ticket_Title"] = Debit_Ticket_Title;
      jsonDocument[ "Message"]="Limite de texto excedido";

      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);
    }else{

      if (Cashless.Set_Parameter_Print_Ticket(Location, Adress_1, Adress_2, Restricted_Ticket_Title, Debit_Ticket_Title))
      {
        esp_task_wdt_init(1000000, true);
        esp_task_wdt_add(NULL);
        unsigned long Timout_Break;
        int Stop_Transaccion = 8000; // Tiempo de espera en milisegundos (15 Seg MAX)
        Timout_Break = millis();

        while ((Buffer_Cashless.Get_Buffer_TITO_7C()[1] == 0xAA && millis() - Timout_Break < Stop_Transaccion))
        {
          esp_task_wdt_reset();
          Serial.println("Esperando respuesta.....");
          vTaskDelay(10);
        }

        int Comando = Buffer_Cashless.Get_Buffer_TITO_7C()[1];
        int Ticket_Flag = Buffer_Cashless.Get_Buffer_TITO_7C()[2];
        switch (Ticket_Flag)
        {
        case 0x00:
          jsonDocument["IsSuccess"] = false;
          jsonDocument[ "Message"]="No fue Posible realizar la configuracion";
          break;
        case 0x01:
          jsonDocument["IsSuccess"] = true;
           jsonDocument[ "Message"]="Configuracion aplicada con exito";
          break;

        default:
          jsonDocument["IsSuccess"] = false;
          jsonDocument[ "Message"]="No fue Posible realizar la configuracion";
          break;
        }

        jsonDocument["Location"] = Location;
        jsonDocument["Adress_1"] = Adress_1;
        jsonDocument["Adress_2"] = Adress_2;
        jsonDocument["Restricted_Ticket"] = Restricted_Ticket_Title;
        jsonDocument["Debit_Ticket_Title"] = Debit_Ticket_Title;
      }
      else
      {
        jsonDocument["IsSuccess"] = false;
        jsonDocument[ "Message"]="No fue Posible realizar la configuracion";
        jsonDocument["Location"] = Location;
        jsonDocument["Adress_1"] = Adress_1;
        jsonDocument["Adress_2"] = Adress_2;
        jsonDocument["Restricted_Ticket"] = Restricted_Ticket_Title;
        jsonDocument["Debit_Ticket_Title"] = Debit_Ticket_Title;
      }

      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);
    }

  }));

  Server_API.on("/Evento_57_Tito_Deshabilitado",HTTP_GET, [](AsyncWebServerRequest *request){
    

    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());


    NVS.begin("Config_ESP32", false);
    NVS.putBool("Only_Tito",false);
    bool Test=NVS.getBool("Only_Tito",false);

    if(Test)
      Variables_globales.Set_Variable_Global(Descarga_Solo_Tito, true);
    else
      Variables_globales.Set_Variable_Global(Descarga_Solo_Tito, false);


    if(!Variables_globales.Get_Variable_Global(Descarga_Solo_Tito))
      jsonDocument["IsSuccess"] = true;
    else
      jsonDocument["IsSuccess"] = false;

    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
    jsonDocument["Desc"] = "Evento 57 Deshabilitado";
    jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 0:
      jsonDocument["Tipo_Maq"] = "AFT";
      break;
    case 1:
      jsonDocument["Tipo_Maq"] = "AFT";
      break;
    case 2:
      jsonDocument["Tipo_Maq"] = "EFT";
      break;
    case 3:
      jsonDocument["Tipo_Maq"] = "AFT";
      break;

    default:
      jsonDocument["Tipo_Maq"] = "";
      break;
    }
    jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
    jsonDocument["Cobro_Tito"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Tito);
    NVS.end();

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
   // Serial.println(Json);
    request->send(200, "application/json", Json);
  });

  Server_API.on("/Evento_57_Tito_Habilitado",HTTP_GET, [](AsyncWebServerRequest *request){
    
    

    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)<4)
    {
      StaticJsonDocument<500> jsonDocument;
      jsonDocument.clear();
      String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());


      NVS.begin("Config_ESP32", false);
      NVS.putBool("Only_Tito",true);
      bool Test=NVS.getBool("Only_Tito",false);

      if(Test)
        Variables_globales.Set_Variable_Global(Descarga_Solo_Tito, true);
      else
        Variables_globales.Set_Variable_Global(Descarga_Solo_Tito, false);


      if(Variables_globales.Get_Variable_Global(Descarga_Solo_Tito))
        jsonDocument["IsSuccess"] = true;
      else
        jsonDocument["IsSuccess"] = false;

      jsonDocument["Fecha_Hora"] = DataTime;
      jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
      jsonDocument["Desc"] = "Evento 57 Tito Habilitado";
      jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
      switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
      {
      case 0:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
      case 1:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
      case 2:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;
      case 3:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;

      default:
        jsonDocument["Tipo_Maq"] = "";
        break;
      }
      jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
      jsonDocument["Cobro_Tito"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Tito);
      NVS.end();

      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
    // Serial.println(Json);
      request->send(200, "application/json", Json);
    }else{

      StaticJsonDocument<500> jsonDocument;
      jsonDocument.clear();
      String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());

      Variables_globales.Set_Variable_Global(Descarga_Solo_Tito, false);


      if(Variables_globales.Get_Variable_Global(Descarga_Solo_Tito))
        jsonDocument["IsSuccess"] = true;
      else
        jsonDocument["IsSuccess"] = false;

      jsonDocument["Fecha_Hora"] = DataTime;
      jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
      jsonDocument["Desc"] = "Tipo de maquina no compatible";
      jsonDocument["Key"] = nullptr;
      switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
      {
      case 0:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
      case 1:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
      case 2:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;
      case 3:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;

      default:
        jsonDocument["Tipo_Maq"] = "";
        break;
      }
      jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
      jsonDocument["Cobro_Tito"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Tito);
      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
    // Serial.println(Json);
      request->send(200, "application/json", Json);

      }

   
  });

  Server_API.on("/Solicitud_Contadores_Tito",HTTP_GET, [](AsyncWebServerRequest *request)
  {
    DynamicJsonDocument jsonDocument(800);
    jsonDocument.clear();
    char Current_IP[4];
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 0:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 1:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 2:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;
    case 3:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    
    default:
        jsonDocument["Tipo_Maq"] = "";
        break;
    }
    if(!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
      jsonDocument["IsSuccess"] = false;
    else
      jsonDocument["IsSuccess"] = true;

    jsonDocument["Entrada_Ticket"] = contadores.Get_Contadores_Int(Ticket_In);
    jsonDocument["Salida_Ticket"] = contadores.Get_Contadores_Int(Ticket_Out);
    
    jsonDocument["Ip"] = IP_toString_(Current_IP);
    jsonDocument["MAC"] = WiFi.macAddress();
    jsonDocument["Id_Maquina"] = 0;
    jsonDocument["Fecha_Hora"] = DataTime;
    
    if(Variables_globales.Get_Variable_Global(Status_AFT_Machine))
      jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
    else
      jsonDocument["Key"] = nullptr;

    if(jsonDocument["IsSuccess"]==true)
      jsonDocument[ "Message"] = "Contadores actualizados correctamente";
    else
      jsonDocument[ "Message"] = "No hay comunicación con la MET";

    String Output;
    serializeJson(jsonDocument, Output); /* Serializa Data */
    //Serial.println(Output);
    request->send(200, "application/json", Output);
  });

  Server_API.on("/Habilitar_Tito",HTTP_GET, [](AsyncWebServerRequest *request){
    
    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)<4)
    {
      NVS.begin("Config_ESP32", false);
      NVS.putBool("Enable_Tito",true);
      bool Test=NVS.getBool("Enable_Tito",false);
      Variables_globales.Set_Variable_Global(Enable_Tito_Ticket,Test);
      NVS.end();

      if(Test && Variables_globales.Get_Variable_Global(Enable_Tito_Ticket))
      {
        
        String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());

        StaticJsonDocument<500> jsonDocument;
        jsonDocument.clear();
        jsonDocument["IsSuccess"] = true;
        jsonDocument["Fecha_Hora"] = DataTime;
        jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
        jsonDocument["Desc"] = "Tito habilitado con exito";
        jsonDocument["Key"]=Buffer_Cashless.Get_Key_Register_AFT_String();
        
        switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
        {
        case 0:
            jsonDocument["Tipo_Maq"] = "AFT";
            break;
        case 1:
            jsonDocument["Tipo_Maq"] = "AFT";
            break;
        case 2:
            jsonDocument["Tipo_Maq"] = "EFT";
            break;
        case 3:
            jsonDocument["Tipo_Maq"] = "AFT";
            break;

        default:
            jsonDocument["Tipo_Maq"] = "";
            break;
        }
        jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
        jsonDocument["Cobro_Tito"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Tito);
        String Json;
        serializeJson(jsonDocument, Json); /* Serializa Data */
       // Serial.println(Json);
        request->send(200, "application/json", Json);
      }
      else
      {
        String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());

        StaticJsonDocument<200> jsonDocument;
        jsonDocument.clear();
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Fecha_Hora"] = DataTime;
        jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
        jsonDocument["Desc"] = "No fue posible habilitar Tito";
        jsonDocument["Key"]=Buffer_Cashless.Get_Key_Register_AFT_String();
        switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
        {
        case 0:
          jsonDocument["Tipo_Maq"] = "AFT";
          break;
        case 1:
          jsonDocument["Tipo_Maq"] = "AFT";
          break;
        case 2:
          jsonDocument["Tipo_Maq"] = "EFT";
          break;
        case 3:
          jsonDocument["Tipo_Maq"] = "AFT";
          break;
        default:
          jsonDocument["Tipo_Maq"] = "";
          break;
        }
        jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
        jsonDocument["Cobro_Tito"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Tito);
        String Json;
        serializeJson(jsonDocument, Json); /* Serializa Data */
       // Serial.println(Json);
        request->send(200, "application/json", Json);
      }
    }
    else
    {
      String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

      StaticJsonDocument<200> jsonDocument;
      jsonDocument.clear();
      jsonDocument["IsSuccess"] = false;
      jsonDocument["Fecha_Hora"] = DataTime;
      jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
      jsonDocument["Desc"] = "No compatible con el tipo de maquina";
      jsonDocument["Key"] = nullptr;
      switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
      {
      case 0:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
      case 1:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
      case 2:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;
      case 3:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;

      default:
        jsonDocument["Tipo_Maq"] = "";
        break;
      }
      jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
      jsonDocument["Cobro_Tito"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Tito);
      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
     // Serial.println(Json);
      request->send(200, "application/json", Json);
    }


  });

  Server_API.on("/Inhabilitar_Tito",HTTP_GET, [](AsyncWebServerRequest *request){
  
 

  if(!Variables_globales.Get_Variable_Global(Enable_Tito_Ticket))
  {

    

    

    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());

    StaticJsonDocument<200> jsonDocument;
    jsonDocument.clear();

    jsonDocument["IsSuccess"] = true;
    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
    jsonDocument["Desc"] = "Tito Inhabilitado con exito";
    jsonDocument["Key"]=Buffer_Cashless.Get_Key_Register_AFT_String();
    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 0:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 1:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 2:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;
    case 3:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    
    default:
        jsonDocument["Tipo_Maq"] = "";
        break;
    }
    jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
    jsonDocument["Cobro_Tito"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Tito);
    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);

  }else{
    NVS.begin("Config_ESP32", false);
    NVS.putBool("Enable_Tito", false);
    bool Test = NVS.getBool("Enable_Tito", false);
    Variables_globales.Set_Variable_Global(Enable_Tito_Ticket, Test);
    NVS.end();
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());

   if(!Test && !Variables_globales.Get_Variable_Global(Enable_Tito_Ticket))
   {

    StaticJsonDocument<200> jsonDocument;
    jsonDocument.clear();

    jsonDocument["IsSuccess"] = true;
    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
    jsonDocument["Desc"] = "Tito Inhabilitado con exito";
    jsonDocument["Key"]=Buffer_Cashless.Get_Key_Register_AFT_String();
    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 0:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 1:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 2:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;
    case 3:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;

    case 5:
        jsonDocument["Tipo_Maq"] = "AFT"; /* Eliminar*/
        break;
    
    default:
        jsonDocument["Tipo_Maq"] = "";
        break;
    }
    jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
    jsonDocument["Cobro_Tito"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Tito);
    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);
   }else{
     StaticJsonDocument<200> jsonDocument;
     jsonDocument.clear();

    jsonDocument["IsSuccess"] = false;
    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
    jsonDocument["Desc"] = "No fue posible Inhabilitar Tito";
    jsonDocument["Key"]=Buffer_Cashless.Get_Key_Register_AFT_String();
    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 0:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 1:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 2:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;
    case 3:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;

    // case 5:
    //     jsonDocument["Tipo_Maq"] = "AFT"; /* Eliminar*/
    //     break;
    
    default:
        jsonDocument["Tipo_Maq"] = "";
        break;
    }
    jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
    jsonDocument["Cobro_Tito"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Tito);
    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);
   }
  }  
  });

  Server_API.on("/Reset_Firmware",HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<200> jsonDocument;
    jsonDocument.clear();


    jsonDocument["IsSuccess"] = true;
    jsonDocument["Desc"] = "Reset Globus IM ESP32 procesado";
    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);
    delay(1000);
    ESP.restart();
  });


  // Server_API.on("/Contadores_Accounting",HTTP_GET,[](AsyncWebServerRequest *request)
  // {
  //   StaticJsonDocument<1024> jsonDocument;
  //   jsonDocument.clear();
    
  //   String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());

  //   if(Variables_globales.Get_Variable_Global(Comunicacion_Maq))
  //   {
  //     jsonDocument["Cancel_Credit"] = contadores.Get_Contadores_Int(Total_Cancel_Credit);
  //     jsonDocument["Coin_In"] = contadores.Get_Contadores_Int(Coin_In);
  //     jsonDocument["Coin_Out"] = contadores.Get_Contadores_Int(Coin_Out);
  //     jsonDocument["Jackpot"] = contadores.Get_Contadores_Int(Jackpot);
  //     jsonDocument["Total_Drop"] = contadores.Get_Contadores_Int(Total_Drop);
  //     jsonDocument["Cancel_Credit_Hand_Pay"] = contadores.Get_Contadores_Int(Cancel_Credit_Hand_Pay);
  //     jsonDocument["Bill_Amount"] = contadores.Get_Contadores_Int(Bill_Amount);
  //     jsonDocument["Games_Played"] = contadores.Get_Contadores_Int(Games_Played);
  //     jsonDocument["Physical_Coin_In"] = contadores.Get_Contadores_Int(Physical_Coin_In); 
  //     jsonDocument["Physical_Coin_Out"] = contadores.Get_Contadores_Int(Physical_Coin_Out); 
  //     jsonDocument["Total_Coin_Drop"] = contadores.Get_Contadores_Int(Total_Coin_Drop); 
  //     jsonDocument["Machine_Paid_Progresive_Payout"] = contadores.Get_Contadores_Int(Machine_Paid_Progresive_Payout);
  //     jsonDocument["Machine_Paid_External_Bonus_Payout"] = contadores.Get_Contadores_Int(Machine_Paid_External_Bonus_Payout); 
  //     jsonDocument["Attendant_Paid_Progresive_Payout"] = contadores.Get_Contadores_Int(Attendant_Paid_Progresive_Payout);
  //     jsonDocument["Attendant_Paid_External_Bonus_Payout"] = contadores.Get_Contadores_Int(Attendant_Paid_External_Bonus_Payout); 
  //     jsonDocument["Attendant_Paid_External_Bonus_Payout"] = contadores.Get_Contadores_Int(Attendant_Paid_Progresive_Payout);
  //     jsonDocument["Current_Credits"] = contadores.Get_Contadores_Int(Current_Credits);
  //     jsonDocument["Door_Open"] = contadores.Get_Contadores_Int(Door_Open);
  //     jsonDocument["Games_Since_Last_Power_Up"] = contadores.Get_Contadores_Int(Games_Since_Last_Power_Up);
  //     // jsonDocument["Casheable_In"] = contadores.Get_Contadores_Int(Casheable_In);
  //     // jsonDocument["Casheable_Restricted_In"] = contadores.Get_Contadores_Int(Casheable_Restricted_In);
  //     // jsonDocument["Casheable_NONrestricted_In"] = contadores.Get_Contadores_Int(Casheable_NONrestricted_In);

  //     // jsonDocument["Casheable_Out"] = contadores.Get_Contadores_Int(Casheable_Out);
  //     // jsonDocument["Casheable_Restricted_Out"] = contadores.Get_Contadores_Int(Casheable_Restricted_Out);
  //     // jsonDocument["Casheable_NONrestricted_Out"] = contadores.Get_Contadores_Int(Casheable_NONrestricted_Out);

  //     String Output;
  //     serializeJson(jsonDocument, Output); /* Serializa Data */
  //     request->send(200, "application/json", Output);
  //   }
  // });
  
  Server_API.begin();
  return true;
}



bool Transsaccion_Cashless::Set_Credit_To_Load(char res[])
{
  unsigned char Temp_1, Temp_2;

  // CREDITOS CASHABLES
  Temp_1 = Conve_Ascii_To_Hex_LL(&res[22]);
  Temp_2 = Conve_Ascii_To_Hex_HH(&res[21]);
  Credit_To_Load[4] = (Temp_2 | Temp_1);
  //--------------------------------------------------------------------------
  Temp_1 = Conve_Ascii_To_Hex_LL(&res[20]);
  Temp_2 = Conve_Ascii_To_Hex_HH(&res[19]);
  Credit_To_Load[3] = (Temp_2 | Temp_1);
  //--------------------------------------------------------------------------
  Temp_1 = Conve_Ascii_To_Hex_LL(&res[18]);
  Temp_2 = Conve_Ascii_To_Hex_HH(&res[17]);
  Credit_To_Load[2] = (Temp_2 | Temp_1);
  //--------------------------------------------------------------------------
  Temp_1 = Conve_Ascii_To_Hex_LL(&res[16]);
  Temp_2 = Conve_Ascii_To_Hex_HH(&res[15]);
  Credit_To_Load[1] = (Temp_2 | Temp_1);
  //--------------------------------------------------------------------------
  Temp_1 = Conve_Ascii_To_Hex_LL(&res[14]);
  Temp_2 = Conve_Ascii_To_Hex_HH(&res[13]);
  Credit_To_Load[0] = (Temp_2 | Temp_1);

  // CREDITOS RESTRINGIDOS
  Temp_1 = Conve_Ascii_To_Hex_LL(&res[33]);
  Temp_2 = Conve_Ascii_To_Hex_HH(&res[32]);
  Credit_To_Load[9] = (Temp_2 | Temp_1);
  //--------------------------------------------------------------------------
  Temp_1 = Conve_Ascii_To_Hex_LL(&res[31]);
  Temp_2 = Conve_Ascii_To_Hex_HH(&res[30]);
  Credit_To_Load[8] = (Temp_2 | Temp_1);
  //--------------------------------------------------------------------------
  Temp_1 = Conve_Ascii_To_Hex_LL(&res[29]);
  Temp_2 = Conve_Ascii_To_Hex_HH(&res[28]);
  Credit_To_Load[7] = (Temp_2 | Temp_1);
  //--------------------------------------------------------------------------
  Temp_1 = Conve_Ascii_To_Hex_LL(&res[27]);
  Temp_2 = Conve_Ascii_To_Hex_HH(&res[26]);
  Credit_To_Load[6] = (Temp_2 | Temp_1);
  //--------------------------------------------------------------------------
  Temp_1 = Conve_Ascii_To_Hex_LL(&res[25]);
  Temp_2 = Conve_Ascii_To_Hex_HH(&res[24]);
  Credit_To_Load[5] = (Temp_2 | Temp_1);
  //--------------------------------------------------------------------------

  // CREDITOS NO RESTRINGIDOS
  Temp_1 = Conve_Ascii_To_Hex_LL(&res[44]);
  Temp_2 = Conve_Ascii_To_Hex_HH(&res[43]);
  Credit_To_Load[14] = (Temp_2 | Temp_1);
  //--------------------------------------------------------------------------
  Temp_1 = Conve_Ascii_To_Hex_LL(&res[42]);
  Temp_2 = Conve_Ascii_To_Hex_HH(&res[41]);
  Credit_To_Load[13] = (Temp_2 | Temp_1);
  //--------------------------------------------------------------------------
  Temp_1 = Conve_Ascii_To_Hex_LL(&res[40]);
  Temp_2 = Conve_Ascii_To_Hex_HH(&res[39]);
  Credit_To_Load[12] = (Temp_2 | Temp_1);
  //--------------------------------------------------------------------------
  Temp_1 = Conve_Ascii_To_Hex_LL(&res[38]);
  Temp_2 = Conve_Ascii_To_Hex_HH(&res[37]);
  Credit_To_Load[11] = (Temp_2 | Temp_1);
  //--------------------------------------------------------------------------
  Temp_1 = Conve_Ascii_To_Hex_LL(&res[36]);
  Temp_2 = Conve_Ascii_To_Hex_HH(&res[35]);
  Credit_To_Load[10] = (Temp_2 | Temp_1);
  //--------------------------------------------------------------------------

  // PUNTOS ACUMULADOS CLIENTE
  Temp_1 = Conve_Ascii_To_Hex_LL(&res[53]);
  Temp_2 = Conve_Ascii_To_Hex_HH(&res[52]);
  Credit_To_Load[18] = (Temp_2 | Temp_1);
  //--------------------------------------------------------------------------
  Temp_1 = Conve_Ascii_To_Hex_LL(&res[51]);
  Temp_2 = Conve_Ascii_To_Hex_HH(&res[50]);
  Credit_To_Load[17] = (Temp_2 | Temp_1);
  //--------------------------------------------------------------------------
  Temp_1 = Conve_Ascii_To_Hex_LL(&res[49]);
  Temp_2 = Conve_Ascii_To_Hex_HH(&res[48]);
  Credit_To_Load[16] = (Temp_2 | Temp_1);
  //--------------------------------------------------------------------------
  Temp_1 = Conve_Ascii_To_Hex_LL(&res[47]);
  Temp_2 = Conve_Ascii_To_Hex_HH(&res[46]);
  Credit_To_Load[15] = (Temp_2 | Temp_1);
  //--------------------------------------------------------------------------

  return true;
}

unsigned char decimalToBCD(unsigned char digit) {
    return ((digit / 10) << 4) | (digit % 10);
}

void integerToBCD(int number, char bcdDigits[])
{
  // Asegurarse de que solo procesamos hasta 5 dígitos
  for (int i = 0; i < 5; ++i)
  {
    bcdDigits[i] = decimalToBCD(number % 10);
    number /= 10;
  }
}

uint32_t Transsaccion_Cashless::BCDtoUint32(char Credit_To_Load[], int Select)
{
  char str[11];
  uint32_t Credit_Cashables=0;

  switch (Select)
  {
  case CASHABLES:
    
    for (int i = 0; i < 5; i++)
    {
      str[i * 2] = (Credit_To_Load[i] >> 4) + '0';
      str[i * 2 + 1] = (Credit_To_Load[i] & 0x0F) + '0';
    }
    str[10] = '\0'; // Asegurarse de que la cadena termine con un carácter nulo

    
    sscanf(str, "%u", &Credit_Cashables);
    return Credit_Cashables;
    break;

  case RESTRICTED:
    for (int i = 0; i < 5; i++)
    {
      str[i * 2] = (Credit_To_Load[5 + i] >> 4) + '0';
      str[i * 2 + 1] = (Credit_To_Load[5 + i] & 0x0F) + '0';
    }
    str[10] = '\0'; // Asegurarse de que la cadena termine con un carácter nulo

    sscanf(str, "%u", &Credit_Cashables);
    return Credit_Cashables;
    break;

  case NON_RESTRICTED:
    for (int i = 0; i < 5; i++)
    {
      str[i * 2] = (Credit_To_Load[10 + i] >> 4) + '0';
      str[i * 2 + 1] = (Credit_To_Load[10 + i] & 0x0F) + '0';
    }
    str[10] = '\0'; // Asegurarse de que la cadena termine con un carácter nulo
    sscanf(str, "%u", &Credit_Cashables);
    return Credit_Cashables;
    break;

  default:
    return Credit_Cashables;
    break;
  }
}

uint32_t Transsaccion_Cashless::BCDtoUint32_Pos(char Credit_To_Load[], int Select,int Inicial_Index)
{
  char str[11];
  uint32_t Credit_Cashables=0;

  switch (Select)
  {
  case CASHABLES:
    
    for (int i = 0; i < 5; i++)
    {
      str[i * 2] = (Credit_To_Load[Inicial_Index+i] >> 4) + '0';
      str[i * 2 + 1] = (Credit_To_Load[Inicial_Index+i] & 0x0F) + '0';
    }
    str[10] = '\0'; // Asegurarse de que la cadena termine con un carácter nulo

    
    sscanf(str, "%u", &Credit_Cashables);
    return Credit_Cashables;
    break;

  case RESTRICTED:
    for (int i = 0; i < 5; i++)
    {
      str[i * 2] = (Credit_To_Load[Inicial_Index + i] >> 4) + '0';
      str[i * 2 + 1] = (Credit_To_Load[Inicial_Index + i] & 0x0F) + '0';
    }
    str[10] = '\0'; // Asegurarse de que la cadena termine con un carácter nulo

    sscanf(str, "%u", &Credit_Cashables);
    return Credit_Cashables;
    break;

  case NON_RESTRICTED:
    for (int i = 0; i < 5; i++)
    {
      str[i * 2] = (Credit_To_Load[Inicial_Index + i] >> 4) + '0';
      str[i * 2 + 1] = (Credit_To_Load[Inicial_Index + i] & 0x0F) + '0';
    }
    str[10] = '\0'; // Asegurarse de que la cadena termine con un carácter nulo
    sscanf(str, "%u", &Credit_Cashables);
    return Credit_Cashables;
    break;

  default:
    return Credit_Cashables;
    break;
  }
}

bool Transsaccion_Cashless::Set_Amount_To_Load(uint32_t Credit_Cashables, uint32_t Credit_Restringidos, uint32_t Credit_No_Restringidos)
{

  Credit_Cashables=Credit_Cashables;
  Credit_Restringidos=Credit_Restringidos;
  Credit_No_Restringidos=Credit_No_Restringidos;

  for(int i = 0; i < 15; i++) {
    Credit_To_Load[i] = 0;
  }

  // Convertir el número a cadena con 10 caracteres, rellenando con ceros a la izquierda si es necesario
  char str[11];
  char str2[11];
  char str3[11];
  sprintf(str, "%010d", Credit_Cashables);
  sprintf(str2, "%010d", Credit_Restringidos);
  sprintf(str3, "%010d", Credit_No_Restringidos);
  
  // Convertir cada par de caracteres a un byte BCD Cashables
  for(int i = 0; i < 5; i++) {
    Credit_To_Load[i] = ((str[i*2] - '0') << 4) | (str[i*2 + 1] - '0');
  }

  // Convertir cada par de caracteres a un byte BCD Restricted
  for (int i = 0; i < 5; i++)
  {
    Credit_To_Load[5 + i] = ((str2[i * 2] - '0') << 4) | (str2[i * 2 + 1] - '0');
  }

  // Convertir cada par de caracteres a un byte BCD non restricted
  for (int i = 0; i < 5; i++)
  {
    Credit_To_Load[10 + i] = ((str3[i * 2] - '0') << 4) | (str3[i * 2 + 1] - '0');
  }

  // Serial.println(BCDtoUint32(Credit_To_Load,CASHABLES));
  // Serial.println(BCDtoUint32(Credit_To_Load,RESTRICTED));
  // Serial.println(BCDtoUint32(Credit_To_Load,NON_RESTRICTED));
  
  if(BCDtoUint32(Credit_To_Load,CASHABLES)==Credit_Cashables && BCDtoUint32(Credit_To_Load,RESTRICTED)==Credit_Restringidos &&BCDtoUint32(Credit_To_Load,NON_RESTRICTED)==Credit_No_Restringidos)  
    return true;
  else
    return false;
}

uint32_t Transsaccion_Cashless::Get_Credit_Number(int Type)
{

  uint32_t creditNumber = 0;
  uint32_t Default = 0;


  switch (Type)
  {
  case 1:
   

    // Reconstruir el número desde los dígitos BCD
    creditNumber += Credit_To_Load[4];             // Decenas de millar
    creditNumber += Credit_To_Load[3] * 10;        // Unidades de millar
    creditNumber += Credit_To_Load[2] * 100;       // Centenas
    creditNumber += Credit_To_Load[1] * 1000;      // Decenas
    creditNumber += Credit_To_Load[0] * 10000;     // Unidades

    return creditNumber;
    break;

  case 2:
    creditNumber += Credit_To_Load[5];         // Decenas de millar
    creditNumber += Credit_To_Load[6] * 10;    // Unidades de millar
    creditNumber += Credit_To_Load[7] * 100;   // Centenas
    creditNumber += Credit_To_Load[8] * 1000;  // Decenas
    creditNumber += Credit_To_Load[9] * 10000; // Unidades
      return creditNumber;
    break;

  case 3:
      creditNumber += Credit_To_Load[10];         // Decenas de millar
      creditNumber += Credit_To_Load[11] * 10;    // Unidades de millar
      creditNumber += Credit_To_Load[12] * 100;   // Centenas
      creditNumber += Credit_To_Load[13] * 1000;  // Decenas
      creditNumber += Credit_To_Load[14] * 10000; // Unidades
      return creditNumber;
    break;
  
  default:
    return Default;
    break;
  }
}






/*
CASHABLES    [0-4]
RESTRICTED  [5-9]
RESTRICTED [10-14]
*/
char* Transsaccion_Cashless::Get_Credit_To_Load(void)
{
  return Credit_To_Load;
}

bool Transsaccion_Cashless::Set_ACK_Bonus(char res[])
{
  memcpy(ACK_Bonus, res, sizeof(ACK_Bonus) / sizeof(ACK_Bonus[0]));
  return true;
}

char* Transsaccion_Cashless::Get_ACK_Bonus(void)
{
  return ACK_Bonus;
}

bool Transsaccion_Cashless::Delete_ACK_Bonus(void)
{
  ACK_Bonus[0]=0xFF;
  ACK_Bonus[1]=0xFF;
  ACK_Bonus[2]=0xFF;
  ACK_Bonus[3]=0xFF;
  ACK_Bonus[4]=0xFF;
  return true;
}

bool Buffer_RX_AFT::Set_RX_AFT_2(int Filtro_buffer)
{
  String MAC;
  char Mac_ESP[6];
  

  int Hora =RTC.getHour(true);
  int Minutes=RTC.getMinute();
  int Seg=RTC.getSecond();

  int Dia=RTC.getDay();
  int Mes=RTC.getMonth();
  int Year=RTC.getYear();
  Year=Year%100;
  Mes = Mes + 1;  // Sumar 1 al valor del mes

  byte byteHora = static_cast<byte>(Hora); // Almacenar la hora en un byte
  byte byteMinutos = static_cast<byte>(Minutes); // Almacenar la hora en un byte
  byte byteSegundos = static_cast<byte>(Seg); // Almacenar la hora en un byte
  byte byteDia = static_cast<byte>(Dia); // Almacenar la hora en un byte
  byte byteMes = static_cast<byte>(Mes); // Almacenar la hora en un byte
  byte byteYear = static_cast<byte>(Year); // Almacenar la hora en un byte

  switch (Filtro_buffer)
  {
  case 6:
     
      MAC=WiFi.macAddress();
      macStringToByteArray(MAC,Mac_ESP);
      /*Asset*/
      Buffer_registro_Mq_AFT[0]=0x01;
      Buffer_registro_Mq_AFT[1]=0x00;
      Buffer_registro_Mq_AFT[2]=0x00;
      Buffer_registro_Mq_AFT[3]=0x00;
      /*POS ID*/
      Buffer_registro_Mq_AFT[4]=0x01;
      Buffer_registro_Mq_AFT[5]=0x00;
      Buffer_registro_Mq_AFT[6]=0x00;
      Buffer_registro_Mq_AFT[7]=0x00;
      /* Dirrecicon MAC */
      Buffer_registro_Mq_AFT[8]=Mac_ESP[0];
      Buffer_registro_Mq_AFT[9]=Mac_ESP[1];
      Buffer_registro_Mq_AFT[10]=Mac_ESP[2];
      Buffer_registro_Mq_AFT[11]=Mac_ESP[3];
      Buffer_registro_Mq_AFT[12]=Mac_ESP[4];
      Buffer_registro_Mq_AFT[13]=Mac_ESP[5];

      /*--->Fecha Actual<---*/

      Buffer_registro_Mq_AFT[14]=byteHora; /* Hora */
      Buffer_registro_Mq_AFT[15]=byteMinutos; /* Minutos */
      Buffer_registro_Mq_AFT[16]=byteSegundos; /* Segundos */
      Buffer_registro_Mq_AFT[17]=byteDia; /* Dia */
      Buffer_registro_Mq_AFT[18]=byteMes; /* Mes */
      Buffer_registro_Mq_AFT[19]=byteYear; /* Año*/
      
      /* Guarda*/
      NVS.begin("Config_ESP32", false);
      NVS.putBytes("Reg_AFT", Buffer_registro_Mq_AFT, sizeof(Buffer_registro_Mq_AFT));
      NVS.end();
      return true;
      break;
  
  default:
    return false;
    break;
  }
}

bool Buffer_RX_AFT::Set_Buffer_Transfer_AFT(char Buffer[])
{
  memcpy(Buffer_RX_Transfer_AFT, Buffer, sizeof(Buffer_RX_Transfer_AFT) / sizeof(Buffer_RX_Transfer_AFT[0]));
  return true;
}



bool Buffer_RX_AFT::Init_Buffer_Transfer_AFT(bool Boolean)
{
  if (Boolean)
  {
    for (int i = 0; i < 128; i++)
    {
      Buffer_RX_Transfer_AFT[i]=0xAA;
    }
  }

  return true;
}

char* Buffer_RX_AFT::Get_Bufffer_Transfer_AFT(void)
{
 
  return Buffer_RX_Transfer_AFT;
}

void Transsaccion_Cashless::Registra_Maquina_Auto(void)
{
  String Json;
  TimeOut_Reg = millis();
  if (Variables_globales.Get_Variable_Global(Comunicacion_Maq) && !Variables_globales.Get_Variable_Global(Status_AFT_Machine) && Variables_globales.Get_Variable_Global(Enable_Cashless) && (TimeOut_Reg - TimeOut_Regf) >= Timout)
  {
    StaticJsonDocument<200> jsonDocument;
    jsonDocument.clear();
    String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour()) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

    int Status;
    

    Status = Registra_Machine();
    delay(300); /* Espera Por respuesta de la maquina */
    
    switch (Status)
    {
    case 1:
      Variables_globales.Set_Variable_Global(Status_AFT_Machine, true);
      jsonDocument["IsSuccess"] = true;
      jsonDocument["Fecha_Hora"] = DataTime;
      jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
      jsonDocument["Trans_ID"] = Cashless.Get_Trans_ID_Int();
      jsonDocument["Desc"] = "Maquina Registrada con Exito";
      jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
      
      serializeJson(jsonDocument, Json); /* Serializa Data */

      if(Transmite_Info_Registro(Json)) /* Intento 1 */
      //   Cashless.Set_Reintento_Registro(true);
      // else
      // {
      //   if(!Transmite_Info_Registro(Json)) /* Intento 2 */
      //     if(!Transmite_Info_Registro(Json)) /* Intento 3*/
      //       Cashless.Set_Reintento_Registro(false);
      //     else
      //       Cashless.Set_Reintento_Registro(true);
      //   else
      //     Cashless.Set_Reintento_Registro(true);
      // }

      Serial.println("Maquina Registrada con exito!");
      break;

    default:
      break;
    }

    TimeOut_Regf=TimeOut_Reg;
  }

  // if((TimeOut_Reg - TimeOut_Regf) >= Timout && Variables_globales.Get_Variable_Global(Status_AFT_Machine) &&Variables_globales.Get_Variable_Global(Enable_Cashless) && !Cashless.Get_Reintento_Registro())
  // {
  //   Transmite_Info_Registro(Json);
  //   TimeOut_Regf=TimeOut_Reg;
  // }
}

bool Transsaccion_Cashless ::Transmite_Info_Registro(String Json)
{

  bool Code = false;
  int httpCode;
  String fwurl = "http://192.168.5.204/Registro_Auto";

  WiFiClient client;
  HTTPClient https;
                     

  https.setTimeout(10000);
  if (https.begin(client, fwurl))
  {
    // https.addHeader("Authorization", "Bearer " + String(Access_Token_Api_Gmaster)); // Agrega el token de autorización
    https.addHeader("Content-Type", "application/json");
    httpCode = https.POST(Json);

    // Serial.println(httpCode);
    if (httpCode == HTTP_CODE_OK)
    {

      String Response = https.getString();
      StaticJsonDocument<500>
          doc,
          filter;
      DeserializationError error = deserializeJson(doc, Response);
      // Serial.println( Response);
      if (error)
      {
#ifdef Debug_HTTPS
        Serial.println("Error Json Contadores ");
#endif
      }
      else
      {
        bool IsSuccess = doc["IsSuccess"];

        if (IsSuccess)
          Code = true;
        else
          Code = false;
      }

      doc.clear();
    }
    else
    {

      Code = false;
    }
    https.end();

    // Serial.println(Code);
    return Code;
  }
  return Code;
}


void Transsaccion_Cashless ::Set_Reintento_Registro(bool Set)
{
  Estado_Tranmision_Auto_Registro=Set;
}

bool Transsaccion_Cashless ::Get_Reintento_Registro(void)
{
  return Estado_Tranmision_Auto_Registro;
}


/* EFT Data */
bool Buffer_RX_AFT::Set_Buffer_Transfer_EFT(char Buffer[])
{
  memcpy(Buffer_RX_Transfer_EFT, Buffer, sizeof(Buffer_RX_Transfer_EFT) / sizeof(Buffer_RX_Transfer_EFT[0]));
  return true;
}

char* Buffer_RX_AFT::Get_Buffer_Transfer_EFT(void)
{
  return Buffer_RX_Transfer_EFT;
}

bool Buffer_RX_AFT::Init_Buffer_Transfer_EFT(bool Boolean)
{
  if (Boolean)
  {
    for (int i = 0; i < 128; i++)
    {
      Buffer_RX_Transfer_EFT[i]=0xAA;
    }
  }

  return true;
}




bool Buffer_RX_AFT::Init_Buffer_TITO(void)
{

  for (int i = 0; i < 128; i++)
  {
    Buffer_Rx_TITO[i]=0xAA;
  }

  if(Buffer_Rx_TITO[0]==0xAA && Buffer_Rx_TITO[127]==0xAA)
    return true;
  else
    return false;
}


bool Buffer_RX_AFT::Set_Buffer_TITO(char Buffer_Machine[])
{
  memcpy(Buffer_Rx_TITO, Buffer_Machine, sizeof(Buffer_Rx_TITO) / sizeof(Buffer_Rx_TITO[0]));

  if(Buffer_Machine[0]==Buffer_Rx_TITO[0])
    return true;
  else
    return false;
}

char* Buffer_RX_AFT::Get_Buffer_TITO(void)
{
  return Buffer_Rx_TITO;
}


bool Buffer_RX_AFT::Init_Buffer_TITO_70(void)
{

  for (int i = 0; i < 128; i++)
  {
    Buffer_Rx_TITO_70[i]=0xAA;
  }

  if(Buffer_Rx_TITO_70[0]==0xAA && Buffer_Rx_TITO_70[127]==0xAA)
    return true;
  else
    return false;
}


bool Buffer_RX_AFT::Set_Buffer_TITO_70(char Buffer_Machine[])
{
  memcpy(Buffer_Rx_TITO_70, Buffer_Machine, sizeof(Buffer_Rx_TITO_70) / sizeof(Buffer_Rx_TITO_70[0]));

  if(Buffer_Machine[0]==Buffer_Rx_TITO_70[0])
    return true;
  else
    return false;
}

char* Buffer_RX_AFT::Get_Buffer_TITO_70(void)
{
  return Buffer_Rx_TITO_70;
}


bool Buffer_RX_AFT::Attend_Tito_Request(int Evento,bool Status_Machine,bool Transaction_Status,bool Enable_Tito)
{ 
  if(Evento==0x57 && Status_Machine && !Transaction_Status&& Enable_Tito) /* Requerimiento Ticket Out*/
  {
    Serial.println("Paso validacion 1");
    return true;
  }else
  {
    Serial.println("No Paso validacion 1");
    return false;
  }
}



bool Buffer_RX_AFT::Set_Buffer_TITO_58(char Buffer[])
{
   memcpy(Buffer_Rx_TITO_58,Buffer, sizeof(Buffer) / sizeof(Buffer[0]));

  if(Buffer[0]==Buffer_Rx_TITO_58[0])
    return true;
  else
    return false;
}


char *Buffer_RX_AFT::Get_Buffer_TITO_58(void)
{
  return Buffer_Rx_TITO_58;
}


bool Buffer_RX_AFT::Init_Buffer_TITO_58(void)
{
  for (int i = 0; i < 128; i++)
  {
    Buffer_Rx_TITO_58[i]=0xAA;
  }

  if(Buffer_Rx_TITO_58[0]==0xAA && Buffer_Rx_TITO_58[127]==0xAA)
    return true;
  else
    return false;
}

bool Buffer_RX_AFT::Set_Buffer_TITO_4D(char Buffer[])
{
  memcpy(Buffer_Rx_TITO_4D, Buffer, sizeof(Buffer_Rx_TITO_4D) / sizeof(Buffer_Rx_TITO_4D[0]));

  if (Buffer[0] == Buffer_Rx_TITO_4D[0])
    return true;
  else
    return false;
}

char *Buffer_RX_AFT::Get_Buffer_TITO_4D(void)
{
  return Buffer_Rx_TITO_4D;
}


bool Buffer_RX_AFT::Init_Buffer_TITO_4D(void)
{
  for (int i = 0; i < 128; i++)
  {
    Buffer_Rx_TITO_4D[i]=0xAA;
  }

  if(Buffer_Rx_TITO_4D[0]==0xAA && Buffer_Rx_TITO_4D[127]==0xAA)
    return true;
  else
    return false;
}


bool Buffer_RX_AFT::Init_Buffer_TITO_3D_3E(void)
{

  Buffer_Rx_TITO_3D=0xAA;

  if(Buffer_Rx_TITO_3D==0xAA)
    return true;
  else 
    return false;
}

bool Buffer_RX_AFT::Set_Buffer_TITO_3D_3E(char Buffer)
{
  
  Buffer_Rx_TITO_3D=Buffer;
  if(Buffer_Rx_TITO_3D==Buffer)
    return true;
  else
    return false;
}

char Buffer_RX_AFT::Get_Buffer_TITO_3D_3E(void)
{
  return Buffer_Rx_TITO_3D;
}





bool Buffer_RX_AFT::Init_Buffer_TITO_7C(void)
{

  for (int i = 0; i < 128; i++)
  {
    Buffer_Rx_TITO_7C[i]=0xAA;
  }

  if(Buffer_Rx_TITO_7C[0]==0xAA && Buffer_Rx_TITO_7C[127]==0xAA)
    return true;
  else
    return false;
}

bool Buffer_RX_AFT::Set_Buffer_TITO_7C(char Buffer[])
{
  
  memcpy(Buffer_Rx_TITO_7C, Buffer, sizeof(Buffer_Rx_TITO_7C) / sizeof(Buffer_Rx_TITO_7C[0]));

  if (Buffer[0] == Buffer_Rx_TITO_7C[0])
    return true;
  else
    return false;
}

char *Buffer_RX_AFT::Get_Buffer_TITO_7C(void)
{
  return Buffer_Rx_TITO_7C;
}