#include <Arduino.h>
#include <ArduinoJson.h>
#include "Persistenca_Info.h"
#include "SD.h"
#include "HTTPClient.h"

#include "Configuracion.h"
#include "RFID.h"
#include <esp_task_wdt.h>

#include "Clase_Variables_Globales.h"
#include "Contadores.h"
#include "ESP32Time.h"
#include "time.h"



extern Configuracion_ESP32 Configuracion;
extern std::string IP_toString_(char IP_Char[]);
extern Cashless_API Info_Cashless;
extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales
extern Contadores_SAS contadores; // Objeto contiene contadores maquina


extern ESP32Time RTC; // Objeto contiene hora y fecha

File Og;
File Temp;

bool Persistenca_Info::SD_(const String &json, String Ruta)
{
    Og = SD.open(Ruta, FILE_APPEND);

    if (!Og)
    {
        return false;
    }

    Og.println(json);
    Og.close();
    return true;
}

bool Persistenca_Info::Enviar_Info(const String &json,int Timeout)
{
    int httpCode;
    bool Code=false;

    char IP_Server[4];
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));

    std::string Ip=IP_toString_(IP_Server);
    String Ip_Server=String(Ip.c_str());
    String Puerto="9595";
    String fwurl = "http://"+Ip_Server+":"+Puerto+"/api/Cashless/Contadores";

    WiFiClient client;

    HTTPClient https;
    https.setTimeout(Timeout);

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

// bool Persistenca_Info::Crea_Archivos_Backup_Maquina(String archivo)
// {
//     if (SD.exists(archivo))
//     {
//         Serial.println("El archivo Existe");
//         return true;
//     }
//     else
//     {

//         File file = SD.open(archivo, FILE_WRITE);

//         if (file)
//         {

//             file.close();
//             Serial.println("Archivo Backup Creado: " + archivo);
//             return true;
//         }
//         else
//         {
//             Serial.println("Archivo Backup Creado: " + archivo);
//             return false;
//         }
//     }
// }


// #define BUFFER_VACIO     0
// #define BUFFER_CON_DATOS 1
// #define FALLA_EN_ARCHIVO 2
// #define SD_NO_INSERTADA  3



// int Persistenca_Info::Get_Status_Backup(String Archivo, int Intentos_Conexion_SD)
// {

//     if (Variables_globales.Get_Variable_Global(SD_INSERT))
//     {
//         bool Pending = false;

//         for (int i = 1; i < Intentos_Conexion_SD; i++)
//         {
//             File file = SD.open(Archivo, FILE_READ);

//             if (file)
//             {
//                 Pending = file && file.size() > 0;
//                 if (Pending)
//                     return BUFFER_CON_DATOS;
//                 else
//                     return BUFFER_VACIO;
//             }
//             delay(5);
//         }

//         return FALLA_EN_ARCHIVO;
//     }
//     else
//         return SD_NO_INSERTADA;
// }


void Persistenca_Info::enviarInformacionMaquina(const String &json)
{

    File file = SD.open("/Buckup_Contadores.log", FILE_READ);

    bool Tramas_Pendientes = file && file.size() > 0;
    file.close();

    if (Tramas_Pendientes)
    {
        Serial.println("Hay pendientes guarda en SD");
        SD_(json, "/Buckup_Contadores.log");
        return;
    }
    Serial.println("No hay pendient intenta enviar");
    if (!Enviar_Info(json))
    { 
        Serial.println("Fallo el envio");
        /* Guardar en SD */
        SD_(json, "/Buckup_Contadores.log");
    }else{
    
        Serial.println("Informacion recibida con Exito!");
    }
}

bool Persistenca_Info::Init_Archive_Backup(void)
{
    if (Variables_globales.Get_Variable_Global(SD_INSERT))
    {
        if (!SD.exists("/Buckup_Contadores.log"))
        {
            File file = SD.open("/Buckup_Contadores.log", FILE_WRITE);

            if (file)
            {
                Serial.println("Archivo Creadoo....");
                file.close();
            }
            else
            {
                Serial.println("No Se Creo el archivo" + String("Buckup_Contadores.log"));
            }
        }else
            Serial.println("Existe el archivo Continua escribiendo");
    }else
        return false;

    return true;
}

