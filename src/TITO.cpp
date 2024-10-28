#include "TITO.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <string.h>
#include <sstream>  // Asegúrate de incluir esta cabecera
#include "Configuracion.h"
#include "RFID.h"
#include "Contadores.h"
#include "Clase_Variables_Globales.h"
#include "ESP32Time.h"
#include "time.h"
#include "Buffer_Cashless.h"
#include "Preferences.h"
extern Preferences NVS;

extern ESP32Time RTC; // Objeto contiene hora y fecha
extern Buffer_RX_AFT Buffer_Cashless;
extern Configuracion_ESP32 Configuracion;
extern Cashless_API Info_Cashless;
extern Contadores_SAS contadores; // Objeto contiene contadores maquina
extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales


extern TITO Tito;
DynamicJsonDocument Objeto_Ticket_Out(800);
DynamicJsonDocument Objeto_Ticket_In(800);
DynamicJsonDocument Objeto_Ticket_In_Transfer(800);


std::string IP_toString_Ticket(char IP_Char[])
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


bool TITO::Init_TITO(void)
{
    return true;
}


bool TITO::Printed_Ticket_Amount(void)
{                               

    return true;
}

bool TITO::Set_Custom_Format_Ticket(String Format)
{
    return true;
}

bool TITO::Ticket_Transfer_To_Machine(void)
{
    return true;
}

bool TITO::Generate_Key_Ticket_Out(void)
{

    Serial.println("Solicitud Token ticket");
    bool Code=false;
    int httpCode;

    char IP_Server[4];
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));

    char Current_IP[4];
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    std::string Ip=IP_toString_Ticket(IP_Server);
    String Ip_Server=String(Ip.c_str());
    String Puerto="9595";
    String fwurl = "http://"+Ip_Server+":"+Puerto+"/api/Cashless/Ticket_Out";

    WiFiClient client;
    HTTPClient https;

    
   
    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
    
    jsonDocument["IsSuccess"] = true;
    jsonDocument["Cliente_ID"] = contadores.Get_Client_ID_Transaccion_Int();
    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Ip"] = IP_toString_Ticket(Current_IP);
    jsonDocument["MAC"] = WiFi.macAddress();
    jsonDocument["Id_Maquina"] = 0;



    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
   // Serial.println(Json);
    
    https.setTimeout(5000); /* 5seg max */
    if (https.begin(client, fwurl))
    {
        https.addHeader("Content-Type", "application/json");
        https.addHeader("hash",Info_Cashless.Get_Hash_Valido());
        https.addHeader("gmsec","GMaster");
        https.addHeader("Authorization", "Bearer " + Info_Cashless.Get_Token_Valido()); // Agrega el token de autorización
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

                if(IsSuccess)
                {
                    String  Validation_Number = doc["Validation_Number"]; /*"1234567890123456"*/
                    if(Set_Parameter_Ticket(Validation_Number))
                        Code=true;
                    else
                        Code=false;
                }
                else
                    Code=false;
            }

            doc.clear();
        }
        else
        {
            Code=false;
        }
        https.end(); 
        return Code;
    }
    return false;
}

bool TITO::Reedeme_Ticket_In(void)
{

    bool Code=false;
    int httpCode;

    char IP_Server[4];
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));

    char Current_IP[4];
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    std::string Ip=IP_toString_Ticket(IP_Server);
    String Ip_Server=String(Ip.c_str());
    String Puerto="9595";
    String fwurl = "http://"+Ip_Server+":"+Puerto+"/api/Cashless/Ticket_In";

    WiFiClient client;
    HTTPClient https;

    
   
    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
    
    jsonDocument["IsSuccess"] = true;
    jsonDocument["Cliente_ID"] = contadores.Get_Client_ID_Transaccion_Int();
    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Ip"] = IP_toString_Ticket(Current_IP);
    jsonDocument["MAC"] = WiFi.macAddress();
    jsonDocument["Id_Maquina"] = 0;

    jsonDocument["Ticket_Amount"] = Objeto_Ticket_In["Saldo"];
    jsonDocument["Validation_Number"] =Objeto_Ticket_In["Numero_Validacion"];
    jsonDocument["Validation_System_ID"] = Objeto_Ticket_In["Validation_System_ID"];
    jsonDocument["Ticket_Status"] = Objeto_Ticket_In["Ticket_Status"];


    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
   // Serial.println(Json);
    
    https.setTimeout(5000); /* 5seg max */
    if (https.begin(client, fwurl))
    {
        https.addHeader("Content-Type", "application/json");
        https.addHeader("hash",Info_Cashless.Get_Hash_Valido());
        https.addHeader("gmsec","GMaster");
        https.addHeader("Authorization", "Bearer " + Info_Cashless.Get_Token_Valido()); // Agrega el token de autorización
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

                if(IsSuccess)
                {
                    Code=true;
                }
                else
                    Code=false;
            }

            doc.clear();
        }
        else
        {
            Code=false;
        }
        https.end(); 
        return Code;
    }
    return false;
}






