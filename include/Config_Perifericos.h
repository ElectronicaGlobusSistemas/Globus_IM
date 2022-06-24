#include <Arduino.h>
#include "Memory_SD.h"
#include "RTC.h"
#include "nvs_flash.h"

//-------------------> Parametros <-------------------------------
#define Clock_frequency 240
#define MCU_Status 2
#define WIFI_Status 15
#define Reset_Config 21
//-----------------------------------------------------------------

//-------------------------> Extern TaskHandle_t <-----------------
extern TaskHandle_t SD_CHECK;          //  Manejador de tareas
extern Sd2Card card;                   //  Memoria SD.
extern TaskHandle_t Ftp_SERVER;        //  Manejador de tareas
extern TaskHandle_t Status_WIFI;       //  Manejador de  Tarea Wifi
extern TaskHandle_t Status_SERVER_TCP; // M,anejador de Tarea Server TCP
extern TaskHandle_t Modo_Bootloader;   // Manejador Bootloader
extern WiFiClient client;              // Declara un objeto cliente para conectarse al servidor
//------------------------------------------------------------------

//----------------------> TaskHandle_t <----------------------------
TaskHandle_t ManagerTask;
//------------------------------------------------------------------

//-----------------------> Prototipo de Funciones <-----------------
void Init_Indicadores_LED(void);
void Init_Configuracion_Inicial(void);
void Reset_Configuracion_Inicial(void);
void TaskManager();
static void ManagerTasks(void *parameter);
//------------------------------------------------------------------

void Init_Config(void)
{
    //------------------------> Config MCU <-------------------------
    setCpuFrequencyMhz(Clock_frequency); // Frecuencia de Nucleos 1 y 0.
    //---------------------------------------------------------------
    Serial.begin(115200); //  Inicializa Monitor Serial Debug
    //---------------------> Inicializa Indicadores <----------------
    pinMode(SD_ChipSelect, OUTPUT); // Selector de Esclavo SPI.
    pinMode(SD_Status, OUTPUT);     // SD Status Como Salida.
    pinMode(MCU_Status, OUTPUT);    // MCU_Status Como Salida.
    pinMode(WIFI_Status, OUTPUT);   // Wifi_Status como Salida.
    pinMode(Reset_Config, INPUT);   // Reset_Config como Entrada.
    Init_Indicadores_LED();         // Reset Indicadores LED'S LOW.
    //---------------------------------------------------------------

    //--------------------> Setup Reloj <----------------------------
    RTC.setTime(0, 12, 10, 9, 6, 2022);
    //---------------------------------------------------------------
    //-------------------> Reset valores NVS <-----------------------
    Reset_Configuracion_Inicial();
    //--------------------> Init NVS Datos <-------------------------
    Init_Configuracion_Inicial(); // Inicializa Config de Memoria
    //---------------------------------------------------------------

    //-----------------> Config Comunicación Maquina <---------------
    Init_UART2(); // Inicializa Comunicación Maquina Puerto #2
    //---------------------------------------------------------------

    //--------------------> Config  WIFI <---------------------------
    CONNECT_WIFI();        // Inicializa  Modulo WIFI
    CONNECT_SERVER_TCP();  // Inicializa Servidor TCP
    init_Comunicaciones(); // Inicializa Tareas TCP
    //--------------------> Módulos <--------------------------------
    Init_SD(); // Inicializa Memoria SD.
    //---------------------------------------------------------------
    // int dia = 21;
    // int mes = 6;
    // char dia_char[] = (char)dia;
    // char mes_char[] = (char)mes;
    // char nombre[12] = {};
    // strcpy(nombre,dia_char);

    // Serial.println("****************************************");
    // Serial.println(nombre);
    Init_FTP_SERVER(); // Inicializa SERVER
    //---------------------------------------------------------------
    Archivo_Format = "37062022.csv"; // Crea Archivo Si no Existe.
    Create_ARCHIVE_Excel(Archivo_Format, Encabezado_Contadores);
    //  Create_ARCHIVE_Excel("37062022.csv",Encabezado_Contadores);
    Init_Wifi();
    //---------------------------------------------------------------
    //-------------------->  Update  <-------------------------------
    Init_Bootloader();

    //---------------------------------------------------------------

    TaskManager(); // Inicia Manejador de Tareas de Verificación
}

void TaskManager()
{
    xTaskCreatePinnedToCore(
        ManagerTasks,              //  Funcion a implementar la tarea
        "TASK MANAGER",            //  Nombre de la tarea
        10000,                     //  Tamaño de stack en palabras (memoria)
        NULL,                      //  Entrada de parametros
        configMAX_PRIORITIES - 10, //  Prioridad de la tarea
        &ManagerTask,              //  Manejador de la tarea
        0);                        //  Core donde se ejecutara la tarea
}

