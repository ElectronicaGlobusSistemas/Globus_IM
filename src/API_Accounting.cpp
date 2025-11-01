#include "ArduinoJson.h"
#include <HTTPClient.h>
#include "API_Accounting.h"
#include <SPIFFS.h>
#include "Configuracion.h"
#include "Contadores.h"
#include <sstream> // Asegúrate de incluir esta biblioteca
#include "RFID.h"
#include <esp_task_wdt.h>
#include "Clase_Variables_Globales.h"

std::vector<String> Premios_SAS_Pendientes;
const char* PremisoFile = "/Premios_SAS.txt";
DynamicJsonDocument Objeto_Premios_SAS(500);
extern ESP32Time RTC;
extern  Configuracion_ESP32 Configuracion;
extern Contadores_SAS contadores; // Objeto contiene contadores maquina
extern Cashless_API Info_Cashless;
extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales

//#define Debug_Premios_SAS


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
        Objeto_Premios_SAS["Id_Operador"]=contadores.Convert_Char_In(contadores.Get_Operador_ID());

        switch (Info_Cashless.Type_Sesion())
        {
        
        case PLAYER_CASHLESS_SESION:
            Objeto_Premios_SAS["Id_Cliente"]= contadores.Get_Client_ID_Transaccion_Int();
        break;
        
        default:
            Objeto_Premios_SAS["Id_Cliente"]= contadores.Convert_Char_In(contadores.Get_Client_ID());
            break;
        }
        Objeto_Premios_SAS["Fecha_Hora"]=DataTime;
        Objeto_Premios_SAS["Ip"] = IP_toString_Acc(Current_IP);
        Objeto_Premios_SAS["MAC"] = WiFi.macAddress();
        Objeto_Premios_SAS["Id_Maquina"] = 0;
        

        String Json;
        serializeJson(Objeto_Premios_SAS, Json); /* Serializa Data */
        

        #ifdef Debug_Premios_SAS
        Serial.println(Json);
        #endif

        Premios_SAS_Pendientes.push_back(Json); // Agrega elemento a la lista

        File file = SPIFFS.open(PremisoFile, "a");
        if (file)
        {
            #ifdef Debug_Premios_SAS
            
            Serial.println("Premio SAS almacenado con exito!");

            #endif
            file.println(Premios_SAS_Pendientes.back());
            file.close();
        }
        
    }else{
        //Serial.println("No es un premio");
    }
}


