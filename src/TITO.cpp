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
#include "SPIFFS.h"
#include <esp_task_wdt.h>
#include "Transacciones.h"




#define TICKET_OUT "0x01"

TaskHandle_t taskHandleTicket = NULL;

bool Result_Key=false;

bool Result_RequestKey=false;

std::vector<String> Ticket_Pendientes;
const char* Ticketfile = "/Ticket.txt";

extern Preferences NVS;

extern ESP32Time RTC; // Objeto contiene hora y fecha
extern Buffer_RX_AFT Buffer_Cashless;
extern Configuracion_ESP32 Configuracion;
extern Cashless_API Info_Cashless;
extern Contadores_SAS contadores; // Objeto contiene contadores maquina
extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales

extern bool Actualiza_Tito_Entradas(void);
extern bool Actualiza_Tito_Salidas(void);


extern bool Flag_Entradas_Tito_OK;
extern bool Flag_Salidas_Tito_OK;
extern TITO Tito;
DynamicJsonDocument Objeto_Ticket_Out(800);
DynamicJsonDocument Objeto_Ticket_In(800);
DynamicJsonDocument Objeto_Ticket_In_Transfer(800);
DynamicJsonDocument Objeto_Ticket_In_Response(800);


extern unsigned char decimalToBCD(unsigned char digit);
extern bool CalcularCRC_Transfer(char Buffer[],int Size_Without_CRC);
extern void sendDataa(const char *datos, unsigned int tamano);
extern void Transmite_Poll_Long(unsigned char Com_SAS);

extern char dat[1];
extern char dat3[1];
extern char dat4[1];

extern TransaccionCashless AFT;

void TITO::Set_Flag_New_Ticket_In(bool Flag_Status)
{
    Flag_New_Transfer_Ticket_In=Flag_Status;
}

void TITO::Set_Flag_New_Ticket_Out(bool Flag_Status)
{
    Flag_New_Transfer_Ticket_Out=Flag_Status;
}


bool TITO::Get_Flag_New_Ticket_In(void)
{
    return Flag_New_Transfer_Ticket_In;
}

bool TITO::Get_Flag_New_Ticket_Out(void)
{
    return Flag_New_Transfer_Ticket_Out;
}


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

   // Serial.println("Solicitud Token ticket");
    bool Code = false;
    int httpCode;

    char IP_Server[4];
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));

    char Current_IP[4];
    String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    std::string Ip = IP_toString_Ticket(IP_Server);
    String Ip_Server = String(Ip.c_str());
    String Puerto = "9595";
    String fwurl = "http://" + Ip_Server + ":" + Puerto + "/api/Tito/Ticket_Out";

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
        https.addHeader("hash", Info_Cashless.Get_Hash_Valido());
        https.addHeader("gmsec", "GMaster");
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

                if (IsSuccess)
                {
                    String Validation_Number = doc["Validation_Number"]; /*"1234567890123456"*/
                    if (Set_Parameter_Ticket(Validation_Number))
                        Code = true;
                    else
                        Code = false;
                }
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

        if (Code)
            Serial.println("Token Ticket Generado correctamente!");
        else
            Serial.println("Token Ticket no generado!");

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
   
    Serial.println(" ------------------------>SOLICITUD (HTTP POST) Numero de validacion <-----------------------------");
    Serial.println(Json);
    Serial.println(" --------------------------------------------------------------------------------------------------");
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
            

            Serial.println("-------------------------------------->Respuesta Gmaster<------------------------------------------");
            String Response = https.getString();
            Serial.println(Response);
            Serial.println(" --------------------------------------------------------------------------------------------------");
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