bool TITO::Set_Parameter_Ticket(String  Validation_Number_)
{
    char Test[8];
    // Convertir los dígitos a BCD
    for (int i = 0; i < 8; i++)
    {
        // Convertir cada par de dígitos
        Test[i] = ((Validation_Number_.charAt(2 * i) - '0') << 4) | (Validation_Number_.charAt(2 * i + 1) - '0');
    }

    // Convertir de BCD de vuelta a String
    String Validation_Number_Recovery = "";
    for (int i = 0; i < 8; i++)
    {
        // Extraer los dos dígitos de cada byte BCD
        Validation_Number_Recovery += String((Test[i] >> 4) & 0x0F); // Dígito alto
        Validation_Number_Recovery += String(Test[i] & 0x0F);        // Dígito bajo
    }

    

    if(Validation_Number_==Validation_Number_Recovery)
    {
        for(int i=0; i<8; i++)
        {
            Validation_Number[i]=Test[i];
        }
        return true;
    }
    return false;
}

char* TITO::Get_Validacion_Number(void)
{
    return Validation_Number;
}


bool TITO::Remove_Validacion_Number(void)
{
    for(int i=0; i<7;i++)
    {
        Validation_Number[i]=0xAA;
    }

    if(Validation_Number[0]==0xAA && Validation_Number[7]==0xAA)
        return true;

    else
        return false;
}



// bool TITO::Remove_System_ID(void)
// {
//     Validations_System_ID=0xAA;

//     if(Validations_System_ID==0xAA)
//         return true;
//     else
//         return false;
// }

int TITO::Get_Validacion_System_ID(void)
{
    return Validations_System_ID;
}



bool TITO::Update_Ticket(int Code, int Type_Ticket)
{
    if (Code == 0x00) /* 0x80  not in cashout  0x81 improper validation rejected */
        Objeto_Ticket_Out["IsSuccess"] = true;
    else
        Objeto_Ticket_Out["IsSuccess"] = false;

    Objeto_Ticket_Out["Code"]=Code;


    switch (Type_Ticket)
    {
    case 0x00:
        Objeto_Ticket_Out["Tipo_Ticket"]=Cashable_Ticket;
        break;
    case 0x01:
        Objeto_Ticket_Out["Tipo_Ticket"]=Restricted_Promotional_Ticket;
        break;

    case 0x80:
        Objeto_Ticket_Out["Tipo_Ticket"]=Not_Waiting_for_System_validation;
        break;
    
    default:
        Objeto_Ticket_Out["Tipo_Ticket"]=Ticket_Type_Not_Identified;
        break;
    }

    return true;

}

uint32_t TITO::Convert_4BCD_Uint32(char Buffer[],int Inicial_Index)
{
    char str[9];
    uint32_t Number = 0;

    for (int i = 0; i < 4; i++)
    {
        str[i * 2] = ( Buffer[Inicial_Index + i] >> 4) + '0';
        str[i * 2 + 1] = ( Buffer[Inicial_Index + i] & 0x0F) + '0';
    }
    str[8] = '\0'; // Asegurarse de que la cadena termine con un carácter nulo

    sscanf(str, "%u", &Number);
    return Number;
}


int TITO::Convert_2BNR_Int(char HighByte,char LowByte)
{
    int resultado = HighByte + LowByte;
    return resultado;
}


