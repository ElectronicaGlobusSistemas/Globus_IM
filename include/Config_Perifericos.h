#include <Arduino.h>
#include "Memory_SD.h"
#include "RTC.h"
<<<<<<< HEAD
=======
#include "nvs_flash.h"

>>>>>>> 0a6858e63669ecc12effe1ba4496756a97320c91
//-------------------> Parametros <-------------------------------
#define Clock_frequency 240
#define MCU_Status 2
#define WIFI_Status 15
//-----------------------------------------------------------------

TaskHandle_t Task1;

void loop2(void *parameter);
void Init_Tasks();
void Init_Indicadores_LED(void);
void Init_Configuracion_Inicial(void);

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
    Init_Indicadores_LED();         // Reset Indicadores LED'S LOW.
    //---------------------------------------------------------------
<<<<<<< HEAD
    //vTaskSuspendAll(); // Suspende Todas Las Tareas.
    RTC.setTime(0, 12, 10,9 , 6, 2022);
    //-------------------->  Módulos <-------------------------------
   
=======
    // vTaskSuspendAll(); // Suspende Todas Las Tareas.
    //---------------------------------------------------------------
    // Inicializa las variables guardadas en memoria NVS
    Init_Configuracion_Inicial();
    //---------------------------------------------------------------
    RTC.setTime(0, 0, 0, 1, 1, 2022);
    //-------------------->  Módulos <-------------------------------
    Init_SD(); // Inicializa Memoria SD.
    Archivo_Format = "17062022.csv";
    Create_ARCHIVE_Excel(Archivo_Format, Encabezado_Contadores);
>>>>>>> 0a6858e63669ecc12effe1ba4496756a97320c91
    //---------------------------------------------------------------
    //-----------------> Config Comunicación Maquina <---------------
    Init_UART2(); // Inicializa Comunicación Maquina Puerto #2
    //---------------------------------------------------------------
    init_Comunicaciones();
    //--------------------> Config  WIFI <---------------------------
    CONNECT_WIFI();
    CONNECT_SERVER_TCP();
<<<<<<< HEAD

    // Archivo_Format= Hora Actualizada..
    //  Init_FTP_SERVER();  // Preconfigura  y Pausa Server FTP
    Init_SD(); // Inicializa Memoria SD.
   // Init_FTP_SERVER();

    Archivo_Format="25062022.csv"; // Crea Archivo Si no Existe.
    Create_ARCHIVE_Excel(Archivo_Format,Encabezado_Contadores);
=======
    Init_Wifi();
>>>>>>> 0a6858e63669ecc12effe1ba4496756a97320c91
    //---------------------------------------------------------------
    //--------------------> Run Tareas <-----------------------------
    Init_Tasks();
    //---------------------------------------------------------------
    //-------------------->  Update  <-------------------------------
    Bootloader(); // Inicializa  Bootloader
    //---------------------------------------------------------------
<<<<<<< HEAD
    
=======
>>>>>>> 0a6858e63669ecc12effe1ba4496756a97320c91
}

void Init_Tasks(void)
{
    xTaskCreatePinnedToCore(
        loop2,    //  Funcion a implementar la tarea
        "Task_1", //  Nombre de la tarea
        10000,    //  Tamaño de stack en palabras (memoria)
        NULL,     //  Entrada de parametros
        1,        //  Prioridad de la tarea
        &Task1,   //  Manejador de la tarea
        0);       //  Core donde se ejecutara la tarea
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
        String ssid = "GLOBUS-DESARROLLO";
        NVS.putString("SSID_DESA", ssid);
    }

    if (!NVS.isKey("PASS_DESA")) // Configura PASSWORD de conexion WIFI
    {
        Serial.println("Guardando Password por defecto...");
        String password = "Globus2020*";
        NVS.putString("PASS_DESA", password);
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

    // Inicializa NRed Wifi y Password
    String ssid = NVS.getString("SSID_DESA");
    Configuracion.Set_Configuracion_ESP32(SSID, ssid);
    String password = NVS.getString("PASS_DESA");
    Configuracion.Set_Configuracion_ESP32(Password, password);
    Serial.print("Conecta a Red: ");
    Serial.println(Configuracion.Get_Configuracion(SSID, "Nombre_Red"));
    /*--------------------------------------------------------------------------------------------------------------------------*/

    Serial.println("\n");
    NVS.end();
}