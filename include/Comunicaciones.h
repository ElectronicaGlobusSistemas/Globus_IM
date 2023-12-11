#include "Internet.h"
#include "Memory_SD.h"
#include <iostream>
#include <string>
#include <nvs_flash.h>
#include "RFID.h"
#include "Utilidades.h"
#include "Errores.h"

#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>


/*------------------------> Debug <----------------------------------*/
//#define Debug_Mensajes_Server
//#define Debug_Transmision
//#define Debug_Mensajes_RFID
/*-------------------------------------------------------------------*/

#define ID_Contadores_Accounting 3
#define ID_Billetes 4
#define ID_Eventos 5
#define ID_Maquina 11
#define ID_ROM_Signature 12
#define ID_Contadores_Cashless 308
#define ID_Informacion_Tarjeta 273
#define ID_Informacion_lector   601
#define ID_Informacion_Cashless 318
#define MAX_LIM_EVENTOS  900

/*----------->Hardware<--------------*/
#define Unlock_Machine  26
/*-----------------------------------*/
int Contador_Transmision = 0;
int Contador_Transmision_Contadores = 0;
int Contador_Maquina_En_Juego = 0;

int Contador_Cancel_Credit_Ant = 0;
int Contador_Cancel_Credit_Act = 0;
bool flag_premio_pagado_cashout = false;

int Contador_Bill_In_Ant = 0;
int Contador_Bill_In_Act = 0;
bool flag_billete_insertado = false;

int Contador_Coin_In_Ant = 0;
int Contador_Coin_In_Act = 0;
// bool flag_maquina_en_juego = false;

extern bool flag_ultimo_contador_Ok;
extern TaskHandle_t Ftp_SERVER; //  Manejador de tareas

TaskHandle_t CommandProcess;

char Archivo_CSV_Contadores[200];
char Archivo_CSV_Eventos[200];
char Archivo_LOG[200];
char Archivo_CSV_Sesiones[200];
char Archivo_CSV_Premios[200];

String string_Fecha;
String string_Fecha_LOG;
String string_Fecha_Eventos;
String string_Fecha_Sesiones;
String string_Fecha_Premios;
int day_copy;
int month_copy;
int year_copy;
bool resultados[5]={false,false,false,false,false};
unsigned short Ptr_Eventos_Marca_Temp = 0x00;
unsigned short Ptr_Eventos_Marca = 0x00;
unsigned short Num_Eventos = 0x0000;
unsigned char Tabla_Eventos_[ 999 ][ 8 ];

/*TIMER Memoria FULL*/
unsigned long Start_Timer=0;
unsigned long Finally_Timer=0;
unsigned long Inter_Timer=60000; // 1 minuto



bool Deshabilita_Handpay=false;
unsigned Start_=0;
unsigned Finally_=0;
unsigned Inter_=20000; 
bool Flash_OK=false;
unsigned Start_timmer=0; // J
unsigned Previous_timmer=0; //l
unsigned Stop_timmer=28800;//K
bool machine_M=false;


int TIMEOUT_SAS_CREDIT_ = 180000;
unsigned long Contador_Inicial=0;
unsigned long Contador_Final=0;
bool Bloqueo=false;
bool Activa_Encuesta=false;
bool No_Comunica=false;
int Valida_Creditos_Actuales=0;

extern int Tiempo_Inactividad_Maquina;

#define Hopper_Enable 14
void Task_Procesa_Comandos(void *parameter);
void Task_Maneja_Transmision(void *parameter);
void Task_Verifica_Hopper(void *parameter);
void Verifica_Cambio_Contadores(void);
void Transmite_Configuracion(void);
void Transmision_Controlada_Contadores(void);
bool Calcula_Cancel_Credit(bool);
extern void Almacena_Evento(unsigned char Evento,ESP32Time RTC);
int Unlock_Machine_(void);
void Maneja_Marca_Eventos(void);
void Transmite_Confirmacion_Info(char High, char Low);
byte *Operador( char*Datos_Operador);
bool Consulta_Conexion_To_Server(void);
void Tratamiento_Trama_Cashless(char res[]);
bool Calcula_First_Cancel_Credit(bool Calcula_Contador);
void Actualiza_Contadores(void);
extern unsigned char Serial_Cashless_UniMil;
extern unsigned char Serial_Cashless_Centenas;
extern unsigned char Serial_Cashless_Decenas;
extern unsigned char Serial_Cashless_Unidades;
extern unsigned char Ant_Serial_Cashless_UniMil;
extern unsigned char Ant_Serial_Cashless_Centenas;
extern unsigned char Ant_Serial_Cashless_Decenas;
extern unsigned char Ant_Serial_Cashless_Unidades;
extern unsigned char New_Serial_Cashless_UniMil;
extern unsigned char New_Serial_Cashless_Centenas;
extern unsigned char New_Serial_Cashless_Decenas;
extern unsigned char New_Serial_Cashless_Unidades;




extern bool Condicion_Cumpl;
extern int Tiempo_Transmision_En_Juego;
extern int Tiempo_Transmision_No_Juego;
/*Timer Counter*/

int Ejecuta=1;
bool Detec=false;
unsigned long New_Timer_Final=0;
int Activa_Timmer=150000; /*100000*/
                  
unsigned long New_Timmer_Inicial=0;
 

/*-------------------------> Variables Timeout Player tracking <---------------------------------------*/
extern unsigned long currentTime;
extern  bool condicionCumplida;
extern  unsigned long startTime;
/*-----------------------------------------------------------------------------------------------------*/


unsigned long Timer_Out_Failed=0;
unsigned long Timer_Out_Failed_Final=0;
int Timer_Failed_OK=40000;
bool Enable_Failed=false;


unsigned long Timer_Reset_In=0;
unsigned long Timer_Reset_Out=0;
int Timer_No_Reset=10000; 

bool Enable_Timer_No_reset=false;




int  Convert_Char_To_Int2(char buffer[])
{
    int resultado = ((buffer[0] - 48) * 10000000) + ((buffer[1] - 48) * 1000000) +
                    ((buffer[2] - 48) * 100000) + ((buffer[3] - 48) * 10000) +
                    ((buffer[4] - 48) * 1000) + ((buffer[5] - 48) * 100) +
                    ((buffer[6] - 48) * 10) + ((buffer[7] - 48) * 1);
    return resultado;
}

int Convert_Char_To_Int10(char buffer[]) {
    int resultado = 0;
    int factor = 1;
    int longitud = strlen(buffer);
    int primerDigito = 0;

    for (int i = 0; i < longitud; i++) {
        if (buffer[i] >= '0' && buffer[i] <= '9') {
            if (buffer[i] != '0' || primerDigito) {
                resultado = resultado * 10 + (buffer[i] - '0');
                primerDigito = 1;
            }
        }
    }

    return resultado;
}

int Convert_Char_To_Int11(char buffer[]) {
    int resultado = 0;
    int primerDigito = 0;
    int longitud = strlen(buffer);

    for (int i = 0; i < longitud; i++) {
        if (isdigit(buffer[i])) {
            if (buffer[i] != '0' || primerDigito) {
                resultado = resultado * 10 + (buffer[i] - '0');
                primerDigito = 1;
            }
        } else {
            // Caracter no válido en la entrada
            return 1; // Puedes elegir manejar el error de otra manera si lo prefieres
        }
    }

    return resultado;
}

void init_Comunicaciones()
{
    xTaskCreatePinnedToCore(
        Task_Procesa_Comandos,
        "Procesa comandos server",
        10000,
        NULL,
        configMAX_PRIORITIES - 3,
        &CommandProcess,
        0); // Core donde se ejecutara la tarea
        
    xTaskCreatePinnedToCore(
        Task_Maneja_Transmision,
        "Maneja la transmision con server",
        8000,
        NULL,
        configMAX_PRIORITIES - 15,
        NULL,
        0); // Core donde se ejecutara la tarea

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6)
    {
        xTaskCreatePinnedToCore(
            Task_Verifica_Hopper,
            "Verifica en estado del Hopper - Poker",
            10000,//10000
            NULL,
            configMAX_PRIORITIES - 4,
            NULL,
            0); // Core donde se ejecutara la tarea
    }

    
}

/*****************************************************************************************/
/********************************* TRANSMITE A SERVIDOR **********************************/
/*****************************************************************************************/

void Transmite_A_Servidor(char buffer[], int len)
{
    if (WiFi.status() == WL_CONNECTED)
    {   
        #ifdef Debug_Mensajes_Server
        Serial.print("Tamaño a enviar: ");
        Serial.println(len);
        #endif
        //    Serial.println("buffer enviado: ");
        //    Serial.println(buffer);
        int length_ = 0;
        if (Configuracion.Get_Configuracion(Tipo_Conexion))
            length_ = clientTCP.write(buffer, len);
        else
        {
            memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));
            IPAddress serverIP(IP_Server[0], IP_Server[1], IP_Server[2], IP_Server[3]);
            uint16_t serverPort = Configuracion.Get_Configuracion(Puerto_Server, 0);

            clientUDP.beginPacket(serverIP, serverPort);
            length_ = clientUDP.write((const uint8_t *)buffer, len);
            clientUDP.endPacket();
        }
        #ifdef Debug_Mensajes_Server
        Serial.print("Bytes enviados: ");
        Serial.println(length_);
        Serial.println("--------------------------------------------------------------------");
        #endif
    }
    else
        Serial.println("No se puede enviar mensaje, no conexion a WIFI");
}

/*****************************************************************************************/
/********************************** TRANSMITE ACK ****************************************/
/*****************************************************************************************/
void Transmite_Confirmacion(char High, char Low)
{
    if (Buffer.Set_buffer_ACK(ID_Eventos, High, Low))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_ACK(), 258);
        #ifdef Debug_Mensajes_Server
        Serial.println("Set buffer general OK");
        #endif
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
    }
    else
    {
        if (Variables_globales.Get_Variable_Global(SD_INSERT)==true)
        {
            if (!Variables_globales.Get_Variable_Global(Fallo_Archivo_LOG))
            {
                if (Variables_globales.Get_Variable_Global(Sincronizacion_RTC) == true && Variables_globales.Get_Variable_Global(Ftp_Mode==false))
                {
                    log_e("No se Pudo Guardar el Buffer General de datos", 101);
                    LOG_ESP(Archivo_LOG, Variables_globales.Get_Variable_Global(Enable_Storage));
                }
            }
        }
        Serial.println("Set buffer general ERROR");
    }
}


/* Configura Timeout de sesion de jugador
(0)=1.5 Minutos Por defecto!
(1)=2 Minutos 
(2)=2.5 Minutos 
(3)=3 Minutos 
(4)=3.5 Minutos 
(5)=4 Minutos
(6)=4.5 Minutos 
(7)=5 Minutos 
(8)=5.5 Minutos
(9)=6 Minutos 
 */
void Config_TimeOut_Player_Tracking(char Datos[])
{
    switch (Datos[4]-48)
    {
    case  0:

        NVS.begin("Config_ESP32", false);
        NVS.putUInt("TimePlayer", 90000); /* 1 minutos & 30 segundos */
        NVS.end();
        Transmite_Confirmacion('E','D');
        delay(150);
        ESP.restart();
        break;

    case  1:

        NVS.begin("Config_ESP32", false);  /* 2 Minutos */
        NVS.putUInt("TimePlayer", 120000);
        NVS.end();
        Transmite_Confirmacion('E','D');
        delay(150);
        ESP.restart();
        break;

    case  2:

        NVS.begin("Config_ESP32", false);  /* 2 Minutos & medio */
        NVS.putUInt("TimePlayer", 150000);
        NVS.end();
        Transmite_Confirmacion('E','D');
        delay(150);
        ESP.restart();
        break;

    case  3:

        NVS.begin("Config_ESP32", false);  /* 3 Minutos*/
        NVS.putUInt("TimePlayer", 180000);
        NVS.end();
        Transmite_Confirmacion('E','D');
        delay(150);
        ESP.restart();
        break;

    case  4:

        NVS.begin("Config_ESP32", false);  /* 3 Minutos & medio */
        NVS.putUInt("TimePlayer",  210000);
        NVS.end();
        Transmite_Confirmacion('E','D');
        delay(150);
        ESP.restart();
        break;

    case  5:

        NVS.begin("Config_ESP32", false);  /* 4 Minutos */
        NVS.putUInt("TimePlayer",  240000);
        NVS.end();
        Transmite_Confirmacion('E','D');
        delay(150);
        ESP.restart();
        break;

    case  6:

        NVS.begin("Config_ESP32", false);  /* 4 Minutos & medio */
        NVS.putUInt("TimePlayer",  270000);
        NVS.end();
        Transmite_Confirmacion('E','D');
        delay(150);
        ESP.restart();
        break;

    case  7:

        NVS.begin("Config_ESP32", false);  /* 5 Minutos */
        NVS.putUInt("TimePlayer",  300000);
        NVS.end();
        Transmite_Confirmacion('E','D');
        delay(150);
        ESP.restart();
        break;

    case  8:

        NVS.begin("Config_ESP32", false);  /* 5.5 Minutos */
        NVS.putUInt("TimePlayer",  330000);
        NVS.end();
        Transmite_Confirmacion('E','D');
        delay(150);
        ESP.restart();
        break;

    case  9:

        NVS.begin("Config_ESP32", false);  /* 6 Minutos */
        NVS.putUInt("TimePlayer",  360000);
        NVS.end();
        Transmite_Confirmacion('E','D');
        delay(150);
        ESP.restart();
        break;

    default:
        NVS.begin("Config_ESP32", false);
        NVS.putUInt("TimePlayer", 90000); /* 1 minutos & 30 segundos */
        NVS.end();
        Transmite_Confirmacion('E','D');
        delay(150);
        ESP.restart();
        break;
    }
}


/* Configura  Tiempo de Transmision de datos a Servidor cuan la maquina esta en juego
(0)=30s Por defecto!
(1)=40s
(2)=50s
(3)=1 Minuto
(4)=1 Minuto 10s
(5)=1 Minuto 20s
(6)=1 Minuto 30s
(7)=1 Minuto 40s
(8)=1 Minuto 50s
(9)=2 Minutos 
*/
void Config_Timer_Transmission_In_Game(char Datos[])
{

    switch (Datos[4]-48)
    {
    case  0:

        NVS.begin("Config_ESP32", false);
        NVS.putUInt("T_En_Juego", 30); /* 30 segundos */
        NVS.end();
        Transmite_Confirmacion('E','C');
        delay(150);
        ESP.restart();

        
        break;

    case  1:

        NVS.begin("Config_ESP32", false);  /* 40 Minutos */
        NVS.putUInt("T_En_Juego", 40);
        NVS.end();
        Transmite_Confirmacion('E','C');
        delay(150);
        ESP.restart();
        break;

    case  2:
       
        NVS.begin("Config_ESP32", false);  /* 50  Segundos */
        NVS.putUInt("T_En_Juego", 50);
        NVS.end();
        Transmite_Confirmacion('E','C');
        delay(150);
        ESP.restart();

        break;

    case  3:

        NVS.begin("Config_ESP32", false);  /* 1 Minuto */
        NVS.putUInt("T_En_Juego", 60);
        NVS.end();
        Transmite_Confirmacion('E','C');
        delay(150);
        ESP.restart();
        break;

    case  4:

        NVS.begin("Config_ESP32", false);  /* 1 Minuto 10s*/
        NVS.putUInt("T_En_Juego",  70);
        NVS.end();
        Transmite_Confirmacion('E','C');
        delay(150);
        ESP.restart();
        break;

    case  5:

        NVS.begin("Config_ESP32", false);  /* 1 Minuto 20s */
        NVS.putUInt("T_En_Juego",  80);
        NVS.end();
        Transmite_Confirmacion('E','C');
        delay(150);
        ESP.restart();
        break;

    case  6:

        NVS.begin("Config_ESP32", false);  /* 1 Minuto 30s */
        NVS.putUInt("T_En_Juego",  90);
        NVS.end();
        Transmite_Confirmacion('E','C');
        delay(150);
        ESP.restart();
        break;

    case  7:

        NVS.begin("Config_ESP32", false);  /* 1 Minuto 40s */
        NVS.putUInt("T_En_Juego",  100);
        NVS.end();
        Transmite_Confirmacion('E','C');
        delay(150);
        ESP.restart();
        break;

    case  8:

        NVS.begin("Config_ESP32", false);  /* 1 Minuto 50s */
        NVS.putUInt("T_En_Juego",  110);
        NVS.end();
        Transmite_Confirmacion('E','C');
        delay(150);
        ESP.restart();
        break;
    
    case  9:

        NVS.begin("Config_ESP32", false);  /* 2 Minutos */
        NVS.putUInt("T_En_Juego",  120);
        NVS.end();
        Transmite_Confirmacion('E','C');
        delay(150);
        ESP.restart();
        break;

    default:
        NVS.begin("Config_ESP32", false);
        NVS.putUInt("T_En_Juego", 30); /* 1 minutos & 30 segundos */
        NVS.end();
        Transmite_Confirmacion('E','C');
        delay(150);
        ESP.restart();
        break;
    }
}