bool TITO::Ticket_Information_Capture(char Buffer_70[],ESP32Time)
{
    String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

    int Ticket_Status=Buffer_70[3];

    char Amount[6];
    /* Amount */
    Amount[0] = Buffer_70[4];
    Amount[1] = Buffer_70[5];
    Amount[2] = Buffer_70[6];
    Amount[3] = Buffer_70[7];
    Amount[4] = Buffer_70[8];

    Convert_5BCD_Uint32(Amount,0);

    int Parsing_Code=Buffer_70[9];

    int Id_System = ((Buffer_70[10] >> 4) * 10) + (Buffer_70[10] & 0x0F);
    char convertedBackToBCD = ((Id_System / 10) << 4) | (Id_System % 10);


    char Validacion_Number_Char[8];

    Validacion_Number_Char[0] = Buffer_70[11];
    Validacion_Number_Char[1] = Buffer_70[12];
    Validacion_Number_Char[2] = Buffer_70[13];
    Validacion_Number_Char[3] = Buffer_70[14];
    Validacion_Number_Char[4] = Buffer_70[15];
    Validacion_Number_Char[5] = Buffer_70[16];
    Validacion_Number_Char[6] = Buffer_70[17];
    Validacion_Number_Char[7] = Buffer_70[18];

   
    Objeto_Ticket_In["Ticket_Status"]=  Ticket_Status;
    Objeto_Ticket_In["Saldo"]= Convert_5BCD_Uint32(Amount,0);
    Objeto_Ticket_In["Numero_Validacion"]=  Validacion_Number(Validacion_Number_Char);
    Objeto_Ticket_In["Validation_System_ID"]=Id_System;

    if(Parsing_Code==0xFF||Id_System!=convertedBackToBCD)
        return false;
    return true;
}


bool TITO::Status_Ticket_In_Data(char Buffer_4D[], ESP32Time RTC)
{
    int Code=Buffer_4D[3];


    if(Code==0x00|| Code==0x01||Code==0x02)
        Objeto_Ticket_In_Transfer["IsSuccess"]=true;
    else
        Objeto_Ticket_In_Transfer["IsSuccess"]=false; 

    char Amount[8];

    Amount[0]=Buffer_4D[4];
    Amount[1]=Buffer_4D[5];
    Amount[2]=Buffer_4D[6];
    Amount[3]=Buffer_4D[7];
    Amount[4]=Buffer_4D[8];
    Amount[5]=Buffer_4D[9];
    Amount[6]=Buffer_4D[10];
    Amount[7]=Buffer_4D[11];

    char Parsing_Code=Buffer_4D[12];

    int Id_System = ((Buffer_4D[13] >> 4) * 10) + (Buffer_4D[13] & 0x0F);
    char convertedBackToBCD = ((Id_System / 10) << 4) | (Id_System % 10);
    
    char Validations[8];

    Validations[0]=Buffer_4D[14];
    Validations[1]=Buffer_4D[15];
    Validations[2]=Buffer_4D[16];
    Validations[3]=Buffer_4D[17];
    Validations[4]=Buffer_4D[18];
    Validations[5]=Buffer_4D[19];
    Validations[6]=Buffer_4D[20];
    Validations[7]=Buffer_4D[21];


    Objeto_Ticket_In_Transfer["Codigo"]=Code;
    Objeto_Ticket_In_Transfer["Saldo"]=Convert_5BCD_Uint32(Amount,0);
    Objeto_Ticket_In_Transfer["Parsing_Code"]=Parsing_Code;
    Objeto_Ticket_In_Transfer["Numero_Validacion"]=  Validacion_Number(Validations);
    Objeto_Ticket_In_Transfer["Validation_System_ID"]=Id_System;
    Objeto_Ticket_In_Transfer["Cliente_ID"]=contadores.Get_Client_ID_Transaccion_Int();    
    


    String Json;
    serializeJson( Objeto_Ticket_In_Transfer, Json); /* Serializa Data */
    Objeto_Ticket_In_Transfer.clear();

    if(Code==0x00|| Code==0x01||Code==0x02)
        return true;
    else
        return false;
}