static void ManagerTasks(void *parameter)
{

    unsigned long Tiempo_Actual = 0;
    unsigned long Tiempo_Previo = 0;
    bool MCU_State = LOW;
    for (;;)
    {
        Tiempo_Actual = millis();

        if ((Tiempo_Actual - Tiempo_Previo) > 100)
        {
            Tiempo_Previo = Tiempo_Actual;
            MCU_State = !MCU_State;
            digitalWrite(MCU_Status, !MCU_State);
        }

        // if (!card.init(SPI_FULL_SPEED,SD_ChipSelect) && !SD.begin(SD_ChipSelect,MOSI,MISO,CLK))
        // {
        //     if (eTaskGetState(SD_CHECK) == eRunning)
        //     {
        //         Serial.println("------->>>>> Rum Task   SD CHECK");
        //         continue;
        //     }
        //     else if (eTaskGetState(SD_CHECK) == eSuspended)
        //     {
        //         Serial.println("------->>>>> Resume Task  SD CHECK");
        //         vTaskResume(SD_CHECK); // Inicia Tarea SD.
        //     }
        // }

        if (Variables_globales.Get_Variable_Global(Ftp_Mode) == true)
        {
            if (eTaskGetState(Ftp_SERVER) == eRunning)
            {
                Serial.println("------->>>>> Run Task   Ftp SERVER ");
                continue;
            }
            else if (eTaskGetState(Ftp_SERVER) == eSuspended)
            {
                Serial.println("------->>>>> Resume Task  Ftp SERVER");
                vTaskResume(Ftp_SERVER); // Inicia Modo FTP SERVER.
            }
        }
        if (WiFi.status() != WL_CONNECTED)
        {
            if (eTaskGetState(Status_WIFI) == eRunning)
            {
                Serial.println("------->>>>> Rum Task   Status WIFI");
                continue;
            }
            else if (eTaskGetState(Status_WIFI) == eSuspended)
            {
                Serial.println("------->>>>> Resume Task  Status WIFI");
                vTaskResume(Status_WIFI); // Inicia Modo Bootlader.
            }

            vTaskResume(Status_WIFI); // Inicia Tarea  WIFI.
        }
        if (!clientTCP.connected() && Configuracion.Get_Configuracion(Tipo_Conexion))
        {
            if (eTaskGetState(Status_SERVER) == eRunning)
            {
                Serial.println("------->>>>> Rum Task   SERVER TCP");
                continue;
            }
            else if (eTaskGetState(Status_SERVER) == eSuspended)
            {
                Serial.println("------->>>>> Resume Task  SERVER TCP");
                vTaskResume(Status_SERVER); // Inicia Tarea  TCP.
            }
        }
        if (Variables_globales.Get_Variable_Global(Bootloader_Mode) == true && WiFi.status() == WL_CONNECTED)
        {
            if (eTaskGetState(Modo_Bootloader) == eRunning)
            {
                Serial.println("------->>>>> Rum Task   Modo Bootloader");
                continue;
            }
            else if (eTaskGetState(Modo_Bootloader) == eSuspended)
            {
                Serial.println("------->>>>> Resume Task  Modo Bootloader");
                vTaskResume(Modo_Bootloader); // Inicia Modo Bootlader.
            }
        }
        delay(50);
        vTaskDelay(1000);
    }
    vTaskDelete(NULL);
    vTaskDelay(10);
}

void Init_Indicadores_LED(void)
{
    digitalWrite(SD_ChipSelect, LOW);
    digitalWrite(SD_Status, LOW);
    digitalWrite(MCU_Status, LOW);
    digitalWrite(WIFI_Status, LOW);
}

