#include <esp_now.h>
#include <Arduino.h>
#include "ArduinoJson.h"
#include "Clase_Variables_Globales.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "Configuracion.h"
#include "Pantalla_TFT.h"
#include "RFID.h"
#include <WebServer.h>
#include <HTTPClient.h>
#include <vector>

#define INIT_PLAYER_TRACKING  0
#define CLOSE_PLAYER_TRACKING 1
#define UPDATE_POINTS         2
#define PING                  3
#define RETURN_SESION         4
#define LECTURA_TARJETA       5
#define SINCRO                10
#define COMANDO_NO_IDENTIFICADO 100
#define UPDATE_TFT            16
#define  DEBUG_TFT



std::vector<String> mensajes;
int cantidad_mensajes = 0;

extern volatile bool flag_ReiniciarEspNow;
WebServer ServerUpdate(8080);

extern Pantalla_TFT DisplayTFT;

extern std::string IP_toString_(char IP_Char[]);
extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales
extern Configuracion_ESP32 Configuracion;


unsigned long lastReconnectAttempt = 0;
unsigned long lastReconnectAttemptConexion = 0;
const unsigned long RECONNECT_INTERVAL = 10000; // cada 10 segundos


unsigned long Current_Failed_TFT;
unsigned long Max_Failed_TFT;



volatile bool ultimoEnvioExitoso = true;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

bool Flag_Conexion_TFT=false;
bool Flag_Status_Init_Player_TFT=false;
bool Flag_Status_Close_Player_TFT=false;

bool Flag_Sincro_TFT=false;
bool Flag_Desincro_TFT=false;
bool Flag_Config_TFT=false;
bool Flag_Update_TFT=false;

int Tipo_TFT=TFT_UNKNOW;

bool Flag_Descarga_TFT=false;
bool Flag_Borrar_TFT=false;

char Tipo_Tarjeta_RFID_TFT='R';
int Id_Cliente_TFT_Display=0;

typedef struct struct_message
{
    char json_data[250];
} struct_message;

struct_message incomingData;

/* MAC Destino */

unsigned long Timeout_Conexion_TFT_Inicial=0;
unsigned long Timeout_Conexion_TFT_Final=0;
bool Enable_Conexion=false;


//uint8_t broadcastAddress[] = {0x48, 0xCA, 0x43, 0x32, 0xD6, 0xA8};
extern uint8_t Address_Device_TFT_Display[];
esp_now_peer_info_t peerInfo;

bool Send_TFT(uint8_t MAC[], uint8_t *Data, int len, int MaxIntentos)
{

    for (int i = 0; i < MaxIntentos; i++)
    {
        esp_err_t result = esp_now_send(MAC, Data, len);
        delay(50);

        if (result == ESP_OK && ultimoEnvioExitoso)
        {
            // Serial.println("Envio OK");
            return true;
        }
    }

    return false;
}