bool TITO::Status_Ticket_Out(char Buffer_4D[], ESP32Time RTC)
{
    
    String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

    char Validation_Date[4];
    char Time[3];
    char Validation_Number_[8];
    char Amount[5];
    int Ticket_Number;
    int Validation_System_ID;
    char Expiration[4];

    /* Validation Type */
    int Validation_Type = Buffer_4D[2];
    int Index_Number = Buffer_4D[3];

    /*DATE*/
    Validation_Date[0] = Buffer_4D[4];
    Validation_Date[1] = Buffer_4D[5];
    Validation_Date[2] = Buffer_4D[6];
    Validation_Date[3] = Buffer_4D[7];


    /*TIME*/
    Time[0] = Buffer_4D[8];
    Time[1] = Buffer_4D[9];
    Time[2] = Buffer_4D[10];

    uint8_t bcdMonth = Validation_Date[0];
    uint8_t bcdDay = Validation_Date[1];
    uint8_t bcdYearHigh = Validation_Date[2];
    uint8_t bcdYearLow = Validation_Date[3];

    uint8_t bcdHours = Time[0];
    uint8_t bcdMinutes = Time[1];
    uint8_t bcdSeconds = Time[2];

    // Convertir de BCD a entero
    int month = ((bcdMonth >> 4) & 0x0F) * 10 + (bcdMonth & 0x0F);
    int day = ((bcdDay >> 4) & 0x0F) * 10 + (bcdDay & 0x0F);
    int yearHigh = ((bcdYearHigh >> 4) & 0x0F) * 10 + (bcdYearHigh & 0x0F); // Parte alta del año (por ejemplo, 20)
    int yearLow = ((bcdYearLow >> 4) & 0x0F) * 10 + (bcdYearLow & 0x0F);    // Parte baja del año (por ejemplo, 24)

    // Formar el año completo
    int year = yearHigh * 100 + yearLow; // Combina las dos partes del año

    // Combina la parte alta y baja
    int hour=((bcdHours >> 4) & 0x0F) * 10 + (bcdHours & 0x0F);
    int minutes=((bcdMinutes>> 4) & 0x0F) * 10 + (bcdMinutes & 0x0F);
    int Seconds=((bcdSeconds >> 4) & 0x0F) * 10 + (bcdSeconds & 0x0F);
    
    String DataTime_Validation = String(year) + "-" + String(month) + "-" + String(day)+ " " + String(hour) + ":" + String(minutes) + ":" + String(Seconds);

    /*VALIDATION ID*/
    Validation_Number_[0] = Buffer_4D[11];
    Validation_Number_[1] = Buffer_4D[12];
    Validation_Number_[2] = Buffer_4D[13];
    Validation_Number_[3] = Buffer_4D[14];
    Validation_Number_[4] = Buffer_4D[15];
    Validation_Number_[5] = Buffer_4D[16];
    Validation_Number_[6] = Buffer_4D[17];
    Validation_Number_[7] = Buffer_4D[18];

    /*AMOUNT*/
    Amount[0] = Buffer_4D[19];
    Amount[1] = Buffer_4D[20];
    Amount[2] = Buffer_4D[21];
    Amount[3] = Buffer_4D[22];
    Amount[4] = Buffer_4D[23];

    /*TICKET NUMBER*/
    Ticket_Number = Convert_2BNR_Int(Buffer_4D[24],Buffer_4D[25]);
    Serial.println(Buffer_4D[24]);
    Serial.println(Buffer_4D[25]);
    /*VALIDATION SYSTEM ID*/
    Validation_System_ID = Buffer_4D[26];

    /*EXPIRATION*/
    Expiration[0] = Buffer_4D[27];
    Expiration[1] = Buffer_4D[28];
    Expiration[2] = Buffer_4D[29];
    Expiration[3] = Buffer_4D[30];
    
    uint32_t Expiration_Ticket = Convert_4BCD_Uint32(Expiration,0);
    Buffer_Cashless.Init_Buffer_TITO_4D();

    Objeto_Ticket_Out["Numero_Indice"]=  Index_Number;
    Objeto_Ticket_Out["Tipo_Validacion"]= Validation_Type;
    Objeto_Ticket_Out["Fecha_Hora_Validacion"]= DataTime_Validation;

    Objeto_Ticket_Out["Numero_Validacion"]=  Validacion_Number(Validation_Number_);
    Objeto_Ticket_Out["Saldo"]= Convert_5BCD_Uint32(Amount,0);
    Objeto_Ticket_Out["Numero_Ticket"]= Ticket_Number;
    Objeto_Ticket_Out["Validation_System_ID"]=Get_Validacion_System_ID();

    if(Expiration_Ticket==9999||Expiration_Ticket==0)
        Objeto_Ticket_Out["Expiracion_Ticket"]=nullptr;
    else
        Objeto_Ticket_Out["Expiracion_Ticket"]=Expiration_Ticket;

    Objeto_Ticket_Out["Cliente_ID"]= contadores.Get_Client_ID_Transaccion_Int();
    Objeto_Ticket_Out["Fecha_Hora"] = DataTime;
    

    String Json;
    serializeJson(Objeto_Ticket_Out, Json); /* Serializa Data */
    
    Remove_Validacion_Number(); /*0xAA*/
   // Remove_System_ID(); /*0xAA*/

    Serial.println(Json);

    return true;
}




uint32_t TITO::Convert_5BCD_Uint32(char Buffer[],int Inicial_Index)
{
    char str[11];
    uint32_t Number = 0;

    for (int i = 0; i < 5; i++)
    {
        str[i * 2] = ( Buffer[Inicial_Index + i] >> 4) + '0';
        str[i * 2 + 1] = ( Buffer[Inicial_Index + i] & 0x0F) + '0';
    }
    str[10] = '\0'; // Asegurarse de que la cadena termine con un carácter nulo

    sscanf(str, "%u", &Number);
    return Number;
}

