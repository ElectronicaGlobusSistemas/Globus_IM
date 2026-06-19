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
#include "AutoUpdate.h"

#include "SD.h"
#include "Pantalla_TFT.h"
#include "freertos/semphr.h"
#include "Memory_SD.h"
#include "Transacciones.h"

#include "ESP32FtpServer.h"
#include "mbedtls/sha1.h"
#include "TITO.h"



#define MAQUINA_EN_JUEGO_ 203
#define VERSION_YA_INSTALADA 204
#define NO_EXISTE_DISPOSITIVO 205
#define DISPOSITIVO_NO_CONECTADO 206
#define ERROR_VERSION 207
#define SIN_RESPUESTA 208
#define EXITOSO 200

#define UDATE_VIA_HTTP true
#define UPDATE_VIA_OTA false

extern DynamicJsonDocument Objeto_Transfer;
extern DynamicJsonDocument Objeto_Transfer_Download;
extern bool Solicitud_Carga_Cashless(void);


extern Pantalla_TFT DisplayTFT;


extern TransaccionCashless AFT;
#define Unlock_Machine  26
extern Configuracion_ESP32 Configuracion;
extern Contadores_SAS contadores; // Objeto contiene contadores maquina
extern Cashless_API Info_Cashless;

extern AutoUpdate UpdateOTA;
extern uint8_t Version_Firmware_[];
extern Preferences NVS;

extern bool Flag_Set_Ticket_Data;
extern bool Flaggg;
extern TaskHandle_t Task_Poker_Hopper;
using namespace std;
extern ESP32Time RTC; // Objeto contiene hora y fecha

extern FtpServer ftpSrv;           //  Objeto servidor FTP

unsigned long TimeOut_Reg=0;
unsigned long TimeOut_Regf=0;
unsigned long Timout=30000;
extern bool Formateo;
extern int Result_Formatt;
bool Consulta_Result_Formatt=true;

extern bool App;
extern const char* LogError;


extern bool Flag_Conexion_TFT;

extern const char* archivo;
extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales
extern Transsaccion_Cashless Cashless;
extern Buffer_RX_AFT Buffer_Cashless;
extern unsigned char Registra_Machine(void);
extern unsigned char Delete_Registro_Machine(void);
extern bool Consulta_Info_Cashless(void);
const unsigned char Tabla_Ascii_Data[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
extern bool Reset_HandPay(void);
extern bool Flag_Critial_Questions;
extern bool Extended_Ticket_Command;

bool Flag_Change_Counters_One=false;
bool Flag_Change_Counters_TWO=false;

bool Flag_Change_Counters_Response=false;
bool Flag_Change_Counters_Break=false;

bool Variable_Solicitud_Operador_Id=false;
bool Solicitud_Expiracion_Ticket=false;


bool volatile Permitir_Ultima_Actualizacion=false;

SemaphoreHandle_t MaquinaSemaphore = xSemaphoreCreateMutex();

bool Request_Inactiva=false;
bool Request_Activa=false;
bool Ack_Maq_Inactiva=false;
bool Ack_Maq_Activa=false;

bool Flag_Handle_Inactiva=false;
bool Flag_Handle_Activa=false;


bool Flag_Recv_Inactiva=false;
bool Flag_Recv_Activa=false;

extern bool Inactiva_Maquina(void);
extern bool Activa_Maquina(void);

extern bool get_Flag_Conexion_TFT(void);
extern bool get_Flag_Desincro_TFT(void);
extern  uint8_t Address_Device_TFT_Display[6];

#include "Buffers.h"
extern Buffers Buffer;            // Objeto de buffer de mensajes servidor

extern char buffer_contadores_ACC[258];
extern bool Actualiza_Tarjeta_Mecanica(char res[]);

extern bool Creditos_Machine(void);
extern bool Encuesta_Creditos_Premio(void);
extern int Convert_Char_To_Int10(char buffer[]);
extern TaskHandle_t Check_Comunication_Maq;
extern TaskHandle_t Mensajes_Server;
extern WiFiUDP clientUDP;    // Declara un objeto para cliente UDP
extern TaskHandle_t RecepcionRS232;
extern TaskHandle_t Encuestas;
extern  TaskHandle_t CommandProcess;
extern std::vector<String> transaccionesPendientes;
extern TITO Tito;

IPAddress ipDest;


std::string IP_toString_A(char IP_Char[])
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


String getMacString(uint8_t mac[6]) {
    char macStr[18]; // 6 bytes * 2 dígitos + 5 ':' + '\0'
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2],
            mac[3], mac[4], mac[5]);
    return String(macStr);
}

char Conve_Ascii_To_Hex_LL(char *Val_Ascii)
{
  char Val1, Val2;

  Val1 = *Val_Ascii;
  Val2 = Val1 - 0x30;
  return (Val2);
}


extern bool Instala_Driver_Mistic();
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

int NivelUsuario(String Nivel)
{
  if (Nivel == "B")
    return 0;
  else if (Nivel == "P")
    return 1;
  else if (Nivel == "G")
    return 2;
  else if (Nivel == "L")
    return 3;
  else
    return 0;
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


AsyncWebServer Server_Bono(22141);
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


int calcularNivelSenal(int rssi) {
  if (rssi >= -50) return 4;        // Excelente
  else if (rssi >= -60) return 3;   // Buena
  else if (rssi >= -70) return 2;   // Regular
  else if (rssi >= -80) return 1;   // Mala
  else return 0;                    // Muy mala o sin señal
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

/* Genera  y retorna token  de acceso desde la API */
String Token_Generator_Update(String Url)
{
  String Output = "";
  int httpCode;
  WiFiClient client;
  HTTPClient https;
  String fwurl = Url;

  https.setTimeout(10000); /* 10seg */
  if (https.begin(client, fwurl))
  {
    httpCode = https.GET();

    // Serial.println(httpCode);
    if (httpCode == HTTP_CODE_OK)
    {

      String Response = https.getString();
      StaticJsonDocument<1024>
          doc,
          filter;
      DeserializationError error = deserializeJson(doc, Response);
      // Serial.println( Response);
      if (error)
      {
#ifdef Debug_HTTPS
        Serial.println("Error Json deserializeJson");
#endif
      }
      else
      {
        // Token Generado;
#ifdef Debug_HTTPS
        Serial.println("Token Generador! OK");
#endif
        String Token = doc["Data"]["access_token"];
        String Token_Expires = doc["Data"]["expires"];
        bool IsSuccess = doc["IsSuccess"];
        String Message = doc["Message"];
        int Evento = doc["Evento"];

        if (IsSuccess)
        {
          Output = Token;
        }
      }
      doc.clear();
    }
    https.end();
  }
  return Output;
}


String GenerarCodigoSHA1(String texto)
{
    // Convertir texto a bytes
    const char* input = texto.c_str();
    size_t length = texto.length();

    // Buffer SHA1 (20 bytes)
    unsigned char hash[20];

    // Calcular SHA1
    mbedtls_sha1_context ctx;
    mbedtls_sha1_init(&ctx);
    mbedtls_sha1_starts_ret(&ctx);
    mbedtls_sha1_update_ret(&ctx, (const unsigned char*)input, length);
    mbedtls_sha1_finish_ret(&ctx, hash);
    mbedtls_sha1_free(&ctx);

    // Convertir a HEX string (igual que C#: b.ToString("x2"))
    char outputHex[41]; // 40 chars + null
    for (int i = 0; i < 20; i++) {
        sprintf(&outputHex[i * 2], "%02x", hash[i]);
    }

    return String(outputHex);
}

bool Transsaccion_Cashless::Init_API_Bono(void)
{
  Server_Bono.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
  request->send(200, "text/plain", "BUDA SERVER V1.0");
  });

   Server_Bono.addHandler(new AsyncCallbackJsonWebHandler("/api/Fidelizacion/BonoCanje", [](AsyncWebServerRequest* request, JsonVariant& json) {


    Serial.println("Solicitud recibida");

    
    int Code=0x100;
    StaticJsonDocument<200> jsonDocument;
    jsonDocument.clear();
    
    bool IsSuccess_Response=false;
    String Msg;

    if (!json.is<JsonObject>()) {

      jsonDocument["IsSuccess"] = false;
      jsonDocument["IsSuccess"] = Code;
      jsonDocument["IsSuccess"] = "Tipo de dato no identificado JSON";
      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);

      return;
    }else
    {


      

      ipDest = request->client()->remoteIP();

      if (AFT.GET_STATUS_TRANSFER() == TransaccionCashless::TRANS_IDLE)
      {


        if(AFT.GET_STATUS_TRANSFER() == TransaccionCashless::TRANS_IDLE)
          AFT.STATUS_TRANSFER(TransaccionCashless::TRANS_RECIBIDA);

        auto &&data = json.as<JsonObject>();
        /* Url Generica */


        bool IsSuccess = data["IsSuccess"];
        int Current_Cliente_ID_Server_Int = data["Cliente_ID"];
        String Type_Trans_Server = data["Trans_Tipo"];
        uint32_t Cashable_Server = data["Saldo_Canjeable"];
        uint32_t Restricted_Server = data["Saldo_Restringido"];
        uint32_t Non_Restricted_Server = data["Saldo_No_Restringido"];
        uint32_t Trans_ID_Server = data["Trans_ID"];
        String Data_Time_Response_Server = data["Fecha_Hora"];
        String Msgg = data["Message"];
        String Nombre_Cliente = data["Cliente_Nombre"];

        int Cashless_ID = 0;

        Objeto_Transfer["Cashless_ID"]=data["Cashless_ID"];
        Objeto_Transfer_Download["Cashless_ID"]=data["Cashless_ID"];

        String Cashless_Estado = data["Cashless_Estado"];
        Objeto_Transfer_Download["Cashless_Estado"] = Cashless_Estado;
        Objeto_Transfer_Download["Cliente_Nombre"]=Nombre_Cliente;
        Objeto_Transfer_Download["Message"]=Msgg;



        String Ip_Tarjeta = data["Ip"];
        String Key = data["Key"];
        String Mac = data["MAC"];
        String Trans_Estado = data["Trans_Estado"];
        String Tipo_Maq = data["Tipo_Maq"];

        int Year, Month, Day, Hour, Minutes, Seconds;
        sscanf(Data_Time_Response_Server.c_str(), "%d-%d-%d %d:%d:%d", &Year, &Month, &Day, &Hour, &Minutes, &Seconds);

        Objeto_Transfer["IsSuccess"] = false;
        Objeto_Transfer["Cliente_ID"] = Current_Cliente_ID_Server_Int;
        Objeto_Transfer["Trans_Tipo"] = Type_Trans_Server;
        Objeto_Transfer["Saldo_Canjeable"] = Cashable_Server;
        Objeto_Transfer["Saldo_Restringido"] = Restricted_Server;
        Objeto_Transfer["Saldo_No_Restringido"] = Non_Restricted_Server;
        Objeto_Transfer["Trans_ID"] = Trans_ID_Server;
        Objeto_Transfer["Fecha_Hora"] = Data_Time_Response_Server;
        Objeto_Transfer["Message"] = Msgg;
        Objeto_Transfer["Cliente_Nombre"] = Nombre_Cliente;

        Objeto_Transfer["Cashless_Estado"] = Cashless_Estado;
        Objeto_Transfer["Ip"] = Ip_Tarjeta;
        Objeto_Transfer["Key"] = Key;
        Objeto_Transfer["MAC"] = Mac;
        Objeto_Transfer["Trans_Estado"] = Trans_Estado;
        Objeto_Transfer["Tipo_Maq"] = "AFT";

        if (Cashless.Set_Amount_To_Load(Cashable_Server, Restricted_Server, Non_Restricted_Server))
        {
          if (Info_Cashless.Set_Controller_Transfer_Especial(true))
            IsSuccess_Response = true;
          else
            IsSuccess_Response = false;

          if (IsSuccess_Response)
            Msg = "Solicitud Recibida con exito!";
          else
            Msg = "Error  en controlador RS232";
        }
        else
        {
          IsSuccess_Response=false;
          Msg = "Error en Conversion de saldos";

          if(AFT.GET_STATUS_TRANSFER() == TransaccionCashless::TRANS_RECIBIDA)
            AFT.STATUS_TRANSFER(TransaccionCashless::TRANS_IDLE);
        }
      }else
      {
        IsSuccess_Response=false;
        Msg = "Dispositivo ocupado, ya existe una instancia";
      }

      jsonDocument["IsSuccess"] = IsSuccess_Response;
      jsonDocument["Message"] = Msg;

      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);

    }
    
  }));



  Server_Bono.addHandler(new AsyncCallbackJsonWebHandler("/ConfigBA", [](AsyncWebServerRequest* request, JsonVariant& json) 
  {

    
    
    String DataTime = String(RTC.getYear()) + "-" +
                      String(RTC.getMonth() + 1) + "-" +
                      String(RTC.getDay()) + " " +
                      String(RTC.getHour(true)) + ":" +
                      String(RTC.getMinute()) + ":" +
                      String(RTC.getSecond());

    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
    bool IsSuccess = false;
    String Msg;

    String texto = request->hasHeader("texto") ? request->getHeader("texto")->value() : "";
    String maqIp = request->hasHeader("maqIp") ? request->getHeader("maqIp")->value() : "";
    String widget = request->hasHeader("widget") ? request->getHeader("widget")->value() : "";
    String time = request->hasHeader("time") ? request->getHeader("time")->value() : "";
    String hashOnline = request->hasHeader("hash") ? request->getHeader("hash")->value() : "";

    String hasInput = texto + "_^_" + maqIp + "_^_" + widget + "_^_" + time;
    String shaLocal = GenerarCodigoSHA1(hasInput);

    // Serial.println(hashOnline);
    // Serial.println(shaLocal);

    if (shaLocal.equalsIgnoreCase(hashOnline))
    {

        //Serial.println("Hash Ok");
        if (!json.is<JsonObject>())
        {
          IsSuccess = false;
          Msg="Error en los datos recibidos compruebe  la informacion";



          jsonDocument["IsSuccess"] = IsSuccess;
          jsonDocument["Message"] = Msg;
          jsonDocument["Data"] = DataTime;

          String Json;
          serializeJson(jsonDocument, Json);
          request->send(200, "application/json", Json);
          Info_Cashless.Log(RTC, "COMANDO_BA" + ipDest.toString(), "JSON_NO_ES_UN_OBJETO_VALIDO");

//#ifdef DEBUG_CONFIG_MODE_TRAMISIONS
//          Serial.println("Json no es un objeto valido");
//#endif

          return;
        }

        //Serial.println("Hash Ok2222222222");

        //erial.println("JSON recibido:");
        // serializeJsonPretty(json, Serial);
        // Serial.println();

        if(Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless)||Variables_globales.Get_Variable_Global(Flag_Sesion_RFID)|| !Variables_globales.Get_Variable_Global(Comunicacion_Maq)||!Variables_globales.Get_Variable_Global(Enable_Cashless)|| Variables_globales.Get_Variable_Global(Flag_Maquina_Juego_Evento)||Variables_globales.Get_Variable_Global(Flag_Maquina_En_Juego))
        {
          IsSuccess = false;
          Msg=" Dispositivo no disponible para la operacion por favor valide el estado de la maquina";



          jsonDocument["IsSuccess"] = IsSuccess;
          jsonDocument["Message"] = Msg;
          jsonDocument["Data"] = DataTime;

          String Json;
          serializeJson(jsonDocument, Json);
          request->send(200, "application/json", Json);
          Info_Cashless.Log(RTC, "COMANDO_BA", ipDest.toString() + " Dispositivo no disponible para la operacion por favor valide el estado de la maquina");

#ifdef DEBUG_CONFIG_MODE_TRAMISIONS
          Serial.println("Json no es un objeto valido");

          
#endif
          return;
        }

        //Serial.println("Hash Ok33333333");

        if (!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
        {

          IsSuccess = false;
          Msg = "Perdida de comunicacion con la MET";

          jsonDocument["IsSuccess"] = IsSuccess;
          jsonDocument["Message"] = Msg;
          jsonDocument["Data"] = DataTime;

          String Json;
          serializeJson(jsonDocument, Json);
          request->send(200, "application/json", Json);
          Info_Cashless.Log(RTC, "COMANDO_BA", ipDest.toString() + " NO HAY COMUNICACION CON LA MET");

#ifdef DEBUG_CONFIG_MODE_TRAMISIONS
          Serial.println("No hay comunicacion con la MET");

#endif
          return;
        }


        if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)!=3)
        {
          IsSuccess = false;
          Msg = "Operacion no permitida para este tipo de maquina (NO ES AFT)";

          jsonDocument["IsSuccess"] = IsSuccess;
          jsonDocument["Message"] = Msg;
          jsonDocument["Data"] = DataTime;

          String Json;
          serializeJson(jsonDocument, Json);
          request->send(200, "application/json", Json);
          Info_Cashless.Log(RTC, "COMANDO_BA", ipDest.toString() + " Operacion no permitida para este tipo de maquina");
          return;
        }

        if (AFT.GET_STATUS_BA() == TransaccionCashless::BA_IDLE && AFT.GET_STATUS_TRANSFER()==TransaccionCashless::TRANS_IDLE)
        {
          // AFT.STATUS_BA(TransaccionCashless::BA_RECIBIDA);


          //Serial.println("Hash Ok444444444444444444444");

          auto &&data = json.as<JsonObject>();

          if (data.containsKey("Assets") && data.containsKey("ID") && data.containsKey("GUID") && data.containsKey("Deno_Contabilidad") && data.containsKey("Deno_Cashless")) 
          {


            // Serial.println(data["Deno_Contabilidad"].as<float>());
            // Serial.println(data["Deno_Cashless"].as<float>());


            //Serial.println("Hash Ok5555555555555555555555");

            IPAddress ipRemote=request->client()->remoteIP();

            
            if(ipRemote!=ipDest && ipRemote != IPAddress(0,0,0,0))
            {
              NVS.begin("Config_ESP32", false);
              NVS.putUInt("Ip_BDA",(uint32_t)ipRemote);
              NVS.end();

              ipDest = ipRemote;
            }


            //Serial.println("Hash Ok66666666666666666666");

            AFT.solicitudBA.ID = data["ID"].as<int>();
            AFT.solicitudBA.GUID = data["GUID"].as<String>();

            AFT.solicitudBA.Deno_Contabilidad_BUDA = data["Deno_Contabilidad"].as<float>();
            AFT.solicitudBA.Deno_Cashless_BUDA = data["Deno_Cashless"].as<float>();

            // --- LIMPIAR EL ARRAY COMPLETO ---
            memset(AFT.solicitudBA.assets, 0, sizeof(AFT.solicitudBA.assets));

            // Obtener lista de assets
            JsonArray assets = data["Assets"].as<JsonArray>();

            //Serial.println(assets);

            //Serial.println("Hash Ok77777777777777777777");

            if (assets.size() != 8)
            {
              //Serial.println("Assets tamaño invalido");
              IsSuccess = false;
              Msg = "Assets debe tener 8 digitos";
              //return;
            }
            else
            {
              int i = 0;
              bool error = false;
              bool allZero = true;

              for (JsonVariant v : assets)
              {
                // 1. Validar tipo
                if (!v.is<const char *>())
                {
                  //Serial.println("Asset no es string");
                  Msg = "Asset no es string";
                  error = true;
                  break;
                }

                const char *c = v.as<const char *>();

                // 2. Validar NULL
                if (c == nullptr)
                {
                  //Serial.println("Asset NULL");

                  Msg = "Asset NULL";
                  error = true;
                  break;
                }

                // 3. Validar longitud (debe ser 1)
                if (strlen(c) != 1)
                {
                  //Serial.println("Asset longitud invalida");
                  Msg = "Asset longitud invalida";
                  error = true;
                  break;
                }

                // 4. Validar que sea dígito
                if (!isdigit(c[0]))
                {
                  //Serial.println("Asset no es digito");
                  Msg = "Asset no es digito";
                  error = true;
                  break;
                }

                // 5. Validar overflow
                if (i >= sizeof(AFT.solicitudBA.assets))
                {
                  //Serial.println("Overflow de assets");
                  Msg = "Overflow de assets";
                  error = true;
                  break;
                }

                AFT.solicitudBA.assets[i++] = c[0];

                if (c[0] != '0')
                {
                  allZero = false;
                }
              }

              if (error)
              {
                IsSuccess = false;
                // return;
              }
              else if (allZero)
              {
                //Serial.println("Assets todos en cero");
                IsSuccess = false;
                Msg = "Monto invalido (todos los digitos en cero)";
              }
              else
              {

                for (int i = 0; i < 4; i++)
                {
                  uint8_t high = assets[i * 2].as<int>();
                  uint8_t low = assets[(i * 2) + 1].as<int>();

                  AFT.solicitudBA.Amount[i] = (high << 4) | low;
                }

                AFT.solicitudBA.count = i;
                AFT.solicitudBA.pendiente = true;

                IsSuccess = true;
                Msg = "Solicitud recibida correctamente";

                uint8_t BonusBCD[4];

                
              }

              // int i = 0;

              // // --- COPIAR EXACTAMENTE COMO LO PIDES ---
              // for (JsonVariant v : assets)
              // {
              //   const char *c = v.as<const char *>();
              //   AFT.solicitudBA.assets[i++] = c[0]; // copiando solo el primer char
              // }

              // AFT.solicitudBA.count = i;

              // AFT.solicitudBA.pendiente = true;

              // IsSuccess = true;
              // Msg = "Solicitud recibida correctamente";
            }
          }
          else
          {
            IsSuccess = false;
            Msg = "Falta uno o mas parametros para procesar la solicitud";
            // AFT.STATUS_BA(TransaccionCashless::BA_RECIBIDA);
          }
        }
        else
        {
          IsSuccess = false;
          Msg = "Dispositivo ocupado para procesar la solicitud";

          IPAddress ipRemote = request->client()->remoteIP();

          if (ipRemote != ipDest && ipRemote != IPAddress(0, 0, 0, 0))
          {
            NVS.begin("Config_ESP32", false);
            NVS.putUInt("Ip_BDA", (uint32_t)ipRemote);
            NVS.end();

            ipDest = ipRemote;
          }
        }
    }
    else
    {
        IsSuccess = false;
        Msg = "Hash no compatible para procesar la solicitud";
    }



    //Serial.println("Hash Ok888888888888888888888");
    
    jsonDocument["IsSuccess"] = IsSuccess;
    jsonDocument["Message"] = Msg;
    jsonDocument["Data"] = DataTime;

    String Json;
    serializeJson(jsonDocument, Json);
    request->send(200, "application/json", Json);

    Info_Cashless.Log(RTC, "COMANDO_BA", ipDest.toString() + " " + Msg);

  }));

  Server_Bono.on("/Borrar_Archivo_BA", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    static DynamicJsonDocument doc(256);
    doc.clear();

    bool eliminado = false;
    bool renombrado = false;

    // Eliminar fifo.txt si existe
    if (SPIFFS.exists("/fifo.txt"))
    {
        eliminado = SPIFFS.remove("/fifo.txt");
    }

    // Renombrar fifo.tmp a fifo.txt si existe
    if (SPIFFS.exists("/fifo.tmp"))
    {
        renombrado = SPIFFS.remove("/fifo.tmp");
    }

    doc["IsSuccess"] = (eliminado || renombrado);

    String response;
    serializeJson(doc, response);

    request->send(200, "application/json", response);
  });

  Server_Bono.begin();
  return true;
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

    case 17:
      jsonDocument["Tipo_Maq"] = "EFT";
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
    
    

    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)<4||Configuracion.Get_Configuracion(Tipo_Maquina, 0)==17)
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

        case 17:
        jsonDocument["Tipo_Maq"] = "EFT";
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

      case 17:
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
    
    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)<4||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
    {
      // bool Test;
      // NVS.begin("Config_ESP32", false);
      // NVS.putBool("Enable_Cashless",true);
      // Test=NVS.getBool("Enable_Cashless",false);
      // Variables_globales.Set_Variable_Global(Enable_Cashless,Test);
      // NVS.end();

      //if (Test && Variables_globales.Get_Variable_Global(Enable_Cashless))
      //{
        StaticJsonDocument<800> jsonDocument;
        jsonDocument.clear();

        /* Verifica si es AFT O EFT */

        if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17) /*EFT*/
        {

          bool Test;
          NVS.begin("Config_ESP32", false);
          NVS.putBool("Enable_Cashless", true);
          Test = NVS.getBool("Enable_Cashless", false);
          Variables_globales.Set_Variable_Global(Enable_Cashless, Test);
          NVS.end();

          Buffer_Cashless.Set_RX_AFT_2(6); /* Crea llave registro solo en Sistema Porque EFT no  necesita este parametro */
          if (Cashless.Get_Trans_ID_EFT() > 0)
          Cashless.Delete_Trans_ID();
          Variables_globales.Set_Variable_Global(Status_AFT_Machine, true);
          jsonDocument["IsSuccess"] = true;
          jsonDocument["Desc"] = "Cashless Habilitado con Exito EFT!";
        }
        else /*AFT*/
        {

          /* Pregunta si la maquina AFT admite registro */

          if (Variables_globales.Get_Variable_Global(Ignore_Register_Machie))
          {

            bool Test;
            NVS.begin("Config_ESP32", false);
            NVS.putBool("Enable_Cashless", true);
            Test = NVS.getBool("Enable_Cashless", false);
            Variables_globales.Set_Variable_Global(Enable_Cashless, Test);
            NVS.end();

            Buffer_Cashless.Set_RX_AFT_2(6); /* Crea llave registro solo en Sistema porque la maquina funciona sin registro */
            if (Cashless.Get_Trans_ID() > 0)
              Cashless.Delete_Trans_ID();

            jsonDocument["IsSuccess"] = true;
            jsonDocument["Desc"] = "Cashless Habilitado con Exito AFT!";
            Variables_globales.Set_Variable_Global(Status_AFT_Machine, true);

          }
          else
          {
            bool Test;
            // Buffer_Cashless.Set_Status_Reg(0xAA);
            Flaggg = true; /* Transmite Registro Maquina */
            delay(500);
            unsigned long Timout_Break_Response1;
            int Stop_Transaccion_Amount_Response1 = 8000; // Tiempo de espera en milisegundos (2 Seg MAX)
            Timout_Break_Response1 = millis();
            esp_task_wdt_init(1000000, true);
            esp_task_wdt_add(NULL);

            while ((Buffer_Cashless.Get_Status_Reg() == 0xAA) && (millis() - Timout_Break_Response1 < Stop_Transaccion_Amount_Response1))
            {
              esp_task_wdt_reset();
              vTaskDelay(300);
              // Serial.println("Esperando respuesta de maquina......");
            }
            String Code;
            // Serial.println(Buffer_Cashless.Get_Status_Reg());
            switch (Buffer_Cashless.Get_Status_Reg())
            {
            case 0x00:

              NVS.begin("Config_ESP32", false);
              NVS.putBool("Enable_Cashless", true);
              Test = NVS.getBool("Enable_Cashless", false);
              Variables_globales.Set_Variable_Global(Enable_Cashless, Test);
              NVS.end();

              jsonDocument["IsSuccess"] = true;
              jsonDocument["Desc"] = "Cashless Habilitado con Exito AFT!";
              Variables_globales.Set_Variable_Global(Status_AFT_Machine, true);
              break;

            case 0x01:

              NVS.begin("Config_ESP32", false);
              NVS.putBool("Enable_Cashless", true);
              Test = NVS.getBool("Enable_Cashless", false);
              Variables_globales.Set_Variable_Global(Enable_Cashless, Test);
              NVS.end();

              jsonDocument["IsSuccess"] = true;
              jsonDocument["Desc"] = "Cashless Habilitado con Exito AFT!";
              Variables_globales.Set_Variable_Global(Status_AFT_Machine, true);
              break;

            case 0x40:
              // NVS.begin("Config_ESP32", false);
              // NVS.putBool("Enable_Cashless", false);
              // Test = NVS.getBool("Enable_Cashless", false);
              // Variables_globales.Set_Variable_Global(Enable_Cashless, Test);
              // NVS.end();
              jsonDocument["IsSuccess"] = false;
              jsonDocument["Desc"] = "Registro AFT pendiente AFT!";
              break;

            case 0x80:
              // NVS.begin("Config_ESP32", false);
              // NVS.putBool("Enable_Cashless", false);
              // Test = NVS.getBool("Enable_Cashless", false);
              // Variables_globales.Set_Variable_Global(Enable_Cashless, Test);
              // NVS.end();
              jsonDocument["IsSuccess"] = false;
              jsonDocument["Desc"] = "Maquina no registrada!";
              break;

            default:
              jsonDocument["IsSuccess"] = false;
              Code = "Codigo: ";
              if (!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                jsonDocument["Desc"] = "No hay comunicacion con la MET";
              else
                jsonDocument["Desc"] = "No hubo respuesta de la maquina! " + Code + String(Buffer_Cashless.Get_Status_Reg());
              // NVS.begin("Config_ESP32", false);
              // NVS.putBool("Enable_Cashless", true);
              // Test = NVS.getBool("Enable_Cashless", false);
              // Variables_globales.Set_Variable_Global(Enable_Cashless, Test);
              // NVS.end();
              break;
            }
          }

          // int Status;
          // Status = Registra_Machine();
          // delay(300); /* Espera Por respuesta de la maquina */
        }

        String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

        jsonDocument["Fecha_Hora"] = DataTime;
        jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);

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

        case 17:
            jsonDocument["Tipo_Maq"] = "EFT";
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

        bool Recv=Reset_TFT();
        delay(800);
        
        if(jsonDocument["IsSuccess"]==true)
          ESP.restart();
      //}
      // else
      // {
      //   String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());

      //   StaticJsonDocument<200> jsonDocument;
      //   jsonDocument.clear();
      //   jsonDocument["IsSuccess"] = false;
      //   jsonDocument["Fecha_Hora"] = DataTime;
      //   jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
      //   jsonDocument["Desc"] = "No fue posible habilitar Cashless";
      //   jsonDocument["Key"]=Buffer_Cashless.Get_Key_Register_AFT_String();
      //   switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
      //   {
      //   case 0:
      //     jsonDocument["Tipo_Maq"] = "AFT";
      //     break;
      //   case 1:
      //     jsonDocument["Tipo_Maq"] = "AFT";
      //     break;
      //   case 2:
      //     jsonDocument["Tipo_Maq"] = "EFT";
      //     break;
      //   case 3:
      //     jsonDocument["Tipo_Maq"] = "AFT";
      //     break;
      //   default:
      //     jsonDocument["Tipo_Maq"] = "";
      //     break;
      //   }
      //   jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
      //   String Json;
      //   serializeJson(jsonDocument, Json); /* Serializa Data */
      //  // Serial.println(Json);
      //   request->send(200, "application/json", Json);
      //   delay(800);
      //   ESP.restart();
      // }
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

      case 17:
        jsonDocument["Tipo_Maq"] = "EFT";
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


    case 17:
        jsonDocument["Tipo_Maq"] = "EFT";
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

    case 17:
        jsonDocument["Tipo_Maq"] = "EFT";
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

    case 17:
        jsonDocument["Tipo_Maq"] = "EFT";
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
  StaticJsonDocument<800> jsonDocument;
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
  if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)<4||Configuracion.Get_Configuracion(Tipo_Maquina, 0)==17)
  {

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
    {
      Buffer_Cashless.Set_RX_AFT_2(6); /* Crea llave registro solo en Sistema Porque EFT no  necesita este parametro */
      jsonDocument["IsSuccess"] = true;
        if (Cashless.Get_Trans_ID_EFT() > 0)
          Cashless.Delete_Trans_ID();
        Variables_globales.Set_Variable_Global(Status_AFT_Machine, true);
        jsonDocument["Fecha_Hora"] = DataTime;
        jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
        jsonDocument["Trans_ID"] = Cashless.Get_Trans_ID_Int();
        jsonDocument["Desc"] = "Maquina EFT Registrada con Exito";
        jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
    }
    else
    {

      if (!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
      {
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Fecha_Hora"] = DataTime;
        jsonDocument["Key"] = nullptr;
        jsonDocument["Trans_ID"] = Cashless.Get_Trans_ID_Int();
        jsonDocument["Desc"] = "No hay comunicacion con la MET";
        jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
      }
      else
      {

        if (Variables_globales.Get_Variable_Global(Ignore_Register_Machie))
        {
          Variables_globales.Set_Variable_Global(Status_AFT_Machine, true);
          Buffer_Cashless.Set_RX_AFT_2(6); /* Crea llave registro solo en Sistema porque la maquina funciona sin registro */
          jsonDocument["IsSuccess"] = true;
          if (Cashless.Get_Trans_ID_Int() > 0)
            Cashless.Delete_Trans_ID();
          jsonDocument["Fecha_Hora"] = DataTime;
          jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
          jsonDocument["Trans_ID"] = Cashless.Get_Trans_ID_Int();
          jsonDocument["Desc"] = "Maquina Registrada con Exito";
          jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
        }
        else
        {
          // Status = Registra_Machine();
          // delay(300); /* Espera Por respuesta de la maquina */
          Buffer_Cashless.Set_Status_Reg(0xAA);
          Flaggg = true; /* Transmite Registro Maquina */
          // delay(100);
          unsigned long Timout_Break_Response1;
          int Stop_Transaccion_Amount_Response1 = 5000; // Tiempo de espera en milisegundos (2 Seg MAX)
          Timout_Break_Response1 = millis();
          esp_task_wdt_init(1000000, true);
          esp_task_wdt_add(NULL);

          while ((Buffer_Cashless.Get_Status_Reg() == 0xAA) && (millis() - Timout_Break_Response1 < Stop_Transaccion_Amount_Response1))
          {
            esp_task_wdt_reset();
            vTaskDelay(300);
            // Serial.println("Esperando respuesta de maquina......");
          }

          switch (Buffer_Cashless.Get_Status_Reg())
          {
          case 0x00:
            jsonDocument["IsSuccess"] = true;

            if (Cashless.Get_Trans_ID_Int() > 0)
              Cashless.Delete_Trans_ID();
            Variables_globales.Set_Variable_Global(Status_AFT_Machine, true);
            jsonDocument["Fecha_Hora"] = DataTime;
            jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
            jsonDocument["Trans_ID"] = Cashless.Get_Trans_ID_Int();
            jsonDocument["Desc"] = "Maquina Registrada con Exito";
            jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
            break;

          case 0x01:
            jsonDocument["IsSuccess"] = true;
            if (Cashless.Get_Trans_ID_Int() > 0)
              Cashless.Delete_Trans_ID();
            Variables_globales.Set_Variable_Global(Status_AFT_Machine, true);
            jsonDocument["Fecha_Hora"] = DataTime;
            jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
            jsonDocument["Trans_ID"] = Cashless.Get_Trans_ID_Int();
            jsonDocument["Desc"] = "Maquina Registrada con Exito";
            jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
            break;

          case 0x40:
            jsonDocument["IsSuccess"] = false;
            jsonDocument["Fecha_Hora"] = DataTime;
            jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
            jsonDocument["Trans_ID"] = Cashless.Get_Trans_ID_Int();
            jsonDocument["Desc"] = "Registro pendiente";
            jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
            break;

          case 0x80:

            jsonDocument["IsSuccess"] = false;
            jsonDocument["Fecha_Hora"] = DataTime;
            jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
            jsonDocument["Trans_ID"] = Cashless.Get_Trans_ID_Int();
            jsonDocument["Desc"] = "Maquina no registrada";
            jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
            break;

          default:
            jsonDocument["IsSuccess"] = false;
            jsonDocument["Fecha_Hora"] = DataTime;
            jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
            jsonDocument["Trans_ID"] = Cashless.Get_Trans_ID_Int();
            jsonDocument["Desc"] = "No hubo respuesta de la maquina";
            jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
            break;
          }
        }
      }

      Buffer_Cashless.Set_Status_Reg(0xAA);
    }

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
                                       // Serial.println(Json);
    request->send(200, "application/json", Json);
  }
  else
  {

    jsonDocument["IsSuccess"] = false;
    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Key"] = nullptr;
    jsonDocument["Trans_ID"] = Cashless.Get_Trans_ID_Int();
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

  if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)<4||Configuracion.Get_Configuracion(Tipo_Maquina, 0)==17)
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

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17) /* Cancela Registro EFT */
    {
      if (Buffer_Cashless.Delete_Key_Register_AFT(true))
        Variables_globales.Set_Variable_Global(Status_AFT_Machine, false);
      Cashless.Delete_Trans_ID();
      jsonDocument["IsSuccess"] = true;
      jsonDocument["Fecha_Hora"] = DataTime;
      if (Variables_globales.Get_Variable_Global(Status_AFT_Machine))
        jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
      else
        jsonDocument["Key"] = nullptr;
      jsonDocument["Trans_ID"] = Cashless.Get_Trans_ID_Int();
      jsonDocument["Desc"] = "Registro Maquina EFT Cancelado con Exito!";
      jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
      jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
      // Serial.println("Se elimino el registro de la maquina con exito");
      Cashless.Set_Reintento_Registro(false);
    }
    else
    {

      if (Variables_globales.Get_Variable_Global(Ignore_Register_Machie))
      {
        if (Buffer_Cashless.Delete_Key_Register_AFT(true))
          Variables_globales.Set_Variable_Global(Status_AFT_Machine, false);
        Cashless.Delete_Trans_ID();
        jsonDocument["IsSuccess"] = true;
        jsonDocument["Fecha_Hora"] = DataTime;
        if (Variables_globales.Get_Variable_Global(Status_AFT_Machine))
          jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
        else
          jsonDocument["Key"] = nullptr;
        jsonDocument["Trans_ID"] = Cashless.Get_Trans_ID_Int();
        jsonDocument["Desc"] = "Registro Maquina AFT Cancelado con Exito!";
        jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
        jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
        // Serial.println("Se elimino el registro de la maquina con exito");
        Cashless.Set_Reintento_Registro(false);
      }
      else
      {

        Status = Delete_Registro_Machine();
        delay(500); /* Espera Por respuesta de la maquina */

        // unsigned long Timout_Break_Response1;
        // int Stop_Transaccion_Amount_Response1 = 5000; // Tiempo de espera en milisegundos (2 Seg MAX)
        // Timout_Break_Response1 = millis();
        // esp_task_wdt_init(1000000, true);
        // esp_task_wdt_add(NULL);

        // while ((millis() - Timout_Break_Response1 < Stop_Transaccion_Amount_Response1))
        // {
        //   esp_task_wdt_reset();
        //   vTaskDelay(300);
        //   // Serial.println("Esperando respuesta de maquina......");
        // }

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
          jsonDocument["Trans_ID"] = Cashless.Get_Trans_ID_Int();
          jsonDocument["Desc"] = "Registro Maquina AFT Cancelado con Exito!";
          jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
          jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
          // Serial.println("Se elimino el registro de la maquina con exito");
          Cashless.Set_Reintento_Registro(false);
          break;
        case 2:

          // Serial.println("Maquina Registrada anteriormente");
          jsonDocument["IsSuccess"] = false;
          jsonDocument["Fecha_Hora"] = DataTime;
          if (Variables_globales.Get_Variable_Global(Status_AFT_Machine))
            jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
          else
            jsonDocument["Key"] = nullptr;
          jsonDocument["Trans_ID"] = Cashless.Get_Trans_ID_Int();
          jsonDocument["Desc"] = "Registro no eliminado";
          jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
          jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
          break;

        default:
          // Serial.println("Maquina no responde ");

          if (!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
          {
            jsonDocument["IsSuccess"] = false;
            jsonDocument["Fecha_Hora"] = DataTime;
            if (Variables_globales.Get_Variable_Global(Status_AFT_Machine))
              jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
            else
              jsonDocument["Key"] = nullptr;
            jsonDocument["Trans_ID"] = Cashless.Get_Trans_ID_Int();
            jsonDocument["Desc"] = "No hay comunicacion con la maquina";
            jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
            jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
          }
          else
          {
            jsonDocument["IsSuccess"] = false;
            jsonDocument["Fecha_Hora"] = DataTime;
            if (Variables_globales.Get_Variable_Global(Status_AFT_Machine))
              jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
            else
              jsonDocument["Key"] = nullptr;
            jsonDocument["Trans_ID"] = Cashless.Get_Trans_ID_Int();
            jsonDocument["Desc"] = "No hubo respuesta de la maquina";
            jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Enable_Cashless);
            jsonDocument["Cobro_Cashless"] = Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss);
          }
          break;
        }
      }
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
  if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)<4||Configuracion.Get_Configuracion(Tipo_Maquina, 0)==17)
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


    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 0:
      if (SPIFFS.exists("/transacciones.txt"))
      {
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/transacciones.txt", "text/plain", true);
        response->addHeader("Txt", "Transferencias Pendientes");
        request->send(response);
      }else
        request->send(400, "application/json", "No existe el archivo transacciones.txt ");
      break;
      
    case 1:
      if (SPIFFS.exists("/transacciones.txt"))
      {
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/transacciones.txt", "text/plain", true);
        response->addHeader("Txt", "Transferencias Pendientes");
        request->send(response);
      }else
        request->send(400, "application/json", "No existe el archivo transacciones.txt");
      break;

    case 2:
      if (SPIFFS.exists("/transacciones.txt"))
      {
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/transacciones.txt", "text/plain", true);
        response->addHeader("Txt", "Transferencias Pendientes");
        request->send(response);
      }else
        request->send(400, "application/json", "No existe el archivo transacciones.txt");
      break;

    case 3:
      if (SPIFFS.exists("/transacciones.txt"))
      {
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/transacciones.txt", "text/plain", true);
        response->addHeader("Txt", "Transferencias Pendientes");
        request->send(response);
      }else
        request->send(400, "application/json", "No existe el archivo transacciones.txt");
      break;

    case 17:
      if (SPIFFS.exists("/transacciones.txt"))
      {
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/transacciones.txt", "text/plain", true);
        response->addHeader("Txt", "Transferencias Pendientes");
        request->send(response);
      }else
        request->send(400, "application/json", "No existe el archivo transacciones.txt");
      break;
    
    default:
        request->send(400, "application/json", "Tipo de maquina no compatible no AFT/EFT");
      break;
    }
    
  });


  Server_API.on("/Borra_Transferencias_Pendientes", HTTP_GET, [](AsyncWebServerRequest *request){
    if (SPIFFS.exists("/transacciones.txt")) { 
      SPIFFS.remove("/transacciones.txt"); // Elimina el archivo

      if(!SPIFFS.exists("/transacciones.txt"))
      {
        transaccionesPendientes.clear(); 
        request->send(200, "text/plain", "Archivo /transacciones.txt eliminado");
      }else{
        request->send(200, "text/plain", "No fue Posible eliminar el archivo /transacciones.txt");
      }
    }else{
      request->send(200, "text/plain", "El archivo /transacciones.txt no existe");
    }
    
  });

  Server_API.on("/Habilita_Bandera_Registro_Maq", HTTP_GET, [](AsyncWebServerRequest *request){

  
  StaticJsonDocument<200> jsonDocument;
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

  case 4:
    jsonDocument["Tipo_Maq"] = "IRT";
    break;

  case 5:
    jsonDocument["Tipo_Maq"] = "Generica";
    break;

  default:
    jsonDocument["Tipo_Maq"] = String(Configuracion.Get_Configuracion(Tipo_Maquina, 0));
    break;
  }

  if (jsonDocument["Tipo_Maq"] == "AFT" || jsonDocument["Tipo_Maq"] == "EFT")
  {

    if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
    {

      bool Ignore_Reg_Maq;
      NVS.begin("Config_ESP32", false);
      NVS.putBool("Ignore_Reg_Maq", true);
      Ignore_Reg_Maq = NVS.getBool("Ignore_Reg_Maq", false);
      Variables_globales.Set_Variable_Global(Ignore_Register_Machie, Ignore_Reg_Maq);
      NVS.end();

      if (Variables_globales.Get_Variable_Global(Ignore_Register_Machie))
      {
        jsonDocument["IsSuccess"] = true;
        jsonDocument["Message"] = "La interfaz no espera registro AFT";
      }
      else
      {
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Message"] = "La interfaz espera registro AFT";
      }
    }
    else
    {
      jsonDocument["IsSuccess"] = false;
      jsonDocument["Message"] = "No hay comunicacion con la MET";
    }
  }
  else
  {
    jsonDocument["IsSuccess"] = false;
    jsonDocument["Message"] = "No compatible con el tipo de maquina";
  }

  String Output;
  serializeJson(jsonDocument, Output); /* Serializa Data */
  // Serial.println(Output);
  request->send(200, "application/json", Output);
  });

  Server_API.on("/Inhabilita_Bandera_Registro_Maq", HTTP_GET, [](AsyncWebServerRequest *request){

  
  StaticJsonDocument<200> jsonDocument;
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

  case 4:
    jsonDocument["Tipo_Maq"] = "IRT";
    break;

  case 5:
    jsonDocument["Tipo_Maq"] = "Generica";
    break;

  default:
    jsonDocument["Tipo_Maq"] = String(Configuracion.Get_Configuracion(Tipo_Maquina, 0));
    break;
  }

  if (jsonDocument["Tipo_Maq"] == "AFT" || jsonDocument["Tipo_Maq"] == "EFT")
  {

    if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
    {

      bool Ignore_Reg_Maq;
      NVS.begin("Config_ESP32", false);
      NVS.putBool("Ignore_Reg_Maq", false);
      Ignore_Reg_Maq = NVS.getBool("Ignore_Reg_Maq", false);
      Variables_globales.Set_Variable_Global(Ignore_Register_Machie, Ignore_Reg_Maq);
      NVS.end();

      if (!Variables_globales.Get_Variable_Global(Ignore_Register_Machie))
      {
        jsonDocument["IsSuccess"] = true;
        jsonDocument["Message"] = "La interfaz espera registro AFT";
      }
      else
      {
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Message"] = "La interfaz no espera registro AFT";
      }
    }
    else
    {
      jsonDocument["IsSuccess"] = false;
      jsonDocument["Message"] = "No hay comunicacion con la MET";
    }
  }
  else
  {
    jsonDocument["IsSuccess"] = false;
    jsonDocument["Message"] = "No compatible con el tipo de maquina";
  }

  String Output;
  serializeJson(jsonDocument, Output); /* Serializa Data */
  // Serial.println(Output);
  request->send(200, "application/json", Output);
  });