/* Configura Tiempo de  Transmision de datos  a servidor cuando la maquina no esta en juego
(0)=2.5 Minutos Por defecto!
(1)=3 Minutos
(2)=3.5 Minutos 
(3)=4 Minutos 
(4)=4.5 Minutos 
(5)=5 Minutos 
(6)=5.5 Minutos 
(7)=6 Minutos
(8)=8 Minutos
(9)=10 Minutos 
 */
void Config_Timer_Transmission_No_Game(char Datos[])
{
    switch (Datos[4]-48)
    {
    case  0:

        NVS.begin("Config_ESP32", false);
        NVS.putUInt("T_No_Juego", 150000); /* 2.5 minutos */
        NVS.end();
        Transmite_Confirmacion('E','A');
        delay(150);
        ESP.restart();
        break;

    case  1:

        NVS.begin("Config_ESP32", false);  /* 3 Minutos */
        NVS.putUInt("T_No_Juego", 180000);
        NVS.end();
        Transmite_Confirmacion('E','A');
        delay(150);
        ESP.restart();
        break;

    case  2:

        NVS.begin("Config_ESP32", false);  /* 3.5 Minutos */
        NVS.putUInt("T_No_Juego", 210000);
        NVS.end();
        Transmite_Confirmacion('E','A');
        delay(150);
        ESP.restart();
        break;

    case  3:

        NVS.begin("Config_ESP32", false);  /* 4 Minutos*/
        NVS.putUInt("T_No_Juego", 240000);
        NVS.end();
        Transmite_Confirmacion('E','A');
        delay(150);
        ESP.restart();
        break;

    case  4:

        NVS.begin("Config_ESP32", false);  /* 4.5 */
        NVS.putUInt("T_No_Juego",  270000);
        NVS.end();
        Transmite_Confirmacion('E','A');
        delay(150);
        ESP.restart();
        break;

    case  5:

        NVS.begin("Config_ESP32", false);  /* 5 Minutos */
        NVS.putUInt("T_No_Juego",  300000);
        NVS.end();
        Transmite_Confirmacion('E','A');
        delay(150);
        ESP.restart();
        break;

    case  6:

        NVS.begin("Config_ESP32", false);  /* 5.5 Minutos */
        NVS.putUInt("T_No_Juego",  330000);
        NVS.end();
        Transmite_Confirmacion('E','A');
        delay(150);
        ESP.restart();
        break;

    case  7:

        NVS.begin("Config_ESP32", false);  /* 6 Minutos */
        NVS.putUInt("T_No_Juego",  360000);
        NVS.end();
        Transmite_Confirmacion('E','A');
        delay(150);
        ESP.restart();
        break;

     case  8:

        NVS.begin("Config_ESP32", false);  /* 8 Minutos */
        NVS.putUInt("T_No_Juego",  480000);
        NVS.end();
        Transmite_Confirmacion('E','A');
        delay(150);
        ESP.restart();
        break;


    case  9:

        NVS.begin("Config_ESP32", false);  /* 10 Minutos */
        NVS.putUInt("T_No_Juego",  600000);
        NVS.end();
        Transmite_Confirmacion('E','A');
        delay(150);
        ESP.restart();
        break;

    default:
        NVS.begin("Config_ESP32", false);
        NVS.putUInt("T_No_Juego", 150000); /* 2.5 minutos */
        NVS.end();
        Transmite_Confirmacion('E','A');
        delay(150);
        ESP.restart();
        break;
    }
}


void Config_Timer_Inactividad_Maquina(char Datos[])
{
    switch (Datos[4]-48)
    {
    case 0:
        NVS.begin("Config_ESP32", false);
        NVS.putUInt("TimerUser", 50); /* 30s */
        NVS.end();
        //Transmite_Confirmacion('E','A');
        delay(150);
        ESP.restart();
        break;

    case 1:
        NVS.begin("Config_ESP32", false);
        NVS.putUInt("TimerUser", 80); /* 1 Minuto */
        NVS.end();
        //Transmite_Confirmacion('E','A');
        delay(150);
        ESP.restart();
        break;

    case 2:
        NVS.begin("Config_ESP32", false);
        NVS.putUInt("TimerUser", 115); /* 1 Minuto 30s */
        NVS.end();
        //Transmite_Confirmacion('E','A');
        delay(150);
        ESP.restart();
        break;

    case 3:
        NVS.begin("Config_ESP32", false);
        NVS.putUInt("TimerUser", 150); /* 2 Minutos */
        NVS.end();
        //Transmite_Confirmacion('E','A');
        delay(150);
        ESP.restart();
        break;

    case 4:
        NVS.begin("Config_ESP32", false);
        NVS.putUInt("TimerUser", 180); /* 2 Minutos 30s */
        NVS.end();
        //Transmite_Confirmacion('E','A');
        delay(150);
        ESP.restart();
        break;

    case 5:
        NVS.begin("Config_ESP32", false);
        NVS.putUInt("TimerUser", 230); /* 3 Minutos*/
        NVS.end();
        //Transmite_Confirmacion('E','A');
        delay(150);
        ESP.restart();
        break;
    
    default:
        NVS.begin("Config_ESP32", false);
        NVS.putUInt("TimerUser", 50); /* 30s */
        NVS.end();
        //Transmite_Confirmacion('E','A');
        delay(150);
        ESP.restart();
        break;
    }
}
/*****************************************************************************************/
/********************************** TRANSMITE ACK CASHLESS *******************************/
/*****************************************************************************************/
void Transmite_Confirmacion_Cashless(char High, char Medium,char Low)
{
    if (Buffer.Set_buffer_ACK_Cashless(ID_Eventos, High,Medium, Low))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_ACK_Cashless(), 258);
        #ifdef Debug_Mensajes_Server
        Serial.println("Set buffer general OK");
        #endif
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
    }
    else
    {
        Serial.println("Set buffer general ERROR");
    }
}

/*****************************************************************************************/
/********************************** TRANSMITE INFO REGISTRO ******************************/
/*****************************************************************************************/

void Transmite_Info_Registro(void)
{
    if (Buffer.Set_buffer_Registro_MQ(300))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_Registro_MQ(), 258);
        #ifdef Debug_Mensajes_Server
        Serial.println("Set buffer general OK");
        #endif
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
    }
    else
    {
        Serial.println("Set buffer general ERROR");
    }
}

void Transmite_Confirmacion_Info(char High, char Low)
{
    if (Buffer.Set_buffer_ACK_Info(ID_Eventos, High, Low))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_Info_ACK(), 258);
        #ifdef Debug_Mensajes_Server
        Serial.println("Set buffer general OK");
        #endif
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
    }
    else
    {
        if (Variables_globales.Get_Variable_Global(SD_INSERT)==true)
        {
            if (!Variables_globales.Get_Variable_Global(Fallo_Archivo_LOG))
            {
                if (Variables_globales.Get_Variable_Global(Sincronizacion_RTC) == true && Variables_globales.Get_Variable_Global(Ftp_Mode==false))
                {
                    log_e("No se Pudo Guardar el Buffer General de datos", 101);
                    LOG_ESP(Archivo_LOG, Variables_globales.Get_Variable_Global(Enable_Storage));
                }
            }
        }
        Serial.println("Set buffer general ERROR");
    }
}
/*****************************************************************************************/
/************************** TRANSMITE CONTADORES ACCOUNTING ******************************/
/*****************************************************************************************/

void Transmite_Contadores_Accounting(void)
{
    if (Buffer.Set_buffer_contadores_ACC(ID_Contadores_Accounting, contadores, RTC, Variables_globales))
    {
        if (Variables_globales.Get_Variable_Global(Serializacion_Serie_Trama))
        {
            contadores.Incrementa_Serie_Trama();
        }
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_contadores_ACC(), 258);
        #ifdef Debug_Mensajes_Server
        Serial.println("Set buffer general OK");
        #endif
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
    }
    else
        Serial.println("Set buffer general ERROR");
}


/*****************************************************************************************/
/************************** TRANSMITE CONTADORES CASHLESS ********************************/
/*****************************************************************************************/

void Transmite_Contadores_Cashless(void)
{
    if (Buffer.Set_buffer_contadores_CASH(ID_Contadores_Cashless, contadores))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_contadores_CASH(), 258);
        #ifdef Debug_Mensajes_Server
        Serial.println("Set buffer general OK");
        #endif
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
    }
    else
        Serial.println("Set buffer general ERROR");
}

/*****************************************************************************************/
/**************************    TRANSMITE INFO CASHLESS    ********************************/
/*****************************************************************************************/

void Transmite_Info_Cashless(void)
{
    if (Buffer.Set_buffer_info_Cashless(ID_Informacion_Cashless))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_info_Cashless(), 258);
        Serial.println("Set buffer general OK");
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
    }
    else
        Serial.println("Set buffer general ERROR");
}

/*****************************************************************************************/
/**************************    TRANSMITE INFO CLIENTE    ********************************/
/*****************************************************************************************/

void Transmite_Info_Cliente(void)
{
    if (Buffer.Set_buffer_info_cliente(303,contadores))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_Buffer_info_cliente(), 258);
        #ifdef Debug_Mensajes_Server
        Serial.println("Set buffer general OK");
        #endif 
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
    }
    else
        Serial.println("Set buffer general ERROR");
}

/*****************************************************************************************/
/************************** TRANSMITE CONTADORES BILLETES ********************************/
/*****************************************************************************************/
void Transmite_Billetes(void)
{
    if (Buffer.Set_buffer_billetes(ID_Billetes, contadores))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_billetes(), 258);
        #ifdef Debug_Mensajes_Server
        Serial.println("Set buffer general OK");
        #endif
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
    }
    else
        Serial.println("Set buffer general ERROR");
}

void Transmite_Info_Memoria_SDCARD(void)
{
    char res[258] = {};
    bzero(res, 258); // Pone el buffer en 0
    memcpy(res, Buffer.Get_buffer_info_MicroSD(), 258);
    #ifdef Debug_Mensajes_Server
    Serial.println("Set buffer general OK");
    #endif
    int len = sizeof(res);
    Transmite_A_Servidor(res, len);
}

void Transmite_Info_Procesador_ESP32(void)
{
    char res[258] = {};
    bzero(res, 258); // Pone el buffer en 0
    memcpy(res, Buffer.Get_buffer_info_MCU(), 258);
    #ifdef Debug_Mensajes_Server
    Serial.println("Set buffer general OK");
    #endif
    int len = sizeof(res);
    Transmite_A_Servidor(res, len);
}

/*****************************************************************************************/
/******************************* SINCRONIZA RELOJ RTC ************************************/
/*****************************************************************************************/


bool Sincroniza_Reloj_RTC(char res[])
{
    int hour, minutes, seconds, day, month, year;

    hour = ((res[4] - 48) * 10) + (res[5] - 48);
    minutes = ((res[6] - 48) * 10) + (res[7] - 48);
    seconds = ((res[8] - 48) * 10) + (res[9] - 48);
    day = ((res[10] - 48) * 10) + (res[11] - 48);
    month = ((res[12] - 48) * 10) + (res[13] - 48);
    year = ((res[14] - 48) * 10) + (res[15] - 48);
    year = year + 2000;

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
        string_Fecha_Sesiones="Sesiones_RFID-"+String(day)+ String(month)+String(year)+".CSV";
        string_Fecha_Premios="Premios_Maquina-"+String(day)+String(month)+String(year)+".CSV";
        /*Convierte nombre de archivos en char*/
        strncpy(Archivo_CSV_Contadores, string_Fecha.c_str(),sizeof(Archivo_CSV_Contadores));
        strncpy(Archivo_LOG, string_Fecha_LOG.c_str(),sizeof(Archivo_LOG));
        strncpy(Archivo_CSV_Eventos, string_Fecha_Eventos.c_str(),sizeof(Archivo_CSV_Eventos));
        strncpy(Archivo_CSV_Sesiones, string_Fecha_Sesiones.c_str(),sizeof(Archivo_CSV_Sesiones));
        strncpy(Archivo_CSV_Premios, string_Fecha_Premios.c_str(),sizeof(Archivo_CSV_Premios));

        /* Crea copia de  fecha */
        day_copy = day;
        month_copy = month;
        year_copy = year;
        Variables_globales.Set_Variable_Global(Flag_Crea_Archivos, true);
        return true;
    }
}
    
/*****************************************************************************************/
/************************** TRANSMITE INFORMACION MAQUINA ********************************/
/*****************************************************************************************/

void Transmite_ID_Maquina(void)
{
    if (Buffer.Set_buffer_id_maq(ID_Maquina, contadores))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_id_maq(), 258);
        #ifdef Debug_Mensajes_Server
        Serial.println("Set buffer general OK");
        #endif
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
    }
    else
        Serial.println("Set buffer general ERROR");
}

/*****************************************************************************************/
/******************************* TRANSMITE EVENTOS SAS ***********************************/
/*****************************************************************************************/

void Transmite_Eventos(void)
{
    if (Buffer.Set_buffer_eventos(ID_Eventos, eventos, RTC))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_eventos(), 258);
        #ifdef Debug_Mensajes_Server
        Serial.println("Set buffer general OK");
        #endif
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
    }
    else
        Serial.println("Set buffer general ERROR");
}

/*****************************************************************************************/
/******************************* TRANSMITE ROM SIGNATURE *********************************/
/*****************************************************************************************/

void Transmite_ROM_Signature(void)
{
    if (Buffer.Set_buffer_ROM_Singnature(ID_ROM_Signature, contadores))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_ROM_Singnature(), 258);
        #ifdef Debug_Mensajes_Server
        Serial.println("Set buffer general OK");
        #endif
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
    }
    else
        Serial.println("Set buffer general ERROR");
}

/*****************************************************************************************/
/******************************* TRANSMITE ROM SIGNATURE *********************************/
/*****************************************************************************************/

bool Configura_Tipo_Maquina(char res[])
{
    uint16_t ID_Maq = Configuracion.Get_Configuracion(Tipo_Maquina, 0);
    uint16_t ID_Maq_Server = res[4] - 48;
    if(res[5]-48>=0)
    {
        if(res[5]-48==0)
        {
          ID_Maq_Server=(res[4] - 48)+9;
        }
        else if(res[5]-48==1 &&res[4]-48==1){
           ID_Maq_Server= 11;
        }
        else if(res[4]-48==1 &&res[5]-48==2)
        {
            ID_Maq_Server= 12;
        }

        else if(res[4]-48==1 &&res[5]-48==3)
        {
            ID_Maq_Server= 13;
        }
    }
    
    if (ID_Maq != ID_Maq_Server)
    {
        #ifdef Debug_Mensajes_Server
        Serial.println("Tipo de maquina diferente");
        #endif
        NVS.begin("Config_ESP32", false);
        NVS.putUInt("TYPE_MAQ", ID_Maq_Server);
        uint16_t tipo_maq = NVS.getUInt("TYPE_MAQ", 0);
        NVS.end();
        if (tipo_maq == ID_Maq_Server)
        {
            Configuracion.Set_Configuracion_ESP32(Tipo_Maquina, tipo_maq);
            return true;
        }
        else
            return false;
    }
    else
    {
        #ifdef Debug_Mensajes_Server
        Serial.println("Tipo de maquina igual");
        #endif 
        return true;
    }
}

/*****************************************************************************************/
/*******************************  TRANSMITE INFO TARJETA *********************************/
/*****************************************************************************************/


void Transmite_Info_Tarjeta(void)
{
    if (Buffer.Set_buffer_info_tarjeta(ID_Informacion_Tarjeta))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_info_tarjeta(), 258);
        #ifdef Debug_Mensajes_Server
        Serial.println("Set buffer general OK");
        #endif
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
    }
    else
        Serial.println("Set buffer general ERROR");
}



/*
Consulta y Transmite informacion de lector RFID

Lector Habilitado           (1) o deshabilidado (0)
Lector Ocupado              (1) o Libre         (0)
Lector Conectado            (1) o Desconectado  (0)
Sesion Abierta jugador      (1) o Cerrada       (0)
Reset Analogo Habilitado    (1) o Reset SAS     (0)

Tiempo Transmision Maq en juego.
Tiempo Transmision no Juego.
Tiempo  cierre Sesion Player Tracking.
*/
void Transmite_Info_Lector(void)
{
    if (Buffer.Set_buffer_info_lector(ID_Informacion_lector))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_info_lector(), 258);
        #ifdef Debug_Mensajes_Server
        Serial.println("Set buffer general OK");
        #endif
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
    }
    else
        Serial.println("Set buffer general ERROR");
}

/*****************************************************************************************/
/******************************* RESPONDE ECO BROADCAST **********************************/
/*****************************************************************************************/

