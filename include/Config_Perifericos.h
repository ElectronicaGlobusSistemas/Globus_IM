#include <Arduino.h>
#include "Memory_SD.h"
#include "RTC.h"
#include "nvs_flash.h"
#include <SPI.h>
#include <SD.h>
#include "RFID.h"
#include "AutoUpdate.h"
#include "Web_Config.h"
#include "Event_Real_Time.h"
#include <ESP32Ping.h>
//#define Debug_Task
//-------------------> Parametros <-------------------------------
#define Clock_frequency  240//240//
#define MCU_Status       2
#define WIFI_Status      15
#define Reset_Config     27
#define Hopper_Enable    14
#define MCU_Status_2     25
#define Unlock_Machine   26
#define FLASH_RESET_Pin  35


#define Sensor_Open_Door 34
#define Sensor_Stacker   35
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
Event_Real_Time Eventos_Hardware;
AutoUpdate UpdateOTA;
Web_Config Task_Web_Config;
Web_Config Web_Info_Config;

//----------------------> TaskHandle_t <----------------------------
TaskHandle_t ManagerTask;

extern TaskHandle_t CommandProcess;

extern Buffer_RX_AFT Buffer_Cashless;
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
extern bool Termina_Bootlader_Timeout;

unsigned long TO=0;
unsigned long T02=0;
int Interv=60000;
bool Update_Enable_DTime=false;



unsigned long TIMEOUT_WiFi_CONNECT=0;
unsigned long TIMEOUT_WiFi_CONNECT_2=0;
int Interval_Connect=10000;
bool WL_DISCONNECT_OK=false;

int extern Inactividad_Usuario_Player_Tracking;
int extern Tiempo_Transmision_En_Juego;
int extern Tiempo_Transmision_No_Juego;
int extern Tiempo_Inactividad_Maquina;
uint32_t extern Dia_Guarda_Logs;
//------------------------------------------------------------------
//---------------------------> Version de programa <----------------

/* Mayor (Major): Se incrementa cuando hay cambios significativos que podrían no ser compatibles con versiones anteriores.
Menor (Minor): Se incrementa cuando se añaden nuevas funcionalidades de forma compatible con versiones anteriores.
Parche (Patch): Se incrementa cuando se corrigen errores o se hacen mejoras menores.
Build: Se puede usar para identificar compilaciones específicas o revisiones menores que no afectan al comportamiento del software.
*/
uint8_t Version_Firmware_[]={2,1,5,0};
uint8_t Address_Device_TFT_Display[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//------------------------------------------------------------------
void Fecha_Update(bool Enable);


void Init_Config(void)
{
   // pinMode(FLASH_RESET_Pin,INPUT_PULLUP);
    /* Define entradas */
   // pinMode(16,INPUT_PULLUP);
   // pinMode(13,OUTPUT);
    pinMode(12,INPUT_PULLDOWN);
    pinMode(36,INPUT);
    pinMode(39,INPUT);
   //pinMode(34,INPUT_PULLUP);
    pinMode(Sensor_Stacker,INPUT_PULLUP);
    pinMode(Sensor_Open_Door,INPUT);
    pinMode(Hopper_Enable, INPUT_PULLDOWN);  // Reset_Config como Entrada.
    pinMode(Reset_Config, INPUT);   // Reset_Config como Entrada.
    /* Define Salidas*/
    pinMode(SD_Status, OUTPUT);     // SD Status Como Salida.
    pinMode(MCU_Status, OUTPUT);    // MCU_Status Como Salida.
    pinMode(WIFI_Status, OUTPUT);   // Wifi_Status como Salida.
    pinMode(5,OUTPUT);
    pinMode(33, OUTPUT);  
    digitalWrite(33,HIGH);
    pinMode (MCU_Status_2,OUTPUT);  // MCU_Status 2 Opcional.
    pinMode(Unlock_Machine,OUTPUT); // Rele Como salida.
    
    //---------------------> Inicializa Indicadores <----------------
    Init_Indicadores_LED();         //  Reset Indicadores LED'S LOW.
    //---------------------------> Version de programa <----------------
    Inicializa_Buffer_Eventos(); /*Inicializa Buffer de Eventos*/
    //------------------------------------------------------------------
    //------------------------> Config MCU <-------------------------
    setCpuFrequencyMhz(Clock_frequency); // Frecuencia de Nucleos 1 y 0.
    //---------------------------------------------------------------
    Serial.begin(115200); //  Inicializa Monitor Serial Debug
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
    Init_RFID(); /* Inicializa Modulo RFID*/
    Init_SD(); // Inicializa Memoria SD Inicializa Bus SPI.
    
    
    //------------------> AutoUpdate <-------------------------------
  //  UpdateOTA.Init_AutoUpdate("","","",Version_Firmware_); /* Inicializa URL */
  //  UpdateOTA.Auto_Update(false); /* Verifica Actualizacion */
    //---------------------------------------------------------------
    
    //---------------------------------------------------------------
    //-------------------> Cliente UDP-TCP <-------------------------
    CONNECT_SERVER_TCP();  // Inicializa Servidor TCP
    init_Comunicaciones(); // Inicializa Tareas TCP
    //-----------------> Config Comunicación Maquina <---------------
    Init_UART2(); // Inicializa Comunicación Maquina Puerto 1 o 2
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
        "TASK MANAGER",            //  Nombre de la tarea //10000
        8000,                     //  Tamaño de stack en palabras (memoria)
        NULL,                      //  Entrada de parametros
        configMAX_PRIORITIES - 10, //  Prioridad de la tarea
        &ManagerTask,              //  Manejador de la tarea
        0);                        //  Core donde se ejecutara la tarea
}

int buttonState = HIGH;    // Estado actual del botón
int lastButtonState = HIGH;  // Estado anterior del botón
unsigned long lastDebounceTime = 0;  // Último tiempo de rebote del botón
unsigned long debounceDelay = 50;    // Tiempo de rebote del botón
unsigned long buttonPressStartTime = 0;  // Tiempo de inicio de la pulsación del botón
unsigned long buttonHoldDuration = 0; 
bool Wifi_State_AP = LOW;
unsigned long Ping_Counter=0;
unsigned long TimerPing=0;
#define MAX_PING_TEST_HIGH      5
#define MAX_PING_TEST_MEDIUM    5
#define MAX_PING_TEST_LOW       5

