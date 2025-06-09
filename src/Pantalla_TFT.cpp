#include <esp_now.h>
#include <Arduino.h>
#include "ArduinoJson.h"
#include "Clase_Variables_Globales.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "Configuracion.h"
#include "Pantalla_TFT.h"





#define INIT_PLAYER_TRACKING  0
#define CLOSE_PLAYER_TRACKING 1
#define UPDATE_POINTS         2
#define PING                  3
#define RETURN_SESION         4


extern std::string IP_toString_(char IP_Char[]);
extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales
extern Configuracion_ESP32 Configuracion;




bool Flag_Conexion_TFT=false;
bool Flag_Status_Init_Player_TFT=false;
bool Flag_Status_Close_Player_TFT=false;



typedef struct struct_message
{
    char json_data[250];
} struct_message;

struct_message incomingData;

/* MAC Destino */
uint8_t broadcastAddress[] = {0x34, 0x85, 0x18, 0x71, 0x0C, 0xCC};

extern uint8_t Address_Device_TFT_Display[];

esp_now_peer_info_t peerInfo;



bool Send_TFT(uint8_t MAC[], uint8_t *Data, int len)
{
    esp_err_t result = esp_now_send(MAC, Data, len);
    if (result == ESP_OK)
        return true;
    else
        return false;
}
bool Await_ms(bool (*condicion)(), unsigned long timeout_ms) {
    unsigned long inicio = millis();
    while (!condicion() && (millis() - inicio < timeout_ms)) {

        Serial.println(" Esperando respuesta  MAC.....");
        vTaskDelay(10);  // No bloquea demasiado la CPU
    }
    return condicion();  // Retorna true si se cumplió la condición, false si fue timeout
}
bool get_Flag_Conexion_TFT() {
    return Flag_Conexion_TFT;
}


/*  Inicializa pantall TFT utilizando el protocolo inalambrico ESP-NOW*/
void Init_TFT_Display(void)
{

    /* Pregunta si existe un dispositivo sincronizado */
    if (Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
    {
        if (esp_now_init() != ESP_OK)
        {
            Serial.println("Error Inicializing  esp Now");
            Variables_globales.Set_Variable_Global(Conexion_TFT_Display, false);
        }
        else
        {

            esp_now_register_recv_cb(OnDataRecv);

            for (int i = 0; i < 6; i++)

            {
                Serial.println(broadcastAddress[i]);
            }

            memcpy(peerInfo.peer_addr, broadcastAddress, 6);
            peerInfo.channel = WiFi.channel();
            peerInfo.encrypt = false;

            if (esp_now_add_peer(&peerInfo) != ESP_OK)
            {
                Serial.println("Error Emparejando el dispositivo");
                Variables_globales.Set_Variable_Global(Conexion_TFT_Display, false);
            }
            else
            {
                /* -----------> Test de conexion <--------------*/

                Serial.println("Pantalla TFT  OK");

                StaticJsonDocument<200> doc;

                String Payload = "";
                int Intentos_Conexion = 3;
                doc["IsSuccess"] = true;
                doc["Opcion"] = PING;

                serializeJson(doc, Payload);

                Send_TFT(broadcastAddress, (uint8_t *)Payload.c_str(), Payload.length());
                /* Conexion de pantalla OK */

                if (Await_ms(get_Flag_Conexion_TFT, 1000))
                {
                    Variables_globales.Set_Variable_Global(Conexion_TFT_Display, true);
                    Serial.println("Conexion de pantalla OK");
                }
                else
                    Variables_globales.Set_Variable_Global(Conexion_TFT_Display, false);

                Flag_Conexion_TFT = false;

                /*--------------------------------------------*/
            }
        }
    }
}

unsigned long Compu=0;
unsigned long ComInicial=0;
int cont_puntos=0;
int cont_Puntos_Boletas=0;

void Prueba_TFT(void)
{
    ComInicial=millis();

    if(ComInicial-Compu>=100 && Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
    {
        int Nivel = random(0, 3);
        Init_Player_TFT("Jose Manuel Ordonez ", cont_puntos, cont_Puntos_Boletas, 0,5000,50000,100000);
        Compu=ComInicial;
        Serial.println("Ejecuto UUpdate display");
        cont_puntos++;
        cont_Puntos_Boletas++;    
    }
    
}

String formatearComoMoneda(uint32_t numero)
{
    String resultado = "";
    String numStr = String(numero);
    int len = numStr.length();

    // Insertar puntos cada 3 dígitos desde la derecha
    int contador = 0;
    for (int i = len - 1; i >= 0; i--)
    {
        resultado = numStr[i] + resultado;
        contador++;
        if (contador % 3 == 0 && i != 0)
        {
            resultado = "." + resultado;
        }
    }

    return "$ "+resultado;
}

bool Init_Player_TFT(String User_Name, int Playertracking_Points, int Points_Tickets, int User_Level,uint32_t Saldo_Canjeable, uint32_t Saldo_Sin_Restriccion,uint32_t Saldo_No_Canjeable)
{
    StaticJsonDocument<200> doc;
    doc.clear();
    String Payload = "";
    int Intentos_Conexion = 3;
    doc["User_Name"] = User_Name;
    doc["Playertracking_Points"] = Playertracking_Points;
    doc["Points_Tickets"] = Points_Tickets;
    doc["User_Level"] = User_Level;

    doc["Saldo_Canjeable"]=formatearComoMoneda(Saldo_Canjeable);
    doc["Saldo_Sin_Restriccion"]=formatearComoMoneda(Saldo_Sin_Restriccion);
    doc["Saldo_No_Canjeable"]=formatearComoMoneda(Saldo_No_Canjeable);
    doc["Opcion"] = UPDATE_POINTS;

    serializeJson(doc, Payload);

    for (int i = 0; i < Intentos_Conexion; i++)
    {
        if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
            return true;
        else
            return false;

        delay(200);
    }
}

bool Close_Player_TFT(void)
{
    StaticJsonDocument<200> doc;

    String Payload = "";
    int Intentos_Conexion = 3;
    doc["User_Name"] = "";
    doc["Playertracking_Points"] = 0;
    doc["Points_Tickets"] = 0;
    doc["User_Level"] = 0;
    doc["Opcion"] = CLOSE_PLAYER_TRACKING;

    serializeJson(doc, Payload);

    for (int i = 0; i < Intentos_Conexion; i++)
    {
        if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
            return true;
        else
            return false;
    }
}

bool Get_Status_Sesion_Player(void)
{
    int Intentos_Conexion = 3;
    StaticJsonDocument<200> jsonDocument;
    jsonDocument.clear();

    if (Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless) ||Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
        jsonDocument["IsSuccess"] = true;
    else
        jsonDocument["IsSuccess"] = false;

    
    jsonDocument["Cashless"] = Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless);
    jsonDocument["Player_Tracking"] = Variables_globales.Get_Variable_Global(Flag_Sesion_RFID);
    jsonDocument["Opcion"]=RETURN_SESION;
    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */

    for (int i = 0; i < Intentos_Conexion; i++)
    {
        if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Json.c_str(), Json.length()))
            return true;
        else
            return false;
    }
}