void Transmite_Eco_Broadcast(void)
{
    char res[258] = {};
    bzero(res, 258); // Pone el buffer en 0

    // Direccion MAC ESP32
    #ifdef Debug_Mensajes_Server
    Serial.print("Direccion MAC: ");
    Serial.println(WiFi.macAddress());
    #endif
    String mac = WiFi.macAddress();

    // Direccion IP ESP32
    #ifdef Debug_Mensajes_Server
    Serial.print("Direccion IP Local: ");
    #endif
    uint32_t ip = WiFi.localIP();
    #ifdef Debug_Mensajes_Server
    Serial.println(WiFi.localIP());
    #endif

    // Socket de conexion
    #ifdef Debug_Mensajes_Server
    Serial.print("Socket: ");
    #endif
    string socket;
    if (Configuracion.Get_Configuracion(Tipo_Conexion))
    {
        #ifdef Debug_Mensajes_Server
        Serial.println(clientTCP.remotePort());
        #endif
        socket = std::to_string(clientTCP.remotePort());
    }
    else
    {
        #ifdef Debug_Mensajes_Server
        Serial.println(clientUDP.remotePort());
        #endif
        socket = std::to_string(clientUDP.remotePort());
    }

    // Mascara subred ESP32
    #ifdef Debug_Mensajes_Server
    Serial.print("Mascara Subred: ");
    Serial.println(WiFi.subnetMask());

    // Puerta de enlace ESP32
    Serial.print("Puerta de enlace: ");
    Serial.println(WiFi.gatewayIP());

    // Nombre de la maquina
    Serial.print("Nombre de MAQ: ");
    #endif
    String Name = Configuracion.Get_Configuracion(Nombre_Maquina, "Nombre_Maq");
    #ifdef Debug_Mensajes_Server
    Serial.println(Name);

    Serial.print("**************************************************************************************");
    Serial.println();
    #endif
    char IP[4];
    bzero(IP, 4);
    memcpy(IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(IP) / sizeof(IP[0]));

    char dir_ip[12] = {};
    int j = 0;
    for (int i = 0; i < 4; i++)
    {
        int octeto = (int)IP[i];
        dir_ip[j] = ((octeto - (octeto % 100)) / 100) + '0';
        j++;
        int resultado = octeto % 100;
        dir_ip[j] = ((resultado - (resultado % 10)) / 10) + '0';
        j++;
        dir_ip[j] = (resultado % 10) + '0';
        j++;
    }

    char IP_GW[4];
    bzero(IP_GW, 4);
    memcpy(IP_GW, Configuracion.Get_Configuracion(Direccion_IP_GW, 'x'), sizeof(IP_GW) / sizeof(IP_GW[0]));

    char dir_ip_gw[12] = {};
    j = 0;
    for (int i = 0; i < 4; i++)
    {
        int octeto = (int)IP_GW[i];
        dir_ip_gw[j] = ((octeto - (octeto % 100)) / 100) + '0';
        j++;
        int resultado = octeto % 100;
        dir_ip_gw[j] = ((resultado - (resultado % 10)) / 10) + '0';
        j++;
        dir_ip_gw[j] = (resultado % 10) + '0';
        j++;
    }

    char SN_MASK[4];
    bzero(SN_MASK, 4);
    memcpy(SN_MASK, Configuracion.Get_Configuracion(Direccion_SN_MASK, 'x'), sizeof(SN_MASK) / sizeof(SN_MASK[0]));

    char sn_mask[12] = {};
    j = 0;
    for (int i = 0; i < 4; i++)
    {
        int octeto = (int)SN_MASK[i];
        sn_mask[j] = ((octeto - (octeto % 100)) / 100) + '0';
        j++;
        int resultado = octeto % 100;
        sn_mask[j] = ((resultado - (resultado % 10)) / 10) + '0';
        j++;
        sn_mask[j] = (resultado % 10) + '0';
        j++;
    }

    res[0] = 'L';
    res[1] = '|';
    res[2] = '0';
    res[3] = '1';
    res[4] = '|';
    // MAC
    res[5] = mac[0];
    res[6] = mac[1];
    res[7] = mac[2];
    res[8] = mac[3];
    res[9] = mac[4];
    res[10] = mac[5];
    res[11] = mac[6];
    res[12] = mac[7];
    res[13] = mac[8];
    res[14] = mac[9];
    res[15] = mac[10];
    res[16] = mac[11];
    res[17] = mac[12];
    res[18] = mac[13];
    res[19] = mac[14];
    res[20] = mac[15];
    res[21] = mac[16];
    res[22] = '|';
    // PORT
    res[23] = '0';
    res[24] = socket[0];
    res[25] = socket[1];
    res[26] = socket[2];
    res[27] = socket[3];
    res[28] = '|';
    // IP
    res[29] = dir_ip[0];
    res[30] = dir_ip[1];
    res[31] = dir_ip[2];
    res[32] = '.';
    res[33] = dir_ip[3];
    res[34] = dir_ip[4];
    res[35] = dir_ip[5];
    res[36] = '.';
    res[37] = dir_ip[6];
    res[38] = dir_ip[7];
    res[39] = dir_ip[8];
    res[40] = '.';
    res[41] = dir_ip[9];
    res[42] = dir_ip[10];
    res[43] = dir_ip[11];
    res[44] = '|';
    // MASCARA
    res[45] = sn_mask[0];
    res[46] = sn_mask[1];
    res[47] = sn_mask[2];
    res[48] = '.';
    res[49] = sn_mask[3];
    res[50] = sn_mask[4];
    res[51] = sn_mask[5];
    res[52] = '.';
    res[53] = sn_mask[6];
    res[54] = sn_mask[7];
    res[55] = sn_mask[8];
    res[56] = '.';
    res[57] = sn_mask[9];
    res[58] = sn_mask[10];
    res[59] = sn_mask[11];
    res[60] = '|';
    // IP Enlace
    res[61] = dir_ip_gw[0];
    res[62] = dir_ip_gw[1];
    res[63] = dir_ip_gw[2];
    res[64] = '.';
    res[65] = dir_ip_gw[3];
    res[66] = dir_ip_gw[4];
    res[67] = dir_ip_gw[5];
    res[68] = '.';
    res[69] = dir_ip_gw[6];
    res[70] = dir_ip_gw[7];
    res[71] = dir_ip_gw[8];
    res[72] = '.';
    res[73] = dir_ip_gw[9];
    res[74] = dir_ip_gw[10];
    res[75] = dir_ip_gw[11];
    res[76] = '|';
    // Nombre MAQ
    res[77] = Name[0];
    res[78] = Name[1];
    res[79] = Name[2];
    res[80] = Name[3];
    res[81] = Name[4];
    res[82] = Name[5];
    res[83] = Name[6];
    res[84] = Name[7];
    res[85] = Name[8];
    res[86] = Name[9];
    res[87] = Name[10];
    res[88] = Name[11];
    res[89] = Name[12];
    res[90] = Name[13];
    res[91] = Name[14];
    res[92] = Name[15];

    #ifdef Debug_Mensajes_Server
    Serial.println("Set buffer general OK");
    #endif

    int len = sizeof(res);
    Transmite_A_Servidor(res, len);
    ;
}

/*****************************************************************************************/
/*************************** GUARDA LA NUEVA CONFIGURACION *******************************/
/*****************************************************************************************/

void Guarda_Configuracion_ESP32(void)
{
    char req[258] = {};
    bzero(req, 258); // Pone el buffer en 0
    memcpy(req, Buffer.Get_buffer_recepcion(), 258);

    char res[258] = {};
    bzero(res, 258); // Pone el buffer en 0

    bool cambios = false;

    if (req[91] != 0x0A || req[92] != 0x0D)
    {
        #ifdef Debug_Mensajes_Server
        Serial.println();
        Serial.println("Error de Set All...");
        Serial.println(req[91], HEX);
        Serial.println(req[92], HEX);
        #endif
        res[0] = 'E';
        res[1] = '0';
        res[2] = '0';
        res[3] = 0x0A;
        res[4] = 0x0D;

        for (int i = 5; i < 256; i++)
        {
            res[i] = '0';
        }
        #ifdef Debug_Mensajes_Server
        Serial.println("Set buffer general OK");
        #endif
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
    }
    else
    {
        int j = 0;
        bool diferencia = false;

        /*******************************************************************************************************/
        /*******************************************************************************************************/
        // Verifica MAC
        #ifdef Debug_Mensajes_Server
        Serial.println();
        for (int i = 3; i < 20; i++)
        {
            Serial.print(req[i]);
        }
        #endif
        String MAC = WiFi.macAddress();
        j = 0;
        for (int i = 3; i < 20; i++)
        {
            if (req[i] != MAC[j])
                return;
            j++;
        }
        #ifdef Debug_Mensajes_Server
        Serial.println();
        Serial.println("Set all Ok...");
        #endif

        /*******************************************************************************************************/
        /*******************************************************************************************************/
        // Verifica Puerto de conexion
        #ifdef Debug_Mensajes_Server
        for (int i = 21; i < 26; i++)
        {
            Serial.print(req[i]);
        }
        Serial.println();
        #endif
        uint16_t puerto = Configuracion.Get_Configuracion(Puerto_Server, 0);
        uint16_t puerto_server;
        puerto_server = ((int)req[22] - 48) * 1000;
        puerto_server += ((int)req[23] - 48) * 100;
        puerto_server += ((int)req[24] - 48) * 10;
        puerto_server += ((int)req[25] - 48);

        if (puerto != puerto_server)
        {
            NVS.begin("Config_ESP32", false);
            #ifdef Debug_Mensajes_Server
            Serial.println("Si hay diferencias en Puerto de conexion...");
            #endif
            NVS.putUInt("Socket", puerto_server);
            uint16_t port_server = NVS.getUInt("Socket", 0);
            Configuracion.Set_Configuracion_ESP32(Puerto_Server, port_server);
            cambios = true;
            NVS.end();
        }

        /*******************************************************************************************************/
        /*******************************************************************************************************/
        // Verifica direccion IP Local
        #ifdef Debug_Mensajes_Server
        for (int i = 27; i < 42; i++)
        {
            Serial.print(req[i]);
        }
        Serial.println();
        #endif
        char IP[4];
        bzero(IP, 4);
        memcpy(IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(IP) / sizeof(IP[0]));
        char dir_ip[12] = {};
        char dir_ip_server[12] = {};
        j = 0;
        for (int i = 0; i < 4; i++)
        {
            int octeto = (int)IP[i];
            dir_ip[j] = ((octeto - (octeto % 100)) / 100) + '0';
            j++;
            int resultado = octeto % 100;
            dir_ip[j] = ((resultado - (resultado % 10)) / 10) + '0';
            j++;
            dir_ip[j] = (resultado % 10) + '0';
            j++;
        }
        j = 0;
        for (int i = 27; i < 42; i++)
        {
            if (req[i] != '.')
            {
                dir_ip_server[j] = req[i];
                j++;
            }
        }
        for (int i = 0; i < 12; i++)
        {
            if (dir_ip[i] != dir_ip_server[i])
                diferencia = true;
        }
        if (diferencia)
        {
            NVS.begin("Config_ESP32", false);
            #ifdef Debug_Mensajes_Server
            Serial.println("Si hay diferencias en IP...");
            #endif
            uint8_t ip[4] = {};
            j = 0;
            for (int i = 0; i < 4; i++)
            {
                int numero = (dir_ip_server[j] - 48);
                int octeto = numero * 100;
                j++;
                numero = (dir_ip_server[j] - 48);
                octeto += (numero * 10);
                j++;
                numero = (dir_ip_server[j] - 48);
                octeto += numero;
                j++;
                ip[i] = octeto;
            }

            char Ip_Server_Actual[4];
            
            bzero(Ip_Server_Actual, 4);
            memcpy(Ip_Server_Actual, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(Ip_Server_Actual) / sizeof(Ip_Server_Actual[0]));

            uint8_t ip_server[] = {192, 168, ip[2], Ip_Server_Actual[3]};
            if (NVS.putBytes("Dir_IP", ip, sizeof(ip)) == 4 && NVS.putBytes("Dir_IP_Serv", ip_server, sizeof(ip_server)) == 4)
            {
                size_t ip_len = NVS.getBytesLength("Dir_IP");
                char IP[ip_len];
                NVS.getBytes("Dir_IP", IP, ip_len);
                Configuracion.Set_Configuracion_ESP32(Direccion_IP, IP);

                size_t ip_serv_len = NVS.getBytesLength("Dir_IP_Serv");
                char IP_SERV[ip_serv_len];
                NVS.getBytes("Dir_IP_Serv", IP_SERV, ip_serv_len);
                Configuracion.Set_Configuracion_ESP32(Direccion_IP_Server, IP_SERV);
            }
            NVS.end();
            diferencia = false;
            cambios = true;
        }

        /*******************************************************************************************************/
        /*******************************************************************************************************/
        // Verifica MASK_SUBRED
        #ifdef Debug_Mensajes_Server
        for (int i = 43; i < 58; i++)
        {
            Serial.print(req[i]);
        }
        Serial.println();
        #endif
        char SN_MASK[4];
        bzero(SN_MASK, 4);
        memcpy(SN_MASK, Configuracion.Get_Configuracion(Direccion_SN_MASK, 'x'), sizeof(SN_MASK) / sizeof(SN_MASK[0]));
        char sn_mask[12] = {};
        char sn_mask_server[12] = {};
        j = 0;
        for (int i = 0; i < 4; i++)
        {
            int octeto = (int)SN_MASK[i];
            sn_mask[j] = ((octeto - (octeto % 100)) / 100) + '0';
            j++;
            int resultado = octeto % 100;
            sn_mask[j] = ((resultado - (resultado % 10)) / 10) + '0';
            j++;
            sn_mask[j] = (resultado % 10) + '0';
            j++;
        }
        j = 0;
        for (int i = 43; i < 58; i++)
        {
            if (req[i] != '.')
            {
                sn_mask_server[j] = req[i];
                j++;
            }
        }
        for (int i = 0; i < 12; i++)
        {
            if (sn_mask[i] != sn_mask_server[i])
                diferencia = true;
        }
        if (diferencia)
        {
            NVS.begin("Config_ESP32", false);
            #ifdef Debug_Mensajes_Server
            Serial.println("Si hay diferencias en SN_MASK...");
            #endif
            uint8_t sn_mask[4] = {};
            j = 0;
            for (int i = 0; i < 4; i++)
            {
                int numero = (sn_mask_server[j] - 48);
                int octeto = numero * 100;
                j++;
                numero = (sn_mask_server[j] - 48);
                octeto += (numero * 10);
                j++;
                numero = (sn_mask_server[j] - 48);
                octeto += numero;
                j++;
                sn_mask[i] = octeto;
            }
            if (NVS.putBytes("Dir_SN_MASK", sn_mask, sizeof(sn_mask)) == 4)
            {
                size_t sn_mask_len = NVS.getBytesLength("Dir_SN_MASK");
                char SN_MASK[sn_mask_len];
                NVS.getBytes("Dir_SN_MASK", SN_MASK, sn_mask_len);
                Configuracion.Set_Configuracion_ESP32(Direccion_SN_MASK, SN_MASK);
            }
            NVS.end();
            diferencia = false;
            cambios = true;
        }

        /*******************************************************************************************************/
        /*******************************************************************************************************/
        // Verifica IP_GW

        #ifdef Debug_Mensajes_Server
        for (int i = 59; i < 74; i++)
        {
            Serial.print(req[i]);
        }
        Serial.println();
        #endif
        char IP_GW[4];
        bzero(IP_GW, 4);
        memcpy(IP_GW, Configuracion.Get_Configuracion(Direccion_IP_GW, 'x'), sizeof(IP_GW) / sizeof(IP_GW[0]));
        char ip_gw[12] = {};
        char ip_gw_server[12] = {};
        j = 0;
        for (int i = 0; i < 4; i++)
        {
            int octeto = (int)IP_GW[i];
            ip_gw[j] = ((octeto - (octeto % 100)) / 100) + '0';
            j++;
            int resultado = octeto % 100;
            ip_gw[j] = ((resultado - (resultado % 10)) / 10) + '0';
            j++;
            ip_gw[j] = (resultado % 10) + '0';
            j++;
        }
        j = 0;
        for (int i = 59; i < 74; i++)
        {
            if (req[i] != '.')
            {
                ip_gw_server[j] = req[i];
                j++;
            }
        }
        for (int i = 0; i < 12; i++)
        {
            if (ip_gw[i] != ip_gw_server[i])
                diferencia = true;
        }
        if (diferencia)
        {
            NVS.begin("Config_ESP32", false);
            #ifdef Debug_Mensajes_Server
            Serial.println("Si hay diferencias en IP_Enlace...");
            #endif
            uint8_t ip_gw[4] = {};
            j = 0;
            for (int i = 0; i < 4; i++)
            {
                int numero = (ip_gw_server[j] - 48);
                int octeto = numero * 100;
                j++;
                numero = (ip_gw_server[j] - 48);
                octeto += (numero * 10);
                j++;
                numero = (ip_gw_server[j] - 48);
                octeto += numero;
                j++;
                ip_gw[i] = octeto;
            }
            if (NVS.putBytes("Dir_IP_GW", ip_gw, sizeof(ip_gw)) == 4)
            {
                size_t ip_gw_len = NVS.getBytesLength("Dir_IP_GW");
                char IP_GW[ip_gw_len];
                NVS.getBytes("Dir_IP_GW", IP_GW, ip_gw_len);
                Configuracion.Set_Configuracion_ESP32(Direccion_IP_GW, IP_GW);
            }
            NVS.end();
            diferencia = false;
            cambios = true;
        }

        /*******************************************************************************************************/
        /*******************************************************************************************************/
        // Verifica Nombre de MAQ
        char Nombre_Maq[16];
        bzero(Nombre_Maq, 16);
        char Nombre_Maq_Server[17];
        bzero(Nombre_Maq_Server, 17);
        diferencia = false;
        strcpy(Nombre_Maq, Configuracion.Get_Configuracion(Nombre_Maquina, "Nombre_Maq").c_str());
        j = 0;
        for (int i = 75; i < 91; i++)
        {
            Nombre_Maq_Server[j] = req[i];
            if (Nombre_Maq[j] != req[i])
                diferencia = true;
            j++;
        }
        if (diferencia)
        {
            NVS.begin("Config_ESP32", false);
            #ifdef Debug_Mensajes_Server
            Serial.println("Si hay diferencias en NOMBRE_MAQ...");
            #endif
            if (NVS.putString("Name_Maq", Nombre_Maq_Server) == 16)
            {
                String name = NVS.getString("Name_Maq");
                Configuracion.Set_Configuracion_ESP32(Nombre_Maquina, name);
            }
            NVS.end();
            diferencia = false;
            cambios = true;
        }

        /*******************************************************************************************************/
        /*******************************************************************************************************/

        res[0] = 'L';
        res[1] = '|';
        res[2] = '0';
        res[3] = '2';
        res[4] = '|';
        res[5] = 'O';
        res[6] = 'K';
        res[7] = '|';
        res[8] = 0x0A;
        res[9] = 0x0D;
        res[10] = '|';
        
        Encuesta_ROM();
        delay(600);
        String string_dato = String(contadores.Get_Contadores_Char(ROM_Signature)[0], HEX);
        String string_dato1 = String(contadores.Get_Contadores_Char(ROM_Signature)[1], HEX);


        char Valida[2];

        Valida[0]=contadores.Get_Contadores_Char(ROM_Signature)[0];
        Valida[1]=contadores.Get_Contadores_Char(ROM_Signature)[1];


        if(Valida[0]!='0'&& Valida[1]!='0')
        {

            (string_dato[0] > 96) ? res[11] = string_dato[0] - 32 : res[11] = string_dato[0];
            (string_dato[1] > 96) ? res[12] = string_dato[1] - 32 : res[12] = string_dato[1];

        //    Serial.println(res[1], HEX);

        
        
            (string_dato1[0] > 96) ? res[13] = string_dato1[0] - 32 : res[13] = string_dato1[0];
            (string_dato1[1] > 96) ? res[14] = string_dato1[1] - 32 : res[14] = string_dato1[1];
            res[15]='&';
        }else{
            res[11] = '0';
            res[12] = '0';
            res[13] = '0';
            res[14] = '0';
            res[15] = '&';
        }
       

        for (int i = 16; i < 256; i++)
        {
            res[i] = '0';
        }
        #ifdef Debug_Mensajes_Server
        Serial.println("Set buffer general OK");
        #endif
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
        if (cambios)
            delay(600);
            ESP.restart();
    }
}

