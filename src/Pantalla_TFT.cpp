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

#include "Transacciones.h"
#include "Contadores.h"

#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_event.h"

#include "ESP32Time.h"
#include "time.h"
#include "esp_wifi.h"

#define INIT_PLAYER_TRACKING 0
#define CLOSE_PLAYER_TRACKING 1
#define UPDATE_POINTS 2
#define PING 3
#define RETURN_SESION 4
#define LECTURA_TARJETA 5
#define SINCRO 10
#define COMANDO_NO_IDENTIFICADO_2 100
#define UPDATE_TFT 16
// #define  DEBUG_TFT
#define CONFIG_TFT 18
#define REFRESH_CONFIG 19
#define INIT 23
#define STATUS_SESION 24
#define DASHBOARD 25

#define UPDATE_CASH 20
#define UPDATE_POINTS 21
#define CLOSE_PLAYER_CASHLESS 22
#define UI_INIT 23

#define UPDATE_GLOBUS_IM_PROGRESO 40
#define CALL_SOPORTE              41
#define LOGIN                     42

extern TransaccionCashless AFT;
extern ESP32Time RTC; // Objeto contiene hora y fecha
extern Contadores_SAS contadores; // Objeto contiene contadores maquina

EstadoSistema estadoAnteriorTFT = ESTADO_OK;
volatile LoginTFT StateKeyboard = STATE_LOGIN_IDLE;


std::vector<String> mensajes;
int cantidad_mensajes = 0;

extern volatile bool flag_ReiniciarEspNow;

volatile bool Flag_Recupera = false;

WebServer ServerUpdate(8080);

QueueHandle_t eventQueue;

unsigned long Timeout_Banner_Inicial = 0;
unsigned long Timeout_Banner_Final = 0;
int Timeout_Intv = 5000;

extern Pantalla_TFT DisplayTFT;
extern Cashless_API Info_Cashless;
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

extern String getMacString(uint8_t mac[6]);
extern const char* archivo;
extern void Init_SD(void);

bool Flag_Conexion_TFT = false;
bool Flag_Status_Init_Player_TFT = false;
bool Flag_Status_Close_Player_TFT = false;

bool Flag_Sincro_TFT = false;
bool Flag_Desincro_TFT = false;
bool Flag_Config_TFT = false;
bool Flag_Update_TFT = false;

int Tipo_TFT = TFT_UNKNOW;

bool Flag_Descarga_TFT = false;
bool Flag_Borrar_TFT = false;

char Tipo_Tarjeta_RFID_TFT = 'R';
int Id_Cliente_TFT_Display = 0;

typedef struct struct_message
{
    char json_data[250];
} struct_message;

struct_message incomingData;

/* MAC Destino */

unsigned long Timeout_Conexion_TFT_Inicial = 0;
unsigned long Timeout_Conexion_TFT_Final = 0;
bool Enable_Conexion = false;

// uint8_t broadcastAddress[] = {0x48, 0xCA, 0x43, 0x32, 0xD6, 0xA8};
extern uint8_t Address_Device_TFT_Display[];
esp_now_peer_info_t peerInfo;

