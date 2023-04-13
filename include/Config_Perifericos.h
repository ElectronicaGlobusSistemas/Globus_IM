#include <Arduino.h>
#include "Memory_SD.h"
#include "RTC.h"
#include "nvs_flash.h"
#include <SPI.h>
#include <SD.h>

//-------------------> Parametros <-------------------------------
#define Clock_frequency  240//240//
#define MCU_Status       2
#define WIFI_Status      15
#define Reset_Config     27
#define Hopper_Enable    14
#define MCU_Status_2     25
#define Unlock_Machine   26
#define FLASH_RESET_Pin  35
//-----------------------------------------------------------------

//-------------------------> Extern TaskHandle_t <-----------------
extern TaskHandle_t SD_CHECK;          //  Manejador de tareas
extern TaskHandle_t Ftp_SERVER;        //  Manejador de tareas
extern TaskHandle_t Status_WIFI;       //  Manejador de  Tarea Wifi
extern TaskHandle_t Status_SERVER_TCP; // M,anejador de Tarea Server TCP
extern TaskHandle_t Modo_Bootloader;   // Manejador Bootloader
extern WiFiClient client;              // Declara un objeto cliente para conectarse al servidor
//------------------------------------------------------------------




extern char Archivo_CSV[100];
//extern int Sd_Mont;


//----------------------> TaskHandle_t <----------------------------
TaskHandle_t ManagerTask;
//------------------------------------------------------------------

//-----------------------> Prototipo de Funciones <-----------------
void Init_Indicadores_LED(void);
void Init_Configuracion_Inicial(void);
void Reset_Configuracion_Inicial(void);
void TaskManager();
static void ManagerTasks(void *parameter);
void FromFloatTobyte(byte* bytes, float dato);
void Config_Red_Serial(String Comando);

extern char Archivo_CSV_Contadores[200];
extern char Archivo_CSV_Eventos[200];
extern char Archivo_LOG[200];
//------------------------------------------------------------------

//---------------------------> Version de programa <----------------
uint8_t Version_Firmware_[]={1,0,1,1}; // 1000--> en  server 1.0
//------------------------------------------------------------------

void Init_Config(void)

