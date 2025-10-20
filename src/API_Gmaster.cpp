#include <Arduino.h>
#include "ArduinoJson.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include "AutoUpdate.h"
#include <esp_task_wdt.h>
#include "API_Gmaster.h"
#include "Contadores.h"
#include "Configuracion.h"
#include "ESP32Time.h"
#include "time.h"
#include "Clase_Variables_Globales.h"
#include "RFID.h"
#include "SD.h"


#define BLOCK_SIZE 258

QueueHandle_t API_Gmaster::colaPendientes = NULL;  // 🔹 se define una sola vez aquí


#define FILE_NAME "/fifo_bin.txt"

#define MAX_LIM_EVENTOS_Api 900

extern ESP32Time RTC; // Objeto contiene hora y fecha
extern Configuracion_ESP32 Configuracion;
extern Contadores_SAS contadores; // Objeto contiene contadores maquina
extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales

extern Cashless_API Info_Cashless;
extern String string_Fecha;
extern String string_Fecha_LOG;
extern String string_Fecha_Eventos;
extern String string_Fecha_Sesiones;
extern String string_Fecha_Premios;

extern char Archivo_CSV_Contadores[200];
extern char Archivo_CSV_Eventos[200];
extern char Archivo_LOG[200];
extern char Archivo_CSV_Sesiones[200];
extern char Archivo_CSV_Premios[200];

extern int day_copy;
extern int month_copy;
extern int year_copy;


extern unsigned short Ptr_Eventos_Marca_Temp;
extern unsigned short Ptr_Eventos_Marca;
extern unsigned short Num_Eventos;
extern unsigned char Tabla_Eventos_[ 999 ][ 8 ];
extern String IP_toString_String(char IP_Char[]);

bool API_Gmaster::Guarda_Trama(char Buffer[])
{

    char temp[BLOCK_SIZE];
    memset(temp, 0, BLOCK_SIZE);
    strncpy(temp, Buffer, BLOCK_SIZE - 1);

    if (xQueueSend(colaPendientes, temp, 0) == pdTRUE)
    {
        Serial.println("📥 Buffer agregado a la cola");
        return true;
    }
    else
    {
        Serial.println("⚠️ Cola llena, guardando en SD directamente");
        return true;
    }
}

bool API_Gmaster::Procesa_Cola_Tramas_Pendientes(void)
{
    bool IsSuccess = false;
    char Buffer[BLOCK_SIZE];

    if (xQueueReceive(colaPendientes, &Buffer, 0) == pdTRUE)
    {
        File file = SD.open(FILE_NAME, FILE_APPEND);
        if (file)
        {
            file.write((const uint8_t *)Buffer, BLOCK_SIZE);
            file.close();
            Serial.println("💾 Bloque guardado en SD");
            IsSuccess = true;
        }
        else
        {
            Serial.println("❌ Error al abrir archivo SD");
        }
    }

    return IsSuccess;
}

bool API_Gmaster::Inicializa_Cola_Tramas(bool Status)
{

    if (colaPendientes != NULL)
        return true;

    if (Status)
    {
        Serial.println("Cola OK");
    }
    colaPendientes = xQueueCreate(50, BLOCK_SIZE);

    if (colaPendientes == NULL)
        return false;
    else
        return true;
}
/*------------------------------------> Solicitudes <---------------------------------------------------------*/