/*****************************************************************************************/
/*************************** INICIALIZA MODO BOOTLOADER **********************************/
/***************************************************************************RETR**********/

bool Inicializa_modo_bootloader(void)
{
    char res[258] = {};
    bzero(res, 258); // Pone el buffer en 0

    Variables_globales.Set_Variable_Global(Bootloader_Mode, true);
    if (Variables_globales.Get_Variable_Global(Bootloader_Mode))
    {
        res[0] = 'L';
        res[1] = '|';
        res[2] = 'S';
        res[3] = 'B';
        res[4] = '|';
        res[5] = 'O';
        res[6] = 'K';

        for (int i = 7; i < 256; i++)
        {
            res[i] = '0';
        }
        #ifdef Debug_Mensajes_Server 
        Serial.println("Set buffer general OK");
        #endif
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
        return true;
    }
}

bool Consulta_Conexion_To_Server(void)
{
    char res[258] = {};
    bzero(res, 258); // Pone el buffer en 0
    int i=0;

    res[0] = 'L';
    res[1] = '|';
    res[2] = 'C';
    res[3] = 'V';
    res[4] = '|';
    
    res[5]=contadores.Get_Operador_INFO_Operador()[0];
    res[6]=contadores.Get_Operador_INFO_Operador()[1];
    res[7]=contadores.Get_Operador_INFO_Operador()[2];
    res[8]=contadores.Get_Operador_INFO_Operador()[3];
    res[9]=contadores.Get_Operador_INFO_Operador()[4];
    res[10]=contadores.Get_Operador_INFO_Operador()[5];
    res[11]=contadores.Get_Operador_INFO_Operador()[6];
    res[12]=contadores.Get_Operador_INFO_Operador()[7];
    
    res[13]='|';

    for (int i = 14; i < 256; i++)
    {
        res[i] = '0';
    }
    
    #ifdef Debug_Mensajes_Server
    Serial.println("Set buffer general OK");
    #endif
    int len = sizeof(res);

    for(i=0; i<2; i++)
    {
        Transmite_A_Servidor(res, len);
        delay(200);
    }
    contadores.Dele_Operador_INFO_Operador();
    return true;
}

/*****************************************************************************************/
/*************************** VALIDA CONTRASEÑA BOOTLOADER ********************************/
/*****************************************************************************************/

bool Valida_contrasena_Bootloader(char buffer[])
{
    
    int i,j; // Iteradores 
    j=0;
    String password="St4rt$B0ot&G1ob5";

    #ifdef Debug_Mensajes_Server
    for (int i = 0; i < 256; i++)
    {
        Serial.print(buffer[i]);
    }
    Serial.println();
    #endif
    for(i=4; i<20; i++)
    {
        if(password[j]!=buffer[i])
        {
            Selector_Modo_SD();
            log_e("Contrasena incorrecta modo Bootloader", Fallo_Contrasena_Bootloader);
            LOG_ESP(Archivo_LOG, Variables_globales.Get_Variable_Global(Enable_Storage));
            return false;
        }
        j++;
    }
    return true;
}

bool Enable_Disable_modo_Ftp_server(bool Enable_S)
{
    char res[258] = {};
    bzero(res, 258); // Pone el buffer en 0

    Variables_globales.Set_Variable_Global(Enable_Storage, false); // Deshabilita Guardado de Datos.
    if (!Variables_globales.Get_Variable_Global(Enable_Storage))
    {
        Variables_globales.Set_Variable_Global(Ftp_Mode, true); // Activa ftp
        if (Variables_globales.Get_Variable_Global(Ftp_Mode))
        {
            Transmite_Confirmacion('T', 'P');
            return true;
        }
        else
        {
            return false;
        }
    }
    return false;
}

/*****************************************************************************************/
/****************************** ACTUALIZA TRAJETA MECANICA *******************************/
/*****************************************************************************************/

bool Actualiza_Tarjeta_Mecanica(char res[])
{
    if (Buffer.Set_buffer_tarjeta_mecanica(res))
    {
        if (Verifica_Tarjeta_Mecanica())
            return true;
        else
            return false;
    }
    else
        return false;
}

/*****************************************************************************************/
/******************************* PROCESA COMANDO RECIBIDO ********************************/
/*****************************************************************************************/

int Comando_Recibido(void)
{
    int32_t Comando, Aux1, Aux2, Aux3, Aux4;

    char res[258] = {};
    bzero(res, 258); // Pone el buffer en 0
    memcpy(res, Buffer.Get_buffer_recepcion(), 258);

    Aux1 = res[0];
    Aux2 = res[1];
    Aux2 = Aux2 << 8;
    Aux3 = res[2];
    Aux3 = Aux3 << 16;
    Aux4 = res[3];
    Aux4 = Aux4 << 24;
    Comando = Aux4 | Aux3 | Aux2 | Aux1;
    return (Comando);
}

/*****************************************************************************************/
/********************************** MENSAJES RFID ****************************************/
/*****************************************************************************************/

void Mensajes_RFID(void)
{

    /* -------------------------> Contadores Inicio sesion RFID<-----------------------------------*/
    if(Variables_globales.Get_Variable_Global(Flag_Contadores_Sesion_ON))  
    {
        Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 0);
        esp_task_wdt_reset();
        Encuesta_Creditos_Premio(); /*Encuesta creditos antes de envio de contadores*/
        delay(300);
        esp_task_wdt_reset();
        #ifdef Debug_Mensajes_RFID
        Serial.println("Contadores Sesion RFID Iniciada....");
        #endif
        if(Variables_globales.Get_Variable_Global(Comunicacion_Maq))
        {
            Transmite_Contadores_Accounting();
            delay(50);
            Transmite_Contadores_Accounting();
        }else{
            Transmite_Confirmacion('A', '0');
        }
        Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);       
        Variables_globales.Set_Variable_Global(Flag_Contadores_Sesion_ON, false);
    }
    /*---------------------------------------------------------------------------------------------*/
    if(Variables_globales.Get_Variable_Global(Flag_Contadores_Sesion_OFF))  
    {

        if(Variables_globales.Get_Variable_Global(Comunicacion_Maq))
        {
            Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 0);
            esp_task_wdt_reset();
            Encuesta_Creditos_Premio(); /*Encuesta creditos antes de envio de contadores*/
            delay(300);
            esp_task_wdt_reset();
            #ifdef Debug_Mensajes_RFID
            Serial.println("Contadores Sesion RFID Terminada....");
            #endif
            Transmite_Contadores_Accounting();
            delay(50);
            Transmite_Contadores_Accounting();    
        }else{
            Transmite_Confirmacion('A', '0');
        }
        Variables_globales.Set_Variable_Global(Handle_RFID_Lector,false);
        Variables_globales.Set_Variable_Global(Flag_Maquina_En_Juego, false);
        Variables_globales.Set_Variable_Global(Flag_Contadores_Sesion_OFF, false);
    }
    /* -------------------------> Contadores Sesion Cerrada<---------------------------------------*/
    
    /* -------------------------> Reset Handpay operador <-----------------------------------------*/
    if(Variables_globales.Get_Variable_Global(Operador_Detected))
    {
        #ifdef Debug_Mensajes_RFID
        Serial.println("Solicitud Reset Premio Handpay via RFID");
        #endif
        if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
        {
            
            delay(5);
            if(Variables_globales.Get_Variable_Global(Type_Hanpay_Reset))
            {
                if (Variables_globales.Get_Variable_Global(Type_Hanpay_Reset) != 9)
                {
                    if (!Variables_globales.Get_Variable_Global(Reset_Handpay_in_Process))
                    {
                        Variables_globales.Set_Variable_Global(Manual_Detected, true);
                        Variables_globales.Set_Variable_Global(Manual_Reset, true);
                    }else{
                            if(Variables_globales.Get_Variable_Global(Conexion_RFID))
                                Status_Barra(ERROR_RESET_HANDPAY);

                            Variables_globales.Set_Variable_Global(Handle_RFID_Lector,false);
                        }
                }
            }
            else
            {
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9)
                {
                    esp_task_wdt_reset();
                    Reset_HandPay();
                    delay(200);
                    esp_task_wdt_reset();
                    switch (Variables_globales.Get_Variable_Global_Char(Reset_Handay_OK))
                    {
                    case 0x00:
                        New_Timer_Final = New_Timmer_Inicial; /*RESET TIMEOUT*/
                        startTime = currentTime;
                        /*Reset realizado con exito*/
                        // contadores.Copy_Operator_In_(); // Add ID operador a Trama
                        Transmite_Confirmacion('C', '0'); /* Trasmite ACK */
                        Variables_globales.Set_Variable_Global_Char(Reset_Handay_OK, 0x04);
                        Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 0);
                        Encuesta_Creditos_Premio(); /*Encuesta creditos despues de premio*/
                        delay(350);
                        Transmite_Contadores_Accounting(); /* Envia Contadores*/
                        Variables_globales.Set_Variable_Global(Flag_Creditos_D_P, false);
                        if (Variables_globales.Get_Variable_Global(Conexion_RFID))
                            Status_Barra(Reset_Exitoso);
                        Reset_Handle_LED();
                        Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                        
                        break;
                    case 0x01: /*No existe condición de reset*/
                        /* Guarda ID Operador */
                        contadores.Close_ID_Operador();
                        startTime = currentTime;
                        Transmite_Confirmacion('C', '1');
                        Variables_globales.Set_Variable_Global_Char(Reset_Handay_OK, 0x04);
                        Variables_globales.Set_Variable_Global(Flag_Maquina_En_Juego, false);
                       // contadores.Delete_Operator_ID_Temp();
                        if (Variables_globales.Get_Variable_Global(Conexion_RFID))
                            Status_Barra(ERROR_RESET_HANDPAY);
                        Reset_Handle_LED();
                        Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                        break;
                    case 0x02: /*Imposible realizar reset*/
                        contadores.Close_ID_Operador();
                        startTime = currentTime;
                        Transmite_Confirmacion('C', '2');
                        // Variables_globales.Set_Variable_Global(Trama_Pendiente,true);
                        Variables_globales.Set_Variable_Global_Char(Reset_Handay_OK, 0x04);
                        //contadores.Delete_Operator_ID_Temp();
                        if (Variables_globales.Get_Variable_Global(Conexion_RFID))
                            Status_Barra(ERROR_RESET_HANDPAY);
                        Reset_Handle_LED();
                        Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                        break;
                    case 0x04: /* No  hay respuesta de la maquina*/
                        startTime = currentTime;
                        Transmite_Confirmacion('C', '4');
                        Variables_globales.Set_Variable_Global_Char(Reset_Handay_OK, 0x04);
                        if (Variables_globales.Get_Variable_Global(Conexion_RFID))
                            Status_Barra(ERROR_RESET_HANDPAY);
                        Reset_Handle_LED();
                        /*Activa Timer  Operador */
                        /*  Maquinas sin rele y sin comando */
                        Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                        break;
                    }
                }
            }
        }
        else
        {
            Transmite_Confirmacion('A', '0');
        }
        Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
        Variables_globales.Set_Variable_Global(Operador_Detected, false);
    }
    /* --------------------------------------------------------------------------------------------*/
    /* -------------------------> CASHLESS <--------------------------------------------------------*/
    if(Variables_globales.Get_Variable_Global(Consulta_Conexion_To_Host))
    {
        Consulta_Conexion_To_Server();
        delay(10);
        Variables_globales.Set_Variable_Global(Consulta_Conexion_To_Host,false);
    }
   
    if(Variables_globales.Get_Variable_Global(Consulta_Info_Cliente))
    {
        #ifdef Debug_Mensajes_RFID
        Serial.println("Consulta info cliente....");
        #endif
        Transmite_Info_Cliente();
        Variables_globales.Set_Variable_Global(Consulta_Info_Cliente,false);
    }
    /*-----------------------------------------------------------------------------------------------*/
}


/*****************************************************************************************/
/********************************** TAREA DE COMANDOS ************************************/
/*****************************************************************************************/

void completarConCeros(char buffer[], int longitudDeseada) {
    int i;
    int longitudActual = strlen(buffer);
    if (longitudActual < longitudDeseada) {
        int cerosFaltantes = longitudDeseada - longitudActual;

        // Mover los caracteres existentes a la derecha
        for (i = longitudActual - 1; i >= 0; i--) {
            buffer[i + cerosFaltantes] = buffer[i];
        }

        // Rellenar con ceros a la izquierda
        for (i = 0; i < cerosFaltantes; i++) {
            buffer[i] = '0';
        }
    }
}