/* Metodo Web para reporte de premios SAS */
bool API_Accounting::Send_Handpay_Informatios(ESP32Time RTC,String Data)
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
    String fwurl = "http://"+Ip_Server+":"+Puerto+"/api/Cashless/ProcesaPremioSAS";
    https.setTimeout(20000); // Establece el tiempo de espera en 20 segundos (20000 ms)

    if (https.begin(client, fwurl))
    {
        
        https.addHeader("Content-Type", "application/json");
        https.addHeader("hash",Info_Cashless.Get_Hash_Valido());
        https.addHeader("gmsec","GMaster");
        https.addHeader("Authorization", "Bearer " + Info_Cashless.Get_Token_Valido());
        
        httpCode = https.POST(Data);


        #ifdef Debug_Premios_SAS
            Serial.println();
            Serial.print("Estado de solicitud HTTP: ");
            Serial.println(httpCode);
        #endif
        
         
        if (httpCode == HTTP_CODE_OK)
        {

            String Response = https.getString();
            DynamicJsonDocument doc(200);
            DeserializationError error = deserializeJson(doc, Response);


            #ifdef Debug_Premios_SAS
            Serial.println(Response);
            #endif

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
                {
                    Status=true;
                    contadores.Close_ID_Operador();
                }
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
void API_Accounting::Delete_PremioSAS_On_List(const char* Filename)
{
    Premios_SAS_Pendientes.erase(Premios_SAS_Pendientes.begin()); /* Elimina premio*/
    const char*tempFilename="/tempFile.txt";

    /* Abre el archivo original en modo lectura */

    File fileOG = SPIFFS.open(PremisoFile, "r");
    if(!fileOG)
    {
        return;
    }

    /* Abre el archivo temporal en modo escritura */
    File TempFile=SPIFFS.open(tempFilename, "w");
    if(!TempFile)
    {
        fileOG.close();
        return;
    }

    /* Elimina la primera linea */

    fileOG.readStringUntil('\n');

    /* Guarda el resto del contenido en el archivo temporal */
    while(fileOG.available())
    {
        String Line=fileOG.readStringUntil('\n');
        TempFile.println(Line);
    }

    /* Cierra  ambos archivos */
    fileOG.close();
    TempFile.close();


    if(SPIFFS.exists(tempFilename))
    {
        /* Elimina el contenido*/
        SPIFFS.remove(PremisoFile);
        SPIFFS.rename(tempFilename, PremisoFile);
    }else{
        SPIFFS.remove(tempFilename);
    }
    
}

/* Metodo para enviar premios SAS  metodo HTTP */
void API_Accounting:: Report_Handpay_Informations_SAS(bool Token_Cashless)
{
    if (Token_Cashless)
    {

        
        Timer_Start_Premios = millis();

        if ((Timer_Start_Premios - Timer_End_Premios_SAS) >= TimeOut_Premios_SAS||!Firts_SAS)
        {

            if (WiFi.status() == WL_CONNECTED)
            {
                if (!Premios_SAS_Pendientes.empty())
                {
                    #ifdef Debug_Premios_SAS
                        Serial.println();
                        Serial.print("Cantidad de premios: ");
                        Serial.println(Premios_SAS_Pendientes.size());
                    #endif
                    String Data = Premios_SAS_Pendientes.front();     /* Toma primer Premio*/
                    bool Status = Send_Handpay_Informatios(RTC, Data); /* Envia premio */

                    if (Status)
                    {
                        
                        #ifdef Debug_Premios_SAS
                            Serial.println("Premio SAS Enviado...");
                        #endif
                        Premios_SAS_Pendientes.erase(Premios_SAS_Pendientes.begin()); /* Elimina premio*/

                        File file = SPIFFS.open(PremisoFile, "w");

                        if (file)
                        {
                            #ifdef Debug_Premios_SAS
                                Serial.println("Premio SAS Eliminado....");
                            #endif
                            for (const auto &transaccion_Premios : Premios_SAS_Pendientes)
                            {
                                esp_task_wdt_reset();
                                file.println(transaccion_Premios);
                            }
                            file.close();
                        }
                        else
                        {
                            File file = SPIFFS.open(PremisoFile, "w");
                            if (file)
                            {
                                #ifdef Debug_Premios_SAS
                                    Serial.println("Premio SAS Eliminado....");
                                #endif
                                for (const auto &transaccion_Premios : Premios_SAS_Pendientes)
                                {
                                    esp_task_wdt_reset();
                                    file.println(transaccion_Premios);
                                }
                                file.close();
                            }
                        }
                    }
                }
                Timer_End_Premios_SAS = Timer_Start_Premios;
            }
            Firts_SAS=true;
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
              //  Serial.println(line);
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



void API_Accounting::Change_Flag_Handler_Cancel_Credit(bool Status_Flag)
{
    Send_Handler_Cancel_Credit=Status_Flag;
}

bool API_Accounting::Get_Flag_Handler_Cancel_Credit(void)
{
    return Send_Handler_Cancel_Credit;
}


/* Envia información de premio */
bool API_Accounting::Send_Counter_App(void)
{


    /* Actualiza Contadores Premio  Cancel y Handpay */
    /* Capturar informacion  antes de reset para validar si cambio y esperar unos segundos */
    bool Status=false;

    char Current_IP[4];
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    StaticJsonDocument<800> jsonDocument;
    jsonDocument.clear();

    WiFiClient client;
    HTTPClient https;
    int httpCode;
    char IP_Server[4];
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));

    std::string Ip=IP_toString_Acc(IP_Server);
    String Ip_Server=String(Ip.c_str());
    String Puerto="9595";
    String fwurl = "http://"+Ip_Server+":"+Puerto+"/api/Cashless/ProcesaPremio";
    https.setTimeout(10000); // Establece el tiempo de espera en 20 segundos (20000 ms)


    if(!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
      jsonDocument["IsSuccess"] = false;
    else
      jsonDocument["IsSuccess"] = true;


    jsonDocument["Cancel_Credit_Hand_Pay"] = contadores.Get_Contadores_Int(Cancel_Credit_Hand_Pay);
    jsonDocument["Total_Cancel_Credit"] = contadores.Get_Contadores_Int(Total_Cancel_Credit);

    char Op[9]; // 8 caracteres + 1 para el carácter nulo

    Op[0] = contadores.Get_Operador_ID()[0];
    Op[1] = contadores.Get_Operador_ID()[1];
    Op[2] = contadores.Get_Operador_ID()[2];
    Op[3] = contadores.Get_Operador_ID()[3];
    Op[4] = contadores.Get_Operador_ID()[4];
    Op[5] = contadores.Get_Operador_ID()[5];
    Op[6] = contadores.Get_Operador_ID()[6];
    Op[7] = contadores.Get_Operador_ID()[7];
    Op[8] = '\0'; // Terminar la cadena con '\0'

    int Id_Op = atoi(Op); // Ahora convierte la cadena a entero
    jsonDocument["Id_Operador"]=Id_Op;
    jsonDocument["Ip"] = IP_toString_(Current_IP);
    jsonDocument["MAC"] = WiFi.macAddress();
    jsonDocument["Id_Maquina"] = 0;
    jsonDocument["Fecha_Hora"] = DataTime;
    

    String Output;
    serializeJson(jsonDocument, Output); /* Serializa Data */
    // Serial.println(Output);
    // Serial.println("-----------------------------------------------------------");
    if (https.begin(client, fwurl))
    {
        
         https.addHeader("Content-Type", "application/json");
        // https.addHeader("hash",Info_Cashless.Get_Hash_Valido());
        // https.addHeader("gmsec","GMaster");
        // https.addHeader("Authorization", "Bearer " + Info_Cashless.Get_Token_Valido());
        
        httpCode = https.POST(Output);


        #ifdef Debug_Premios_SAS
            Serial.println();
            Serial.print("Estado de solicitud HTTP: ");
            Serial.println(httpCode);
        #endif
        
         
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
                {
                    Status=true;
                    contadores.Close_ID_Operador();
                }
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