void API_Gmaster::Transmite_Confirmacion_API(char Buffer[], String api, bool Token_Valido)
{

    if (Token_Valido)
    {

        if (WiFi.status() == WL_CONNECTED)
        {
            char Current_IP[4];
            memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

            int httpCode;
            // String fwurl = "http://192.168.5.100:5364/Api/Tarjeta/ProcesarEventos?idMaquina=" + Configuracion.Get_Configuracion(Id_Maquina, "Id_Maquina"); /* URL */
            String fwurl = Controlador_Principal + api + "?" + "Id=" + Configuracion.Get_Configuracion(Id_Maquina, "Id_Maquina")+"&Ip="+IP_toString_String(Current_IP);
           // Serial.println(fwurl);
            WiFiClient client;
           
            HTTPClient https;
            https.setTimeout(10000);
            if (https.begin(client, fwurl))
            {
                /* -----------------------> Configuración de la solicitud <-------------------------------- */
                https.addHeader("Authorization", "Bearer " + String(Access_Token_Api_Gmaster));
                https.addHeader("Content-Type", "application/json");
                /*------------------------------------------------------------------------------------------*/
                httpCode = https.POST((uint8_t *)Buffer, 258);
               // Serial.println(httpCode);
                if (httpCode == HTTP_CODE_OK)
                {

                    /* -----------------------> Respuesta <-------------------------------------------------*/
                    String payload = https.getString();
                    /*--------------------------------------------------------------------------------------*/
                   // Serial.println(payload);
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
                        String errorMsg = String(error.c_str());
                        Info_Cashless.Log(RTC, "ENVIO_CONFIRMACION_ACK_API", "ERROR_DESERIALIZANDO_RESPUESTA: " + errorMsg);
                    }
                    else
                    {
                        bool IsSuccess = doc["IsSuccess"];

                        if (IsSuccess)
                        {
#ifdef Debug_HTTPS
                            Serial.println("Confirmación recibida con exito!");
#endif
                            Info_Cashless.Log(RTC, "ENVIO_CONFIRMACION_ACK_API", "CONFIRMACION_RECIBIDA_CON_EXITO");
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
                    Info_Cashless.Log(RTC, "ENVIO_CONFIRMACION_ACK_API", "FALLO_EN_PETICION_HTTP_CODIGO_ERROR: " + String(httpCode));
                }
                https.end();
            }
            else
            {
                https.end();
            }
            // client->stop();
            // delete client;
        }
        else
        {
#ifdef Debug_HTTPS
            Serial.print("No conectado a la red WiFi");
#endif
            Info_Cashless.Log(RTC, "ENVIO_CONFIRMACION_ACK_API", "FALLO_NO_CONECTADO_A_LA_RED_WIFI");
        }
    }else
        Info_Cashless.Log(RTC, "ENVIO_CONFIRMACION_ACK_API", "FALLO_TOKEN_DE_ACCESO_NO_GENERADO");
}

bool API_Gmaster::Trasmite_Contadores_Gmaster_Api(char Buffer[], String Api, bool Token_Valido)
{

    bool IsSuccess=false;
    
    if (Token_Valido)
    {

        if (WiFi.status() == WL_CONNECTED)
        {

            char Current_IP[4];
            memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

            int httpCode;
            String fwurl = Controlador_Principal + Api + "?" + "Id=" + Configuracion.Get_Configuracion(Id_Maquina, "Id_Maquina") + "&Ip=" + IP_toString_String(Current_IP);
            //Serial.println(fwurl);
            // String fwurl = "http://192.168.5.100:5364/Api/Tarjeta/ProcesarContadores?idMaquina=36087"; /* URL */
            // String fwurl = "http://192.168.5.100:5364/Api/Tarjeta/ProcesarContadores?idMaquina=" + Configuracion.Get_Configuracion(Id_Maquina, "Id_Maquina"); /* URL */
            // Serial.println(fwurl);
            // Serial.println(fwurl);

            WiFiClient client;
            HTTPClient https;
            https.setTimeout(10000);


            //Serial.println("Actualizandoo....");
            if (https.begin(client, fwurl))
            {
                https.addHeader("Authorization", "Bearer " + String(Access_Token_Api_Gmaster)); // Agrega el token de autorización
                https.addHeader("Content-Type", "application/json");
                httpCode = https.POST((uint8_t *)Buffer, 258);
                
                
                // Serial.println(httpCode);
                if (httpCode == HTTP_CODE_OK)
                {

                    String Response = https.getString();
                    StaticJsonDocument<200>
                        doc,
                        filter;
                    DeserializationError error = deserializeJson(doc, Response);

                    if (error)
                    {
#ifdef Debug_HTTPS
                        Serial.println("Error Json Contadores ");
#endif
                        String errorMsg = String(error.c_str());
                        Info_Cashless.Log(RTC, "ENVIO_TRAMA_CONTADORES_API", "ERROR_DESERIALIZANDO_OBJETO_RESPONSE: " + errorMsg);

                        IsSuccess=false;
                    }
                    else
                    {
                        bool IsSuccess = doc["IsSuccess"];
                        String Length = doc["Data"];

                        if (IsSuccess)
                        {
                            if (Variables_globales.Get_Variable_Global(Serializacion_Serie_Trama))
                            {
                                contadores.Incrementa_Serie_Trama();
                            }
#ifdef Debug_HTTPS
                            Serial.println("Longitud de trama " + Length);
                            Serial.println("Contadores recibidos con exito!");
#endif
                            Info_Cashless.Log(RTC, "ENVIO_TRAMA_CONTADORES_API", "CONTADORES_RECIBIDOS_CON_EXITO");
                            IsSuccess=true;
                        }
                        else
                        {
                            Info_Cashless.Log(RTC, "ENVIO_TRAMA_CONTADORES_API", "CONTADORES_RECIBIDOS_NO_PROCESADOS");
                            IsSuccess=false;
                        }
                    }

                    doc.clear();
                }
                else
                {
#ifdef Debug_HTTPS
                    Serial.print("No se pudo enviar");
                    Serial.println(httpCode);
#endif
                    Info_Cashless.Log(RTC, "ENVIO_TRAMA_CONTADORES_API", "FALLO_EN_PETICION_HTTP_CODIGO_ERROR: " + String(httpCode));
                    IsSuccess=false;
                }
                https.end();
            }
            else

            {
                Info_Cashless.Log(RTC, "ENVIO_TRAMA_CONTADORES_API", "NO_SE_ESTABLECIO_CONEXION_CON_SERVER" + String(fwurl));
                https.end();
                IsSuccess=false;
            }

            // client->flush();
            // client->stop();
            // delete client;
        }
        else
        {
#ifdef Debug_HTTPS
            Serial.print("No conectado a la red WiFi");
#endif
            Info_Cashless.Log(RTC, "ENVIO_TRAMA_CONTADORES_API", "FALLO_NO_CONECTADO_A_LA_RED_WIFI");
            IsSuccess=false;
        }
    }
    else
    {
        Info_Cashless.Log(RTC, "ENVIO_TRAMA_CONTADORES_API", "FALLO_TOKEN_DE_ACCESO_NO_GENERADO");
        IsSuccess=false;
    }

    return IsSuccess;
        
}

void API_Gmaster::Transmite_Eventos_Gmaster_Api(char Buffer[], String Api, bool Token_Valido)
{

    if (Token_Valido)
    {

        if (WiFi.status() == WL_CONNECTED)
        {

            char Current_IP[4];
            memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

            int httpCode;
            // String fwurl = "http://192.168.5.100:5364/Api/Tarjeta/ProcesarEventos?idMaquina=" + Configuracion.Get_Configuracion(Id_Maquina, "Id_Maquina"); /* URL */

            String fwurl = Controlador_Principal + Api + "?" + "Id=" + Configuracion.Get_Configuracion(Id_Maquina, "Id_Maquina") + "&Ip=" + IP_toString_String(Current_IP);
            // Serial.println(fwurl);
            WiFiClient client;
            HTTPClient https;

            https.setTimeout(10000);
            // https.setTimeout(10000); /* 10Seg */

            if (https.begin(client, fwurl))
            {
                https.addHeader("Authorization", "Bearer " + String(Access_Token_Api_Gmaster)); // Agrega el token de autorización
                https.addHeader("Content-Type", "application/json");

                httpCode = https.POST((uint8_t *)Buffer, 258);

                if (httpCode == HTTP_CODE_OK || httpCode == HTTPC_ERROR_READ_TIMEOUT)
                {
#ifdef Debug_HTTPS
                    Serial.println("Evento Enviando");
#endif

                    String payload = https.getString();

                    StaticJsonDocument<500> doc, filter;
                    DeserializationError error = deserializeJson(doc, payload);

                    if (error)
                    {
#ifdef Debug_HTTPS
                        Serial.println("Error Json Eventos ");
#endif
                        Info_Cashless.Log(RTC, "ENVIO_EVENTOS_API", "ERROR_DESERIALIZANDO_OBJETO_RESPONSE");
                    }
                    else
                    {
                        bool IsSuccess = doc["IsSuccess"];
                        // Serial.println(IsSuccess);
                        if (IsSuccess)
                            Marca_Eventos_Api();
                        else
                            Info_Cashless.Log(RTC, "ENVIO_EVENTOS_API", "EVENTOS_NO_RECIBIDOS_POR_SERVIDOR");
                    }

                    doc.clear();
                }else{
                    Info_Cashless.Log(RTC, "ENVIO_EVENTOS_API", "FALLO_EN_PETICION_HTTP_CODIGO_ERROR: " + String(httpCode));
                }
                https.end();
            }
            else
            {
                Info_Cashless.Log(RTC, "ENVIO_EVENTOS_API", "NO_SE_ESTABLECIO_CONEXION_CON " + String(fwurl));
                https.end();
            }
            // client->stop();
            // delete client;
        }
        else
        {
#ifdef Debug_HTTPS
            Serial.print("No conectado a la red WiFi");
#endif
            Info_Cashless.Log(RTC, "ENVIO_EVENTOS_API", "FALLO_NO_CONECTADO_A_LA_RED_WIFI");
        }
    }
    else
        Info_Cashless.Log(RTC, "ENVIO_EVENTOS_API", "FALLO_TOKEN_DE_ACCESO_NO_GENERADO");
}

void API_Gmaster::Sincroniza_Reloj_RTC_API(String Api)
{

    
    Timer_Sincro_RTC = millis();
    // Serial.println(Inicia_Solicitud);
    // Serial.println(Variables_globales.Get_Variable_Global(Token_Valido_Generado));
    // Serial.println(Variables_globales.Get_Variable_Global(Sincronizacion_RTC));
    // Serial.println(WiFi.status());


    if (!Inicia_Solicitud && Variables_globales.Get_Variable_Global(Token_Valido_Generado) && !Variables_globales.Get_Variable_Global(Sincronizacion_RTC) && WiFi.status()==WL_CONNECTED)
    {

        char Current_IP[4];
        memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));
        int httpCode;

        String fwurl = Controlador_Principal + Api + "?" + "Id=" + Configuracion.Get_Configuracion(Id_Maquina, "Id_Maquina")+"&Ip="+IP_toString_String(Current_IP);
        //String fwurl = "http://192.168.5.100:5364/Api/Tarjeta/GeneraRtc";
       // Serial.println(fwurl);
        WiFiClient client;
        //Serial.println(fwurl);
        HTTPClient https;
        https.setTimeout(10000);
        if (https.begin(client, fwurl))
        {

           // Serial.println(Access_Token_Api_Gmaster);
            https.addHeader("Authorization", "Bearer " + String(Access_Token_Api_Gmaster)); // Agrega el token de autorización
            #ifdef Debug_HTTPS
            Serial.print("[HTTPS] GET...\n");
            #endif
            httpCode = https.GET();
           // Serial.println(httpCode);
            if (httpCode == HTTP_CODE_OK)
            {

                String payload = https.getString();

                /* Hora -> Dia-> Mes-> Ano */

                StaticJsonDocument<1024> doc, filter;
                DeserializationError error = deserializeJson(doc, payload);

                // Verificar errores de deserialización
                if (error)
                {
                    #ifdef Debug_HTTPS
                    Serial.print("deserializeJson() failed: ");
                    Serial.println(error.c_str());
                    #endif

                    String errorMsg = String(error.c_str());
                    Info_Cashless.Log(RTC, "SINCRONIZA_RTC_API", "ERROR_DESERIALIZANDO_RESPUESTA: " + errorMsg);
                }
                else
                {

                    /* Url Generica */
                    int hour, minutes, seconds, day, month, year;

                    hour = doc["Data"]["Hour"];
                    minutes = doc["Data"]["Minutes"];
                    seconds = doc["Data"]["Seconds"];
                    day = doc["Data"]["Day"];
                    month = doc["Data"]["Month"];
                    year = doc["Data"]["Year"];
                    bool Serie_Trama_confirma = doc["Data"]["Serie_Trama"];
                    bool IsSuccess = doc["IsSuccess"];

                    if (IsSuccess)
                    {

                        if (Serie_Trama_confirma)
                        {
                            if (!Variables_globales.Get_Variable_Global(Serializacion_Serie_Trama))
                            {
                                if (contadores.Incrementa_Serie_Trama())
                                {
                                    Variables_globales.Set_Variable_Global(Serializacion_Serie_Trama, true);
                                }
                            }
                        }

                        RTC.setTime(seconds, minutes, hour, day, month, year);

                        if ((hour == RTC.getHour(true)) && (minutes == RTC.getMinute()) && (day == RTC.getDay()) && ((month - 1) == RTC.getMonth()) && (year == RTC.getYear()))
                        {

//#ifdef Debug_Mensajes_Server
                            Serial.println("RTC sincronizado con exito!");
//#endif
                            /*---------------------------> Crea archivos------- <------------------------------------------ */

                            string_Fecha = "Contadores-" + String(day) + String(month) + String(year) + ".CSV";
                            string_Fecha_LOG = "Log-" + String(day) + String(month) + String(year) + ".TXT";
                            string_Fecha_Eventos = "Eventos-" + String(day) + String(month) + String(year) + ".CSV";
                            string_Fecha_Sesiones = "Sesiones_RFID-" + String(day) + String(month) + String(year) + ".CSV";
                            string_Fecha_Premios = "Premios_Maquina-" + String(day) + String(month) + String(year) + ".CSV";
                            /*Convierte nombre de archivos en char*/
                            strncpy(Archivo_CSV_Contadores, string_Fecha.c_str(), sizeof(Archivo_CSV_Contadores));
                            strncpy(Archivo_LOG, string_Fecha_LOG.c_str(), sizeof(Archivo_LOG));
                            strncpy(Archivo_CSV_Eventos, string_Fecha_Eventos.c_str(), sizeof(Archivo_CSV_Eventos));
                            strncpy(Archivo_CSV_Sesiones, string_Fecha_Sesiones.c_str(), sizeof(Archivo_CSV_Sesiones));
                            strncpy(Archivo_CSV_Premios, string_Fecha_Premios.c_str(), sizeof(Archivo_CSV_Premios));

                            /* Crea copia de  fecha */
                            day_copy = day;
                            month_copy = month;
                            year_copy = year;

                            Variables_globales.Set_Variable_Global(Sincronizacion_RTC, true);
                            Variables_globales.Set_Variable_Global(Flag_Crea_Archivos, true);
                        }

                        Info_Cashless.Log(RTC, "SINCRONIZA_RTC_API", "RTC_SINCRONIZADO_CON_EXITO");
                    }else{
                        Info_Cashless.Log(RTC, "SINCRONIZA_RTC_API", "FALLA_SINCRONIZANDO_RTC_IsSuccess_False");
                    }
                    doc.clear();
                }
            }
            else
            {
#ifdef Debug_HTTPS
                Serial.print("Ack no enviado");
                Serial.println(httpCode);
#endif
                Info_Cashless.Log(RTC, "SINCRONIZA_RTC_API", "FALLO_EN_PETICION_HTTP_CODIGO: "+String(httpCode));
            }
            https.end();
        }else{
            Info_Cashless.Log(RTC, "SINCRONIZA_RTC_API", "NO_SE_ESTABLECIO_CONEXION_CON_SERVIDOR: "+String(fwurl));
            https.end();
        }
        // client->stop();
        // delete client;
        Inicia_Solicitud = true;
        Timer_Sincro_Previo_RTC = Timer_Sincro_RTC;
    }

    if ((Timer_Sincro_RTC - Timer_Sincro_Previo_RTC) > Timer_Sincro_Ok && Variables_globales.Get_Variable_Global(Token_Valido_Generado) && !Variables_globales.Get_Variable_Global(Sincronizacion_RTC) && WiFi.status()==WL_CONNECTED)
    {
        char Current_IP[4];
        memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

        int httpCode;
        // String fwurl = Controlador_Principal + Api + "?" + "Id_Maquina=" + Configuracion.Get_Configuracion(Id_Maquina, "Id_Maquina");
        String fwurl = Controlador_Principal + Api + "?" + "Id=" + Configuracion.Get_Configuracion(Id_Maquina, "Id_Maquina")+"&Ip=" + IP_toString_String(Current_IP)+"&Mac="+WiFi.macAddress();
        //fwurl = "http://192.168.5.100:5364/Api/Tarjeta/GeneraRtc";
       // Serial.println(fwurl);

        WiFiClient client;
       
        HTTPClient https;
        https.setTimeout(500);
        if (https.begin(client, fwurl))
        {
            https.addHeader("Authorization", "Bearer " + String(Access_Token_Api_Gmaster)); // Agrega el token de autorización
#ifdef Debug_HTTPS
            Serial.print("[HTTPS] GET...\n");
#endif
            httpCode = https.GET();
            if (httpCode == HTTP_CODE_OK)
            {

                String payload = https.getString();

                /* Hora -> Dia-> Mes-> Ano */

                StaticJsonDocument<1024> doc, filter;
                DeserializationError error = deserializeJson(doc, payload);

                // Verificar errores de deserialización
                if (error)
                {
#ifdef Debug_HTTPS
                    Serial.print("deserializeJson() failed: ");
                    Serial.println(error.c_str());
#endif
                }
                else
                {
                    /* Url Generica */
                    int hour, minutes, seconds, day, month, year;

                    hour = doc["Data"]["Hour"];
                    minutes = doc["Data"]["Minutes"];
                    seconds = doc["Data"]["Seconds"];
                    day = doc["Data"]["Day"];
                    month = doc["Data"]["Month"];
                    year = doc["Data"]["Year"];
                    bool IsSuccess = doc["IsSuccess"];
                    bool Serie_Trama_confirma = doc["Data"]["Serie_Trama"];

                    if (IsSuccess)
                    {

                        if (Serie_Trama_confirma)
                        {
                            if (!Variables_globales.Get_Variable_Global(Serializacion_Serie_Trama))
                            {
                                if (contadores.Incrementa_Serie_Trama())
                                {
                                    Variables_globales.Set_Variable_Global(Serializacion_Serie_Trama, true);
                                }
                            }
                        }
                        RTC.setTime(seconds, minutes, hour, day, month, year);

                        if ((hour == RTC.getHour(true)) && (minutes == RTC.getMinute()) && (day == RTC.getDay()) && ((month - 1) == RTC.getMonth()) && (year == RTC.getYear()))
                        {

#ifdef Debug_Mensajes_Server
                            Serial.println("RTC sincronizado con exito!");
#endif
                            /*---------------------------> Crea archivos------- <------------------------------------------ */

                            string_Fecha = "Contadores-" + String(day) + String(month) + String(year) + ".CSV";
                            string_Fecha_LOG = "Log-" + String(day) + String(month) + String(year) + ".TXT";
                            string_Fecha_Eventos = "Eventos-" + String(day) + String(month) + String(year) + ".CSV";
                            string_Fecha_Sesiones = "Sesiones_RFID-" + String(day) + String(month) + String(year) + ".CSV";
                            string_Fecha_Premios = "Premios_Maquina-" + String(day) + String(month) + String(year) + ".CSV";
                            /*Convierte nombre de archivos en char*/
                            strncpy(Archivo_CSV_Contadores, string_Fecha.c_str(), sizeof(Archivo_CSV_Contadores));
                            strncpy(Archivo_LOG, string_Fecha_LOG.c_str(), sizeof(Archivo_LOG));
                            strncpy(Archivo_CSV_Eventos, string_Fecha_Eventos.c_str(), sizeof(Archivo_CSV_Eventos));
                            strncpy(Archivo_CSV_Sesiones, string_Fecha_Sesiones.c_str(), sizeof(Archivo_CSV_Sesiones));
                            strncpy(Archivo_CSV_Premios, string_Fecha_Premios.c_str(), sizeof(Archivo_CSV_Premios));

                            /* Crea copia de  fecha */
                            day_copy = day;
                            month_copy = month;
                            year_copy = year;

                            Variables_globales.Set_Variable_Global(Sincronizacion_RTC, true);
                            Variables_globales.Set_Variable_Global(Flag_Crea_Archivos, true);
                        }
                    }
                    doc.clear();
                }
            }
            else
            {
#ifdef Debug_HTTPS
                Serial.print("Ack no enviado");
                Serial.println(httpCode);
#endif
            }
            https.end();
        }else{
            https.end();
        }
        // client->stop();
        // delete client;
        Timer_Sincro_Previo_RTC = Timer_Sincro_RTC;
    }
}