// bool Ignore_Reg_Maq=NVS.getBool("Ignore_Reg_Maq",false);
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

    case 17:
        jsonDocument["Tipo_Maq"] = "EFT";
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

  Server_API.on("/Habilitar_Multiplicador_EFT",HTTP_GET, [](AsyncWebServerRequest *request)
  {

    
    DynamicJsonDocument jsonDocument(800);
    jsonDocument.clear();
    char Current_IP[4];
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    uint16_t TipoMaq = Configuracion.Get_Configuracion(Tipo_Maquina, 0);
    if (TipoMaq != 2)
    {
      jsonDocument["IsSuccess"] = false;
      jsonDocument["Message"] = "Tipo de maquina no compatible no es EFT  550";

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
      case 4:
        jsonDocument["Tipo_Maq"] = "IRT";
        break;
      case 5:
        jsonDocument["Tipo_Maq"] = "Generica";
        break;
      case 6:
        jsonDocument["Tipo_Maq"] = "Poker";
        break;

      case 17:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;

      default:
        jsonDocument["Tipo_Maq"] = "";
        break;
      }


      String Output;
      serializeJson(jsonDocument, Output); /* Serializa Data */
      // Serial.println(Output);
      request->send(200, "application/json", Output);
      return;
    }


    

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

   
  });

  Server_API.on("/Borra_Id_Cliente", HTTP_GET, [](AsyncWebServerRequest *request)
  {

    IPAddress ipCliente = request->client()->remoteIP();

    DynamicJsonDocument jsonDocument(800);
    jsonDocument.clear();
    char Current_IP[4];
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    bool IsSuccess=false;
    IsSuccess=true;
    jsonDocument["IsSuccess"]=IsSuccess;
    jsonDocument["Id_Cliente"] = contadores.Get_Client_ID_Transaccion_Int();
    Info_Cashless.Remove_Currrent_Player_Sesion();
    jsonDocument[ "Message"] = "Id de cliente en memoria eliminado con exito!";
    jsonDocument["Ip"] = IP_toString_(Current_IP);
    jsonDocument["Fecha_Hora"] = DataTime;
    String Output;
    serializeJson(jsonDocument, Output); /* Serializa Data */
    request->send(200, "application/json", Output);
    Info_Cashless.Log(RTC,"SOLICITUD_BORRADO_CLIENTE_ID","RECIBIDA_DESDE:"+ipCliente.toString()+" RESET_DISPOSITIVO");
    delay(1000);
    ESP.restart(); 
  });

  /**
   * @brief Endpoint: /Cierre_Sesion_Duplicada
   *
   * Manejador HTTP que procesa la notificación de sesión duplicada enviada por el servidor.
   * Cierra la sesión actual de fidelización o tramita una descarga cashless según el contexto.
   *
   * - Si hay sesión cashless activa: se procesa una descarga (AFT.Procesa_Sesion_Duplicada()).
   * - Si hay sesión de fidelización: se cierra directamente (Info_Cashless.Close_Player_Tracking_Sesion()).
   *
   * @return JSON con:
   *   - "IsSuccess": true/false
   *   - "Message": Descripción del resultado
   *   - "Ip": IP configurada del módulo
   *   - "Fecha_Hora": Marca de tiempo del evento
   *
   * @note 
   */
  Server_API.on("/Cierre_Sesion_Duplicada",HTTP_GET,[](AsyncWebServerRequest *request){

    IPAddress ipCliente = request->client()->remoteIP();
    
    DynamicJsonDocument jsonDocument(800);
    jsonDocument.clear();
    char Current_IP[4];
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    bool IsSuccess=false;
    String Msg;

    Creditos_Machine();
    delay(250);
    int Creditos_Actuales_Maq = Convert_Char_To_Int10(contadores.Get_Contadores_Char(24));


    if(!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
    {
      IsSuccess = false;
      Msg = "No hay comunicacion con la MET";
    } else if(Creditos_Actuales_Maq>10)
    {
      IsSuccess = false;
      Msg = "No se puede cerrar la sesion porque los creditos actuales son mayores que 10";
    }
    else
    {
      if (Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
      {

        if (Variables_globales.Get_Variable_Global(Enable_Cashless))
        {
          if (Info_Cashless.Type_Sesion() == PLAYER_CASHLESS_SESION)
          {
            if (AFT.GET_STATUS_TRANSFER() == TransaccionCashless::TRANS_IDLE)
            {
              switch (AFT.Procesa_Sesion_Duplicada("Solicitud descarga Gmaster"))
              {
              case 0:
                IsSuccess = true;
                Msg = "Solicitud de cierre recibida con exito";
                break;

              case 2:
                IsSuccess = false;
                Msg = "Tarjeta ocupada o con transaccion pendiente";
                break;

              case 1:
                IsSuccess = false;
                Msg = "Error procesando la solicitud de descarga";
                break;

              default:
                IsSuccess = false;
                Msg = "Error procesando la solicitud de descarga";
                break;
              }
            }
            else
            {
              IsSuccess = false;
              Msg = "Tarjeta ocupada o con transaccion pendiente";
            }
          }
          else
          {
            Info_Cashless.Close_Player_Tracking_Sesion(true);
            Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
            // Info_Cashless.Reader_Lock(false);
            contadores.Close_ID_Client_Transaccion();
            IsSuccess = true;
            Msg = "Solicitud de cierre de sesion fidelizacion en maquina cashless recibida con exito";
          }
        }
        else
        {

          Info_Cashless.Close_Player_Tracking_Sesion(true);
          Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
          // Info_Cashless.Reader_Lock(false);
          contadores.Close_ID_Client_Transaccion();
          IsSuccess = true;
          Msg = "Solicitud de cierre fidelizacion recibida con exito";
        }
      }
      else
      {
        IsSuccess = false;
        Msg = "No existe Sesion iniciada para procesar la solicitud";
      }
    }
    

    jsonDocument["IsSuccess"]=IsSuccess;
    jsonDocument["Message"] = Msg;
    jsonDocument["Ip"] = IP_toString_(Current_IP);
    jsonDocument["Fecha_Hora"] = DataTime;


    String Output;
    serializeJson(jsonDocument, Output); /* Serializa Data */
    request->send(200, "application/json", Output);
    Info_Cashless.Log(RTC,"COMANDO_SESION_DUPLICADA_RECIBIDO "+ipCliente.toString(),Msg);

  });


  Server_API.on("/api/Fidelizacion/Habilitar_Validacion_Falla_Cashless", HTTP_GET, [](AsyncWebServerRequest *request){

    IPAddress ipCliente = request->client()->remoteIP();
    DynamicJsonDocument jsonDocument(800);
    jsonDocument.clear();


    bool IsSuccess=false;
    String Msg;
    NVS.begin("Config_ESP32", false);
    NVS.putBool("fid_post_cash",true);
    bool fid_post_cash=NVS.getBool("fid_post_cash",false);
    NVS.end();

    if (fid_post_cash)
    {
      IsSuccess = true;
      Msg="Validacion Falla Cashless Habilitada correctamente";
      Variables_globales.Set_Variable_Global(Enable_Fidelizacion_After_Cashless_Fail, fid_post_cash);
    }
    else
    {
      IsSuccess = false;
      Msg="Validacion Falla Cashless no habilitada";
    }
      
    jsonDocument["IsSuccess"]=IsSuccess;
    jsonDocument[ "Message"] = Msg;

    String Output;
    serializeJson(jsonDocument, Output); /* Serializa Data */
    request->send(200, "application/json", Output);
    
    Info_Cashless.Log(RTC,"SOLICITUD_VALIDACION_CASHLESS_RECIBIDO "+ipCliente.toString(),Msg);
   
  });

  Server_API.on("/api/Fidelizacion/Deshabilitar_Validacion_Falla_Cashless", HTTP_GET, [](AsyncWebServerRequest *request){


    IPAddress ipCliente = request->client()->remoteIP();

    DynamicJsonDocument jsonDocument(800);
    jsonDocument.clear();
    
    bool IsSuccess=false;
    String Msg;
    NVS.begin("Config_ESP32", false);
    NVS.putBool("fid_post_cash",false);
    bool fid_post_cash=NVS.getBool("fid_post_cash",false);
    NVS.end();

    if (!fid_post_cash)
    {
      IsSuccess = true;
      Msg="Validacion Falla Cashless Deshabilitada correctamente";
      Variables_globales.Set_Variable_Global(Enable_Fidelizacion_After_Cashless_Fail, fid_post_cash);
    }
    else
    {
      IsSuccess = false;
      Msg="Validacion Falla Cashless no deshabilitada";
    }
      
    jsonDocument["IsSuccess"]=IsSuccess;
    jsonDocument[ "Message"] = Msg;

    String Output;
    serializeJson(jsonDocument, Output); /* Serializa Data */
    request->send(200, "application/json", Output);
    
    Info_Cashless.Log(RTC,"SOLICITUD_VALIDACION_CASHLESS_RECIBIDO "+ipCliente.toString(),Msg);

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
  /* Metodo Web configura la informacion de ticket 
  Location: Nombre del Casino
  Adress_1: Direccion 1
  Adress_2: Direccion 2
  Restricted_Ticket:  Titulo para ticket restringidos
  Debit_Ticket_Title: Titulo para ticket debito
  Ticket Casheable default: Cashout Vaouncher
  */
  Server_API.addHandler(new AsyncCallbackJsonWebHandler("/Configura_Informacion_Ticket", [](AsyncWebServerRequest* request, JsonVariant& json) {
    


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

        Buffer_Cashless.Init_Buffer_TITO_7C();
        Flag_Set_Ticket_Data=true; /* Habilita Lectura */
        esp_task_wdt_init(1000000, true);
        esp_task_wdt_add(NULL);
        unsigned long Timout_Break;
        int Stop_Transaccion = 8000; // Tiempo de espera en milisegundos (15 Seg MAX)
        Timout_Break = millis();

        while ((Buffer_Cashless.Get_Buffer_TITO_7C()[1] == 0xAA && millis() - Timout_Break < Stop_Transaccion))
        {
          esp_task_wdt_reset();
          if(Buffer_Cashless.Get_Buffer_TITO_7C()[1]==0x7C||Buffer_Cashless.Get_Buffer_TITO_7C()[2]==0x01)
            break;
          //Serial.println("Esperando respuesta.....");
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
        jsonDocument[ "Message"]="Error en datos ";
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

  /* Metodo Web para deshabilitar el cobro por  modulo TITO */
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

  /* Metodo Web para habilitar el cobro por modulo TITO */
  Server_API.on("/Evento_57_Tito_Habilitado",HTTP_GET, [](AsyncWebServerRequest *request){
    
    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)<4||Configuracion.Get_Configuracion(Tipo_Maquina, 0)==17)
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

  /* Metodo Web Envia contadores  TITO */
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
  /* Metodo Web Habilita el modulo TITO */
  Server_API.on("/Habilitar_Tito",HTTP_GET, [](AsyncWebServerRequest *request){



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
        case 4:
          jsonDocument["Tipo_Maq"] = "IRT";
          break;
        case 5:
          jsonDocument["Tipo_Maq"] = "Generica";
          break;
        case 6:
          jsonDocument["Tipo_Maq"] = "Poker";
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
  /* Metodo Web Deshabilita el modulo TITO */
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

  /* Metodo Web para descargar archivo de transacciones pendiente modulo TITO */
  Server_API.on("/Transferencias_Tito", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/Ticket.txt", "text/plain", true);
    response->addHeader("Txt", "Transferencias Pendientes");
    request->send(response);
  });

  Server_API.addHandler(new AsyncCallbackJsonWebHandler("/api/TITO/Configuracion_Servidor", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                        {
    StaticJsonDocument<300> jsonDocument;

            // Validar JSON
    if (!json.is<JsonObject>())
    {
      jsonDocument["IsSuccess"] = false;
      jsonDocument["Message"] = "JSON invalido";

      String response;
      serializeJson(jsonDocument, response);

      request->send(200, "application/json", response);
      return;
    }

    JsonObject data = json.as<JsonObject>();

    // Validar keys
    if (!data.containsKey("Puerto") ||
        !data.containsKey("Ip_Servidor"))
    {
      jsonDocument["IsSuccess"] = false;
      jsonDocument["Message"] = "Faltan parametros";

      String response;
      serializeJson(jsonDocument, response);

      request->send(200, "application/json", response);
      return;
    }

    uint16_t Puerto = data["Puerto"].as<uint16_t>();
    String Ip_Servidor = data["Ip_Servidor"].as<String>();

    uint8_t ip[4];

    // Validar IP
    if (sscanf(Ip_Servidor.c_str(),
               "%hhu.%hhu.%hhu.%hhu",
               &ip[0],
               &ip[1],
               &ip[2],
               &ip[3]) != 4)
    {
      jsonDocument["IsSuccess"] = false;
      jsonDocument["Message"] = "IP invalida";

      String response;
      serializeJson(jsonDocument, response);

      request->send(200, "application/json", response);
      return;
    }

    // Mostrar IP
    Serial.printf("IP: %d.%d.%d.%d\n",
                  ip[0],
                  ip[1],
                  ip[2],
                  ip[3]);

    Serial.printf("Puerto: %u\n", Puerto);

    // Guardar en NVS
    NVS.begin("Config_ESP32", false);

    NVS.putUInt("Tito_Port", Puerto);
    NVS.putBytes("Tito_IP", ip, sizeof(ip));

    NVS.end();

    // Actualizar variables en RAM
    memcpy(Tito.Tito_Config_Manager.IP_Server_Tito,
           ip,
           sizeof(ip));

    Tito.Tito_Config_Manager.Port_Server_Tito = Puerto;

    jsonDocument["IsSuccess"] = true;
    jsonDocument["Message"] = "Configuracion guardada correctamente";
    jsonDocument["Puerto"] = Tito.Tito_Config_Manager.Port_Server_Tito;
    jsonDocument["Tito_IP"] = IP_toString_Ip((char*)Tito.Tito_Config_Manager.IP_Server_Tito);  

    String response;
    serializeJson(jsonDocument, response);

    request->send(200, "application/json", response); 
  }));

  Server_API.on("/api/TITO/Informacion_Configuracion", HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<200> jsonDocument;
    jsonDocument.clear();

    jsonDocument["IsSuccess"] = true;
    jsonDocument["Message"]="Informacion generada correctamente";
    jsonDocument["Enable_Tito"]=Variables_globales.Get_Variable_Global(Enable_Tito_Ticket);
    jsonDocument["Cobro_Tito"]=Variables_globales.Get_Variable_Global(Enable_Cashless);
    
    jsonDocument["Puerto"] = Tito.Tito_Config_Manager.Port_Server_Tito;
    jsonDocument["Tito_IP"] = IP_toString_Ip((char*)Tito.Tito_Config_Manager.IP_Server_Tito); 


    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json); 
  });

  /* Metodo Web configura la expiracion de ticket */
  Server_API.addHandler(new AsyncCallbackJsonWebHandler("/Configura_expiracion_ticket", [](AsyncWebServerRequest* request, JsonVariant& json) {

    
    int Code=0x100;
    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
  
    if (!json.is<JsonObject>()) {

      jsonDocument["IsSuccess"] = false;
      jsonDocument["Mesagge"] = "Tipo de dato no identificado JSON";
      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);

      return;
    }else
    {

      auto&& data = json.as<JsonObject>();

      if(!data.containsKey("Cashable_ticket_expiration") ||!data.containsKey("Restricted_ticket_expiration") )
      {
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Mesagge"] = "Tiempo de expiracion no soportado (mayor a 9999 dias)";
        String Json;
        serializeJson(jsonDocument, Json); /* Serializa Data */
        request->send(200, "application/json", Json);
      }else{
        uint32_t Cashable_ticket_expiration = data["Cashable_ticket_expiration"].as<uint32_t>();
        uint32_t Restricted_ticket_expiration = data["Restricted_ticket_expiration"].as<uint32_t>();

        if(Cashable_ticket_expiration>9999||Restricted_ticket_expiration>9999)
        {
          jsonDocument["IsSuccess"] = false;
          jsonDocument["Mesagge"] = "Tiempo de expiracion no soportado (mayor a 9999 dias)";
          String Json;
          serializeJson(jsonDocument, Json); /* Serializa Data */
          request->send(200, "application/json", Json);
          return;
        }
        else
        {


          if(!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
          {
            jsonDocument["IsSuccess"] = false;
            jsonDocument["Mesagge"] = "No hay comunicaciion con la MET";
            String Json;
            serializeJson(jsonDocument, Json); /* Serializa Data */
            request->send(200, "application/json", Json);
            return;
          }

          Buffer_Cashless.Init_Buffer_TITO_Data();
          char Expiration_Cashable[2];
          char Expiration__Restricted[2];
         

          // Calcular los dos bytes en formato BCD
          Expiration_Cashable[1] = ((Cashable_ticket_expiration % 10) | ((Cashable_ticket_expiration / 10 % 10) << 4));         // Byte bajo: últimos 2 dígitos
          Expiration_Cashable[0] = ((Cashable_ticket_expiration / 100 % 10) | ((Cashable_ticket_expiration / 1000 % 10) << 4)); // Byte alto: primeros 2 dígitos


          // Calcular los dos bytes en formato BCD
          Expiration__Restricted[1] = ((Restricted_ticket_expiration % 10) | ((Restricted_ticket_expiration / 10 % 10) << 4));         // Byte bajo: últimos 2 dígitos
          Expiration__Restricted[0] = ((Restricted_ticket_expiration / 100 % 10) | ((Restricted_ticket_expiration / 1000 % 10) << 4)); // Byte alto: primeros 2 dígitos
          

          char Buffer[4];
          // Expiración de tickets Cashables 
          Buffer[0] = Expiration_Cashable[0];
          Buffer[1] = Expiration_Cashable[1];
          // Expiración de tickets Restricted
          Buffer[2] = Expiration__Restricted[0];
          Buffer[3] = Expiration__Restricted[1];
          
          Buffer_Cashless.Set_Buffer_TITO_Data(Buffer);

          unsigned long Timout_Break_Response;
          int Stop_Transaccion_Amount_Response = 5000; // Tiempo de espera en milisegundos (2 Seg MAX)
          Timout_Break_Response = millis();
          esp_task_wdt_init(1000000, true);
          esp_task_wdt_add(NULL);

          Extended_Ticket_Command=true; /* Envia comando */
          Solicitud_Expiracion_Ticket=true;

          
          while ((Buffer_Cashless.Get_Buffer_TITO_7B()[1] == 0xAA || Buffer_Cashless.Get_Buffer_TITO_7B()[1] == 0x00) && (millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response))
          {
            esp_task_wdt_reset();

            if (Buffer_Cashless.Get_Buffer_TITO_7B()[1] != 0xAA && Buffer_Cashless.Get_Buffer_TITO_7B()[1] != 0x00)
              break;
            vTaskDelay(300);
             //Serial.println("Esperando repuesta de la maquina.....!");
          }
          // Serial.println(Buffer_Cashless.Get_Buffer_TITO_7B()[1],DEC);
          // Serial.println(Buffer_Cashless.Get_Buffer_TITO_7B()[9],DEC);
          // Serial.println(Buffer_Cashless.Get_Buffer_TITO_7B()[10],DEC);
          if(Buffer_Cashless.Get_Buffer_TITO_7B()[1]==0x7B)
          {
            //Serial.println(" Comando Recibido por la maquina...");
            
            uint32_t Cashable_ticket_exp = 0;
            uint32_t Restricted_ticket_exp = 0;

            // Extraer dígitos del formato BCD
            Cashable_ticket_exp += ((Buffer_Cashless.Get_Buffer_TITO_7B()[9] >> 4) & 0x0F) * 1000; // Dígito más significativo del byte alto
            Cashable_ticket_exp += (Buffer_Cashless.Get_Buffer_TITO_7B()[9] & 0x0F) * 100;         // Segundo dígito del byte alto
            Cashable_ticket_exp += ((Buffer_Cashless.Get_Buffer_TITO_7B()[10] >> 4) & 0x0F) * 10;   // Dígito más significativo del byte bajo
            Cashable_ticket_exp += (Buffer_Cashless.Get_Buffer_TITO_7B()[10] & 0x0F);               // Segundo dígito del byte bajo

            // Extraer dígitos del formato BCD
            Restricted_ticket_exp += ((Buffer_Cashless.Get_Buffer_TITO_7B()[11] >> 4) & 0x0F) * 1000; // Dígito más significativo del byte alto
            Restricted_ticket_exp += (Buffer_Cashless.Get_Buffer_TITO_7B()[11] & 0x0F) * 100;         // Segundo dígito del byte alto
            Restricted_ticket_exp += ((Buffer_Cashless.Get_Buffer_TITO_7B()[12] >> 4) & 0x0F) * 10;   // Dígito más significativo del byte bajo
            Restricted_ticket_exp += (Buffer_Cashless.Get_Buffer_TITO_7B()[12] & 0x0F);               // Segundo dígito del byte bajo

            // Serial.println("----------------------------------------");
            // Serial.println(Cashable_ticket_exp);
            // Serial.println(Restricted_ticket_exp);
            // Serial.println("----------------------------------------");

            // Serial.println("----------------------------------------");
            // Serial.println(Cashable_ticket_expiration);
            // Serial.println(Restricted_ticket_expiration);
            // Serial.println("----------------------------------------");

            if(Cashable_ticket_expiration==Cashable_ticket_exp && Restricted_ticket_expiration == Restricted_ticket_exp)
              jsonDocument["IsSuccess"] = true;

            else if(Cashable_ticket_expiration==0 && Restricted_ticket_expiration==0)
              jsonDocument["IsSuccess"] = true;
            else
              jsonDocument["IsSuccess"] = false;

            jsonDocument["Cashable_ticket_expiration"] = Cashable_ticket_exp;
            jsonDocument["Restricted_ticket_expiration"] = Restricted_ticket_exp;

            if(Cashable_ticket_expiration==0 && Restricted_ticket_expiration==0)
              jsonDocument["Mesagge"] = "Informacion generada correctamente";
            else
              jsonDocument["Mesagge"] = "Configuracion aplicada correctamente";
          }else{

            //Serial.println(" Comando no recibido....");
            jsonDocument["IsSuccess"] = false;
            jsonDocument["Cashable_ticket_expiration"] = nullptr;
            jsonDocument["Restricted_ticket_expiration"] = nullptr;
            jsonDocument["Mesagge"] = "No se aplico  la configuracion";
          }
          Buffer_Cashless.Init_Buffer_TITO_7B();
          Solicitud_Expiracion_Ticket=false;
          String Json;
          serializeJson(jsonDocument, Json); /* Serializa Data */
          jsonDocument.clear();
          
          request->send(200, "application/json", Json);
        }
      }                                      
    }
    }));

  /* Metodo Web configura parametros para habilitar/deshabilitar impresora para modulo TITO */
  Server_API.addHandler(new AsyncCallbackJsonWebHandler("/Config",[](AsyncWebServerRequest*request,JsonVariant& json)
  {

    StaticJsonDocument<800> jsonDocument;
    jsonDocument.clear();
  
    if (!json.is<JsonObject>()) {

      jsonDocument["IsSuccess"] = false;
      jsonDocument["Mesagge"] = "Tipo de dato no identificado JSON";
      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);

      return;
    }else{

      auto&& data = json.as<JsonObject>();

      if(data.containsKey("Printer_As_Cashout") && data.containsKey("Printer_As_Handpay")&& data.containsKey("Validate_Handpay")&&data.containsKey("Print_Restricted_Ticket")&&data.containsKey("Tickets_For_Foreign")&&data.containsKey("Ticket_Redemption"))
      {

        bool Use_Printer_As_Cashout_Device=data["Printer_As_Cashout"].as<bool>();
        bool Use_Printer_As_handpay_Receipt_Device=data["Printer_As_Handpay"].as<bool>();
        bool Validate_Handpay=data["Validate_Handpay"].as<bool>();
        bool Print_Restricted_Ticket=data["Print_Restricted_Ticket"].as<bool>();
        bool Tickets_For_Foreign=data["Tickets_For_Foreign"].as<bool>();
        bool Ticket_Redemption=data["Ticket_Redemption"].as<bool>();

     
        uint8_t LSB = 0x00;
        uint8_t MSB = 0x00;

        // Asignar los valores booleanos a los bits correspondientes
        LSB |= (Use_Printer_As_Cashout_Device << 0);         // Bit 0
        LSB |= (Use_Printer_As_handpay_Receipt_Device << 1); // Bit 1
        LSB |= (Validate_Handpay << 2);                      // Bit 2
        LSB |= (Print_Restricted_Ticket << 3);               // Bit 3
        LSB |= (Tickets_For_Foreign << 4);                   // Bit 4
        LSB |= (Ticket_Redemption << 5);                     // Bit 5
       

        // Los bits 6-7 permanecen en 0, no es necesario modificarlos
        Serial.printf("Resultado: 0x%02X\n", LSB); // Imprime el resultado, en este


        char Config_Two_Bytes[2];
        Config_Two_Bytes[0]=LSB;
        Config_Two_Bytes[1]=MSB;

        jsonDocument["IsSuccess"] = true;
        jsonDocument["Printer_As_Cashout"] = Use_Printer_As_Cashout_Device;
        jsonDocument["Printer_As_Handpay"] = Use_Printer_As_handpay_Receipt_Device;
        jsonDocument["Validate_Handpay"] = Validate_Handpay;
        jsonDocument["Print_Restricted_Ticket"] = Print_Restricted_Ticket;
        jsonDocument["Tickets_For_Foreign"] = Tickets_For_Foreign;
        jsonDocument["Ticket_Redemption"] = Ticket_Redemption;
        jsonDocument["Mesagge"] = "Configuracion aplicada correctamente";


        
        String Json;
        serializeJson(jsonDocument, Json); /* Serializa Data */
        request->send(200, "application/json", Json);
      }else{
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Mesagge"] = "Tiempo de expiracion no soportado (mayor a 9999 dias)";
        String Json;
        serializeJson(jsonDocument, Json); /* Serializa Data */
        request->send(200, "application/json", Json);
      }
    }

  }));


  // Server_API.addHandler(new AsyncCallbackJsonWebHandler("/Configura_Expiracion_Ticket",[](AsyncWebServerRequest*request,JsonVariant& json){

  //   StaticJsonDocument<800> jsonDocument;
  //   jsonDocument.clear();


  //   String  Mi_Mi= data["Ssid"].as<String>();

  //   String Json;
  //   serializeJson(jsonDocument, Json); /* Serializa Data */
  //   request->send(200, "application/json", Json);
  //   return;
  // }));

 

  /*********************************************************************************************************/
  /**********************************************PREMIOS SAS************************************************/
  /*********************************************************************************************************/

  /* Metodo Web Habilita el envio de premios SAS */
  Server_API.on("/Habilitar_Premios_SAS", HTTP_GET, [](AsyncWebServerRequest *request)
  {

    String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

    StaticJsonDocument<200> jsonDocument;
    jsonDocument.clear();

    NVS.begin("Config_ESP32", false);

    /* Actualiza valor en memoria Flash */
    NVS.putBool("P_SAS", true);
    bool Test = NVS.getBool("P_SAS", false);
    /* Actualiza valor  en RAM */
    Variables_globales.Set_Variable_Global(Handle_Premios_SAS, Test);
    NVS.end();


    if(Test && Variables_globales.Get_Variable_Global(Handle_Premios_SAS))
    {
      jsonDocument["IsSuccess"] = true;
    }else{
      jsonDocument["IsSuccess"] = false;
    }

    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Handle_Premios_SAS);
    jsonDocument["Desc"] = "Premios SAS Habilitados correctamente";

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);
    if(Test && Variables_globales.Get_Variable_Global(Handle_Premios_SAS))
    {
      delay(800);
      ESP.restart();
    }
  });

  /* Metodo Web deshabilita el envio de premios SAS */
  Server_API.on("/Inhabilitar_Premios_SAS", HTTP_GET, [](AsyncWebServerRequest *request)
  {

    String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

    StaticJsonDocument<200> jsonDocument;
    jsonDocument.clear();

    NVS.begin("Config_ESP32", false);

    /* Actualiza valor en memoria Flash */
    NVS.putBool("P_SAS", false);
    bool Test = NVS.getBool("P_SAS", false);
    /* Actualiza valor  en RAM */
    Variables_globales.Set_Variable_Global(Handle_Premios_SAS, Test);
    NVS.end();


    if(!Test && !Variables_globales.Get_Variable_Global(Handle_Premios_SAS))
    {
      jsonDocument["IsSuccess"] = true;
    }else{
      jsonDocument["IsSuccess"] = false;
    }

    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Estado"] = Variables_globales.Get_Variable_Global(Handle_Premios_SAS);
    jsonDocument["Desc"] = "Premios SAS Inhabilitados correctamente";

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);
   
  });

  Server_API.on("/Formatear_Sistema_Archivos",HTTP_GET,[](AsyncWebServerRequest*request)
  {
    String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());
      
    StaticJsonDocument<200> jsonDocument;
    jsonDocument.clear();

    if(SPIFFS.format())
      jsonDocument["IsSuccess"] = true;
    else
      jsonDocument["IsSuccess"] = false;

      
    
  });

  Server_API.on("/Premios_SAS_Pendientes", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if (Variables_globales.Get_Variable_Global(Handle_Premios_SAS))
    {
      if (SPIFFS.exists("/Premios_SAS.txt"))
      {
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/Premios_SAS.txt", "text/plain", true);
        response->addHeader("Txt", "Premios SAS Pendientes");
        request->send(response);
      }else{
        request->send(404, "text/plain", "Archivo no existe");
      }
    }else{
      request->send(404, "text/plain", "Premios SAS no Habilitados");
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

    Server_API.on("/Encuesta_Transaccion_Pendiente",HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
    bool IsSuccess=false;
    String Msg="";
    int status_code=AFT.GET_STATUS_TRANSFER();

    if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
    {


      

      IsSuccess=true;
      if (AFT.GET_STATUS_TRANSFER()==TransaccionCashless::TRANS_TIMEOUT)
      {
        switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
        {
        case 0:
          jsonDocument["Tipo_Maq"] = "AFT";
          App = true;
          Msg = "Proceso de consulta transaccion AFT iniciado con exito!";
          break;
        case 1:
          jsonDocument["Tipo_Maq"] = "AFT";
          App = true;
          Msg = "Proceso de consulta transaccion AFT iniciado con exito!";
          break;
        case 2:
          jsonDocument["Tipo_Maq"] = "EFT";
          // if (Variables_globales.Get_Variable_Global(Flag_Sesion_RFID) || Info_Cashless.Type_Sesion() == PLAYER_CASHLESS_SESION)
          // {
          //   Variables_globales.Set_Variable_Global(Event_Dowmload_Cashless_Pending, true);
          // }
          // else
          // {
          //   Variables_globales.Set_Variable_Global(Event_Load_Cashless_Pending, true);
          // }
          // Msg = "Proceso de consulta transaccion EFT iniciado con exito!";
          status_code=TransaccionCashless::TRANS_IDLE;

          Msg = "El tipo de maquina no es compatible para esta consulta";
          break;
        case 3:
          jsonDocument["Tipo_Maq"] = "AFT";
          App = true;
          Msg = "Proceso de consulta transaccion AFT iniciado con exito!";
          break;

        case 17:
          jsonDocument["Tipo_Maq"] = "EFT";
          Msg = "El tipo de maquina no es compatible para esta consulta";
          status_code=TransaccionCashless::TRANS_IDLE;
          break;

        default:
          jsonDocument["Tipo_Maq"] = "";
          Msg = "El tipo de maquina no es compatible para esta consulta";
          status_code=TransaccionCashless::TRANS_IDLE;
          break;
        }
      }
      else
      {

        /* No exite transaccion pendiente.*/
        App = false;
        Msg = "No existe transaccion pendiente de consulta";


        // switch (AFT.GET_STATUS_TRANSFER())
        // {

        // case TransaccionCashless::TRANS_IDLE:
        //   Msg = "No existe transaccion pendiente de consulta";
        //   break;

        // case TransaccionCashless::TRANS_RECIBIDA:
        //   Msg = "Transaccion recibida";
        //   break;

        // case TransaccionCashless::TRANS_EN_PROGRESO:
        //   Msg = "Transaccion en proceso";
        //   break;

        // case TransaccionCashless::TRANS_PENDIENTE:
        //   Msg = "Transaccion pendiente";
        //   break;

        // case TransaccionCashless::TRANS_FINALIZADA:
        //   Msg = "Transaccion finalizada";
        //   break;

        // default:
        //   Msg = "En estado no identificado";
        //   break;
        // }
      }
    }
    else
    {
      IsSuccess = false;
      Msg = "No hay comunicacion con la MET";
    }

    jsonDocument["IsSuccess"] = IsSuccess;
    jsonDocument["Status"] = status_code;
    jsonDocument["Message"] = Msg;
    //jsonDocument["Desc"] = "Reset Globus IM ESP32 procesado";
    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);
    delay(1000);
  });


  Server_API.on("/Desbloqueo_Transaccion_en_progreso",HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
    bool IsSuccess=false;
    String Msg="";
    int status_code=AFT.GET_STATUS_TRANSFER();

    if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
    {
      App=true;
    }
    else
    {
      IsSuccess = false;
      Msg = "No hay comunicacion con la MET";
    }

    jsonDocument["IsSuccess"] = IsSuccess;
    jsonDocument["Status"] = status_code;
    jsonDocument["Message"] = Msg;
    //jsonDocument["Desc"] = "Reset Globus IM ESP32 procesado";
    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);
    delay(1000);
  });

  /* En pruebas nuevo desarrollo ---->*/
  /* Configuración de datos */
  Server_API.addHandler(new AsyncCallbackJsonWebHandler("/Configuracion_Parametros_red", [](AsyncWebServerRequest* request, JsonVariant& json) {
    

    bool Cambios=false;
    StaticJsonDocument<800> jsonDocument;
    jsonDocument.clear();


    if (!json.is<JsonObject>()) {


      jsonDocument["IsSuccess"] = false;

      jsonDocument["Ssid"] = nullptr;
      jsonDocument["Passwarod"] = nullptr;
      jsonDocument["Local_Ip"] = nullptr;
      jsonDocument["Port"] = nullptr;

      jsonDocument["Subnet_mask"] = nullptr;
      jsonDocument["Gateway"]=nullptr;
      jsonDocument["Primary_Dns"]=nullptr;
      jsonDocument["Secondary_Dns"]=nullptr;
      jsonDocument["Message"]="El tipo de dato no es un JSON";


      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);
      return;
    }

    NVS.begin("Config_ESP32", false);
    auto&& data = json.as<JsonObject>();

    String  SSID_UPDATE= data["Ssid"].as<String>();
    String  PASSWORD_UPDATE= data["Passwarod"].as<String>();
    String  LOCAL_IP_UPDATE= data["Local_Ip"].as<String>();
    String  PORT_UPDATE= data["Port"].as<String>();
    
    Serial.println(PORT_UPDATE);
    String  SUBNET_MASK_UPDATE= data["Subnet_mask"].as<String>();
    String  GATEWAY_UPDATE= data["Gateway"].as<String>();
    String  PRIMARY_DNS_UPDATE= data["Primary_Dns"].as<String>();
    String  SECONDARY_DNS_UPDATE= data["Secondary_Dns"].as<String>();

    IPAddress local_ip;
    IPAddress gateway;
    IPAddress subnet_mask;
    IPAddress primary_dns;
    IPAddress secondary_dns;

    if (data.containsKey("Port") && !data["Port"].isNull())
    {
      /* PUERTO UDP */
      uint16_t CURRENT_PORT = Configuracion.Get_Configuracion(Puerto_Server, 0);
      if (CURRENT_PORT != PORT_UPDATE.toInt())
      {
        uint16_t PORT_Up = PORT_UPDATE.toInt();
        Serial.println("Puerto diferente...");
        NVS.putUInt("Socket", PORT_Up);
        Configuracion.Set_Configuracion_ESP32(Puerto_Server, PORT_Up);
        Cambios = true;
      }
    }

    /* SSID */

    if (data.containsKey("Ssid")&& !data["Ssid"].isNull())
    {
      String CURRENT_SSID = Configuracion.Get_Configuracion(SSID, "Nombre_Red");
      if (CURRENT_SSID != SSID_UPDATE)
      {
        Serial.println("SSID diferente...");
        NVS.putString("SSID_DESA", SSID_UPDATE);
        Configuracion.Set_Configuracion_ESP32(SSID, SSID_UPDATE);
        Cambios = true;
      }
    }

    /*PASSWORD*/

    if (data.containsKey("Passwarod")&& !data["Passwarod"].isNull())
    {
      String CURRENT_PASWORD = Configuracion.Get_Configuracion(Password, "Password_red");
      if (CURRENT_PASWORD != PASSWORD_UPDATE)
      {
        Serial.println("PASSWORD diferente...");
        NVS.putString("PASS_DESA", PASSWORD_UPDATE);
        Configuracion.Set_Configuracion_ESP32(Password, PASSWORD_UPDATE);
        Cambios = true;
      }
    }
    
    /* IP LOCAL */

    char IP_Local_[4];
    local_ip.fromString(LOCAL_IP_UPDATE);
    gateway.fromString(GATEWAY_UPDATE);
    subnet_mask.fromString(SUBNET_MASK_UPDATE);
    primary_dns.fromString(PRIMARY_DNS_UPDATE);
    secondary_dns.fromString(SECONDARY_DNS_UPDATE);
    

    memcpy(IP_Local_, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(IP_Local_) / sizeof(IP_Local_[0]));

    if (data.containsKey("Local_Ip")&& !data["Local_Ip"].isNull())
    {
      if (local_ip[0] != IP_Local_[0] || local_ip[1] != IP_Local_[1] || local_ip[2] != IP_Local_[2] || local_ip[3] != IP_Local_[3])
      {
        Serial.println("IP LOCAL  diferente...");
        uint8_t ip_local_temp[] = {local_ip[0], local_ip[1], local_ip[2], local_ip[3]};
        NVS.putBytes("Dir_IP", ip_local_temp, sizeof(ip_local_temp));

        /* Actualiza */
        IP_Local_[0] = ip_local_temp[0];
        IP_Local_[1] = ip_local_temp[1];
        IP_Local_[2] = ip_local_temp[2];
        IP_Local_[3] = ip_local_temp[3];
        Configuracion.Set_Configuracion_ESP32(Direccion_IP, IP_Local_);
        Cambios = true;
      }
    }

    char DNS_Primary_Starage[4];
    memcpy(DNS_Primary_Starage, Configuracion.Get_Configuracion(Dns_One_IP, 'x'), sizeof(DNS_Primary_Starage) / sizeof(DNS_Primary_Starage[0]));

    if (data.containsKey("Primary_Dns")&& !data["Primary_Dns"].isNull())
    {
      if (primary_dns[0] != DNS_Primary_Starage[0] || primary_dns[1] != DNS_Primary_Starage[1] || primary_dns[2] != DNS_Primary_Starage[2] || primary_dns[3] != DNS_Primary_Starage[3])
      {
        Serial.println("DNS 1 diferente...");
        uint8_t ip_dns_update[] = {primary_dns[0], primary_dns[1], primary_dns[2], primary_dns[3]};
        NVS.putBytes("Dns_Primary", ip_dns_update, sizeof(ip_dns_update));
        DNS_Primary_Starage[0] = ip_dns_update[0];
        DNS_Primary_Starage[1] = ip_dns_update[1];
        DNS_Primary_Starage[2] = ip_dns_update[2];
        DNS_Primary_Starage[3] = ip_dns_update[3];
        Configuracion.Set_Configuracion_ESP32(Dns_One_IP, DNS_Primary_Starage);
        Cambios = true;
      }
    }

    char DNS_Secondary_Starage[4];
    memcpy(DNS_Secondary_Starage, Configuracion.Get_Configuracion(Dns_Two_IP, 'x'), sizeof(DNS_Secondary_Starage) / sizeof(DNS_Secondary_Starage[0]));

    if (data.containsKey("Secondary_Dns")&& !data["Secondary_Dns"].isNull())
    {
      if (secondary_dns[0] != DNS_Secondary_Starage[0] || secondary_dns[1] != DNS_Secondary_Starage[1] || secondary_dns[2] != DNS_Secondary_Starage[2] || secondary_dns[3] != DNS_Secondary_Starage[3])
      {
        Serial.println("DNS 2 diferente...");
        uint8_t ip_dns_sec_update[] = {secondary_dns[0], secondary_dns[1], secondary_dns[2], secondary_dns[3]};
        NVS.putBytes("Dns_Secondary", ip_dns_sec_update, sizeof(ip_dns_sec_update));
        DNS_Secondary_Starage[0] = ip_dns_sec_update[0];
        DNS_Secondary_Starage[1] = ip_dns_sec_update[1];
        DNS_Secondary_Starage[2] = ip_dns_sec_update[2];
        DNS_Secondary_Starage[3] = ip_dns_sec_update[3];
        Configuracion.Set_Configuracion_ESP32(Dns_Two_IP, DNS_Secondary_Starage);
        Cambios = true;
      }
    }

    char IP_GW_Storage[4];
    memcpy(IP_GW_Storage, Configuracion.Get_Configuracion(Direccion_IP_GW, 'x'), sizeof(IP_GW_Storage) / sizeof(IP_GW_Storage[0]));

    if (data.containsKey("Gateway") && !data["Gateway"].isNull())
    {
      /* Verifica GW*/
      if (gateway[0] != IP_GW_Storage[0] || gateway[1] != IP_GW_Storage[1] || gateway[2] != IP_GW_Storage[2] || gateway[3] != IP_GW_Storage[3])
      {
        Serial.println("PUERTA ENLACE diferente...");
        uint8_t ip_gw_upd[] = {gateway[0], gateway[1], gateway[2], gateway[3]};
        NVS.putBytes("Dir_IP_GW", ip_gw_upd, sizeof(ip_gw_upd));
        IP_GW_Storage[0] = ip_gw_upd[0];
        IP_GW_Storage[1] = ip_gw_upd[1];
        IP_GW_Storage[2] = ip_gw_upd[2];
        IP_GW_Storage[3] = ip_gw_upd[3];
        Configuracion.Set_Configuracion_ESP32(Direccion_IP_GW, IP_GW_Storage);
        Cambios = true;
      }
    }

    char Submask_[4];
    memcpy(Submask_, Configuracion.Get_Configuracion(Direccion_SN_MASK, 'x'), sizeof(Submask_) / sizeof(Submask_[0]));

    if (data.containsKey("Subnet_mask")&& !data["Subnet_mask"].isNull())
    {
      if (subnet_mask[0] != Submask_[0] || subnet_mask[1] != Submask_[1] || subnet_mask[2] != Submask_[2] || subnet_mask[3] != Submask_[3])
      {
        Serial.println("MASCARA SUBRED diferente...");
        uint8_t Submask__[] = {subnet_mask[0], subnet_mask[1], subnet_mask[2], subnet_mask[3]};
        NVS.putBytes("Dir_SN_MASK", Submask__, sizeof(Submask__));

        Submask_[0] = Submask__[0];
        Submask_[1] = Submask__[1];
        Submask_[2] = Submask__[2];
        Submask_[3] = Submask__[3];
        Configuracion.Set_Configuracion_ESP32(Direccion_SN_MASK, Submask_);
        Cambios = true;
      }
    }

    NVS.end();

    if(Cambios)
      jsonDocument["IsSuccess"] = true;
    else
      jsonDocument["IsSuccess"] = false;

    jsonDocument["Ssid"] = Configuracion.Get_Configuracion(SSID, "Nombre_Red");
    jsonDocument["Passwarod"] = Configuracion.Get_Configuracion(Password, "Password_red");
    jsonDocument["Local_Ip"] = IP_toString_A(Configuracion.Get_Configuracion(Direccion_IP, 'x'));
    jsonDocument["Port"] = Configuracion.Get_Configuracion(Puerto_Server, 0);

    jsonDocument["Subnet_mask"] = IP_toString_A(Configuracion.Get_Configuracion(Direccion_SN_MASK, 'x'));
    jsonDocument["Gateway"] = IP_toString_A(Configuracion.Get_Configuracion(Direccion_IP_GW, 'x'));
    jsonDocument["Primary_Dns"] = IP_toString_A(Configuracion.Get_Configuracion(Dns_One_IP, 'x'));
    jsonDocument["Secondary_Dns"] = IP_toString_A(Configuracion.Get_Configuracion(Dns_Two_IP, 'x'));

    if(Cambios)
      jsonDocument["Message"] = "Configuración aplicada con exito";
    else
      jsonDocument["Message"] = "No existen cambios en la configuración";

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);
    delay(500);
    if(Cambios)
      ESP.restart();
  }));



 
  
  

  // Server_API.on("/Termina_Player_Tracking", HTTP_GET, [](AsyncWebServerRequest *request)
  // {

  //   int Code=0;

  //   StaticJsonDocument<200> jsonDocument;
  //   jsonDocument.clear();

  //   if (Info_Cashless.Type_Sesion() != PLAYER_CASHLESS_SESION)
  //   {
  //     if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
  //     {
  //       if (Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
  //       {
  //         Close_Sesion_Player_Tracking();
  //         if (contadores.Verify_Close(contadores.Get_Client_ID()))
  //         {
  //           Code = 0x00; /* OK */
  //           jsonDocument["IsSuccess"] = true;
  //           jsonDocument["Codigo"] = Code;
  //           jsonDocument["Message"] ="Sesion player tracking cerrada con exito";
  //         }
  //         else
  //         {
  //           /* ERROR EN INTERFAZ GLOBUS IM ESP32 */
  //           Code = 0x01; /* OK */
  //           jsonDocument["IsSuccess"] = false;
  //           jsonDocument["Codigo"] = Code;
  //           jsonDocument["Message"] ="Error en validacion de cierre";
  //         }
  //       }
  //       else
  //       {
  //         Code = 0x02; /* NO TIENE SESION PLAYER TRACKING ABIERTA */
  //         jsonDocument["IsSuccess"] = false;
  //         jsonDocument["Codigo"] = Code;
  //         jsonDocument["Message"] ="No existe sesion player tracking";
  //       }
  //     }else{
  //       Code=0x03; /* NO HAY COMUNICACIÓN CON LA MET */
  //       jsonDocument["IsSuccess"] = false;
  //       jsonDocument["Codigo"] = Code;
  //       jsonDocument["Message"] ="No hay comunicación con la MET";
  //     }
  //   }
  //   else
  //   {
  //     /* EN SESION CASHLESS NO PUEDE ATENDER LA SOLICITUD */
  //     Code=0x04;
  //     jsonDocument["IsSuccess"] = false;
  //     jsonDocument["Codigo"] = Code;
  //     jsonDocument["Message"] ="Sesión cashless activa";
  //   }

  //   String Json;
  //   serializeJson(jsonDocument, Json); /* Serializa Data */
  //   request->send(200, "application/json", Json);

  // });

  
  Server_API.addHandler(new AsyncCallbackJsonWebHandler("/Actualizacion_Globus_IM_ESP32", [](AsyncWebServerRequest* request, JsonVariant& json) {

    
    int Code=0x100;
    StaticJsonDocument<200> jsonDocument;
    jsonDocument.clear();
    
    

    if (!json.is<JsonObject>()) {

      jsonDocument["IsSuccess"] = false;
      jsonDocument["Code"] = Code;
      jsonDocument["Message"] = "Tipo de dato no identificado JSON";
      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);

      return;
    }else
    {

      auto&& data = json.as<JsonObject>();
      /* Url Generica */

      if (data.containsKey("Url") && data.containsKey("Api_Token") && data.containsKey("Api_Bin") && data.containsKey("Api_Respuesta") && data.containsKey("Version_Act") && data.containsKey("Api_Version"))
      {

        char Current_IP[4];
        memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));
        IP_toString_Ip(Current_IP);

        std::string Ip = IP_toString_Ip(Current_IP);
        String Ip_Local = String(Ip.c_str());

        if (!Variables_globales.Get_Variable_Global(Updating_System))
        {

          String URL_Generic = data["Url"].as<String>();
          /* Url Api Genera token */
          String Api_Token = data["Api_Token"].as<String>();
          /*Url Descarga de archivo*/
          String Api_Bin = data["Api_Bin"].as<String>();
          /* URL Api de respuesta */
          String Api_Res = data["Api_Respuesta"].as<String>();
          /*Url Parametrizada para generar token */
          String Url_Completa = URL_Generic + Api_Token + "?" + "Mac=" + WiFi.macAddress()+"&Ip="+Ip_Local;
          /* Version de actualización */
          String Version_Programa = data["Version_Act"].as<String>();
          /* URL Api version */
          String Api_Version = data["Api_Version"].as<String>();
          esp_task_wdt_init(15000, true);
          esp_task_wdt_add(NULL);
          String Token_Valido = Token_Generator_Update(Url_Completa);
          esp_task_wdt_reset();
          //Serial.println(Token_Valido);
          if (Token_Valido != "")
          {
            /* Inicializa URL para descarga */
            UpdateOTA.Init_AutoUpdate(Version_Programa, URL_Generic, Api_Version, Api_Bin, Api_Res, Token_Valido, Version_Firmware_);
            
            Code = 0x00;
            jsonDocument["IsSuccess"] = true;
            jsonDocument["Code"] = Code;
            jsonDocument["Message"] = "Comando de actualización recibido con exito";
            
            String Json;
            serializeJson(jsonDocument, Json); /* Serializa Data */
            request->send(200, "application/json", Json);
            /* Si la tarjeta tiene conexion con Lector lo bloquea  */
            if (Variables_globales.Get_Variable_Global(Conexion_RFID))
            {
              Variables_globales.Set_Variable_Global(Updating_System, true);
              UpdateOTA.Timer_Update(480000); /* Timeout para desbloquear lector */
            }
            /* Inicia Actualizacion */
            Variables_globales.Set_Variable_Global(AutoUPDATE_OK, true);
            return;
          }
          else
          {
            Variables_globales.Set_Variable_Global(Updating_System, false);
            Code = 0x01;
            jsonDocument["IsSuccess"] = false;
            jsonDocument["Code"] = Code;
            jsonDocument["Message"] = "Error generando token de acceso";
            String Json;
            serializeJson(jsonDocument, Json); /* Serializa Data */
            request->send(200, "application/json", Json);
            return;
          }
        }
        else
        {
          Code = 0x03;
          jsonDocument["IsSuccess"] = false;
          jsonDocument["Code"] = Code;
          jsonDocument["Message"] = "Actualizacion actualmente en curso";
          String Json;
          serializeJson(jsonDocument, Json); /* Serializa Data */
          request->send(200, "application/json", Json);
          return;
        }
      }
      else
      {
        Code = 0x02;
        jsonDocument["IsSuccess"] = true;
        jsonDocument["Code"] = Code;
        jsonDocument["Message"] = "Falta uno o mas parametros en la URL";
        String Json;
        serializeJson(jsonDocument, Json); /* Serializa Data */
        request->send(200, "application/json", Json);
        return;
      }
    }
    
  }));


 


  Server_API.addHandler(new AsyncCallbackJsonWebHandler("/Solicitud_Reset_Handpay", [](AsyncWebServerRequest* request, JsonVariant& json) {

     IPAddress ipCliente = request->client()->remoteIP();

    char Current_IP[4];
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));
    
    if (!json.is<JsonObject>()) {

      StaticJsonDocument<200> jsonDocument;
      jsonDocument.clear();
      jsonDocument["IsSuccess"] =false;
      jsonDocument["Data"] = nullptr;
      jsonDocument["Ack"] = nullptr;
      jsonDocument["Ip"] = IP_toString_(Current_IP);
      jsonDocument["Message"]="La data no es un json ";
      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);
      Info_Cashless.Log(RTC, "COMANDO_RESET_HANDPAY_RECIBIDO "+ipCliente.toString(), "JSON_NO_ES_UN_OBJETO_VALIDO");
      DisplayTFT.Mensaje_TFT(
        "[Operador App]\nLa informacion recibida no es valida para la solicitud\n"
        "Por favor verifique",
        true);
      return;

    }else
    {
      auto&& data = json.as<JsonObject>();
      

      if(!data.containsKey("OperadorId"))
      {
        StaticJsonDocument<200> jsonDocument;
        jsonDocument.clear();
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Data"] = nullptr;
        jsonDocument["Ack"] = nullptr;
        jsonDocument["Ip"] = IP_toString_(Current_IP);
        jsonDocument["Message"] = "No existe la clave (OperadorId) en el json";
        String Json;
        serializeJson(jsonDocument, Json); /* Serializa Data */
        request->send(200, "application/json", Json);
        Info_Cashless.Log(RTC, "COMANDO_RESET_HANDPAY_RECIBIDO "+ipCliente.toString(), "NO_EXISTE_ID_OPERADOR");

        DisplayTFT.Mensaje_TFT(
        "[Operador App]\nNo existe tarjeta operador\n"
        "Por favor verifique",
        true);
        return; 
      }else
      {

        
        int Id_Op = data["OperadorId"].as<int>();
        //Serial.println(Id_Op);
        char Id_Op_Array[9];
        char Copia_Op[8];

        snprintf(Id_Op_Array, sizeof(Id_Op_Array), "%08d", Id_Op);

        Copia_Op[0]=Id_Op_Array[0];
        Copia_Op[1]=Id_Op_Array[1];
        Copia_Op[2]=Id_Op_Array[2];
        Copia_Op[3]=Id_Op_Array[3];
        Copia_Op[4]=Id_Op_Array[4];
        Copia_Op[5]=Id_Op_Array[5];
        Copia_Op[6]=Id_Op_Array[6];
        Copia_Op[7]=Id_Op_Array[7];

        contadores.Close_ID_Operador();
        contadores.Set_Id_Operador_Generico(Copia_Op);

      
        // Serial.print(int(contadores.Get_Operador_ID()[0]) - 48);
        // Serial.print(int(contadores.Get_Operador_ID()[1]) - 48);
        // Serial.print(int(contadores.Get_Operador_ID()[2]) - 48);
        // Serial.print(int(contadores.Get_Operador_ID()[3]) - 48);
        // Serial.print(int(contadores.Get_Operador_ID()[4]) - 48);
        // Serial.print(int(contadores.Get_Operador_ID()[5]) - 48);
        // Serial.print(int(contadores.Get_Operador_ID()[6]) - 48);
        // Serial.println(int(contadores.Get_Operador_ID()[7]) - 48);
      }
    }

    /* Retarda envio de premio por socket */
    Variable_Solicitud_Operador_Id=true;


    bool Handle_Encuesta=false;

    

    /* Contadores Antes del Premio */
    int Cancel_Credit_Inicial=contadores.Get_Contadores_Int(Total_Cancel_Credit);
    int Cancel_Credit_Handpay_Inicial=contadores.Get_Contadores_Int(Cancel_Credit_Hand_Pay);

    unsigned long Timout_Break_Response;
    int Stop_Transaccion_Amount_Response = 5000; // Tiempo de espera en milisegundos (2 Seg MAX)
    Timout_Break_Response = millis();
    esp_task_wdt_init(1000000, true);
    esp_task_wdt_add(NULL);

    int Ack=0x04;

    /* RESET HANDPAY SAS o RELE */
    if (Variables_globales.Get_Variable_Global(Conexion_RFID))
      Status_Barra(TARJETA_OPERADOR_INSERT);
    Reset_Handle_LED();

    

    if (Variables_globales.Get_Variable_Global(Type_Hanpay_Reset))
    {

      if (Variables_globales.Get_Variable_Global(Reset_Handpay_in_Process))
      {

        StaticJsonDocument<200> jsonDocument;
        jsonDocument.clear();
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Data"] = nullptr;
        jsonDocument["Ack"] = nullptr;
        jsonDocument["Ip"] = IP_toString_(Current_IP);
        jsonDocument["Message"] = "Dispositivo Ocupado  Reset Handpay en proceso";
        String Json;
        serializeJson(jsonDocument, Json); /* Serializa Data */
        request->send(200, "application/json", Json);

        Variable_Solicitud_Operador_Id=false;
        Info_Cashless.Log(RTC, "COMANDO_RESET_HANDPAY_RECIBIDO "+ipCliente.toString(), "RESET_HANDPAY_EN_PROCESO");

        return;
      }
      else
      {


        DisplayTFT.Mensaje_TFT(
        "[Operador App]\nVerificando condicion de pago..\n"
        "Por favor espere..",
        false);
        
        Variables_globales.Set_Variable_Global(Reset_Handpay_in_Process, true);

        if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 13 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 14 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 9 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 15||Variables_globales.Get_Variable_Global(Flag_Validacion_Creditos_Actuales))
        {

          if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 18)
            Handle_Encuesta = true;
          delay(10);
          digitalWrite(Unlock_Machine, LOW);
          delay(250);
          digitalWrite(Unlock_Machine, HIGH);

          unsigned long Timout_Break_Response_;
          int Stop_Transaccion_Amount_Response_ = 7000; // Tiempo de espera en milisegundos (10 Seg MAX)
          Timout_Break_Response_ = millis();

          delay(10);
          Variables_globales.Set_Variable_Global(Flag_Contadores_Sesion_ON,true);

          while (millis() - Timout_Break_Response_ < Stop_Transaccion_Amount_Response_)
          {
            if(contadores.Get_Status_Flag_Premio())
              break;

            esp_task_wdt_reset();
            vTaskDelay(150);
          }
        }
        else
        {
          delay(10);
          Creditos_Machine(); /* Encuesta creditos*/
          delay(100);
          int Creditos_Actuales = Convert_Char_To_Int10(contadores.Get_Contadores_Char(24));

          if (Creditos_Actuales <= 0)
          {
            /* No hay condición de reset */
            //Serial.println("Maquina no en condicion de pago..... ");
            Ack = 0x01;
            contadores.Close_ID_Operador();
            Handle_Encuesta = false;
            Status_Barra(ERROR_RESET_HANDPAY);
            Reset_Handle_LED();
            Variables_globales.Set_Variable_Global(Reset_Handpay_in_Process, false);

            DisplayTFT.Mensaje_TFT("[Operador App]\nNo existe condicion de pago creditos en 0",true);
          }

          else if (Creditos_Actuales > 0)
          {
            Handle_Encuesta = true;
            delay(10);
            digitalWrite(Unlock_Machine, LOW);
            delay(250);
            digitalWrite(Unlock_Machine, HIGH);
            // Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
            //Serial.println(" Condicion de pago OK ");
          }

          if (Handle_Encuesta)
          {

            unsigned long Timout_Break_Response_;
            int Stop_Transaccion_Amount_Response_ = 10000; // Tiempo de espera en milisegundos (10 Seg MAX)
            Timout_Break_Response_ = millis();

            while (millis() - Timout_Break_Response_ < Stop_Transaccion_Amount_Response_)
            {
              esp_task_wdt_reset();

              /* Simula LLave Maquina */
              digitalWrite(Unlock_Machine, LOW);
              delay(10);
              Creditos_Machine(); /* Encuesta creditos*/
              delay(100);
              digitalWrite(Unlock_Machine, HIGH);

              /*Consulta creditos */
              int Creditos_Actuales_ = Convert_Char_To_Int10(contadores.Get_Contadores_Char(24));
              delay(3);

              if (Creditos_Actuales_ == 0) /* Reset_Realizado con Exito*/
              {

                Ack = 0x00;
                /* Actualiza Maquinas */
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 15 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 6 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 14)
                {
                  if (Cancel_Credit_Inicial > 0 && Cancel_Credit_Handpay_Inicial > 0)
                    Flag_Critial_Questions = true;
                }

                if (Variables_globales.Get_Variable_Global(Conexion_RFID))
                  Status_Barra(Reset_Exitoso);
                Reset_Handle_LED();
                // delay(50);
                // Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO, true); /* Inicia Timer Operador */
                Variables_globales.Set_Variable_Global(Reset_Handpay_in_Process, false);
                Handle_Encuesta = false;

                DisplayTFT.Mensaje_TFT(
                    "[Operador App]\nPremio destildado correctamente\n"
                    "Actualizando contadores....",
                    true);
              }

              /* Si la maquina no comunica en medio del proceso se cancela */
              if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 15 && !Variables_globales.Get_Variable_Global(Comunicacion_Maq))
              {
                Ack = 0x04;
                if (Variables_globales.Get_Variable_Global(Conexion_RFID))
                  Status_Barra(ERROR_RESET_HANDPAY);
                Reset_Handle_LED();
                contadores.Close_ID_Operador();
                Handle_Encuesta = false;
                Variables_globales.Set_Variable_Global(Reset_Handpay_in_Process, false);

                DisplayTFT.Mensaje_TFT(
                    "[Operador App]\nNo se pudo destildar el premio\n"
                    "Verifique la conexion o estado de la maquina",
                    true);
              }

              if (!Handle_Encuesta)
              {
                esp_task_wdt_reset();
                break;
              }
              esp_task_wdt_reset();
              vTaskDelay(300);
              //Serial.println("Esperando descarga por rele.....");
            }

            /* Si despues de  que termine el tiempo de espera los creditos son >0 es decir no hubo reset
            Cancela la operacion por desconexion de rele o   maquina todavia en juego */
            int Creditos_Actuales_ = Convert_Char_To_Int10(contadores.Get_Contadores_Char(24));

            if (Creditos_Actuales_ > 0)
            {

              //Serial.println("Se Agoto el tiempo de espera ");
              Ack = 0x04;
              if (Variables_globales.Get_Variable_Global(Conexion_RFID))
                Status_Barra(ERROR_RESET_HANDPAY);
              Reset_Handle_LED();
              contadores.Close_ID_Operador();
              Handle_Encuesta = false;
              Variables_globales.Set_Variable_Global(Reset_Handpay_in_Process, false);

              DisplayTFT.Mensaje_TFT(
                  "[Operador App]\nNo se pudo destildar el premio\n"
                  "Verifique la conexion o estado de la maquina",
                  true);
            }
          }
        }
      }


      StaticJsonDocument<1024> jsonDocument;
      jsonDocument.clear();

      Buffer.Set_buffer_contadores_ACC_NO(3, contadores, RTC, Variables_globales);
      char res[258] = {};
      bzero(res, 258); // Pone el buffer en 0
      memcpy(res, Buffer.Get_buffer_contadores_ACC_NO(), 258);

      String convertedString = "";
      for (int i = 0; i < 258; i++)
      {
        convertedString += String(res[i]);
      }
      //Serial.println(convertedString);

      String Ack_Final = "";
      switch (Ack)
      {
      case 0x00:
        jsonDocument["IsSuccess"] = true;
        Ack_Final = "C0";
        break;

      case 0x01:
        jsonDocument["IsSuccess"] = false;
        Ack_Final = "C1";
        break;

      case 0x02:
        jsonDocument["IsSuccess"] = false;
        Ack_Final = "C2";
        break;

      case 0x04:
        jsonDocument["IsSuccess"] = true;
        Ack_Final = "C4";
        break;

      case 0x05:
        jsonDocument["IsSuccess"] = false;
        Ack_Final = "HI";
      break;

      default:
        jsonDocument["IsSuccess"] = false;
        Ack_Final = "C4";
        break;
      }

      jsonDocument["Data"] = convertedString;
      jsonDocument["Ack"] = Ack_Final;
      jsonDocument["Ip"] = IP_toString_(Current_IP);

      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      //Serial.println(Json);
      request->send(200, "application/json", Json);

      Variable_Solicitud_Operador_Id=false;
      Variables_globales.Set_Variable_Global(Reset_Handpay_in_Process,false);

    }
    else
    {

      DisplayTFT.Mensaje_TFT(
        "[Operador App]\nVerificando condicion de pago..\n"
        "Por favor espere..",
        false);

      unsigned long Timout_Break;
      int Stop_Transaccion = 5000; // Tiempo de espera en milisegundos (10 Seg MAX)
      
      /*----------------------------------------------------------------*/
      /* Solicitud Reset Handpay */
      Reset_HandPay(); /* Reset Premio por SAS*/
      delay(200);
      // while (Variables_globales.Get_Variable_Global_Char(Reset_Handay_OK) == 0x04 && (millis() - Timout_Break < Stop_Transaccion))
      // {
      //   esp_task_wdt_reset();
          
      //   vTaskDelay(300);
      //   //Serial.println("Esperando respuesta de comando....");
      // }
      /*----------------------------------------------------------------*/

      switch (Variables_globales.Get_Variable_Global_Char(Reset_Handay_OK))
      {
      case 0x00: /*Reset realizado con exito*/
        
        Flag_Critial_Questions = true;
        //Transmite_Confirmacion('C', '0');
        Variables_globales.Set_Variable_Global_Char(Reset_Handay_OK, 0x04);
        Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 0);
        Encuesta_Creditos_Premio();
       
        //Transmite_Contadores_Accounting();
        Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);

        if (Variables_globales.Get_Variable_Global(Conexion_RFID))
          Status_Barra(Reset_Exitoso);
        Reset_Handle_LED();
        Ack=0x00;
        Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO, true);

        DisplayTFT.Mensaje_TFT(
            "[Operador App]\nPremio destildado correctamente\n"
            "Actualizando contadores....",
            true);

        break;
      case 0x01: /*No existe condición de reset*/
        /* Guarda ID Operador */
        contadores.Close_ID_Operador();
        //Transmite_Confirmacion('C', '1');
        Variables_globales.Set_Variable_Global_Char(Reset_Handay_OK, 0x04);
        Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
        if (Variables_globales.Get_Variable_Global(Conexion_RFID))
          Status_Barra(ERROR_RESET_HANDPAY);
        Reset_Handle_LED();
        Ack=0x01;

        DisplayTFT.Mensaje_TFT("[Operador App]\nNo se pudo destildar el premio\n Por favor Revise conexion o estado de la maquina",true);

        break;
      case 0x02: /*Imposible realizar reset*/
        contadores.Close_ID_Operador();
        //Transmite_Confirmacion('C', '2');
        Variables_globales.Set_Variable_Global_Char(Reset_Handay_OK, 0x04);
        Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);

        if (Variables_globales.Get_Variable_Global(Conexion_RFID))
          Status_Barra(ERROR_RESET_HANDPAY);
        Reset_Handle_LED();

        Ack=0x02;

        DisplayTFT.Mensaje_TFT("[Operador App]\nLa maquina no tiene premio pendiente\n Por favor Verifique condicion de pago",true);
        break;
      case 0x04: /* No  hay respuesta de la maquina*/
        //Transmite_Confirmacion('C', '4');
        Variables_globales.Set_Variable_Global_Char(Reset_Handay_OK, 0x04);
        Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
        if (Variables_globales.Get_Variable_Global(Conexion_RFID))
          Status_Barra(ERROR_RESET_HANDPAY);
        Reset_Handle_LED();
        Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO, true);
        Ack=0x04;

        DisplayTFT.Mensaje_TFT(
            "[Operador App]\nNo se pudo destildar el premio\n"
            "Verifique la conexion o estado de la maquina",
            true);
        break;

      default:
        Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
        Variables_globales.Set_Variable_Global_Char(Reset_Handay_OK, 0x04);
        Reset_Handle_LED();
        Ack=0x04;

        DisplayTFT.Mensaje_TFT(
            "[Operador App]\nNo se pudo destildar el premio\n"
            "Verifique la conexion o estado de la maquina",
            true);
        break;
      }

      int Cancel_Credit_Final = contadores.Get_Contadores_Int(Total_Cancel_Credit);
      int Cancel_Credit_Handpay_Final = contadores.Get_Contadores_Int(Cancel_Credit_Hand_Pay);

      // if (Ack == 0x00) /* Reset OK */
      // {

      //   Serial.println("Reset OK");

      //   while (millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response)
      //   {
      //     esp_task_wdt_reset();
      //     if (!Flag_Critial_Questions && Cancel_Credit_Final>Cancel_Credit_Inicial||!Flag_Critial_Questions && Cancel_Credit_Handpay_Final>Cancel_Credit_Handpay_Inicial)
      //       break;
      //     Serial.println(" Espera el cambio de contadores... ");
      //     vTaskDelay(300);
      //   }
      // }

      if (Ack == 0x00)
      {
        Flag_Critial_Questions = true;
        Flag_Change_Counters_Response = true;
        Timout_Break = millis();
        while ((millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response) && !Flag_Change_Counters_Break)
        {
          //Serial.println("Espera el cambio de contadores...");
          vTaskDelay(pdMS_TO_TICKS(300)); // Conversión segura a ticks para FreeRTOS
        }

        Flag_Change_Counters_One=false;
        
      }
      Flag_Change_Counters_Break=false;

      StaticJsonDocument<1024> jsonDocument;
      jsonDocument.clear();

      
      String Ack_Final = "";
      switch (Ack)
      {
      case 0x00:
        jsonDocument["IsSuccess"] = true;
        Ack_Final = "C0";
        break;

      case 0x01:
        jsonDocument["IsSuccess"] = false;
        Ack_Final = "C1";
        break;

      case 0x02:
        jsonDocument["IsSuccess"] = false;
        Ack_Final = "C2";
        break;

      case 0x04:
        jsonDocument["IsSuccess"] = false;
        Ack_Final = "C4";
        break;

      default:
        jsonDocument["IsSuccess"] = false;
        Ack_Final = "C4";
        break;
      }


      Buffer.Set_buffer_contadores_ACC_NO(3, contadores, RTC, Variables_globales);
      char res[258] = {};
      bzero(res, 258); // Pone el buffer en 0
      memcpy(res, Buffer.Get_buffer_contadores_ACC_NO(), 258);

      String convertedString = "";
      for (int i = 0; i < 258; i++)
      {
        convertedString += String(res[i]);
      }
      //Serial.println(convertedString);

      jsonDocument["Data"] = convertedString;
      jsonDocument["Ack"] = Ack_Final;
      jsonDocument["Ip"] = IP_toString_(Current_IP);

      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      //Serial.println(Json);
      request->send(200, "application/json", Json);
      Variable_Solicitud_Operador_Id=false;
    }

  }));

  // Server_API.addHandler(new AsyncCallbackJsonWebHandler("/Actualizacion", [](AsyncWebServerRequest* request, JsonVariant& json) {
  //   int Code;
  //   Code = 0x00;
  //   StaticJsonDocument<200> jsonDocument;
  //   jsonDocument.clear();

  //   jsonDocument["IsSuccess"] = true;
  //   jsonDocument["Code"] = Code;
  //   jsonDocument["Message"] = "Comando de actualización recibido con exito";
  //   String Json;
  //   serializeJson(jsonDocument, Json); /* Serializa Data */

  //   request->send(200, "application/json", Json);

  // }));


  // Server_API.on("/Actualizacion", HTTP_GET, [](AsyncWebServerRequest *request)
  // {
  //     int Code;
  //   Code = 0x00;
  //   StaticJsonDocument<200> jsonDocument;
  //   jsonDocument.clear();

  //   jsonDocument["IsSuccess"] = true;
  //   jsonDocument["Code"] = Code;
  //   jsonDocument["Message"] = "Comando de actualización recibido con exito";
  //   String Json;
  //   serializeJson(jsonDocument, Json); /* Serializa Data */

  //   request->send(200, "application/json", Json);
  // });

  // Server_API.on("/Ping_Globus_IM",HTTP_GET, [](AsyncWebServerRequest *request){
  //   StaticJsonDocument<200> jsonDocument;
  //   jsonDocument.clear();