bool Send_TFT(uint8_t MAC[], uint8_t *Data, int len, int MaxIntentos)
{

    for (int i = 0; i < MaxIntentos; i++)
    {


        esp_err_t result = esp_now_send(MAC, Data, len);
        delay(100);

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

bool Await_ms(bool (*condicion)(), unsigned long timeout_ms)
{
    unsigned long inicio = millis();
    while (!condicion() && (millis() - inicio < timeout_ms))
    {

#ifdef DEBUG_TFT
// Serial.println(" Esperando respuesta  MAC.....");
#endif
        vTaskDelay(10); // No bloquea demasiado la CPU
    }
    return condicion(); // Retorna true si se cumplió la condición, false si fue timeout
}

bool get_Flag_Conexion_TFT()
{
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
    return Flag_Update_TFT;
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

String MacArrayToString(const uint8_t *mac, uint8_t length = 6)
{
    String macStr = "";

    for (uint8_t i = 0; i < length; i++)
    {
        if (mac[i] < 0x10)
            macStr += "0";

        macStr += String(mac[i], HEX);

        if (i < (length - 1))
            macStr += ":";
    }

    macStr.toUpperCase();

    return macStr;
}


void Check_TFT_Reconnect(unsigned long Timeout)
{
    if (Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
    {

        if (flag_ReiniciarEspNow)
        {
            if (Init_TFT_Display(true, Address_Device_TFT_Display))
                flag_ReiniciarEspNow = false;
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
            StaticJsonDocument<100> doc;

            String Payload;
            int Intentos_Conexion = 3;
            doc["IsSuccess"] = true;
            
            if(WiFi.status()==WL_CONNECTED)
                doc["EstadoGlo"] = true;
            else
                doc["EstadoGlo"] = false;
            doc["Opcion"] = PING;

            serializeJson(doc, Payload);

            bool Issuccess = Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length());
            if (Issuccess)
            {
#ifdef DEBUG_TFT
                Serial.println("Verifica conexion pantalla OK");
#endif
                Variables_globales.Set_Variable_Global(Conexion_TFT_Display, true);
            }
            else
            {
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

            if (MAC == nullptr)
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
            {

                Flag_Conexion_TFT = false;
                StaticJsonDocument<100> doc;

                String Payload;
                int Intentos_Conexion = 3;
                doc["IsSuccess"] = true;
                bool Status=false;

                if (WiFi.status() == WL_CONNECTED)
                {
                    Status=true;
                    doc["EstadoGlo"] = true;
                }
                else
                {
                    Status=false;
                    doc["EstadoGlo"] = false;
                }
                    

                doc["Opcion"] = PING;

                serializeJson(doc, Payload);

                bool Issuccess = Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length());
                /* Conexion de pantalla OK */

                if (Await_ms(get_Flag_Conexion_TFT, 1000) || Issuccess)
                {

                    Variables_globales.Set_Variable_Global(Conexion_TFT_Display, true);
                    Flag_Conexion_TFT = false;
                    Info_Cashless.Log(RTC, "ESTADO_PANTALLA_TFT", "ACTUALIZA ESTADO DE CONEXION: " +String(Status)+": "+ MacArrayToString(Address_Device_TFT_Display), archivo, INFO_);
                }

                return true;
            }
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

                if (esp_now_add_peer(&peerInfo) == ESP_OK || esp_now_add_peer(&peerInfo) == ESP_ERR_ESPNOW_EXIST)
                {

                    Flag_Conexion_TFT = false;
                    StaticJsonDocument<100> doc;

                    String Payload;
                    int Intentos_Conexion = 3;
                    doc["IsSuccess"] = true;

                    if(WiFi.status()==WL_CONNECTED)
                        doc["EstadoGlo"] = true;
                    else
                        doc["EstadoGlo"] = false;

                    doc["Opcion"] = PING;

                    serializeJson(doc, Payload);

                    bool Issuccess = Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length());
                    /* Conexion de pantalla OK */

                    if (Await_ms(get_Flag_Conexion_TFT, 1000) || Issuccess)
                    {

                        Variables_globales.Set_Variable_Global(Conexion_TFT_Display, true);
                        // #ifdef DEBUG_TFT
                        Serial.println("📺 Pantalla TFT Wireless Inicializada...✅");
                        // #endif
                        Flag_Conexion_TFT = false;

                        Menssage_TFT("Comunicacion establecida con interfaz Globus IM", 2500, true);
                        DisplayTFT.configtft.timeoutSaldosCONFIG =
                            Config_Parameter_TFT(DisplayTFT.configtft.timeoutSaldosCONFIG, DisplayTFT.configtft.timeoutImagenesCONFIG, DisplayTFT.configtft.timeoutCarrucel_MensajesCONFIG, DisplayTFT.configtft.timeoutMensajesCONFIG);

                        if (ConsultarBanners())
                            EnviarBannersPorEspNow();


                        Info_Cashless.Log(RTC,"ESTADO_PANTALLA_TFT","CONEXION ESTABLECIDA: " + MacArrayToString(Address_Device_TFT_Display),archivo,INFO_);

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
                        Info_Cashless.Log(RTC,"ESTADO_PANTALLA_TFT","CONEXION NO ESTABLECIDA: " + MacArrayToString(Address_Device_TFT_Display),archivo,ERROR_);
                        return false;
                    }
                }
                else
                {

                    
#ifdef DEBUG_TFT
                    Serial.println(esp_now_add_peer(&peerInfo));
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

unsigned long Compu = 0;
unsigned long ComInicial = 0;
int cont_puntos = 0;
int cont_Puntos_Boletas = 0;

void Prueba_TFT(void)
{
    ComInicial = millis();

    if (ComInicial - Compu >= 100 && Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
    {
        int Nivel = random(0, 3);
        // Init_Player_TFT("Jose Manuel Ordonez ", 10000, 50000, 0,5000,50000,100000,cont_puntos,cont_Puntos_Boletas);
        Compu = ComInicial;
        Serial.println("Ejecuto UUpdate display");
        cont_puntos++;
        cont_Puntos_Boletas++;
    }
}

String formatearComoMoneda(uint32_t numero)
{
    uint32_t deno = (uint32_t)DisplayTFT.info.DenoCashless;

    if (deno < 1 || deno > 10000)
        return "$ -";

    // ✅ redondeo correcto
    uint32_t Result = (numero + deno / 2) / deno;

    String numStr = String(Result);
    int len = numStr.length();

    String resultado = "";
    resultado.reserve(len + len / 3 + 2); // 🔥 evita realloc

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

    return "$ " + resultado;
}

String LimitarSinCortarPalabras(const String &texto, size_t maxLen)
{
    if (texto.length() <= maxLen)
        return texto;

    // Buscar el último espacio antes del límite
    int lastSpace = texto.lastIndexOf(' ', maxLen);

    // Si no hay espacios, toca cortar normal (palabra gigante)
    if (lastSpace == -1)
        return texto.substring(0, maxLen);

    return texto.substring(0, lastSpace);
}
String PrimerNombre_UltimoApellido(const String &nombreCompleto)
{
    String temp = nombreCompleto;
    temp.trim();

    int firstSpace = temp.indexOf(' ');
    if (firstSpace == -1)
        return temp; // solo un nombre

    // Primer nombre
    String nombre = temp.substring(0, firstSpace);

    // Buscar último espacio para obtener el apellido real
    int lastSpace = temp.lastIndexOf(' ');
    String apellido = temp.substring(lastSpace + 1);

    return nombre + " " + apellido;
}

bool Pantalla_TFT::Reconexion_TFT_(unsigned long Timeout)
{

    /* Existe un dispositivo configurado, no esta  inicializada la pantalla,  y esta inicializado el lector RFID*/

    if (Variables_globales.Get_Variable_Global(Status_Device_TFT_Display) && !Variables_globales.Get_Variable_Global(Conexion_TFT_Display) && Variables_globales.Get_Variable_Global(Conexion_RFID) && Current_Intento_Conexion_TFT < MAX_INTENTOS_CONEXION_TFT)
    {

        if ((millis() - Timeout_Reconexion_TFT) >= Timeout)
        {
            Timeout_Reconexion_TFT = millis();

            if (Init_TFT_Display())
            {
                
                // Report_Http_Code(TFT_NOT_INIT, "📺 Pantalla TFT inicializada correctamente ✅", true);
                //Serial.println();
                Current_Intento_Conexion_TFT = 0;
            }
            else
            {

                if (Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
                {
#ifdef Init_RFID_
                    Serial.println("📺 Pantalla TFT no inicializada ❌");
#endif
                    // Report_Http_Code(TFT_NOT_INIT, "📺 Pantalla TFT no inicializada ❌", false);

                    Current_Intento_Conexion_TFT++;
                    return false;
                }
                else
                {
#ifdef Init_RFID_
                    Serial.println("📺 No exite Pantalla TFT asociada 🚫");
#endif
                    // Report_Http_Code(TFT_NOT_INIT, "📺 No exite Pantalla TFT asociada 🚫", true);
                    Current_Intento_Conexion_TFT = (MAX_INTENTOS_CONEXION_TFT) + 1; // Evita que siga intentando conexion si no existe dispositivo asociado
                    return false;
                }
            }
        }
    }

    return false;
}

bool Pantalla_TFT::Verifica_Conexion_ESPNow(unsigned long Timeout)
{

    static bool IsSuccess = false;

    if (millis() - DisplayTFT.configtft.Timeout_Conexion >= Timeout)
    {
        /* Verifica conexion TFT */

        if (Variables_globales.Get_Variable_Global(Status_Device_TFT_Display) && !Variables_globales.Get_Variable_Global(Conexion_TFT_Display) && Intentos_Conexion<MAX_INTENTOS_CONEXION_TFT)
        {
            /** Si tengo pantalla sincronizada y no esta conectada  */

            Flag_Conexion_TFT = false;
            StaticJsonDocument<100> doc;

            String Payload;
            int Intentos_Conexion = 3;
            doc["IsSuccess"] = true;

            if (WiFi.status() == WL_CONNECTED)
                doc["EstadoGlo"] = true;
            else
                doc["EstadoGlo"] = false;

            doc["Opcion"] = PING;

            serializeJson(doc, Payload);

            bool Issuccess = Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length());
            /* Conexion de pantalla OK */

            if (Await_ms(get_Flag_Conexion_TFT, 1000))
            {

                Variables_globales.Set_Variable_Global(Conexion_TFT_Display, true);
                #ifdef DEBUG_TFT
                Serial.println("📺 Pantalla TFT Wireless Inicializada...✅");
                #endif
                Flag_Conexion_TFT = false;

                Menssage_TFT("Comunicacion establecida con interfaz Globus IM", 2500, true);
                DisplayTFT.configtft.timeoutSaldosCONFIG =
                    Config_Parameter_TFT(DisplayTFT.configtft.timeoutSaldosCONFIG, DisplayTFT.configtft.timeoutImagenesCONFIG, DisplayTFT.configtft.timeoutCarrucel_MensajesCONFIG, DisplayTFT.configtft.timeoutMensajesCONFIG);

                if (ConsultarBanners())
                    EnviarBannersPorEspNow();

                //return true;
                IsSuccess=true;


                Intentos_Conexion=0;

                Info_Cashless.Log(RTC,"ESTADO_PANTALLA_TFT","RECONEXION ESTABLECIDA CON EXITO: " + MacArrayToString(Address_Device_TFT_Display),archivo,INFO_);
            }
            else
            {

                Intentos_Conexion++;
                
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
                //return false;

                IsSuccess=false;

                if(Intentos_Conexion>=MAX_INTENTOS_CONEXION_TFT)
                {
                    Variables_globales.Set_Variable_Global(Conexion_TFT_Display, false);
                    Info_Cashless.Log(RTC,"ESTADO_PANTALLA_TFT","MAXIMO DE INTENTOS DE CONEXION ALCANZADO: " + MacArrayToString(Address_Device_TFT_Display),archivo,ERROR_);
                }
            }
        }
        // if (AFT.GET_STATUS_TRANSFER() == TransaccionCashless::TRANS_IDLE)
        // {
        //     Flag_Conexion_TFT = false;
        //     StaticJsonDocument<100> doc;

        //     String Payload;
        //     int Intentos_Conexion = 3;
        //     doc["IsSuccess"] = true;

        //     if (WiFi.status() == WL_CONNECTED)
        //         doc["EstadoGlo"] = true;
        //     else
        //         doc["EstadoGlo"] = false;

        //     // bool estadoRandom = random(0, 2); // 0 o 1

        //     // if (estadoRandom)
        //     //     doc["EstadoGlo"] = true;
        //     // else
        //     //     doc["EstadoGlo"] = false;

        //     doc["Opcion"] = PING;

        //     serializeJson(doc, Payload);

        //     bool Issuccess = Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length());
        //     /* Conexion de pantalla OK */

        //     if (Await_ms(get_Flag_Conexion_TFT, 1000))
        //     {

        //         Variables_globales.Set_Variable_Global(Conexion_TFT_Display, true);
        //         // #ifdef DEBUG_TFT
        //         //Serial.println("📺 Ping Tft ✅");
        //         IsSuccess=true;
        //     }
        //     else
        //     {

        //         DisplayTFT.configtft.Contador_Conexion++;
        //         IsSuccess=false;
        //     }
        // }

        // if (DisplayTFT.configtft.Contador_Conexion >= MAX_LIMITE_CONEXION_TFT)
        // {
        //     /* Reiniciar comunicacion */

        //     IsSuccess=false;
        //     DisplayTFT.configtft.Contador_Conexion=0;
        // }

        DisplayTFT.configtft.Timeout_Conexion = millis();
    }

    return IsSuccess;
}

/* Ejecuta  transacciones asincronas con la pantalla TFT*/
void Pantalla_TFT::Task_Handle_TFT_Display(int Timeout)
{


    DisplayTFT.Verifica_Conexion_ESPNow();

    if (Variables_globales.Get_Variable_Global(Status_Device_TFT_Display) && Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
    {

        Notify_Status_Machine_Now();
        DisplayTFT.handleTFT();

        /* Verifica_Conexion_ESPNow () */



       

        if ((millis() - TimeoutTaskTFT) > Timeout)
        {

            TimeoutTaskTFT = millis();

            if (!Variables_globales.Get_Variable_Global(Ftp_Mode) && !Variables_globales.Get_Variable_Global(Updating_System))
            {
                DisplayTFT.Task_Banner_TFT(480000);
            }

            // if (Variables_globales.Get_Variable_Global(Flag_Update_TFT_Globus_IM))
            // {
            //     Variables_globales.Set_Variable_Global(Flag_Update_TFT_Globus_IM, false);

            //     DisplayTFT.Actualiza_Puntos_TFT_Globus_IM(DisplayTFT.info.Usuario, DisplayTFT.info.Casino, DisplayTFT.NivelUsuario(DisplayTFT.info.Nivel_Usuario), DisplayTFT.info.Total_Fide, DisplayTFT.info.Total_Bole, DisplayTFT.info.Actual_Fide, DisplayTFT.info.Actual_Bole);
            // }

            if (Flag_Recupera)
            {
                Flag_Recupera = false;

                if (DisplayTFT.Actualiza_Puntos_TFT_Globus_IM(DisplayTFT.info.Usuario, DisplayTFT.info.Casino, DisplayTFT.NivelUsuario(DisplayTFT.info.Nivel_Usuario), DisplayTFT.info.Total_Fide, DisplayTFT.info.Total_Bole, DisplayTFT.info.Actual_Fide, DisplayTFT.info.Actual_Bole))
                {
                }

                StaticJsonDocument<200> doc;
                doc.clear();
                String Payload = "";
                int Intentos_Conexion = 3;

                doc["Opcion"] = DASHBOARD;
                doc["Message"] = "Existe sesion activa en tarjeta";

                serializeJson(doc, Payload);

                Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length());
            }
        }
    }
}

bool Pantalla_TFT::Actualiza_Menu_TFT_Globus_IM(int Option)
{
    bool IsSuccess = false;

    if (Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
    {
        StaticJsonDocument<100> doc;
        doc.clear();
        String Payload = "";
        int Intentos_Conexion = 3;
        int Op;
        String Msg = "";

        switch (Option)
        {
        case DASHBOARD:
            Msg = "Inicia dashboard por inicio de sesion fidelizacion";
            Op = Option;
            break;

        case UI_INIT:
            Msg = "Inicia Main no existe sesion iniciada";
            Op = Option;
            break;

        default:
            Msg = "Inicia dashboard por inicio de sesion fidelizacion";
            Op = Option;
            break;
        }

        doc["Opcion"] = Op;
        doc["Message"] = Msg;

        serializeJson(doc, Payload);

        if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
        {
            IsSuccess = true;
        }
        else
        {
            IsSuccess = false;
        }
    }

    return IsSuccess;
}

int Pantalla_TFT::NivelUsuario(String Nivel)
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

bool Pantalla_TFT::Progreso(int Progress)
{
    StaticJsonDocument<300> doc;
    doc.clear();
    String Payload = "";
    bool IsSuccess = false;
    /*
        CASINO
        SALDO CANJEABLE
        SALDO SIN RESTRICCION
        SALDO RESTRINGIDO
    */
    doc["Progreso"] = Progress;
    doc["Opcion"] = UPDATE_GLOBUS_IM_PROGRESO;

    serializeJson(doc, Payload);

    if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
        IsSuccess = true;
    else
        IsSuccess = false;

    return IsSuccess;
}

bool Pantalla_TFT::Actualiza_Saldos_TFT_Globus_IM(String Casino, uint32_t Saldo_Canjeable, uint32_t Saldo_Sin_Restriccion, uint32_t Saldo_No_Canjeable, bool Sesion)
{

    bool IsSuccess = false;

    if (Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
    {
        StaticJsonDocument<300> doc;
        doc.clear();
        String Payload = "";

        /*
            CASINO
            SALDO CANJEABLE
            SALDO SIN RESTRICCION
            SALDO RESTRINGIDO
        */
        doc["Ca"] = LimitarSinCortarPalabras(Casino, 80);
        doc["SC"] = formatearComoMoneda(Saldo_Canjeable);
        doc["SR"] = formatearComoMoneda(Saldo_Sin_Restriccion);
        doc["SNR"] = formatearComoMoneda(Saldo_No_Canjeable);
        doc["S"] = Sesion;
        doc["Opcion"] = UPDATE_CASH;

        serializeJson(doc, Payload);

        if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
            IsSuccess = true;
        else
            IsSuccess = false;

        return IsSuccess;
    }
    else
        return IsSuccess;
}

bool Pantalla_TFT::Actualiza_Puntos_TFT_Globus_IM(String User_Name, String Casino, int User_Level, float Total_Playertracking_Points, float Total_Points_Tickets, float Current_Playertracking_Points, float Current_Points_Tickets)
{
    bool IsSuccess = false;

    if (Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
    {
        StaticJsonDocument<400> doc;
        doc.clear();
        String Payload = "";

        // Serial.println(Total_Points_Tickets);
        doc["Nc"] = PrimerNombre_UltimoApellido(User_Name);
        doc["Ca"] = LimitarSinCortarPalabras(Casino, 25);
        doc["TF"] = Total_Playertracking_Points;
        doc["TB"] = Total_Points_Tickets;
        doc["AB"] = Current_Points_Tickets;
        doc["AF"] = Current_Playertracking_Points;
        doc["UL"] = User_Level;
        doc["Opcion"] = UPDATE_POINTS;

        serializeJson(doc, Payload);
        // Serial.println(Payload);
        if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
            IsSuccess = true;
        else
            IsSuccess = false;

        return IsSuccess;
    }
    else
        return IsSuccess;
}

bool Pantalla_TFT::Cierra_Sesion_Player_Tracking_TFT_Globus_IM(void)
{
    bool IsSuccess = false;

    if (Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
    {
        StaticJsonDocument<100> doc;
        doc.clear();
        String Payload = "";

        /*
         */
        doc["IsSuccess"] = true;
        doc["Opcion"] = CLOSE_PLAYER_TRACKING;

        serializeJson(doc, Payload);

        // Serial.println(Payload);

        if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
            IsSuccess = true;
        else
            IsSuccess = false;

        return IsSuccess;
    }
    else
        return IsSuccess;
}

bool Pantalla_TFT::Home_TFT_Globus_IM(void)
{
    bool IsSuccess = false;

    if (Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
    {
        StaticJsonDocument<100> doc;
        doc.clear();
        String Payload = "";

        doc["IsSuccess"] = true;
        doc["Opcion"] = UI_INIT;

        serializeJson(doc, Payload);

        // Serial.println(Payload);

        if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
            IsSuccess = true;
        else
            IsSuccess = false;

        return IsSuccess;
    }
    else
        return IsSuccess;
}

bool Pantalla_TFT::Cierra_Sesion_Player_Cashless_TFT_Globus_IM(String Casino, uint32_t Saldo_Canjeable, uint32_t Saldo_Sin_Restriccion, uint32_t Saldo_No_Canjeable)
{
    bool IsSuccess = false;

    if (Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
    {
        StaticJsonDocument<300> doc;
        doc.clear();
        String Payload = "";

        doc["IsSuccess"] = true;
        doc["Ca"] = LimitarSinCortarPalabras(Casino, 25);
        doc["SC"] = formatearComoMoneda(Saldo_Canjeable);
        doc["SR"] = formatearComoMoneda(Saldo_Sin_Restriccion);
        doc["SNR"] = formatearComoMoneda(Saldo_No_Canjeable);
        doc["Opcion"] = CLOSE_PLAYER_CASHLESS;

        serializeJson(doc, Payload);

        // Serial.println(Payload);

        if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
            IsSuccess = true;
        else
            IsSuccess = false;

        return IsSuccess;
    }
    else
        return IsSuccess;
}

// bool Init_Player_TFT(String User_Name, float Total_Playertracking_Points, float Total_Points_Tickets, int User_Level, uint32_t Saldo_Canjeable, uint32_t Saldo_Sin_Restriccion, uint32_t Saldo_No_Canjeable, float Current_Playertracking_Points, float Current_Points_Tickets, int Tipo_Sesion, String Casino)
// {

//     if (Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
//     {

//         bool IsSuccess = false;
//         StaticJsonDocument<1024> doc;
//         doc.clear();
//         String Payload = "";
//         String Payload2 = "";
//         int Intentos_Conexion = 3;

//         if (Tipo_Sesion == PLAYER_TRACKING_CASHLESS)
//         {
//             doc["Nc"] = PrimerNombre_UltimoApellido(User_Name);
//             doc["Ca"] = LimitarSinCortarPalabras(Casino, 25);
//             doc["TF"] = Total_Playertracking_Points;
//             doc["TB"] = Total_Points_Tickets;
//             doc["AB"] = Current_Points_Tickets;
//             doc["AF"] = Current_Playertracking_Points;
//             doc["UL"] = User_Level;
//             doc["Ts"] = Tipo_Sesion;
//             doc["Opcion"] = UPDATE_POINTS;

//             // doc["Saldo_Canje"] = "0";
//             // doc["Saldo_Sin_Restri"] = "0";
//             // doc["Saldo_No_Canje"] = "0";

//             serializeJson(doc, Payload);

//             // Serial.println(Payload);
//             // Serial.println((Payload.length()));

//             for (int i = 0; i < Intentos_Conexion; i++)
//             {
//                 if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
//                     return true;

//                 vTaskDelay(200);
//             }

//             return false;
//         }
//         else
//         {

//             //Serial.println("Cashlesss");

//             /* Informacion ventana cashless  Casino-Saldos */
//             doc["Ca"] = LimitarSinCortarPalabras(Casino, 80);
//             doc["SC"] = formatearComoMoneda(Saldo_Canjeable);
//             doc["SR"] = formatearComoMoneda(Saldo_Sin_Restriccion);
//             doc["SNR"] = formatearComoMoneda(Saldo_No_Canjeable);
//             doc["Ts"] = Tipo_Sesion;
//             doc["Opcion"] = UPDATE_POINTS;

//             serializeJson(doc, Payload);

//             for (int i = 0; i < Intentos_Conexion; i++)
//             {
//                 if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
//                     IsSuccess = true;
//                 else
//                     IsSuccess = false;

//                 vTaskDelay(200);
//             }
//             doc.clear();

//             /* Informacion ventana  Puntos */

//             doc["Nc"] = PrimerNombre_UltimoApellido(User_Name);
//             doc["Ca"] = LimitarSinCortarPalabras(Casino, 25);
//             doc["TF"] = Total_Playertracking_Points;
//             doc["TB"] = Total_Points_Tickets;
//             doc["AB"] = Current_Points_Tickets;
//             doc["AF"] = Current_Playertracking_Points;
//             doc["UL"] = User_Level;
//             doc["Ts"] = Tipo_Sesion;
//             doc["Opcion"] = UPDATE_POINTS;

//             serializeJson(doc, Payload2);

//             // Serial.println(Payload2);
//             // Serial.println((Payload2.length()));

//             for (int i = 0; i < Intentos_Conexion; i++)
//             {
//                 if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload2.c_str(), Payload2.length()))
//                     IsSuccess = true;
//                 else
//                     IsSuccess = false;

//                 vTaskDelay(200);
//             }
//         }

//         // doc["Tipo_S"] = Tipo_Sesion;
//         // doc["Opcion"] = UPDATE_POINTS;

//         // serializeJson(doc, Payload);

//         // Serial.println(Payload);
//         // Serial.println((Payload.length()));

//         // for (int i = 0; i < Intentos_Conexion; i++)
//         // {
//         //     if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
//         //         return true;
//         //     else
//         //         return false;

//         //     delay(200);
//         // }

//         return IsSuccess;
//     }
//     else
//         return false;
// }

// bool Close_Player_TFT(uint32_t Saldo_Canjeable, uint32_t Saldo_Sin_Restriccion, uint32_t Saldo_No_Canjeable, int Tipo_Sesion)
// {
//     if (Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
//     {
//         StaticJsonDocument<800> doc;
//         String Payload = "";

//         doc["SC"] = formatearComoMoneda(Saldo_Canjeable);
//         doc["SR"] = formatearComoMoneda(Saldo_Sin_Restriccion);
//         doc["SNR"] = formatearComoMoneda(Saldo_No_Canjeable);
//         doc["Ts"] = Tipo_Sesion; /*0 cashless 1 fidelizacion  Este parametro se utiliza para saber si debe abrir ventana de saldos o no*/

//         doc["Opcion"] = CLOSE_PLAYER_TRACKING;

//         serializeJson(doc, Payload);

//         if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
//             return true;
//         else
//             return false;
//     }
//     else
//     {
//         return false;
//     }
// }

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

        if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
            return true;
        else
            return false;

        delay(200);
    }
    else
        return false;

    return false;
}

bool Config_Parameter_TFT(uint32_t timeoutSaldos, uint32_t timeoutImagenes, uint32_t timeoutCarrucel_Mensajes, uint32_t timeoutMensajes)
{

    if (Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
    {

        bool IsSuccess = true;
        StaticJsonDocument<500> doc;
        doc.clear();
        String Payload = "";
        int Intentos_Conexion = 3;

        doc["timeoutSaldos"] = timeoutSaldos;
        doc["timeoutImagenes"] = timeoutImagenes;
        doc["timeoutCarrucel_Mensajes"] = timeoutCarrucel_Mensajes;
        doc["timeoutMensajes"] = timeoutMensajes;

        doc["IsSuccess"] = IsSuccess;
        doc["Opcion"] = CONFIG_TFT;

        serializeJson(doc, Payload);

        if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
            return true;
        else
            return false;
    }
    else
        return false;
}

bool Get_Status_Sesion_Player(void)
{
    int Intentos_Conexion = 3;
    StaticJsonDocument<200> jsonDocument;
    jsonDocument.clear();

    if (Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless) || Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
        jsonDocument["IsSuccess"] = true;
    else
        jsonDocument["IsSuccess"] = false;

    jsonDocument["Cashless"] = Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless);
    jsonDocument["Player_Tracking"] = Variables_globales.Get_Variable_Global(Flag_Sesion_RFID);
    jsonDocument["Opcion"] = RETURN_SESION;
    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */

    for (int i = 0; i < Intentos_Conexion; i++)
    {
        if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Json.c_str(), Json.length()))
            return true;
        else
            return false;
    }
    return false;
}

void Ping_Response(void)
{
    StaticJsonDocument<128> doc;
    doc.clear();

    String Payload = "";
    int Intentos_Conexion = 3;
    doc["IsSuccess"] = true;

    if (WiFi.status() == WL_CONNECTED)
        doc["EstadoGlo"] = true;
    else
        doc["EstadoGlo"] = false;

    doc["Opcion"] = PING;
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
        int i = 0;
        bool Issucess = doc["IsSuccess"];
        int Option = doc["Opcion"];
        int Id_Cliente = doc["Id_Cliente"];
        String Tipo_Tarjeta = doc["Tipo_Tarjeta"];

        String Version_App = doc["Version"].as<String>();

        int EventoTFT=doc["Evento"].as<int>();
        String MessageTFT=doc["Message"].as<String>();

        static String UsuarioPantalla;
        static String ContrasenaPantalla;

        switch (Option)
        {
        case PING: /* Conexion de pantalla TFT */
            if (Issucess)
            {
                Flag_Conexion_TFT = true;
                DisplayTFT.info.Version_Firmware_tft = Version_App;
            }
            break;

        case UPDATE_POINTS:
            if (Issucess)
                Flag_Status_Init_Player_TFT = true;
            break;

        case CLOSE_PLAYER_TRACKING:
            if (Issucess)
                Flag_Status_Close_Player_TFT = true;
            break;

        case RETURN_SESION:
            Get_Status_Sesion_Player();
            break;

        case LECTURA_TARJETA:
            if (Issucess)
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
                Id_Cliente_TFT_Display = Id_Cliente;
                Variables_globales.Set_Variable_Global(Lectura_RFD_TFT_Display, true);
            }
            break;

        case SINCRO:
            if (Issucess)
            {
                if (!doc.containsKey("Tipo_TFT"))
                    Tipo_TFT = TFT_UNKNOW;
                else
                    Tipo_TFT = doc["Tipo_TFT"];

                Flag_Sincro_TFT = true;
            }
            else
                Flag_Sincro_TFT = false;

#ifdef DEBUG_TFT
            Serial.println("Mensaje confirmacion recibido");
#endif
            break;

        case DESINCRO:
            if (Issucess)
            {
                Tipo_TFT = TFT_UNKNOW;
                Flag_Desincro_TFT = true;
            }
            else
                Flag_Desincro_TFT = false;
            break;

        case CONFIG:
            if (Issucess)
                Flag_Config_TFT = true;
            else
                Flag_Config_TFT = false;
            break;

        case DOWNLOAD:
            if (Issucess)
                Flag_Descarga_TFT = true;
            else
                Flag_Descarga_TFT = false;
            break;

        case REMOVE_IMG_TFT:
            if (Issucess)
                Flag_Borrar_TFT = true;
            else
                Flag_Borrar_TFT = false;
            break;

        case UPDATE_TFT:
            if (Issucess)
                Flag_Update_TFT = true;
            else
                Flag_Update_TFT = false;
            break;

        case REFRESH_CONFIG:
            Config_Parameter_TFT(DisplayTFT.configtft.timeoutSaldosCONFIG, DisplayTFT.configtft.timeoutImagenesCONFIG, DisplayTFT.configtft.timeoutMensajesCONFIG, DisplayTFT.configtft.timeoutMensajesCONFIG);
            break;

        case STATUS_SESION:
            if ((Variables_globales.Get_Variable_Global(Flag_Sesion_RFID) ||
                 Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless)))
            {

                //     Flag_Recupera=true;

                DisplayTFT.Notify_Now(EVENT_RECUPERA_SESION);
            }else{
                Flag_Conexion_TFT = false;
                StaticJsonDocument<100> doc;

                String Payload;
                int Intentos_Conexion = 1;
                doc["IsSuccess"] = true;

                if (WiFi.status() == WL_CONNECTED)
                    doc["EstadoGlo"] = true;
                else
                    doc["EstadoGlo"] = false;

                doc["Opcion"] = PING;

                serializeJson(doc, Payload);

                bool Issuccess = Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length());

                if(Await_ms(get_Flag_Conexion_TFT, 1000))
                {

                    Variables_globales.Set_Variable_Global(Conexion_TFT_Display, true);
                    #ifdef DEBUG_TFT
                    Serial.println("📺 Pantalla TFT Wireless Inicializada...✅");
                    #endif
                    Flag_Conexion_TFT = false;

                    Menssage_TFT("Comunicacion establecida con interfaz Globus IM", 2500, true);
                    DisplayTFT.configtft.timeoutSaldosCONFIG =
                        Config_Parameter_TFT(DisplayTFT.configtft.timeoutSaldosCONFIG, DisplayTFT.configtft.timeoutImagenesCONFIG, DisplayTFT.configtft.timeoutCarrucel_MensajesCONFIG, DisplayTFT.configtft.timeoutMensajesCONFIG);

                    if (ConsultarBanners())
                        EnviarBannersPorEspNow();

                    // return true;
                    Intentos_Conexion = 0;
                }
                else
                {

                    Intentos_Conexion++;
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
                    // return false;
                }
                /* Conexion de pantalla OK */
            }

            break;


        case CALL_SOPORTE:
            DisplayTFT.info.Evento = EventoTFT;
            DisplayTFT.info.Message = MessageTFT;

            DisplayTFT.Notify_Now(CALL_OPERATOR_EVENT);
            break;


        case LOGIN:

            if (Variables_globales.Get_Variable_Global(Flag_Sesion_RFID) || Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless))
            {
                Serial.println(" Ya existe una sesion Activa");
            }
            else
            {
                UsuarioPantalla = doc["Cedula"].as<String>();
                ContrasenaPantalla = doc["Contrasena"].as<String>();

                switch (DisplayTFT.Recibe_Info_Login(UsuarioPantalla, ContrasenaPantalla, true))
                {
                case REQUEST_OK:
                    DisplayTFT.SendLoginResponse(Address_Device_TFT_Display, true, LOGIN, REQUEST_OK);
                    break;

                case BUSY_TFT:
                    /* code */
                    DisplayTFT.SendLoginResponse(Address_Device_TFT_Display, false, LOGIN, BUSY_TFT);
                    break;

                case ERROR_REQUEST:
                    /* code */
                    DisplayTFT.SendLoginResponse(Address_Device_TFT_Display, false, LOGIN, ERROR_REQUEST);
                    break;

                default:
                    DisplayTFT.SendLoginResponse(Address_Device_TFT_Display, false, LOGIN, ERROR_REQUEST);
                    break;
                }
            }

        break;

        default:
            // StaticJsonDocument<200> doc;
            // doc.clear();
            // String Payload = "";
            // int Intentos_Conexion = 3;

            // doc["Opcion"] = COMANDO_NO_IDENTIFICADO_2;
            // doc["Message"] = "Comando no identificado recibido";

            // serializeJson(doc, Payload);

            // for (int i = 0; i < Intentos_Conexion; i++)
            // {
            //     if (Send_TFT(Address_Device_TFT_Display, (uint8_t *)Payload.c_str(), Payload.length()))
            //         break;
            // }

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

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
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

void Task_Conexion_TFT(int Timeout, int Max_Intentos)
{

    static int Current_Intentos = 0;

    if (Variables_globales.Get_Variable_Global(Status_Device_TFT_Display) && Current_Intentos < Max_Intentos)
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

                    // if (Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
                    //     Init_Player_TFT(userName, points_fide, points_Bole, level);
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

String Url = "http://mi-servidor.com/firmware_tft.bin";

void handleFirmware()
{
    WiFiClient client; // Cliente hacia el esclavo
    HTTPClient http;   // Cliente hacia el servidor central

    //Serial.println("Esclavo solicitó firmware, abriendo conexión al servidor...");
    if (http.begin(client, Url))
    {
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK)
        {
            // Enviar cabeceras HTTP al esclavo
            ServerUpdate.sendHeader("Content-Type", "application/octet-stream");
            ServerUpdate.sendHeader("Connection", "close");
            ServerUpdate.send(200);

            // Obtener stream del servidor central
            WiFiClient *stream = http.getStreamPtr();

            uint8_t buff[1024]; // buffer de 1 KB
            while (http.connected())
            {
                size_t len = stream->available();
                if (len)
                {
                    int c = stream->readBytes(buff, ((len > sizeof(buff)) ? sizeof(buff) : len));
                    ServerUpdate.client().write(buff, c); // reenviar al esclavo
                }
                vTaskDelay(10);
            }
            //Serial.println("Firmware enviado al esclavo!");
        }
        else
        {
            ServerUpdate.send(500, "text/plain", "Error descargando firmware");
        }
        http.end();
    }
    else
    {
        ServerUpdate.send(500, "text/plain", "No se pudo conectar al servidor central");
    }
}

void Init_Server(const char *ssid, const char *password)
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

void Pantalla_TFT::Task_Banner_TFT(unsigned long Timeout)
{

    // AFT.STATUS_BA(TransaccionCashless::BA_EN_PROGRESO);

    if (Variables_globales.Get_Variable_Global(Conexion_TFT_Display) && Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
    {
        Timeout_Banner_Inicial = millis();

        if ((Timeout_Banner_Inicial - Timeout_Banner_Final) >= Timeout)
        {

            //(" Desde el task");
            Timeout_Banner_Final = Timeout_Banner_Inicial;
            if (AFT.GET_STATUS_TRANSFER() == TransaccionCashless::TRANS_IDLE && AFT.GET_STATUS_BA() == TransaccionCashless::BA_IDLE)
            {
                if (ConsultarBanners())
                    EnviarBannersPorEspNow();
            }
        }
    }
}

void Pantalla_TFT::handleTFT(void)
{
    // if (DisplayTFT.info.Actualiza_Saldos_Iniciales)
    // {
    //     Menssage_TFT("La sesión se ha iniciado exitosamente.", DisplayTFT.configtft.timeoutMensajesCONFIG, true);
    //     // Serial.println("Actualiza Saldos Iniciales ");
    //     DisplayTFT.info.Actualiza_Saldos_Iniciales = false;

    //     DisplayTFT.Actualiza_Saldos_TFT_Globus_IM(DisplayTFT.info.Casino, DisplayTFT.info.Saldo_Canjeable, DisplayTFT.info.Saldo_No_Restrindigo, DisplayTFT.info.Saldo_Restringido);

    //     DisplayTFT.Actualiza_Puntos_TFT_Globus_IM(DisplayTFT.info.Usuario, DisplayTFT.info.Casino, DisplayTFT.NivelUsuario(DisplayTFT.info.Nivel_Usuario), DisplayTFT.info.Total_Fide, DisplayTFT.info.Total_Bole, DisplayTFT.info.Actual_Fide, DisplayTFT.info.Actual_Bole);
    // }

    // if (DisplayTFT.info.Actualiza_Saldos_Finales)
    // {
    //     // Serial.println("Actualiza Saldos Finales ");
    //     DisplayTFT.info.Actualiza_Saldos_Finales = false;

    //     DisplayTFT.Cierra_Sesion_Player_Cashless_TFT_Globus_IM(DisplayTFT.info.Casino, DisplayTFT.info.Saldo_Canjeable, DisplayTFT.info.Saldo_No_Restrindigo, DisplayTFT.info.Saldo_Restringido);
    // }

    // if (DisplayTFT.info.Actualiza_Puntos_Iniciales)
    // {

    //     Menssage_TFT("Sesion iniciada correctamente",DisplayTFT.configtft.timeoutMensajesCONFIG,true);
    //     DisplayTFT.info.Actualiza_Puntos_Iniciales = false;

    //     DisplayTFT.Actualiza_Puntos_TFT_Globus_IM(DisplayTFT.info.Usuario, DisplayTFT.info.Casino, DisplayTFT.NivelUsuario(DisplayTFT.info.Nivel_Usuario), DisplayTFT.info.Total_Fide, DisplayTFT.info.Total_Bole, DisplayTFT.info.Actual_Fide, DisplayTFT.info.Actual_Bole);
    // }

    // if (DisplayTFT.info.Actualiza_Puntos_Finales)
    // {
    //     Menssage_TFT("Sesion cerrada correctamente",DisplayTFT.configtft.timeoutMensajesCONFIG,true);
    //     DisplayTFT.info.Actualiza_Puntos_Finales = false;
    //     DisplayTFT.Cierra_Sesion_Player_Tracking_TFT_Globus_IM();
    // }
}

bool ConsultarBanners(void)
{

    // Serial.println("Consulta Banner Publicitario");
    char IP_Server[4];
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));

    std::string Ip = IP_toString_(IP_Server);
    String Ip_Server = String(Ip.c_str());
    String Puerto = "9595";
    String fwurl = "http://" + Ip_Server + ":" + Puerto + "/api/Fidelizacion/ConsultarMesajesPublicitarios";

    WiFiClient client;
    HTTPClient http;
    int httpCode;

    http.addHeader("Content-Type", "application/json");
    http.setTimeout(4000);

    if (http.begin(client, fwurl))
    {
        httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK)
        {
            String payload = http.getString();
            StaticJsonDocument<1024> doc;
            DeserializationError err = deserializeJson(doc, payload);

            if (err)
            {
#ifdef Debug_HTTPS
                Serial.println("Error Json deserializeJson");
#endif
            }
            else
            {
                bool IsSuccess = false;

                IsSuccess = doc["IsSuccess"].as<bool>();

                if (IsSuccess)
                {
                    // JsonArray arr = doc["Data"].as<JsonArray>();
                    // cantidad_mensajes = arr.size();

                    // mensajes.clear();
                    // mensajes.reserve(cantidad_mensajes);

                    // bool IsSu=false;

                    // for (int i = 0; i < cantidad_mensajes; i++)
                    // {
                    //     mensajes.push_back(arr[i].as<String>());
                    //     // Serial.println("Banner recibido:");
                    //     // Serial.println(mensajes[i]);
                    // }

                    JsonArray arr = doc["Data"].as<JsonArray>();
                    int nuevosCount = arr.size();

                    // --- Detectar cambios ---
                    bool huboCambio = false;

                    if (mensajes.size() == 0 && nuevosCount > 0)
                    {
                        huboCambio = true;

                        // Serial.println("Depppp");
                    }
                    else
                    {
                        // Si la cantidad es diferente, ya hay cambios
                        if (nuevosCount != mensajes.size())
                        {
                            huboCambio = true;
                        }

                        // Comparar contenido uno a uno
                        for (int i = 0; i < nuevosCount && i < mensajes.size(); i++)
                        {
                            String nuevo = arr[i].as<String>();
                            if (nuevo != mensajes[i])
                            {
                                huboCambio = true;
                            }
                        }
                    }

                    // --- Actualizar mensajes si hay cambios ---
                    if (huboCambio)
                    {
                        // if (mensajes.size() == 0)
                        //     Serial.println("Primera actualizacion");
                        // else
                        //     Serial.println("Cambio");

                        mensajes.clear();
                        mensajes.reserve(nuevosCount);

                        for (int i = 0; i < nuevosCount; i++)
                        {
                            mensajes.push_back(arr[i].as<String>());
                        }
                    }
                    else
                    {
                        http.end();
                        return false;
                    }

                    if (huboCambio)
                    {
                        http.end();
                        return true;
                    }

                    else
                    {
                        http.end();
                        return false;
                    }
                }
                else
                {
                    http.end();
                    return false;
                }
            }
        }
        else
        {
            http.end();
            return false;
        }
    }
    else
    {
        return false;
    }

    // int httpCode = http.GET();

    // if (httpCode != 200)
    // {
    //     Serial.println("Error consultando banners");
    //     http.end();
    //     return false;
    // }

    // String payload = http.getString();
    // http.end();

    // StaticJsonDocument<4096> doc;
    // DeserializationError err = deserializeJson(doc, payload);

    // if (err)
    // {
    //     Serial.println("Error parseando JSON");
    //     return false;
    // }

    // bool IsSuccess = false;

    // IsSuccess = doc["IsSuccess"].as<bool>();

    // if (IsSuccess)
    // {
    //     // JsonArray arr = doc["Data"].as<JsonArray>();
    //     // cantidad_mensajes = arr.size();

    //     // mensajes.clear();
    //     // mensajes.reserve(cantidad_mensajes);

    //     // bool IsSu=false;

    //     // for (int i = 0; i < cantidad_mensajes; i++)
    //     // {
    //     //     mensajes.push_back(arr[i].as<String>());
    //     //     // Serial.println("Banner recibido:");
    //     //     // Serial.println(mensajes[i]);
    //     // }

    //     JsonArray arr = doc["Data"].as<JsonArray>();
    //     int nuevosCount = arr.size();

    //     // --- Detectar cambios ---
    //     bool huboCambio = false;

    //     if (mensajes.size() == 0 && nuevosCount > 0)
    //     {
    //         huboCambio = true;

    //         // Serial.println("Depppp");
    //     }
    //     else
    //     {
    //         // Si la cantidad es diferente, ya hay cambios
    //         if (nuevosCount != mensajes.size())
    //         {
    //             huboCambio = true;
    //         }

    //         // Comparar contenido uno a uno
    //         for (int i = 0; i < nuevosCount && i < mensajes.size(); i++)
    //         {
    //             String nuevo = arr[i].as<String>();
    //             if (nuevo != mensajes[i])
    //             {
    //                 huboCambio = true;
    //             }
    //         }
    //     }

    //     // --- Actualizar mensajes si hay cambios ---
    //     if (huboCambio)
    //     {
    //         // if (mensajes.size() == 0)
    //         //     Serial.println("Primera actualizacion");
    //         // else
    //         //     Serial.println("Cambio");

    //         mensajes.clear();
    //         mensajes.reserve(nuevosCount);

    //         for (int i = 0; i < nuevosCount; i++)
    //         {
    //             mensajes.push_back(arr[i].as<String>());
    //         }
    //     }
    //     else
    //     {
    //         // Serial.println("No cambio");
    //     }

    //     if (huboCambio)
    //         return true;
    //     else
    //         return false;
    // }
    // else
    // {
    //     return false;
    // }

    return false;
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

        // Serial.println(Payload);

        if (Payload.length() > 250)
        {
            Serial.printf("Mensaje %d demasiado largo (%d bytes)\n",
                          i, Payload.length());
            continue;
        }

        if (!Send_TFT(Address_Device_TFT_Display,
                      (uint8_t *)Payload.c_str(), Payload.length()))
        {
            Serial.println("Error enviando mensaje");
            return false;
        }
    }

    // mensajes.clear();
    // mensajes.shrink_to_fit();
    // cantidad_mensajes = 0;

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

bool Pantalla_TFT::Captura_Puntos_Sesion(void)
{

    // Serial.println("Consulta Banner Publicitario");
    char IP_Server[4];
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));

    bool Output = false;

    std::string Ip = IP_toString_(IP_Server);
    String Ip_Server = String(Ip.c_str());
    String Puerto = "9595";
    String fwurl = "http://" + Ip_Server + ":" + Puerto + "/api/Fidelizacion/Informacion_Pantalla";

    WiFiClient client;
    HTTPClient http;
    int httpCode;

    http.addHeader("Content-Type", "application/json");
    http.setTimeout(4000);

    if (http.begin(client, fwurl))
    {
        httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK)
        {
            String payload = http.getString();
            StaticJsonDocument<500> doc;
            DeserializationError err = deserializeJson(doc, payload);

            if (err)
            {
#ifdef Debug_HTTPS
                Serial.println("Error Json deserializeJson");
#endif
            }
            else
            {
                bool IsSuccess = false;

                IsSuccess = doc["IsSuccess"].as<bool>();

                if (IsSuccess)
                {

                    JsonVariant info = doc["InformacionPuntosPantalla"];

                    DisplayTFT.info.Usuario = info["Cliente_Nombre"].as<String>();
                    DisplayTFT.info.Casino = info["Casino"].as<String>();

                    DisplayTFT.info.DenoCashless = info["Deno_Cashless"].as<float>();
                    DisplayTFT.info.Total_Fide = info["Total_Fide"].as<float>();
                    DisplayTFT.info.Total_Bole = info["Total_Bole"].as<float>();
                    DisplayTFT.info.Nivel_Usuario = info["Nivel_Usuario"].as<String>();
                    DisplayTFT.info.Actual_Fide = info["Actual_Fide"].as<float>();
                    DisplayTFT.info.Actual_Bole = info["Actual_Bole"].as<float>();

                    Output = true;
                }
                else
                {
                    Output = false;
                }
            }
        }

        http.end();
    }

    return Output;
}

void Pantalla_TFT::Notify_Now(EventType Notify)
{

    if (eventQueue != NULL && Variables_globales.Get_Variable_Global(Status_Device_TFT_Display) && Variables_globales.Get_Variable_Global(Conexion_TFT_Display))
    {
        ButtonEvent ev;
        ev.type = Notify;

        xQueueSend(eventQueue, &ev, 0);
    }
}

void Pantalla_TFT::Mensaje_TFT(String mensaje, bool Ocultar)
{
    DisplayTFT.info.Mensaje = mensaje;
    DisplayTFT.info.Ocultar = Ocultar;
    DisplayTFT.Notify_Now(EVENT_MENSAJES);
}

EstadoSistema Pantalla_TFT::ObtenerEstadoSistema(void)
{
    
    if(Variables_globales.Get_Variable_Global(Ftp_Mode))
        return FTP_MODE;

    if (WiFi.status() != WL_CONNECTED)
        return ERROR_WIFI;

    if (!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
        return ERROR_MAQUINA;

    if (!Variables_globales.Get_Variable_Global(Conexion_RFID))
        return ERROR_LECTOR;

    return ESTADO_OK;
}

void Pantalla_TFT::Notify_Status_Machine_Now()
{
    EstadoSistema estadoActual = ObtenerEstadoSistema();

    // 🚫 Si no cambió, no hagas nada
    if (estadoActual == estadoAnteriorTFT)
        return;

    estadoAnteriorTFT = estadoActual;

    // ✅ Solo entra aquí si hubo cambio
    switch (estadoActual)
    {
    case ERROR_WIFI:
        Mensaje_TFT("Sin conexion WiFi\n Verificando conexion...", false);
        break;

    case ERROR_MAQUINA:
        Mensaje_TFT("Sin comunicacion maquina\n Por favor espere...", false);
        break;

    case ESTADO_OK:
        // Mensaje_TFT("Sistema OK", true);
        Mensaje_TFT("Dispositivo Disponible", true);
        break;

    case ERROR_LECTOR:
        Mensaje_TFT("Dispositivo lector no conectado\n Por favor verifique la conexion..", true);
        break;

    case FTP_MODE:
        Mensaje_TFT("Modo de mantenimiento habilitado\n Por favor reinicie el dispositivo para salir.", false);
        break;
    }
}

void tareaEventos(void *pvParameters)
{
    ButtonEvent event;

    while (true)
    {
        if (xQueueReceive(eventQueue, &event, portMAX_DELAY))
        {
            switch (event.type)
            {
            case EVENT_ACTUALIZAR_INFO_INICIAL_SESION_CASHLESS:
                // Serial.println("Actualiza Info Pantalla");

                DisplayTFT.Actualiza_Saldos_TFT_Globus_IM(DisplayTFT.info.Casino, DisplayTFT.info.Saldo_Canjeable, DisplayTFT.info.Saldo_No_Restrindigo, DisplayTFT.info.Saldo_Restringido);

                DisplayTFT.Actualiza_Puntos_TFT_Globus_IM(DisplayTFT.info.Usuario, DisplayTFT.info.Casino, DisplayTFT.NivelUsuario(DisplayTFT.info.Nivel_Usuario), DisplayTFT.info.Total_Fide, DisplayTFT.info.Total_Bole, DisplayTFT.info.Actual_Fide, DisplayTFT.info.Actual_Bole);

                break;

            case EVENT_ACTUALIZAR_INFO_FINAL_SESION_CASHLESS:

                // Serial.println("Actualiza Saldos Finales ");
                DisplayTFT.info.Actualiza_Saldos_Finales = false;
                DisplayTFT.Cierra_Sesion_Player_Cashless_TFT_Globus_IM(DisplayTFT.info.Casino, DisplayTFT.info.Saldo_Canjeable, DisplayTFT.info.Saldo_No_Restrindigo, DisplayTFT.info.Saldo_Restringido);

                break;

            case EVENT_ACTUALIZAR_INFO_INICIAL_SESION_FIDELIZACION:

                DisplayTFT.Actualiza_Saldos_TFT_Globus_IM(DisplayTFT.info.Casino, DisplayTFT.info.Saldo_Canjeable, DisplayTFT.info.Saldo_No_Restrindigo, DisplayTFT.info.Saldo_Restringido, true);

                DisplayTFT.info.Actualiza_Puntos_Iniciales = false;
                DisplayTFT.Actualiza_Puntos_TFT_Globus_IM(DisplayTFT.info.Usuario, DisplayTFT.info.Casino, DisplayTFT.NivelUsuario(DisplayTFT.info.Nivel_Usuario), DisplayTFT.info.Total_Fide, DisplayTFT.info.Total_Bole, DisplayTFT.info.Actual_Fide, DisplayTFT.info.Actual_Bole);
                break;

            case EVENT_ACTUALIZAR_INFO_FINAL_SESION_FIDELIZACION:
                DisplayTFT.info.Actualiza_Puntos_Finales = false;
                DisplayTFT.Cierra_Sesion_Player_Tracking_TFT_Globus_IM();
                break;

            case EVENT_MENSAJES:
                Menssage_TFT(DisplayTFT.info.Mensaje, DisplayTFT.configtft.timeoutMensajesCliente, DisplayTFT.info.Ocultar);
                break;

            case EVENT_ACTUALIZA_PUNTOS:
                DisplayTFT.Actualiza_Puntos_TFT_Globus_IM(DisplayTFT.info.Usuario, DisplayTFT.info.Casino, DisplayTFT.NivelUsuario(DisplayTFT.info.Nivel_Usuario), DisplayTFT.info.Total_Fide, DisplayTFT.info.Total_Bole, DisplayTFT.info.Actual_Fide, DisplayTFT.info.Actual_Bole);
                break;

            case EVENT_RECUPERA_SESION:
                if (DisplayTFT.Get_Info_Cliente(contadores.Get_Client_ID_Transaccion_Int()))
                {

                    DisplayTFT.Actualiza_Saldos_TFT_Globus_IM(DisplayTFT.info.Casino, DisplayTFT.info.Saldo_Canjeable, DisplayTFT.info.Saldo_No_Restrindigo, DisplayTFT.info.Saldo_Restringido);

                    DisplayTFT.Actualiza_Puntos_TFT_Globus_IM(DisplayTFT.info.Usuario, DisplayTFT.info.Casino, DisplayTFT.NivelUsuario(DisplayTFT.info.Nivel_Usuario), DisplayTFT.info.Total_Fide, DisplayTFT.info.Total_Bole, DisplayTFT.info.Actual_Fide, DisplayTFT.info.Actual_Bole);
                }
                else
                {

                    DisplayTFT.info.Mensaje = "Sesion recuperada.\nNivel y puntos no disponibles en pantalla.";
                    DisplayTFT.info.Ocultar = true;
                    DisplayTFT.Notify_Now(EVENT_MENSAJES);
                }

                break;

            case EVENT_ON_START_OTA:
                DisplayTFT.info.Mensaje = "Actualizando interfaz Globus IM ESP32\nPor favor espere...";
                DisplayTFT.info.Ocultar = false;
                DisplayTFT.Notify_Now(EVENT_MENSAJES);
                break;

            case EVENT_ON_PROGRESS:
                DisplayTFT.Progreso(DisplayTFT.info.Progress);
                break;

            case EVENT_ON_ERROR:
                DisplayTFT.info.Mensaje = "Interfaz Globus IM ESP32\n Error de actualizacion";
                DisplayTFT.info.Ocultar = true;
                DisplayTFT.Notify_Now(EVENT_MENSAJES);
                break;

            case EVENT_ON_END:
                DisplayTFT.info.Mensaje = "Interfaz Globus IM ESP32\n Actualizacion finalizada.";
                DisplayTFT.info.Ocultar = true;
                
                break;


            case CALL_OPERATOR_EVENT:

                if (DisplayTFT.Evento_Soporte(DisplayTFT.info.Evento, DisplayTFT.info.Message))
                {

                    DisplayTFT.info.Mensaje = "Llamando al operador\n Por favor espere...";
                    DisplayTFT.info.Ocultar = true;
                    DisplayTFT.Notify_Now(EVENT_MENSAJES);
                }
                else
                {
                    DisplayTFT.info.Mensaje = "Error solicitando operador\n Por favor Intente nuevamente";
                    DisplayTFT.info.Ocultar = true;
                    DisplayTFT.Notify_Now(EVENT_MENSAJES);
                }
                break;


            case EVENT_RESET_ESPNOW:

                
                esp_wifi_set_promiscuous(true);
                esp_wifi_set_channel(5, WIFI_SECOND_CHAN_NONE);
                esp_wifi_set_promiscuous(false);

                esp_now_deinit();

                if (Init_TFT_Display(true, Address_Device_TFT_Display))
                {
                    flag_ReiniciarEspNow = true;
                    //Serial.println("ESPNOW----OK");

                }
                else
                    flag_ReiniciarEspNow = false;

                // if(WiFi.status()!= WL_CONNECTED)
                //     WiFi.reconnect();
                break;

            case EVENT_CONEXION_WIFI_ESTAB:
                //Serial.println("Conectado....");

                if (Init_TFT_Display(true, Address_Device_TFT_Display))
                {
                    flag_ReiniciarEspNow = true;

                    //Serial.println("ESPNOW----OK");
                }
                else
                    flag_ReiniciarEspNow = false;
                break;

            case EVENT_RECOVERY_SD:
                
                Init_SD();
            break;

            case EVENT_LOGIN_TFT:
                if (DisplayTFT.Login_TFT(DisplayTFT.info.Usuario_Espnow, DisplayTFT.info.Contrasena_Espnow, "/api/Login"))
                {
                    Serial.println("Exitosooooo");
                }
                else
                {
                    Serial.println("Con ERRROR");
                }
            break;
            }
        }
    }
}

bool Pantalla_TFT::Get_Info_Cliente(int Cliente)
{

    

    bool Output = false;

   char IP_Server[4];
    char Current_IP[4];
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));

    std::string Ip = IP_toString_(IP_Server);
    std::string maquinaIp = IP_toString_(Current_IP);
    String Ip_Server = String(Ip.c_str());

    String MaquinaIp = String(maquinaIp.c_str());

    // http://192.168.5.180:9090/api/Tarjeta/ConsultaPuntosCliente?tarjetaId=13260&maquinaIp=192.168.5.130

    String Puerto = "9090";
    String fwurl = "http://" + Ip_Server + ":" + Puerto + "/api/Tarjeta/ConsultaPuntosCliente?tarjetaId=" + String(Cliente) + "&"+"maquinaIp="+MaquinaIp;

    WiFiClient client;
    HTTPClient http;
    int httpCode;

    http.addHeader("Content-Type", "application/json");
    http.setTimeout(4000);

    if (http.begin(client, fwurl))
    {
        httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK)
        {
            String payload = http.getString();
            StaticJsonDocument<500> doc;
            DeserializationError err = deserializeJson(doc, payload);

            if (err)
            {
#ifdef Debug_HTTPS
                Serial.println("Error Json deserializeJson");
#endif
            }
            else
            {
                bool IsSuccess = false;

                IsSuccess = doc["IsSuccess"].as<bool>();

                if (IsSuccess)
                {

                    JsonVariant info = doc["Data"];

                    DisplayTFT.info.Usuario = info["Cliente_Nombre"].as<String>();
                    DisplayTFT.info.Casino = info["Casino"].as<String>();

                    DisplayTFT.info.DenoCashless = info["Deno_Cashless"].as<float>();
                    DisplayTFT.info.Total_Fide = info["Total_Fide"].as<float>();
                    DisplayTFT.info.Total_Bole = info["Total_Bole"].as<float>();
                    DisplayTFT.info.Nivel_Usuario = info["Nivel_Usuario"].as<String>();
                    DisplayTFT.info.Actual_Fide = info["Actual_Fide"].as<float>();
                    DisplayTFT.info.Actual_Bole = info["Actual_Bole"].as<float>();
                    DisplayTFT.info.Sesion_Server = info["ClienteEnSesion"].as<bool>();
                    Output = true;
                }
                else
                {
                    Output = false;
                }
            }
        }

        http.end();
    }

    return Output;
}