{
   // pinMode(FLASH_RESET_Pin,INPUT_PULLUP);
    /* Define entradas */
   // pinMode(16,INPUT_PULLUP);
    pinMode(12,INPUT_PULLDOWN);
    pinMode(36,INPUT);
    pinMode(39,INPUT);
    pinMode(34,INPUT);
    
    pinMode(Hopper_Enable, INPUT_PULLDOWN);  // Reset_Config como Entrada.
    pinMode(Reset_Config, INPUT);   // Reset_Config como Entrada.
    /* Define Salidas*/
    pinMode(SD_Status, OUTPUT);     // SD Status Como Salida.
    pinMode(MCU_Status, OUTPUT);    // MCU_Status Como Salida.
    pinMode(WIFI_Status, OUTPUT);   // Wifi_Status como Salida.
    
    pinMode(33, OUTPUT);  
    digitalWrite(33,HIGH);

    pinMode (MCU_Status_2,OUTPUT);  // MCU_Status 2 Opcional.
    pinMode(Unlock_Machine,OUTPUT); // Rele Como salida.

    
    Init_Indicadores_LED();         //  Reset Indicadores LED'S LOW.
    //---------------------------> Version de programa <----------------
    Inicializa_Buffer_Eventos(); /*Inicializa Buffer de Eventos*/
    //------------------------------------------------------------------
    //------------------------> Config MCU <-------------------------
    setCpuFrequencyMhz(Clock_frequency); // Frecuencia de Nucleos 1 y 0.
    //---------------------------------------------------------------
    Serial.begin(115200); //  Inicializa Monitor Serial Debug
    //---------------------> Inicializa Indicadores <----------------
   // pinMode(5, OUTPUT); // Define como salida Selector de Esclavo SPI SS1 SD.
    
   // digitalWrite(5,HIGH); // Desactiva  Esclavo SPI SS1 SD.
    
    //---------------------------------------------------------------

    //--------------------> Setup Reloj Default <--------------------
    RTC.setTime(0, 12, 10, 9, 6, 2022);
   
    //---------------------------------------------------------------
  //  FLASH_RESET(); /* FLASH Reset  Manual*/
    //-------------------> Reset valores NVS <-----------------------
    Reset_Configuracion_Inicial();
    //--------------------> Init NVS Datos <-------------------------
    Init_Configuracion_Inicial(); // Inicializa Config de Memoria
    //---------------------------------------------------------------
   
   
   
    //--------------------> Config  WIFI <---------------------------
    CONNECT_WIFI();        // Inicializa  Modulo WIFI
    //------------------> Init Memoria SD <--------------------------
    Init_SD(); // Inicializa Memoria SD.
    //---------------------------------------------------------------
   // SD_FORMATT();
    

    CONNECT_SERVER_TCP();  // Inicializa Servidor TCP

    init_Comunicaciones(); // Inicializa Tareas TCP
    
    //-----------------> Config Comunicación Maquina <---------------
    Init_UART2(); // Inicializa Comunicación Maquina Puerto #2
    //---------------------------------------------------------------
    
    //--------------------> Task  SERVER <---------------------------
    Init_FTP_SERVER();
    //---------------------------------------------------------------
    //--------------------> Task Wifi <------------------------------
    Init_Wifi();
    //---------------------------------------------------------------
    //--------------------> Task Update  <---------------------------
    Init_Bootloader();
    //---------------------------------------------------------------
    //--------------------> Task Manager <---------------------------
    TaskManager(); // Inicia Manejador de Tareas de Verificación
    //---------------------------------------------------------------
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
    long conta = 0;

    for (;;)
    {
        while (Serial.available() > 0)
        {
            String Command = Serial.readString(); // read until timeout
            Config_Red_Serial(Command);
        }
        Tiempo_Actual = millis();
        if ((Tiempo_Actual - Tiempo_Previo) > 50)
        {
            Tiempo_Previo = Tiempo_Actual;
            MCU_State = !MCU_State;
            digitalWrite(MCU_Status, !MCU_State);
            digitalWrite(MCU_Status_2,!MCU_State);
        }
        if (WiFi.status() != WL_CONNECTED && eTaskGetState(Status_WIFI) == eSuspended)
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
                continue;
            }
        }
        if (!clientTCP.connected() && Configuracion.Get_Configuracion(Tipo_Conexion) && eTaskGetState(Status_SERVER) == eSuspended)
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
                continue;
            }
        }
        if (Variables_globales.Get_Variable_Global(Bootloader_Mode) == true && WiFi.status() == WL_CONNECTED && eTaskGetState(Modo_Bootloader) == eSuspended)
        {
            if (eTaskGetState(Modo_Bootloader) == eRunning)
            {
                Serial.println("------->>>>> Rum Task   Modo Bootloader");
            }
            else if (eTaskGetState(Modo_Bootloader) == eSuspended)
            {
                
                if(Variables_globales.Get_Variable_Global(Sincronizacion_RTC)==true)
                {
                    Serial.println("------->>>>> Resume Task  Modo Bootloader");
                    vTaskResume(Modo_Bootloader); // Inicia Modo Bootlader.
                    /*Captura información inicio Bootloader*/
                    String Hora = RTC.getTime();
                    String Fecha = RTC.getDate();
                    String Mes;
                    int month = RTC.getMonth();
                    switch (month)
                    {
                    case 0:
                        Mes = "01";
                        break;
                    case 1:
                        Mes = "02";
                        break;
                    case 2:
                        Mes = "03";
                        break;
                    case 3:
                        Mes = "04";
                        break;
                    case 4:
                        Mes = "05";
                        break;
                    case 5:
                        Mes = "06";
                        break;
                    case 6:
                        Mes = "07";
                        break;
                    case 7:
                        Mes = "08";
                        break;
                    case 8:
                        Mes = "09";
                        break;
                    case 9:
                        Mes = "10";
                        break;
                    case 10:
                        Mes = "11";
                        break;
                    case 11:
                        Mes = "12";
                        break;
                    default:
                        break;
                    }
                    Serial.println("Guardando Fecha Bootloader.....");
                    NVS.begin("Config_ESP32", false);
                    uint8_t Fecha_Modo_Bootloader[] = {Hora[0], Hora[1], Hora[3], Hora[4], Hora[6], Hora[7], Fecha[9], Fecha[10], Mes[0], Mes[1], Fecha[14], Fecha[15]};
                    NVS.putBytes("Fecha_Boot", Fecha_Modo_Bootloader, sizeof(Fecha_Modo_Bootloader));

                    size_t Fecha_len = NVS.getBytesLength("Fecha_Boot");
                    uint8_t Datos_Fecha_B[Fecha_len];
                    NVS.getBytes("Fecha_Boot", Datos_Fecha_B, sizeof(Datos_Fecha_B));
                    NVS.end();
                }
            }
        }
        if (Variables_globales.Get_Variable_Global(SD_INSERT) == false && Variables_globales.Get_Variable_Global(Ftp_Mode) == true)
        {
            Serial.println("Memoria SD Desconectada..");
            Serial.println("Desconecta Modo FTP");
            Variables_globales.Set_Variable_Global(Ftp_Mode, false);
        }

        if(WiFi.status() != WL_CONNECTED&&Variables_globales.Get_Variable_Global(Ftp_Mode) == true)
        {
            Variables_globales.Set_Variable_Global(Ftp_Mode, false);
        }
        
        if (WiFi.status() == WL_CONNECTED && Variables_globales.Get_Variable_Global(Bootloader_Mode))
        {
           ArduinoOTA.handle2();
        }
        
        delay(100);
        vTaskDelay(1000);
    }
    vTaskDelay(10);
}