//Init_Parameter_Update
    

  //   jsonDocument["IsSuccess"] = true;
  //   jsonDocument["message"] = "Conexión_OK";
  //   String Json;
  //   serializeJson(jsonDocument, Json); /* Serializa Data */
  //   request->send(200, "application/json", Json);
  // });

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

  Server_API.addHandler(new AsyncCallbackJsonWebHandler("/Actualiza_Contadores_Mecanicos",[](AsyncWebServerRequest*request,JsonVariant& json)
  {

    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
  
    if (!json.is<JsonObject>()) {

      jsonDocument["IsSuccess"] = false;
      jsonDocument["Message"] = "Tipo de dato recibido no es un json";
      jsonDocument["Ack"] = "A8";

      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);

      return;
    }else{

      auto&& data = json.as<JsonObject>();
      String Config;
      serializeJson(data, Config);  // Serializa el JsonObject completo

      if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 15)
      {
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Message"] = "Tipo de maquina no compatible ⚠️";
        jsonDocument["Ack"] = "A8";
        
        String Json;
        serializeJson(jsonDocument, Json); /* Serializa Data */
        request->send(200, "application/json", Json);
        return;
      }

      if(data.containsKey("Cancel_Credit")&& data.containsKey("Coin_In")&&data.containsKey("Coin_Out")&&data.containsKey("Total_Drop") &&data.containsKey("Multiplicador_Cancel_Credit")&& data.containsKey("Multiplicador_Coin_In")&&data.containsKey("Multiplicador_Coin_Out")&&data.containsKey("Multiplicador_Total_Drop"))
      {

        char Cancel_Credit_Data_char[9];
        char Coin_In_Data_char[9];
        char Coin_Out_Data_char[9];
        char Total_Drop_Data_char[9];

        char Cancel_Credit_Mult_char[9];
        char Coin_In_Mult_char[9];
        char Coin_Out_Mult_char[9];
        char Total_Drop_Mult_char[9];

        

        /* Contadores */
        int Cancel_Credit_Data=data["Cancel_Credit"].as<int>();
        int Coin_In_Data=data["Coin_In"].as<int>();
        int Coin_Out_Data=data["Coin_Out"].as<int>();
        int Total_Drop_Data=data["Total_Drop"].as<int>();

        /* Multiplicadores */
        int Cancel_Credit_Mult=data["Multiplicador_Cancel_Credit"].as<int>();
        int Coin_In_Mult=data["Multiplicador_Coin_In"].as<int>();
        int Coin_Out_Mult=data["Multiplicador_Coin_Out"].as<int>();
        int Total_Drop_Mult=data["Multiplicador_Total_Drop"].as<int>();


        if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 9)
        {
          Cancel_Credit_Mult=0;
          Coin_In_Mult=0;
          Coin_Out_Mult=0;
          Total_Drop_Mult=0;
        }



        snprintf(Cancel_Credit_Data_char,sizeof(Cancel_Credit_Data_char),"%08d",Cancel_Credit_Data);
        snprintf(Coin_In_Data_char,sizeof(Coin_In_Data_char),"%08d",Coin_In_Data);
        snprintf(Coin_Out_Data_char,sizeof(Coin_Out_Data_char),"%08d",Coin_Out_Data);
        snprintf(Total_Drop_Data_char,sizeof(Total_Drop_Data_char),"%08d",Total_Drop_Data);

        snprintf(Cancel_Credit_Mult_char, sizeof(Cancel_Credit_Mult_char), "%08d", Cancel_Credit_Mult);
        snprintf(Coin_In_Mult_char, sizeof(Coin_In_Mult_char), "%08d", Coin_In_Mult);
        snprintf(Coin_Out_Mult_char, sizeof(Coin_Out_Mult_char), "%08d", Coin_Out_Mult);
        snprintf(Total_Drop_Mult_char, sizeof(Total_Drop_Mult_char), "%08d", Total_Drop_Mult);


        // Serial.println(Cancel_Credit_Data_char);
        // Serial.println(Coin_In_Data_char);
        // Serial.println(Coin_Out_Data_char);
        // Serial.println(Total_Drop_Data_char);


        // Serial.println(Cancel_Credit_Mult_char);
        // Serial.println(Coin_In_Mult_char);
        // Serial.println(Coin_Out_Mult_char);
        // Serial.println(Total_Drop_Mult_char);


        /* Trama Gmaster */

        char Trama_Gmaster[128];
        int currentIndex = 4;  // Comienza en el índice 4 de Trama_Gmaster

        for (int i = 0; i < 8; ++i)
        {
          Trama_Gmaster[currentIndex++] = Cancel_Credit_Data_char[i];
        }

        for (int i = 0; i < 8; ++i)
        {
          Trama_Gmaster[currentIndex++] = Coin_In_Data_char[i];
        }


        for (int i = 0; i < 8; ++i)
        {
          Trama_Gmaster[currentIndex++] = Coin_Out_Data_char[i];
        }

        for (int i = 0; i < 8; ++i)
        {
          Trama_Gmaster[currentIndex++] = Total_Drop_Data_char[i];
        }




        for (int i = 0; i < 8; ++i)
        {
          Trama_Gmaster[currentIndex++] = Cancel_Credit_Mult_char[i];
        }

        for (int i = 0; i < 8; ++i)
        {
          Trama_Gmaster[currentIndex++] = Coin_In_Mult_char[i];
        }


        for (int i = 0; i < 8; ++i)
        {
          Trama_Gmaster[currentIndex++] = Coin_Out_Mult_char[i];
        }

        for (int i = 0; i < 8; ++i)
        {
          Trama_Gmaster[currentIndex++] = Total_Drop_Mult_char[i];
        }

        //Serial.println(Trama_Gmaster);

        if(Actualiza_Tarjeta_Mecanica(Trama_Gmaster))
        {
          jsonDocument["IsSuccess"] = true;
          jsonDocument["Message"] = "Contadores actualizados correctamente ✅";
          jsonDocument["Ack"] = "A7";
          Info_Cashless.Log(RTC,"CONTADORES_INICIALES_MECANICOS_RECIBIDOS ",Config);
        }
        else
        {
          jsonDocument["IsSuccess"] = false;
          jsonDocument["Message"] = "No hay comunicacion con la MET 🔌❌";
          jsonDocument["Ack"] = "A8";
        }
          

        String Json;
        serializeJson(jsonDocument, Json); /* Serializa Data */
        request->send(200, "application/json", Json);
      }else{
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Message"] = "Falta uno o mas parametros para realizar la operacion ❌ ";
        jsonDocument["Ack"] = "A8";
        String Json;
        serializeJson(jsonDocument, Json); /* Serializa Data */
        request->send(200, "application/json", Json);
        Info_Cashless.Log(RTC,"CONTADORES_INICIALES_MECANICOS_RECIBIDOS ","FALTA_UNO_O_MAS_PARAMETROS "+Config);
      }
    }

  }));

  Server_API.on("/Solicitud_SERVER_FTP_OPEN", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
    String Msg="";
    bool IsSuccess=false;

    if (!Variables_globales.Get_Variable_Global(Ftp_Mode))
    {
      
      if (Variables_globales.Get_Variable_Global(SD_INSERT))
      {

        ftpSrv.begin("GlobusAmin", "Globussistemas23", "SuperGlobusAdmin", "SuperG2023");

        Variables_globales.Set_Variable_Global(Comunicacion_Maq, false);
        Variables_globales.Set_Variable_Global(Ftp_Mode, true);

        if (Variables_globales.Get_Variable_Global(Ftp_Mode))
        {

          vTaskDelete(Check_Comunication_Maq);
          vTaskDelete(Mensajes_Server);
          vTaskDelete(RecepcionRS232);
          vTaskDelete(Encuestas);
          vTaskDelete(CommandProcess);
          clientUDP.stop();
          IsSuccess = true;
          Msg = "Modo FTP Iniciado con Exito!";
          Info_Cashless.Log(RTC,"COMANDO_INICIO_MODO_FTP_RECIBIDO","INICIADO_CON_EXITO");
          
        }
        else
        {
          IsSuccess = false;
          Msg = "Error Iniciando modo FTP";
          Info_Cashless.Log(RTC,"COMANDO_INICIO_MODO_FTP_RECIBIDO","ERROR_INICIANDO_FTP");
        }
      }
      else
      {
        IsSuccess = false;
        Msg = "Memoria SD no inicializada o no insertada";
        Info_Cashless.Log(RTC,"COMANDO_INICIO_MODO_FTP_RECIBIDO","MEMORIA_SD_NO_INICIALIZADA_O_NO_INSERTADA");
      }
    }
    else
    {
      IsSuccess = true;
      Msg = "Ya Existe una instancia del modo FTP";
      Info_Cashless.Log(RTC,"COMANDO_INICIO_MODO_FTP_RECIBIDO","YA_EXISTE_UN_");
    }

    jsonDocument["IsSuccess"] = IsSuccess;
    jsonDocument["Message"] = Msg;

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);
  
  });

  Server_API.on("/Solicitud_SERVER_FTP_CLOSE", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
    String Msg="Reiniciando Dispositivo!";
    bool IsSuccess=true;

    jsonDocument["IsSuccess"] = IsSuccess;
    jsonDocument["Message"] = Msg;

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);
    delay(1000);
      ESP.restart(); 

  });

  Server_API.on("/Borra_Log_Transacciones", HTTP_GET, [](AsyncWebServerRequest *request)
                {

    if(!Variables_globales.Get_Variable_Global(SD_INSERT)||!SD.exists("/LogESP.txt"))
    {
      request->send(404, "text/plain", "Archivo no encontrado");
      return;
    }else
    {

      if (SD.remove("/LogESP.txt"))
      {

        /* Crea archivo con fecha nuevamente */
        File f = SD.open("/LogESP.txt", FILE_WRITE);
        if (f)
        {
          /* Fecha creacion de archivo log */
          NVS.begin("Config_ESP32", false);
          NVS.putULong("Fecha_log", RTC.getEpoch());
          NVS.end();
          f.close();
        }

        request->send(200, "text/plain", "Archivo log Borrado");

        return;
      }
      else
      {
        request->send(200, "text/plain", "No se Borro el");
      }
    } });

  Server_API.addHandler(new AsyncCallbackJsonWebHandler("/Tiempo_Elimina_Log", [](AsyncWebServerRequest* request, JsonVariant& json) {
    


    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();


    if (!json.is<JsonObject>()) {


      jsonDocument["IsSuccess"] = false;

      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);
      return;
    }

    auto &&data = json.as<JsonObject>();

    if (!data.containsKey("Tiempo") || !data.containsKey("Unidades"))
    {
      jsonDocument["IsSuccess"] = false;
    }
    else
    {
      uint32_t tiempo = data["Tiempo"].as<uint32_t>();
      
      
      if (tiempo == 0)
      {
        tiempo = 15;
        
      }

      if (Info_Cashless.Guarda_Tiempo_Log(false, tiempo))
        jsonDocument["IsSuccess"] = true;
      else
        jsonDocument["IsSuccess"] = false;
    }

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);
    

  }));

  /* ------------------------> Metodos  Control Maquina <----------------------------------------------- */
  Server_API.on("/Inactiva_Maquina", HTTP_GET, [](AsyncWebServerRequest *request)
  {

    IPAddress ipCliente = request->client()->remoteIP();

    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    StaticJsonDocument<200> jsonDocument;
    //jsonDocument.clear();

    bool IsSuccess = false;
    String Msg = "";
    String Log="";

    if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
    {
      if (xSemaphoreTake(MaquinaSemaphore, (TickType_t)0) == pdTRUE)
      {
        
        Flag_Recv_Inactiva = false;
        Ack_Maq_Inactiva = false;
        Request_Inactiva = true;
        delay(1);
        Flag_Handle_Inactiva = true;

        unsigned long Timout_Break_Response1;
        int Stop_Transaccion_Amount_Response1 = 5000; // Tiempo de espera en milisegundos (2 Seg MAX)
        Timout_Break_Response1 = millis();

        while (!Ack_Maq_Inactiva && (millis() - Timout_Break_Response1 < Stop_Transaccion_Amount_Response1))
        {
          esp_task_wdt_reset(); // Alimenta el WDT
          // Serial.println("Esperando respuesta Activa.....");

          if ((millis() - Timout_Break_Response1) >= Stop_Transaccion_Amount_Response1)
          {
            // Serial.println("Tiempo agotado, saliendo del while con break.");
            break;
          }
          vTaskDelay(pdMS_TO_TICKS(10));
        }

        if (Ack_Maq_Inactiva && Flag_Recv_Inactiva)
        {
          IsSuccess = true;
          Msg = "Máquina inactivada con éxito.";
          Log="MAQUINA_INACTIVADA_CON_EXITO";
          Variables_globales.Set_Variable_Global(Flag_Premio_Mistery,true);
        }
        else
        {
          IsSuccess = false;
          Msg = "Fallo al inactivar la máquina.";
          Log="FALLO_INACTIVANDO_MAQUINA";
        }

        Ack_Maq_Inactiva = false;
        Request_Inactiva = false;
        Flag_Handle_Inactiva = false;
        Flag_Recv_Inactiva = false;
        xSemaphoreGive(MaquinaSemaphore);
      }
      else
      {
        IsSuccess = false;
        Msg = "Máquina ya está en proceso.";
        Log="YA_EXISTE_UNA_SOLICITUD";
      }
    }
    else
    {
      IsSuccess = false;
      Msg = "No Hay Comunicacion Con la MET";
      Log="NO_HAY_COMUNICACION_CON_LA_MET";
    }

    jsonDocument["IsSuccess"] = IsSuccess;
    jsonDocument["Message"] = Msg;
    jsonDocument["Fecha_Hora"] = DataTime;

    String Json;
    serializeJson(jsonDocument, Json);
    request->send(200, "application/json", Json);

    Info_Cashless.Log(RTC,"SOLICITUD_INACTIVA_MAQUINA_API_RECIBIDA "+ipCliente.toString(),Log);
  });

  Server_API.on("/Activa_Maquina", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    IPAddress ipCliente = request->client()->remoteIP();
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    StaticJsonDocument<200> jsonDocument;

    bool IsSuccess = false;
    String Msg = "";
    String Log="";

  if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
  {
    if (xSemaphoreTake(MaquinaSemaphore, (TickType_t)0) == pdTRUE)
    {

      Flag_Recv_Activa = false;
      Ack_Maq_Activa = false;
      Request_Activa = true;
      delay(1);
      Flag_Handle_Activa = true;

      unsigned long Timout_Break_Response1;
      int Stop_Transaccion_Amount_Response1 = 5000; // Tiempo de espera en milisegundos (2 Seg MAX)
      Timout_Break_Response1 = millis();

      while (!Ack_Maq_Activa && (millis() - Timout_Break_Response1 < Stop_Transaccion_Amount_Response1))
      {

        esp_task_wdt_reset(); // Alimenta el WDT
        // Serial.println("Esperando respuesta Activa.....");

        if ((millis() - Timout_Break_Response1) >= Stop_Transaccion_Amount_Response1)
        {
          // Serial.println("Tiempo agotado, saliendo del while con break.");
          break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
      }

      if (Ack_Maq_Activa && Flag_Recv_Activa)
      {
        IsSuccess = true;
        Msg = "Máquina Activada con éxito.";
        Log="MAQUINA_ACTIVADA_CON_EXITO";
      }
      else
      {
        IsSuccess = false;
        Msg = "Fallo al activar la máquina.";
        Log="FALLA_ACTIVANDO_MAQUINA";
      }

      Ack_Maq_Activa = false;
      Request_Activa = false;
      Flag_Handle_Activa = false;
      Flag_Recv_Activa = false;
      xSemaphoreGive(MaquinaSemaphore);
    }
    else
    {
      IsSuccess = false;
      Msg = "Máquina ya está en proceso.";
      Log="YA_EXISTE_UNA_SOLICITUD";
    }
  }
  else
  {
    IsSuccess = false;
    Msg = "No Hay Comunicacion Con la MET";
    Log="NO_HAY_COMUNICACION_CON_LA_MET";
  }

  jsonDocument["IsSuccess"] = IsSuccess;
  jsonDocument["Message"] = Msg;
  jsonDocument["Fecha_Hora"] = DataTime;

  String Json;
  serializeJson(jsonDocument, Json);
  request->send(200, "application/json", Json);

  Info_Cashless.Log(RTC,"SOLICITUD_ACTIVA_MAQUINA_API_RECIBIDA "+ipCliente.toString(),Log);
  });
  /*-----------------------------------------------------------------------------------------------------*/

  /* ----------------------------------> Pantalla TFT <---------------------------------------------------*/

  Server_API.addHandler(new AsyncCallbackJsonWebHandler("/api/Fidelizacion/Sincro_TFT_Display",[](AsyncWebServerRequest*request,JsonVariant& json)
  {

    StaticJsonDocument<250> jsonDocument;
    jsonDocument.clear();
    IPAddress ipCliente = request->client()->remoteIP();


    String Error="";
    String Msg="";
  
    if (!json.is<JsonObject>()) {

      jsonDocument["IsSuccess"] = false;
      jsonDocument["Message"] = "Tipo de dato no identificado JSON";
      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);
      Error="JSON_NO_ES_UN_OBJETO_VALIDO";
      Info_Cashless.Log(RTC, "COMANDO_SINCRONIZAR_PANTALLA_RECIBIDO"+ipCliente.toString(), Error);
      return;
    }else{

      auto&& data = json.as<JsonObject>();

      if(data.containsKey("MAC_Device"))
      {
        bool Is_OK=false;

        String MAC = data["MAC_Device"].as<String>();

       // Serial.println(MAC);


        if (esMacValida(MAC))
        {
          uint8_t Mac_TFT[6];
          uint8_t MacRead[6];
          bool IsSuccess = false;

          if (sscanf(MAC.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                     &Mac_TFT[0], &Mac_TFT[1], &Mac_TFT[2], &Mac_TFT[3], &Mac_TFT[4], &Mac_TFT[5]) == 6)
          {

            Is_OK=Init_TFT_Display(true, Mac_TFT);

            if (Is_OK)
            {
              StaticJsonDocument<200> doc;

              String Payload = "";
              int Intentos_Conexion = 3;
              doc["IsSuccess"] = true;
              doc["Opcion"] = SINCRO;
              doc["Mac_T"] = MAC;               /* Mac TFT Sincro */
              doc["Mac_G"] = WiFi.macAddress(); /* Mac Globus IM ESP32 */
              serializeJson(doc, Payload);

              if (Send_TFT(Mac_TFT, (uint8_t *)Payload.c_str(), Payload.length()))
              {
                if (Await_ms(get_Flag_Sincro_TFT, 1000))
                {
                  IsSuccess = true; /* Sincro OK*/
                  Msg="Sincronizacion recibida con exito";
                }
                else
                {
                  IsSuccess = false; /* Falla Sincro */
                  Msg = "No se recibio respuesta de la pantalla TFT";
                  Error = "No_se_recibio_respuesta_de_la_pantalla_TFT " + MAC;
                  Info_Cashless.Log(RTC, "COMANDO_SINCRONIZAR_PANTALLA_RECIBIDO " + ipCliente.toString(), Error);
                  Stop_TFT_Display();
                }
              }
              else
              {
                IsSuccess = false;
                Msg = "Mensaje de sincronizacion no enviado";

                Error = "Mensaje_de_sincronizacion_no_enviado " + MAC;
                Info_Cashless.Log(RTC, "COMANDO_SINCRONIZAR_PANTALLA_RECIBIDO " + ipCliente.toString(), Error);
                Stop_TFT_Display();
              }
            }
            else
            {
              IsSuccess = false;
              Msg = "Protocolo inalambrico no inicializado";
              Error = "Mensaje_de_sincronizacion_no_enviado " + MAC;
              Info_Cashless.Log(RTC, "COMANDO_SINCRONIZAR_PANTALLA_RECIBIDO " + ipCliente.toString(), Error);
            }
          }
          else
          {
            IsSuccess = false;
            Msg = "Error de conversion String-uint8_t";
            Error="Error_de_conversion_String-uint8_t";
            Info_Cashless.Log(RTC, "COMANDO_SINCRONIZAR_PANTALLA_RECIBIDO "+ipCliente.toString(), Error);
          }


          jsonDocument["IsSuccess"]=IsSuccess;
          jsonDocument["Message"]=Msg;
          String Json;
          serializeJson(jsonDocument, Json); /* Serializa Data */
          request->send(200, "application/json", Json);

          if (IsSuccess)
          {

            NVS.begin("Config_ESP32", false);
            NVS.putBytes("Address_TFT", Mac_TFT, sizeof(Mac_TFT));
            NVS.putInt("Type_TFT",get_Tipo_TFT());          
            NVS.end();


            // for(int i=0; i<5;i++)
            // {
            //   Serial.println(Mac_TFT[i],HEX);
            // }

            Error="DISPOSITIVO_PANTALLA_TFT_SINCRONIZADA_CON_EXITO";
            Info_Cashless.Log(RTC, "REINICIO_DISPOSITIVO", Error);

            StaticJsonDocument<200> doc;

            String Payload = "";
            int Intentos_Conexion = 15;
            doc["IsSuccess"] = true;
            doc["Opcion"] = REESTART_TFT;
            
            for(int i=0; i<6; i++)
            {
              Serial.println(Mac_TFT[i]);
            }
            
            serializeJson(doc, Payload);

            Serial.println(Payload);

            Send_TFT(Mac_TFT, (uint8_t *)Payload.c_str(), Payload.length());
            
            delay(1000);

            ESP.restart();
          }
        }else{
          jsonDocument["IsSuccess"] = false;
          jsonDocument["Message"] = "Error Formato de MAC debe ser XX:XX:XX:XX:XX:XX";
          String Json;
          serializeJson(jsonDocument, Json); /* Serializa Data */
          request->send(200, "application/json", Json);

          Error="Error_Formato_de_MAC_debe_ser_XX:XX:XX:XX:XX:XX";
          Info_Cashless.Log(RTC, "COMANDO_SINCRONIZAR_PANTALLA_RECIBIDO", Error);
        }
      }else{
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Message"] = "No exite informacion de la MAC";
        String Json;
        serializeJson(jsonDocument, Json); /* Serializa Data */
        request->send(200, "application/json", Json);

        Error="NO_EXISTE_INFORMACION_DE_LA_MAC";
        Info_Cashless.Log(RTC, "COMANDO_SINCRONIZAR_PANTALLA_RECIBIDO", Error);
      }
    }

  }));

  Server_API.addHandler(new AsyncCallbackJsonWebHandler("/api/Fidelizacion/Actualiza_Puntos_Fidelizacion",[](AsyncWebServerRequest* request, JsonVariant& json) 
  {
    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
    String Msg;
    bool IsSuccess = false;

    //Serial.println("Recibida por el API");

    // --- VALIDACIÓN JSON ---
    if (!json.is<JsonObject>()) {
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Message"] = "JSON inválido o no es un objeto";
        String Json;
        serializeJson(jsonDocument, Json);
        request->send(200, "application/json", Json);

        Info_Cashless.Log(RTC,"COMANDO_ACTUALIZACION_PUNTOS_FIDELIZACION_RECIBIDO","Objeto inválido recibido");
        return;
    }

    // --- VALIDAR SI HAY DISPOSITIVO TFT ---
    if (!Variables_globales.Get_Variable_Global(Status_Device_TFT_Display)) {
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Message"] = "No existe dispositivo sincronizado";
        String Json; serializeJson(jsonDocument, Json);
        request->send(200, "application/json", Json);

        Info_Cashless.Log(RTC,"COMANDO_ACTUALIZACION_PUNTOS_FIDELIZACION_RECIBIDO","Error no existe dispositivo TFT no sincronizado");
        return;
    }

    // --- VALIDAR SI TFT ESTÁ INICIALIZADA ---
    if (!Variables_globales.Get_Variable_Global(Conexion_TFT_Display)) {
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Message"] = "Pantalla TFT no inicializada";
        String Json; serializeJson(jsonDocument, Json);
        request->send(200, "application/json", Json);

        Info_Cashless.Log(RTC,"COMANDO_ACTUALIZACION_PUNTOS_FIDELIZACION_RECIBIDO","Error dispositivo TFT no conectado");

        return;
    }

    // --- VALIDAR SI HAY SESIÓN ACTIVA ---
    if (!(Variables_globales.Get_Variable_Global(Flag_Sesion_RFID) ||
          Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless))) 
    {
      jsonDocument["IsSuccess"] = false;
      jsonDocument["Message"] = "Dispositivo no tiene una sesión activa";

      if (Permitir_Ultima_Actualizacion)
        Permitir_Ultima_Actualizacion = false;
      else
      {
        String Json;
        serializeJson(jsonDocument, Json);
        request->send(200, "application/json", Json);

        Info_Cashless.Log(RTC,"COMANDO_ACTUALIZACION_PUNTOS_FIDELIZACION_RECIBIDO","El dispositivo no tiene sesión activa");
        return;
      }
    }

    // --- PROCESAR PAYLOAD ---
    JsonObject data = json.as<JsonObject>();

    if (!data.containsKey("Cliente_Nombre") ||
        !data.containsKey("Casino") ||
        !data.containsKey("Deno_Cashless") ||
        !data.containsKey("Total_Fide") ||
        !data.containsKey("Total_Bole") ||
        !data.containsKey("Nivel_Usuario") ||
        !data.containsKey("Actual_Fide") ||
        !data.containsKey("Actual_Bole"))
    {
      jsonDocument["IsSuccess"] = false;
      jsonDocument["Message"] = "Falta uno o mas Parametros";



      String Json;
      serializeJson(jsonDocument, Json);
      request->send(200, "application/json", Json);

      Info_Cashless.Log(RTC,"COMANDO_ACTUALIZACION_PUNTOS_FIDELIZACION_RECIBIDO","Falta uno o mas parametros para procesar la solicitud");
      return; // No actualizar
    }

    if (data["Cliente_Nombre"].as<String>().length() == 0 ||
        data["Casino"].as<String>().length() == 0 ||
        data["Nivel_Usuario"].as<String>().length() == 0)
    {
      jsonDocument["IsSuccess"] = false;
      jsonDocument["Message"] = "Error en datos recibidos";
      String Json;
      serializeJson(jsonDocument, Json);
      request->send(200, "application/json", Json);

      Info_Cashless.Log(RTC,"COMANDO_ACTUALIZACION_PUNTOS_FIDELIZACION_RECIBIDO","Error en datos recibidos");
      return;
    }

    float deno = data["Deno_Cashless"].as<float>();

    if (Variables_globales.Get_Variable_Global(Enable_Cashless))
    {
      if (isnan(deno) || deno <= 0.0f)
      {
        jsonDocument["IsSuccess"] = false;
        // Convertir correctamente el float a String para evitar formato extraño
        String denoStr = String(deno, 4); // 4 decimales opcional
        jsonDocument["Message"] = "Error: Deno_Cashless invalido (" + denoStr + ")";
        String respuestaJson;
        serializeJson(jsonDocument, respuestaJson);
        request->send(200, "application/json", respuestaJson);
        return;
      }
    }

    String usuario = data["Cliente_Nombre"].as<String>();
    String casino = data["Casino"].as<String>();
    float denotest = data["Deno_Cashless"].as<float>();
    float totalFide = data["Total_Fide"].as<float>();
    float totalBole = data["Total_Bole"].as<float>();
    String nivelUsuario = data["Nivel_Usuario"].as<String>();
    float actualFide = data["Actual_Fide"].as<float>();
    float actualBole = data["Actual_Bole"].as<float>();

    DisplayTFT.info.Usuario = usuario;
    DisplayTFT.info.Casino = casino;
    DisplayTFT.info.DenoCashless = denotest;
    DisplayTFT.info.Total_Fide = totalFide;
    DisplayTFT.info.Total_Bole = totalBole;
    DisplayTFT.info.Nivel_Usuario = nivelUsuario;
    DisplayTFT.info.Actual_Fide = actualFide;
    DisplayTFT.info.Actual_Bole = actualBole;

    bool datosOK = true;

    datosOK &= (DisplayTFT.info.Usuario == usuario);
    datosOK &= (DisplayTFT.info.Casino == casino);
    datosOK &= (DisplayTFT.info.DenoCashless == denotest);
    datosOK &= (DisplayTFT.info.Total_Fide == totalFide);
    datosOK &= (DisplayTFT.info.Total_Bole == totalBole);
    datosOK &= (DisplayTFT.info.Nivel_Usuario == nivelUsuario);
    datosOK &= (DisplayTFT.info.Actual_Fide == actualFide);
    datosOK &= (DisplayTFT.info.Actual_Bole == actualBole);

    if (!datosOK)
    {
      jsonDocument["IsSuccess"] = false;
      jsonDocument["Message"] = "Error: actualizando pantalla TFT";

      String respuesta;
      serializeJson(jsonDocument, respuesta);
      request->send(200, "application/json", respuesta);

      Info_Cashless.Log(RTC,"COMANDO_ACTUALIZACION_PUNTOS_FIDELIZACION_RECIBIDO","Error: actualizando pantalla TFT");
      return;
    }
    else
    {
      jsonDocument["IsSuccess"] = true;
      jsonDocument["Message"] = "Actualizacion recibida con exito";

      if (Variables_globales.Get_Variable_Global(Flag_Sesion_RFID) ||
          Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless))
      {
        if (!Permitir_Ultima_Actualizacion)
          Permitir_Ultima_Actualizacion = true;
      }


      DisplayTFT.Notify_Now(EVENT_ACTUALIZA_PUNTOS);
      //Variables_globales.Set_Variable_Global(Flag_Update_TFT_Globus_IM, true);

      String Json;
      serializeJson(jsonDocument, Json);
      request->send(200, "application/json", Json);
    }
 
  }));



  Server_API.on("/api/Fidelizacion/Consulta_Informacion_Puntos", HTTP_GET, [](AsyncWebServerRequest *request)
  {

    if(Variables_globales.Get_Variable_Global(Status_Device_TFT_Display) && Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
    {
      StaticJsonDocument<500> jsonDocument;
      jsonDocument.clear();



      if(!(Variables_globales.Get_Variable_Global(Flag_Sesion_RFID) || Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless)))
      {
        jsonDocument["IsSuccess"] = false;
        jsonDocument["Message"] = "Dispositivo no tiene una sesión activa";
        jsonDocument["Cliente_Nombre"] = DisplayTFT.info.Usuario;
        jsonDocument["Casino"] = DisplayTFT.info.Casino;
        jsonDocument["Deno_Cashless"] = DisplayTFT.info.DenoCashless;
        jsonDocument["Total_Fide"] = DisplayTFT.info.Total_Fide;
        jsonDocument["Total_Bole"] = DisplayTFT.info.Total_Bole;
        jsonDocument["Nivel_Usuario"] = DisplayTFT.info.Nivel_Usuario;
        jsonDocument["Actual_Fide"] = DisplayTFT.info.Actual_Fide;
        jsonDocument["Actual_Bole"] = DisplayTFT.info.Actual_Bole;

        String Json;
        serializeJson(jsonDocument, Json); /* Serializa Data */
        request->send(200, "application/json", Json);

        Info_Cashless.Log(RTC,"COMANDO_CONSULTA_INFORMACION_PUNTOS_FIDELIZACION_RECIBIDO","El dispositivo no tiene sesión activa");
        return;
      }
    
      jsonDocument["IsSuccess"] = true;
      jsonDocument["Message"] = "Consulta realizada con exito";
      jsonDocument["Cliente_Nombre"] = DisplayTFT.info.Usuario;
      jsonDocument["Casino"] = DisplayTFT.info.Casino;
      jsonDocument["Deno_Cashless"] = DisplayTFT.info.DenoCashless;
      jsonDocument["Total_Fide"] = DisplayTFT.info.Total_Fide;
      jsonDocument["Total_Bole"] = DisplayTFT.info.Total_Bole;
      jsonDocument["Nivel_Usuario"] = DisplayTFT.info.Nivel_Usuario;
      jsonDocument["Actual_Fide"] = DisplayTFT.info.Actual_Fide;
      jsonDocument["Actual_Bole"] = DisplayTFT.info.Actual_Bole;

      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);
    }
    else
    {
      StaticJsonDocument<200> jsonDocument;
      jsonDocument.clear();
      jsonDocument["IsSuccess"] = false;
      jsonDocument["Message"] = "No existe dispositivo sincronizado";

      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);
    }
  });

  Server_API.on("/api/Fidelizacion/Desincro_TFT_Display", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
    bool IsSuccess=false;
    String Msg;

    if (!Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
    {
      Msg="No existe dispositivo sincronizado";

      jsonDocument["IsSuccess"] = IsSuccess;
      jsonDocument["Message"] = Msg;
      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);

      Info_Cashless.Log(RTC, "COMANDO_SINCRONIZAR_PANTALLA_RECIBIDO", Msg);
      return;
    }else{

      // StaticJsonDocument<500> doc;

      // String Payload;
      // int Intentos_Conexion = 5;
      // doc["IsSuccess"] = true;
      // doc["Opcion"] = PING;

      // serializeJson(doc, Payload);
      
      // Serial.println(Payload);
      // Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length());

      // if (Await_ms(get_Flag_Conexion_TFT, 2000))
      // {
        
        StaticJsonDocument<200> doc;

        String Payload = "";
        int Intentos_Conexion = 5;
        doc["IsSuccess"] = true;

        char macStr[18]; // 6 bytes * 2 chars + 5 ':' + 1 '\0'
        sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                Address_Device_TFT_Display[0], Address_Device_TFT_Display[1], Address_Device_TFT_Display[2], Address_Device_TFT_Display[3], Address_Device_TFT_Display[4], Address_Device_TFT_Display[5]);

        String MACT = String(macStr);
        //Serial.println(MACT); // Muestra: 24:6F:28:AB:CD:EF

        doc["Mac_T"] = MACT;               /* Mac TFT Sincro */
        doc["Mac_G"] = WiFi.macAddress();
        doc["Opcion"] = DESINCRO;

        serializeJson(doc, Payload);

        Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length());
  
        if (Await_ms(get_Flag_Desincro_TFT, 1000))
        {
          uint8_t Backup_Mac[6];
          memcpy(Backup_Mac, Address_Device_TFT_Display, sizeof(Address_Device_TFT_Display)); // Copia los 6 bytes
          
          uint8_t Adress_TFT_null[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
          NVS.begin("Config_ESP32", false);
          NVS.putBytes("Address_TFT", Adress_TFT_null, sizeof(Adress_TFT_null));

          size_t adress_TFT_Display_Len = NVS.getBytesLength("Address_TFT");
          uint8_t Adress_TFT_Display[adress_TFT_Display_Len];
          int Count_test = 0;
          NVS.getBytes("Address_TFT", Adress_TFT_Display, adress_TFT_Display_Len);

          NVS.putInt("Type_TFT",TFT_UNKNOW); 
          int Test_type=NVS.getInt("Type_TFT",TFT_UNKNOW);

          NVS.end();

          int cont=0;
          for(int i=0; i<6; i++)
          {
            if(Adress_TFT_Display[i]==0x00)
            cont++;
          }

          if(cont>=6 && Test_type==TFT_UNKNOW)
            IsSuccess=true;
          else
            IsSuccess=false;


          jsonDocument["IsSuccess"] = IsSuccess;

          if(IsSuccess)
            Msg="Se elimino la sincronizacion con exito!";

          jsonDocument["Message"] = Msg;
          jsonDocument["Mac"] = getMacString(Backup_Mac);

          String Json;
          serializeJson(jsonDocument, Json); /* Serializa Data */
          request->send(200, "application/json", Json);


          Info_Cashless.Log(RTC, "COMANDO_SINCRONIZAR_PANTALLA_RECIBIDO", Msg);
          StaticJsonDocument<200> doc;  

          String Payload = "";
          int Intentos_Conexion = 5;
          doc["IsSuccess"] = true;
          doc["Opcion"] = REESTART_TFT;

          serializeJson(doc, Payload);

          
          Send_TFT(Backup_Mac, (uint8_t *)Payload.c_str(), Payload.length());
          delay(500);
          ESP.restart();
        }
        else
        {

          IsSuccess=false;
          Msg="No se recibio respuesta del dispositivo";
          jsonDocument["IsSuccess"] = IsSuccess;
          jsonDocument["Message"] = Msg;
          jsonDocument["Mac"] = getMacString(Address_Device_TFT_Display);
          String Json;
          serializeJson(jsonDocument, Json); /* Serializa Data */
          request->send(200, "application/json", Json);
          Info_Cashless.Log(RTC, "COMANDO_SINCRONIZAR_PANTALLA_RECIBIDO", Msg);
        }
      }
      // else
      // {
      //   jsonDocument["IsSuccess"] = IsSuccess;
      //   jsonDocument["Message"] = "Dispositivo no conectado";
      //   jsonDocument["Mac"]=getMacString(Address_Device_TFT_Display);
      //   String Json;
      //   serializeJson(jsonDocument, Json); /* Serializa Data */
      //   request->send(200, "application/json", Json);
      //   return;
      // }

      
    //}


  });

  Server_API.on("/api/Fidelizacion/Reset_TFT_Display", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
    bool IsSuccess=false;
    String Msg;

    if (!Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
    {

      Msg="No existe dispositivo sincronizado";

      jsonDocument["IsSuccess"] = IsSuccess;
      jsonDocument["Message"] = Msg;
      jsonDocument["Mac"] = getMacString(Address_Device_TFT_Display);

      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);

      Info_Cashless.Log(RTC, "COMANDO_RESET_PANTALLA_RECIBIDO", Msg);

      return;
    }
    else
    {
      StaticJsonDocument<500> doc;
      String Payload;
      int Intentos_Conexion = 5;
      doc["IsSuccess"] = true;
      doc["Opcion"] = PING;


      serializeJson(doc, Payload);

      Serial.println(Payload);
      Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length());

      if (Await_ms(get_Flag_Conexion_TFT, 2000))
      {

        StaticJsonDocument<200> doc;

        String Payload = "";
        int Intentos_Conexion = 5;
        doc["IsSuccess"] = true;
        doc["Mac"] = getMacString(Address_Device_TFT_Display);
        doc["Opcion"] = REESTART_TFT;

        serializeJson(doc, Payload);

        if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
        {
          IsSuccess = true;
          Msg="Reset de pantalla TFT realizado con exito";
        }
        else
        {
          IsSuccess = false;
          Msg="No se recibio respuesta del dispositivo";
        }
         
        jsonDocument["IsSuccess"] = IsSuccess;
        jsonDocument["Message"] = Msg;
        jsonDocument["Mac"] = getMacString(Address_Device_TFT_Display);

        String Json;
        serializeJson(jsonDocument, Json); /* Serializa Data */
        request->send(200, "application/json", Json);

        Info_Cashless.Log(RTC, "COMANDO_RESET_PANTALLA_RECIBIDO", Msg);
      }
      else
      {
        IsSuccess = false;
        Msg="No se recibio respuesta del dispositivo";

        jsonDocument["IsSuccess"] = IsSuccess;
        jsonDocument["Message"] = Msg;
        jsonDocument["Mac"] = getMacString(Address_Device_TFT_Display);
        String Json;
        serializeJson(jsonDocument, Json); /* Serializa Data */
        request->send(200, "application/json", Json);

        Info_Cashless.Log(RTC, "COMANDO_RESET_PANTALLA_RECIBIDO", Msg);
      }
    }
  });

  Server_API.addHandler(new AsyncCallbackJsonWebHandler("/api/Fidelizacion/Configuracion_TFT", [](AsyncWebServerRequest* request, JsonVariant& json)
  {
    
    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
    bool IsSuccess=false;
    String Msg="";

    if (!json.is<JsonObject>()) {

      Msg="No existe objeto json";
      jsonDocument["IsSuccess"] = IsSuccess;
      jsonDocument["Message"] = Msg;
      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);
      return;
    }


    if(Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
    {

      auto &&data = json.as<JsonObject>();

      

      if(data.containsKey("Timeout_Ventana_Saldos")&&data.containsKey("Timeout_Imagenes")&&data.containsKey("Timeout_Carrucel_Mensajes")&&data.containsKey("Timeout_Mensajes"))
      {

        uint32_t timeoutSaldos = data["Timeout_Ventana_Saldos"].as<uint32_t>();
        uint32_t timeoutImagenes = data["Timeout_Imagenes"].as<uint32_t>();
        uint32_t timeoutCarrucel_Mensajes = data["Timeout_Carrucel_Mensajes"].as<uint32_t>();
        uint32_t timeoutMensajes = data["Timeout_Mensajes"].as<uint32_t>();

        if (timeoutSaldos > 0 && timeoutImagenes > 0 && timeoutCarrucel_Mensajes > 0 && timeoutMensajes > 0)
        {

          uint32_t ConfigPacket[] = {timeoutSaldos, timeoutImagenes, timeoutCarrucel_Mensajes, timeoutMensajes};
          NVS.begin("Config_ESP32", false);
          NVS.putBytes("Config_TFT", ConfigPacket, sizeof(ConfigPacket));

          uint32_t ConfigPacketRead[4];
          NVS.getBytes("Config_TFT", ConfigPacketRead, sizeof(ConfigPacketRead));
          NVS.end();

          if (ConfigPacketRead[0] == timeoutSaldos && ConfigPacketRead[1] == timeoutImagenes && ConfigPacketRead[2] == timeoutCarrucel_Mensajes && ConfigPacketRead[3] == timeoutMensajes)
          {

            DisplayTFT.configtft.timeoutSaldosCONFIG = timeoutSaldos;
            DisplayTFT.configtft.timeoutImagenesCONFIG = timeoutImagenes;
            DisplayTFT.configtft.timeoutCarrucel_MensajesCONFIG = timeoutCarrucel_Mensajes;
            DisplayTFT.configtft.timeoutMensajesCONFIG = timeoutMensajes;

            IsSuccess = true;
            Msg = "Configuracion aplicada con exito";
          }
          else
          {
            IsSuccess = false;
            Msg = "Error aplicando configuracion";
          }
        }
        else
        {
          IsSuccess = false;
          Msg = "Error en uno o mas parametros";
        }
      }
      else
      {
        IsSuccess = false;
        Msg = "Error falta uno o mas parametros para la solicitud";
      }
    }
    else
    {
      Msg = "No existe dispositivo sincronizado";
      IsSuccess = false;
      
    }

    jsonDocument["IsSuccess"] = IsSuccess;
    jsonDocument["Message"] = Msg;

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);
    
  }));

  Server_API.addHandler(new AsyncCallbackJsonWebHandler("/api/Fidelizacion/Modo_Descarga_TFT", [](AsyncWebServerRequest* request, JsonVariant& json)
  {
    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
    bool IsSuccess = false;
    String Msg = "";

    if (!json.is<JsonObject>()) {
        Msg = "No existe objeto json";
        jsonDocument["IsSuccess"] = IsSuccess;
        jsonDocument["Message"] = Msg;
        String Json;
        serializeJson(jsonDocument, Json);
        request->send(200, "application/json", Json);
        return;
    }

    if (!Variables_globales.Get_Variable_Global(Conexion_TFT_Display) || !Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
    {

      if(!Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
        Msg = "No Existe Dispositivo Sincronizado";

      else if(!Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
        Msg = "Pantalla TFT no inicializada";

     
      jsonDocument["IsSuccess"] = IsSuccess;
      jsonDocument["Message"] = Msg;
      String Json;
      serializeJson(jsonDocument, Json);
      request->send(200, "application/json", Json);
      return;
    }

    auto &&data = json.as<JsonObject>();

    String FileName = data.containsKey("FileName") ? data["FileName"].as<String>() : "default.bmp";
    String Url      = data.containsKey("Url") ? data["Url"].as<String>() : "";
    String SSID_Wifi = Configuracion.Get_Configuracion(SSID, "Nombre_Red");
    String Password_Wifi = Configuracion.Get_Configuracion(Password, "Password_red");

    if (Url == "") {
        Msg = "Faltan parámetros obligatorios (Url, SSID, Password)";
        jsonDocument["IsSuccess"] = IsSuccess;
        jsonDocument["Message"] = Msg;
        String Json;
        serializeJson(jsonDocument, Json);
        request->send(200, "application/json", Json);
        return;
    }

    // Empaquetamos la configuración para enviarla a la TFT
    StaticJsonDocument<300> doc;
    String Payload;
    int Intentos_Conexion = 5;

    doc["IsSuccess"] = true;
    doc["FileName"] = FileName;
    doc["Url"] = Url;
    doc["SSID"] = SSID_Wifi;
    doc["Password"] = Password_Wifi;
    doc["Opcion"] = DOWNLOAD;

    serializeJson(doc, Payload);

    for (int i = 0; i < Intentos_Conexion; i++)
    {
      if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
      {
        break;
      }
    }

    // Espera que la TFT confirme que entró en modo descarga
    if (Await_ms(get_Flag_Descarga_TFT, 2000))
    {
      IsSuccess = true;
      Msg = "TFT conectada a red e iniciando descarga de imagen";
    }
    else
    {
      Msg = "Error iniciando modo descarga";
    }

    jsonDocument["IsSuccess"] = IsSuccess;
    jsonDocument["Message"] = Msg;

    String Json;
    serializeJson(jsonDocument, Json);
    request->send(200, "application/json", Json);

  }));

  Server_API.addHandler(new AsyncCallbackJsonWebHandler("/api/Fidelizacion/Borrar_Imagen_TFT", [](AsyncWebServerRequest* request, JsonVariant& json)
  {
    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
    bool IsSuccess = false;
    String Msg = "";

    if (!json.is<JsonObject>()) {
        Msg = "No existe objeto json";
        jsonDocument["IsSuccess"] = IsSuccess;
        jsonDocument["Message"] = Msg;
        String Json;
        serializeJson(jsonDocument, Json);
        request->send(200, "application/json", Json);
        return;
    }

    auto &&data = json.as<JsonObject>();

    if (!data.containsKey("FileName")) {
        Msg = "Falta parámetro obligatorio: FileName";
        jsonDocument["IsSuccess"] = IsSuccess;
        jsonDocument["Message"] = Msg;
        String Json;
        serializeJson(jsonDocument, Json);
        request->send(200, "application/json", Json);
        return;
    }

    String FileName = data["FileName"].as<String>();

    // Empaquetamos la orden de borrado
    StaticJsonDocument<200> doc;
    String Payload;
    int Intentos_Conexion = 5;

    doc["IsSuccess"] = true;
    doc["FileName"] = FileName;
    doc["Opcion"] =REMOVE_IMG_TFT;

    serializeJson(doc, Payload);

    for (int i = 0; i < Intentos_Conexion; i++) {
        if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length())) {
            break;
        }
    }

    // Espera confirmación de la TFT
    if (Await_ms(get_Flag_Borrar_TFT, 2000)) {
        IsSuccess = true;
        Msg = "Imagen borrada con éxito";
    } else {
        Msg = "No fue posible borrar la imagen: "+FileName;
    }

    jsonDocument["IsSuccess"] = IsSuccess;
    jsonDocument["Message"] = Msg;

    String Json;
    serializeJson(jsonDocument, Json);
    request->send(200, "application/json", Json);

  }));

  // Server_API.addHandler(new AsyncCallbackJsonWebHandler("/api/Fidelizacion/Update_Firmware_TFT", [](AsyncWebServerRequest* request, JsonVariant& json)
  // {
  //   StaticJsonDocument<200> jsonDocument;
  //   jsonDocument.clear();

  //   bool IsSuccess = false;
  //   String Message = "";

  //   if (Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
  //   {

  //     auto &&data = json.as<JsonObject>();

  //     if(data.containsKey("Url") &&data.containsKey("Api_Token")&& data.containsKey("Api_Bin")&&data.containsKey("Api_Respuesta"))
  //     {
  //       String Url = data["Url"].as<String>();
  //       String Api_Token = data["Api_Token"].as<String>();
  //       String Api_Bin = data["Api_Bin"].as<String>();
  //       String Api_Respuesta = data["Api_Respuesta"].as<String>();
  //       String Version_Programa = data["Version_Act"].as<String>();
  //       String Api_Version = data["Api_Version"].as<String>();

  //       String SSID_Wifi = Configuracion.Get_Configuracion(SSID, "Nombre_Red");
  //       String Password_Wifi = Configuracion.Get_Configuracion(Password, "Password_red");

  //       StaticJsonDocument<200> doc;
  //       doc.clear();
  //       String Payload = "";
  //       int Intentos_Conexion = 3;

  //       doc["Opcion"] = UPDATE_TFT;
  //       doc["Ssid"] = SSID_Wifi;
  //       doc["password"] = Password_Wifi;
  //       doc["Url"] = Url;
        

  //       // doc["Api_Token"] = Api_Token;
  //       // doc["Api_Bin"] = Api_Bin;
  //       // doc["Api_Respuesta"] = Api_Respuesta;
  //       // doc["Version_Act"] = Version_Programa;
  //       // doc["Api_Version"] = Api_Version;

  //       doc["Message"] = "Comando de actualizacion de pantalla TFT";

  //       serializeJson(doc, Payload);

  //       Serial.println("Comando de solicitud de actualizacion TFT recibida");
  //       Serial.println(Payload);

  //       for (int i = 0; i < Intentos_Conexion; i++)
  //       {
  //         if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
  //           break;
  //       }

  //       if (Await_ms(get_Flag_Update_TFT, 1000))
  //         IsSuccess = true;
  //       else
  //         IsSuccess = false;

  //       if (IsSuccess)
  //         Message = "Pantalla TFT conectada a " + SSID_Wifi;
  //       else
  //         Message = "Sin respuesta de la pantalla TFT";
  //     }else{
  //       IsSuccess = false;
  //       Message = "Falta uno o mas parametros para la solicitud";
  //     }
  //   }
  //   else
  //   {
  //     IsSuccess = false;
  //     Message = "No existe pantalla TFT Sincronizada";
  //   }

  //   jsonDocument["IsSuccess"] = IsSuccess;
  //   jsonDocument["Message"] = Message;

  //   String Json;
  //   serializeJson(jsonDocument, Json); /* Serializa Data */
  //   request->send(200, "application/json", Json);
  // }));

  Server_API.addHandler(new AsyncCallbackJsonWebHandler("/api/Fidelizacion/Update_Firmware_TFT", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                        {
    StaticJsonDocument<200> jsonDocument;
    jsonDocument.clear();

    bool IsSuccess = false;
    String Message = "";
    int Codigo;

    String VersionTFT;
    
    if (Variables_globales.Get_Variable_Global(Status_Device_TFT_Display) && Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
    {

      if (Variables_globales.Get_Variable_Global(Flag_Sesion_RFID) || Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless))
      {

        IsSuccess = false;
        Message = "No se puede iniciar actualizacion porque la maquina esta en una sesion";
        Codigo=MAQUINA_EN_JUEGO_;
      }
      else
      {

        /* No hay Sesion de juego */
        auto &&data = json.as<JsonObject>();

        if (data.containsKey("version_firmware_tft"))
        {
          /* Actualiza Version */
          Flag_Conexion_TFT = false;
          StaticJsonDocument<100> doc;

          String Payload;
          int Intentos_Conexion = 3;
          doc["IsSuccess"] = true;
          doc["Opcion"] = PING;

          serializeJson(doc, Payload);

          bool Issuccess = Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length());
          /* Conexion de pantalla OK */

          if (Await_ms(get_Flag_Conexion_TFT, 1000) || Issuccess)
          {

            String VersionUpdate = data["version_firmware_tft"].as<String>();
            if (DisplayTFT.info.Version_Firmware_tft != VersionUpdate && DisplayTFT.info.Version_Firmware_tft != ""&& VersionUpdate!="")
            {

              StaticJsonDocument<200> doc;
              doc.clear();
              String Payload = "";
              int Intentos_Conexion = 3;

              String SSID_Wifi = Configuracion.Get_Configuracion(SSID, "Nombre_Red");
              String Password_Wifi = Configuracion.Get_Configuracion(Password, "Password_red");

              char Ip[4];
              char IpServer[4];
              memcpy(Ip, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Ip) / sizeof(Ip[0]));
              memcpy(IpServer, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IpServer) / sizeof(IpServer[0]));

              IPAddress IPString(Ip[0], Ip[1], Ip[2], Ip[3]);
              IPAddress IPServerString(IpServer[0], IpServer[1], IpServer[2], IpServer[3]);

              doc["Opcion"] = UPDATE_OTA;
              doc["Ssid"] = SSID_Wifi;
              doc["password"] = Password_Wifi;
              doc["Ip"] = IPString.toString();
              doc["Ip_Server"] = IPServerString.toString();
              doc["Type_Update"] = UDATE_VIA_HTTP;

              serializeJson(doc, Payload);

              if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
              {
                IsSuccess = true;
                Message = "Comando de actualizacion recibido con exito";
                Codigo = EXITOSO;
              }
              else
              {
                IsSuccess = false;
                Message = "No se recibio respuesta del dispositivo";
                Codigo = SIN_RESPUESTA;
              }
            }
            else if (DisplayTFT.info.Version_Firmware_tft == VersionUpdate)
            {
              IsSuccess = false;
              Message = "Version de actualizacion igual a version instalada";
              Codigo = VERSION_YA_INSTALADA;
            }
            else
            {
              IsSuccess = false;
              Message = "No se pudo obtener la version de la pantalla TFT";
              Codigo = ERROR_VERSION;
            }
          }else{
            IsSuccess = false; 
            Message = "Error dispositivo no re";
            Codigo = ERROR_VERSION;
          }
        }
        else
        {
          IsSuccess = false;
          Message = "Version de actualizacion no especificada";
          Codigo = ERROR_VERSION;
        }
      }
    }
    else
    {
      IsSuccess = false;
      Message = "No existe dispositivo sincronizado o conectado";
      Codigo = NO_EXISTE_DISPOSITIVO;
    }

    jsonDocument["IsSuccess"] = IsSuccess;
    jsonDocument["Message"] = Message;
    jsonDocument["Codigo"] = Codigo;

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json); }));

  /*------------------------------------------------------------------------------------------------------*/
  /* ------------------------------------> CONFIGURA <---------------------------------------------------*/
  /* COMANDO SOCKET-API */
  Server_API.addHandler(new AsyncCallbackJsonWebHandler("/Configurar_Transmision", [](AsyncWebServerRequest* request, JsonVariant& json) {

    IPAddress ipCliente = request->client()->remoteIP();
    
    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
    bool IsSuccess=false;

    #define SOCKET 0
    #define API    1

    if (!json.is<JsonObject>())
    {
      jsonDocument["IsSuccess"] = false;
      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);
      Info_Cashless.Log(RTC, "COMANDO_CONFIGURAR_TRANSMISION "+ipCliente.toString(), "JSON_NO_ES_UN_OBJETO_VALIDO");

      #ifdef DEBUG_CONFIG_MODE_TRAMISIONS
      Serial.println("Json no es un objeto valido");
      #endif
      return;
    }

    auto &&data = json.as<JsonObject>();

    if(data.containsKey("Modo_Transmision"))
    {
      int Modo_Transmision = data["Modo_Transmision"].as<int>();
  
      NVS.begin("Config_ESP32", false);
      switch (Modo_Transmision)
      {
      case SOCKET:
        NVS.putBool("TYPE_TM", false);
        if (!NVS.getBool("TYPE_TM"))
        {
          IsSuccess = true;
          jsonDocument["Message"] = "Metodo de transmision socket configurado con exito ✅";
          Info_Cashless.Log(RTC, "COMANDO_CONFIGURAR_TRANSMISION "+ipCliente.toString(), "METODO_SOCKET_CONFIGURADO");
          #ifdef DEBUG_CONFIG_MODE_TRAMISIONS
          Serial.println("Modo Socket Configurado Con Exito");
          #endif
        }
        else
        {
          IsSuccess = false;
          jsonDocument["Message"] = "Metodo de transmision socket no configurado ❌";
          Info_Cashless.Log(RTC, "COMANDO_CONFIGURAR_TRANSMISION "+ipCliente.toString(), "METODO_SOCKET_NO_CONFIGURADO");
          #ifdef DEBUG_CONFIG_MODE_TRAMISIONS
          Serial.println("Modo Socket Configurado no configurado");
          #endif
        }
        break;

      case API:
        NVS.putBool("TYPE_TM",true);
        if(NVS.getBool("TYPE_TM"))
        {
          IsSuccess=true;
          jsonDocument["Message"] = "Metodo de transmision API configurado con exito ✅";
          Info_Cashless.Log(RTC, "COMANDO_CONFIGURAR_TRANSMISION "+ipCliente.toString(), "METODO_API_CONFIGURADO");
          #ifdef DEBUG_CONFIG_MODE_TRAMISIONS
          Serial.println("Modo API Configurado Con Exito");
          #endif
        }
        else
        {
          IsSuccess=false;
          jsonDocument["Message"] = "Metodo de transmision API no configurado ❌";
          Info_Cashless.Log(RTC, "COMANDO_CONFIGURAR_TRANSMISION "+ipCliente.toString(), "METODO_API_NO_CONFIGURADO");
          #ifdef DEBUG_CONFIG_MODE_TRAMISIONS
          Serial.println("Modo API No Configurado");
          #endif
        }
        break;
      
      default:
        IsSuccess=false;
        jsonDocument["Message"] = "Metodo de transmision no identificado:❌"+ String(Modo_Transmision);
        Info_Cashless.Log(RTC, "COMANDO_CONFIGURAR_TRANSMISION "+ipCliente.toString(), "METODO_TRANSMISION_NO_IDENTIFICADO");
        #ifdef DEBUG_CONFIG_MODE_TRAMISIONS
        Serial.println("Modo De Transmision no identificado");
        #endif
        break;
      }
        NVS.end();

    }else
    {
      IsSuccess=false;
      jsonDocument["Message"] = "Metodo de transmision no recibido ❌";
      String Key="Modo_Transmision";
      Info_Cashless.Log(RTC, "COMANDO_CONFIGURAR_TRANSMISION "+ipCliente.toString(), "NO_EXISTE_LLAVE:"+Key);
      #ifdef DEBUG_CONFIG_MODE_TRAMISIONS
      Serial.println("No existe la  llave en el objeto de modo tranmision");
      #endif
    }

    jsonDocument["IsSuccess"] = IsSuccess;
    
    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);

    if (IsSuccess)
    {
      Info_Cashless.Log(RTC, "REINICIO_DISPOSITIVO "+ipCliente.toString(), "CONFIGURACION_MODO_TRANMISION");
      Serial.println("Configuracion modo de transmision aplicada.. Reiniciando...");
      delay(2000);
      ESP.restart();
    }
  
  }));
  /* COMANDO URL MODO API */
  Server_API.addHandler(new AsyncCallbackJsonWebHandler("/Configurar_URL_API", [](AsyncWebServerRequest* request, JsonVariant& json) {

    IPAddress ipCliente = request->client()->remoteIP();
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());


    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
    bool IsSuccess=false;
    String Msg;

    if (!json.is<JsonObject>())
    {
      Msg= "No es un  objeto json valido";
      jsonDocument["IsSuccess"] = IsSuccess;
      jsonDocument["Fecha_Hora"] = DataTime;
      jsonDocument["Message"] = Msg;
      String Json;
      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);
      Info_Cashless.Log(RTC, "COMANDO_CONFIGURAR_URL_API_RECIBIDO "+ipCliente.toString(), "JSON_NO_ES_UN_OBJETO_VALIDO");
      return;
    }

    auto &&data = json.as<JsonObject>();

    if (!data.containsKey("Controlador") && !data.containsKey("Metodo_Contadores")&&!data.containsKey("Metodo_Eventos")  && !data.containsKey("Metodo_RTC") && !data.containsKey("Metodo_Token"))
    {
      String Json;

      Msg="No existen las llaves en el objeto json";
      jsonDocument["IsSuccess"] = IsSuccess;
      jsonDocument["Fecha_Hora"] = DataTime;
      jsonDocument["Message"] = Msg;

      serializeJson(jsonDocument, Json); /* Serializa Data */
      request->send(200, "application/json", Json);
      Info_Cashless.Log(RTC, "COMANDO_CONFIGURAR_URL_API_RECIBIDO "+ipCliente.toString(), "NO_EXISTE_INFORMACION_CONFIGURACION");
      return;
    }
    else
    {

      NVS.begin("Config_ESP32", false);

      String Controlador_Principal_Actual = Configuracion.Get_Configuracion_ES(Controlador_P, "Controlador_Principal");
      String Metodo_RTC__Actual = Configuracion.Get_Configuracion_ES(Metodo_Sincro_RTC, "Metodo_SincroRTC");
      String Metodo_Contadores__Actual = Configuracion.Get_Configuracion_ES(Metodo_Conta, "Metodo_Contadores");
      String Metodo_Eventos__Actual = Configuracion.Get_Configuracion_ES(Metodo_Event, "Metodo_Eventos");
      String Metodo_Token__Actual = Configuracion.Get_Configuracion_ES(Metodo_Access_T, "Metodo_Token");

      String Config = "";

      String Controlador_Principal_Update; 
      String Metodo_Contadores_Update;

      String Metodo_RTC_Update; 
      String Metodo_Eventos_Update;
      String Metodo_Token_Update; 

      if (!data.containsKey("Controlador"))
      {
        Controlador_Principal_Update=Controlador_Principal_Actual;
        Config = Controlador_Principal_Update;
      }
      else
      {
        Controlador_Principal_Update=data["Controlador"].as<String>();
        if(Controlador_Principal_Update=="")
          Controlador_Principal_Update=Controlador_Principal_Actual;
        Config = Controlador_Principal_Update;
      }
        
      Config += "|";

      if(!data.containsKey("Metodo_RTC"))
      {
        Metodo_RTC_Update=Metodo_RTC__Actual;
        Config+=Metodo_RTC_Update;
      }else
      {
        Metodo_RTC_Update=data["Metodo_RTC"].as<String>();
        Config+=Metodo_RTC_Update;
      }
      
      Config += "|";
      if (!data.containsKey("Metodo_Contadores"))
      {
        Metodo_Contadores_Update=Metodo_Contadores__Actual;
        Config += Metodo_Contadores_Update;
      }
      else
      {
        Metodo_Contadores_Update=data["Metodo_Contadores"].as<String>();
        Config += Metodo_Contadores_Update;
      }
        
      Config += "|";

      if (!data.containsKey("Metodo_Eventos"))
      {
        Metodo_Eventos_Update=Metodo_Eventos__Actual;
        Config += Metodo_Eventos_Update;
      }
      else
      {
        Metodo_Eventos_Update=data["Metodo_Eventos"].as<String>();
        Config += Metodo_Eventos_Update;
      }
        
      Config += "|";

      if (!data.containsKey("Metodo_Token"))
      {
        Metodo_Token_Update=Metodo_Token__Actual;
        Config += Metodo_Token_Update;
      }
      else
      {
        Metodo_Token_Update=data["Metodo_Token"].as<String>();
        Config += Metodo_Token_Update;
      }
        
      //Serial.println(Config);

      NVS.putString("Param_API", Config);

      String savedString = NVS.getString("Param_API", "");
      String ParametersAp[5];

      int pos = 0;
      int startPos = 0;
      for (int i = 0; i < savedString.length(); i++)
      {
        if (savedString.charAt(i) == '|')
        {
          ParametersAp[pos++] = savedString.substring(startPos, i);
          startPos = i + 1;
        }
      }
      // Agrega el último elemento
      ParametersAp[pos] = savedString.substring(startPos);

      Configuracion.Set_Configuracion_ESP32_ES(Controlador_P, ParametersAp);
      Configuracion.Set_Configuracion_ESP32_ES(Metodo_Conta, ParametersAp);
      Configuracion.Set_Configuracion_ESP32_ES(Metodo_Event, ParametersAp);
      Configuracion.Set_Configuracion_ESP32_ES(Metodo_Sincro_RTC, ParametersAp);
      Configuracion.Set_Configuracion_ESP32_ES(Metodo_Access_T, ParametersAp);

      NVS.end();

      if (Configuracion.Get_Configuracion_ES(Controlador_P, "Controlador_Principal") == Controlador_Principal_Update && Configuracion.Get_Configuracion_ES(Metodo_Conta, "Metodo_Contadores") == Metodo_Contadores_Update &&Configuracion.Get_Configuracion_ES(Metodo_Event,"Metodo_Eventos")==Metodo_Eventos_Update && Configuracion.Get_Configuracion_ES(Metodo_Sincro_RTC,"Metodo_SincroRTC")==Metodo_RTC_Update &&Configuracion.Get_Configuracion_ES(Metodo_Access_T,"Metodo_Token")==Metodo_Token_Update)
      {
        IsSuccess = true;
        Msg="Configuracion Realizada con exito";
        Info_Cashless.Log(RTC, "COMANDO_CONFIGURAR_URL_API_RECIBIDO "+ipCliente.toString(), "CONFIGURACION_EXITOSA: "+Config);
      }
        
      else
      {
        IsSuccess = false;
        Msg="No fue posible guardar la configuracion";
        Info_Cashless.Log(RTC, "COMANDO_CONFIGURAR_URL_API_RECIBIDO "+ipCliente.toString(), "NO_FUE_POSIBLE_GUARDAR_LA_CONFIGURACION: "+Config);
      }
        
    }
    jsonDocument["IsSuccess"]=IsSuccess;
    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Message"]=Msg;

  
    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);

    delay(1000);
    if(IsSuccess)
      ESP.restart();
  }));
  /*-------------------------------------------------------------------------------------------------------*/

  Server_API.on("/SD_Formato_FAT32", HTTP_GET, [](AsyncWebServerRequest *request)
  {

    bool IsSuccess=false;
    String Msg="";

    IPAddress ipCliente = request->client()->remoteIP();
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    StaticJsonDocument<200> jsonDocument;

    // Verifica que la tarjeta SD esté montada y que el archivo exista
    if (!Variables_globales.Get_Variable_Global(SD_INSERT))
    {
      IsSuccess = false;
      Msg = "Memoria SD no insertada o no inicializada 💾❌";
    }
    else
    {

      if (!Formateo && Result_Formatt==0 && Consulta_Result_Formatt)
      {
        IsSuccess = true;
        Msg = "Solicitud Formateo SDFAT32 recibida ✅";
        Formateo=true;
        Consulta_Result_Formatt=false;
      }else
      {
        IsSuccess = false;
        Msg = "Proceso de formateo actualmente en curso! ⏳";
      }

    }

    jsonDocument["IsSuccess"] = IsSuccess;
    jsonDocument["Message"] = Msg;
    jsonDocument["Fecha_Hora"] = DataTime;

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);

    });

  Server_API.on("/Resultado_Formateo_SDFAT32", HTTP_GET, [](AsyncWebServerRequest *request)
  {

    bool IsSuccess=false;
    String Msg="";

    IPAddress ipCliente = request->client()->remoteIP();
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    StaticJsonDocument<200> jsonDocument;

    // Verifica que la tarjeta SD esté montada y que el archivo exista


    

    if (!Variables_globales.Get_Variable_Global(SD_INSERT))
    {
      IsSuccess = false;
      Msg = "Memoria SD no insertada o no inicializada 💾❌";
    }
    else
    {

      switch (Result_Formatt)
      {
      case FORMAT_NOT_STARTED:
        IsSuccess = false;
        Msg = "No existe proceso de formateo Iniciado";
        Info_Cashless.Log(RTC,"FORMATO_FAT32_SD","NO-EXISTE-PROCESO"+ ipCliente.toString());
        break;

      case FORMAT_IN_PROGRESS:
        IsSuccess = false;
        Msg = "Formateando tarjeta SD....";

        break;

      case FORMAT_SUCCESS:
        IsSuccess = true;
        Msg = "Tarjeta formateada con exito! ✅";
        Result_Formatt=0;

        Info_Cashless.Log(RTC,"FORMATO_FAT32_SD","MEMORIA-FORMATEADA-CON-EXITO-DESDE- "+ ipCliente.toString());
        break;

      case FORMAT_FAILED:
        IsSuccess = false;
        Msg = "Falla  formateando tarjeta SD ❌";
        Result_Formatt=0;
        break;

      default:

        IsSuccess = false;
        Msg = "Falla  formateando tarjeta SD ❌";
        Result_Formatt=0;
        break;
      }

    }

    jsonDocument["IsSuccess"] = IsSuccess;
    jsonDocument["Message"] = Msg;
    jsonDocument["Fecha_Hora"] = DataTime;

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);
    Consulta_Result_Formatt=true;
    });

  Server_API.on("/Ping_Globus", HTTP_GET, [](AsyncWebServerRequest *request)
  {

    /* *********************************Borrar */
    // NVS.begin("Config_ESP32", false);
    // uint8_t Adress_TFT_Display[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    // NVS.putBytes("Address_TFT", Adress_TFT_Display, sizeof(Adress_TFT_Display));
    // NVS.end();

    char version_str[16];
    sprintf(version_str, "%d.%d.%d.%d", Version_Firmware_[0], Version_Firmware_[1], Version_Firmware_[2], Version_Firmware_[3]);

    bool IsSuccess=false;


    IPAddress ipCliente = request->client()->remoteIP();
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    StaticJsonDocument<512> jsonDocument;

    // /* Estados Maquina */
    jsonDocument["Comunicacion_Maq"] = Variables_globales.Get_Variable_Global(Comunicacion_Maq);
    jsonDocument["Maquina_En_Juego"] = Variables_globales.Get_Variable_Global(Flag_Maquina_En_Juego);
    jsonDocument["Estado_Cashless"]= Variables_globales.Get_Variable_Global(Enable_Cashless);
    jsonDocument["Estado_Tito"]= Variables_globales.Get_Variable_Global(Enable_Tito_Ticket);
    jsonDocument["Evento_Maq_Juego"]= Variables_globales.Get_Variable_Global(Flag_Maquina_Juego_Evento);
    
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

    case 4:
      jsonDocument["Tipo_Maq"] = "IRT";
    break;

    case 5:
      jsonDocument["Tipo_Maq"] = "Generica";
    break;

    case 6:
      jsonDocument["Tipo_Maq"] = "Poker";
    break;

    case 17:
      jsonDocument["Tipo_Maq"] = "EFT";
      break;


    default:
      jsonDocument["Tipo_Maq"] = "";
      break;
    }

    // /* Informacion Jugador */
    jsonDocument["Estado_Sesion"]= Variables_globales.Get_Variable_Global(Flag_Sesion_RFID); 
    jsonDocument["Creditos_Actuales"]= contadores.Get_Contadores_Int(24);
    jsonDocument["Id_Cliente"]= contadores.Get_Client_ID_Int();
    if (contadores.Get_Client_ID_Int() > 0)
    {

      String Tipo_Sesion = "";
      switch (Info_Cashless.Type_Sesion())
      {
      case PLAYER_CASHLESS_SESION:
        Tipo_Sesion = "Cashless";
        break;

      case PLAYER_TRACKING_SESION:
        Tipo_Sesion = "Fidelizacion";
        break;

      default:
        Tipo_Sesion = "No Identificada";
        break;
      }

      jsonDocument["Tipo_Sesion"] = Tipo_Sesion;
    }


    // /* Informacion de red */
    jsonDocument["Nivel_Señal_Wifi"] =calcularNivelSenal(WiFi.RSSI());
    jsonDocument["Canal_WiFi"] = WiFi.channel();


   
    jsonDocument["Estado_WiFi"] = WiFi.isConnected() ? "CONECTADO" : "DESCONECTADO";
    jsonDocument["Hostname"]=WiFi.getHostname();
    jsonDocument["IsSuccess"] =  WiFi.isConnected() ? IsSuccess=true: IsSuccess =false;
    jsonDocument["Message"] = "Informacion generada con exito ✅";
    jsonDocument["Fecha_Hora"] = DataTime;


    jsonDocument["Firmware_Version"] = version_str;

    if(Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
    {
      jsonDocument["Conexion_TFT"] = true;
      jsonDocument["Firmware_Version_TFT"] = DisplayTFT.info.Version_Firmware_tft;
    }
    else
      jsonDocument["Conexion_TFT"] = false;

    
    if(Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
      jsonDocument["Mac_TFT"] = getMacString(Address_Device_TFT_Display);
    else
      jsonDocument["Mac_TFT"] = NULL;

    
    if(Variables_globales.Get_Variable_Global(Conexion_RFID))
      jsonDocument["Lector_RFID"] = true;
    else
      jsonDocument["Lector_RFID"] = false;


    if(Variables_globales.Get_Variable_Global(Ftp_Mode))
      jsonDocument["Modo_FTP"] = true;
    else
      jsonDocument["Modo_FTP"] = false;

    
    if(Variables_globales.Get_Variable_Global(Bootloader_Mode))
      jsonDocument["Bootloader"] = true;
    else
      jsonDocument["Bootloader"] = false;

    if(Variables_globales.Get_Variable_Global(Updating_System))
      jsonDocument["AutoUpdate"] = true;
    else
      jsonDocument["AutoUpdate"] = false;

    jsonDocument["Mac_Globus_IM"]=WiFi.macAddress();
    

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */

    jsonDocument.clear();
    request->send(200, "application/json", Json);
    //Consulta_Result_Formatt=true;
    //Info_Cashless.Log(RTC,"SOLICITUD_PING_GLOBUS_RECIBIDA"+Json);
  });


  Server_API.on("/api/Configuracion/Habilitar_Lectura_Sesiones_Sin_Tarjeta", HTTP_GET, [](AsyncWebServerRequest *request)
  {

  
    bool IsSuccess=false;
    String Msg="";

    StaticJsonDocument<800> jsonDocument;

    IPAddress ipCliente = request->client()->remoteIP();
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    
    NVS.begin("Config_ESP32", false);
    NVS.putBool("Sesiones_Ac",true);
    bool Test=NVS.getBool("Sesiones_Ac",false);
    NVS.end();


    if(Test)
    {
      IsSuccess=true;
      Msg="Sesiones acumuladas sin tarjeta habilitadas correctamente";
      Variables_globales.Set_Variable_Global(Flag_Sesiones_Acumuladas_Sin_Tarjeta,true);
    }else{
      IsSuccess=false;
      Msg="Error habilitando sesiones acumuladas sin tarjeta ";
      Variables_globales.Set_Variable_Global(Flag_Sesiones_Acumuladas_Sin_Tarjeta,false);
    }

    // /* Estados Maquina */
    jsonDocument["IsSuccess"] = IsSuccess;
    jsonDocument["Message"] = Msg;
    
    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);
    //Consulta_Result_Formatt=true;

    if(IsSuccess)
      Info_Cashless.Log(RTC, "COMANDO_HABILITAR_SESIONES "+ipCliente.toString(), "CONFIGURACION_EXITOSA");
    else
      Info_Cashless.Log(RTC, "COMANDO_HABILITAR_SESIONES "+ipCliente.toString(), "ERROR SESIONES SIN TARJETA NO HABILITADAS");
    //Info_Cashless.Log(RTC,"SOLICITUD_PING_GLOBUS_RECIBIDA"+Json);
  });

  Server_API.on("/api/Configuracion/Deshabilitar_Lectura_Sesiones_Sin_Tarjeta", HTTP_GET, [](AsyncWebServerRequest *request)
  {

  
    bool IsSuccess=false;
    String Msg="";

    StaticJsonDocument<800> jsonDocument;

    IPAddress ipCliente = request->client()->remoteIP();
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    
    NVS.begin("Config_ESP32", false);
    NVS.putBool("Sesiones_Ac",false);
    bool Test=NVS.getBool("Sesiones_Ac",false);
    NVS.end();


    if(!Test)
    {
      IsSuccess=true;
      Msg="Sesiones acumuladas sin tarjeta deshabilitadas";
      Variables_globales.Set_Variable_Global(Flag_Sesiones_Acumuladas_Sin_Tarjeta,false);
    }else{
      IsSuccess=false;
      Msg="Error deshabilitando sesiones acumuladas sin tarjeta ";
      Variables_globales.Set_Variable_Global(Flag_Sesiones_Acumuladas_Sin_Tarjeta,true);
    }

    // /* Estados Maquina */
    jsonDocument["IsSuccess"] = IsSuccess;
    jsonDocument["Message"] = Msg;
    
    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);
    //Consulta_Result_Formatt=true;

    if(IsSuccess)
      Info_Cashless.Log(RTC, "COMANDO_DESHABILITAR_SESIONES "+ipCliente.toString(), "CONFIGURACION_EXITOSA");
    else
      Info_Cashless.Log(RTC, "COMANDO_DESHABILITAR_SESIONES "+ipCliente.toString(), "ERROR SESIONES SIN TARJETA NO DESHABILITADAS");
    //Info_Cashless.Log(RTC,"SOLICITUD_PING_GLOBUS_RECIBIDA"+Json);
  });

 



  Server_API.on("/api/Configuracion/Habilitar_Simulador", HTTP_GET, [](AsyncWebServerRequest *request)
  {

    bool IsSuccess=false;
    String Msg="";

    IPAddress ipCliente = request->client()->remoteIP();
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    StaticJsonDocument<200> jsonDocument;


    NVS.begin("Config_ESP32", false);
    NVS.putBool("Simulador",true);
    bool test=NVS.getBool("Simulador",true);
    NVS.end();

    if (test)
    {
      IsSuccess = true;
      Msg="Simulador habilitado con exito";
    }else{
      IsSuccess = false;
      Msg="Error habilitando Simulador";
    }

    // Verifica que la tarjeta SD esté montada y que el archivo exista
    

    jsonDocument["IsSuccess"] = IsSuccess;
    jsonDocument["Message"] = Msg;
    jsonDocument["Fecha_Hora"] = DataTime;

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);

    delay(1000);
    ESP.restart();

    });

  Server_API.on("/api/Configuracion/Inhabilitar_Simulador", HTTP_GET, [](AsyncWebServerRequest *request)
  {

    bool IsSuccess=false;
    String Msg="";

    IPAddress ipCliente = request->client()->remoteIP();
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    StaticJsonDocument<200> jsonDocument;


    NVS.begin("Config_ESP32", false);
    NVS.putBool("Simulador",false);
    bool test=NVS.getBool("Simulador",false);
    NVS.end();

    if (!test)
    {
      IsSuccess = true;
      Msg="Simulador inhabilido con exito";
    }else{
      IsSuccess = false;
      Msg="Error inhabilitando simulador";
    }

    // Verifica que la tarjeta SD esté montada y que el archivo exista
    

    jsonDocument["IsSuccess"] = IsSuccess;
    jsonDocument["Message"] = Msg;
    jsonDocument["Fecha_Hora"] = DataTime;

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);

    delay(1000);
    ESP.restart();

    });


  Server_API.on("/sas_log", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if (SPIFFS.exists("/sas_log.txt"))
    {
        request->send(SPIFFS, "/sas_log.txt", "text/plain");
    }
    else
    {
        request->send(404, "text/plain", "Log no encontrado");
    }
  });


  Server_API.on("/borrar_sas_log", HTTP_GET, [](AsyncWebServerRequest *request) {

    if (SPIFFS.remove("/sas_log.txt"))
        request->send(200, "text/plain", "Log borrado");
    else
        request->send(500, "text/plain", "Error al borrar");

  });