/* Reset de pantalla TFT*/
bool Reset_TFT(void)
{
    if (Variables_globales.Get_Variable_Global(Status_Device_TFT_Display) && Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
    {
        StaticJsonDocument<200> doc;

        String Payload = "";
        int Intentos_Conexion = 6;
        doc["IsSuccess"] = true;
        doc["Opcion"] = REESTART_TFT;

        serializeJson(doc, Payload);

        for (int i = 0; i < Intentos_Conexion; i++)
        {
            esp_err_t result = esp_now_send(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length());

            if (result == ESP_OK)
                return true;
            else
                return false;
        }
    }
    return false;
}

bool Await_ms(bool (*condicion)(), unsigned long timeout_ms) {
    unsigned long inicio = millis();
    while (!condicion() && (millis() - inicio < timeout_ms)) {


        #ifdef DEBUG_TFT
        //Serial.println(" Esperando respuesta  MAC.....");
        #endif
        vTaskDelay(10);  // No bloquea demasiado la CPU
    }
    return condicion();  // Retorna true si se cumplió la condición, false si fue timeout
}

bool get_Flag_Conexion_TFT() {
    return Flag_Conexion_TFT;
}

bool get_Flag_Sincro_TFT(void)
{
    return Flag_Sincro_TFT;
}

bool get_Flag_Desincro_TFT(void)
{
    return Flag_Desincro_TFT;
}

bool get_Flag_Config_TFT(void)
{
    return Flag_Config_TFT;
}


bool get_Flag_Update_TFT(void)
{
    return  Flag_Update_TFT;
}
/*  Inicializa pantall TFT utilizando el protocolo inalambrico ESP-NOW*/

bool get_Flag_Descarga_TFT(void)
{
    return Flag_Descarga_TFT;
}

bool get_Flag_Borrar_TFT(void)
{
    return Flag_Borrar_TFT;
}

int get_Tipo_TFT(void)
{
    return Tipo_TFT;
}

bool Stop_TFT_Display(void)
{
    esp_err_t err = esp_now_deinit();

    if (err == ESP_OK || err == ESP_ERR_ESPNOW_NOT_INIT)
        return true;
    else
        return false;
}

void Check_TFT_Reconnect(unsigned long Timeout)
{
    if (Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
    {

        if(flag_ReiniciarEspNow)
        {
            if(Init_TFT_Display(true, Address_Device_TFT_Display))
                flag_ReiniciarEspNow=false;
            delay(50);
        }

        if (!Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
        {
            unsigned long currentMillis = millis();

            if (currentMillis - lastReconnectAttempt >= Timeout)
            {
                lastReconnectAttempt = currentMillis;
                #ifdef DEBUG_TFT
                Serial.println("♻️ Intentando reconectar pantalla TFT...");
                #endif
                bool Result = Init_TFT_Display();

                if (Result)
                {
                    #ifdef DEBUG_TFT
                    Serial.println("📺 Pantalla TFT Wireless Inicializada...✅");
                    #endif
                }
                    
                else
                {
                    #ifdef DEBUG_TFT
                    Serial.print("Dispositivo no encontrado: ");
                    #endif
                }
                    
            }
        }

        unsigned long currentMillisConexion = millis();

        if ((currentMillisConexion - lastReconnectAttemptConexion) >= Timeout)
        {
            lastReconnectAttemptConexion = currentMillisConexion;
            StaticJsonDocument<500> doc;

            String Payload;
            int Intentos_Conexion = 3;
            doc["IsSuccess"] = true;
            doc["Opcion"] = PING;

            serializeJson(doc, Payload);

            bool Issuccess = Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length());
            if (Issuccess)
            {
                #ifdef DEBUG_TFT
                Serial.println("Verifica conexion pantalla OK");
                #endif
                Variables_globales.Set_Variable_Global(Conexion_TFT_Display, true);
            }else{
                Current_Failed_TFT++;
            }
           
        }
    }
}

bool Init_TFT_Display(bool EspNow, uint8_t MAC[6])
{
    /* Pregunta si existe un dispositivo sincronizado */

    if (EspNow)
    {
        esp_err_t err = esp_now_init();
        if (err == ESP_OK || err == ESP_ERR_ESPNOW_EXIST)
        {

            if(MAC==nullptr)
            {
                #ifdef DEBUG_TFT
                Serial.println("No existe MAC para sincronizacion temporal");
                #endif
                return false;
            }

            esp_now_register_recv_cb(OnDataRecv);
            
            memcpy(peerInfo.peer_addr, MAC, 6);
            peerInfo.channel = WiFi.channel();
            peerInfo.encrypt = false;

            if (esp_now_add_peer(&peerInfo) == ESP_OK || esp_now_add_peer(&peerInfo) == ESP_ERR_ESPNOW_EXIST)
                return true;
            else
            {
                #ifdef DEBUG_TFT
                Serial.println("Error Emparejando el dispositivo");
                #endif
                return false;
            }
        }
        #ifdef DEBUG_TFT
        Serial.println("Error Inicializing  esp Now");
        #endif
        return false;
    }
    else
    {

        if (Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
        {
            if (esp_now_init() != ESP_OK)
            {
#ifdef DEBUG_TFT
                Serial.println("Error Inicializing  esp Now");
#endif
                Variables_globales.Set_Variable_Global(Conexion_TFT_Display, false);

                return false;
            }
            else
            {
                /* Registra Callback recepcion ESPNOW*/
                esp_now_register_recv_cb(OnDataRecv);
                esp_now_register_send_cb(OnDataSent);
#ifdef DEBUG_TFT
                // for (int i = 0; i < 6; i++)
                // {

                //     Serial.print(broadcastAddress[i], HEX);
                //     Serial.print(":");
                // }
                // Serial.println();
#endif
                memcpy(peerInfo.peer_addr, Address_Device_TFT_Display, 6);
                peerInfo.channel = WiFi.channel();
                peerInfo.encrypt = false;


                if(esp_now_add_peer(&peerInfo) == ESP_OK||esp_now_add_peer(&peerInfo) == ESP_ERR_ESPNOW_EXIST)
                {

                    Flag_Conexion_TFT = false;
                    StaticJsonDocument<500> doc;

                    String Payload;
                    int Intentos_Conexion = 3;
                    doc["IsSuccess"] = true;
                    doc["Opcion"] = PING;

                    serializeJson(doc, Payload);
                    
                   
                    
                    bool Issuccess=Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length());
                    /* Conexion de pantalla OK */

                    if (Await_ms(get_Flag_Conexion_TFT, 1000) || Issuccess)
                    {
                        Variables_globales.Set_Variable_Global(Conexion_TFT_Display, true);
                        //#ifdef DEBUG_TFT
                        Serial.println("📺 Pantalla TFT Wireless Inicializada...✅");
                        //#endif
                        Flag_Conexion_TFT = false;

                        Menssage_TFT("Estableciendo conexion con dispositivo Globus IM...",2500,true);

                        if(ConsultarBanners())
                            EnviarBannersPorEspNow();
                        

                        return true;
                    }
                    else
                    {
                        Variables_globales.Set_Variable_Global(Conexion_TFT_Display, false);
#ifdef DEBUG_TFT

                        Serial.print("Dispositivo no encontrado: ");
                        for (int i = 0; i < 6; i++)
                        {
                            Serial.print(Address_Device_TFT_Display[i], HEX);
                            Serial.print(":");
                        }
                        Serial.println();
#endif
                        Flag_Conexion_TFT = false;
                        return false;
                    }
                }
                else
                {

                    Serial.println(esp_now_add_peer(&peerInfo));
#ifdef DEBUG_TFT
                    Serial.println("Error Emparejando el dispositivo");
#endif
                    Variables_globales.Set_Variable_Global(Conexion_TFT_Display, false);
                    return false;
                }
            }
        }
        else
            return false;
    }

    return false;
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
        Init_Player_TFT("Jose Manuel Ordonez ", 10000, 50000, 0,5000,50000,100000,cont_puntos,cont_Puntos_Boletas);
        Compu=ComInicial;
        Serial.println("Ejecuto UUpdate display");
        cont_puntos++;
        cont_Puntos_Boletas++;    
    }
    
}

String formatearComoMoneda(uint32_t numero)
{
    String resultado = "";
    String numStr = String(uint32_t(numero/DisplayTFT.Get_DenoCashless()));
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

bool Init_Player_TFT(String User_Name, int Total_Playertracking_Points, int Total_Points_Tickets, int User_Level, uint32_t Saldo_Canjeable, uint32_t Saldo_Sin_Restriccion, uint32_t Saldo_No_Canjeable, int Current_Playertracking_Points, int Current_Points_Tickets,int Tipo_Sesion)
{

    if (Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
    {
        StaticJsonDocument<1024> doc;
        doc.clear();
        String Payload = "";
        int Intentos_Conexion = 3;

        doc["Nombre"] = User_Name;
        doc["Total_Fide"] = Total_Playertracking_Points;
        doc["Total_Bole"] = Total_Points_Tickets;

        doc["Actual_Bole"] = Current_Points_Tickets;
        doc["Actual_Fide"] = Current_Playertracking_Points;

        doc["User_Level"] = User_Level;

        

        doc["Saldo_Canje"] = formatearComoMoneda(Saldo_Canjeable);
        doc["Saldo_Sin_Restri"] = formatearComoMoneda(Saldo_Sin_Restriccion);
        doc["Saldo_No_Canje"] = formatearComoMoneda(Saldo_No_Canjeable);
        doc["Tipo_S"] = Tipo_Sesion;
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
    else
        return false;
}

bool Close_Player_TFT(uint32_t Saldo_Canjeable, uint32_t Saldo_Sin_Restriccion, uint32_t Saldo_No_Canjeable, int Tipo_Sesion)
{

    DisplayTFT.Reset_Nombre_Cliente();

    if (Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
    {
        StaticJsonDocument<200> doc;

        String Payload = "";
        int Intentos_Conexion = 3;
        doc["User_Name"] = "";
        doc["Playertracking_Points"] = 0;
        doc["Points_Tickets"] = 0;
        doc["User_Level"] = 0;

        doc["Saldo_Canje"] = formatearComoMoneda(Saldo_Canjeable);
        doc["Saldo_Sin_Restri"] = formatearComoMoneda(Saldo_Sin_Restriccion);
        doc["Saldo_No_Canje"] = formatearComoMoneda(Saldo_No_Canjeable);

        doc["Tipo_S"] = Tipo_Sesion;
        doc["Opcion"] = CLOSE_PLAYER_TRACKING;

        serializeJson(doc, Payload);

        for (int i = 0; i < Intentos_Conexion; i++)
        {
            if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
                return true;
            else
                return false;
        }
    }else{
        return false;
    }
}

bool Menssage_TFT(String Message, int timeout, bool IsSuccess)
{

    if (Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
    {
        StaticJsonDocument<500> doc;
        doc.clear();
        String Payload = "";
        int Intentos_Conexion = 3;

        doc["Message"] = Message;
        doc["Timeout"] = timeout;
        doc["IsSuccess"] = IsSuccess;
        doc["Opcion"] = RETURN_SESION;

        serializeJson(doc, Payload);

        for (int i = 0; i < Intentos_Conexion; i++)
        {
            if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
                return true;
            else
                return false;

            delay(200);
        }
    }else
        return false;
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

    if (Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
    {
        if (memcmp(mac, Address_Device_TFT_Display, 6) != 0)
        {

            #ifdef DEBUG_TFT
            Serial.println("MAC no autorizada, ignorando mensaje.");
            #endif
            return;
        }
    }

    memcpy(&incomingData, incomingDataPtr, len);

    #ifdef DEBUG_TFT
    Serial.println("JSON recibido:");
    Serial.println(incomingData.json_data);
    #endif

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, incomingData.json_data);

    if (!error)
    {
        int i=0;
        bool Issucess = doc["IsSuccess"];
        int Option = doc["Opcion"];
        int Id_Cliente = doc["Id_Cliente"];
        String Tipo_Tarjeta=doc["Tipo_Tarjeta"];

        
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
        
        case LECTURA_TARJETA:
            if(Issucess)
            {

                if (Tipo_Tarjeta == "C")
                {

                    Tipo_Tarjeta_RFID_TFT = 'C';
                }
                else if (Tipo_Tarjeta == "O")
                {
                    Tipo_Tarjeta_RFID_TFT = 'O';
                }
                else
                {
                    Tipo_Tarjeta_RFID_TFT = 'R';
                }
                Id_Cliente_TFT_Display=Id_Cliente;
                Variables_globales.Set_Variable_Global(Lectura_RFD_TFT_Display,true);
            }
            break;

        case  SINCRO:
            if (Issucess)
            {
                if (!doc.containsKey("Tipo_TFT"))
                    Tipo_TFT = TFT_UNKNOW;
                else
                    Tipo_TFT = doc["Tipo_TFT"];

                Flag_Sincro_TFT = true;
            }
            else
                Flag_Sincro_TFT=false;

            #ifdef DEBUG_TFT
            Serial.println("Mensaje confirmacion recibido");
            #endif
        break;

        case DESINCRO:
            if(Issucess)
            {
                Tipo_TFT= TFT_UNKNOW;
                Flag_Desincro_TFT=true;
            }
            else
                Flag_Desincro_TFT=false;
        break;

        case CONFIG:
            if(Issucess)
                Flag_Config_TFT=true;
            else
                Flag_Config_TFT=false;
        break;

        case DOWNLOAD:
            if(Issucess)
                Flag_Descarga_TFT=true;
            else
                Flag_Descarga_TFT=false;
            break;

        case REMOVE_IMG_TFT:
            if(Issucess)
                Flag_Borrar_TFT=true;
            else
                Flag_Borrar_TFT=false;
            break;

        case UPDATE_TFT:
            if(Issucess)
                Flag_Update_TFT=true;
            else
                Flag_Update_TFT=false;
            break;

        default:
            StaticJsonDocument<200> doc;
            doc.clear();
            String Payload = "";
            int Intentos_Conexion = 3;

            doc["Opcion"] = COMANDO_NO_IDENTIFICADO;
            doc["Message"] = "Comando no identificado recibido";

            serializeJson(doc, Payload);

            for (int i = 0; i < Intentos_Conexion; i++)
            {
                if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
                    break;
            }

            break;
        }
        // Serial.println("Nombre: " + userName);
        // Serial.println("Puntos: " + String(points));
        // Serial.println("Nivel: " + String(level));

        // sendConfirmation(mac);
    }
    else
    {

        #ifdef DEBUG_TFT
        Serial.print("Error parsing JSON: ");
        Serial.println(error.c_str());
        #endif
    }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
//   char macStr[18];
//   snprintf(macStr, sizeof(macStr),
//            "%02X:%02X:%02X:%02X:%02X:%02X",
//            mac_addr[0], mac_addr[1], mac_addr[2],
//            mac_addr[3], mac_addr[4], mac_addr[5]);

//   Serial.print("📡 Envío a ");
//   Serial.print(macStr);
//   Serial.print(" -> ");

//   if (status == ESP_NOW_SEND_SUCCESS) {
//     Serial.println("✔️ Enviado correctamente");
//   } else {
//     Serial.println("❌ Falló el envío");
//   }

  ultimoEnvioExitoso = (status == ESP_NOW_SEND_SUCCESS);
}

void Task_Conexion_TFT(int Timeout,int Max_Intentos)
{


    static int Current_Intentos=0;

    if (Variables_globales.Get_Variable_Global(Status_Device_TFT_Display) && Current_Intentos<Max_Intentos)
    {

        if (!Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
        {
            Timeout_Conexion_TFT_Inicial = millis();

            if ((Timeout_Conexion_TFT_Inicial - Timeout_Conexion_TFT_Final) > Timeout)
            {
                Init_TFT_Display();
                Timeout_Conexion_TFT_Final = Timeout_Conexion_TFT_Inicial;
                Current_Intentos++;
            }
        }
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



String Url="http://mi-servidor.com/firmware_tft.bin";

void handleFirmware() {
  WiFiClient client;          // Cliente hacia el esclavo
  HTTPClient http;            // Cliente hacia el servidor central

  Serial.println("Esclavo solicitó firmware, abriendo conexión al servidor...");
  if (http.begin(client, Url)) {
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      // Enviar cabeceras HTTP al esclavo
      ServerUpdate.sendHeader("Content-Type", "application/octet-stream");
      ServerUpdate.sendHeader("Connection", "close");
      ServerUpdate.send(200);

      // Obtener stream del servidor central
      WiFiClient* stream = http.getStreamPtr();

      uint8_t buff[1024];  // buffer de 1 KB
      while (http.connected()) {
        size_t len = stream->available();
        if (len) {
          int c = stream->readBytes(buff, ((len > sizeof(buff)) ? sizeof(buff) : len));
          ServerUpdate.client().write(buff, c);   // reenviar al esclavo
        }
        vTaskDelay(10);
      }
      Serial.println("Firmware enviado al esclavo!");
    } else {
      ServerUpdate.send(500, "text/plain", "Error descargando firmware");
    }
    http.end();
  } else {
    ServerUpdate.send(500, "text/plain", "No se pudo conectar al servidor central");
  }
}


void Init_Server(const char *ssid,const char *password)
{

    WiFi.mode(WIFI_MODE_APSTA);
    WiFi.softAP(ssid, password);


    ServerUpdate.on("/firmware.bin", HTTP_GET, handleFirmware);

    ServerUpdate.begin();
}


void RumUpdateTFT()
{
    ServerUpdate.handleClient();
}




bool ConsultarBanners() { 
    HTTPClient http;
    http.begin("http://192.168.5.110:9595/api/pantalla/banner");
    http.setTimeout(4000);

    int httpCode = http.GET();

    if (httpCode != 200) {
        Serial.println("Error consultando banners");
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    StaticJsonDocument<4096> doc;
    DeserializationError err = deserializeJson(doc, payload);

    if (err) {
        Serial.println("Error parseando JSON");
        return false;
    }

    JsonArray arr = doc["Mensajes"];
    cantidad_mensajes = arr.size();

    mensajes.clear();
    mensajes.reserve(cantidad_mensajes);

    for (int i = 0; i < cantidad_mensajes; i++) {
        mensajes.push_back(arr[i].as<String>());
        Serial.println("Banner recibido:");
        Serial.println(mensajes[i]);
    }

    return true;
}


bool EnviarBannersPorEspNow()
{
    for (int i = 0; i < mensajes.size(); i++)
    {
        StaticJsonDocument<350> doc;
        doc["Opcion"] = BANNER_TFT;
        doc["Index"] = i;
        doc["Total"] = mensajes.size();
        doc["Mensaje"] = mensajes[i];

        String Payload;
        serializeJson(doc, Payload);

        Serial.println(Payload);

        if (Payload.length() > 250)
        {
            Serial.printf("Mensaje %d demasiado largo (%d bytes)\n", 
                          i, Payload.length());
            continue;
        }

        if (!Send_TFT(Address_Device_TFT_Display, 
                      (uint8_t*)Payload.c_str(), Payload.length()))
        {
            Serial.println("Error enviando mensaje");
            return false;
        }
    }

    mensajes.clear();
    mensajes.shrink_to_fit();
    cantidad_mensajes = 0;

    return true;
}

// #define MAX_MENSAJES 5
// #define LONGITUD_MAX_MENSAJE 120

// char mensajes[MAX_MENSAJES][LONGITUD_MAX_MENSAJE];
// int cantidad_mensajes = 0;
// bool EnviarBannersPorEspNow();

// bool ConsultarBanners() { 
//     HTTPClient http;
//     http.begin("http://192.168.5.110:9595/api/pantalla/banner");
//     http.setTimeout(4000);

//     int httpCode = http.GET();

//     if (httpCode != 200) {
//         Serial.println("Error consultando banners");
//         http.end();
//         return false;
//     }

//     String payload = http.getString();
//     http.end();

//     StaticJsonDocument<2048> doc;
//     DeserializationError err = deserializeJson(doc, payload);

//     if (err) {
//         Serial.println("Error parseando JSON");
//         return false;
//     }

//     JsonArray arr = doc["Mensajes"];
//     cantidad_mensajes = arr.size();

//     if (cantidad_mensajes > MAX_MENSAJES) cantidad_mensajes = MAX_MENSAJES;

//     for (int i = 0; i < cantidad_mensajes; i++) {
//         const char* msg = arr[i];
//         strncpy(mensajes[i], msg, LONGITUD_MAX_MENSAJE - 1);
//         mensajes[i][LONGITUD_MAX_MENSAJE - 1] = '\0'; // asegurar fin de string

//         Serial.println("Banner recibido: ");
//         Serial.println(mensajes[i]);
//     }
   
//     return true;
// }

// bool EnviarBannersPorEspNow()
// {

//     for (int i = 0; i < cantidad_mensajes; i++)
//     {

//         int Intentos_Conexion = 1;
//         StaticJsonDocument<250> doc;
//         doc["Opcion"] = BANNER_TFT;
//         doc["Index"] = i;
//         doc["Total"] = cantidad_mensajes;
//         doc["Mensaje"] = mensajes[i];

//         String Payload;
//         serializeJson(doc, Payload);

//         Serial.println(Payload);

//         // Verificar límite ESP-NOW
//         if (Payload.length() > 250)
//         {
//             Serial.printf("Mensaje %d demasiado largo (%d bytes)\n", i, Payload.length());
//             continue;
//         }

//         for (int i = 0; i < Intentos_Conexion; i++)
//         {
//             if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
//                 break;
//         }

//         // esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)Payload.c_str(), Payload.length());

//         // if (result != ESP_OK) {
//         //     Serial.printf("Fallo enviando mensaje %d, error: %d\n", i, result);
//         //     return false;
//         // }
//     }

//     return true;
// }