int API_Gmaster::Verify_Expires_Token(bool SincroRTC, int Contador, bool Token_Valido, String Api_Token)
{

    int Dia_Expires = Get_DateTime_Expires_Token()[0];
    int Mes_Expires = Get_DateTime_Expires_Token()[1];
    int Year_Expires = Get_DateTime_Expires_Token()[2];
    int Current_Day = RTC.getDay();
    int Current_Month = RTC.getMonth();
    int Current_Year = RTC.getYear();

    Timmer_Create_Token = millis();
    TimeOut_Verify_Token=millis();

    if (Dia_Expires == 0 && Mes_Expires == 0 && Year_Expires == 0 && !SincroRTC)
    {
        if(!Token_Inicial && !Variables_globales.Get_Variable_Global(Token_Valido_Generado))
        {
            Variables_globales.Set_Variable_Global(Token_Valido_Generado,false);
            Token_Generator_Gmaster(Api_Token);
            #ifdef Debug_HTTPS
                Serial.println("Crea Token Acceso Inicial");
            #endif
            
            Token_Inicial=true;
            Timmer_Create_Token_Previous = Timmer_Create_Token;
            if(Variables_globales.Get_Variable_Global(Token_Valido_Generado))
            {
              //  Serial.println("Token Creado...OK");
                return 0;
            }     
            else
                return 1;
        }
        /* ---------------------> Crea Token Inicial <-------------------------- */

        if ((Timmer_Create_Token - Timmer_Create_Token_Previous) > TimeOut_Token && !Variables_globales.Get_Variable_Global(Token_Valido_Generado))
        {
           // Serial.println("Solicitud Token Bucle...");
            Variables_globales.Set_Variable_Global(Token_Valido_Generado,false);
            Token_Generator_Gmaster(Api_Token);
            #ifdef Debug_HTTPS
                Serial.println("Solicitud Token Acceso! ");
            #endif
            Timmer_Create_Token_Previous = Timmer_Create_Token;
            if(Variables_globales.Get_Variable_Global(Token_Valido_Generado))
            {
             //   Serial.println("Token Creado...OK");
                return 0;
            }
            else
                return 1;
        }
        return 4; /* Verificando.....*/
    }
    else
    {

        if ((TimeOut_Verify_Token - TimeOut_Verify_Token_Previous) > TimeOut_Verify)
        {
            /* ---------------------> Verifica expiracion <------------------------- */
            Mes_Expires = Mes_Expires - 1;
            // Verifica si el año actual es mayor al año de expiración
            if (Current_Year > Year_Expires)
            {
                Variables_globales.Set_Variable_Global(Token_Valido_Generado,false);
                #ifdef Debug_HTTPS
                    Serial.println("Solicitud nuevo Token expirado por Año!");
                #endif
                Token_Generator_Gmaster(Api_Token);
                TimeOut_Verify_Token_Previous=TimeOut_Verify_Token;
                if (Variables_globales.Get_Variable_Global(Token_Valido_Generado))
                    return 0;
                else
                    return 1;
            }
            else if (Current_Year == Year_Expires)
            {
                // El año es el mismo, verifica el mes
                if (Current_Month > Mes_Expires)
                {
                    // Serial.println(Mes_Expires);
                    // Serial.println("Crea Token.....");
                    // Serial.println(Current_Month);
                    Variables_globales.Set_Variable_Global(Token_Valido_Generado,false);
                    #ifdef Debug_HTTPS
                    Serial.println("Solicitud nuevo Token expirado por Mes!");
                    #endif
                    Token_Generator_Gmaster(Api_Token);
                   // Serial.println("Crea Token Acceso.... Por mes");
                    TimeOut_Verify_Token_Previous=TimeOut_Verify_Token;
                    if (Variables_globales.Get_Variable_Global(Token_Valido_Generado))

                        return 0;
                    else
                        return 1;
                }
                else if (Current_Month == Mes_Expires)
                {
                    // El mes es el mismo, verifica el día
                    if (Current_Day >= Dia_Expires)
                    {
                        Variables_globales.Set_Variable_Global(Token_Valido_Generado,false);
                        #ifdef Debug_HTTPS
                        Serial.println("Solicitud nuevo Token expirado por dia!");
                        #endif
                        Token_Generator_Gmaster(Api_Token);
                        TimeOut_Verify_Token_Previous=TimeOut_Verify_Token;
                        if (Variables_globales.Get_Variable_Global(Token_Valido_Generado))

                            return 0;
                        else
                            return 1;
                    }
                }else{
                    TimeOut_Verify_Token_Previous=TimeOut_Verify_Token;
                  //  Serial.println("No expirado...");
                    return 3; /* Token No ha Expirado Token OK */ 
                }
            }
            TimeOut_Verify_Token_Previous=TimeOut_Verify_Token;
           // Serial.println("No expirado...");
            return 3; /* Token No ha Expirado Token OK */ 
        }
        return 4; /* Verificando.....*/
    }

    return 5;
}