//     Server_API.addHandler(new AsyncCallbackJsonWebHandler("/ConfigBA", [](AsyncWebServerRequest* request, JsonVariant& json) 
//     {

//     ipDest = request->client()->remoteIP();
    
//     String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());

//     StaticJsonDocument<500> jsonDocument;
//     jsonDocument.clear();
//     bool IsSuccess=false;
//     String Msg;

//     String texto = request->hasHeader("texto") ? request->getHeader("texto")->value() : "";
//     String maqIp = request->hasHeader("maqIp") ? request->getHeader("maqIp")->value() : "";
//     String widget = request->hasHeader("widget") ? request->getHeader("widget")->value() : "";
//     String time = request->hasHeader("time") ? request->getHeader("time")->value() : "";
//     String hashOnline = request->hasHeader("hash") ? request->getHeader("hash")->value() : "";

//     String hasInput = texto + "_^_" + maqIp + "_^_" + widget + "_^_" + time;
//     String shaLocal = GenerarCodigoSHA1(hasInput);

//     Serial.println(hashOnline);
//     Serial.println(shaLocal);

//     if (shaLocal.equalsIgnoreCase(hashOnline))
//     {

//       Serial.println("Hash Ok");
//       if (!json.is<JsonObject>())
//       {
//         jsonDocument["IsSuccess"] = false;
//         String Json;
//         serializeJson(jsonDocument, Json); /* Serializa Data */
//         request->send(200, "application/json", Json);
//         Info_Cashless.Log(RTC, "COMANDO_BA" + ipDest.toString(), "JSON_NO_ES_UN_OBJETO_VALIDO");