void PuntoAcceso_On(int Reset_Pin)
{
   // Serial.println(WiFi.softAPgetStationNum());
     int reading = digitalRead(Reset_Pin);

        if (reading != lastButtonState)
        {
            lastDebounceTime = millis();
        }

        if ((millis() - lastDebounceTime) > debounceDelay)
        {
            if (reading != buttonState)
            {
                buttonState = reading;

                if (buttonState == LOW)
                {
                    // Botón presionado
                    buttonPressStartTime = millis();
                }
            }
        }

        // Verificar si ha pasado suficiente tiempo desde el inicio de la pulsación
        unsigned long currentTime = millis();
        if (buttonState == LOW && (currentTime - buttonPressStartTime >= 10000))
        {

            for (int i = 0; i < 25; i++)
            {
                Wifi_State_AP = !Wifi_State_AP;
                digitalWrite(WIFI_Status, !Wifi_State_AP);
                delay(100);
            }
            wifi_mode_t currentMode = WiFi.getMode();
           // Serial.println( WiFi.getMode());
            if (currentMode==WIFI_MODE_APSTA)
            {
                WiFi.softAPdisconnect();
                WiFi.mode(WIFI_MODE_STA);
                Variables_globales.Set_Variable_Global(Access_Point_Mode,false);
            }else{
                WiFi.mode(WIFI_MODE_APSTA);
                Task_Web_Config.Init_Web_Server();
                Variables_globales.Set_Variable_Global(Access_Point_Mode,true);
            }
            buttonPressStartTime=currentTime;


            if(WiFi.status()==WL_CONNECTED)
            {
                digitalWrite(WIFI_Status, HIGH);
            }else{
                if(WiFi.status()!=WL_CONNECTED)
                {
                    digitalWrite(WIFI_Status, LOW);
                }
            }
        }
        lastButtonState = reading;
}

void Ping_Test(unsigned long Timeout, bool Sesion_Act, bool Status_Maq)
{

    if (WiFi.status() == WL_CONNECTED)
    {
        if ((millis() - TimerPing) >= Timeout)
        {
            IPAddress gateway = WiFi.gatewayIP();
            //Serial.println(gateway);
            if (Ping.ping((gateway)))
            {
                Ping_Counter = 0;
            }
            else
                Ping_Counter++;

            // unsigned Intentos = MAX_PING_TEST_HIGH;

            // if (Sesion_Act)
            //     Intentos = MAX_PING_TEST_LOW;
            // else if (Status_Maq)
            //     Intentos = MAX_PING_TEST_MEDIUM;
            // else
            //     Intentos = MAX_PING_TEST_HIGH;

            if (Ping_Counter > 5)
            {
                Ping_Counter = 0;
                WiFi.disconnect(true); /* Lanza tarea de reconexion WiFi */
            }
            TimerPing = millis();
        }
    }
    else
    {
        Ping_Counter = 0;
    }
}