String API_Gmaster::Token_Generator_Gmaster(String Api)
{

    char Current_IP[4];
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    String payload;
    int httpCode;
    String fwurl = Controlador_Principal + Api + "?" + "Id=" + Configuracion.Get_Configuracion(Id_Maquina, "Id_Maquina") + "&Ip=" + IP_toString_String(Current_IP) + "&Mac=" + WiFi.macAddress();
    // String fwurl = "http://192.168.5.100:5364/Api/Token/GenerarTokenApi?Id=" + Configuracion.Get_Configuracion(Id_Maquina, "Id_Maquina");
    // https://cashlessapi.globussistemas.net/Api/Token/GenerarTokenApi?Id=35856

    /* En el 80 las peticiones de HTTP*/
    //  Serial.println(fwurl);
#ifdef Debug_HTTPS
    Serial.println(fwurl);
#endif

    WiFiClient client;
    HTTPClient https;
    String Output = "";

    https.setTimeout(10000);

    if (WiFi.status() == WL_CONNECTED)
    {

        if (https.begin(client, fwurl))
        {
#ifdef Debug_HTTPS
            Serial.print("[HTTPS] GET...\n");
#endif
            httpCode = https.GET();
            // #ifdef Debug_HTTPS
            //    Serial.println(httpCode);
            // #endif
            if (httpCode == HTTP_CODE_OK)
            {
                payload = https.getString();

#ifdef Debug_HTTPS
                Serial.println(payload);
#endif

                StaticJsonDocument<1024>
                    doc,
                    filter;
                DeserializationError error = deserializeJson(doc, payload);

                if (error)
                {
#ifdef Debug_HTTPS
                    Serial.print("deserializeJson() failed: ");
                    Serial.println(error.c_str());
#endif
                    https.end();
                    // delete client;
                    // client->stop();

                    String errorMsg = String(error.c_str());
                    Info_Cashless.Log(RTC, "TOKEN_API_ACCOUNTING", "ERROR_DESERIALIZANDO_OBJETO_RESPONSE: " + errorMsg);
                    return Output; /* Vacio*/
                }
                else
                {
                    // Token Generado;

                    String Token = doc["Data"]["access_token"];
                    int hour = doc["Data"]["expires"]["Hour"];
                    int minutes = doc["Data"]["expires"]["Minutes"];
                    int seconds = doc["Data"]["expires"]["Seconds"];
                    int Token_Expires_Day = doc["Data"]["expires"]["Day"];
                    int Token_Expires_Month = doc["Data"]["expires"]["Month"];
                    int Token_Expires_Year = doc["Data"]["expires"]["Year"];
                    bool IsSuccess = doc["IsSuccess"];
                    // String Message = doc["Message"];
                    // int Evento = doc["Evento"];
                    if (!doc.containsKey("IsSuccess"))
                    {
                        IsSuccess = false;
                    }

                    if (IsSuccess)
                    {

                        // #ifdef Debug_HTTPS
                        Serial.println("Token Generador! OK");
                        // #endif

#ifdef Debug_HTTPS
                        Serial.println(Token_Expires_Day);
                        Serial.println(Token_Expires_Month);
                        Serial.println(Token_Expires_Year);
#endif

                        Set_DateTime_Expires_Token(Token_Expires_Day, Token_Expires_Month, Token_Expires_Year);
                        Init_Access_Token(Token); /* Inicia Token de Acceso */
                        Variables_globales.Set_Variable_Global(Token_Valido_Generado, true);
                        doc.clear();
                        https.end();
                        // delete client;
                        // client->stop();

                        Info_Cashless.Log(RTC, "TOKEN_API_ACCOUNTING", "TOKEN_GENERADO_EXPIRACION: " + String(Token_Expires_Day) + ":" + String(Token_Expires_Month) + ":" + String(Token_Expires_Year));
                        return Output = Token;
                    }
                    else
                    {
                        doc.clear();

                        https.end();
                        // delete client;
                        // client->stop();

                        Info_Cashless.Log(RTC, "TOKEN_API_ACCOUNTING", "ERROR_GENERANDO_TOKEN_IsSuccess_false");
                        return Output;
                    }
                    doc.clear();
                }
                https.end();
                // delete client;
                // client->stop();
                return Output;
            }
            else
            {

#ifdef Debug_HTTPS
                Serial.print("error in downloading version file:");
                Serial.println(httpCode);
#endif

                https.end();
                // delete client;
                // client->stop();
                Info_Cashless.Log(RTC, "TOKEN_API_ACCOUNTING", "FALLA_EN_PETICION_HTTP_CODIGO: " + String(httpCode));
                return Output; /* Vacio*/
            }
            https.end();
        }
        else
        {
            Info_Cashless.Log(RTC, "TOKEN_API_ACCOUNTING", "NO_SE_ESTABLECIO_CONEXION_CON: " + String(fwurl));
            https.end();
        }
        // delete client;
        // client->stop();
    }
    else
    {
#ifdef Debug_HTTPS
        Serial.print("No conectado a la red WiFi");
#endif
        Info_Cashless.Log(RTC, "TOKEN_API_ACCOUNTING", "NO_CONECTADO_A_LA_RED_WIFI");
        return Output;
    }
    return Output; /* Vacio*/
}