// #ifdef DEBUG_CONFIG_MODE_TRAMISIONS
//         Serial.println("Json no es un objeto valido");
// #endif
//         return;
//       }

//       if (AFT.GET_STATUS_BA() == TransaccionCashless::BA_IDLE)
//       {

//         auto &&data = json.as<JsonObject>();

//         if (data.containsKey("Assets") && data.containsKey("ID") && data.containsKey("GUID"))
//         {

//           solicitudBA.pendiente = true;
//           solicitudBA.ID = data["ID"].as<int>();
//           solicitudBA.GUID = data["GUID"].as<String>();

//           // LIMPIAR ANTES DE COPIAR
//           assetsBuffer.clear();

//           // copiar el array de assets
//           JsonArray assets = data["Assets"].as<JsonArray>();

//           int i = 0;

//           for (JsonVariant v : assets)
//           {
//             const char *c = v.as<const char *>();
//             solicitudBA.assets[i++] = c[0];
//           }

//           solicitudBA.count = i;

//           IsSuccess = true;
//           Msg = "Solicitud recibida correctamente";
//         }else{
//           IsSuccess = false;
//           Msg = "Falta uno o mas parametros para procesar la solicitud";
//         }
//       }
//       else
//       {
//         IsSuccess = false;
//         Msg = "Dispositivo ocupado para procesar la solicitud";
//       }
//     }
//     else
//     {
//       IsSuccess=false;
//       Msg="Hash no compatible para procesar la solicitud";
//     }

    
//     jsonDocument["IsSuccess"]=IsSuccess;
//     jsonDocument["Message"]=Msg;
//     jsonDocument["Data"] = DataTime;