void Ping_Response(void)
{
    StaticJsonDocument<128> doc;
    doc.clear();

    String Payload = "";
    int Intentos_Conexion = 3;
    doc["IsSuccess"] = true;
    doc["Opcion"]=PING;
    serializeJson(doc, Payload);

    for (int i = 0; i < Intentos_Conexion; i++)
    {
        esp_err_t result = esp_now_send(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length());

        if (result == ESP_OK)
            break;
    }
}



void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len)
{

    if (memcmp(mac, broadcastAddress, 6) != 0)
    {
        Serial.println("MAC no autorizada, ignorando mensaje.");
        return;
    }

    memcpy(&incomingData, incomingDataPtr, len);

    Serial.println("JSON recibido:");
    Serial.println(incomingData.json_data);

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, incomingData.json_data);

    if (!error)
    {
        bool Issucess = doc["IsSuccess"];
        int Option = doc["Opcion"];


        switch (Option)
        {
        case PING: /* Conexion de pantalla TFT */
            if(Issucess)
                Flag_Conexion_TFT = true;
            break;

        case UPDATE_POINTS:
            if(Issucess)
                Flag_Status_Init_Player_TFT=true;
            break;

        case CLOSE_PLAYER_TRACKING:
            if(Issucess)
                Flag_Status_Close_Player_TFT=true;
            break;
            

        case RETURN_SESION:
            Get_Status_Sesion_Player();
            break;
        
        default:
            break;
        }
        // Serial.println("Nombre: " + userName);
        // Serial.println("Puntos: " + String(points));
        // Serial.println("Nivel: " + String(level));

        // sendConfirmation(mac);
    }
    else
    {
        Serial.print("Error parsing JSON: ");
        Serial.println(error.c_str());
    }
}

bool Up_Points(void)
{
    bool Code = false;
    int httpCode;
    WiFiClient client;
    HTTPClient https;

    char IP_Server[4];
    char Current_IP[4];
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));

    std::string Ip = IP_toString_(IP_Server);
    std::string maquinaIp = IP_toString_(Current_IP);
    String Ip_Server = String(Ip.c_str());

    String MaquinaIp = String(maquinaIp.c_str());
    String Puerto = "9595";
    String fwurl = "http://" + Ip_Server + ":" + Puerto + "/api/Fidelizacion/Uppuntos";

    https.setTimeout(10000); /* 10seg */
    if (https.begin(client, fwurl))
    {
        httpCode = https.GET();

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
                Serial.println("Error Json deserializeJson");
#endif
            }
            else
            {
                bool IsSuccess = doc["IsSuccess"];

                String userName = doc["User_Name"].as<String>();
                int points_fide = doc["Playertracking_Points"];
                int points_Bole = doc["Points_Tickets"];
                int level = doc["User_Level"];

                if (doc.containsKey("User_Name") && doc.containsKey("Playertracking_Points") && doc.containsKey("Points_Tickets") && doc.containsKey("User_Level"))
                {

                    if (Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
                        Init_Player_TFT(userName, points_fide, points_Bole, level);
                }

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
        return Code;
    }
    return false;
}