/* Funcion  Para Recibir ACK SERVER */
void Task_Procesa_Comandos(void *parameter)
{
    /* WDT para tarea */
  //  esp_task_wdt_init(100000, true);
  //  esp_task_wdt_add(NULL);
   unsigned long Respuesta_Carga_Bonus = millis();
   int TIMEOUT_CONECT_SERVER = 3500;

   int Dest;
   char Compuesto[258];
    bzero(Compuesto, 258);

    for (;;)
    {
        Verifica_Cambio_Contadores();

        if (Variables_globales.Get_Variable_Global(Dato_Entrante_Valido))
        {
           // esp_task_wdt_reset();
            Variables_globales.Set_Variable_Global(Dato_Entrante_Valido, false);
            //            flag_dato_valido_recibido = false;

            char res[258] = {};
            bzero(res, 258); // Pone el buffer en 0

            char Tmp[258] = {};
            bzero(Tmp, 258);

            memcpy(res, Buffer.Get_buffer_recepcion(), 258);
           
            switch (Comando_Recibido())
            {
            case 3:
            #ifdef Debug_Mensajes_Server
                Serial.println("Solicitud de contadores SAS");
            #endif
                if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                {
                    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)==6)
                    {
                        if(Variables_globales.Get_Variable_Global(Primer_Cancel_Credit)==false)
                        {
                            if (Calcula_First_Cancel_Credit(true))
                            {
                                Variables_globales.Set_Variable_Global(Primer_Cancel_Credit, true);
                            }
                        }
                        Transmite_Contadores_Accounting();
                    }
                    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 6)
                    {
                        Transmite_Contadores_Accounting();
                    }
                }
                else
                {
                    Transmite_Confirmacion('A', '0');
                }
                break;
            case 4:
                #ifdef Debug_Mensajes_Server
                Serial.println("Solicitud de billetes SAS");
                #endif
                if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                    Transmite_Billetes();
                else
                    Transmite_Confirmacion('A', '0');
                break;
                
            case 5:
                #ifdef Debug_Mensajes_Server
                Serial.println("Solicitud de Eventos SAS");
                #endif
                Transmite_Eventos();
                break;

            case 7:
                #ifdef Debug_Mensajes_Server
                Serial.println("Evento recibido con exito");
                #endif
                Maneja_Marca_Eventos();
                break;

            case 8:
                #ifdef Debug_Mensajes_Server
                Serial.println("Sincroniza reloj RTC");
                #endif
                if (Sincroniza_Reloj_RTC(res))
                {
                    Variables_globales.Set_Variable_Global(Sincronizacion_RTC, true);
                    Bootloader_Enable=Variables_globales.Get_Variable_Global(Sincronizacion_RTC);
                    Transmite_Confirmacion('A', '4');
                }
                else
                {
                    Transmite_Confirmacion('A', '5');
                    Bootloader_Enable=Variables_globales.Get_Variable_Global(Sincronizacion_RTC);
                }
                break;
            case 11:
                #ifdef Debug_Mensajes_Server
                Serial.println("Solicitud de informacion MAQ");
                #endif
                if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                    Transmite_ID_Maquina();
                else
                    Transmite_Confirmacion('A', '0');
                break;

            case 12:
                #ifdef Debug_Mensajes_Server
                Serial.println("Solicitud de ROM Signature Maq");
                #endif
                if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                {
                    Encuesta_ROM();
                    delay(300);
                    Transmite_ROM_Signature();
                }
                    
                else
                    Transmite_Confirmacion('A', '0');
                break;

            case 13:
                #ifdef Debug_Mensajes_Server
                Serial.println("Solicitud Actualizar Contadores Mecanicos");
                #endif
                if (Actualiza_Tarjeta_Mecanica(res))
                    Transmite_Confirmacion('A', '7');
                else
                    Transmite_Confirmacion('A', '8');

                for (int i = 0; i < 256; i++)
                {
                    Serial.print(res[i]);
                }
                Serial.println();
                break;

            case 14:
                #ifdef Debug_Mensajes_Server
                Serial.println("Solicitud Reset ESP32");
                #endif
                Transmite_Confirmacion('A', 'F');
                delay(200);
                ESP.restart();
                break;

            case 15:
                #ifdef Debug_Mensajes_Server
                Serial.println("Solicitud de inactivar maquina");
                #endif
                if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                {
                    if (Inactiva_Maquina())
                        Transmite_Confirmacion('A', 'B');
                    else
                        Transmite_Confirmacion('A', 'C');
                }
                else
                    Transmite_Confirmacion('A', '0');
                break;

            case 18:
                 #ifdef Debug_Mensajes_Server
                Serial.println("Solicitud de activar maquina");
                #endif
                if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                {
                    if (Activa_Maquina())
                        Transmite_Confirmacion('A', '9');
                    else
                        Transmite_Confirmacion('A', 'A');
                }
                else
                    Transmite_Confirmacion('A', '0');
                break;

            case 181:
                #ifdef Debug_Mensajes_Server
                Serial.println("Confirmacion contadores recibido con exito");
                #endif
                //                Transmite_Confirmacion('A', '1');
                break;

            case 190:
                #ifdef Debug_Mensajes_Server
                Serial.println("Serializa trama de contadores");
                #endif
                if (!Variables_globales.Get_Variable_Global(Serializacion_Serie_Trama))
                {
                    if (contadores.Incrementa_Serie_Trama())
                    {
                        Variables_globales.Set_Variable_Global(Serializacion_Serie_Trama, true);
                        Transmite_Confirmacion('A', '1'); // Transmite ACK a Server
                    }
                }
                else
                    Transmite_Confirmacion('A', '1'); // Transmite ACK a Server
                break;
            case 273: 
                #ifdef Debug_Mensajes_Server
                Serial.println("Solicitud de información tarjeta");
                #endif
                Transmite_Info_Tarjeta();
                break;
                
            case 271:
                if(Valida_contrasena_Bootloader(res))
                {
                    #ifdef Debug_Mensajes_Server
                    Serial.println("Solicitud de Bootloader");
                    #endif

                    if(Variables_globales.Get_Variable_Global(Sincronizacion_RTC))
                    {
                        Inicializa_modo_bootloader();
                    }else{
                        Transmite_Confirmacion('A', '3');
                        
                    }
                }else
                {
                    Transmite_Confirmacion('C', 'I');
                    #ifdef Debug_Mensajes_Server
                    Serial.println("Contraseña incorrecta");
                    #endif
                }
                break;
            case 318:
                    #ifdef Debug_Mensajes_Server
                    Serial.println("Solicitud de información Cashless");
                    #endif
                    if(Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                    {
                        Consulta_Info_Cashless();
                        if(Variables_globales.Get_Variable_Global(Consulta_Info_Cashless_OK))
                        {
                            Transmite_Info_Cashless();    
                        }
                        
                    }else{
                        Transmite_Confirmacion('A', '0');
                    }
                    Variables_globales.Set_Variable_Global(Consulta_Info_Cashless_OK,false);
                    break;
            case 200:
                #ifdef Debug_Mensajes_Server
                Serial.println("Solicitud de configuracion maquina");
                #endif
               
                if (((res[4] - 48) < 10) && Configura_Tipo_Maquina(res))
                {
                    Transmite_Confirmacion('C', '9');
                    delay(150);
                    ESP.restart();
                }
                else
                    Transmite_Confirmacion('C', 'A');
                break;

            case 308:
                #ifdef Debug_Mensajes_Server
                Serial.println("Solicitud de contadores Cashless");
                #endif
                if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                    Transmite_Contadores_Cashless();
                else
                    Transmite_Confirmacion('A', '0');
                break;

            case 315:
                #ifdef Debug_Mensajes_Server
                Serial.println("Solicitud  Open Server FTP ");
                #endif
                if (Variables_globales.Get_Variable_Global(SD_INSERT) == true)
                {
                    Enable_Disable_modo_Ftp_server(true);
                }
                else
                {
                    Transmite_Confirmacion('P', 'T');
                }
                break;
            case 316:
                #ifdef Debug_Mensajes_Server
                Serial.println("Solicitud Close Server FTP  ");
                #endif
                if (!Enable_Disable_modo_Ftp_server(false))
                {
                    Transmite_Confirmacion('P', 'T');
                }
                delay(200);
                ESP.restart();
                break;
            case 317:
                #ifdef Debug_Mensajes_Server
                Serial.println("Solicitud Información de MicroSD");
                #endif
               // Transmite_Info_Memoria_SDCARD();
                break;

            case 502:
                if (res[4] - 48 < 3 && res[4] - 48 != 0)
                {
                    #ifdef Debug_Mensajes_Server
                    Serial.println("Solicitud de configuración Puerto COM RS232");
                    #endif
                    NVS.begin("Config_ESP32", false);
                    uint16_t Port_COM = res[4] - 48;
                    NVS.putUInt("COM", Port_COM);
                    NVS.end();
                    Transmite_Confirmacion('P', 'C');
                    delay(200);
                    ESP.restart();
                    
                }else
                {
                    Transmite_Confirmacion('P', 'N');
                    #ifdef Debug_Mensajes_Server
                    Serial.println("Puerto COM no existe!");
                    #endif
                }
                break;
           
            case 191:

               // #ifdef Debug_Mensajes_Server
                               // Serial.println("Premio registrado con exito");
               // #endif
              //  Serial.println("Premio registrado con exito");
                Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                Reset_Handle_LED();
               
                if (contadores.Close_ID_Operador())
                {
                    Condicion_Cumpl = false;
                    Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO, false);
                    Transmite_Confirmacion('A', '2');
                    if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                    {
                        if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                        {
                            Variables_globales.Set_Variable_Global(Flag_Maquina_En_Juego, false);
                            Transmite_Contadores_Accounting();
                        }
                    }
                    else
                    {
                        Transmite_Confirmacion('A', '0');
                    }
                }
                
               
                break;

            case 503:
                #ifdef Debug_Mensajes_Server
                Serial.println("Solicitud de Cierre Sesion");
                #endif
                Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                
                if(Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                {
                    if(Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
                    {
                        if (Close_Sesion_Player_Tracking())
                        {
                            
                            Transmite_Confirmacion('D', 'D');
                        }
                    }
                   
                }else{
                    Transmite_Confirmacion('A', '0');
                }
                break;

            case 320:
                #ifdef Debug_Mensajes_Server
                Serial.println("Solicitud Reset Premio Handpay");
                #endif
                contadores.Close_ID_Operador();
                condicionCumplida=false;
                
                if(Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                {
                    startTime = currentTime;
                    New_Timer_Final = New_Timmer_Inicial; /*RESET TIMEOUT*/
                    
                    /* Completa Con ceros  a la izquierda */
                    Tmp[0]=res[4];
                    Tmp[1]=res[5];
                    Tmp[2]=res[6];
                    Tmp[3]=res[7];
                    Tmp[4]=res[8];
                    Tmp[5]=res[9];
                    Tmp[6]=res[10];
                    Tmp[7] = res[11];

                    int i;
                    int longitudActual = strlen(Tmp);
                    if (longitudActual < 8)
                    {
                        int cerosFaltantes = 8 - longitudActual;

                        // Mover los caracteres existentes a la derecha
                        for (i = longitudActual - 1; i >= 0; i--)
                        {
                            Tmp[i + cerosFaltantes] = Tmp[i];
                        }

                        // Rellenar con ceros a la izquierda
                        for (i = 0; i < cerosFaltantes; i++)
                        {
                            Tmp[i] = '0';
                        }
                    }
                    /*----------------------------------------------*/

                    /*-------------->  Trama Completa 8  dig  <--------*/
                    res[4]=Tmp[0];
                    res[5]=Tmp[1];
                    res[6]=Tmp[2];
                    res[7]=Tmp[3];
                    res[8]=Tmp[4];
                    res[9]=Tmp[5];
                    res[10]=Tmp[6];
                    res[11]=Tmp[7];
                    /*-----------------------------------------------*/

                    /* ------------> Guarda ID operador <----------- */
                    contadores.Set_Operador_ID(res);
                    /*-----------------------------------------------*/


                    /* -------------> Debug Operador <---------------*/
                    #ifdef Debug_Mensajes_Server
                        Serial.print("Solicitud Operador: ");
                    
                        Serial.print(int(contadores.Get_Operador_ID()[0])-48);
                        Serial.print(int(contadores.Get_Operador_ID()[1])-48);
                        Serial.print(int(contadores.Get_Operador_ID()[2])-48);
                        Serial.print(int(contadores.Get_Operador_ID()[3])-48);
                        Serial.print(int(contadores.Get_Operador_ID()[4])-48);
                        Serial.print(int(contadores.Get_Operador_ID()[5])-48);
                        Serial.print(int(contadores.Get_Operador_ID()[6])-48);
                        Serial.println(int(contadores.Get_Operador_ID()[7])-48);
                    #endif
                    /*-----------------------------------------------*/
                    
                    /* RESET HANDPAY SAS o RELE */
                    if (Variables_globales.Get_Variable_Global(Conexion_RFID))
                        Status_Barra(TARJETA_OPERADOR_INSERT);
                    Reset_Handle_LED();

                    if (Variables_globales.Get_Variable_Global(Type_Hanpay_Reset))
                    {
                        if (!Variables_globales.Get_Variable_Global(Reset_Handpay_in_Process))
                        {
                            //contadores.Set_Operador_ID_Temp_APP(res);
                            Variables_globales.Set_Variable_Global(Manual_Detected, true);
                            Variables_globales.Set_Variable_Global(Manual_Reset, true);
                        }else{

                            if(Variables_globales.Get_Variable_Global(Conexion_RFID))
                                Status_Barra(ERROR_RESET_HANDPAY);
                            Variables_globales.Set_Variable_Global(Handle_RFID_Lector,false);
                            Transmite_Confirmacion('H', 'I');
                        }
                    }
                    else
                    {
                        /*----------------------------------------------------------------*/
                          /* Solicitud Reset Handpay */  
                          Reset_HandPay(); /* Reset Premio por SAS*/
                          delay(200);
                        /*----------------------------------------------------------------*/

                        switch (Variables_globales.Get_Variable_Global_Char(Reset_Handay_OK))
                        {
                        case 0x00: /*Reset realizado con exito*/
                            Transmite_Confirmacion('C', '0');
                            Variables_globales.Set_Variable_Global_Char(Reset_Handay_OK, 0x04);
                            Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 0);
                            Encuesta_Creditos_Premio();
                            delay(400);
                            Transmite_Contadores_Accounting();
                            Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                            if (Variables_globales.Get_Variable_Global(Conexion_RFID))
                                Status_Barra(Reset_Exitoso);
                            Reset_Handle_LED();
                            Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO,true);
                            break;
                        case 0x01: /*No existe condición de reset*/
                            /* Guarda ID Operador */
                            contadores.Close_ID_Operador();
                            Transmite_Confirmacion('C', '1');
                            Variables_globales.Set_Variable_Global_Char(Reset_Handay_OK, 0x04);
                            Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                            if (Variables_globales.Get_Variable_Global(Conexion_RFID))
                                Status_Barra(ERROR_RESET_HANDPAY);
                            Reset_Handle_LED();
                            break;
                        case 0x02: /*Imposible realizar reset*/
                            contadores.Close_ID_Operador();
                            Transmite_Confirmacion('C', '2');
                            Variables_globales.Set_Variable_Global_Char(Reset_Handay_OK, 0x04);

                            Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                            if (Variables_globales.Get_Variable_Global(Conexion_RFID))
                                Status_Barra(ERROR_RESET_HANDPAY);
                            Reset_Handle_LED();
                            break;
                        case 0x04: /* No  hay respuesta de la maquina*/
                            Transmite_Confirmacion('C', '4');
                            Variables_globales.Set_Variable_Global_Char(Reset_Handay_OK, 0x04);
                            Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                            if (Variables_globales.Get_Variable_Global(Conexion_RFID))
                                Status_Barra(ERROR_RESET_HANDPAY);
                            Reset_Handle_LED();
                            Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO, true);
                            break;
                        }
                    }
                }
                else
                    Transmite_Confirmacion('A', '0');
                break;
            case 500:
                //Finally_ = Start_;
                nvs_flash_erase();
                nvs_flash_init();
                while (Flash_OK==false)
                {
                    Start_ = millis();
                    if ((Start_ - Finally_) >= Inter_)
                    {
                         ESP.restart();
                         Flash_OK=true;
                         Finally_ = Start_;
                    }
                }
                Transmite_Confirmacion('F', 'B');
                delay(100);
                ESP.restart();
                break;

                case 504:

                    if ((res[4] - 48 )<=1)
                    {
                        if((res[4]-48)==0)
                        {
                            NVS.begin("Config_ESP32", false);
                            NVS.putBool("RHNSAS",false);
                            NVS.end();
                            Transmite_Confirmacion('R', 'D');
                            delay(200);
                            ESP.restart();
                        }else if((res[4]-48)==1)
                        {
                            NVS.begin("Config_ESP32", false);
                            NVS.putBool("RHNSAS",true);
                            NVS.end();
                            Transmite_Confirmacion('R', 'A');
                            delay(200);
                            ESP.restart();
                        }else{
                            Transmite_Confirmacion('C', 'R');
                        }
                    }else{
                        Transmite_Confirmacion('C', 'R');
                    }
                    break;
                case 505:

                    if((res[4]-48)==1) /* Jose*/
                    {
                        NVS.begin("Config_ESP32", false);
                        size_t ip_server_len1 = NVS.getBytesLength("Dir_IP_Serv");
                        char IP_Server1[ip_server_len1];
                        NVS.getBytes("Dir_IP_Serv", IP_Server1, ip_server_len1);
                        IP_Server1[3] = 204;
                        NVS.putBytes("Dir_IP_Serv", IP_Server1, sizeof(IP_Server1));
                        NVS.end();
                        delay(500);
                        ESP.restart();
                    }else if((res[4]-48)==4) /* Andres*/
                    {
                        NVS.begin("Config_ESP32", false);
                        size_t ip_server_len1 = NVS.getBytesLength("Dir_IP_Serv");
                        char IP_Server1[ip_server_len1];
                        NVS.getBytes("Dir_IP_Serv", IP_Server1, ip_server_len1);
                        IP_Server1[3] = 100;
                        NVS.putBytes("Dir_IP_Serv", IP_Server1, sizeof(IP_Server1));
                        NVS.end();
                        delay(500);
                        ESP.restart();
                    }else if((res[4]-48)==3) /* Adrian*/
                    {
                        NVS.begin("Config_ESP32", false);
                        size_t ip_server_len1 = NVS.getBytesLength("Dir_IP_Serv");
                        char IP_Server1[ip_server_len1];
                        NVS.getBytes("Dir_IP_Serv", IP_Server1, ip_server_len1);
                        IP_Server1[3] = 200;
                        NVS.putBytes("Dir_IP_Serv", IP_Server1, sizeof(IP_Server1));
                        NVS.end();
                        delay(500);
                        ESP.restart();
                    }else{
                        #ifdef Debug_Mensajes_Server
                        Serial.println("Dirección diferente a desarrollo");
                        #endif
                    }

                    break;
                    /*---------------------------------------->CASHLESS<-------------------------------------------*/
                
                 case 138:
                     Variables_globales.Set_Variable_Global(Conexion_To_Host, true);

                     // Variables_globales.Set_Variable_Global(AutoUPDATE_OK,true);
                     // Variables_globales.Set_Variable_Global(Solicitud_Carga_Bonus,true);
                     // contadores.Set_Meter_Legacy_Bonus_Awards(res);
                     // Carga_Bonus_Maquina();
                     // delay(600);

                     // if(Variables_globales.Get_Variable_Global(Flag_ACK_Carga_Bonus_Pendiente))
                     //     Transmite_Confirmacion_Cashless('A','A','A');
                     break;

                 case 603:
                    // #ifdef Debug_Mensajes_Server
                    // Serial.println("Solicitud de actualizacion de firmware!"); /* OK*/
                    // #endif

                     if (!Variables_globales.Get_Variable_Global(Updating_System))
                     {
                         switch (contadores.Init_Parameter_Update(res))
                         {

                         case 0:
                           //  #ifdef Debug_Mensajes_Server
                           //  Serial.println("URL Recibidas con exito!");
                           //  #endif
                             Variables_globales.Set_Variable_Global(Updating_System, true);
                             Variables_globales.Set_Variable_Global(AutoUPDATE_OK, false);
                             delay(1);
                             Variables_globales.Set_Variable_Global(AutoUPDATE_OK, true);
                             break;

                         case 1:
                             #ifdef Debug_Mensajes_Server
                             Serial.println("Falla deserializando Json de solititud ");
                             #endif
                             Variables_globales.Set_Variable_Global(Updating_System, false);
                             break;

                         case 2:
                           //  #ifdef Debug_Mensajes_Server
                           //  Serial.println("Token de acceso no generado");
                           //  #endif
                             Variables_globales.Set_Variable_Global(Updating_System, false);
                             break;

                         case 3:
                             #ifdef Debug_Mensajes_Server
                             Serial.println("Proceso no identificado ");
                             #endif
                             Variables_globales.Set_Variable_Global(Updating_System, false);
                             break;

                         case 4:
                             #ifdef Debug_Mensajes_Server
                             Serial.println("Proceso no identificado ");
                             #endif
                             Variables_globales.Set_Variable_Global(Updating_System, false);
                             break;

                         default:
                             #ifdef Debug_Mensajes_Server
                             Serial.println("Proceso no identificado ");
                             #endif
                             Variables_globales.Set_Variable_Global(Updating_System, false);
                             break;
                         }
                         break;
                     }
                     break;
                 case 604:
                    #ifdef Debug_Mensajes_Server
                    Serial.println("Configura Timer Transmision en juego "); /*OK*/
                    #endif
                    Config_Timer_Transmission_In_Game(res);
                break;

                case 605:
                    #ifdef Debug_Mensajes_Server
                    Serial.println("Solicitud info lector");
                    #endif 
                    Transmite_Info_Lector();
                break;

                case 606:
                    #ifdef Debug_Mensajes_Server
                    Serial.println("Configura Timer Transmision no juego "); /*OK*/
                    #endif
                    Config_Timer_Transmission_No_Game(res);
                break;

                case 607:
                    #ifdef Debug_Mensajes_Server
                    Serial.println("Configura Player Tracking "); /*OK */
                    #endif
                    Config_TimeOut_Player_Tracking(res);
                break;

                case 608:

                    #ifdef Debug_Mensajes_Server
                    Serial.println("Configura Player")
                    #endif

                    Config_Timer_Inactividad_Maquina(res);
                break;


                // case 603:
                //     if(res[4]=='0')
                //     {
                //         NVS.begin("Config_ESP32", false);
                //         NVS.putBool("LECTOR",false);
                //         NVS.end();
                //         Variables_globales.Set_Variable_Global(Consulta_Info_Lector_Rfid,false);
                //         if(!Variables_globales.Get_Variable_Global(Consulta_Info_Lector_Rfid))
                //             Status_Barra(MODULO_KO);
                //     }else if(res[4]=='1')
                //     {
                //         NVS.begin("Config_ESP32", false);
                //         NVS.putBool("LECTOR",true);
                //         NVS.end();
                //         Variables_globales.Set_Variable_Global(Consulta_Info_Lector_Rfid,true);
                //         if(Variables_globales.Get_Variable_Global(Consulta_Info_Lector_Rfid))
                //             Status_Barra(MODULO_OK);
                //     }
                //   break;

                //  case 601:
                //      Transmite_Info_Lector();
                //    break;

                // case 300:
                //     #ifdef Debug_Mensajes_Server
                //     Serial.println("Solicitud de registro AFT");
                //     #endif 
                //     int Status;
                //     Status=Registra_Machine();
                //     if(Status==1)
                //     {
                //         #ifdef Debug_Mensajes_Server
                //         Serial.println("MAQUINA REGISTRADA OK");
                //         #endif 
                //         Transmite_Info_Registro();
                //     }else if(Status==2)
                //     {
                //         Transmite_Confirmacion_Cashless('4','0','2');
                //     }else if(Status==3)
                //     {
                //         Transmite_Confirmacion_Cashless('4','0','0');
                //     }
                //     /* Aqui Validar */
                // break;

                //  case 301:
                //     #ifdef Debug_Mensajes_Server
                //     Serial.println("Confirmacion registro AFT Maq");
                //     #endif 

                // break;

                // case 302:
                //     #ifdef Debug_Mensajes_Server
                //     Serial.println("Cancelacion registro AFT");
                //     #endif 
                //     if(Delete_Registro_Machine())
                //     {
                //         #ifdef Debug_Mensajes_Server
                //         Serial.println("Maquina no registrada..");
                //         #endif 
                       
                //     }else{
                //         #ifdef Debug_Mensajes_Server
                //         Serial.println("Registro no cancelado..");
                //         #endif 
                //     }
                // break;

                // case 303:
                //     #ifdef Debug_Mensajes_Server
                //     Serial.println("Consulta informacion cliente AFT");
                //     #endif

                //     if (contadores.Verify_Client())
                //     {
                //         Almacena_Serial_Cashless_Extern(2);
                //         if (Compara_Serial_Cashless_Extern())
                //         {
                //          delay(100);
                //          Tratamiento_Trama_Cashless(res);
                //         }
                //     }else{
                //         /*Transmite transaccion abortada..*/
                //         Serial.println("Transaccion abortada");
                //     }

                // break;

                // case 305:
                //     Serial.println("Confirma carga de creditos AFT");
                // break;

                // case 307:
                //     Serial.println("Confirma descarga creditos AFT");

                //     /* Descarga cashelss */
                //     /*Transmite ACK AFT 4 0 6*/
                //     /**/
                //     contadores.Close_ID_Client_Temp();
                //     /*Habilita lectura de tarjeta*/

                // break;

                // case 309:
                //     Serial.println("Aborta carga cashless");
                //     if(!Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
                //     {
                //         if(contadores.Close_ID_Client_Temp()&&contadores.Close_ID_Client()&&contadores.Close_ID_Operador());
                //     }

                // break;

                // case 310:
                //     Serial.println("Tarjeta bloqueada AFT");
                //     if(!Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
                //         Player_Tracking_Sesion();
                // break;

                // case 321:
                //     Serial.println("Saldo insuficiente carga cashless");
                //     /* Inicia Player Tracking...*/
                //     if(!Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
                //         Player_Tracking_Sesion();
                // break;
                // case 311:
                //     Serial.println("Tarjeta desbloqueada AFT");
                //     Variables_globales.Set_Variable_Global(Handle_RFID_Lector,false);
                // break;

                // case 313:
                //     Serial.println("Cliente no existe en BD");
                //     if (!Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
                //     {
                //         contadores.Close_ID_Client_Temp();
                //         contadores.Close_ID_Client();
                //         contadores.Close_ID_Operador();
                //         contadores.Delete_Operator_ID_Temp();
                //         if(Variables_globales.Get_Variable_Global(Conexion_RFID))
                //             Status_Barra(CLIENTE_NO_BD);
                //     }
                // break;
                // case 350:
                //     Serial.println("Solicitud de enventos AFT");
                // break;
                // case 351:
                //     Serial.println("marca eventos AFT");
                // break;
                // case 139:
                //     Serial.println("Inhabilita carga AFT");
                //     /* Guarda en memoria  Estado de AFT= FALSE*/
                // break;
                // case 140:
                //     Serial.println("Habilita carga AFT");
                //     /* Guarda en memoria  Estado de AFT = TRUE*/
                // break;

            default:
                #ifdef Debug_Mensajes_Server
                Serial.println(Comando_Recibido());
                for (int i = 0; i < 256; i++)
                {
                    Serial.print(res[i]);
                }
                Serial.println();
                #endif
                break;
            }
            #ifdef Debug_Mensajes_Server
            Serial.println("--------------------------------------------------------------------------");
            #endif

        }
        else if (Variables_globales.Get_Variable_Global(Dato_Entrante_No_Valido))
        {
            Variables_globales.Set_Variable_Global(Dato_Entrante_No_Valido, false);

            char res[258] = {};
            bzero(res, 258); // Pone el buffer en 0
            memcpy(res, Buffer.Get_buffer_recepcion(), 258);
            /*
            for(int i=0; i<258; i++)
            {
                Serial.println(res[i]);
            }
            */
           /*
            if (res[0] == 'S' && res[1] == 'B')
            {
                Serial.println("Solicitud de Bootloader");
                Inicializa_modo_bootloader();
            }
             */
            
            if (res[0] == 'E' && res[1] == 'B')
            {
                #ifdef Debug_Mensajes_Server
                Serial.println("Eco Broadcast");
                #endif
                Transmite_Eco_Broadcast();
            }
            else if (res[0] == 'S' && res[1] == 'A')
            {
                #ifdef Debug_Mensajes_Server
                Serial.println("Guarda Configuracion");
                #endif
                for (int i = 0; i < 117; i++)
                {
                   #ifdef Debug_Mensajes_Server
                    Serial.print(res[i]);
                    #endif
                }
                Guarda_Configuracion_ESP32();
            }
            else if(res[0]=='V'&&res[1]=='C')
            {
                #ifdef Debug_Mensajes_Server
                                Serial.println();
                                Serial.print("ESTADO SERVIDOR: ");
                #endif

                #ifdef Debug_Mensajes_Server
                                Serial.println("Conectado!");
                #endif
                Variables_globales.Set_Variable_Global(Conexion_To_Host, true);
            }
            /*
            else if(res[0]=='F' && res[1]=='P')
            {
                Serial.println("Solicitud FTP Server INPUT");
                Enable_Disable_modo_Ftp_server(true);
            }
            else if(res[0]=='F' && res[1]=='O')
            {
                Serial.println("Solicitud FTP Server OUT");
                Enable_Disable_modo_Ftp_server(false);
            }
            else if(res[0]=='M' && res[1]=='A')
            {
                Serial.println("Solicitud Información de MicroSD");
            }
            else if(res[0]=='P' && res[1]=='C')
            {
                Serial.println("Solicitud Información de Procesador");
            }
            */
            else
            {

                #ifdef Debug_Mensajes_Server
                for (int i = 0; i < 256; i++)
                {
                    Serial.print(res[i]);
                }
                Serial.println();
                #endif
                Transmite_Confirmacion('C', 'R');
            }
        }
        else if (Variables_globales.Get_Variable_Global(Dato_Evento_Valido))
        {
            
            Almacena_Evento(eventos.Get_evento(),RTC);
            if(Variables_globales.Get_Variable_Global(Comunicacion_Maq))
            {
                Transmite_Eventos();
            }
            Variables_globales.Set_Variable_Global(Dato_Evento_Valido, false);

            char evento = eventos.Get_evento();
            #ifdef Debug_Mensajes_Server
            Serial.print("Evento enviado....... ");
            Serial.println(evento, HEX);
            #endif
            switch (evento)
            {
            case 0x51:
                Verifica_Premio_1B();
                break;

            default:
                break;
            }
        }
        if(Variables_globales.Get_Variable_Global(Flag_Memoria_SD_Full)==true)
        {
            Start_Timer=millis();
            if((Start_Timer-Finally_Timer)>=Inter_Timer)
            {
                /*Memoria SD Llena envia evento o ACK*/
                Transmite_Confirmacion('M', 'L');
                /* Reset timer*/
                Finally_Timer=Start_Timer;
            }
        }
        if(Variables_globales.Get_Variable_Global(Flag_Memoria_SD_Full)==false)
        {
            /* Reset timer*/
            Finally_Timer = Start_Timer;
        }

       // Mensajes_RFID();
        vTaskDelay(100 / portTICK_PERIOD_MS);
      //  continue;
    }
    vTaskDelay(10);
}