//     String Json;
//     serializeJson(jsonDocument, Json); /* Serializa Data */
//     request->send(200, "application/json", Json);

  
//   }));

    //Server_API.begin();

  //   Server_API.addHandler(new AsyncCallbackJsonWebHandler("/api/Fidelizacion/BonoCanje", [](AsyncWebServerRequest* request, JsonVariant& json) {

    
  //   int Code=0x100;
  //   StaticJsonDocument<200> jsonDocument;
  //   jsonDocument.clear();
    
  //   bool IsSuccess_Response=false;
  //   String Msg;

  //   if (!json.is<JsonObject>()) {

  //     jsonDocument["IsSuccess"] = false;
  //     jsonDocument["IsSuccess"] = Code;
  //     jsonDocument["IsSuccess"] = "Tipo de dato no identificado JSON";
  //     String Json;
  //     serializeJson(jsonDocument, Json); /* Serializa Data */
  //     request->send(200, "application/json", Json);

  //     return;
  //   }else
  //   {

  //     if (AFT.GET_STATUS_TRANSFER() == TransaccionCashless::TRANS_IDLE)
  //     {


  //       if(AFT.GET_STATUS_TRANSFER() == TransaccionCashless::TRANS_IDLE)
  //         AFT.STATUS_TRANSFER(TransaccionCashless::TRANS_RECIBIDA);

  //       auto &&data = json.as<JsonObject>();
  //       /* Url Generica */

        


  //       bool IsSuccess = data["IsSuccess"];
  //       int Current_Cliente_ID_Server_Int = data["Cliente_ID"];
  //       String Type_Trans_Server = data["Trans_Tipo"];
  //       uint32_t Cashable_Server = data["Saldo_Canjeable"];
  //       uint32_t Restricted_Server = data["Saldo_Restringido"];
  //       uint32_t Non_Restricted_Server = data["Saldo_No_Restringido"];
  //       uint32_t Trans_ID_Server = data["Trans_ID"];
  //       String Data_Time_Response_Server = data["Fecha_Hora"];
  //       String Msgg = data["Message"];
  //       String Nombre_Cliente = data["Cliente_Nombre"];

  //       int Cashless_ID = 0;
  //       String Cashless_Estado = data["Cashless_Estado"];
  //       String Ip_Tarjeta = data["Ip"];
  //       String Key = data["Key"];
  //       String Mac = data["MAC"];
  //       String Trans_Estado = data["Trans_Estado"];
  //       String Tipo_Maq = data["Tipo_Maq"];

  //       int Year, Month, Day, Hour, Minutes, Seconds;
  //       sscanf(Data_Time_Response_Server.c_str(), "%d-%d-%d %d:%d:%d", &Year, &Month, &Day, &Hour, &Minutes, &Seconds);

  //       Objeto_Transfer["IsSuccess"] = false;
  //       Objeto_Transfer["Cliente_ID"] = Current_Cliente_ID_Server_Int;
  //       Objeto_Transfer["Trans_Tipo"] = Type_Trans_Server;
  //       Objeto_Transfer["Saldo_Canjeable"] = Cashable_Server;
  //       Objeto_Transfer["Saldo_Restringido"] = Restricted_Server;
  //       Objeto_Transfer["Saldo_No_Restringido"] = Non_Restricted_Server;
  //       Objeto_Transfer["Trans_ID"] = Trans_ID_Server;
  //       Objeto_Transfer["Fecha_Hora"] = Data_Time_Response_Server;
  //       Objeto_Transfer["Message"] = Msgg;
  //       Objeto_Transfer["Cliente_Nombre"] = Nombre_Cliente;

  //       Objeto_Transfer["Cashless_Estado"] = Cashless_Estado;
  //       Objeto_Transfer["Ip"] = Ip_Tarjeta;
  //       Objeto_Transfer["Key"] = Key;
  //       Objeto_Transfer["MAC"] = Mac;
  //       Objeto_Transfer["Trans_Estado"] = Trans_Estado;
  //       Objeto_Transfer["Tipo_Maq"] = "AFT";

  //       if (Cashless.Set_Amount_To_Load(Cashable_Server, Restricted_Server, Non_Restricted_Server))
  //       {
  //         Solicitud_Carga_Cashless();

  //         IsSuccess_Response = true;
  //         Msg = "Solicitud Recibida con exito!";
  //       }
  //       else
  //       {
  //         IsSuccess_Response=false;
  //         Msg = "Error en Conversion de saldos";

  //         if(AFT.GET_STATUS_TRANSFER() == TransaccionCashless::TRANS_RECIBIDA)
  //           AFT.STATUS_TRANSFER(TransaccionCashless::TRANS_IDLE);
  //       }
  //     }else
  //     {
  //       IsSuccess_Response=false;
  //       Msg = "Dispositivo ocupado, ya existe una instancia";
  //     }

  //     jsonDocument["IsSuccess"] = IsSuccess_Response;
  //     jsonDocument["Message"] = Msg;

  //     String Json;
  //     serializeJson(jsonDocument, Json); /* Serializa Data */
  //     request->send(200, "application/json", Json);

  //   }
    
  // }));

  Server_API.on("/api/Configuracion/Deshabilitar_Validacion_Creditos_Del_Rele", HTTP_GET, [](AsyncWebServerRequest *request)
  {

    bool IsSuccess=false;
    String Msg="";

    StaticJsonDocument<200> jsonDocument;

    IPAddress ipCliente = request->client()->remoteIP();
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    
    NVS.begin("Config_ESP32", false);
    NVS.putBool("V_Creditos",true);
    bool Test=NVS.getBool("V_Creditos",false);
    NVS.end();

    if(Test)
    {
      IsSuccess=true;
      Msg="Validacion de creditos del rele deshabilitada correctamente";
    }else{
      IsSuccess=false;
      Msg="Error deshabilitando validacion de creditos del rele";
    }

    // /* Estados Maquina */
    jsonDocument["IsSuccess"] = IsSuccess;
    jsonDocument["Message"] = Msg;
    
    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);
    //Consulta_Result_Formatt=true;

    delay(1000);
    ESP.restart();

  });


  Server_API.on("/api/Configuracion/Habilitar_Validacion_Creditos_Del_Rele", HTTP_GET, [](AsyncWebServerRequest *request)
  {

    bool IsSuccess=false;
    String Msg="";

    StaticJsonDocument<200> jsonDocument;

    IPAddress ipCliente = request->client()->remoteIP();
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    
    NVS.begin("Config_ESP32", false);
    NVS.putBool("V_Creditos",false);
    bool Test=NVS.getBool("V_Creditos",false);
    NVS.end();

    if(!Test)
    {
      IsSuccess=true;
      Msg="Validacion de creditos del rele habilitada correctamente";
    }else{
      IsSuccess=false;
      Msg="Error habilitando validacion de creditos del rele";
    }

    // /* Estados Maquina */
    jsonDocument["IsSuccess"] = IsSuccess;
    jsonDocument["Message"] = Msg;
    
    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    request->send(200, "application/json", Json);
    //Consulta_Result_Formatt=true;

    delay(1000);
    ESP.restart();
  });

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

