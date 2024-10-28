#include "ArduinoJson.h"
#include <HTTPClient.h>
#include "API_Accounting.h"
#include <SPIFFS.h>
#include "Configuracion.h"
#include "Contadores.h"
#include <sstream> // Asegúrate de incluir esta biblioteca

std::vector<String> Premios_SAS_Pendientes;
const char* PremisoFile = "/Premios_SAS.txt";
DynamicJsonDocument Objeto_Premios_SAS(500);
extern ESP32Time RTC;
extern  Configuracion_ESP32 Configuracion;
extern Contadores_SAS contadores; // Objeto contiene contadores maquina


std::string IP_toString_Acc(char IP_Char[])
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


unsigned long API_Accounting::Convert_Counter(char buffer_Data_Met[])
{

    unsigned long resultado = ((buffer_Data_Met[0] - 48) * 1000000000UL) +
                              ((buffer_Data_Met[1] - 48) * 100000000UL) +
                              ((buffer_Data_Met[2] - 48) * 10000000UL) +
                              ((buffer_Data_Met[3] - 48) * 1000000UL) +
                              ((buffer_Data_Met[4] - 48) * 100000UL) +
                              ((buffer_Data_Met[5] - 48) * 10000UL) +
                              ((buffer_Data_Met[6] - 48) * 1000UL) +
                              ((buffer_Data_Met[7] - 48) * 100UL) +
                              ((buffer_Data_Met[8] - 48) * 10UL) +
                              ((buffer_Data_Met[9] - 48) * 1UL);
    return resultado;
}

unsigned long API_Accounting::Convert_Counter_4digit(char buffer_Data_Met[])
{

    unsigned long Resultado = ((buffer_Data_Met[0] - 48) * 1000) +
                              ((buffer_Data_Met[1] - 48) * 100) +
                              ((buffer_Data_Met[2] - 48) * 10) +
                              ((buffer_Data_Met[3] - 48) * 1);
    return Resultado;
}

/* Agrega premio SAS a lista de premios pendientes por transmitir */
void API_Accounting::Save_Handpay_Informations(char Buffer_MET[128], char Contador[])
{
    if(Convert_Counter(Contador)>0)
    {

        char Current_IP[4];
        String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());
        memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));
        unsigned long Amount=Convert_Counter(Contador);
        /* Eliminar Contador de Buffer */

        unsigned long Premio_SAS= Amount;
        int Progressive_Group=Buffer_MET[2];
        int Level=Buffer_MET[3];
        int Reset_ID=Buffer_MET[13];

        

        String Parcial_Pay=String(Buffer_MET[9],DEC)+String(Buffer_MET[10],DEC)+String(Buffer_MET[11],DEC)+String(Buffer_MET[12],DEC);
        
        Objeto_Premios_SAS.clear();


        Objeto_Premios_SAS["Grupo_Progresivo"]=Progressive_Group;
        Objeto_Premios_SAS["Nivel"]=Level;
        Objeto_Premios_SAS["Premio_SAS"]=Premio_SAS;
        Objeto_Premios_SAS["Pago_Parcial"]=Parcial_Pay;
        Objeto_Premios_SAS["Reset_ID"]=Reset_ID;
        Objeto_Premios_SAS["Id_Operador"]=contadores.Get_Operador_ID_Int((char*)contadores.Get_Operador_ID());
        Objeto_Premios_SAS["Fecha_Hora"]=DataTime;
        Objeto_Premios_SAS["Ip"] = IP_toString_Acc(Current_IP);
        Objeto_Premios_SAS["MAC"] = WiFi.macAddress();
        Objeto_Premios_SAS["Id_Maquina"] = 0;

        String Json;
        serializeJson(Objeto_Premios_SAS, Json); /* Serializa Data */
        
        Serial.println(Json);
        

        Premios_SAS_Pendientes.push_back(Json); // Agrega elemento a la lista

        // File file = SPIFFS.open(PremisoFile, "a");
        // if (file)
        // {

        //     for (const auto &transaccion_Premios : Premios_SAS_Pendientes)
        //     {
        //         file.println(transaccion_Premios);
        //     }
        //     file.close();
        // }
        
    }else{
        Serial.println("No es un premio");
    }
}

/* Metodo Web para reporte de premios SAS */
bool API_Accounting::Sed_Handpay_Informatios(ESP32Time RTC,String Data)
{
    bool Status=false;

    WiFiClient client;
    HTTPClient https;
    int httpCode;
    char IP_Server[4];
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));

    std::string Ip=IP_toString_Acc(IP_Server);
    String Ip_Server=String(Ip.c_str());
    String Puerto="9595";
    String fwurl = "http://"+Ip_Server+":"+Puerto+"/api/Cashless/Premios_SAS";
    https.setTimeout(15000);

    if (https.begin(client, fwurl))
    {
        
        https.addHeader("Content-Type", "application/json");
        // https.addHeader("hash",Info_Cashless.Get_Hash_Valido());
        // https.addHeader("gmsec","GMaster");
        // https.addHeader("Authorization", "Bearer " + Info_Cashless.Get_Token_Valido());
        
        httpCode = https.POST(Data);

      //  Serial.println(httpCode);
         
        if (httpCode == HTTP_CODE_OK)
        {

            String Response = https.getString();
            DynamicJsonDocument doc(200);
            DeserializationError error = deserializeJson(doc, Response);

            if (error)
            {
#ifdef Debug_HTTPS
                Serial.println("Error Json Contadores ");
#endif
                Status=false;
            }
            else
            {
                bool IsSuccess = doc["IsSuccess"];

                if(IsSuccess)
                    Status=true;
                else
                    Status=false;
            }

            doc.clear();
        }
        else
        {
           Status=false;
        }
        https.end();
    }

    return Status;
}

/* Reporta y actualiza Lista de premios pendientes */
void API_Accounting::Report_Informations_SAS(void)
{

    if (WiFi.status() == WL_CONNECTED)
    {
        if (!Premios_SAS_Pendientes.empty())
        {
            String Data = Premios_SAS_Pendientes.front();     /* Toma primer Premio*/
            bool Status = Sed_Handpay_Informatios(RTC, Data); /* Envia premio */
            
            if(Status)
            {
               
                Premios_SAS_Pendientes.erase(Premios_SAS_Pendientes.begin()); /* Elimina premio*/

                // File file = SPIFFS.open(PremisoFile, "w");
                // if (file)
                // {
                //     for (const auto &transaccion_Premios : Premios_SAS_Pendientes)
                //     {
                //         file.println(transaccion_Premios);
                //     }
                //     file.close();
                // }
                // else
                // {
                //     File file = SPIFFS.open(PremisoFile, "w");
                //     if (file)
                //     {
                //         for (const auto &transaccion_Premios : Premios_SAS_Pendientes)
                //         {
                //             file.println(transaccion_Premios);
                //         }
                //         file.close();
                //     }
                // }
            }   
        }
    }
}

/* Carga Premios SAS pendientes en memoria RAM para transmitirlos*/
void API_Accounting::Load_Premios_SAS(void)
{
    if (SPIFFS.exists(PremisoFile))
    {
        File file = SPIFFS.open(PremisoFile, "r");
        if (file)
        {
           
            while (file.available())
            {
                String line = file.readStringUntil('\n');
                //Serial.println(line);
                Premios_SAS_Pendientes.push_back(line);
            }
            file.close();
        }
    }else{

        File file = SPIFFS.open(PremisoFile, "a");
        if (file)
        {
            file.close();
        }
    }
}