/*****************************************************************************************/
/******************************** TAREA DE TRANSMISION ***********************************/
/*****************************************************************************************/

void Task_Maneja_Transmision(void *parameter)
{
    for (;;)
    {
        if (Variables_globales.Get_Variable_Global(Flag_Maquina_En_Juego) && Contador_Maquina_En_Juego < Tiempo_Inactividad_Maquina) //50 //150  2minutos
            Contador_Maquina_En_Juego++;
        else if (Variables_globales.Get_Variable_Global(Flag_Maquina_En_Juego) && Contador_Maquina_En_Juego >= Tiempo_Inactividad_Maquina)
        {
            Variables_globales.Set_Variable_Global(Flag_Maquina_En_Juego, false);
            Contador_Maquina_En_Juego = 0;
        }
        
        Actualiza_Contadores();
        Verifica_Cambio_Contadores();
        Transmite_Configuracion();
        Transmision_Controlada_Contadores();
      //  Handle_ACK();
        Mensajes_RFID();
        vTaskDelay(800 / portTICK_PERIOD_MS);
        continue;
    }
    vTaskDelay(10);
}

void Verifica_Cambio_Contadores(void)
{
    if (flag_ultimo_contador_Ok && Variables_globales.Get_Variable_Global(Comunicacion_Maq))
    {
        // DETECTA CAMBIO COIN IN - MAQUINA EN JUEGO
        
        Contador_Coin_In_Act = contadores.Get_Contadores_Int(Coin_In);
        if (Contador_Coin_In_Ant == 0)
            Contador_Coin_In_Ant = Contador_Coin_In_Act;
        
        else if (Contador_Coin_In_Act > Contador_Coin_In_Ant)
        {
            Contador_Coin_In_Ant = Contador_Coin_In_Act;
            Variables_globales.Set_Variable_Global(Flag_Maquina_En_Juego, true);
            //            flag_maquina_en_juego = true;
            New_Timer_Final=New_Timmer_Inicial; /*RESET TIMEOUT*/
            startTime = currentTime;
            
        }
        
        // DETECTA CAMBIO CANCEL CREDIT - PREMIO PAGADO
        Contador_Cancel_Credit_Act = contadores.Get_Contadores_Int(Cancel_Credit_Hand_Pay);
        if (Contador_Cancel_Credit_Ant == 0)
            Contador_Cancel_Credit_Ant = Contador_Cancel_Credit_Act;
        else if (Contador_Cancel_Credit_Act > Contador_Cancel_Credit_Ant)
        {
            Contador_Cancel_Credit_Ant = Contador_Cancel_Credit_Act;
            flag_premio_pagado_cashout = true;
            
        }

        // DETECTA CAMBIO BILLETERO - BILLETE INSERTADO
        Contador_Bill_In_Act = contadores.Get_Contadores_Int(Bill_Amount);
        if (Contador_Bill_In_Ant == 0)
            Contador_Bill_In_Ant = Contador_Bill_In_Act;
        else if (Contador_Bill_In_Act > Contador_Bill_In_Ant)
        {
            Contador_Bill_In_Ant = Contador_Bill_In_Act;
            flag_billete_insertado = true;
        }
    }
}