uint32_t Transsaccion_Cashless::FBCDtoUint32(char Credit_To_Load[], int Select)
{

  char str[9]; // 8 caracteres + terminador nulo
  uint32_t Credit_Cashables = 0;

  switch (Select)
  {
  case CASHABLES:
    for (int i = 0; i < 4; i++)
    {
      str[i * 2] = (Credit_To_Load[i] >> 4) + '0';
      str[i * 2 + 1] = (Credit_To_Load[i] & 0x0F) + '0';
    }
    str[8] = '\0'; // Asegurar terminación de cadena
    sscanf(str, "%u", &Credit_Cashables);
    return Credit_Cashables;

  case RESTRICTED:
    for (int i = 0; i < 4; i++)
    {
      str[i * 2] = (Credit_To_Load[4 + i] >> 4) + '0';
      str[i * 2 + 1] = (Credit_To_Load[4 + i] & 0x0F) + '0';
    }
    str[8] = '\0'; // Asegurar terminación de cadena
    sscanf(str, "%u", &Credit_Cashables);
    return Credit_Cashables;

  case NON_RESTRICTED:
    for (int i = 0; i < 4; i++)
    {
      str[i * 2] = (Credit_To_Load[8 + i] >> 4) + '0';
      str[i * 2 + 1] = (Credit_To_Load[8 + i] & 0x0F) + '0';
    }
    str[8] = '\0'; // Asegurar terminación de cadena
    sscanf(str, "%u", &Credit_Cashables);
    return Credit_Cashables;

  default:
    return Credit_Cashables;
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

uint32_t Transsaccion_Cashless::BCD4toUint32_New_Prototipo(char *buffer, int startIndex)
{
  char str[9];

  for (int i = 0; i < 4; i++)
  {
    str[i * 2] =
        ((buffer[startIndex + i] >> 4) & 0x0F) + '0';

    str[i * 2 + 1] =
        (buffer[startIndex + i] & 0x0F) + '0';
  }

  str[8] = '\0';

  uint32_t value;

  sscanf(str, "%u", &value);

  return value;
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


/* Metodo para convertir saldos  de entero a protocolo AFT/EFT */
bool Transsaccion_Cashless::Set_Amount_To_Load(uint32_t Credit_Cashables, uint32_t Credit_Restringidos, uint32_t Credit_No_Restringidos)
{

  Credit_Cashables=Credit_Cashables;
  Credit_Restringidos=Credit_Restringidos;
  Credit_No_Restringidos=Credit_No_Restringidos;

  for(int i = 0; i < 15; i++) {
    Credit_To_Load[i] = 0;
  }

  /* Protocolo EFT */
  if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
  {

    char str[9], str2[9], str3[9]; // 8 dígitos + terminador nulo

    // Formatear los números a 8 dígitos con ceros a la izquierda
    sprintf(str, "%08d", Credit_Cashables);
    sprintf(str2, "%08d", Credit_Restringidos);
    sprintf(str3, "%08d", Credit_No_Restringidos);

    // Convertir cada par de caracteres a un byte BCD para Cashables
    for (int i = 0; i < 4; i++) {
        Credit_To_Load[i] = ((str[i * 2] - '0') << 4) | (str[i * 2 + 1] - '0');
    }

    // Convertir cada par de caracteres a un byte BCD para Restricted
    for (int i = 0; i < 4; i++) {
        Credit_To_Load[4 + i] = ((str2[i * 2] - '0') << 4) | (str2[i * 2 + 1] - '0');
    }

    // Convertir cada par de caracteres a un byte BCD para Non-Restricted
    for (int i = 0; i < 4; i++) {
        Credit_To_Load[8 + i] = ((str3[i * 2] - '0') << 4) | (str3[i * 2 + 1] - '0');
    }

    // for (int i = 0; i < 4; i++)
    // {
    //   Serial.println(Credit_To_Load[i],HEX);
    // }

    // Serial.println(BCDtoUint32(Credit_To_Load,CASHABLES));
    // Serial.println(BCDtoUint32(Credit_To_Load,RESTRICTED));
    // Serial.println(BCDtoUint32(Credit_To_Load,NON_RESTRICTED));

    /* Realiza operacion contraria para verificar la igualdad de los datos y validar la conversion exitosa de los saldos */

    if (FBCDtoUint32(Credit_To_Load, CASHABLES) == Credit_Cashables && FBCDtoUint32(Credit_To_Load, RESTRICTED) == Credit_Restringidos && FBCDtoUint32(Credit_To_Load, NON_RESTRICTED) == Credit_No_Restringidos)
      return true;
    else
      return false;
  }
  else
  {

    /* Protocolo AFT */

    // Convertir el número a cadena con 10 caracteres, rellenando con ceros a la izquierda si es necesario
    char str[11];
    char str2[11];
    char str3[11];
    sprintf(str, "%010d", Credit_Cashables);
    sprintf(str2, "%010d", Credit_Restringidos);
    sprintf(str3, "%010d", Credit_No_Restringidos);

    // Convertir cada par de caracteres a un byte BCD Cashables
    for (int i = 0; i < 5; i++)
    {
      Credit_To_Load[i] = ((str[i * 2] - '0') << 4) | (str[i * 2 + 1] - '0');
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

    if (BCDtoUint32(Credit_To_Load, CASHABLES) == Credit_Cashables && BCDtoUint32(Credit_To_Load, RESTRICTED) == Credit_Restringidos && BCDtoUint32(Credit_To_Load, NON_RESTRICTED) == Credit_No_Restringidos)
      return true;
    else
      return false;
  }
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


char Transsaccion_Cashless::Get_ACK_Bonus_Copia(void)
{
  return Flag_Legacy_Pay;
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

int Transsaccion_Cashless::Get_Status_AFT_Bonus(int Update_Status)
{
  return Status_AFT_B;
}

void Transsaccion_Cashless::Set_Status_AFT_Bonus(int Status)
{
  Status_AFT_B=Status;
}

bool Transsaccion_Cashless::Requerimiento_Bonusing(int Evento, bool Enable_Bonusing)
{

  if (Enable_Bonusing)
  {
    if (Evento == 0x7C)
    {
      Flag_Legacy_Pay = 0x7C;
      Serial.println(" Bono cargado....!");
      Variables_globales.Set_Variable_Global(Solicitud_Carga_Bonus,false);
    }
    return true;
  }
  else
    return false;
}


bool Transsaccion_Cashless::Requerimiento_Bonusing(int Evento, bool Enable_Bonusing,int Status_Bonus)
{

  if (Enable_Bonusing && Status_Bonus==BONUS_PENDING)
  {
    if (Evento == 0x7C)
      Cashless.Set_Status_AFT_Bonus(BONUS_OK);
    return true;
  }
  else
    return false;
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

bool Buffer_RX_AFT::Tester(int Test)
{

  for (int i = 0; i < 128; i++)
  {
    Buffer_RX_Transfer_AFT[i] = Test;
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


bool Buffer_RX_AFT::Set_Buffer_TITO_7B(char Buffer_Machine[])
{

  for(int i=0; i<128;i++)
  {
    Buffer_Rx_TITO_7B[i]=Buffer_Machine[i];
  }
  return true;
}

char* Buffer_RX_AFT::Get_Buffer_TITO_7B(void)
{
  return Buffer_Rx_TITO_7B;
}

bool Buffer_RX_AFT::Init_Buffer_TITO_7B(void)
{

  for (int i = 0; i < 128; i++)
  {
    Buffer_Rx_TITO_7B[i]=0xAA;
  }

  if(Buffer_Rx_TITO_7B[0]==0xAA &&  Buffer_Rx_TITO_7B[127]==0xAA)
    return true;
  else
    return false;
}


void Buffer_RX_AFT::Set_Buffer_TITO_Data(char Buffer_Transfer[])
{
  Buffer_Rx_TITO_Obj[0]=Buffer_Transfer[0];
  Buffer_Rx_TITO_Obj[1]=Buffer_Transfer[1];
  Buffer_Rx_TITO_Obj[2]=Buffer_Transfer[2];
  Buffer_Rx_TITO_Obj[3]=Buffer_Transfer[3];
}


void Buffer_RX_AFT::Init_Buffer_TITO_Data()
{
  Buffer_Rx_TITO_Obj[0]=0x00;
  Buffer_Rx_TITO_Obj[1]=0x00;
  Buffer_Rx_TITO_Obj[2]=0x00;
  Buffer_Rx_TITO_Obj[3]=0x00;
}



char* Buffer_RX_AFT::Get_Buffer_TITO_Data()
{
  return Buffer_Rx_TITO_Obj;
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


void Buffer_RX_AFT::Set_Buffer_Reg_AFT(char Buffer[])
{
  memcpy(Buffer_Reg_Rev,Buffer, sizeof(Buffer) / sizeof(Buffer[0]));
}

char* Buffer_RX_AFT::Get_Buffer_Reg_AFT(void)
{
  return Buffer_Reg_Rev;
}

void Buffer_RX_AFT::Init_Buffer_Reg()
{
  for(int i=0; i<128; i++)
  {
    Buffer_Reg_Rev[i]=0xAA;
  }
}



void Buffer_RX_AFT::Set_Status_Reg(int New_Status)
{
  Status_Reg =New_Status;
}

int  Buffer_RX_AFT::Get_Status_Reg()
{
  return Status_Reg;
}


bool Buffer_RX_AFT::Set_Buffer_Lock_Status_74(char Buffer[])
{

  memcpy(Buffer_Rx_Lock_Status, Buffer, sizeof(Buffer_Rx_Lock_Status) / sizeof(Buffer_Rx_Lock_Status[0]));

  if (Buffer[0] == Buffer_Rx_Lock_Status[0])
    return true;
  else
    return false;
}


char* Buffer_RX_AFT::Get_Buffer_Lock_Status_74(void)
{
  return Buffer_Rx_Lock_Status;
}

bool Buffer_RX_AFT::Init_Buffer_Lock_Status_74(void)
{
  for (int i = 0; i < 128; i++)
  {
    Buffer_Rx_Lock_Status[i] = 0xAA;
  }

  if (Buffer_Rx_Lock_Status[0] == 0xAA)
    return true;
  else
    return false;
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

bool esMacValida(String mac) {
  if (mac.length() != 17) return false;

  for (int i = 0; i < mac.length(); i++) {
    if (i % 3 == 2) {
      if (mac.charAt(i) != ':') return false;
    } else {
      char c = mac.charAt(i);
      if (!isHexadecimalDigit(c)) return false;
    }
  }

  return true;
}