void Init_Indicadores_LED(void)
{
    digitalWrite(WIFI_Status, HIGH);
    digitalWrite(Unlock_Machine,HIGH);
   // digitalWrite(SD_ChipSelect, LOW);
    digitalWrite(SD_Status, LOW);
    digitalWrite(MCU_Status, LOW);
    digitalWrite(MCU_Status_2, HIGH);
    digitalWrite(33,HIGH);
    digitalWrite(32,HIGH);
}

void Init_Configuracion_Inicial(void)
{
    
    
    Serial.println("\n");
    Serial.println("Inicializando modulo...");

    NVS.begin("Config_ESP32", false);

    if(!NVS.isKey("Ver_Fir")) // Configura versión firmware actual.
    {
        Serial.println("Guarda Version de firmware actual");
        NVS.putBytes("Ver_Fir", Version_Firmware_, sizeof(Version_Firmware_));
    }
        size_t Ver_fir_len = NVS.getBytesLength("Ver_Fir");
        uint8_t Version[Ver_fir_len];
        NVS.getBytes("Ver_Fir", Version, sizeof(Version));
        Serial.print("Version del software: ");
    
        Serial.print(Version[0]);
        if(Version[1]==0){Serial.print(".");}
        Serial.print(Version[2]); 
        if(Version[3]!=0){Serial.print(Version[3]);}
        if(Version[1]!=0){Serial.print(Version[1]);}
        Serial.println();

        if (NVS.isKey("Fecha_Boot"))
        {
        size_t Fecha_len = NVS.getBytesLength("Fecha_Boot");
        uint8_t Datos_Fecha_B[Fecha_len];
        NVS.getBytes("Fecha_Boot", Datos_Fecha_B, sizeof(Datos_Fecha_B));
        String Fecha_Bootlader = String(char(Datos_Fecha_B[6])) + String(char(Datos_Fecha_B[7])) + "/" + String(char(Datos_Fecha_B[8])) + String(char(Datos_Fecha_B[9])) + "/" + String(char(Datos_Fecha_B[10])) + String(char(Datos_Fecha_B[11]));
        if (Fecha_Bootlader != NULL)
        {
           Serial.println("Ultima fecha Bootloader: " + Fecha_Bootlader);
        }
        }

        if(int(Version[0])!=int(Version_Firmware_[0]) || int(Version[1])!=int(Version_Firmware_[1]) || int(Version[2])!=int(Version_Firmware_[2]) ||int(Version[3])!=int(Version_Firmware_[3])) //  Si la version guardada en memoria es diferente a la actual del codigo
        {
            Serial.println(" Nueva version de software detectada ");
          
            NVS.putBytes("Ver_Fir",Version_Firmware_, sizeof(Version_Firmware_));
            Serial.print(Version_Firmware_[0]); // 1
            if (Version_Firmware_[1] == 0)
            {
                Serial.print(".");
            }                        
            Serial.print(Version_Firmware_[2]); 

            if (Version_Firmware_[3] != 0)
            {
                Serial.print(Version_Firmware_[3]);
            }
            if (Version_Firmware_[1] != 0)
            {
                Serial.print(Version_Firmware_[1]);
            }
            Serial.println();
        }

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
        uint8_t ip_server[] = {192, 168, 5, 200};
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
        char name[17] = {'M', 'a', 'q', '_', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
        NVS.putString("Name_Maq", name);
    }

    if (!NVS.isKey("SSID_DESA")) // Configura SSID de conexion WIFI
    {
        Serial.println("Guardando SSID por defecto...");
        String ssid = "GLOBUS_ONLINEW";
        //String ssid = "GLOBUS-DESARROLLO";
        NVS.putString("SSID_DESA", ssid); 
    }

    if (!NVS.isKey("PASS_DESA")) // Configura PASSWORD de conexion WIFI
    {
        Serial.println("Guardando Password por defecto...");
          String password = "Globus#OnlineW324";
        //String password = "Globus2020*";
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
        // 7 = IGT Riel
        // 8 = IGT Riel Con Bill
        // 9 = Mecanicas
        // 10 = Poker-solo-SAS 5 contadores
        //
        uint16_t tipo_maq = 5;
        NVS.putUInt("TYPE_MAQ", tipo_maq);
    }
    if (!NVS.isKey("COM"))
    {
        Serial.println("Puerto COM1 por defecto...");
        // 1 = COM1,
        // 2 = COM2,
        uint16_t Port_COM = 1;
        NVS.putUInt("COM", Port_COM);
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
    case 7:
        Serial.println("IGT Riel");
        break;
    case 8:
        Serial.println("IGT Riel Con Bill");
        break;
    case 9:
        Serial.println("Mecanicas");
        break;
    case 10:
        Serial.println("Simple-4 contadores");
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
    while (digitalRead(Reset_Config) == LOW)
    {
        digitalWrite(WIFI_Status,LOW);
        if (millis() > 10000)
        {
            Serial.println("Reset activado............................");
            for (int i = 0; i < 50; i++)
            {
                MCU_State = !MCU_State;
                digitalWrite(MCU_Status, !MCU_State);
                digitalWrite(MCU_Status_2,!MCU_State);
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
            Serial.println("Reset finalizado en............");
            delay(1000);
            ESP.restart();
        }
    }
    return;
}

void FromFloatTobyte(byte* bytes, float dato)
{
    int length= sizeof(float);
    for(int i=0; i <length;i++)
    {
        bytes[i]=((byte*)&dato)[i];
    }
}

void Config_Red_Serial(String Comando)
{
    String Red="";
    bool Flash_OK_=false;
    unsigned long Tm=0;
    unsigned long Tf=0;
    int inter_v=20000;

    if(Comando[0]=='R'&&Comando[1]=='E'&&Comando[2]=='D'&&Comando[4]=='-')
    {

        if (Comando[5]==NULL ||Comando[5]>53)
        {
            Serial.println("------->Comando no identificado");
        }
        else
        {
            /*IP LOCAL */
            NVS.begin("Config_ESP32", false);
            size_t ip_len = NVS.getBytesLength("Dir_IP");
            char IP[ip_len];
            NVS.getBytes("Dir_IP", IP, ip_len);
            IP[2] = int(Comando[5] - 48);
            NVS.putBytes("Dir_IP", IP, sizeof(IP));

            /*IP SERVER*/
            size_t ip_server_len1 = NVS.getBytesLength("Dir_IP_Serv");
            char IP_Server1[ip_server_len1];
            NVS.getBytes("Dir_IP_Serv", IP_Server1, ip_server_len1);
            IP_Server1[2] = int(Comando[5] - 48);
            NVS.putBytes("Dir_IP_Serv", IP_Server1, sizeof(IP_Server1));


            /*IP GET*/
            size_t ip_gw_len=NVS.getBytesLength("Dir_IP_GW");
            char IP_GW[ip_gw_len];
            NVS.getBytes("Dir_IP_GW",IP_GW, sizeof(IP_GW));
            IP_GW[2]=int(Comando[5]-48);
            NVS.putBytes("Dir_IP_GW",IP_GW, sizeof(IP_GW));
    
            /*REDES*/

            if(Comando[3]==48){

                Serial.println("CONFIGURA---GLOBUS_DESARROLLO");
                String ssid = "GLOBUS-DESARROLLO";
                NVS.putString("SSID_DESA", ssid);
                String password = "Globus2020*";
                NVS.putString("PASS_DESA", password);
                NVS.end();
                delay(500);
                ESP.restart();
            }
             else if(Comando[3]==49)
            {
                Serial.println("CONFIGURA---GLOBUS_ONLINEW");
                String ssid = "GLOBUS_ONLINEW";
                NVS.putString("SSID_DESA", ssid);
                String password = "Globus#OnlineW324";
                NVS.putString("PASS_DESA", password);
                NVS.end();
                delay(500);
                ESP.restart();
            }
            else if(Comando[3]==50)
            {
                Serial.println("CONFIGURA---GLOBUS_ONLINEW2");
                String ssid = "GLOBUS_ONLINEW2";
                NVS.putString("SSID_DESA", ssid);
                String password = "Globus#OnlineW324";
                NVS.putString("PASS_DESA", password);
                NVS.end();
                delay(500);
                ESP.restart();
            }
            else if(Comando[3]==51)
            {
                Serial.println("CONFIGURA---GLOBUS_ONLINEW3");
                String ssid = "GLOBUS_ONLINEW3";
                NVS.putString("SSID_DESA", ssid);
                String password = "Globus#OnlineW324";
                NVS.putString("PASS_DESA", password);
                NVS.end();
                delay(500);
                ESP.restart();
            }
            else if(Comando[3]==52)
            {
                Serial.println("CONFIGURA---GLOBUS_ONLINEW4");
                String ssid = "GLOBUS_ONLINEW4";
                NVS.putString("SSID_DESA", ssid);
                String password = "Globus#OnlineW324";
                NVS.putString("PASS_DESA", password);
                NVS.end();
                delay(500);
                ESP.restart();
            }
            else if(Comando[3]==53)
            {
                Serial.println("CONFIGURA---GLOBUS_ONLINEW5");
                String ssid = "GLOBUS_ONLINEW5";
                NVS.putString("SSID_DESA", ssid);
                String password = "Globus#OnlineW324";
                NVS.putString("PASS_DESA", password);
                NVS.end();
                delay(500);
                ESP.restart();
            }
            else if(Comando[3]==54)
            {
                Serial.println("CONFIGURA---GLOBUS_ONLINEW6");
                String ssid = "GLOBUS_ONLINEW6";
                NVS.putString("SSID_DESA", ssid);
                String password = "Globus#OnlineW324";
                NVS.putString("PASS_DESA", password);
                NVS.end();
                delay(500);
                ESP.restart();
            }
             else if(Comando[3]==55)
            {
                Serial.println("CONFIGURA---GLOBUS_ONLINEW7");
                String ssid = "GLOBUS_ONLINEW7";
                NVS.putString("SSID_DESA", ssid);
                String password = "Globus#OnlineW324";
                NVS.putString("PASS_DESA", password);
                NVS.end();
                delay(500);
                ESP.restart();
            }
             else if(Comando[3]==56)
            {
                Serial.println("CONFIGURA---GLOBUS_ONLINEW8");
                String ssid = "GLOBUS_ONLINEW8";
                NVS.putString("SSID_DESA", ssid);
                String password = "Globus#OnlineW324";
                NVS.putString("PASS_DESA", password);
                NVS.end();
                delay(500);
                ESP.restart();
            }
             else if(Comando[3]==57)
            {
                Serial.println("CONFIGURA---GLOBUS_IMPERIAL");
                String ssid = "GLOBUS_IMPERIAL";
                NVS.putString("SSID_DESA", ssid);
                String password = "Globus#OnlineW324";
                NVS.putString("PASS_DESA", password);
                NVS.end();
                delay(500);
                ESP.restart();
            }else{
                Serial.println("------->Comando no identificado");
                NVS.end();
            }
            NVS.end();
        }
    }else{

        if(Comando=="SERVER200")
        {
            NVS.begin("Config_ESP32", false);
            size_t ip_server_len1 = NVS.getBytesLength("Dir_IP_Serv");
            char IP_Server1[ip_server_len1];
            NVS.getBytes("Dir_IP_Serv", IP_Server1, ip_server_len1);
            IP_Server1[3] =200;
            NVS.putBytes("Dir_IP_Serv", IP_Server1, sizeof(IP_Server1));
            NVS.end();
            delay(500);
            ESP.restart();
        }
        /*
        else if(Comando[0]=='R' && Comando[1]=='E' && Comando[2]=='D')
        {
            int pos;
            char Ssd;
            char Pass;
                for(int i=0; i<sizeof(Comando);i++)
                {
                    if(Comando[i]=='=')
                    {
                        if(Comando[i]=='|')
                        {
                            break;
                        }
                        Ssd=Ssd+i;
                        
                    }
                    pos++;
                }
               
                if(Comando[pos]=='P'&&Comando[pos+1]=='A' && Comando[pos+2]=='S'&&Comando[pos+3]=='S')
                {
                    for (int i = pos+4; i < sizeof(Comando); i++)
                    {
                        if (Comando[i] == '=')
                        {
                            if (Comando[i] == '|')
                            {
                                break;
                            }
                            Pass = Pass + i;
                        }
                    }
                }
                pos=0;
                Serial.println(Pass);
                Serial.println(Ssd);
        }
        */
        else if(Comando=="FLASHOK")
        {
            nvs_flash_erase();
            nvs_flash_init();
            while (1)
            {
                Serial.println("Borrando datos...");
                Tm = millis();
                if ((Tm - Tf) >= inter_v)
                {
                    Tf=Tm;
                    delay(500);
                    ESP.restart();
                    break;
                }
            }
            delay(500);
            ESP.restart();
        }
        else if(Comando[0]=='P' &&Comando[1]=='U'&&Comando[2]=='E'&&Comando[3]=='R'&&Comando[4]=='T'&&Comando[5]=='O')
        {
            if(Comando[6]==49)
            {
                /*puerto RS232 1*/
                NVS.begin("Config_ESP32", false);
                uint16_t Port_COM = 1;
                NVS.putUInt("COM", Port_COM);
                NVS.end();
                delay(500);
                ESP.restart();
            }
            if(Comando[6]==50)
            {
                /*puerto RS232 2*/
                NVS.begin("Config_ESP32", false);
                uint16_t Port_COM = 2;
                NVS.putUInt("COM", Port_COM);
                NVS.end();
                delay(500);
                ESP.restart();
            }
        }
        else if(Comando=="SERVER100")
        {
            NVS.begin("Config_ESP32", false);
            size_t ip_server_len1 = NVS.getBytesLength("Dir_IP_Serv");
            char IP_Server1[ip_server_len1];
            NVS.getBytes("Dir_IP_Serv", IP_Server1, ip_server_len1);
            IP_Server1[3] =100;
            NVS.putBytes("Dir_IP_Serv", IP_Server1, sizeof(IP_Server1));
            NVS.end();
            delay(500);
            ESP.restart();
        }
        else if(Comando=="SERVER204")
        {
            NVS.begin("Config_ESP32", false);
            size_t ip_server_len1 = NVS.getBytesLength("Dir_IP_Serv");
            char IP_Server1[ip_server_len1];
            NVS.getBytes("Dir_IP_Serv", IP_Server1, ip_server_len1);
            IP_Server1[3] =204;
            NVS.putBytes("Dir_IP_Serv", IP_Server1, sizeof(IP_Server1));
            NVS.end();
            delay(500);
            ESP.restart();
        }else{
            Serial.println("------->Comando no identificado");
        } 
    }
}