bool Pantalla_TFT::Reset_Pantalla_TFT(void)
{
    StaticJsonDocument<500> doc;
    String Payload;
    int Intentos_Conexion = 5;
    doc["IsSuccess"] = true;
    doc["Opcion"] = PING;

    bool IsSuccess = false;

    serializeJson(doc, Payload);

    //Serial.println(Payload);
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
            IsSuccess = true;
        else
            IsSuccess = false;
    }
    else
    {
        return IsSuccess;
    }

    return IsSuccess;
}

void Pantalla_TFT::Init_Handle()
{
    eventQueue = xQueueCreate(8, sizeof(ButtonEvent));

    xTaskCreatePinnedToCore(
        tareaEventos,
        "Eventos",
        4096,
        NULL,
        1,
        NULL,
        1);
}

bool Pantalla_TFT::Evento_Soporte(int Evento, String Message)
{
    char IP_Server[4];
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));

    bool Output = false;

    std::string Ip = IP_toString_(IP_Server);
    String Ip_Server = String(Ip.c_str());
    

    // http://192.168.5.180:9090/api/Tarjeta/ConsultaPuntosCliente?tarjetaId=13260&maquinaIp=192.168.5.130

    String Puerto = "9090";
    String fwurl = "http://" + Ip_Server + ":" + Puerto + "/api/Soporte/Solicitud_Usuario";

    WiFiClient client;
    HTTPClient http;
    int httpCode;

    StaticJsonDocument<450> jsonDocument;
    jsonDocument.clear();
    char Current_IP[4];
    String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    jsonDocument["IsSuccess"] = true;
    jsonDocument["Evento"] = Evento;
    jsonDocument["Message"] = Message;
    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Ip"] = IP_toString_(Current_IP);
    jsonDocument["MAC"] = WiFi.macAddress();


    if(Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless))
        jsonDocument["Cliente_ID"] = contadores.Get_Client_ID_Transaccion_Int();
    else
        jsonDocument["Cliente_ID"] = contadores.Get_Client_ID_Int();

    jsonDocument["Id_Maquina"] = 0;

    http.addHeader("Content-Type", "application/json");
    http.setTimeout(4000);

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */

    if (http.begin(client, fwurl))
    {
        httpCode = http.POST(Json);

        if (httpCode == HTTP_CODE_OK)
        {
            String payload = http.getString();
            StaticJsonDocument<500> doc;
            DeserializationError err = deserializeJson(doc, payload);

            if (err)
            {
#ifdef Debug_HTTPS
                Serial.println("Error Json deserializeJson");
#endif
            }
            else
            {
                bool IsSuccess = false;

                IsSuccess = doc["IsSuccess"].as<bool>();

                if (IsSuccess)
                {
                    Output = true;
                }
                else
                {
                    Output = false;
                }
            }
        }

        http.end();
    }

    return Output;
}