/*-----------------------------------------------------------------------------------------------------------*/
String API_Gmaster::Get_Controlador_Api(void)
{
    return Controlador_Principal;
}
String API_Gmaster::Get_Access_Token_String()
{
    return Access_Token_Api_Gmaster;
}
/*-----------------------------------------> Utilidades <----------------------------------------------------*/
bool API_Gmaster::Init_Access_Token(String Access_Token)
{
    Access_Token_Api_Gmaster=Access_Token;
    
    if( Access_Token_Api_Gmaster=="")
        return false;
    else
        return true;
}

bool API_Gmaster::Init_Controlador_Principal(String Api_Controlador)
{
    Controlador_Principal = Api_Controlador;

    if (Controlador_Principal == "")
        return false;
    else
        return true;
}

void Inicializa_Buffer_Eventos_Api(void)
{
    unsigned short i;
    unsigned char j;

    for (i = 0; i < 999; i++)
    {
        for (j = 0; j < 8; j++)
        {
            Tabla_Eventos_[i][j] = 0xFF;
        }
    }
    Ptr_Eventos_Marca_Temp = 0x00;
    Ptr_Eventos_Marca = 0x00;
    Num_Eventos = 0x0000;
}

void API_Gmaster:: Marca_Eventos_Api(void)
{
    if(Ptr_Eventos_Marca_Temp>=MAX_LIM_EVENTOS_Api)
    {
        Inicializa_Buffer_Eventos_Api();
    }else{
        Ptr_Eventos_Marca = Ptr_Eventos_Marca_Temp;
    }
}