bool Cumple_Condicion=false;
void Transmite_Configuracion(void)
{
    switch (Contador_Transmision)
    {
    case 10:
        if (!Variables_globales.Get_Variable_Global(Sincronizacion_RTC))
        {
            Transmite_Confirmacion('A', '3');
        }
            
        // if(flag_ultimo_contador_Ok &&!Variables_globales.Get_Variable_Global(Primer_Cancel_Credit)&&Variables_globales.Get_Variable_Global(Comunicacion_Maq)==true&& Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6)
        // {
        //     if (Calcula_First_Cancel_Credit(true))
        //         Variables_globales.Set_Variable_Global(Primer_Cancel_Credit, true);
        // }
        break;

    case 20:
        if (!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
            Transmite_Confirmacion('A', '0');

         if(flag_ultimo_contador_Ok &&!Variables_globales.Get_Variable_Global(Primer_Cancel_Credit)&&Variables_globales.Get_Variable_Global(Comunicacion_Maq)==true&& Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6)
         {
             if (Calcula_First_Cancel_Credit(true))
                 Variables_globales.Set_Variable_Global(Primer_Cancel_Credit, true);
         }
        break;

    case 30:
        if (!Variables_globales.Get_Variable_Global(Serializacion_Serie_Trama))
            Transmite_Confirmacion('B', 'C');
        // if(flag_ultimo_contador_Ok &&!Variables_globales.Get_Variable_Global(Primer_Cancel_Credit)&&Variables_globales.Get_Variable_Global(Comunicacion_Maq)==true&& Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6)
        // {
        //     if (Calcula_First_Cancel_Credit(true))
        //         Variables_globales.Set_Variable_Global(Primer_Cancel_Credit, true);
        // }
        break;

    case 40:
        if (!Variables_globales.Get_Variable_Global(Primer_Cancel_Credit) && Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6&&Variables_globales.Get_Variable_Global(Comunicacion_Maq)==true && flag_ultimo_contador_Ok)
        {
            if (Calcula_First_Cancel_Credit(true))
                Variables_globales.Set_Variable_Global(Primer_Cancel_Credit, true);
        }
        Contador_Transmision = 0;
        break;
    }
    Contador_Transmision++;
}

int Ejecjuta=0;
int Contador=0;
void Transmision_Controlada_Contadores(void)
{
    Contador_Transmision_Contadores++;
    Contador++;
    if (Variables_globales.Get_Variable_Global(Comunicacion_Maq)) // Si hay comunicacion con la maquina...
    {
        
        // Si la maquina NO esta en juego, transmite cada 2 minutos, si el valor es 120
        if (!Variables_globales.Get_Variable_Global(Flag_Maquina_En_Juego) && !flag_premio_pagado_cashout && !flag_billete_insertado && !Variables_globales.Get_Variable_Global(Operador_Detected))
        {
            New_Timmer_Inicial=millis();
            /*----------------------> Trama de contadores  2 minutos <-----------------------------------------------------------*/
            if ((New_Timmer_Inicial - New_Timer_Final) >= Tiempo_Transmision_No_Juego)
            {
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6 && Variables_globales.Get_Variable_Global(Flag_Hopper_Enable) != true && Variables_globales.Get_Variable_Global(Primer_Cancel_Credit) == true)
                {
                    #ifdef Debug_Transmision
                    Serial.println("Contadores, maquina NO juego....");
                    #endif
                    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9)
                    {
                            Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 0);

                          //  Encuesta_Creditos_Premio(); /*Encuesta creditos antes de envio de contadores*/
                          //  delay(200);
                              Actualiza_Maquina_En_Juego(); /*Actualiza Billetero,Creditos,Coin in,Coin out*/
                              delay(250);
                    }
                        Transmite_Contadores_Accounting();
                }
                else if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 6)
                {
                    #ifdef Debug_Transmision
                    Serial.println("Contadores, maquina NO juego....");
                    #endif
                    Contador_Transmision_Contadores = 0;
                    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9)
                    {
                        Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 0);
                       // Encuesta_Creditos_Premio(); /*Encuesta creditos antes de envio de contadores*/
                       // delay(200);
                          Actualiza_Maquina_En_Juego(); /*Actualiza Billetero,Creditos,Coin in,Coin out*/
                          delay(250);
                    }
                    
                        Transmite_Contadores_Accounting();
                }
                 Contador_Transmision_Contadores = 0;
                 New_Timer_Final=New_Timmer_Inicial;/*RESET TIMEOUT*/
            }
        }
        // Si la maquina SI esta en juego, transmite cada 30 segundos, si el valor es 30
        else if (Variables_globales.Get_Variable_Global(Flag_Maquina_En_Juego) && !flag_premio_pagado_cashout && !flag_billete_insertado)
        {
            if (Contador_Transmision_Contadores >= Tiempo_Transmision_En_Juego)
            {

                if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) !=6)
                {
                    #ifdef Debug_Transmision
                    Serial.println("Contadores, maquina SI juego....");
                    #endif
                    //Encuesta_Creditos_Premio(); /*Encuesta creditos antes de envio de contadores*/
                    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9)
                    {
                        Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 0);
                        Actualiza_Maquina_En_Juego(); /*Actualiza Billetero,Creditos,Coin in,Coin out*/
                        delay(250);
                    }
                    
                    //                Variables_globales.Set_Variable_Global(Flag_Maquina_En_Juego, false);
                    //                flag_maquina_en_juego = false;
                    Transmite_Contadores_Accounting();
                    Contador_Transmision_Contadores = 0;
                }
                else if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) ==6 && Variables_globales.Get_Variable_Global(Flag_Hopper_Enable)!=true && Variables_globales.Get_Variable_Global(Primer_Cancel_Credit)==true)
                {
                    #ifdef Debug_Transmision
                    Serial.println("Contadores, maquina SI juego....");
                    #endif

                    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9)
                    {
                          Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 0);
                          Actualiza_Maquina_En_Juego(); /*Actualiza Billetero,Creditos,Coin in,Coin out*/
                          delay(250);
                    }
                    Transmite_Contadores_Accounting();
                    //                Variables_globales.Set_Variable_Global(Flag_Maquina_En_Juego, false);
                    //                flag_maquina_en_juego = false;
                    Contador_Transmision_Contadores = 0;
                }else{
                     Contador_Transmision_Contadores = 0;
                }
            }
                
        }

        // Si cambio el cancel credit, porque se pago un premio
        else if (flag_premio_pagado_cashout)
        {

            

            #ifdef Debug_Transmision
            Serial.println("Contadores, premio pagado....");
            #endif
            if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 6)
            {
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9)
                {
                    Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 0);

                    Encuesta_Creditos_Premio(); /*Encuesta creditos antes de envio de contadores*/
                    delay(350);
                }
                    Variables_globales.Set_Variable_Global(Flag_Maquina_En_Juego, false);  
                    Transmite_Contadores_Accounting();
            }

            flag_premio_pagado_cashout = false;
            
        }

        // Si cambio el billetero, porque se ingreso un nuevo billete
        else if (flag_billete_insertado)
        {
            if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6)
            {
                #ifdef Debug_Transmision
                Serial.println("Contadores, billete insertado....");
                #endif
                Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 0);
                Actualiza_Maquina_En_Juego(); /*Actualiza Billetero,Creditos,Coin in,Coin out*/
                delay(350);

                if (Variables_globales.Get_Variable_Global(Flag_Creditos_D_P))
                {
                    Variables_globales.Set_Variable_Global(Flag_Creditos_D_P, false);
                    if (Variables_globales.Get_Variable_Global(Primer_Cancel_Credit) == false && flag_ultimo_contador_Ok && Variables_globales.Get_Variable_Global(Comunicacion_Maq) == true)
                    {
                         /* Un no existe cancel credit y se ingreso un billete*/
                        if (Calcula_Cancel_Credit(true))
                        {
                            Variables_globales.Set_Variable_Global(Primer_Cancel_Credit, true);
                        }
                    }
                    Transmite_Contadores_Accounting();
                }
            }
            else if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 6)
            {
                #ifdef Debug_Transmision
                Serial.println("Contadores, billete insertado....");
                #endif
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9)
                {
                    Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 0);

                    Actualiza_Maquina_En_Juego(); /*Actualiza Billetero,Creditos,Coin in,Coin out*/
                    delay(350);
                }
                    Transmite_Contadores_Accounting();
            }
            
            flag_billete_insertado = false;
        }
       // Serial.println(contadores.Verify_ID_Op());
    }
    else
    {
        if (Contador_Transmision_Contadores >= 100)
        {
            Contador_Transmision_Contadores = 0;
            Transmite_Confirmacion('A', '0');
        }
    }
}



bool Calcula_Cancel_Credit(bool Calcula_Contador)
{
    int Cancel_Credit_Poker, Coin_In_Poker, Coin_Out_Poker, Drop_Poker, Creditos_Poker, Residuo;
    int uni, dec, cen, unimil, decmil, centmil, unimill, decmill;
    char Contador_Cancel_Credit_Poker[9];
    bzero(Contador_Cancel_Credit_Poker, 9);
    int Cancel_Credit_Poker_2;
    char Contador_NULL[9]{'F','F','F','F','F','F','F','F'};

    if (Calcula_Contador)
    {
        /*----------------------->  Valores Finales <-------------------------------- */
        Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 0);
        Encuesta_Creditos_Premio();
        delay(250);
        /*----------------------------------------------------------------------------*/

       
        Coin_In_Poker=Convert_Char_To_Int11(Coin_In_Poker_Data);
        Coin_Out_Poker=Convert_Char_To_Int11(Coin_Out_Poker_Data);
        Drop_Poker=Convert_Char_To_Int11(Total_Drop_Poker_Data);
        Creditos_Poker=Convert_Char_To_Int11(CurrentCredit_Poker_Data);

        // Error en formato de datos  Si contiene letras el contador 
        if(Coin_In_Poker==1||Coin_Out_Poker==1||Drop_Poker==1)
        {
            contadores.Set_Contadores(Total_Cancel_Credit, Contador_NULL);
            contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_NULL);
            return false;
        }

        
    //     Coin_In_Poker =Convert_Char_To_Int2(contadores.Get_Contadores_Char(Coin_In));
    //     //Coin_In_Poker = contadores.Get_Contadores_Int(Coin_In);
    //    // Coin_Out_Poker = contadores.Get_Contadores_Int(Coin_Out);
    //     Coin_Out_Poker =Convert_Char_To_Int2(contadores.Get_Contadores_Char(Coin_Out));
    //    // Drop_Poker = contadores.Get_Contadores_Int(Total_Drop);
    //     Drop_Poker=Convert_Char_To_Int2(contadores.Get_Contadores_Char(Total_Drop));
    //    // Creditos_Poker = contadores.Get_Contadores_Int(Current_Credits);
    //     Creditos_Poker=Convert_Char_To_Int2(contadores.Get_Contadores_Char(Current_Credits));
    //     /*Verifica si la maquina fue borrada*/

        if(Drop_Poker<=0) //Caso 1 Billetero en 0
        {
            if(Coin_In_Poker>0||Coin_Out_Poker>0) /*Si no ingresa billetes*/
            {
                Serial.println("Error en contadores"); //Caso 2 Billetero en 0 y contadores entrada y salida bien
                return false;
            } 
            else if(Coin_In_Poker==0&&Coin_Out_Poker==0)
            {
                Serial.println("Maquina Borrada o error?");
                return false;   //Caso 3 Maquina Borrada.
            }
        }
        else if(Coin_In_Poker==0 && Coin_Out_Poker==0)
        {
            Serial.println("ERROR");
            return false;
        }
        else if(Coin_In_Poker==0 || Coin_Out_Poker==0)
        {
            Serial.println("ERROR");
            return false;
        }
        
        else if((Drop_Poker - Coin_In_Poker)==Drop_Poker && Coin_Out_Poker>0)
        {
            Serial.println("ERROR");
            return false;
        }
        else if(Coin_Out_Poker<=0 && Drop_Poker>0 && Coin_In_Poker>0)
        {
            Serial.println("ERROR");
            return false;
        }
       
        /* Intenta calcular Premio*/
        Cancel_Credit_Poker = ((Drop_Poker - Coin_In_Poker) + Coin_Out_Poker) - Creditos_Poker;

        if(Cancel_Credit_Poker==Coin_In_Poker||Cancel_Credit_Poker==Coin_Out_Poker||Cancel_Credit_Poker<=0 ||Cancel_Credit_Poker==Drop_Poker)
        {
            Serial.println("ERROR");
            return false;
        }   
    }
    Cancel_Credit_Poker_2=Cancel_Credit_Poker; /*Iguala variables para validar despues de conversion*/
  //  Serial.print("contador cancel credit int poker es: ");
  //  Serial.println(Cancel_Credit_Poker);

    decmill = Cancel_Credit_Poker / 10000000;
    Contador_Cancel_Credit_Poker[0] = decmill + 48;
    Residuo = Cancel_Credit_Poker % 10000000;

    unimill = Residuo / 1000000;
    Contador_Cancel_Credit_Poker[1] = unimill + 48;
    Residuo = Cancel_Credit_Poker % 1000000;

    centmil = Residuo / 100000;
    Contador_Cancel_Credit_Poker[2] = centmil + 48;
    Residuo = Cancel_Credit_Poker % 100000;

    decmil = Residuo / 10000;
    Contador_Cancel_Credit_Poker[3] = decmil + 48;
    Residuo = Cancel_Credit_Poker % 10000;

    unimil = Residuo / 1000;
    Contador_Cancel_Credit_Poker[4] = unimil + 48;
    Residuo = Cancel_Credit_Poker % 1000;

    cen = Residuo / 100;
    Contador_Cancel_Credit_Poker[5] = cen + 48;
    Residuo = Cancel_Credit_Poker % 100;

    dec = Residuo / 10;
    Contador_Cancel_Credit_Poker[6] = dec + 48;
    Residuo = Cancel_Credit_Poker % 10;

    uni = Residuo;
    Contador_Cancel_Credit_Poker[7] = uni + 48;

  //  Serial.print("contador cancel credit char poker es: ");
  //  Serial.println(Contador_Cancel_Credit_Poker);
    /*Si la conversion Fallo Copia y original diferentes*/
    if(Convert_Char_To_Int2(Contador_Cancel_Credit_Poker)!=Cancel_Credit_Poker_2)
    {
      //  Serial.println("La conversion fallo");
      //  Serial.println(Contador_Cancel_Credit_Poker);
      //  Serial.println(Cancel_Credit_Poker_2);

        Selector_Modo_SD();
        log_e("Fallo En la conversion Cancel credit Poker", Fallo_Conversion_Cancel_Poker);
        LOG_ESP(Archivo_LOG, Variables_globales.Get_Variable_Global(Enable_Storage));
        return false; /*Intenta nuevamente el calculo del premio*/
    }else{ /* Si Son iguales (No existe diferencia)*/

        /* Verifica el guardado */
        if (contadores.Set_Contadores(Total_Cancel_Credit, Contador_Cancel_Credit_Poker) && contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_Cancel_Credit_Poker))
        {
            return true; /*Contadores OK*/
        }

        if (contadores.Set_Contadores(Total_Cancel_Credit, Contador_Cancel_Credit_Poker) == false || contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_Cancel_Credit_Poker) == false)
        {
            return false; /*Uno o los dos contadores fallo*/
        }
    }  
}