char* TITO::Get_Validatioins_Number_Out(void)
{
    return Validation_Number_Out;
}
bool TITO::Set_Validations_Number_Out(String Validations_Number_Out)
{
    char Test[8];
    // Convertir los dígitos a BCD
    for (int i = 0; i < 8; i++)
    {
        // Convertir cada par de dígitos
        Test[i] = ((Validations_Number_Out.charAt(2 * i) - '0') << 4) | (Validations_Number_Out.charAt(2 * i + 1) - '0');
    }

    // Convertir de BCD - String
    String Validation_Number_Recovery = "";
    for (int i = 0; i < 8; i++)
    {
        // Extraer los dos dígitos de cada byte BCD
        Validation_Number_Recovery += String((Test[i] >> 4) & 0x0F); // Dígito alto
        Validation_Number_Recovery += String(Test[i] & 0x0F);        // Dígito bajo
    }

    if (Validations_Number_Out == Validation_Number_Recovery)
    {
        for (int i = 0; i < 8; i++)
        {
            Validation_Number_Out[i] = Test[i];
        }
        return true;
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

uint8_t TITO::Get_Validacion_System_ID(void)
{
    return Validations_System_ID;
}


void TITO::Set_Confirma_Ticket(bool Confirma)
{
    Current_4D_3E=Confirma;
}

bool TITO::Get_Confirma_Ticket(void)
{
    return Current_4D_3E;
}

bool TITO::Update_Ticket(int Code, int Type_Ticket)
{
    if (Code == 0x00) /* 0x80  not in cashout  0x81 improper validation rejected */
        Objeto_Ticket_Out["IsSuccess"] = false;
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


    if(Code==0x00)
        return true;
    else
        return false;
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


uint8_t BCD_to_Decimal(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

bool TITO::Ticket_Information_Capture(char Buffer_70[], ESP32Time)
{
    String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

    int Ticket_Status = Buffer_70[3];

    char Amount_Data[5];
    /* Amount */
    Amount_Data[0] = Buffer_Cashless.Get_Buffer_TITO_70()[4];
    Amount_Data[1] = Buffer_Cashless.Get_Buffer_TITO_70()[5];
    Amount_Data[2] = Buffer_Cashless.Get_Buffer_TITO_70()[6];
    Amount_Data[3] = Buffer_Cashless.Get_Buffer_TITO_70()[7];
    Amount_Data[4] = Buffer_Cashless.Get_Buffer_TITO_70()[8];

    int Parsing_Code = Buffer_70[9];

    uint8_t Id_System = ((Buffer_70[10] >> 4) * 10) + (Buffer_70[10] & 0x0F);
    uint8_t convertedBackToBCD = ((Id_System / 10) << 4) | (Id_System % 10);

    char Id_System_String[10];
    sprintf(Id_System_String, "0x%02d", Id_System);


    char Validacion_Number_Char[8];

    Validacion_Number_Char[0] = Buffer_70[11];
    Validacion_Number_Char[1] = Buffer_70[12];
    Validacion_Number_Char[2] = Buffer_70[13];
    Validacion_Number_Char[3] = Buffer_70[14];
    Validacion_Number_Char[4] = Buffer_70[15];
    Validacion_Number_Char[5] = Buffer_70[16];
    Validacion_Number_Char[6] = Buffer_70[17];
    Validacion_Number_Char[7] = Buffer_70[18];

    // for(int i=11; i< 19; i++)
    // {
    //     Serial.println(Buffer_70[i],HEX);
    // }

    Buffer_Cashless.Init_Buffer_TITO_70(); /* Borra respuesta 0xAA */

    Objeto_Ticket_In["Ticket_Status"] = Ticket_Status;
    Objeto_Ticket_In["Saldo"] = Convert_5BCD_Uint32(Amount_Data, 0);
    Objeto_Ticket_In["Numero_Validacion"] = Validacion_Number(Validacion_Number_Char);
    Objeto_Ticket_In["Validation_System_ID"] = Id_System_String;

    if (Parsing_Code == 0xFF)
    {
        Serial.println(Parsing_Code);
        Serial.println(Id_System_String);
        Serial.println(convertedBackToBCD);
        Serial.println(Convert_5BCD_Uint32(Amount_Data, 0));
        return false;
    }
       
    String Json;
    serializeJson(Objeto_Ticket_In, Json); /* Serializa Data */
    Serial.println("--------------->Consulta informacion Ticket (HTTP POST) <----------------------");
    Serial.println(Json);
    Serial.println("-------------------------------------------------------------------------------");
    if (Consult_Ticket(Json))
    {
       
        return true;
    }
        
    else
        return false;
}

/* Metodo reporta estado de transaccion Final Tito */
bool TITO::Status_Ticket_In_Data(char Buffer_4D[], ESP32Time RTC)
{

    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());



    int Code=Buffer_4D[3];


    if(Code==0x00|| Code==0x01||Code==0x02)
        Objeto_Ticket_In_Transfer["IsSuccess"]=true;
    else if(Code==0x40)
        Objeto_Ticket_In_Transfer["IsSuccess"]=false;
    else
        Objeto_Ticket_In_Transfer["IsSuccess"]=false; 

    char Amount[5];

    Amount[0]=Buffer_4D[4];
    Amount[1]=Buffer_4D[5];
    Amount[2]=Buffer_4D[6];
    Amount[3]=Buffer_4D[7];
    Amount[4]=Buffer_4D[8];
    // Amount[5]=Buffer_4D[9];
    // Amount[6]=Buffer_4D[10];
    // Amount[7]=Buffer_4D[11];



    char Parsing_Code=Buffer_4D[9];

    int Id_System = ((Buffer_4D[10] >> 4) * 10) + (Buffer_4D[10] & 0x0F);
    char convertedBackToBCD = ((Id_System / 10) << 4) | (Id_System % 10);

    char Id_System_String[10];
    sprintf(Id_System_String, "0x%02d", Id_System);
    
    char Validations[8];

    Validations[0]=Buffer_4D[11];
    Validations[1]=Buffer_4D[12];
    Validations[2]=Buffer_4D[13];
    Validations[3]=Buffer_4D[14];
    Validations[4]=Buffer_4D[15];
    Validations[5]=Buffer_4D[16];
    Validations[6]=Buffer_4D[17];
    Validations[7]=Buffer_4D[18];

    
    char Id_Cli[8];

    Id_Cli[0]=contadores.Get_Client_ID()[0];
    Id_Cli[1]=contadores.Get_Client_ID()[1];
    Id_Cli[2]=contadores.Get_Client_ID()[2];
    Id_Cli[3]=contadores.Get_Client_ID()[3];
    Id_Cli[4]=contadores.Get_Client_ID()[4];
    Id_Cli[5]=contadores.Get_Client_ID()[5];
    Id_Cli[6]=contadores.Get_Client_ID()[6];
    Id_Cli[7]=contadores.Get_Client_ID()[7];
    
    Objeto_Ticket_In_Transfer["Codigo"]=Code;
    Objeto_Ticket_In_Transfer["Saldo"]=Convert_5BCD_Uint32(Amount,0);
    Objeto_Ticket_In_Transfer["Parsing_Code"]=Parsing_Code;
    Objeto_Ticket_In_Transfer["Numero_Validacion"]=  Validacion_Number(Validations);
    Objeto_Ticket_In_Transfer["Validation_System_ID"]=Id_System_String;
    Objeto_Ticket_In_Transfer["Cliente_ID"]=atoi(Id_Cli);   
    Objeto_Ticket_In_Transfer["Trans_Tipo"]="0x00";
    Objeto_Ticket_In_Transfer["Fecha_Hora"]=DataTime;

    String Json;
    serializeJson( Objeto_Ticket_In_Transfer, Json); /* Serializa Data */
   // Objeto_Ticket_In_Transfer.clear();
    Serial.println(" ------------------------>SOLICITUD (HTTP POST) ACK<-----------------------------");
    Serial.println(Json);
    Serial.println(" -------------------------------------------------------------------------------");
    if(Code==0x40)
        Flag_Parcial_New_Transfer_Ticket_In=true;
    else
        Set_Flag_New_Ticket_In(true);


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
    int Validation_System_ID_New;
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
    // Serial.println(Buffer_4D[24]);
    // Serial.println(Buffer_4D[25]);

    uint8_t Id_System = ((Buffer_4D[26] >> 4) * 10) + (Buffer_4D[26] & 0x0F);
    /*VALIDATION SYSTEM ID*/
   // Validation_System_ID_New = Buffer_4D[26];
    char Id_System_String[10];
    sprintf(Id_System_String, "0x%02d", Id_System);
    

    /*EXPIRATION*/
    Expiration[0] = Buffer_4D[27];
    Expiration[1] = Buffer_4D[28];
    Expiration[2] = Buffer_4D[29];
    Expiration[3] = Buffer_4D[30];
    
    uint32_t Expiration_Ticket = Convert_4BCD_Uint32(Expiration,0);
    Buffer_Cashless.Init_Buffer_TITO_4D();



    char Id_Cli[8];

    Id_Cli[0]=contadores.Get_Client_ID()[0];
    Id_Cli[1]=contadores.Get_Client_ID()[1];
    Id_Cli[2]=contadores.Get_Client_ID()[2];
    Id_Cli[3]=contadores.Get_Client_ID()[3];
    Id_Cli[4]=contadores.Get_Client_ID()[4];
    Id_Cli[5]=contadores.Get_Client_ID()[5];
    Id_Cli[6]=contadores.Get_Client_ID()[6];
    Id_Cli[7]=contadores.Get_Client_ID()[7];


    if(Objeto_Ticket_Out["Code"]==0x00)
    {
        Objeto_Ticket_Out["Codigo"]=0x00;
        Objeto_Ticket_Out["IsSuccess"] = true;
    }else{
        Objeto_Ticket_Out["Codigo"]=0xFF;
        Objeto_Ticket_Out["IsSuccess"] = false;
    }
        

    Objeto_Ticket_Out["Numero_Indice"]=  Index_Number;
    Objeto_Ticket_Out["Tipo_Validacion"]= Validation_Type;
    Objeto_Ticket_Out["Fecha_Hora_Validacion"]= DataTime_Validation;
    Objeto_Ticket_Out["Trans_Tipo"]="0x01";
    Objeto_Ticket_Out["Numero_Validacion"]=  Validacion_Number(Validation_Number_);
    Objeto_Ticket_Out["Saldo"]= Convert_5BCD_Uint32(Amount,0);
    Objeto_Ticket_Out["Numero_Ticket"]= Ticket_Number;
    Objeto_Ticket_Out["Validation_System_ID"]=Id_System_String;

   

    if(Expiration_Ticket==9999||Expiration_Ticket==0)
        Objeto_Ticket_Out["Expiracion_Ticket"]=nullptr;
    else
        Objeto_Ticket_Out["Expiracion_Ticket"]=Expiration_Ticket;

    Objeto_Ticket_Out["Cliente_ID"]= atoi(Id_Cli);
    Objeto_Ticket_Out["Fecha_Hora"] = DataTime;
    

    String Json;
    serializeJson(Objeto_Ticket_Out, Json); /* Serializa Data */
    
    Remove_Validacion_Number(); /*0xAA*/
   // Remove_System_ID(); /*0xAA*/
    Serial.println(" ------------------------>SOLICITUD (HTTP POST) ACK TICKET OUT<-----------------------------");
    Serial.println(Json);
    Serial.println(" -------------------------------------------------------------------------------------------");
    Set_Flag_New_Ticket_Out(true);
    return true;
}


/* Atiende Requerimiento TITO 57  Ticket Out */
bool TITO::Requerimiento_TITO_Ticket_Out(int Evento, bool Habilita_Tito, bool Solo_Cashless, bool Solo_Tito)
{

    if (Evento == 0x3D || Evento == 0x3E && !Tito.Transfer_Tito_Is_ready())
    {
        Serial.println("Ticket Impreso OK");
        Set_Confirma_Ticket(true);
    }

    switch (Evento)
    {
    case 0x57:
        if (Tito.Transfer_Tito_Is_ready())
        {
            if (!Solo_Cashless && Habilita_Tito && Solo_Tito)
            {
                Tito.STATUS_TITO_TRANSFER(TITO::TRANS_TICKET_RECIBIDA);

                if (!Variables_globales.Get_Variable_Global(Attend_Pending_Tito_Request))
                    Variables_globales.Set_Variable_Global(Attend_Pending_Tito_Request, true);
                return Variables_globales.Get_Variable_Global(Attend_Pending_Tito_Request);
            }
        }
        break;

    default:
        break;
    }

    return false;
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

// void TITO::Request_Handle_Tito(void)
// {

//     if(Get_Status_Token_Ticket_Http())
//     {
//         if(!Get_Status())
//         {
//             if(Generate_Key_Ticket_Out())
//             {   Variables_globales.Set_Variable_Global(Attend_Pending_Tito_Request,true);
//                 Set_Status(true);
//             }
//         }
//         Solicitud_Token_Ticket_Http(false);
//     }

//     if(Get_Status_Reedened_Ticket_Http())
//     {

//         if (!Get_Status_Ticket_In())
//         {
//             if (Reedeme_Ticket_In())
//             {
//                 Variables_globales.Set_Variable_Global(Attend_Pending_Tito_Request_In, true);
//                 Set_Status_Ticket_In(true);
//             }
//         }
//         Silicitud_Reedemed_Ticket_Http(false);
//     }
// }

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

  if(Validations_System_ID>99)
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


bool TITO::Set_Amount_Ticket_Transfer(uint32_t Amount_Transfer)
{
    Amount_Transfer=Amount_Transfer;

    char strOut[11];
    char str[11];

    uint32_t Test_Credit=0;

    for( int i=0; i<5; i++)
    {
        Amount_Ticket_Transfer[i]=0;
    }
   
    
    sprintf(str, "%010d", Amount_Transfer);

    // Convertir cada par de caracteres a un byte BCD Cashables
    for (int i = 0; i < 5; i++)
    {
        Amount_Ticket_Transfer[i] = ((str[i * 2] - '0') << 4) | (str[i * 2 + 1] - '0');
    }



    /* Proceso inverso para saber si la conversion esta OK */
    for (int i = 0; i < 5; i++)
    {
      strOut[i * 2] = (Amount_Ticket_Transfer[i] >> 4) + '0';
      strOut[i * 2 + 1] = (Amount_Ticket_Transfer[i] & 0x0F) + '0';
    }
    strOut[10] = '\0'; // Asegurarse de que la cadena termine con un carácter nulo

    
    sscanf(strOut, "%u", &Test_Credit);

    if(Test_Credit==Amount_Transfer)
        return true;
    else
        return false;
}



char* TITO::Get_Amount_Ticket_Transfer(void)
{
    return Amount_Ticket_Transfer;
}

bool TITO::Consult_Ticket(String Ticket_Informations)
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
    String fwurl = "http://"+Ip_Server+":"+Puerto+"/api/Tito/Ticket_In";

    WiFiClient client;
    HTTPClient https;

    
    https.setTimeout(5000); /* 5seg max */
    if (https.begin(client, fwurl))
    {
        https.addHeader("Content-Type", "application/json");
        https.addHeader("hash",Info_Cashless.Get_Hash_Valido());
        https.addHeader("gmsec","GMaster");
        https.addHeader("Authorization", "Bearer " + Info_Cashless.Get_Token_Valido()); // Agrega el token de autorización
        httpCode = https.POST(Ticket_Informations);

        // Serial.println(httpCode);
        if (httpCode == HTTP_CODE_OK)
        {

            String Response = https.getString();
            Serial.println("------------------> Respuesta Gmaster <------------------------------------");
            Serial.println(Response);
            Serial.println("---------------------------------------------------------------------------");
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

               // Serial.println(Response);
               // Objeto_Ticket_In_Response.clear();

                bool IsSuccess = doc["IsSuccess"];
                Objeto_Ticket_In_Response["IsSuccess"]=IsSuccess;
                
                const char *Transfer_Codestr = doc["Transfer_Code"];

                int Transfer_Code = strtol(Transfer_Codestr, nullptr, 16);
                // Serial.print("Transfer_Code (entero hexadecimal): ");
                // Serial.println(Transfer_Code, HEX); // Mostrar como hexadecimal

                
                Objeto_Ticket_In_Response["Transfer_Code"]=Transfer_Code;

                if(IsSuccess)
                {
                    Objeto_Ticket_In_Response["Transfer_Amount"]=doc["Saldo"];
                    Objeto_Ticket_In_Response["Parsing_Code"]=0x00;

                   // String hexValue = doc["Validation_System_ID"]; // Recibir el valor hexadecimal como string, por ejemplo "0x12"
                    //uint8_t Validation_System_ID =  doc["Validation_System_ID"];
                    
                    const char *validationSystemIDStr = doc["Validation_System_ID"];


                    // Serial.print("Validation_System_ID (cadena): ");
                    // Serial.println(validationSystemIDStr);
                    int validationSystemID = strtol(validationSystemIDStr, nullptr, 16);
                    // Serial.print("Validation_System_ID (entero hexadecimal): ");
                    // Serial.println(validationSystemID, HEX); // Mostrar como hexadecimal
                    Objeto_Ticket_In_Response["Validation_System_ID"]=validationSystemID;

                    Objeto_Ticket_In_Response["Numero_Validacion"]=doc["Numero_Validacion"];
                    Objeto_Ticket_In_Response["Restricted_Expiration"]=doc["Expiracion_Ticket"];
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


bool TITO::Requerimiento_TITO_Ticket_In(int Evento, bool Habilita_Tito)
{
    if (Evento == 0x67 && Habilita_Tito)
    {
        Serial.println("Evento Ticket Insertado...");
        if (!Variables_globales.Get_Variable_Global(Attend_Pending_Tito_Request_In))
            Variables_globales.Set_Variable_Global(Attend_Pending_Tito_Request_In, true);
        return Variables_globales.Get_Variable_Global(Attend_Pending_Tito_Request_In);
    }
    else
        return false;
}

void TITO::Load_Pending_Ticket_Transactions(void)
{

    if (SPIFFS.exists(Ticketfile))
    {
        File file = SPIFFS.open(Ticketfile, "r");
        if (file)
        {
           
            while (file.available())
            {
                String line = file.readStringUntil('\n');
              //  Serial.println(line);
                Ticket_Pendientes.push_back(line);
            }
            file.close();
        }
    }else{

        File file = SPIFFS.open(Ticketfile, "a");
        if (file)
        {
            file.close();
        }
    }
}

bool TITO::Send_Transfer_Ticket(const String & json)
{

    int httpCode;
    int Code=false;

    char IP_Server[4];
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));
    std::string Ip=IP_toString_(IP_Server);
    String Ip_Server=String(Ip.c_str());
    String Puerto="9595";
    String fwurl = "http://"+Ip_Server+":"+Puerto+"/api/Tito/Ack";
    WiFiClient client;

    HTTPClient https;
   
    https.setTimeout(15000);

    if (https.begin(client, fwurl))
    {
        https.addHeader("Content-Type", "application/json");
        https.addHeader("hash",Info_Cashless.Get_Hash_Valido());
        https.addHeader("gmsec","GMaster");
        https.addHeader("Authorization", "Bearer " + Info_Cashless.Get_Token_Valido()); // Agrega el token de autorización
        httpCode = https.POST(json);

        if (httpCode == HTTP_CODE_OK)
        {
            String Response = https.getString();
            StaticJsonDocument<1024>
                doc,
                filter;
            DeserializationError error = deserializeJson(doc, Response);
            if (error)
            {
                #ifdef Debug_HTTPS
                                Serial.println("Error Json Contadores ");
                #endif
                Code=false;
            }
            else
            {
                bool IsSuccess = doc["IsSuccess"];
                String Msgg=doc["Message"];

                if(IsSuccess)
                {
                    Code=true;
                }
                    
                else
                {
                    Code=false;
                }
                   
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
    return Code;
}

void TITO::New_Transfer_Ticket(const String& json)
{
    if(!Send_Transfer_Ticket(json))
    {
        Serial.println(" Transaccion ticket no recibida ");
        Ticket_Pendientes.push_back(json); /* Agrega a la lista si no se puede enviar */
        Save_Ticket_Transaction(); 
        //Transaccion_Finalizada();
    }else{

        Serial.println(" Transaccion ticket ");
        StaticJsonDocument<200> filter;
        StaticJsonDocument<200> doc;
        doc.clear();
        filter.clear();
        // Crear un filtro para incluir solo la clave "IsSuccess"
        filter["IsSuccess"] = true;

        DeserializationError error = deserializeJson(doc, json, DeserializationOption::Filter(filter));
        if (!error)
        {
            bool isSuccess = doc["IsSuccess"];

            if (isSuccess)
            {
               Updated_Ticket_Counters(json); /* Transaccion OK  envia trama contadores */
            }
        }

        STATUS_TITO_TRANSFER(TITO::TRANS_TICKET_IDLE);
        //Transaccion_Finalizada();
    }
    
}

void TITO::Save_Ticket_Transaction(void)
{
    File file = SPIFFS.open(Ticketfile, "w");
    if (file) {
        for (const auto& transaccion : Ticket_Pendientes) {
            file.println(transaccion);
        }
        file.close();
    }
}

void TITO::Set_Flag_Ticket_Out_Pending(int Status)
{
    Flag_New_Transfer_Ticket__Out_Pending_=Status;
}

bool TITO::Get_Flag_Ticket_Out_Pending(void)
{
    return Flag_New_Transfer_Ticket__Out_Pending_;
}

void TITO::Available_Ticket_Transfer(int Timeout)
{
    bool Cambios=false;

    if (Get_Flag_Ticket_Out_Pending())
        Ticket_Mark_Pending();
    

    if(Get_Flag_New_Ticket_In())
    {
        String Json;
        serializeJson(Objeto_Ticket_In_Transfer, Json); /* Serializa Data */
        Objeto_Ticket_In_Transfer.clear(); /*  Limpia  Objeto Ack Transferencias carga */
        New_Transfer_Ticket(Json);
        Set_Flag_New_Ticket_In(false);
    }

    if(Get_Flag_New_Ticket_Out())
       Ticket_Marked_As_Completed();
    

    if(Flag_Parcial_New_Transfer_Ticket_In)
    {
        //Serial.println("Entrega parcial");
        String Json;
        serializeJson(Objeto_Ticket_In_Transfer, Json); /* Serializa Data */
        Objeto_Ticket_In_Transfer.clear(); /*  Limpia  Objeto Ack Transferencias carga */
        Send_Transfer_Ticket(Json);
        Flag_Parcial_New_Transfer_Ticket_In=false;
    }

    if(Flag_Parcial_New_Transfer_Ticket_Out)
    {
        String Json;
        serializeJson(Objeto_Ticket_Out, Json); /* Serializa Data */
        Objeto_Ticket_Out.clear(); /*  Limpia  Objeto Ack Transferencias carga */
        Send_Transfer_Ticket(Json);
        Flag_Parcial_New_Transfer_Ticket_Out=false;
    }

    if (!Ticket_Pendientes.empty())
    {
        Timeout_Tito_Transfer_Inicial=millis();
        if ((Timeout_Tito_Transfer_Inicial - Timeout_Tito_Transfer_Final) >= Timeout)
        {
            String transaccion = Ticket_Pendientes.front(); /* Toma la primera transferencia */

            if (Send_Transfer_Ticket(transaccion)) /* Intenta enviarla */
            {
                Ticket_Pendientes.erase(Ticket_Pendientes.begin()); /* Si el servidor la recibio la elimina */
                Cambios = true;

                /* Acualiza contadores Ticket */
                StaticJsonDocument<200> filter;
                StaticJsonDocument<200> doc;
                doc.clear();
                filter.clear();
                // Crear un filtro para incluir solo la clave "IsSuccess"
                filter["IsSuccess"] = true;

                DeserializationError error = deserializeJson(doc, transaccion, DeserializationOption::Filter(filter));
                if (!error)
                {
                    bool isSuccess = doc["IsSuccess"];

                    if (isSuccess)
                    {
                        Updated_Ticket_Counters(transaccion); /* Transaccion OK  envia trama contadores tito */
                    }
                }
            }
            Timeout_Tito_Transfer_Final=Timeout_Tito_Transfer_Inicial;
        }
    }

    if(Cambios)
        Save_Ticket_Transaction();
}

bool TITO::Updated_Ticket_Counters(String Type_Transaccion)
{

    #define TICKET_IN   "0x00"
    #define TICKET_OUT  "0x01"

    WiFiClient client;
    HTTPClient https;


    bool Code=false;
    int httpCode;

    char IP_Server[4];
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));
    std::string Ip=IP_toString_(IP_Server);
    String Ip_Server=String(Ip.c_str());
    String Puerto="9595";
    String fwurl = "http://"+Ip_Server+":"+Puerto+"/api/Tito/Contadores";


      /* Crea Objeto*/
    
    StaticJsonDocument<200> doc;
    StaticJsonDocument<200> filter;
    doc.clear();
    filter.clear();
    filter["Trans_Tipo"] = true; // Especificar la clave que deseas deserializar
    // Deserializar el JSON con el filtro
    DeserializationError error = deserializeJson(doc, Type_Transaccion, DeserializationOption::Filter(filter));

    String Trans_Tipo = doc["Trans_Tipo"];
    //Serial.println(Trans_Tipo);

    if(Trans_Tipo==TICKET_IN)
    {
        Actualiza_Tito_Entradas();


        unsigned long Respuesta_Server = millis();
        int TIMEOUT_CONECT_SERVER = 1500; // Espera 1.5 seg para Encuestar contadores
        while (!Flag_Entradas_Tito_OK && millis() - Respuesta_Server < TIMEOUT_CONECT_SERVER)
        {
#ifdef DEBUG_RFID
            Serial.println("Encuestando contadores.....");
#endif
            vTaskDelay(300);
            if(Flag_Entradas_Tito_OK)
                break;
        }

        Flag_Entradas_Tito_OK=false;

    }

    if(Trans_Tipo==TICKET_OUT)
    {
        Actualiza_Tito_Salidas();

        unsigned long Respuesta_Server = millis();
        int TIMEOUT_CONECT_SERVER = 1500; // Espera 1.5 seg para Encuestar contadores
        while (!Flag_Salidas_Tito_OK && millis() - Respuesta_Server < TIMEOUT_CONECT_SERVER)
        {
#ifdef DEBUG_RFID
            Serial.println("Encuestando contadores.....");
#endif
            vTaskDelay(300);
            if(Flag_Salidas_Tito_OK)
                break;
        }

        Flag_Salidas_Tito_OK=false;
    }

    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
    char Current_IP[4];
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    if (!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
        jsonDocument["IsSuccess"] = false;
    else
        jsonDocument["IsSuccess"] = true;

    jsonDocument["Ticket_In"] = contadores.Get_Contadores_String(Ticket_In);
    jsonDocument["Ticket_Out"] = contadores.Get_Contadores_String(Ticket_Out);
    
    jsonDocument["Ip"] = IP_toString_(Current_IP);
    jsonDocument["MAC"] = WiFi.macAddress();
    jsonDocument["Id_Maquina"] = 0;
    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    Serial.println(" ------------------------>SOLICITUD (HTTP POST) CONTADORES<-----------------------------");
    Serial.println(Json);
    Serial.println(" ---------------------------------------------------------------------------------------");
    https.setTimeout(10000); /* 10seg */

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
                    Code=true;
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





bool TITO::Await_Command_57(unsigned long Timeout)
{

    bool IsSuccess = false;
    unsigned long Timout_Break_Response;

    Buffer_Cashless.Init_Buffer_TITO();

    for (int i = 0; i < 1; i++)
    {
        sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        Transmite_Poll_Long(0x57);
        delay(500);
    }

    Timout_Break_Response = millis();

    while ((Buffer_Cashless.Get_Buffer_TITO()[1] == 0xAA) && (millis() - Timout_Break_Response < Timeout))
    {
        /* Poll de espera para mantener comunicacion */
        AFT.Mantiene_Comunicacion();


        esp_task_wdt_reset();
        delay(200);
        sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        Transmite_Poll_Long(0x57);

        if (Buffer_Cashless.Get_Buffer_TITO()[1] != 0xAA)
            break;
        vTaskDelay(10);
        // Serial.println("Esperando por la transferencia.....!");
    }

    int Recv = Buffer_Cashless.Get_Buffer_TITO()[1];

    if (Recv == 0x57)
        IsSuccess = true;
    else
        IsSuccess = false;

    return IsSuccess;
}

void TITO::Send_Command_58()
{
    char Amount[6];
    /* Amount */
    Amount[0] = Buffer_Cashless.Get_Buffer_TITO()[3];
    Amount[1] = Buffer_Cashless.Get_Buffer_TITO()[4];
    Amount[2] = Buffer_Cashless.Get_Buffer_TITO()[5];
    Amount[3] = Buffer_Cashless.Get_Buffer_TITO()[6];
    Amount[4] = Buffer_Cashless.Get_Buffer_TITO()[7];
    Buffer_Cashless.Init_Buffer_TITO();

    char Host_Command[13];
    int Host_Size = 10;

    /*DIRECCION MAQUINA */
    Host_Command[0] = 0x01;
    /* COMANDO */
    Host_Command[1] = 0x58;
    /*VALIDATION SYSTEM ID */

    Host_Command[2] = decimalToBCD(Tito.Get_Validacion_System_ID());
    /* AMOUNT*/
    Host_Command[3] = Tito.Get_Validacion_Number()[0];
    Host_Command[4] = Tito.Get_Validacion_Number()[1];
    Host_Command[5] = Tito.Get_Validacion_Number()[2];
    Host_Command[6] = Tito.Get_Validacion_Number()[3];
    Host_Command[7] = Tito.Get_Validacion_Number()[4];
    Host_Command[8] = Tito.Get_Validacion_Number()[5];
    Host_Command[9] = Tito.Get_Validacion_Number()[6];
    Host_Command[10] = Tito.Get_Validacion_Number()[7];

    /* CRC */
    Host_Command[11] = 0x00;
    Host_Command[12] = 0x00;

    // CalcularCRC_Tmp(); // Calcula CRC
    CalcularCRC_Transfer(Host_Command, Host_Size);
    Buffer_Cashless.Init_Buffer_TITO_58();
    Buffer_Cashless.Init_Buffer_TITO_3D_3E();
    delay(1);

    for (int i = 0; i < 13; i++)
    {
        if (i == 0)
            sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        else
            Transmite_Poll_Long(Host_Command[i]);
    }
}

bool TITO::Await_Command_58(unsigned long Timeout)
{

    unsigned long Timout_Break_Response;
    bool IsSuccess = false;
    Timout_Break_Response = millis();

    while ((Buffer_Cashless.Get_Buffer_TITO_58()[1] == 0xAA) && (millis() - Timout_Break_Response < Timeout))
    {
        esp_task_wdt_reset();
        AFT.Mantiene_Comunicacion();
        if (Buffer_Cashless.Get_Buffer_TITO_58()[1] != 0xAA)
            break;
        vTaskDelay(10);
        // Serial.println("Esperando por la transferencia.....!");
    }

    int Recv = Buffer_Cashless.Get_Buffer_TITO_58()[1];

    if (Recv == 0x58)
        IsSuccess = true;
    else
        IsSuccess = false;

    return IsSuccess;
}

void TITO::Request_Transfer_Tito_Out(void)
{
    STATUS_TITO_TRANSFER(TITO::TRANS_TICKET_EN_PROGRESO);

    Serial.println("--------------------------------> Evento 57 Atendido <-----------------------------------------------");

    if (Generate_Key_Ticket_Out())
    {

        /* Limpia buffer para la transaccion */
        Buffer_Cashless.Init_Buffer_TITO();

        if (Await_Command_57(2000))
        {

            Tito.Increase_Transaction_Number_ID_Tito();
            // Serial.println("------------------------> Comando 57 Rebido por la maquina <----------------------------------");

            /* Type Cashout */
            int Type_Ticket = Buffer_Cashless.Get_Buffer_TITO()[2];
            char Amount[6];
            /* Amount */
            Amount[0] = Buffer_Cashless.Get_Buffer_TITO()[3];
            Amount[1] = Buffer_Cashless.Get_Buffer_TITO()[4];
            Amount[2] = Buffer_Cashless.Get_Buffer_TITO()[5];
            Amount[3] = Buffer_Cashless.Get_Buffer_TITO()[6];
            Amount[4] = Buffer_Cashless.Get_Buffer_TITO()[7];

            /* Limpia buffer de recepcion tito */
            Buffer_Cashless.Init_Buffer_TITO();

            char Host_Command[13];
            int Host_Size = 10;

            /*DIRECCION MAQUINA */
            Host_Command[0] = 0x01;
            /* COMANDO */
            Host_Command[1] = 0x58;
            /*VALIDATION SYSTEM ID */

            Host_Command[2] = decimalToBCD(Tito.Get_Validacion_System_ID());
            /* AMOUNT*/
            Host_Command[3] = Tito.Get_Validacion_Number()[0];
            Host_Command[4] = Tito.Get_Validacion_Number()[1];
            Host_Command[5] = Tito.Get_Validacion_Number()[2];
            Host_Command[6] = Tito.Get_Validacion_Number()[3];
            Host_Command[7] = Tito.Get_Validacion_Number()[4];
            Host_Command[8] = Tito.Get_Validacion_Number()[5];
            Host_Command[9] = Tito.Get_Validacion_Number()[6];
            Host_Command[10] = Tito.Get_Validacion_Number()[7];

            /* CRC */
            Host_Command[11] = 0x00;
            Host_Command[12] = 0x00;

            // CalcularCRC_Tmp(); // Calcula CRC
            CalcularCRC_Transfer(Host_Command, Host_Size);
            Buffer_Cashless.Init_Buffer_TITO_58();
            Buffer_Cashless.Init_Buffer_TITO_3D_3E();
            delay(1);

            Send_Command_58(); /* Envia Comando 58 */

            if (Await_Command_58(15000))
            {
                int Status = Buffer_Cashless.Get_Buffer_TITO_58()[2];
                /* Recibio la data */
                // Serial.println("------------------------> Comando 58 Rebido por la maquina <----------------------------------");
                if (Status == 0x00)
                {
                    // Serial.println(" Ticket OK ");

                    Tito.Update_Ticket(Status, Type_Ticket);
                    Tito.Set_Flag_Ticket_Out_Pending(true);

                    if (Waiting_for_the_printed_ticket_event(15000))
                    {
                        STATUS_TITO_TRANSFER(TITO::TRANS_TICKET_PENDIENTE);

                        if (Waiting_For_This_Final_Ticket_Transaction(10000))
                        {
                            Tito.Status_Ticket_Out(Buffer_Cashless.Get_Buffer_TITO_4D(), RTC);
                            // Tito.Status_Process_Ticket(false);
                        }
                        else
                        {
                            STATUS_TITO_TRANSFER(TITO::TRANS_TICKET_TIMEOUT);
                            //Serial.println("TIEMOUT");
                        }
                    }
                    else
                    {
                        STATUS_TITO_TRANSFER(TITO::TRANS_TICKET_TIMEOUT);
                        //Serial.println("EVENTO NO RECIBIDO");
                    }
                    /* Ticket generado parcialmente */
                }
                else
                {

                    STATUS_TITO_TRANSFER(TITO::TRANS_TICKET_IDLE);
                    /* 0x80 y 0x81*/
                    Serial.println("Error Generando Ticket ");
                    Tito.Update_Ticket(0xFF, 0xAA);
                    Tito.Set_Flag_Ticket_Out_Pending(true);
                    Tito.Status_Process_Ticket(false);
                }
            }
            else
            {

                STATUS_TITO_TRANSFER(TITO::TRANS_TICKET_IDLE);
                Tito.Update_Ticket(0xFF, 0xAA);
                Tito.Set_Flag_Ticket_Out_Pending(true);
                Serial.println("Comando 58 no recibido ");
                Tito.Status_Process_Ticket(false);
            }
        }
        else
        {
            STATUS_TITO_TRANSFER(TITO::TRANS_TICKET_IDLE);
        }
    }
    else
    {
        Serial.println("Error Metodo HTTP Key");
        STATUS_TITO_TRANSFER(TITO::TRANS_TICKET_IDLE);
    }
}






bool TITO::Awaiting_Key(unsigned long timeout)
{
    unsigned long Timout_Break_Response;
    bool IsSuccess = false;
    Timout_Break_Response = millis();
   
    while (!Result_Key && (millis() - Timout_Break_Response < timeout))
    {
        esp_task_wdt_reset();
        AFT.Mantiene_Comunicacion();
        if (Result_Key)
            break;
        vTaskDelay(10);
        // Serial.println("Esperando por la transferencia.....!");
    }
    Result_Key = false;
    return Result_RequestKey;
}

void TITO::Ticket_Mark_Pending(void)
{
    String Json;
    Objeto_Ticket_Out["Trans_Tipo"] = TICKET_OUT;
    if (Objeto_Ticket_Out["Code"] == 0x00)
        Objeto_Ticket_Out["Codigo"] = 0x40;
    else
        Objeto_Ticket_Out["Codigo"] = 0xFF;
    serializeJson(Objeto_Ticket_Out, Json); /* Serializa Data */

    Send_Transfer_Ticket(Json);
    Serial.println(" ------------------------>SOLICITUD (HTTP POST) ACK TICKET OUT PENDIENTE <------------------");
    Serial.println(Json);
    Serial.println(" -------------------------------------------------------------------------------------------");
    Set_Flag_Ticket_Out_Pending(false);
}

void TITO::Ticket_Marked_As_Completed(void)
{
    String Json;
    serializeJson(Objeto_Ticket_Out, Json); /* Serializa Data */
    Objeto_Ticket_Out.clear();              /*  Limpia  Objeto Ack Transferencias carga */
    New_Transfer_Ticket(Json);
    Set_Flag_New_Ticket_Out(false);
}

void Task_Generate_Ticket(void *pvParameters)
{
    Serial.println("[TAREA] Iniciando generación de ticket...");
    Result_RequestKey = Tito.Generate_Key_Ticket_Out();
    Serial.println("[TAREA] Ticket finalizado. Eliminando tarea...");
    Result_Key=true;
    vTaskDelete(NULL);
}



bool Start_Ticket_Task(void)
{
    if (taskHandleTicket == NULL)
    {
        BaseType_t result = xTaskCreatePinnedToCore(
            Task_Generate_Ticket,   // función de la tarea
            "TicketTask_Key",       // nombre
            5000,                   // stack size
            NULL,                   // parámetros
            configMAX_PRIORITIES - 10, // prioridad
            &taskHandleTicket,      // handle
            0                       // core
        );

        if (result == pdPASS)
        {
            Serial.println("[INFO] Tarea de ticket lanzada correctamente.");
            return true;
        }
        else
        {
            Serial.println("[ERROR] No se pudo crear la tarea de ticket (memoria insuficiente).");
            taskHandleTicket = NULL;
            return false;
        }
    }
    else
    {
        Serial.println("[INFO] Ya existe una tarea de ticket en ejecución.");
        return false;
    }
}


bool TITO::Waiting_for_the_printed_ticket_event(unsigned long timeout)
{
    unsigned long Timout_Break_Response;
    Timout_Break_Response = millis();
   
    while (!Get_Confirma_Ticket() && (millis() - Timout_Break_Response < timeout))
    {
        esp_task_wdt_reset();
        AFT.Mantiene_Comunicacion();

        if (Get_Confirma_Ticket())
            break;

        vTaskDelay(200);
        //Serial.println("Imprimiendo ticket......");
    }

    return Get_Confirma_Ticket();
}

bool TITO::Waiting_For_This_Final_Ticket_Transaction(unsigned long Timeout)
{

    bool IsSucess = false;
    // Serial.println("Confirma Ticket ");
    Buffer_Cashless.Init_Buffer_TITO_4D();
    delay(10);

    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x4D);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0xC2);
    Transmite_Poll_Long(0xAC);

    unsigned long Timout_Break_Response;
    Timout_Break_Response = millis();
    esp_task_wdt_init(1000000, true);
    esp_task_wdt_add(NULL);

    delay(800);

    while ((Buffer_Cashless.Get_Buffer_TITO_4D()[1] == 0xAA || Buffer_Cashless.Get_Buffer_TITO_4D()[1] == 0x00) && (millis() - Timout_Break_Response < Timeout))
    {
        esp_task_wdt_reset();
        sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        Transmite_Poll_Long(0x4D);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0xC2);
        Transmite_Poll_Long(0xAC);
        

        if (Buffer_Cashless.Get_Buffer_TITO_4D()[1] != 0xAA && Buffer_Cashless.Get_Buffer_TITO_4D()[1] != 0x00)
            break;

        vTaskDelay(10);
        //Serial.println("Esperando por la transferencia.....!");
    }

    if (Buffer_Cashless.Get_Buffer_TITO_4D()[1] == 0x4D)
        IsSucess = true;
    else
        IsSucess = false;

    return IsSucess;
}