/* Dia, Mes, año, Hora, Minutos,segundos, limite */
bool API_Gmaster::Set_Event_Time(int Day_Evento, int Month_Evento, int Year_Evento, int Hour_Evento, int Minutes_Evento,int Seconds_Evento,int Limite)
{
    Month_Evento=Month_Evento-1; 
    int Current_Day = RTC.getDay();
    int Current_Month = RTC.getMonth();
    int Current_Year = RTC.getYear();

    int Current_Hour=RTC.getHour();
    int Current_Minutes=RTC.getMinute();

    if(Day_Evento==0&&  Month_Evento==0&&Year_Evento==0 && Year_Evento==0&& Hour_Evento==0 && Minutes_Evento==0 && Seconds_Evento==0)
        return true;

    if(Current_Day>Month_Evento)
        return true;

    if(Day_Evento==Current_Day&&  Month_Evento==Current_Month&&Year_Evento==Current_Year && Current_Hour <= Hour_Evento && (Hour_Evento - Current_Hour) * 60 + (Minutes_Evento - Current_Minutes) <= 60)
        return true;
    else
        return false;


    return false;
}

bool API_Gmaster::Set_DateTime_Expires_Token(int Day,int Month,int Year)
{
    DateTime_Expires[0]=Day;
    DateTime_Expires[1]=Month;
    DateTime_Expires[2]=Year;
    

    if(DateTime_Expires[0]!=Day||DateTime_Expires[1]!=Month||DateTime_Expires[2]!=Year)
        return false;
    return true;
}