bool Calcula_First_Cancel_Credit(bool Calcula_Contador)
{
    int Cancel_Credit_Poker, Coin_In_Poker, Coin_Out_Poker, Drop_Poker, Creditos_Poker, Residuo;
    int uni, dec, cen, unimil, decmil, centmil, unimill, decmill;
    char Contador_Cancel_Credit_Poker[9];
    bzero(Contador_Cancel_Credit_Poker, 9);
    int Cancel_Credit_Poker_2;
    char Contador_NULL[9]{'F','F','F','F','F','F','F','F'}; /*Array Error de data*/

    if (Calcula_Contador)
    {
        Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 0);

        /* ----------------> Actualiza  Contadores de maquina <---------------*/
        Encuesta_Creditos_Premio();
        delay(400);
        Creditos_Machine(); /* Encuesta creditos*/
        delay(100);
        /*--------------------------------------------------------------------*/
       
       /* ------------------> Get contadores <--------------------------------*/
        Coin_In_Poker=Convert_Char_To_Int11(Coin_In_Poker_Data);
        Coin_Out_Poker=Convert_Char_To_Int11(Coin_Out_Poker_Data);
        Drop_Poker=Convert_Char_To_Int11(Total_Drop_Poker_Data);
        Creditos_Poker=Convert_Char_To_Int11(CurrentCredit_Poker_Data);
        /*---------------------------------------------------------------------*/

        /*----------------> Error en formato de uno de los contadores<-------- */
        if(Coin_In_Poker==1||Coin_Out_Poker==1||Drop_Poker==1)
        {
            contadores.Set_Contadores(Total_Cancel_Credit, Contador_NULL);
            contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_NULL);
            Variables_globales.Set_Variable_Global(Primer_Cancel_Credit,true);
            return false;
        }
        /*--------------------------------------------------------------------*/

    //     Coin_In_Poker =Convert_Char_To_Int2(contadores.Get_Contadores_Char(Coin_In));
    //     //Coin_In_Poker = contadores.Get_Contadores_Int(Coin_In);
    //    // Coin_Out_Poker = contadores.Get_Contadores_Int(Coin_Out);
    //     Coin_Out_Poker =Convert_Char_To_Int2(contadores.Get_Contadores_Char(Coin_Out));
    //    // Drop_Poker = contadores.Get_Contadores_Int(Total_Drop);
    //     Drop_Poker=Convert_Char_To_Int2(contadores.Get_Contadores_Char(Total_Drop));
    //    // Creditos_Poker = contadores.Get_Contadores_Int(Current_Credits);
    //     Creditos_Poker=Convert_Char_To_Int2(contadores.Get_Contadores_Char(Current_Credits));
    //     /*Verifica si la maquina fue borrada*/
    //     /* Intenta calcular Premio*/

    /*-----------------------------------> Calcula Cancel Credit <-------------------------------------------*/
        Cancel_Credit_Poker = ((Drop_Poker - Coin_In_Poker) + Coin_Out_Poker) - Creditos_Poker;
       // Cancel_Credit_Poker=Drop_Poker;
    /*------------------------------------------------------------------------------------------------------*/
        if(Cancel_Credit_Poker<=0)
        {
            contadores.Set_Contadores(Total_Cancel_Credit, Contador_NULL);
            contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_NULL);
            Variables_globales.Set_Variable_Global(Primer_Cancel_Credit,true);
            return false;
        }
        if(Cancel_Credit_Poker==Drop_Poker||Cancel_Credit_Poker==Coin_In_Poker||Cancel_Credit_Poker==Coin_Out_Poker)
        {
            contadores.Set_Contadores(Total_Cancel_Credit, Contador_NULL);
            contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_NULL);
            Variables_globales.Set_Variable_Global(Primer_Cancel_Credit,true);
            return false;
        }
        if(Drop_Poker<=0) //Caso 1 Billetero en 0
        {
            if(Coin_In_Poker>0||Coin_Out_Poker>0) /*Si no ingresa billetes*/
            {
                contadores.Set_Contadores(Total_Cancel_Credit, Contador_NULL);
                contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_NULL);
                Variables_globales.Set_Variable_Global(Primer_Cancel_Credit,true);
                return false;
            } 
            else if(Coin_In_Poker==0&&Coin_Out_Poker==0)
            {
                contadores.Set_Contadores(Total_Cancel_Credit, Contador_NULL);
                contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_NULL);
                Variables_globales.Set_Variable_Global(Primer_Cancel_Credit,true);
                return false;   //Caso 3 Maquina Borrada.
            }
        }
        /* Error Controlado */
        if(Coin_In_Poker==0 && Coin_Out_Poker==0)
        {
            contadores.Set_Contadores(Total_Cancel_Credit, Contador_NULL);
            contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_NULL);
            Variables_globales.Set_Variable_Global(Primer_Cancel_Credit,true);
            return false;
        }
       
        if((Drop_Poker - Coin_In_Poker)==Drop_Poker && Coin_Out_Poker>0)
        {
            contadores.Set_Contadores(Total_Cancel_Credit, Contador_NULL);
            contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_NULL);
            Variables_globales.Set_Variable_Global(Primer_Cancel_Credit,true);
            return false;
        }
        if(Coin_Out_Poker<=0 && Drop_Poker>0 && Coin_In_Poker>0)
        { 
            contadores.Set_Contadores(Total_Cancel_Credit, Contador_NULL);
            contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_NULL);
            Variables_globales.Set_Variable_Global(Primer_Cancel_Credit,true);
            return false;
        }
    }
    Cancel_Credit_Poker_2=Cancel_Credit_Poker; /*Iguala variables para validar despues de conversion*/
   // Serial.print("contador cancel credit int poker es: ");
   // Serial.println(Cancel_Credit_Poker);

    decmill = Cancel_Credit_Poker / 10000000;
    Contador_Cancel_Credit_Poker[0] = decmill + 48;
    Residuo = Cancel_Credit_Poker % 10000000;

    unimill = Residuo / 1000000;
    Contador_Cancel_Credit_Poker[1] = unimill + 48;
    Residuo = Cancel_Credit_Poker % 1000000;

    centmil = Residuo / 100000;
    Contador_Cancel_Credit_Poker[2] = centmil + 48;
    Residuo = Cancel_Credit_Poker % 100000;

    decmil = Residuo / 10000;
    Contador_Cancel_Credit_Poker[3] = decmil + 48;
    Residuo = Cancel_Credit_Poker % 10000;

    unimil = Residuo / 1000;
    Contador_Cancel_Credit_Poker[4] = unimil + 48;
    Residuo = Cancel_Credit_Poker % 1000;

    cen = Residuo / 100;
    Contador_Cancel_Credit_Poker[5] = cen + 48;
    Residuo = Cancel_Credit_Poker % 100;

    dec = Residuo / 10;
    Contador_Cancel_Credit_Poker[6] = dec + 48;
    Residuo = Cancel_Credit_Poker % 10;

    uni = Residuo;
    Contador_Cancel_Credit_Poker[7] = uni + 48;

  //  Serial.print("contador cancel credit char poker es: ");
  //  Serial.println(Contador_Cancel_Credit_Poker);
    /*Si la conversion Fallo Copia y original diferentes*/
    if(Convert_Char_To_Int2(Contador_Cancel_Credit_Poker)!=Cancel_Credit_Poker_2)
    {
        Serial.println("La conversion fallo");
        Serial.println(Contador_Cancel_Credit_Poker);
        Serial.println(Cancel_Credit_Poker_2);
        contadores.Set_Contadores(Total_Cancel_Credit, Contador_NULL);
        contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_NULL);
        Variables_globales.Set_Variable_Global(Primer_Cancel_Credit,true);
        return false; /*Intenta nuevamente el calculo del premio*/
    }else{ /* Si Son iguales (No existe diferencia)*/

        /* Verifica el guardado */
        if (contadores.Set_Contadores(Total_Cancel_Credit, Contador_Cancel_Credit_Poker) && contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_Cancel_Credit_Poker))
        {
            return true; /*Contadores OK*/
        }

        if (contadores.Set_Contadores(Total_Cancel_Credit, Contador_Cancel_Credit_Poker) == false || contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_Cancel_Credit_Poker) == false)
        {
            contadores.Set_Contadores(Total_Cancel_Credit, Contador_NULL);
            contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_NULL);
            Variables_globales.Set_Variable_Global(Primer_Cancel_Credit,true);
            return false; /*Uno o los dos contadores fallo*/
        }
    }  
}



/*Desbloquea Maquina mediante comando
320 o via RFID y transmite a Servidor el estado del proceso
(C1) No hay concidición de reset
(C2) Reset handpay realizado con Exito
(C4) No hay respuesta de la maquina */
void RESET_HANDPAY_NOT_SAS(void)
{
    if (Variables_globales.Get_Variable_Global(Manual_Reset))
    {
        if (Variables_globales.Get_Variable_Global(Manual_Detected))
        {
            Variables_globales.Set_Variable_Global(Reset_Handpay_in_Process, true);

            if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 13)
            {
                Activa_Encuesta = true;
                delay(10);
                digitalWrite(Unlock_Machine, LOW);
                delay(250);
                digitalWrite(Unlock_Machine, HIGH);
                Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
            }
            else
            {
                delay(10);
                Creditos_Machine(); /* Encuesta creditos*/
                delay(100);
                int Creditos_Actuales = Convert_Char_To_Int10(contadores.Get_Contadores_Char(24));

                if (Creditos_Actuales <= 0)
                { /* No hay condición de reset */
                    contadores.Close_ID_Operador();
                    Activa_Encuesta = false;
                    Variables_globales.Set_Variable_Global(Manual_Reset, false);
                    Variables_globales.Set_Variable_Global(Manual_Detected, false);
                    Transmite_Confirmacion('C', '1');
                    delay(10);
                    if (Variables_globales.Get_Variable_Global(Conexion_RFID))
                        Status_Barra(ERROR_RESET_HANDPAY);
                    Reset_Handle_LED();
                    Variables_globales.Set_Variable_Global(Reset_Handpay_in_Process, false);
                    Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                    
                    Enable_Failed = false;
                    Enable_Timer_No_reset = false;
                }
                else if (Creditos_Actuales > 0)
                {
                    Activa_Encuesta = true;
                    delay(10);
                    digitalWrite(Unlock_Machine, LOW);
                    delay(250);
                    digitalWrite(Unlock_Machine, HIGH);
                    Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                }
            }

            Variables_globales.Set_Variable_Global(Manual_Detected, false);
        }
        delay(50);
        if (Activa_Encuesta) /* Loop()*/
        {
          //  Timer_Reset_In=millis();

            digitalWrite(Unlock_Machine, LOW);
            delay(10);
            Creditos_Machine(); /* Encuesta creditos*/
            delay(100);
            digitalWrite(Unlock_Machine, HIGH);
            int Creditos_Actuales_ = Convert_Char_To_Int10(contadores.Get_Contadores_Char(24));
            delay(3);
            
            if (Creditos_Actuales_ == 0) /* Reset_Realizado con Exito*/
            {
                
                if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 6)
                {
                    Transmite_Confirmacion('C', '0');
                    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9)
                    {
                        Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 0);
                        delay(1);
                        Encuesta_Creditos_Premio();
                        delay(350);
                    }
                    Transmite_Contadores_Accounting();
                    if (Variables_globales.Get_Variable_Global(Conexion_RFID))
                        Status_Barra(Reset_Exitoso);
                    Reset_Handle_LED();
                    delay(50);
                    Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                    
                }
                Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO,true); /* Inicia Timer Operador */
                Variables_globales.Set_Variable_Global(Manual_Reset, false);
                Variables_globales.Set_Variable_Global(Manual_Detected, false);
                Valida_Creditos_Actuales=0;
                Variables_globales.Set_Variable_Global(Reset_Handpay_in_Process,false);
                Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                Activa_Encuesta = false;
                Enable_Failed=false;
                Enable_Timer_No_reset=false; 
            }
            if(!Variables_globales.Get_Variable_Global(Comunicacion_Maq)&&Creditos_Actuales_>0)
            {


                Timer_Out_Failed=millis();

                if(!Enable_Failed)
                {


                    Timer_Out_Failed_Final=Timer_Out_Failed;
                    Enable_Failed=true;
                }

                if((Timer_Out_Failed-Timer_Out_Failed_Final)>=Timer_Failed_OK)
                {
                    
                    Transmite_Confirmacion('C', '4');

                    /* Si  fue un operador  */
                    
                        if (Variables_globales.Get_Variable_Global(Conexion_RFID))
                            Status_Barra(ERROR_RESET_HANDPAY);
                        Reset_Handle_LED();
                    Enable_Timer_No_reset=false; 
                    contadores.Close_ID_Operador();
                    Variables_globales.Set_Variable_Global(Manual_Reset, false);
                    Variables_globales.Set_Variable_Global(Manual_Detected, false);
                    Variables_globales.Set_Variable_Global(Handle_RFID_Lector,false);
                    Activa_Encuesta = false;
                    Valida_Creditos_Actuales=0;
                    Variables_globales.Set_Variable_Global(Reset_Handpay_in_Process,false);    
                    Enable_Failed=false;
                    Timer_Out_Failed_Final=Timer_Out_Failed;
                    
                }
            }

            // if(Variables_globales.Get_Variable_Global(Comunicacion_Maq)||Creditos_Actuales_<=0)
            // {
            //     Enable_Failed=false;
            // }

            // if(!Enable_Timer_No_reset)
            // {
            //     Timer_Reset_Out=Timer_Reset_In;
            //     Enable_Timer_No_reset=true;
            // }
            // if((Timer_Reset_In-Timer_Reset_Out)>=Timer_No_Reset && Creditos_Actuales_>0)
            // {

            //     Transmite_Confirmacion('C', '4');
            //     Status_Barra(ERROR_RESET_HANDPAY);


            //     contadores.Close_ID_Operador();
            //     Variables_globales.Set_Variable_Global(Manual_Reset, false);
            //     Variables_globales.Set_Variable_Global(Manual_Detected, false);
            //     Variables_globales.Set_Variable_Global(Handle_RFID_Lector,false);
            //     Activa_Encuesta = false;
            //     Valida_Creditos_Actuales=0;
            //     Variables_globales.Set_Variable_Global(Reset_Handpay_in_Process,false);    
            //     Enable_Failed=false;
            //     Enable_Timer_No_reset=false; 
            //     Timer_Reset_Out=Timer_Reset_In;
            // }
        }
    }
}

/* Verifica senal de  Hopper en Maquinas Poker para calcular el premio 
Transmite a servidor (D1) */     
void Task_Verifica_Hopper(void *parameter)
{
    int Conta_Poll_Cancel_Poker = 0;
    int contadorActiv=0;
    
    unsigned long Respuesta_SAS = 0;
    
    
    for (;;)
    {
        Verifica_Cambio_Contadores();
        contadorActiv++;
        //Serial.println("Verificando Hopper");

        
        if (digitalRead(Hopper_Enable) == HIGH){
            Serial.println("Hopper HIGH");
            Variables_globales.Set_Variable_Global(Flag_Hopper_Enable, true);
            Condicion_Cumpl=false; /* Reset Timer ID Operador  por tiempo de descarga maquina Poker*/
        }
                
        else if (digitalRead(Hopper_Enable) == LOW && Variables_globales.Get_Variable_Global(Flag_Hopper_Enable))
        {
           Serial.println("Hopper LOW");
            Conta_Poll_Cancel_Poker++;
            if (Conta_Poll_Cancel_Poker > 10 && Variables_globales.Get_Variable_Global(Calc_Cancel_Credit) && Convert_Char_To_Int11(CurrentCredit_Poker_Data)<=0)
            {
                
                Variables_globales.Set_Variable_Global(Flag_Hopper_Enable, false);
                Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 0);
                Encuesta_Creditos_Premio();
                delay(400);
                if (Calcula_Cancel_Credit(true)==true)
                {   
                    delay(1);
                    New_Timer_Final=New_Timmer_Inicial;/*RESET TIMEOUT*/
                    startTime = currentTime; // Posible reset handpay
                    Variables_globales.Set_Variable_Global(Calc_Cancel_Credit, false);
                    Serial.println("Premio Pagado Poker *************************************");
                    Transmite_Confirmacion('C', '0');
                    
                    Variables_globales.Set_Variable_Global(Flag_Creditos_D_P, false);
                    Transmite_Contadores_Accounting(); /*Transmite contadores por calculo de premio*/
                    /* Si  fue un operador  */
                    if (Variables_globales.Get_Variable_Global(Conexion_RFID))
                        Status_Barra(Reset_Exitoso);
                    Reset_Handle_LED();
                    delay(1);
                    // if (Variables_globales.Get_Variable_Global(Conexion_RFID))
                    //     Status_Barra(GMASTER_CONFIRMA_RESET); /*Espera por confirmación de sistema en AZUL*/
                    // Reset_Handle_LED();
                    Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                }
                Transmite_Confirmacion('D', '1');
                Conta_Poll_Cancel_Poker = 0;
                contadorActiv=0;
            }
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}


void Inicializa_Buffer_Eventos(void)
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

void Maneja_Marca_Eventos(void)
{
    if(Ptr_Eventos_Marca_Temp>=MAX_LIM_EVENTOS)
    {
        Inicializa_Buffer_Eventos();
    }else{
        Ptr_Eventos_Marca = Ptr_Eventos_Marca_Temp;
    }
    Transmite_Confirmacion('A','1');
}

void Tratamiento_Trama_Cashless(char res[])
{
    unsigned char Temp_1, Temp_2;

    if (contadores.Verify_Client())
    {
        /* ID Cliente en sistema y en tarjeta son iguales */
        if (res[11] == contadores.Get_Client_ID_Temp()[0])
        {

            /* Recupera valores BCD*/

            if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 1 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 3)
            {
                /* Incrementa Numero de Transaccion ID*/
                /* Transferencia AFT_Maq*/

                /*SI*/
                    /*Flag transaccion OK*/
                    /*SESION AFT 1*/
                    /*ACK AFT 3,1,4*/
                /*NO*/
                    /*borra ID Temp*/
                    /*borra Operador*/
                    /*Habilita lectura RFID*/
                    /*Indica transacion NO OK*/
                    /*Transmite mensaje transacion abortada*/
            }
            else if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2) // EFT
            {
                /* Incrementa Numero de Transaccion ID EFT*/
                /* Transferencia EFT_Maq*/
                /*SI*/
                    /*SESION EFT 1*/
                    /*ACK AFT 3,1,4*/
                    /*Transmite_Transaccion a sistema*/
                /*NO*/
                    /* Habilita lectura RFID*/
                    /*Transmite mensaje transacion abortada*/
            }
        }
        else
        {
            /* ID servidor diferente ID de solicitud*/
        }
    }else{
        /* Transmite Transacion aborted*/
        /*Habilita lector RFID*/
    }
}


void Actualiza_Contadores(void)
{
    /* Actualiza Contadores Por eventos Maquina */
    if(Variables_globales.Get_Variable_Global(Comunicacion_Maq))
    {
        if (Variables_globales.Get_Variable_Global(Billete_Insert))
        {
           
            Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 0);
            Actualiza_Maquina_En_Juego(); /*Actualiza Billetero,Creditos,Coin in,Coin out*/
            delay(250);
            Transmite_Contadores_Accounting();
            Variables_globales.Set_Variable_Global(Billete_Insert, false);
        }
    }else{

        if(Variables_globales.Get_Variable_Global(Billete_Insert))
        {
            Variables_globales.Set_Variable_Global(Billete_Insert, false);
        }
    }
}