void Init_Configuracion_Inicial(void)
{
    Serial.println("\n");
    Serial.println("Inicializando modulo...");
    // Borrar particiones creadas en NVS
    // nvs_flash_erase();
    // nvs_flash_init();
    NVS.begin("Config_ESP32", false);

    if (!NVS.isKey("Dir_IP")) // Configura la IP de conexion
    {
        Serial.println("Guardando IP por defecto...");
        uint8_t ip[] = {192, 168, 5, 250};
        NVS.putBytes("Dir_IP", ip, sizeof(ip));
    }

    if (!NVS.isKey("Dir_IP_GW")) // Configura la IP de enlace
    {
        Serial.println("Guardando IP_GW por defecto...");
        uint8_t ip_gw[] = {192, 168, 5, 1};
        NVS.putBytes("Dir_IP_GW", ip_gw, sizeof(ip_gw));
    }

    if (!NVS.isKey("Dir_SN_MASK")) // Configura la Mascara Subred
    {
        Serial.println("Guardando SN_MASK por defecto...");
        uint8_t sn_mask[] = {255, 255, 255, 0};
        NVS.putBytes("Dir_SN_MASK", sn_mask, sizeof(sn_mask));
    }

    if (!NVS.isKey("Dir_IP_Serv")) // Configura la IP de servidor
    {
        Serial.println("Guardando IP Server por defecto...");
        uint8_t ip_server[] = {192, 168, 5, 208};
        NVS.putBytes("Dir_IP_Serv", ip_server, sizeof(ip_server));
    }

    if (!NVS.isKey("Socket")) // Configura el numero de socket
    {
        Serial.println("Guardando Puerto por defecto...");
        uint16_t port = 1001;
        NVS.putUInt("Socket", port);
    }

    if (!NVS.isKey("Name_Maq")) // Configura el nombre de la MAQ
    {
        Serial.println("Guardando Nombre MAQ por defecto...");
        char name[17] = {'M', 'a', 'q', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
        NVS.putString("Name_Maq", name);
    }

    if (!NVS.isKey("SSID_DESA")) // Configura SSID de conexion WIFI
    {
        Serial.println("Guardando SSID por defecto...");
        //        String ssid = "GLOBUS_ONLINEW";
        String ssid = "GLOBUS-DESARROLLO";
        NVS.putString("SSID_DESA", ssid);
    }

    if (!NVS.isKey("PASS_DESA")) // Configura PASSWORD de conexion WIFI
    {
        Serial.println("Guardando Password por defecto...");
        //        String password = "Globus#OnlineW324";
        String password = "Globus2020*";
        NVS.putString("PASS_DESA", password);
    }

    if (!NVS.isKey("TYPE_CONNECT")) // Configura el tipo de conexion server
    {
        Serial.println("Guardando tipo de conexion por defecto UDP");
        // Conexion UDP = false
        // Conexion TCP = true
        bool Conexion_Server = false;
        NVS.putBool("TYPE_CONNECT", Conexion_Server);
    }

    if (!NVS.isKey("TYPE_MAQ"))
    {
        Serial.println("Guardando tipo de maquina");
        // 0 = Por defecto,
        // 1 = Cashless AFT (Contadores en creditos)
        // 2 = Cashless EFT
        // 3 = Cashless AFT Single (contadores netos)
        // 4 = IRT
        // 5 = Generica (Encuesta simple)
        // 6 = Poker
        uint16_t tipo_maq = 5;
        NVS.putUInt("TYPE_MAQ", tipo_maq);
    }

    /*--------------------------------------------------------------------------------------------------------------------------*/
    /*--------------------------------------------------------------------------------------------------------------------------*/
    /*--------------------------------------------------------------------------------------------------------------------------*/

    // Inicializa Direccion IP
    size_t ip_len = NVS.getBytesLength("Dir_IP");
    char IP[ip_len];
    NVS.getBytes("Dir_IP", IP, ip_len);
    Configuracion.Set_Configuracion_ESP32(Direccion_IP, IP);
    char IP_prueba[4];
    bzero(IP_prueba, 4);
    memcpy(IP_prueba, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(IP_prueba) / sizeof(IP_prueba[0]));
    Serial.print("Direccion IP: ");
    for (int i = 0; i < 4; i++)
    {
        Serial.print((int)IP_prueba[i]);
        Serial.print(" ");
    }
    Serial.println();
    /*--------------------------------------------------------------------------------------------------------------------------*/

    // Inicializa Direccion IP Enlace
    size_t ip_gw_len = NVS.getBytesLength("Dir_IP_GW");
    char IP_GW[ip_gw_len];
    NVS.getBytes("Dir_IP_GW", IP_GW, ip_gw_len);
    Configuracion.Set_Configuracion_ESP32(Direccion_IP_GW, IP_GW);
    IP_prueba[4];
    bzero(IP_prueba, 4);
    memcpy(IP_prueba, Configuracion.Get_Configuracion(Direccion_IP_GW, 'x'), sizeof(IP_prueba) / sizeof(IP_prueba[0]));
    Serial.print("Direccion IP Enlace: ");
    for (int i = 0; i < 4; i++)
    {
        Serial.print((int)IP_prueba[i]);
        Serial.print(" ");
    }
    Serial.println();
    /*--------------------------------------------------------------------------------------------------------------------------*/

    // Inicializa Direccion Mascara Subred
    size_t sn_mask_len = NVS.getBytesLength("Dir_SN_MASK");
    char SN_MASK[sn_mask_len];
    NVS.getBytes("Dir_SN_MASK", SN_MASK, sn_mask_len);
    Configuracion.Set_Configuracion_ESP32(Direccion_SN_MASK, SN_MASK);
    IP_prueba[4];
    bzero(IP_prueba, 4);
    memcpy(IP_prueba, Configuracion.Get_Configuracion(Direccion_SN_MASK, 'x'), sizeof(IP_prueba) / sizeof(IP_prueba[0]));
    Serial.print("Direccion Mascara Subred: ");
    for (int i = 0; i < 4; i++)
    {
        Serial.print((int)IP_prueba[i]);
        Serial.print(" ");
    }
    Serial.println();
    /*--------------------------------------------------------------------------------------------------------------------------*/

    // Inicializa Direccion IP Servidor
    size_t ip_serv_len = NVS.getBytesLength("Dir_IP_Serv");
    char IP_SERV[ip_serv_len];
    NVS.getBytes("Dir_IP_Serv", IP_SERV, ip_serv_len);
    Configuracion.Set_Configuracion_ESP32(Direccion_IP_Server, IP_SERV);
    IP_prueba[4];
    bzero(IP_prueba, 4);
    memcpy(IP_prueba, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_prueba) / sizeof(IP_prueba[0]));
    Serial.print("Direccion IP Servidor: ");
    for (int i = 0; i < 4; i++)
    {
        Serial.print((int)IP_prueba[i]);
        Serial.print(" ");
    }
    Serial.println();
    /*--------------------------------------------------------------------------------------------------------------------------*/

    // Inicializa Puerto de conexion a servidor
    uint16_t port_server = NVS.getUInt("Socket", 0);
    Configuracion.Set_Configuracion_ESP32(Puerto_Server, port_server);
    Serial.print("Puerto de conexion: ");
    Serial.println(Configuracion.Get_Configuracion(Puerto_Server, 0));
    /*--------------------------------------------------------------------------------------------------------------------------*/

    // Inicializa Nombre Maq
    String name = NVS.getString("Name_Maq");
    Configuracion.Set_Configuracion_ESP32(Nombre_Maquina, name);
    Serial.print("Nombre de la maquina: ");
    Serial.println(Configuracion.Get_Configuracion(Nombre_Maquina, "Nombre_Maq"));
    /*--------------------------------------------------------------------------------------------------------------------------*/

    // Inicializa Red Wifi y Password
    String ssid = NVS.getString("SSID_DESA");
    Configuracion.Set_Configuracion_ESP32(SSID, ssid);
    String password = NVS.getString("PASS_DESA");
    Configuracion.Set_Configuracion_ESP32(Password, password);
    Serial.print("Conecta a Red: ");
    Serial.println(Configuracion.Get_Configuracion(SSID, "Nombre_Red"));
    /*--------------------------------------------------------------------------------------------------------------------------*/

    // Inicializa Tipo de conexion Servidor UDP o TCP
    bool Conexion_Server = NVS.getBool("TYPE_CONNECT");
    Configuracion.Set_Configuracion_ESP32(Tipo_Conexion, Conexion_Server);
    if (Configuracion.Get_Configuracion(Tipo_Conexion))
        Serial.println("Tipo de conexion a servidor: TCP");
    else
        Serial.println("Tipo de conexion a servidor: UDP");
    /*--------------------------------------------------------------------------------------------------------------------------*/

    // Inicializa configuracion tipo de maquina
    uint16_t tipo_maq = NVS.getUInt("TYPE_MAQ", 0);
    Configuracion.Set_Configuracion_ESP32(Tipo_Maquina, tipo_maq);
    Serial.print("Configuracion tipo de maquina: ");
    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 0:
        Serial.println("Defecto");
        break;
    case 1:
        Serial.println("Cashless AFT");
        break;
    case 2:
        Serial.println("Cashless EFT");
        break;
    case 3:
        Serial.println("Cashless AFT Single");
        break;
    case 4:
        Serial.println("IRT");
        break;
    case 5:
        Serial.println("Generica");
        break;
    case 6:
        Serial.println("Poker");
        break;
    default:
        break;
    }
    /*--------------------------------------------------------------------------------------------------------------------------*/

    Serial.println("\n");
    NVS.end();
}

void Reset_Configuracion_Inicial(void)
{
    bool MCU_State = LOW;
    while (digitalRead(Reset_Config) == HIGH)
    {
        if (millis() > 10000)
        {
            Serial.println("Reset activado............................");
            for (int i = 0; i < 50; i++)
            {
                MCU_State = !MCU_State;
                digitalWrite(MCU_Status, !MCU_State);
                delay(100);
            }

            NVS.begin("Config_ESP32", false);

            // Inicializa Direccion IP
            Serial.println("Reset IP por defecto...");
            size_t ip_len = NVS.getBytesLength("Dir_IP");
            char IP[ip_len];
            NVS.getBytes("Dir_IP", IP, ip_len);
            IP[3] = 250;
            NVS.putBytes("Dir_IP", IP, sizeof(IP));

            // Reset Nombre MAQ
            Serial.println("Reset Nombre MAQ por defecto...");
            char name[17] = {'M', 'a', 'q', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
            NVS.putString("Name_Maq", name);
            NVS.end();
            ESP.restart();
        }
    }
    return;
}