static void ManagerTasks(void *parameter)
{
    unsigned long Tiempo_Actual = 0;
    unsigned long Tiempo_Previo = 0;
    bool MCU_State = LOW;
    long conta = 0;
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(1000);

    

    for (;;)
    {
        TIMEOUT_WiFi_CONNECT=millis();
        //---------------------------------> Config via Serial <-----------------------------------------
        // if (Serial.available() > 0)
        // {
        //     String Command = Serial.readString(); // read until timeout
        //     Config_Red_Serial(Command);
        //     Serial.flush();
        // }
        //------------------------------------------------------------------------------------------------
        //-----------------------------> MCU piloto <-----------------------------------------------------
        Tiempo_Actual = millis();
        if ((Tiempo_Actual - Tiempo_Previo) > 50)
        {
            Tiempo_Previo = Tiempo_Actual;
            MCU_State = !MCU_State;
            digitalWrite(MCU_Status, !MCU_State);
            digitalWrite(MCU_Status_2,!MCU_State);
        }
        //--------------------------------------------------------------------------------------------------

        //-----------------------------> Verifica conexion WIFI <--------------------------------------------
        
        if (WiFi.status() != WL_CONNECTED && !Variables_globales.Get_Variable_Global(Access_Point_Mode))
        {
            Variables_globales.Set_Variable_Global(Verifica_Conexion_WIFI,true);
        }

        if(Variables_globales.Get_Variable_Global(Verifica_Conexion_WIFI) && !Variables_globales.Get_Variable_Global(Access_Point_Mode))
        {
            if(!WL_DISCONNECT_OK)
            {
                TIMEOUT_WiFi_CONNECT_2=TIMEOUT_WiFi_CONNECT;
                RECONECT_WIFI_ESP(); /* Ejecuta reconexion WiFi*/
                WL_DISCONNECT_OK=true;
                Info_Cashless.Log(RTC,"PERDIDA_CONEXION_WIFI","TASK_RECUPERA_CONEXION");
            }
            if((TIMEOUT_WiFi_CONNECT-TIMEOUT_WiFi_CONNECT_2)>=Interval_Connect)
            {
                RECONECT_WIFI_ESP(); 
                TIMEOUT_WiFi_CONNECT_2=TIMEOUT_WiFi_CONNECT;
            }
        }
        //--------------------------------------------------------------------------------------------------------

        //-------------------------------------> Verifica Socket TCP <--------------------------------------------
        if (!clientTCP.connected() && Configuracion.Get_Configuracion(Tipo_Conexion) && eTaskGetState(Status_SERVER) == eSuspended)
        {
            if (eTaskGetState(Status_SERVER) == eRunning)
            {
                #ifdef Debug_Task
                Serial.println("------->>>>> Rum Task   SERVER TCP");
                #endif
                continue;
            }
            else if (eTaskGetState(Status_SERVER) == eSuspended)
            {
                #ifdef Debug_Task
                Serial.println("------->>>>> Resume Task  SERVER TCP");
                #endif
                vTaskResume(Status_SERVER); // Inicia Tarea  TCP.
                continue;
            }
        }
        //-------------------------------------------------------------------------------------------------------


        
        //----------------------------------------------> Verifica Update <--------------------------------------
        // if (Variables_globales.Get_Variable_Global(Bootloader_Mode) == true && WiFi.status() == WL_CONNECTED && eTaskGetState(Modo_Bootloader) == eSuspended)
        // {
        //     if (eTaskGetState(Modo_Bootloader) == eRunning)
        //     {
        //         #ifdef Debug_Task
        //         Serial.println("------->>>>> Rum Task   Modo Bootloader");
        //         #endif
        //     }
        //     else if (eTaskGetState(Modo_Bootloader) == eSuspended)
        //     {
                
        //         if(Variables_globales.Get_Variable_Global(Sincronizacion_RTC)==true)
        //         {
        //             #ifdef Debug_Task
        //             Serial.println("------->>>>> Resume Task  Modo Bootloader");
        //             #endif
        //             vTaskResume(Modo_Bootloader); // Inicia Modo Bootlader.
        //         }
        //     }
        // }
        //-------------------------------------------------------------------------------------------------------
         //--------------------------------------> Verifica Status SD en modo FTP <-------------------------------
        if (Variables_globales.Get_Variable_Global(SD_INSERT) == false && Variables_globales.Get_Variable_Global(Ftp_Mode) == true)
        {
            #ifdef Debug_Task
            Serial.println("Memoria SD Desconectada..");
            Serial.println("Desconecta Modo FTP");
            #endif
            Variables_globales.Set_Variable_Global(Ftp_Mode, false);
        }
        //------------------------------------------------------------------------------------------------------
         //---------------------------------> Verifica Timeout inactividad Bootloader <--------------------------
        // if(Termina_Bootlader_Timeout)
        // {
        //     #ifdef Debug_Task
        //     Serial.println("Tiempo de espera de actualización agotado...");
        //     #endif
        //     Variables_globales.Set_Variable_Global(Bootloader_Mode,false);
        //     vTaskSuspend(Modo_Bootloader);
        //     Termina_Bootlader_Timeout=false;
        // }
        //-------------------------------------------------------------------------------------------------------

         //----------------------------------> Verifica Modo FTP <------------------------------------------------
        if(Variables_globales.Get_Variable_Global(Ftp_Mode) == true && WiFi.status() != WL_CONNECTED)
        {
            Variables_globales.Set_Variable_Global(Ftp_Mode, false);
        }
        //-------------------------------------------------------------------------------------------------------

        //---------------------------------> Activa Actualizacion Manual <---------------------------------------
        if (WiFi.status() == WL_CONNECTED && Variables_globales.Get_Variable_Global(Bootloader_Mode))
        {
            if(!Update_Enable_DTime)
            {
                Fecha_Update(true);
                Update_Enable_DTime=true;
            }
           ArduinoOTA.handle();
        
        }

        if(WiFi.status()!=WL_CONNECTED && Variables_globales.Get_Variable_Global(Bootloader_Mode))
        {
            Variables_globales.Set_Variable_Global(Bootloader_Mode,false);
            
        }

        if(WiFi.status()==WL_CONNECTED && Variables_globales.Get_Variable_Global(AutoUPDATE_OK))
        {
            UpdateOTA.Confirmacion_ACK_HTTPS(URL_OK, RES_URL); /* URL OK */
            delay(10);
            UpdateOTA.Auto_Update(Variables_globales.Get_Variable_Global(Flag_Maquina_En_Juego), Variables_globales.Get_Variable_Global(Flag_Hopper_Enable), flag_billete_insertado, flag_premio_pagado_cashout, Variables_globales.Get_Variable_Global(Flag_Sesion_RFID), Convert_Char_To_Int10(contadores.Get_Contadores_Char(Current_Credits)),Variables_globales.Get_Variable_Global(Access_Point_Mode)); /* Agregar parametros para  verificar que la maquina no este en juego */
            Variables_globales.Set_Variable_Global(AutoUPDATE_OK, false);
        }

        PuntoAcceso_On(Reset_Config);

        if(Variables_globales.Get_Variable_Global(Enable_Mechanical_Events))
            Eventos_Hardware.EVENT_REAL_TIME();
        //EVENT_REAL_TIME();
        //--------------------------------------------------------------------------------------------------------
        //delay(100);
        //vTaskDelay(1000);

        // UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(Encuestas);
        // Serial.print("Minimo espacio libre en stack: ");
        // Serial.println(uxHighWaterMark);
        // UBaseType_t uxHighWaterMark2 = uxTaskGetStackHighWaterMark(RecepcionRS232);
        // Serial.print("Minimo espacio libre en stack RS232: ");
        // Serial.println(uxHighWaterMark2);
       
        Ping_Test(2000,false,false);

        //FtpFast();
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
    vTaskDelay(10);
}

void Fecha_Update(bool Enable)
{

    if (Enable)
    {
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

void Default_DateTime_Update(bool Enable)
{

    if (Enable)
    {
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
       
    }
}

void Init_Indicadores_LED(void)
{
    digitalWrite(WIFI_Status, LOW);
    digitalWrite(Unlock_Machine,HIGH);
   // digitalWrite(SD_ChipSelect, LOW);
    digitalWrite(SD_Status, LOW);
    digitalWrite(MCU_Status, LOW);
    digitalWrite(MCU_Status_2, LOW);
    digitalWrite(33,HIGH);
    digitalWrite(32,HIGH);
}

void Init_Configuracion_Inicial(void)
{
    
    
    Serial.println("\n");
    Serial.println("Inicializando modulo...");
    // Borrar particiones creadas en NVS
    // nvs_flash_erase();
    // nvs_flash_init();

    NVS.begin("Config_ESP32", false);
    if(!NVS.isKey("Ver_Fir")) // Configura versión firmware actual.
    {
        Serial.println("Guarda Version de firmware actual");
        NVS.putBytes("Ver_Fir", Version_Firmware_, sizeof(Version_Firmware_));
    }
        size_t Ver_fir_len = NVS.getBytesLength("Ver_Fir");
        uint8_t Version[Ver_fir_len];
        NVS.getBytes("Ver_Fir", Version, sizeof(Version));
        Serial.print("Version de firmware: ");
    
        for(int i=0; i<4;i++)
        {
            Serial.print(Version[i]);

            if(i<3)
            {
                Serial.print(".");
            }
        }
        Serial.println();

        if (!NVS.isKey("Fecha_Boot"))
        {
            Serial.println(" Fecha Booloader por defecto....");
            Default_DateTime_Update(true);
        }

        if(int(Version[0])!=int(Version_Firmware_[0]) || int(Version[1])!=int(Version_Firmware_[1]) || int(Version[2])!=int(Version_Firmware_[2]) ||int(Version[3])!=int(Version_Firmware_[3])) //  Si la version guardada en memoria es diferente a la actual del codigo
        {
            Serial.println(" Nueva version de software detectada..... ");
          
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

    if (!NVS.isKey("Dir_IP_Serv2")) // Configura la IP de servidor
    {
        Serial.println("Guardando IP Server 2 por defecto...");
        uint8_t ip_server2[] = {192, 168, 5, 200};
        NVS.putBytes("Dir_IP_Serv2", ip_server2, sizeof(ip_server2));
    }


    if(!NVS.isKey("Dns_Primary"))
    {
        uint8_t Dns_One[] = {8, 8, 8, 8}; // optional
        NVS.putBytes("Dns_Primary", Dns_One, sizeof(Dns_One));
    }

    if(!NVS.isKey("Dns_Secondary"))
    {
        uint8_t Dns_Two[] = {8, 8, 4, 4}; // optional
        NVS.putBytes("Dns_Secondary", Dns_Two, sizeof(Dns_Two));
    }


    if (!NVS.isKey("Socket")) // Configura el numero de socket
    {
        Serial.println("Guardando Puerto por defecto...");
        uint16_t port = 1001;
        NVS.putUInt("Socket", port);
    }

    if (!NVS.isKey("Socket2")) // Configura el numero de socket
    {
        Serial.println("Guardando Puerto por defecto 2...");
        uint16_t port2 = 1005;
        NVS.putUInt("Socket2", port2);
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
        // 9 = Mecanicas 2 contadores
        // 10 = Poker-solo-SAS 5 contadores
        // 11 = Aristocrat Australiana
        // 12 = Simple No cancel
        // 13 = Poker_Ertech_Plus (Simple-No creditos)
        // 14 = Poker_Ertech_Slot (IGT)
        // 15 = Mecanicas 4 contadores 
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
    if(!NVS.isKey("RHNSAS"))
    {
        Serial.println("Reset Handpay no SAS no habilitado por defecto...");
        // Reset handpay no SAS habilitado = true
        // Reset handpay no SAS deshabilitado = false
        bool RESET_HANDPAY_RELE=false;
        NVS.putBool("RHNSAS", RESET_HANDPAY_RELE);
    }

    if(!NVS.isKey("LECTOR"))
    {
        Serial.println("Lector RFID habilitado por defecto....");
        bool Enable_Lector=true;
        NVS.putBool("LECTOR", Enable_Lector);
    }

    if(!NVS.isKey("Connect_W"))
    {
        Serial.println("Intentos_WIFI: 0");
        // 1 = Fallo 1,
        // 2 = Fallo 2,
        // 3 = Fallo 3,
        // 4 = Fallo 4,
        // 5 = Fallo 5
        /* Ignora  el reset */
        uint16_t Intento_Conexion = 0;
        NVS.putUInt("Connect_W", Intento_Conexion);
    }

    if(!NVS.isKey("TimePlayer"))
    {
        int Timer_Player=90000;
        Serial.println("Timeout_Player_Tracking por defecto....");
        NVS.putUInt("TimePlayer",Timer_Player);
    }

    if(!NVS.isKey("T_En_Juego"))
    {
        int Timer_Transmission_In_Game=60;/* 1 Minuto*/  /*  antes Cada 30s*/
        Serial.println("Tiempo transmision maquina en juego por defecto...");
        NVS.putUInt("T_En_Juego",Timer_Transmission_In_Game);
    }

    if(!NVS.isKey("T_No_Juego"))
    {
        int Timer_Transmission_Not_Game=180000; /* 3 Minutos */ /* antes  Cada 2.5 minutos */
        Serial.println("Tiempo transmision maquina no juego por defecto...");
        NVS.putUInt("T_No_Juego",Timer_Transmission_Not_Game);
    }

    if(!NVS.isKey("TimerUser"))
    {
        int Timer_User=50;
        Serial.println("Timeout_Usuario por defecto....");
        NVS.putUInt("TimerUser",Timer_User);
    }

    if (!NVS.isKey("TYPE_TM")) // 
    {
        Serial.println("Guardando tipo de Transmision por defecto Socket....");
        // Transmision Socket = false
        // Transmision API = true
        bool TRANSMISSION_DATA = false;
        NVS.putBool("TYPE_TM", TRANSMISSION_DATA);
    }

    if(!NVS.isKey("SocketAP"))
    {
        Serial.println("Guarda puerto AP por defecto....");
        uint16_t portAP = 80;
        NVS.putUInt("SocketAP", portAP);
    }


    if (!NVS.isKey("ID_MQ")) // Configura ID maquina 
    {
        Serial.println("Guardando ID Por defecto....");
        String Id_Maquina_t = "00000";
        //String ssid = "GLOBUS-DESARROLLO";
        NVS.putString("ID_MQ", Id_Maquina_t); 
    }

    if(!NVS.isKey("Param_API"))
    {
        /* Controlador principal */
            /* Metodo RTC */
            /* Metodo contadores */
            /* Metodo Eventos */
            /* Metodo Token */
        String Controlador_Principal="http://cashlessapi.globussistemas.net/";
        String Metodo_RTC="api/Tarjeta/GeneraRtc";
        String Metodo_Contadores="api/Tarjeta/ProcesarContadores";
        String Metodo_Eventos="api/Tarjeta/ProcesarEventos";
        String Metodo_Token="api/Token/GenerarTokenApi";

        String joinedString = "";
        joinedString=Controlador_Principal+"|"+Metodo_RTC+"|"+Metodo_Contadores+"|"+Metodo_Eventos+"|"+Metodo_Token;
        NVS.putString("Param_API",joinedString);
    }

    if(!NVS.isKey("Event_Mecanic"))
    {
        /* Eventos mecanicos habilitados = True */
        /* Eventos mecanicos inhabilitados = false */

        bool Event_Mecanic = false;
        NVS.putBool("Event_Mecanic", Event_Mecanic);
    }


    /* *************************** CASHLESS **************************************/
    if(!NVS.isKey("Reg_AFT"))
    {
        char Temp_Register_AFT[20];

        for(int i=0; i<20; i++)
        {
            Temp_Register_AFT[i]=0x00;
        }
        NVS.putBytes("Reg_AFT", Temp_Register_AFT, sizeof(Temp_Register_AFT));
    }


    if(!NVS.isKey("Trans_ID"))
    {
        uint32_t Trans_ID=0;
        NVS.putUInt("Trans_ID",Trans_ID);
    }

    if(!NVS.isKey("Enable_Cashless"))
    {
        bool Enable=false; /* Deshabilitado por defecto!*/
        NVS.putBool("Enable_Cashless",Enable);
    }

    if(!NVS.isKey("Id_Client"))
    {
        byte ID_Client_Sesion[8]={'0', '0', '0', '0','0','0','0','0'};
        NVS.putBytes("Id_Client",ID_Client_Sesion,sizeof(ID_Client_Sesion));
    }

    if(!NVS.isKey("Sesion_Type"))
    {
        int Type_Sesion = SESION_DEFAULT;
        NVS.putInt("Sesion_Type", SESION_DEFAULT);
    }
    
    if(!NVS.isKey("Only_Cashless"))
    {
        bool Only_Cashless=false;
        NVS.putBool("Only_Cashless",Only_Cashless);
    }

    if(!NVS.isKey("Spiffs"))
    {
        bool Default_Formatt_true=true;
        NVS.putBool("Spiffs",Default_Formatt_true);
    }


    if(!NVS.isKey("ID_Tito"))
    {
        int Default_Validations_System_ID=0;
        NVS.putInt("ID_Tito",Default_Validations_System_ID);
    }

    if(!NVS.isKey("Only_Tito"))
    {
        bool Only_Tito=false;
        NVS.putBool("Only_Tito",Only_Tito);
    }

    if(!NVS.isKey("Enable_Tito"))
    {
        bool testTito=false;
        NVS.putBool("Enable_Tito",testTito);
    }

    if(!NVS.isKey("Cash_Pending"))
    {
        /* Recupera estado transacción pendiente Cashless */
        bool Pending=false;
        NVS.putBool("Cash_Pending",Pending);
    }

    if(!NVS.isKey("Cash_Pen_Dow"))
    {
        /* Recupera estado transacción pendiente Cashless */
        bool Pending=false;
        NVS.putBool("Cash_Pen_Dow",Pending);
    }

    if(!NVS.isKey("P_SAS"))
    {
        /* Procesamiento Premios SAS 
        True= Habilitados 
        False= Deshabilitados
        */
        bool Premios_SAS=false;
        NVS.putBool("P_SAS",Premios_SAS);
    }

    if(!NVS.isKey("Ignore_Reg_Maq"))
    {
        /* Ignora registro maquina 
        True= Ignora registro Maq Cashless
        False= Espera registro Maq Cashless
        */
        bool Ignore_Register=false;
        NVS.putBool("Ignore_Reg_Maq",Ignore_Register);
    }


    // if(!NVS.isKey("Address_TFT"))
    // {
    //     uint8_t Adress_TFT_Display[] = {0x34, 0x85, 0x18, 0x71, 0x0C, 0xCC};
    //     NVS.putBytes("Address_TFT", Adress_TFT_Display, sizeof(Adress_TFT_Display));
    // }
    if (!NVS.isKey("TimeBackup"))
    {
        uint32_t time = 15;
        NVS.getULong("TimeBackup", time);
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

    // // Inicializa Direccion IP Servidor
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

    size_t ip_serv_len2 = NVS.getBytesLength("Dir_IP_Serv2");
    char IP_SERV2[ip_serv_len2];
    char IP_prueba2[4];
    NVS.getBytes("Dir_IP_Serv2", IP_SERV2, ip_serv_len2);
    Configuracion.Set_Configuracion_ESP32(Direccion_IP_Server2, IP_SERV2);
    IP_prueba[4];
    bzero(IP_prueba2, 4);
    memcpy(IP_prueba2, Configuracion.Get_Configuracion(Direccion_IP_Server2, 'x'), sizeof(IP_prueba2) / sizeof(IP_prueba2[0]));
    Serial.print("Direccion IP Servidor 2: ");
    for (int i = 0; i < 4; i++)
    {
        Serial.print((int)IP_prueba2[i]);
        Serial.print(" ");
    }
    Serial.println();
    /*--------------------------------------------------------------------------------------------------------------------------*/

     size_t ip_dns_len = NVS.getBytesLength("Dns_Primary");
    char IP_DNS[ip_dns_len];

    NVS.getBytes("Dns_Primary", IP_DNS, ip_dns_len);
    Configuracion.Set_Configuracion_ESP32(Dns_One_IP, IP_DNS);
    IP_prueba[4];
    bzero(IP_prueba, 4);
    memcpy(IP_prueba, Configuracion.Get_Configuracion(Dns_One_IP, 'x'), sizeof(IP_prueba) / sizeof(IP_prueba[0]));
    Serial.print("DNS Primario: ");
    for (int i = 0; i < 4; i++)
    {
        Serial.print((int)IP_prueba[i]);
        Serial.print(" ");
    }
    Serial.println();

    size_t ip_dns_len_two = NVS.getBytesLength("Dns_Secondary");
    char IP_DNS_two[ip_dns_len_two];

    NVS.getBytes("Dns_Secondary", IP_DNS_two, ip_dns_len_two);
    Configuracion.Set_Configuracion_ESP32(Dns_Two_IP, IP_DNS_two);
    IP_prueba[4];
    bzero(IP_prueba, 4);
    memcpy(IP_prueba, Configuracion.Get_Configuracion(Dns_Two_IP, 'x'), sizeof(IP_prueba) / sizeof(IP_prueba[0]));
    Serial.print("DNS Secundario: ");
    for (int i = 0; i < 4; i++)
    {
        Serial.print((int)IP_prueba[i]);
        Serial.print(" ");
    }
    Serial.println();

    // Inicializa Puerto de conexion a servidor
    uint16_t port_server = NVS.getUInt("Socket", 0);
    Configuracion.Set_Configuracion_ESP32(Puerto_Server, port_server);
    Serial.print("Puerto de conexion: ");
    Serial.println(Configuracion.Get_Configuracion(Puerto_Server, 0));


    uint16_t port_server2 = NVS.getUInt("Socket2", 0);
    Configuracion.Set_Configuracion_ESP32(Puerto_Server2, port_server2);
    Serial.print("Puerto de conexion 2: ");
    Serial.println(Configuracion.Get_Configuracion(Puerto_Server2, 0));


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
        Serial.println("Simple");
        break;
    case 11:
        Serial.println("Aristocrat Australiana");
        break;
    case 12:
        Serial.println("Simple No Cancel");
        break;
    case 13:
        Serial.println("Poker_Ertech_Simple");
        break;
    case 14:
        Serial.println("Poker_Ertech_Slot");
        break;
    
    case 15:
         Serial.println("Mecanicas 4 contadores ");
        break;


    case 16:
        Serial.println("Ruleta IRT");
        break;

    case 17:
        Serial.println("Aristocrat EFT");
    break;

    default:
        break;
    }

    Serial.print("Reset Handpay: ");
    bool Tipo_Reset_Premio=NVS.getBool("RHNSAS",false);
    if(Tipo_Reset_Premio)
    {
        Serial.println("NO SAS Habilitado");
        Variables_globales.Set_Variable_Global(Type_Hanpay_Reset,true);
    }
        
    else
    {
        Serial.println("SAS");
        Variables_globales.Set_Variable_Global(Type_Hanpay_Reset,false);
    }


    Serial.print("Estado lector RFID : ");
    bool Enable_Lector_RFID=NVS.getBool("LECTOR",true);

    if(Enable_Lector_RFID)
    {
        Serial.println("Habilitado");
        Variables_globales.Set_Variable_Global(Consulta_Info_Lector_Rfid,true);
    }else{
        Serial.println("Deshabilitado");
        Variables_globales.Set_Variable_Global(Consulta_Info_Lector_Rfid,false);
    }

    
    Serial.print("Reinicios por intentos de Conexion: ");
    int Intentos_CWIFI= NVS.getUInt("Connect_W", 0);
    Serial.println(Intentos_CWIFI);
    if(Intentos_CWIFI>=5) /*-- Reinicios por intentos --*/
    {
        Variables_globales.Set_Variable_Global(Excepcion_WIFI,true);
    }else{
        Variables_globales.Set_Variable_Global(Excepcion_WIFI,false);
    }       


    Serial.print("TimeOut Player Tracking: ");
    Inactividad_Usuario_Player_Tracking=NVS.getUInt("TimePlayer",90000);
    switch (Inactividad_Usuario_Player_Tracking)
    {
    case 90000:
        Serial.println(" 1.5 Minutos ");
        break;

    case 120000:
        Serial.println(" 2 Minutos ");
        break;
    
    case 150000:
        Serial.println(" 2.5 Minutos ");
        break;
    case 180000:
        Serial.println(" 3 Minutos ");
        break;
    
    case 210000:
        Serial.println(" 3.5 Minutos ");
        break;

    case 240000:
        Serial.println(" 4 Minutos ");
        break;

    case 270000:
        Serial.println(" 4.5 Minutos ");
        break;

    case 300000:
        Serial.println(" 5 Minutos ");
        break;

    case 330000:
        Serial.println(" 5.5 Minutos  ");
        break;

    case 360000:
        Serial.println(" 6 Minutos ");
        break;
    
    default:

        Serial.println(" 1.5 Minutos ");
        break;
    }
    

    Serial.print("Tiempo de Transmision En Juego: ");
    Tiempo_Transmision_En_Juego=NVS.getUInt("T_En_Juego",30);
     switch (Tiempo_Transmision_En_Juego)
    {
    case 30:
        Serial.println(" 30 segundos ");
        break;

    case 40:
        Serial.println(" 40 segundos ");
        break;
    
    case 50:
        Serial.println(" 50 segundos ");
        break;
    case 60:
        Serial.println(" 1 Minuto");
        break;
    
    case 70:
        Serial.println(" 1 Minuto 10 Segundos ");
        break;

    case 80:
        Serial.println(" 1 Minuto 20 Segundos ");
        break;

    case 90:
        Serial.println(" 1 Minuto 30 Segundos ");
        break;

    case 100:
        Serial.println(" 1 Minuto 40 Segundos ");
        break;

    case 110:
        Serial.println(" 1 Minuto 50 Segundos ");
        break;

    case 120:
        Serial.println(" 2 Minutos ");
        break;
    
    default:

        Serial.println(" 30 Segundos ");
        break;
    }

    Serial.print("Tiempo de Transmision No Juego: ");
    Tiempo_Transmision_No_Juego=NVS.getUInt("T_No_Juego",150000);
       switch (Tiempo_Transmision_No_Juego)
    {
    case 150000:
        Serial.println(" 2.5 Minutos ");
        break;

    case 180000:
        Serial.println(" 3 Minutos ");
        break;
    
    case 210000:
        Serial.println(" 3.5 Minutos ");
        break;
    case 240000:
        Serial.println(" 4 Minutos ");
        break;
    
    case 270000:
        Serial.println(" 4.5 Minutos ");
        break;

    case 300000:
        Serial.println(" 5 Minutos ");
        break;

    case 330000:
        Serial.println(" 5.5 Minutos ");
        break;

    case 360000:
        Serial.println(" 6 Minutos ");
        break;

    case 480000:
        Serial.println(" 8 Minutos ");
        break;

    case 600000:
        Serial.println(" 10 Minutos ");
        break;

    case 1800000:
        Serial.println(" 30 Minutos ");
        break;

    case 3600000:
        Serial.println(" 1 Hora ");
        break;

    case 5400000:
        Serial.println(" 1 hora 30 Minutos ");
        break;   
    default:
        Serial.println(" 2.5 Minutos ");
        break;
    }

    Serial.print("Tiempo de Actividad de maquina: ");
    Tiempo_Inactividad_Maquina = NVS.getUInt("TimerUser", 50);
    switch (Tiempo_Inactividad_Maquina)
    {
    case 50:
        Serial.println(" 30s ");
        break;

    case 80:
        Serial.println("1 Minuto");
        break;

    case 115:
        Serial.println(" 1 Minuto 30s ");
        break;
    case 150:
        Serial.println(" 2 Minutos ");
        break;

    case 180:
        Serial.println(" 2 Minutos 30s ");
        break;

    case 230:
        Serial.println(" 3 Minutos ");
        break;

    default:

        Serial.println(" 30s ");
        break;
    }


    // Inicializa Tipo de conexion Servidor API o Socket
    bool Type = NVS.getBool("TYPE_TM");
    Serial.print("Tranmision de datos via: ");
    if(Type)
    {
        /* Inicializar Datos de API */
        Serial.println("API");
        Variables_globales.Set_Variable_Global(Gmaster_API_Mode,true);
    }
    else
    {
        Serial.println("Socket");
        Variables_globales.Set_Variable_Global(Gmaster_API_Mode,false);
    }

    uint16_t port_AP = NVS.getUInt("SocketAP", 0);
    Configuracion.Set_Configuracion_ESP32(Puerto_AP, port_AP);
    Serial.print("Puerto de conexion AP: ");
    Serial.println(Configuracion.Get_Configuracion(Puerto_AP, 0));


    String Id_Maquina_Temp = NVS.getString("ID_MQ");
    Configuracion.Set_Configuracion_ESP32(Id_Maquina, Id_Maquina_Temp);
    Serial.print("Id Maquina: ");
    Serial.println(Configuracion.Get_Configuracion(Id_Maquina, "Id_Maquina"));


    // size_t ParameterApi_len = NVS.getBytesLength("Param_API");
    // String ParametersAp[ ParameterApi_len];
    // NVS.getBytes("Param_API", ParametersAp, ParameterApi_len);



    // String Controlador_Principal="http://cashlessapi.globussistemas.net/";
    // String Metodo_RTC="api/Tarjeta/GeneraRtc";
    // String Metodo_Contadores="api/Tarjeta/ProcesarContadores";
    // String Metodo_Eventos="api/Tarjeta/ProcesarEventos";
    // String Metodo_Token="api/Token/GenerarTokenApi";

    // String ParametersAp[5];

    // ParametersAp[0]=Controlador_Principal;
    // ParametersAp[1]=Metodo_RTC;
    // ParametersAp[2]=Metodo_Contadores;
    // ParametersAp[3]=Metodo_Eventos;
    // ParametersAp[4]=Metodo_Token;

    String savedString = NVS.getString("Param_API", "");
    String ParametersAp[5];

    int pos = 0;
    int startPos = 0;
    for (int i = 0; i < savedString.length(); i++) {
        if (savedString.charAt(i) == '|') {
            ParametersAp[pos++] = savedString.substring(startPos, i);
            startPos = i + 1;
        }
    }
    // Agrega el último elemento
    ParametersAp[pos] = savedString.substring(startPos);

    Configuracion.Set_Configuracion_ESP32_ES(Controlador_P, ParametersAp);
    Configuracion.Set_Configuracion_ESP32_ES(Metodo_Conta, ParametersAp);
    Configuracion.Set_Configuracion_ESP32_ES(Metodo_Event, ParametersAp);
    Configuracion.Set_Configuracion_ESP32_ES(Metodo_Sincro_RTC, ParametersAp);
    Configuracion.Set_Configuracion_ESP32_ES(Metodo_Access_T, ParametersAp);
    Serial.print("Controlador Principal: ");
    Serial.println(Configuracion.Get_Configuracion_ES(Controlador_P,"Controlador_Principal"));
    Api_G.Init_Controlador_Principal(Configuracion.Get_Configuracion_ES(Controlador_P,"Controlador_Principal"));
    Serial.print("API Contadores: ");
    Serial.println(Configuracion.Get_Configuracion_ES(Metodo_Conta,"Metodo_Contadores"));
    Serial.print("API Eventos: ");
    Serial.println(Configuracion.Get_Configuracion_ES(Metodo_Event,"Metodo_Eventos"));
    Serial.print("API Sincro RTC: ");
    Serial.println(Configuracion.Get_Configuracion_ES(Metodo_Sincro_RTC,"Metodo_SincroRTC"));
    Serial.print("API Token: ");
    Serial.println(Configuracion.Get_Configuracion_ES(Metodo_Access_T,"Metodo_Token"));
        

    bool Enable_Eventos= NVS.getBool("Event_Mecanic",false);

    if(Enable_Eventos)
    {
        Variables_globales.Set_Variable_Global(Enable_Mechanical_Events,true);
        eventos.Set_Timer_Ignore_Event(10000);
        Serial.println("Eventos Mecanicos habilitados ");
    }else{
        Variables_globales.Set_Variable_Global(Enable_Mechanical_Events,false);
        Serial.println("Eventos mecanicos deshabilitados ");
    }

    uint16_t Port_COM = NVS.getUInt("COM",1);
    Variables_globales.Set_Variable_Global_Uint16(Uart_Port_Select,Port_COM);
    if(Port_COM==1)
        Serial.println("Puerto RS232: COM1");
    else if(Port_COM==2)
        Serial.println("Puerto RS232: COM2");

    /*--------------------------------------------------------------------------------------------------------------------------*/


    /********************************************************** Cashless ****************************************************** */

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) < 4 || Configuracion.Get_Configuracion(Tipo_Maquina, 0)==17)
    {
        size_t Key_AFT_Size = NVS.getBytesLength("Reg_AFT");
        char AFT_Key[Key_AFT_Size];
        NVS.getBytes("Reg_AFT", AFT_Key, Key_AFT_Size);
        Buffer_Cashless.Set_Key_Register_AFT(AFT_Key);

        Serial.print("Estado de maquina AFT: ");
        if (AFT_Key[0] == 0x00 && AFT_Key[19] == 0x00)
        {
            Variables_globales.Set_Variable_Global(Status_AFT_Machine, false);
            Serial.println("No registrada");
        }

        else
        {
            Variables_globales.Set_Variable_Global(Status_AFT_Machine, true);
            Serial.println("Registrada");
        }

        Serial.print("ID Transaccion Maquina : ");
        /* ---------------> Trans ID <-------------*/
        uint32_t Trans_ID = NVS.getUInt("Trans_ID", 0);
        Cashless.Set_Inicial_Trans_ID(Trans_ID);
        /*-----------------------------------------*/
        Serial.println(Cashless.Get_Trans_ID_Int());

        bool Status_Cashless = NVS.getBool("Enable_Cashless", false);


        Variables_globales.Set_Variable_Global(Enable_Cashless, Status_Cashless);
        if (Variables_globales.Get_Variable_Global(Enable_Cashless))
            Serial.println("Transacciones Cashless Habilitadas");
        else
            Serial.println("Transacciones Cashless Inhabilitadas");
       

        size_t Leng_id = NVS.getBytesLength("Id_Client");
        byte Current_Id_Recovery[Leng_id];
        NVS.getBytes("Id_Client", Current_Id_Recovery, Leng_id);
        int Type_Sesion = NVS.getInt("Sesion_Type", SESION_DEFAULT);
        contadores.Set_Current_Cliente_Recover(Current_Id_Recovery, Type_Sesion);

        bool Only_Cashless=NVS.getBool("Only_Cashless",false);
        Variables_globales.Set_Variable_Global(Descarga_Solo_Cashelss,Only_Cashless);
        
        bool Ignore_Reg_Maq;
        Ignore_Reg_Maq=NVS.getBool("Ignore_Reg_Maq",false);
        Variables_globales.Set_Variable_Global(Ignore_Register_Machie,Ignore_Reg_Maq);

        if(Ignore_Reg_Maq)
            Serial.println("Flag Ignora registro ");

    }else{
        Cashless.Set_Inicial_Trans_ID(0);
        Variables_globales.Set_Variable_Global(Enable_Cashless, false);
        Serial.println("Transacciones Cashless Inhabilitadas");
        byte Current_Id_Recovery[8]={'0','0','0','0','0','0','0','0'};
        contadores.Set_Current_Cliente_Recover(Current_Id_Recovery, SESION_DEFAULT);
        // size_t Leng_id = NVS.getBytesLength("Id_Client");
        // byte Current_Id_Recovery[Leng_id];
        // NVS.getBytes("Id_Client", Current_Id_Recovery, Leng_id);
        // int Type_Sesion = NVS.getInt("Sesion_Type", SESION_DEFAULT);
        // contadores.Set_Current_Cliente_Recover(Current_Id_Recovery, Type_Sesion);
        // Variables_globales.Set_Variable_Global(Descarga_Solo_Cashelss,false);
    }


    

    bool Test_Formatt_Spiffs=NVS.getBool("Spiffs",true);
    Variables_globales.Set_Variable_Global(Default_Formatt,Test_Formatt_Spiffs);


    int Validations_System_ID_Rec=NVS.getInt("ID_Tito",0);
    Tito.Set_Inicial_Trans_ID_Tito(Validations_System_ID_Rec);
    Serial.print("Trasaccion ID Tito: ");
    Serial.println(Validations_System_ID_Rec);
    
    bool Tito_Test=NVS.getBool("Only_Tito",false);

    Variables_globales.Set_Variable_Global(Descarga_Solo_Tito,Tito_Test);

    bool Test_Enable_Tito=NVS.getBool("Enable_Tito");

    Variables_globales.Set_Variable_Global(Enable_Tito_Ticket,Test_Enable_Tito);

    if(Variables_globales.Get_Variable_Global(Enable_Tito_Ticket))
        Serial.println("Tito: Habilitado");
    else
        Serial.println("Tito: Deshabilitado");


    bool Test_Premios_SAS=NVS.getBool("P_SAS");
    Variables_globales.Set_Variable_Global(Handle_Premios_SAS,Test_Premios_SAS);
    if(Variables_globales.Get_Variable_Global(Handle_Premios_SAS))
        Serial.println("Premios SAS: Habilitados");
    else
        Serial.println("Premios SAS: Deshabilitatos");


    // size_t adress_TFT_Display_Len= NVS.getBytesLength("Address_TFT");
    // uint8_t Adress_TFT_Display[adress_TFT_Display_Len];
    // int Count_test=0;
    // NVS.getBytes("Address_TFT",Adress_TFT_Display,adress_TFT_Display_Len);

    // for (int i = 0; i < 6; i++)
    // {
    //     if (Adress_TFT_Display[i] == 0x00)
    //     {
    //         Count_test++;
    //     }
    // }

    // if(Count_test>=6)
    // {
    //     /* No Existe un dispositivo sincronizado */
    //     Variables_globales.Set_Variable_Global(Status_Device_TFT_Display,false);
    //     Variables_globales.Set_Variable_Global(Conexion_TFT_Display,false);
    //     Serial.println("Pantalla TFT: No existe dispositivo sincronizado");
    // }else{
    //     /* Existe un dispositivo sincronizado */
    //     memcpy(Address_Device_TFT_Display, Adress_TFT_Display, sizeof(Address_Device_TFT_Display) / sizeof(Address_Device_TFT_Display[0]));
        
    //     Serial.print("Pantalla TFT Wireless: ");
    //     for(int i=0; i<6; i++)
    //     {
    //         Serial.print(Address_Device_TFT_Display[i],HEX);
    //         Serial.print(":");
    //     }
    //     Variables_globales.Set_Variable_Global(Status_Device_TFT_Display,true);
    // }


    Dia_Guarda_Logs=NVS.getULong("TimeBackup",15);

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
            /*REDES*/
            if(Comando[3]==48){

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
                size_t ip_gw_len = NVS.getBytesLength("Dir_IP_GW");
                char IP_GW[ip_gw_len];
                NVS.getBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));
                IP_GW[2] = int(Comando[5] - 48);
                NVS.putBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));

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
                size_t ip_gw_len = NVS.getBytesLength("Dir_IP_GW");
                char IP_GW[ip_gw_len];
                NVS.getBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));
                IP_GW[2] = int(Comando[5] - 48);
                NVS.putBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));

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
                size_t ip_gw_len = NVS.getBytesLength("Dir_IP_GW");
                char IP_GW[ip_gw_len];
                NVS.getBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));
                IP_GW[2] = int(Comando[5] - 48);
                NVS.putBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));

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
                size_t ip_gw_len = NVS.getBytesLength("Dir_IP_GW");
                char IP_GW[ip_gw_len];
                NVS.getBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));
                IP_GW[2] = int(Comando[5] - 48);
                NVS.putBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));

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
                size_t ip_gw_len = NVS.getBytesLength("Dir_IP_GW");
                char IP_GW[ip_gw_len];
                NVS.getBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));
                IP_GW[2] = int(Comando[5] - 48);
                NVS.putBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));

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
                size_t ip_gw_len = NVS.getBytesLength("Dir_IP_GW");
                char IP_GW[ip_gw_len];
                NVS.getBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));
                IP_GW[2] = int(Comando[5] - 48);
                NVS.putBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));

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
                size_t ip_gw_len = NVS.getBytesLength("Dir_IP_GW");
                char IP_GW[ip_gw_len];
                NVS.getBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));
                IP_GW[2] = int(Comando[5] - 48);
                NVS.putBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));

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
                size_t ip_gw_len = NVS.getBytesLength("Dir_IP_GW");
                char IP_GW[ip_gw_len];
                NVS.getBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));
                IP_GW[2] = int(Comando[5] - 48);
                NVS.putBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));

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
                size_t ip_gw_len = NVS.getBytesLength("Dir_IP_GW");
                char IP_GW[ip_gw_len];
                NVS.getBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));
                IP_GW[2] = int(Comando[5] - 48);
                NVS.putBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));

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
                size_t ip_gw_len = NVS.getBytesLength("Dir_IP_GW");
                char IP_GW[ip_gw_len];
                NVS.getBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));
                IP_GW[2] = int(Comando[5] - 48);
                NVS.putBytes("Dir_IP_GW", IP_GW, sizeof(IP_GW));

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
              //  NVS.end();
            }
          //  NVS.end();
        }
    }
    else if (Comando[0] == 'M' && Comando[1] == 'E' && Comando[2] == 'C' && Comando[3] == 'A' && Comando[4] == 'N' && Comando[5] == 'I' && Comando[6] == 'C' && Comando[7] == 'A' && Comando[8] == '=')
    {

        if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
        {

            //  000472750000000100000001000507930000000000000000  
            //  000472750000000100000001000507930000000000000000  
            // MECANICA=00057275000000050000000500060793

            /* Mecanica conectada */
            
            char res[38];
            /* Cancel credit */
            res[4] = (Comando[9]);
            res[5] = Comando[10];
            res[6] = Comando[11];
            res[7] = Comando[12];
            res[8] = Comando[13];
            res[9] = Comando[14];
            res[10] = Comando[15];
            res[11] = Comando[16];

            /* Multiplicador Cancel */

            res[12] = Comando[17];
            res[13] = Comando[18];
            res[14] = Comando[19];
            res[15] = Comando[20];
            res[16] = Comando[21];
            res[17] = Comando[22];
            res[18] = Comando[23];
            res[19] = Comando[24];

            /* Multiplicador Billetero */
            res[20] = Comando[25];
            res[21] = Comando[26];
            res[22] = Comando[27];
            res[23] = Comando[28];
            res[24] = Comando[29];
            res[25] = Comando[30];
            res[26] = Comando[31];
            res[27] = Comando[32];

            /* Cancel credit */
            res[28] = Comando[33];
            res[29] = Comando[34];
            res[30] = Comando[35];
            res[31] = Comando[36];
            res[32] = Comando[37];
            res[33] = Comando[38];
            res[34] = Comando[39];
            res[35] = Comando[40];

            res[36] = 0x00;
            res[37] = 0x00;

            
            if(Actualiza_Tarjeta_Mecanica(res))
                Serial.println("Contadores mecanicos actualizados!");
            else
                Serial.println("Contadores mecanicos NO actualizados!");
        }
        else
            Serial.println("----->Tarjeta Mecanica no conectada");
        
    }else if(Comando[0] == 'M' && Comando[1] == 'E' && Comando[2] == 'C' && Comando[3] == 'A' && Comando[4] == 'N' && Comando[5] == 'I' && Comando[6] == 'C' && Comando[7] == 'A' && Comando[8] == '2' && Comando[9] == '=' )
    {

         if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
        {

            /* Mecanica conectada */
            
            char res[70];
           
            res[0]=0x00;
            res[1]=0x00;
            res[2]=0x00;
            res[3]=0x00;

            /* Cancel credit */
            res[4] = (Comando[10]);
            res[5] = Comando[11];
            res[6] = Comando[12];
            res[7] = Comando[13];
            res[8] = Comando[14];
            res[9] = Comando[15];
            res[10] = Comando[16];
            res[11] = Comando[17];

            /* Coin In */
            res[12] = Comando[18];
            res[13] = Comando[19];
            res[14] = Comando[20];
            res[15] = Comando[21];
            res[16] = Comando[22];
            res[17] = Comando[23];
            res[18] = Comando[24];
            res[19] = Comando[25];

            /* Coin Out */
            res[20] = Comando[26];
            res[21] = Comando[27];
            res[22] = Comando[28];
            res[23] = Comando[29];
            res[24] = Comando[30];
            res[25] = Comando[31];
            res[26] = Comando[32];
            res[27] = Comando[33];

            /* Total Drop */
            res[28] = Comando[34];
            res[29] = Comando[35];
            res[30] = Comando[36];
            res[31] = Comando[37];
            res[32] = Comando[38];
            res[33] = Comando[39];
            res[34] = Comando[40];
            res[35] = Comando[41];

            /* Multiplicador cancel */
            res[36] = Comando[42];
            res[37] = Comando[43];
            res[38] = Comando[44];
            res[39] = Comando[45];
            res[40] = Comando[46];
            res[41] = Comando[47];
            res[42] = Comando[48];
            res[43] = Comando[49];

            /* Multiplicador Coin In */
            res[44] = Comando[50];
            res[45] = Comando[51];
            res[46] = Comando[52];
            res[47] = Comando[53];
            res[48] = Comando[54];
            res[49] = Comando[55];
            res[50] = Comando[56];
            res[51] = Comando[57];

            /* Multiplicador Coin Out */
            res[52] = Comando[58];
            res[53] = Comando[59];
            res[54] = Comando[60];
            res[55] = Comando[61];
            res[56] = Comando[62];
            res[57] = Comando[63];
            res[58] = Comando[64];
            res[59] = Comando[65];

            /* Multiplicador Total drop */
            res[60] = Comando[66];
            res[61] = Comando[67];
            res[62] = Comando[68];
            res[63] = Comando[69];
            res[64] = Comando[70];
            res[65] = Comando[71];
            res[66] = Comando[72];
            res[67] = Comando[73];
            
            if(Actualiza_Tarjeta_Mecanica(res))
                Serial.println("Contadores mecanicos actualizados!");
            else
                Serial.println("Contadores mecanicos NO actualizados!");
        }
        else
            Serial.println("----->Tarjeta Mecanica no conectada");
    }
    else{

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