String Persistenca_Info::Test(void)
{
    String Outp;
    Outp=Contadores_Accounting(3);
    return Outp;
}

void Persistenca_Info::Task_Info(int Timeout)
{
    TimeoutInicial = millis();

    if ((TimeoutInicial - TimeoutFinal) < Timeout)
        
        return;

    TimeoutFinal = TimeoutInicial;

    const char *pathOriginal = "/Buckup_Contadores.log";
    const char *pathTemporal = "/temporal.log";

    if (!SD.exists(pathOriginal))
    {

        if (SD.exists(pathTemporal))
        {
            Serial.println("Archivo temproral dectado!");
            SD.rename(pathTemporal, pathOriginal);
        }
        else
        {
            Init_Archive_Backup();
            return;
        }
    }

    File original = SD.open(pathOriginal, FILE_READ);
    if (!original || original.size() == 0) {
        original.close();
        //SD.remove(pathOriginal);
        return;
    }

    // Leer primera línea
    String linea = original.readStringUntil('\n');
    linea.trim();

    // Si está vacía, eliminarla
    if (linea.length() == 0) {
        original.close();
        //SD.remove(pathOriginal);
        return;
    }

    Serial.println(linea);

    // Si no se pudo enviar, conservar archivo tal cual
    if (!Enviar_Info(linea)) {
        original.close();  // No modificamos nada
        return;
    }

    // Si se envió correctamente: copiar el resto del archivo a uno temporal
    File temporal = SD.open(pathTemporal, FILE_WRITE);
    if (!temporal) {
        Serial.println("Error al abrir archivo temporal");
        original.close();
        return;
    }

    // Copiar líneas restantes
    while (original.available()) {

        esp_task_wdt_reset();
        String restante = original.readStringUntil('\n');
        restante.trim();
        if (restante.length() > 0) {
            temporal.println(restante);
        }
        vTaskDelay(5);
    }

    original.close();
    temporal.close();

    // Reemplazar original por temporal
    SD.remove(pathOriginal);
    SD.rename(pathTemporal, pathOriginal);

    Serial.println("Primera línea procesada y eliminada");
}

uint32_t Persistenca_Info::Generate_CheckSum(char Ip_ip[],char Ip_Server[],int Type_Machine)
{
    uint32_t sum = 0;
    for (int i = 0; i < 4; i++) {
        sum += Ip_ip[i];
        sum += Ip_Server[i];
    }
    sum += Type_Machine;
    return sum;
}

bool esJsonValido(const String &input) {
    // StaticJsonDocument<2048> doc;
    // DeserializationError error = deserializeJson(doc, input);

    // if (error) {
    //     Serial.print("Error al parsear JSON: ");
    //     Serial.println(error.c_str());
    //     return false;
    // }

    return true;
}