void TITO::Solicitud_Token_Ticket_Http(bool Status)
{
    Solicitud_Token_Ticket=Status;
}



void TITO::Silicitud_Reedemed_Ticket_Http(bool Status)
{
    Solicitud_Redeemed_Ticket=Status;
}

bool TITO::Get_Status_Reedened_Ticket_Http()
{
    return Solicitud_Redeemed_Ticket;
}

bool TITO:: Get_Status_Token_Ticket_Http(void)
{
    return Solicitud_Token_Ticket;
}

void TITO::Request_Handle_Tito(void)
{

    if(Get_Status_Token_Ticket_Http())
    {
        if(!Get_Status())
        {
            if(Generate_Key_Ticket_Out())
            {   Variables_globales.Set_Variable_Global(Attend_Pending_Tito_Request,true);
                Set_Status(true);
            }
        }
        Solicitud_Token_Ticket_Http(false);
    }

    if(Get_Status_Reedened_Ticket_Http())
    {

        if (!Get_Status_Ticket_In())
        {
            if (Reedeme_Ticket_In())
            {
                Variables_globales.Set_Variable_Global(Attend_Pending_Tito_Request_In, true);
                Set_Status_Ticket_In(true);
            }
        }
        Silicitud_Reedemed_Ticket_Http(false);
    }
}

bool TITO::Handle_Event_Tito(int Evento,bool Enable)
{

    if (Enable)
    {
        switch (Evento)
        {
        case 0x57:/* Requerimiento de sistema */
            Solicitud_Token_Ticket_Http(true);
            return true;
        break;

        // case 0x67: /* Ticket insertado */
        //     Silicitud_Reedemed_Ticket_Http(true);
        //     return true;
        // break;

        default:
            return false;
            break;
        }
    }
    return false;
}




bool TITO::Get_Status(void)
{
    return Attend_Tito_OK;
}

void TITO::Set_Status(bool Set)
{
    Attend_Tito_OK=Set;
}


bool TITO::Get_Status_Ticket_In(void)
{
    return Attend_Tito_In_OK;
}

void TITO::Set_Status_Ticket_In(bool Set)
{
    Attend_Tito_In_OK=Set;
}





void TITO::Set_Status_3D(bool Set)
{
    Ticket_OK=Set;
}



bool TITO::Get_Status_3D()
{
    return Ticket_OK;
}


bool TITO::Get_Status_Process_Ticket()
{
    return Ticket_In_Process;
}

void TITO::Status_Process_Ticket(bool Set)
{
    Ticket_In_Process=Set;
}

String TITO::Validacion_Number(char Buffer[])
{
    String Validation_Number_Recovery = "";
    for (int i = 0; i < 8; i++)
    {
        // Extraer los dos dígitos de cada byte BCD
        Validation_Number_Recovery += String((Buffer[i] >> 4) & 0x0F); // Dígito alto
        Validation_Number_Recovery += String(Buffer[i] & 0x0F);        // Dígito bajo
    }

    return Validation_Number_Recovery;
}
uint32_t TITO::Convert_8BCD_Uint32(char Buffer[], int Inicial_Index)
{
    char str[17];  // Para almacenar 16 dígitos + '\0'
    uint32_t Number = 0;

    for (int i = 0; i < 8; i++)
    {
        str[i * 2] = (Buffer[Inicial_Index + i] >> 4) + '0';
        str[i * 2 + 1] = (Buffer[Inicial_Index + i] & 0x0F) + '0';
    }
    str[16] = '\0';  // Asegurarse de que la cadena termine con un carácter nulo

    sscanf(str, "%u", &Number);
    return Number;
}


bool TITO::Increase_Transaction_Number_ID_Tito(void)
{
  Validations_System_ID++;

  if(Validations_System_ID>=99)
  {
    Validations_System_ID=0;
  }
  
  /* ---------> Guarda en memoria ID <-------- */
  NVS.begin("Config_ESP32", false);
  NVS.putInt("ID_Tito",Validations_System_ID);
  NVS.end();
  /*-------------------------------------------*/
  return true;
}

   



bool TITO::Set_Inicial_Trans_ID_Tito(int New_Validation_System_ID)
{
    Validations_System_ID=New_Validation_System_ID;
    if(Validations_System_ID==New_Validation_System_ID)
        return true;
    else
        return false;
}