int * API_Gmaster::Get_DateTime_Expires_Token(void)
{
    return DateTime_Expires;
}
/*-------------------------------------------------------------------------------------------------------------*/
// fwurl =Controlador_Principal+Api+"?"+ "Id="+Configuracion.Get_Configuracion(Id_Maquina, "Id_Maquina");
void Recycl()
{
    // String Comando_Contadores=String(Command);
    // String Contador_Cancel_Credit=String(contadores.Get_Contadores_Char(Total_Cancel_Credit));
    // String Contador_Coin_In=String(contadores.Get_Contadores_Char(Coin_In));
    // String Contador_Coin_Out=String (contadores.Get_Contadores_Char(Coin_Out));
    // String Contador_Total_Driop=String (contadores.Get_Contadores_Char(Total_Drop));
    // String Contador_Jackpot=String (contadores.Get_Contadores_Char(Jackpot));
    // String Contador_Physical_Coin_In=String(contadores.Get_Contadores_Char(Physical_Coin_In));
    // String Contador_Physical_Coin_Out=String(contadores.Get_Contadores_Char(Physical_Coin_Out));
    // String Contador_Total_Coin_Drop=String(contadores.Get_Contadores_Char(Total_Coin_Drop));
    // String Contador_Machine_Paid_Progresive_Payout=String(contadores.Get_Contadores_Char(Machine_Paid_Progresive_Payout));
    // String Contador_Machine_Paid_External_Bonus_Payout=String(contadores.Get_Contadores_Char(Machine_Paid_External_Bonus_Payout));

    // String Contador_Attendant_Paid_Progresive_Payout=String(contadores.Get_Contadores_Char(Attendant_Paid_Progresive_Payout));
    // String Contador_Ticket_In=String(contadores.Get_Contadores_Char(Ticket_In));
    // String Contador_Ticket_Out=String(contadores.Get_Contadores_Char(Ticket_Out));
    // String Contador_Cancel_Credit_Hand_Pay=String(contadores.Get_Contadores_Char(Cancel_Credit_Hand_Pay));
    // String Contador_Bill_Amount=String(contadores.Get_Contadores_Char(Bill_Amount));

    // String Contador_Games_Since_Last_Power_Up=String(contadores.Get_Contadores_Char(Games_Since_Last_Power_Up));
    // String Contador_Games_Played=String(contadores.Get_Contadores_Char(Games_Played));
    // String Contador_Door_Open=String(contadores.Get_Contadores_Char(Door_Open));
    // String Contador_Current_Credits=String(contadores.Get_Contadores_Char(Current_Credits));

    // String IP_Tarjeta="00000"

    // Serial.println(Comando_Contadores+"|"+Contador_Cancel_Credit+"|"+Contador_Coin_In+"|"+Contador_Coin_Out);

    
    

   // Serial.println(Json_Contadores);   
    // StaticJsonDocument<1024> jsonDocument;

    // /* --------------------------------> Convierte contadores en Json <------------------------------------------ */

    // jsonDocument["Total_Cancel_Credit"] = contadores.Get_Contadores_Char(Total_Cancel_Credit);
    // jsonDocument["Coin_In"] = contadores.Get_Contadores_Char(Coin_In);
    // jsonDocument["Coin_Out"] = contadores.Get_Contadores_Char(Coin_Out);

    // if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 4)
    //     jsonDocument["Total_Drop"] = contadores.Get_Contadores_Char(Physical_Coin_In);
    // else
    //     jsonDocument["Total_Drop"] = contadores.Get_Contadores_Char(Total_Drop);

    // jsonDocument["Jackpot"] = contadores.Get_Contadores_Char(Jackpot);
    // jsonDocument["Physical_Coin_In"] = contadores.Get_Contadores_Char(Physical_Coin_In);
    // jsonDocument["Physical_Coin_Out"] = contadores.Get_Contadores_Char(Physical_Coin_Out);

    // jsonDocument["Total_Coin_Drop"] = contadores.Get_Contadores_Char(Total_Coin_Drop);
    // jsonDocument["Machine_Paid_Progresive_Payout"] = contadores.Get_Contadores_Char(Machine_Paid_Progresive_Payout);
    // jsonDocument["Machine_Paid_External_Bonus_Payout"] = contadores.Get_Contadores_Char(Machine_Paid_External_Bonus_Payout);
    // jsonDocument["Attendant_Paid_Progresive_Payout"] = contadores.Get_Contadores_Char(Attendant_Paid_Progresive_Payout);

    // jsonDocument["Attendant_Paid_External_Bonus_Payout"] = contadores.Get_Contadores_Char(Attendant_Paid_External_Bonus_Payout);

    // jsonDocument["Ticket_In"] = contadores.Get_Contadores_Char(Ticket_In);
    // jsonDocument["Ticket_Out"] = contadores.Get_Contadores_Char(Ticket_Out);

    // if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 4 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 10)
    //     jsonDocument["Cancel_Credit_Hand_Pay"] = contadores.Get_Contadores_Char(Total_Cancel_Credit);
    // else
    //     jsonDocument["Cancel_Credit_Hand_Pay"] = contadores.Get_Contadores_Char(Cancel_Credit_Hand_Pay);

    // jsonDocument["Bill_Amount"] = contadores.Get_Contadores_Char(Bill_Amount);
    // jsonDocument["Games_Since_Last_Power_Up"] = contadores.Get_Contadores_Char(Games_Since_Last_Power_Up);
    // jsonDocument["Games_Played"] = contadores.Get_Contadores_Char(Games_Played);

    // jsonDocument["Door_Open"] = contadores.Get_Contadores_Char(Door_Open);
    // jsonDocument["Current_Credits"] = contadores.Get_Contadores_Char(Current_Credits);

    // jsonDocument["IP_Tarjeta"] = "00000";
    // jsonDocument["Bytes_Libres"] = "0000000000";
    // jsonDocument["Serie_Trama"] = contadores.Get_Contadores_Char(Serie_Trama);
    // jsonDocument["ID_Jugador"] = String(contadores.Get_Client_ID()[0]-48)+String(contadores.Get_Client_ID()[1]-48)+String(contadores.Get_Client_ID()[2]-48)+String(contadores.Get_Client_ID()[3]-48)+String(contadores.Get_Client_ID()[4]-48)+String(contadores.Get_Client_ID()[5]-48)+String(contadores.Get_Client_ID()[6]-48)+String(contadores.Get_Client_ID()[7]-48);


    // String Mes;
    // int month = RTC.getMonth();
    // String Fecha = RTC.getDate();
    // switch (month)
    // {
    // case 0:
    //     Mes = "01";
    //     break;
    // case 1:
    //     Mes = "02";
    //     break;
    // case 2:
    //     Mes = "03";
    //     break;
    // case 3:
    //     Mes = "04";
    //     break;
    // case 4:
    //     Mes = "05";
    //     break;
    // case 5:
    //     Mes = "06";
    //     break;
    // case 6:
    //     Mes = "07";
    //     break;
    // case 7:
    //     Mes = "08";
    //     break;
    // case 8:
    //     Mes = "09";
    //     break;
    // case 9:
    //     Mes = "10";
    //     break;
    // case 10:
    //     Mes = "11";
    //     break;
    // case 11:
    //     Mes = "12";
    //     break;
    // default:
    //     break;
    // }
    // jsonDocument["Hora_Fecha"] = RTC.getTime() + "|"+ String(Fecha[9])+String(Fecha[10])+"|"+ Mes+"|"+ String(Fecha[14])+String (Fecha[15]);

    // jsonDocument["Maquina_activa"] = Variables_globales.Get_Variable_Global(Flag_Maquina_En_Juego);
    // jsonDocument["ID_Operador"] = String (contadores.Get_Operador_ID()[0]-48)+ String (contadores.Get_Operador_ID()[1]-48)+String (contadores.Get_Operador_ID()[2]-48)+String (contadores.Get_Operador_ID()[3]-48)+String (contadores.Get_Operador_ID()[4]-48)+String (contadores.Get_Operador_ID()[5]-48)+String (contadores.Get_Operador_ID()[6]-48)+String (contadores.Get_Operador_ID()[7]-48);

    // jsonDocument["CRC"] = "99";

    // serializeJson(jsonDocument, Json_Contadores);

    /*---------------------------------------------------------------------------------------*/
   // Serial.println(Json_Contadores);

//     /* -----------------------------------------> URL<---------------------------------------*/
//     fwurl = URL_API_Gmaster + "?" + "Contadores=" + Json_Contadores;
//     /*---------------------------------------------------------------------------------------*/

//     /*
//     Agregar URL Datos confirmacion */
//     // #ifdef Debug_HTTPS
//     //  Serial.println(fwurl);
//     // #endif
//     WiFiClient *client = new WiFiClient;

//     if (client)
//     {

//         HTTPClient https;

//         if (https.begin(*client, fwurl))
//         {
//             https.addHeader("Authorization", "Bearer " + String(Access_Token_Api_Gmaster)); // Agrega el token de autorización
// #ifdef Debug_HTTPS
//             Serial.print("[HTTPS] GET...\n");
// #endif
//             httpCode = https.GET();
//             if (httpCode == HTTP_CODE_OK)
//             {
// #ifdef Debug_HTTPS
//                 Serial.println("Transmite Evento");
// #endif
//                 return true;
//             }
//             else
//             {
// #ifdef Debug_HTTPS
//                 Serial.print("No se pudo enviar Evento");
//                 Serial.println(httpCode);
// #endif
//                 return false;
//             }
//             https.end();
//         }
//         client->stop();
//         delete client;
//     }
}