String Persistenca_Info::Contadores_Accounting(int ComandoGpollSAS)
{

    char Current_IP[4];
    String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    StaticJsonDocument<1024> jsonDocument;
    jsonDocument.clear();

    if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
        jsonDocument["IsSuccess"] = true;
    else
        jsonDocument["IsSuccess"] = false;

    jsonDocument["Comando"] = ComandoGpollSAS;
    jsonDocument["Total_Cancel_Credit"] = contadores.Get_Contadores_Int(Total_Cancel_Credit);
    jsonDocument["Coin_In"] = contadores.Get_Contadores_Int(Coin_In);
    jsonDocument["Coin_Out"] = contadores.Get_Contadores_Int(Coin_Out);

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 4)
        jsonDocument["Total_Drop"] = contadores.Get_Contadores_Int(Total_Drop);
    else
        jsonDocument["Total_Drop"] = contadores.Get_Contadores_Int(Total_Drop);

    jsonDocument["Jackpot"] = contadores.Get_Contadores_Int(Jackpot);

    jsonDocument["Physical_Coin_In"] = contadores.Get_Contadores_Int(Physical_Coin_In);
    jsonDocument["Physical_Coin_Out"] = contadores.Get_Contadores_Int(Physical_Coin_Out);

    jsonDocument["Total_Coin_Drop"] = contadores.Get_Contadores_Int(Total_Coin_Drop);
    jsonDocument["Machine_Paid_Progresive_Payout"] = contadores.Get_Contadores_Int(Machine_Paid_Progresive_Payout);
    jsonDocument["Machine_Paid_External_Bonus_Payout"] = contadores.Get_Contadores_Int(Machine_Paid_External_Bonus_Payout);
    jsonDocument["Attendant_Paid_Progresive_Payout"] = contadores.Get_Contadores_Int(Attendant_Paid_Progresive_Payout);
    jsonDocument["Attendant_Paid_External_Bonus_Payout"] = contadores.Get_Contadores_Int(Attendant_Paid_External_Bonus_Payout);
    jsonDocument["Ticket_In"] = contadores.Get_Contadores_Int(Ticket_In);
    jsonDocument["Ticket_Out"] = contadores.Get_Contadores_Int(Ticket_Out);

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 4 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 10)
        jsonDocument["Cancel_Credit_Hand_Pay"] = contadores.Get_Contadores_Int(Total_Cancel_Credit);
    else
        jsonDocument["Cancel_Credit_Hand_Pay"] = contadores.Get_Contadores_Int(Cancel_Credit_Hand_Pay);

    jsonDocument["Bill_Amount"] = contadores.Get_Contadores_Int(Bill_Amount);
    jsonDocument["Games_Since_Last_Power_Up"] = contadores.Get_Contadores_Int(Games_Since_Last_Power_Up);
    jsonDocument["Games_Played"] = contadores.Get_Contadores_Int(Games_Played);
    jsonDocument["Door_Open"] = contadores.Get_Contadores_Int(Door_Open);
    jsonDocument["Current_Credits"] = contadores.Get_Contadores_Int(Current_Credits);

    char buffer[5]; // 2 bytes hex + terminador nulo
    sprintf(buffer, "%02X%02X",
            contadores.Get_Contadores_Char(ROM_Signature)[0],
            contadores.Get_Contadores_Char(ROM_Signature)[1]);

    jsonDocument["ROM_Signature"] = buffer;

    jsonDocument["Serie_Trama"] = contadores.Get_Contadores_Int(Serie_Trama);
    jsonDocument["Cliente_ID"] = contadores.Get_Client_ID_Int();
    jsonDocument["Fecha_Hora"] = DataTime;

    jsonDocument["Estado_Maquina"] = Variables_globales.Get_Variable_Global(Flag_Maquina_En_Juego);
    jsonDocument["Operador_ID"] = contadores.Get_Operador_ID_Int_Op();

    jsonDocument["Ip"] = IP_toString_(Current_IP);
    jsonDocument["MAC"] = WiFi.macAddress();
    jsonDocument["Id_Maquina"] = 0;

    String Output;
    serializeJson(jsonDocument, Output); /* Serializa Data */
    return Output;
}

String Persistenca_Info::Eventos_Accounting(int ComandoGpollSAS, int Evento_SAS)
{


    char Current_IP[4];
    String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();

    if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
        jsonDocument["IsSuccess"] = true;
    else
        jsonDocument["IsSuccess"] = false;

    char EventoStr[6];  // Suficiente para "0xFF" + null
    sprintf(EventoStr, "0x%X", Evento_SAS);
    jsonDocument["Comando"] = ComandoGpollSAS;
    jsonDocument["Evento"] = EventoStr;
    jsonDocument["Message"] = "Evento SAS";
    jsonDocument["Fecha_Hora"] = DataTime;

    jsonDocument["Ip"] = IP_toString_(Current_IP);
    jsonDocument["MAC"] = WiFi.macAddress();
    jsonDocument["Id_Maquina"] = 0;

    String Output;
    serializeJson(jsonDocument, Output); /* Serializa Data */
    return Output;
}