bool Pantalla_TFT::Login_TFT(const String &Usuario,
                             const String &Contrasena,
                             const String &URL)
{
    //-------------------------------------------------
    // Inicialización segura
    //-------------------------------------------------
    bool Output = false;

    DisplayTFT.info.Id_Usuario_TFT = "";
    DisplayTFT.info.Contrasena_TFT = "";
    DisplayTFT.info.Id_TFT_Temporal = 0;

    //-------------------------------------------------
    // Validaciones entrada
    //-------------------------------------------------
    if (Usuario.isEmpty() ||
        Contrasena.isEmpty() ||
        URL.isEmpty())
    {
//#ifdef Debug_HTTPS
        Serial.println("[LOGIN] Parametros invalidos");
//#endif
        return false;
    }

    if (Usuario.length() <=0 ||
        Contrasena.length() <=0 )
    {
//#ifdef Debug_HTTPS
        Serial.println("[LOGIN] Longitud invalida");
//#endif
        return false;
    }

    //-------------------------------------------------
    // Verificar WiFi
    //-------------------------------------------------
    if (WiFi.status() != WL_CONNECTED)
    {
//#ifdef Debug_HTTPS
        Serial.println("[LOGIN] WiFi desconectado");
//#endif
        return false;
    }

    //-------------------------------------------------
    // Obtener IP servidor
    //-------------------------------------------------
    char IP_Server[4] = {0};

    auto cfg = Configuracion.Get_Configuracion(Direccion_IP_Server, 'x');

    if (cfg == nullptr)
        return false;

    memcpy(IP_Server, cfg, 4);

    String Ip_Server = String(IP_toString_(IP_Server).c_str());

    if (Ip_Server.length() < 7)
    {
//#ifdef Debug_HTTPS
        Serial.println("[LOGIN] IP servidor invalida");
//#endif
        return false;
    }

    //-------------------------------------------------
    // Construcción URL
    //-------------------------------------------------
    const String Puerto = "9090";
    String fwurl =
        "http://" +
        Ip_Server +
        ":" +
        Puerto +
        URL;

//#ifdef Debug_HTTPS
    Serial.println("[LOGIN] URL:");
    Serial.println(fwurl);
//#endif

    //-------------------------------------------------
    // Construcción JSON
    //-------------------------------------------------
    StaticJsonDocument<512> req;

    char Current_IP[4] = {0};

    auto cfgLocal =
        Configuracion.Get_Configuracion(Direccion_IP, 'x');

    if (cfgLocal != nullptr)
        memcpy(Current_IP, cfgLocal, 4);

    String fecha =
        String(RTC.getYear()) + "-" +
        String(RTC.getMonth() + 1) + "-" +
        String(RTC.getDay()) + " " +
        String(RTC.getHour(true)) + ":" +
        String(RTC.getMinute()) + ":" +
        String(RTC.getSecond());

    req["Usuario"] = Usuario;
    req["Password"] = Contrasena;
    req["Fecha_Hora"] = fecha;
    req["Ip"] = IP_toString_(Current_IP);
    req["MAC"] = WiFi.macAddress();
    req["Id_Maquina"] = 0;

    String body;
    serializeJson(req, body);

    //-------------------------------------------------
    // HTTP
    //-------------------------------------------------
    WiFiClient client;
    HTTPClient http;

    http.setTimeout(6000);
    http.setConnectTimeout(3000);

    if (!http.begin(client, fwurl))
    {
//#ifdef Debug_HTTPS
        Serial.println("[LOGIN] begin fallo");
//#endif
        return false;
    }

    http.addHeader("Content-Type",
                   "application/json");

    int httpCode =
        http.POST(body);

//#ifdef Debug_HTTPS
    Serial.printf(
        "[LOGIN] HTTP=%d\n",
        httpCode);
//#endif

    if (httpCode != HTTP_CODE_OK)
    {
        http.end();
        return false;
    }

    String payload =
        http.getString();

    http.end();

    if (payload.isEmpty())
    {
//#ifdef Debug_HTTPS
        Serial.println("[LOGIN] Payload vacio");
//#endif
        return false;
    }

    //-------------------------------------------------
    // Parse respuesta
    //-------------------------------------------------
    StaticJsonDocument<512> doc;

    auto err =
        deserializeJson(doc,
                        payload);

    if (err)
    {
//#ifdef Debug_HTTPS
        Serial.println(
            "[LOGIN] JSON invalido");
//#endif
        return false;
    }

    //-------------------------------------------------
    // Validación respuesta
    //-------------------------------------------------
    if (!doc["IsSuccess"].as<bool>())
        return false;

    if (!(doc.containsKey("Usuario") &&
          doc.containsKey("Contrasena") &&
          doc.containsKey("Id_Usuario")))
    {
        return false;
    }

    String UsuarioSrv =
        doc["Usuario"].as<String>();

    String PasswordSrv =
        doc["Contrasena"].as<String>();

    int Id =
        doc["Id_Usuario"].as<int>();

    if (Id <= 0)
        return false;

    //-------------------------------------------------
    // Validación credenciales
    //-------------------------------------------------
    if (Usuario != UsuarioSrv)
        return false;

    if (Contrasena != PasswordSrv)
        return false;

    //-------------------------------------------------
    // Login exitoso
    //-------------------------------------------------
    DisplayTFT.info.Id_Usuario_TFT =
        UsuarioSrv;

    DisplayTFT.info.Contrasena_TFT =
        PasswordSrv;

    DisplayTFT.info.Id_TFT_Temporal =
        Id;

    Output = true;

//#ifdef Debug_HTTPS
    Serial.println(
        "[LOGIN] OK");
//#endif

    return Output;
}

int Pantalla_TFT::Recibe_Info_Login(String Usuario, String Contrasena, bool IsSuccess)
{
    static int issuccess = 0;

    if (StateKeyboard != STATE_LOGIN_IDLE)
    {
        issuccess = 0; /* Busy*/
        return issuccess;
    }
    else
    {
        if (IsSuccess)
        {

            StateKeyboard = STATE_LOGIN_IN_PROGRESS;
            DisplayTFT.info.Usuario_Espnow = Usuario;
            DisplayTFT.info.Contrasena_Espnow = Contrasena;

            DisplayTFT.Notify_Now(EVENT_LOGIN_TFT);
            issuccess = 1; /* Recibido*/
        }
        else
        {
            StateKeyboard = STATE_LOGIN_IDLE;
            issuccess = 2; /*Error*/
        }
    }

    return issuccess;
}



bool Pantalla_TFT::SendLoginResponse(uint8_t *addr,
                       bool success,
                       int code,int HardCode)
{
    StaticJsonDocument<128> doc;

    doc["Message"] = "login_response";
    doc["IsSuccess"] = success;
    doc["Opcion"] = code;
    doc["HardCode"]=HardCode;

    String payload;
    serializeJson(doc, payload);

    return Send_TFT(addr,
                    (uint8_t*)payload.c_str(),
                    payload.length());
}