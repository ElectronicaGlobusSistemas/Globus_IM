/**
 * @file RFID.cpp
 * @author Globus Sistemas
 * @brief 
 * @version 1.0
 * @date 
 * 
 * @copyright Copyright (c) 2023
 * 
 */

/*
Lector RFID  SPI/I2C  Utiliza la interfaz  SPI para controlar  el modulo RC522 y la interfaz I2C-
para  el control de las notificaciones - sonoras y visuales. Para las notificaciones utiliza LED'S
NEOPIXEL WS2812B el cual requiere de un pin del procesador.Por otro lado, es necesario modificar la libreria MFRC522
especificamente el archivo MFRC522.cpp, debido a que  el procesador no cuenta  con los pines suficientes
para  controlar el modulo de RFID.Para dar solución, se implemento un expansor I2C  con el objetivo de generar mas 
entras/salidas digitales en el procesador especificamente en los pines SS y RST  del modulo RC522. Por ultimo,
como el modulo  RFID  utiliza el mismo BUS SPI que la tarjeta SD se   debe optimizar el uso  de cada dispositivo
liberando el bus despues de usarlo.

*************************************
********FUNCIONES PRINCIPALES********
*************************************


---> Init_RFID:  Realiza un Scanner en el puerto I2C para detectar si se encuentra conectado el modulo RFID
     El modulo tiene la dirección 0x27

---> Lee_Tarjeta: Detecta  tarjeta cliente/operador 
*/
#include <WiFi.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <MFRC522.h>
#include "Buffers.h"
#include "Maquina_Estados.h"
#include "Configuracion.h"
#include "PCF8574.h"
#include <Adafruit_NeoPixel.h>
#include "RFID.h"
#include "Memory_SD.h"
#include <SD.h>
#include "I2CScanner.h"
#include "Errores.h"
#include "ArduinoJson.h"
#include <HTTPClient.h>
#include "Buffer_Cashless.h"
#include <queue>
#include <SPIFFS.h>
#include <esp_task_wdt.h>

#include "Preferences.h"
#include <time.h>
#include "Transacciones.h"




#include "Pantalla_TFT.h"

#define OFFSET_PLAYER_TRACKING      30000
#define PAYOUT_TIMEOUT              10000
#define CREDIT_MINIMUM              10
#define TIME_EXECUTE                5000

time_t inicioSesion;
time_t Corte_Hora;

extern Preferences NVS;
extern IPAddress ipDest;


Pantalla_TFT DisplayTFT;
esp_timer_handle_t readTimer;
esp_timer_handle_t Token_Cash;
// Vector para almacenar las transacciones pendientes
std::vector<String> transaccionesPendientes;

int Tipo_Display=TFT_UNKNOW;

// Vector para almacenar las transacciones pendientes
std::vector<String> SesionesPendientes;
SemaphoreHandle_t mutexSesiones = xSemaphoreCreateMutex();
// Flag para verificar si hay transacciones pendientes
bool hayTransaccionesPendientes = false;
bool hayTransaccionesPendientes_Download = false;
bool Hay_BA_Pendientes=false;
bool Transfer_Pending_Load=false;
bool Transfer_Pending_Download=false;

unsigned long Count_Reader_Init=0;
unsigned long Count_Failed_Reader=0;

extern int Inactividad_Usuario_Player_Tracking;

unsigned long Brek_Point=0;
unsigned long Brek_Point_Final=0;
int Timeout_Break_Point=20000;

bool Update_In_Cashless=false;
bool Update_Out_Cashless=false;
bool Update_In_Tito=false;
bool Update_Out_Tito=false;
// Archivo para almacenar transacciones pendientes
const char* transaccionesFile = "/transacciones.txt";

// Archivo para almacenar sesiones pendientes
const char* SesionesFile = "/sesionespendientes.txt";

extern const char* archivo;

extern const char* archivo_Fide;
extern const char* archivo_Cashless;
extern const char* archivo_Acounting;
//const char* LogError = "/Loggin.txt";
// Intervalo de reintento (60 segundos)
const unsigned long intervaloReintento = 30000;
bool  Reset_Timer_Tranfer_Pending=false;
unsigned long tiempoUltimoReintento = 0;
unsigned long Tiempo_Inicio_Intento=0;

int CodeHttp=0;

int Copia_Bill_Amount_Sesiones=0;
bool Sesion_Anterior_Existe=false;

#define ADRESS          0x27
#define RST_RFID        22
#define Buzzer          1 
#define Control         2
#define LED1            3
#define LED2            4
#define LED3            5
#define Sensor          6
#define LED4            7
#define SS_PIN          34
#define Init_RFID_ 
#define Control_General 

extern bool Flag_Entradas_Cashless_OK;
extern bool Flag_Salidas_Cashless_OK;

bool Flag_Sesion_Ok=false;

//#define DEBUG_RFID
//#define DEBUG_SESIONES_A

extern unsigned long New_Timer_Final;
extern unsigned long New_Timmer_Inicial;


unsigned long Timer_Inicial_Poll;
unsigned long Timer_Final_Poll;
int Timeout_Check_Reader_Polling=3000;


long entrada = 0;
long salida = 0;

/* ------------------------------> Variables externas <-------------------------------------------*/
extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales
extern Buffers Buffer;            // Objeto de buffer de mensajes servidor
extern bool Solicitud_Carga_Cashless(void);
extern bool Solicitud_Descarga_Cashless(void);
extern bool Actualiza_Cashless_Salidas(void);
extern bool Actualiza_Cashless_Entradas(void);
extern bool Consulta_Creditos_Cashless(void);
extern bool Creditos_Machine(void);
extern bool Solicitud_Forzada_Carga_Cashless();
extern Contadores_SAS contadores; // Objeto contiene contadores maquina
extern Configuracion_ESP32 Configuracion;
extern ESP32Time RTC;
extern Transsaccion_Cashless Cashless;
extern SPIClass spiRFID;



Cashless_API Info_Cashless;
extern Buffer_RX_AFT Buffer_Cashless;
extern int Estado;
extern int Sig_Estado;
extern int Debug_;
int Contador_Colores=0;
bool Handle_RF=false;
extern unsigned long Machine_In_Game;
extern bool Machine_In_Game_Status;
extern unsigned long  Machine_Previous;
extern bool PREMIO_PENDIENTE;

extern int Contador_Transmision_Contadores;
extern int Contador_Maquina_En_Juego;

extern char Archivo_CSV_Sesiones[200];
bool Activa_ALERT=false;
extern char Archivo_CSV_Contadores[200];
extern char Archivo_CSV_Eventos[200];
extern char Archivo_LOG[200];
extern char Archivo_CSV_Premios[200];
extern bool Condicion_Cumpl;
extern char Tipo_Tarjeta_RFID_TFT;
extern int Id_Cliente_TFT_Display;

static unsigned long tiempo_inicio_sesion = 0;

char Buffer_Info_Lector[200];

unsigned long Timeout_Task_PlayerInicial=0;

DynamicJsonDocument Objeto_Transfer(1024);
DynamicJsonDocument Objeto_Transfer_Download(1024);
/*-----------------------------------------------------------------------------------------------*/
/* ------------------------------> Instancias <-------------------------------------------------- */
//MFRC522 mfrc522(P2, MFRC522::UNUSED_PIN);   // Create MFRC522 instance.

 MFRC522 mfrc522(P2, P0);   // Create MFRC522 instance.
 PCF8574 pcf8574(ADRESS); //  Dirección expansor I2C.
/*------------------------------------------------------------------------------------------------*/

/* -----------------------------> Controlador de tareas <-----------------------------------------*/
 TaskHandle_t RFID;    //  Manejador de tareas
 static void Read_RFID(void *parameter);
/*------------------------------------------------------------------------------------------------*/

/* -----------------------------> Inicializa Barra de estatus <-----------------------------------*/
Adafruit_NeoPixel Barra_Status_Sesion_Client=Adafruit_NeoPixel(4,13,NEO_RGB + NEO_KHZ800);
esp_timer_handle_t token_timer;
esp_timer_handle_t Cashless_Pending;
/*------------------------------------------------------------------------------------------------*/
I2CScanner scanner;


extern TransaccionCashless AFT;


int NivelUsuarioInt(String Nivel)
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

void Sesion_Abierta_Color(int Figura);
/* --------------------------------------> Variables <--------------------------------------------*/
 unsigned long Time_Previo_=0;
 unsigned long Timeout_Espera=60000; // Tiempo de espera  inactividad. 80s
 unsigned long Time_Init_ =0;

 /*Variables */
 unsigned long Timeout_Sesion_Inicial =0;
 unsigned long Timeout_Sesion_Final_=0;
 int Timeout_Inactividad= 120000 ;  // 2minutos 

 unsigned long Start_Cambio_Color=0;    //  Variable para Timer
 unsigned long Previous_Cambio_Color=0; //  Variable para Timer
 unsigned long OK_Color=200;           //  Tiempo de cambio de  color  y figura en sesion abierta

bool Handle_LED=false;
void Consulta_Info_Cliente_Sistema(void);
void Serializacion_cashless(void);
bool VERIFY_WIRE_CONNECTION=false;



bool Sesion_Anterior=false;
bool LED=false;
unsigned long  Encendido=0;
unsigned long  Apagado=0;
int Contador_Colores_Verde=0;
const uint8_t num_addresses = 1;
const byte addresses[num_addresses] = { 0x27};
bool results[num_addresses] = {false};
unsigned long TimeOUT=0;
unsigned long TimeIn=0;
unsigned Parpadeo;
bool Sesion_Cerrada_Color=false;
int toneDuration = 1000; // Duración del tono en milisegundos
int halfPeriod = 370; // Mitad del período de conmutación para obtener una frecuencia de 2700 Hz
int numCycles = toneDuration * 1000 / (2 * halfPeriod); // Número de ciclos para la duración total del tono
int Compuesta=0;
/*-------------------------> Variables Timeout Player tracking <---------------------------------------*/
extern unsigned long currentTime;
extern  bool condicionCumplida;
extern  unsigned long startTime;
/*-----------------------------------------------------------------------------------------------------*/

unsigned long TimeOut_Automatico=0;
unsigned long TimeOut_Automatico_Inicial=0;
int Close_Automatico=120000;

bool New_State_Sesion=false;

extern int Intentos_Conect_RFID;

unsigned long Verify_Status_RFID=0;
unsigned long Verify_Status_RFID_Final=0;
int Enable_Verify=20000;




/* Metodo para  generar sonidos de notificacion */
void customTone(int Itera, int Tono)
{
    if(Tono==1)
    {
      for (int i = 0; i < 80; i++)
      {
          pcf8574.digitalWrite(P1, HIGH);
          delayMicroseconds(300); // wait for 1ms
          pcf8574.digitalWrite(P1, LOW);
          delayMicroseconds(5); // wait for 1ms
      }
      // output another frequency
      for (int i = 0; i < 100; i++)
      {
          pcf8574.digitalWrite(P1, HIGH);
          delayMicroseconds(300);
          pcf8574.digitalWrite(P1, LOW);
          delayMicroseconds(5);
      }
    }
    else if(Tono==2)
    {
        for (int i = 0; i < 10; i++)
      {
          pcf8574.digitalWrite(P1, HIGH);
          delayMicroseconds(300); // wait for 1ms
          pcf8574.digitalWrite(P1, LOW);
          delayMicroseconds(5); // wait for 1ms
      }
      // output another frequency
      for (int i = 0; i < 50; i++)
      {
          pcf8574.digitalWrite(P1, HIGH);
          delayMicroseconds(300);
          pcf8574.digitalWrite(P1, LOW);
          delayMicroseconds(5);
      }
    }

    else if(Tono==3)
    {
         for (int i = 0; i < 80; i++)
      {
          pcf8574.digitalWrite(P1, HIGH);
          delayMicroseconds(500); // wait for 1ms
          pcf8574.digitalWrite(P1, LOW);
          delayMicroseconds(300); // wait for 1ms
      }
      // output another frequency
      for (int i = 0; i < 100; i++)
      {
          pcf8574.digitalWrite(P1, HIGH);
          delayMicroseconds(500);
          pcf8574.digitalWrite(P1, LOW);
          delayMicroseconds(300);
      }
    } else if(Tono==4)
    {
          for (int i = 0; i < 255; i++)
      {
          pcf8574.digitalWrite(P1, HIGH);
          delay(1); // wait for 1ms
          pcf8574.digitalWrite(P1, LOW);
          delay(1);
      }
      // output another frequency
      for (int i = 0; i < 100; i++)
      {
          pcf8574.digitalWrite(P1, HIGH);
          delay(2);
          pcf8574.digitalWrite(P1, LOW);
          delay(2);
      }
          
    }
  }


//delay(100);
// //void Check_RFID(void)
// {
//     byte gain = mfrc522.PCD_GetAntennaGain();

//     if (gain >= 0 && gain <= 64)
//     {
//         Variables_globales.Set_Variable_Global(Conexion_RFID, true); /*Modulo OK*/
//     }
//     else
//     {
//         Variables_globales.Set_Variable_Global(Conexion_RFID, false);
//     }
// }

void Reset_Down_Mode(void)
{
    pcf8574.digitalWrite(P0, HIGH);
    mfrc522.PCD_Reset();
}

/**********************************************************************************/
/*                              CONECTA MÓDULO DESPUES DE INICIO                  */
/**********************************************************************************/
void Check_RFID(void)
{

    if (Tipo_Display != TFT_M5STACK_DIAL)
    {
        byte version;
        scanner.Init();

        for (uint8_t index = 0; index < num_addresses; index++)
        {
            results[index] = scanner.Check(addresses[index]);
        }

        for (uint8_t index = 0; index < num_addresses; index++)
        {
            if (results[index])
            {
                // Serial.print("Found device ");
                // Serial.print(index);
                // Serial.print(" at address ");
                // Serial.println(addresses[index], HEX);
                VERIFY_WIRE_CONNECTION = true;
            }
        }
        if (VERIFY_WIRE_CONNECTION)
        {

            pcf8574.begin(); // Inicializa Expansor I2C
            pcf8574.pinMode(P1, OUTPUT);
            pcf8574.pinMode(P0, INPUT);
            pcf8574.pinMode(P2, OUTPUT);

            delay(5);    /* Espera para aplicar configuración */
            SPI.begin(); /* Inicializa Puerto SPI*/
            mfrc522.PCD_Reset();
            pcf8574.digitalWrite(P0, LOW);
            delay(300);
            mfrc522.PCD_Init(); // Inicializa Módulo RFID
            delay(10);
            // mfrc522.PCD_Init(); // Inicializa Módulo RFID
            Serial.println(pcf8574.digitalRead(P0));
            Status_Barra(INICIO_MODULO);
            delay(100);
            // mfrc522.PCD_Init();                              // Inicializa Módulo RFID
            mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_48dB); // Activa  antena con ganancia 33dB RxGain_33dB
            byte gain = mfrc522.PCD_GetAntennaGain();        // Obtiene  configuracion de antena  para verifica conexión  de modulo RFID

            if (gain == mfrc522.RxGain_18dB ||
                gain == mfrc522.RxGain_23dB ||
                gain == mfrc522.RxGain_33dB ||
                gain == mfrc522.RxGain_38dB ||
                gain == mfrc522.RxGain_43dB ||
                gain == mfrc522.RxGain_48dB)
            {
                /* --------------------------- >Configura RIFD <------------------------------------*/
                mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_48dB); /* Configura Gancia en la antena*/
#ifdef Init_RFID_
                Serial.println("Modulo RFID Inicializado....");
#endif
                version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
                Report_Http_Code(READER_KO, "Modulo RFID Inicializado correctamente version:  " + String(version), true);
                Variables_globales.Set_Variable_Global(Conexion_RFID, true); /*Modulo OK*/
                Status_Barra(MODULO_OK);
                Variables_globales.Set_Variable_Global(Verify_Modulo_RFID, true);
            }
            else
            {
#ifdef Init_RFID_
                Serial.println("Error Inicializando Modulo");
#endif
                Variables_globales.Set_Variable_Global(Conexion_RFID, false);
                Status_Barra(MODULO_KO);
                Intentos_Conect_RFID++;
                Report_Http_Code(READER_KO, "Modulo RFID no inicializado:  " + String(version), false);
            }
        }
        else
        {
            Serial.println("Modulo RFID no Conectado...");
            Intentos_Conect_RFID++;
            Report_Http_Code(READER_KO, "Modulo RFID no conectado:  ", false);
        }
    }
    else
    {
        Variables_globales.Set_Variable_Global(Verify_Modulo_RFID, true);
    }
}

/**********************************************************************************/
/*                              Inicializa Modulo RFID                            */
/**********************************************************************************/

void RFID_TFT_DISPLAY()
{
    
    if (Variables_globales.Get_Variable_Global(Lectura_RFD_TFT_Display))
    {
        
        Variables_globales.Set_Variable_Global(Lectura_RFD_TFT_Display, false);
        byte Tipo[18];

        byte Id[18];
        char Id_Client[18];
        Tipo[0] = Tipo_Tarjeta_RFID_TFT;
        // Serial.println(Tipo[0]);

        
        sprintf(Id_Client, "%08d", Id_Cliente_TFT_Display);
        // Serial.println();

        int Cont=1;
        for (int i = 0; i < 17; i++)
        {
            Id[i] = Id_Client[i];
            // Serial.println(Id[i]);
        }

        /*------------------------------------------------------------------------------------------*/
        /*---------------------------------->Usuario Valido <----------------------------------------*/

        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
        bool OK = false;

        /* Pregunta si es maquina Cashless */
        if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) < 4 ||
            Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
        {
#ifdef DEBUG_RFID
            Serial.println("Maquina Cashless identificada");
#endif

            /* Si tiene  transacciones pendientes  no permite transacciones */
            if (Variables_globales.Get_Variable_Global(Event_Dowmload_Cashless_Pending) || Variables_globales.Get_Variable_Global(Event_Load_Cashless_Pending) || !transaccionesPendientes.empty())
            {
#ifdef DEBUG_RFID
                Serial.println("Transferencia pendiente!");
#endif
                OK = false;
            }
            else
            {
#ifdef DEBUG_RFID
                Serial.println("No existen transferencias pendientes!");
#endif
                OK = true;
            }
        }
        else /* Si no es maquina Cashless no valida transacciones pendientes para lectura de tarjetas */
        {
#ifdef DEBUG_RFID
            Serial.println("Maquinas Solo fidelizacion");
#endif
            OK = true;
        }

        if (OK && !Variables_globales.Get_Variable_Global(Status_Games_Machine))
        {
            
            Info_Cashless.Log(RTC, "LECTURA_TARJETA", "OK");
            Cliente_VS_Operador(Tipo, Id);
        }

        else
        {

            if (Variables_globales.Get_Variable_Global(Enable_Cashless))
            {
                if (transaccionesPendientes.empty())
                {
                    Report_Http_Code(TRANSFER_PENDING, "Lectura de tarjeta rechazada por Transaccion pendiente por consulta en maquina");
                    Status_Barra(302);
                }

                else if (Variables_globales.Get_Variable_Global(Status_Games_Machine))
                {
                    Report_Http_Code(MAQUINA_EN_JUEGO, "lectura de tarjeta rechazada por maquina en juego");
                    Status_Barra(302);
                    Info_Cashless.Log(RTC, "Rechaza_lectura_de_tarjeta_por_Maquina_en_juego");
                }
                else
                {
                    /* El servidor aun no recibe el ack */
                    Report_Http_Code(TRANSFER_PENDING, "lectura de tarjeta rechazada por transaccion pendiente de recepcion");
                    Status_Barra(302);
                    Info_Cashless.Log(RTC, "Rechaza_lectura_por_transaccion_pendiente_de_recepcion");
                }
            }
            else
            {
                Report_Http_Code(MAQUINA_EN_JUEGO, "Rechaza lectura de tarjeta por maquina en juego");
                Status_Barra(302);
                Info_Cashless.Log(RTC, "Rechaza_lectura_de_tarjeta_por_Maquina_en_juego");
            }
        }

        return;
    }
}

void Init_RFID(void)
{
    /*---------------> SCAN I2C <----------------------------*/
    byte version;

    Wire.begin(21, 22);
    scanner.Init();

    for (uint8_t index = 0; index < num_addresses; index++)
    {
        results[index] = scanner.Check(addresses[index]);
    }

    for (uint8_t index = 0; index < num_addresses; index++)
    {
        if (results[index])
        {
            // Serial.print("Found device ");
            // Serial.print(index);
            // Serial.print(" at address ");
            // Serial.println(addresses[index], HEX);
            VERIFY_WIRE_CONNECTION = true;
        }
    }
    /*-------------------------------------------------------*/
    if (VERIFY_WIRE_CONNECTION)
    {
        pcf8574.begin(); // Inicializa Expansor I2C
        delay(5);        /* Espera para estabilizar I2C */
        pcf8574.pinMode(P1, OUTPUT);
        pcf8574.pinMode(P0, INPUT);
        pcf8574.pinMode(P2, OUTPUT);
        delay(5);    /* Espera para aplicar configuración */
        SPI.begin(); /* Inicializa Puerto SPI*/
        mfrc522.PCD_Reset();
        pcf8574.digitalWrite(P0, LOW);
        delay(300);
        mfrc522.PCD_Init(); // Inicializa Módulo RFID
        delay(10);
        // mfrc522.PCD_Init(); // Inicializa Módulo RFID
        Status_Barra(INICIO_MODULO);
        delay(100);
        // Serial.println(pcf8574.digitalRead(P0));
        // mfrc522.PCD_Init();                              // Inicializa Módulo RFID
        mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_48dB); // Activa  antena con ganancia 33dB RxGain_33dB
        byte gain = mfrc522.PCD_GetAntennaGain();        // Obtiene  configuracion de antena  para verifica conexión  de modulo RFID

        mfrc522.PCD_WriteRegister(MFRC522::TxControlReg, 0x03); // Non-inverting TX2
        mfrc522.PCD_WriteRegister(MFRC522::GsNReg, 0x44);       // Also try FF
        mfrc522.PCD_WriteRegister(MFRC522::CWGsPReg, 0x0F);     // Also try 3F
        mfrc522.PCD_WriteRegister(MFRC522::ModGsPReg, 0x0F);    // Also try //3F

        if (gain == mfrc522.RxGain_18dB ||
            gain == mfrc522.RxGain_23dB ||
            gain == mfrc522.RxGain_33dB ||
            gain == mfrc522.RxGain_38dB ||
            gain == mfrc522.RxGain_43dB ||
            gain == mfrc522.RxGain_48dB)
        {

            /* --------------------------- >Configura RIFD <------------------------------------*/
            mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_48dB); /* Configura Gancia en la antena*/
#ifdef Init_RFID_
            Serial.println("💳 Modulo RFID Inicializado....✅");
#endif
            Variables_globales.Set_Variable_Global(Conexion_RFID, true); /*Modulo OK*/
            Status_Barra(MODULO_OK);
            Variables_globales.Set_Variable_Global(Verify_Modulo_RFID, true);
            version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
            String Hex_De = "0x";
            Report_Http_Code(READER_OK, "💳 Modulo RFID Inicializado correctamente version ✅" + Hex_De + String(version, HEX) + ":", true);

            if (Init_TFT_Display())
            {
                Report_Http_Code(TFT_NOT_INIT, "📺 Pantalla TFT inicializada correctamente ✅", true);
                Serial.println();
            }
            else
            {

                if (Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
                {
#ifdef Init_RFID_
                    Serial.println("📺 Pantalla TFT no inicializada ❌");
#endif
                    Report_Http_Code(TFT_NOT_INIT, "📺 Pantalla TFT no inicializada ❌", false);
                }
                else
                {
#ifdef Init_RFID_
                    Serial.println("📺 No exite Pantalla TFT asociada 🚫");
#endif
                    Report_Http_Code(TFT_NOT_INIT, "📺 No exite Pantalla TFT asociada 🚫", true);
                }
            }
        }

        else
        {
            String Hex_De = "0x";
#ifdef Init_RFID_
            Serial.println("Error Inicializando Modulo");
#endif
            Variables_globales.Set_Variable_Global(Conexion_RFID, false);
            Status_Barra(MODULO_KO);
            Report_Http_Code(READER_KO, "Modulo RFID no inicializado" + Hex_De + String(version, HEX) + ":", false);
        }
    }
    else
    {
        Serial.println("Modulo RFID no Conectado...");

        Report_Http_Code(READER_KO, "Modulo RFID no conectado:  ", false);
    }
}

void Resurrect_reader(void)
{

    // if (Tipo_Display != TFT_M5STACK_DIAL)
    // {
    Brek_Point = millis();

    if ((Brek_Point - Brek_Point_Final) >= Timeout_Break_Point && !Variables_globales.Get_Variable_Global(Updating_System))
    {
        bool Test;
        byte version;
        if (Variables_globales.Get_Variable_Global(Conexion_RFID))
        {
            // Serial.println(Count_Reader_Init);
            mfrc522.PCD_AntennaOff();
            delay(5);
            Count_Reader_Init++;
            SPI.end();   /* Inicializa Puerto SPI*/
            delay(5);    /* Espera para aplicar configuración */
            SPI.begin(); /* Inicializa Puerto SPI*/
            delay(5);
            mfrc522.PCD_Reset();
            pcf8574.digitalWrite(P0, LOW);
            delay(100);
            mfrc522.PCD_Init(); // Inicializa Módulo RFID
            delay(10);
            // Inicializa Módulo RFID
            mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_48dB); // Activa  antena con ganancia 33dB RxGain_33dB
            mfrc522.PCD_AntennaOn();
            mfrc522.PCD_WriteRegister(MFRC522::TxControlReg, 0x03);
            mfrc522.PCD_WriteRegister(MFRC522::GsNReg, 0x44);
            mfrc522.PCD_WriteRegister(MFRC522::CWGsPReg, 0x0F);
            mfrc522.PCD_WriteRegister(MFRC522::ModGsPReg, 0x0F);

            byte gain = mfrc522.PCD_GetAntennaGain();        // Obtiene  configuracion de antena  para verifica conexión  de modulo RFID

            mfrc522.PCD_WriteRegister(MFRC522::TxControlReg, 0x03); // Non-inverting TX2
            mfrc522.PCD_WriteRegister(MFRC522::GsNReg, 0x44);       // Also try FF
            mfrc522.PCD_WriteRegister(MFRC522::CWGsPReg, 0x0F);     // Also try 3F
            mfrc522.PCD_WriteRegister(MFRC522::ModGsPReg, 0x0F);    // Also try //3F

            if (gain == mfrc522.RxGain_18dB ||
                gain == mfrc522.RxGain_23dB ||
                gain == mfrc522.RxGain_33dB ||
                gain == mfrc522.RxGain_38dB ||
                gain == mfrc522.RxGain_43dB ||
                gain == mfrc522.RxGain_48dB)
            {

                /* --------------------------- >Configura RIFD <------------------------------------*/
                mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_48dB); /* Configura Gancia en la antena*/
                                                                 // #ifdef Init_RFID_
                                                                 // Serial.println("Modulo RFID Inicializado....");
                //  #endif
                Variables_globales.Set_Variable_Global(Conexion_RFID, true); /*Modulo OK*/
                // Status_Barra(MODULO_OK);
                Variables_globales.Set_Variable_Global(Verify_Modulo_RFID, true);
                Test = true;
            }
            else
            {
                Test = false;
                // #ifdef Init_RFID_
                // Serial.println("Error Inicializando Modulo");
                // #endif
                Variables_globales.Set_Variable_Global(Conexion_RFID, false);
                Count_Failed_Reader++;
            }

            if (Count_Reader_Init >= 50 && !Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
            {
                String Hex_De = "0x";
                version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
                // if (Test)
                //     Report_Http_Code(READER_OK, "Modulo RFID iniciado correctamente" + Hex_De + String(version, HEX) + ":", true);
                // else
                //     Report_Http_Code(READER_KO, "Modulo RFID no inicializado" + Hex_De + String(version, HEX) + ":", false);

                Count_Reader_Init = 0;
            }
        }
        else
        {
            if (Count_Reader_Init >= 50 && !Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
            {
                Report_Http_Code(READER_KO, "Modulo RFID no conectado:  ", false);
                Count_Reader_Init = 0;
            }
        }

        // if(Count_Failed_Reader>=5)
        // {

        //     Report_Http_Code(RSET_IP,"Limite de intentos de conexion de lector RFID alcanzado reinicio de dispositivo:  ",false);

        //     delay(250);
        //     ESP.restart();
        //     Count_Failed_Reader=0;
        // }

        Brek_Point_Final = Brek_Point;
    }
    //}//
}

void check_Status_Reader_Polling(void)
{

    Timer_Inicial_Poll = millis();

    if ((Timer_Inicial_Poll - Timer_Final_Poll) >= Timeout_Check_Reader_Polling && Variables_globales.Get_Variable_Global(Conexion_RFID))
    {
        byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);

        bool Test=false;

        if(pcf8574.digitalRead(P0) == LOW)
        {
            pcf8574.pinMode(P0, OUTPUT);		// Now set the resetPowerDownPin as digital output.
			pcf8574.digitalWrite(P0, HIGH);
            Test=true;
        }
        if(Test)
            mfrc522.PCD_Reset();
        
        // Serial.print("VersionReg: ");
        // Serial.println(version, HEX);
        if (version == 0x00 || version == 0xFF)
        {
        
           
            delay(10);
            mfrc522.PCD_Init(); // Inicializa Módulo RFID                           // Inicializa Módulo RFID
            mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_48dB); // Activa  antena con ganancia 33dB RxGain_33dB
        }
        
       // SPI.begin();
        // Paso 1: Activar alimentación lógica (si aplica)
        digitalWrite(5, HIGH); // Si este pin controla alimentación o lógica asociada al lector
        // Paso 2: Poner el pin de reset en LOW (reset activo)
        pcf8574.pinMode(P0, OUTPUT);
        pcf8574.digitalWrite(P0, LOW);
        delay(10); // Breve espera con reset activo (mínimo 1ms, con 10ms sobra)
        // Paso 3: Poner el pin de reset en HIGH (salida del reset)
        pcf8574.digitalWrite(P0, HIGH);
        delay(100); // Esperar a que el RC522 arranque (oscila + lógica interna)
        mfrc522.PCD_Reset();
        delay(10);
        mfrc522.PCD_Init(); // Inicializa Módulo RFID  
        //Serial.println(pcf8574.digitalRead(P0));

        Timer_Final_Poll = Timer_Inicial_Poll; 
    }
}

/* Verifica estado de lector RFID en tiempo de ejecución */

/*
void Check_RFID_Real_Time(void)

{
    if ((Verify_Status_RFID - Verify_Status_RFID_Final) >= Enable_Verify && !Variables_globales.Get_Variable_Global(Ftp_Mode))
    {
        if (mfrc522.PCD_DumpVersionToSerial2())
        {
            Variables_globales.Set_Variable_Global(Conexion_RFID, true);
        }
        else
        {
            Variables_globales.Set_Variable_Global(Conexion_RFID, false);
        }

        Verify_Status_RFID_Final = Verify_Status_RFID;
    }
}
*/

void Animation_Free_Session(void)
{
    if ((Start_Cambio_Color - Previous_Cambio_Color) >= OK_Color && !Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
    {
        Contador_Colores_Verde++;
        // Serial.println(Contador_Colores_Verde);
        int num = 0;
        if (random(1, 5) < 3)
        {
            num = 1;
        }
        else
        {
            if (Contador_Colores_Verde > 60)
            {
                num = random(2, 8);
            }
            else
            {
                num = 1;
            }
        }
        if (Handle_RF == false)
        {
            num = 1;
            Handle_RF = true;
        }
        if (!Handle_LED)
        {
            if (Sesion_Cerrada_Color == true)
            {
                Sesion_Cerrada_Color = false;
                num = 1;
                Contador_Colores_Verde = 0;
            }
            Sesion_Abierta_Color(num);
        }
        Previous_Cambio_Color = millis();
        if (Contador_Colores_Verde >= 120)
        {
            Contador_Colores_Verde = 0;
        }
    }
}
/* Metodo para leer tarjetas RFID usuario - operador  */
void Lee_Tarjeta()
{

    // switch (Tipo_Display)
    // {

    // case TFT_M5STACK_DIAL:
    //     RFID_TFT_DISPLAY();
    //     break;

    // case TFT_48S3_43:
    // case TFT_UNKNOW:

    // default:
    //     /* Aqui Logica lector RFID externo */
    //     break;
    // }

    Verify_Status_RFID=millis();

    Start_Cambio_Color = millis();
    byte block;
    byte len;
    byte block_2;
    MFRC522::StatusCode status;


    

  //  Check_RFID_Real_Time();
    
    
        /* Habilitado */

        /*----------------------------------------> Indicador estados del lector <---------------------------------------------*/
        if (!Variables_globales.Get_Variable_Global(Conexion_RFID) || !Variables_globales.Get_Variable_Global(Comunicacion_Maq) || Variables_globales.Get_Variable_Global(Flag_Sesion_RFID) ||Variables_globales.Get_Variable_Global(Updating_System) || Variables_globales.Get_Variable_Global(Access_Point_Mode)|| WiFi.status()!=WL_CONNECTED)
        {
            if (!Handle_LED)
            {
                /*---------------------------------------> Sesion Iniciada <-------------------------------------------------------*/
                if (Variables_globales.Get_Variable_Global(Flag_Sesion_RFID) || !Variables_globales.Get_Variable_Global(Conexion_RFID) && Variables_globales.Get_Variable_Global(Comunicacion_Maq) &&!Variables_globales.Get_Variable_Global(Updating_System) &&!Variables_globales.Get_Variable_Global(Access_Point_Mode))
                {
                    if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                    {
                        if (!Activa_ALERT)
                        {
                            Barra_Status_Sesion_Client.setBrightness(20);
                            Barra_Status_Sesion_Client.clear();
                            Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                            Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                            Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                            Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                            Barra_Status_Sesion_Client.show();
                        }
                    }
                }
                /*-----------------------------------------------------------------------------------------------------------------*/

                /*-------------------------------------> NO HAY COMUNICACION <-----------------------------------------------------*/
                if (!Variables_globales.Get_Variable_Global(Comunicacion_Maq) && Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9 && !Variables_globales.Get_Variable_Global(Updating_System) && !Variables_globales.Get_Variable_Global(Access_Point_Mode) && Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 15)
                {

                    TimeOut_Automatico_Inicial = millis();
                    /* Garantiza Mantener el tiempo */
                    if (!New_State_Sesion)
                    {
                        TimeOut_Automatico = TimeOut_Automatico_Inicial;
                        New_State_Sesion = true;
                    }

                    if ((TimeOut_Automatico_Inicial - TimeOut_Automatico) >= Close_Automatico && Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
                    {

                        // if (Info_Cashless.Type_Sesion() != PLAYER_CASHLESS_SESION)
                        // {
                        //     Close_Sesion_Player_Tracking();
                        //     Report_Http_Code(CLOSE_PLAYER_TRACKING,"Sesion cerrada por  perdida de comunicacion con la MET: ");
                        // }   
                        TimeOut_Automatico = TimeOut_Automatico_Inicial;
                    }
                    if (!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                    {
                        Status_Barra(NO_HAY_COMUNICACION);
                    }
                }
                else
                {
                    New_State_Sesion = false;
                }
                /*-----------------------------------------------------------------------------------------------------------------*/
                if(!Variables_globales.Get_Variable_Global(Conexion_RFID) &&!Variables_globales.Get_Variable_Global(Updating_System) && !Variables_globales.Get_Variable_Global(Access_Point_Mode))
                {
                    Barra_Status_Sesion_Client.clear();
                    Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(255, 0, 255)); // gris
                    Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(255, 0, 255)); // gris
                    Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(255, 0, 255)); // gris
                    Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(255, 0, 255)); // gris
                    Barra_Status_Sesion_Client.show();
                }

                if(Variables_globales.Get_Variable_Global(Updating_System) &&!Variables_globales.Get_Variable_Global(Access_Point_Mode))
                {
                    Status_Barra(UPDATING_SYS);
                }

                if(Variables_globales.Get_Variable_Global(Access_Point_Mode))
                {
                   Status_Barra(NOT_AP_MODE); 
                }

                if(WiFi.status()!= WL_CONNECTED && Variables_globales.Get_Variable_Global(Comunicacion_Maq)&& !Variables_globales.Get_Variable_Global(Updating_System)&&!Variables_globales.Get_Variable_Global(Access_Point_Mode))
                {
                    Status_Barra(WIFI_CONEXION_FAILED); 
                }
            }
        }
        /*-----------------------------------------------------------------------------------------------------------------*/
        if (Variables_globales.Get_Variable_Global(Conexion_RFID) && Variables_globales.Get_Variable_Global(Comunicacion_Maq) && Info_Cashless.Get_Status_Reader() == false && !Variables_globales.Get_Variable_Global(Updating_System) && !Variables_globales.Get_Variable_Global(Access_Point_Mode) && WiFi.status()==WL_CONNECTED)
        {

            Animation_Free_Session();
           
            // if ((!mfrc522.PICC_IsNewCardPresent())||!mfrc522.PICC_ReadCardSerial())
            // {
            //     RESET_Handle();
            //     return;
            // }
            // digitalWrite(5,HIGH);
            // pcf8574.digitalWrite(P2, LOW);

            if (!mfrc522.PICC_IsNewCardPresent())
            {
                RESET_Handle();
                return;
            }

            // Select one of the cards
            if (!mfrc522.PICC_ReadCardSerial())
            {
                RESET_Handle();
                return;
            }
            
            // Serial.println(F("**Card Detected:**"));
            //-------------------------------------------

            mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid)); // dump some details about the card

            // if (!mfrc522.PICC_ReadCardSerial())
            // {
            //     RESET_Handle();
            //     return;
            // }

            MFRC522::MIFARE_Key key;
            for (byte i = 0; i < 6; i++)
                key.keyByte[i] = 0xFF;

#ifdef DEBUG_RFID
            Serial.println(F("**Card Detected:**"));
            Serial.print(F("ID: "));
#endif
            byte buffer1[18];
            block = 1;
            len = 18;
            byte buffer2[18];
            //------------------------------------------- > Obtiene type tarjeta <------------------------------------
            status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
            if (status != MFRC522::STATUS_OK)
            {

                Menssage_TFT("Error de lectura de tarjeta\n Por favor intente nuevamente");

#ifdef DEBUG_RFID
                Serial.print(F("Authentication failed: "));
                Serial.println(mfrc522.GetStatusCodeName(status));
#endif
                // Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                // mfrc522.PICC_HaltA();
                // mfrc522.PCD_StopCrypto1();
                // RESET_Handle();
                Status_Barra(ERROR_LECTURA);
                Report_Http_Code(AUTHENTICATION_ERROR,"Error de autenticacion de tarjeta: ");
                Info_Cashless.Log(RTC,"LECTURA_TARJETA","Error_autenticando_tarjeta");
                return;
            }

            delay(50);

            status = mfrc522.MIFARE_Read(block, buffer1, &len);
            if (status != MFRC522::STATUS_OK)
            {

                Menssage_TFT("Error de autenticacion\n Por favor intente nuevamente");

#ifdef DEBUG_RFID
                Serial.print(F("Reading failed: "));
                Serial.println(mfrc522.GetStatusCodeName(status));
#endif
                // Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                // mfrc522.PICC_HaltA();
                // mfrc522.PCD_StopCrypto1();
                // RESET_Handle();

                Status_Barra(ERROR_LECTURA);
                Report_Http_Code(READ_ERROR,"Error de lectura de tarjeta: ");
                Info_Cashless.Log(RTC,"LECTURA_TARJETA","Error_en_lectura_tarjeta");
                return;
            }

            buffer2[0] = buffer1[1];
            buffer2[1] = buffer1[2];
            buffer2[2] = buffer1[3];
            buffer2[3] = buffer1[4];
            buffer2[4] = buffer1[5];
            buffer2[5] = buffer1[6];
            buffer2[6] = buffer1[7];
            buffer2[7] = buffer1[8];

            delay(50);
            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();

            //delay(150);

            // pcf8574.digitalWrite(P2, HIGH);
            // digitalWrite(5,LOW);
            
            
            /*-----------------------------> Verifica Usuario Valido <--------------------------------*/
            if (!contadores.Verity_ID_NOT_NULL(buffer2, 'M'))
            {
                // Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                Status_Barra(ERROR_LECTURA);
                Info_Cashless.Log(RTC, "LECTURA_TARJETA", "Buffer_de_etiqueta_RFID_Nulo");
                return;
            }
            else if (!contadores.Verify_Client_ID(buffer2))
            {
                // Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                Status_Barra(ERROR_LECTURA);
                Info_Cashless.Log(RTC, "LECTURA_TARJETA", "Buffer_de_etiqueta_RFID_0");
                return;
            }
            else if (!contadores.Verity_ID_NOT_NULL(buffer1, 'C'))
            {
                // Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                Status_Barra(ERROR_LECTURA);
                Info_Cashless.Log(RTC, "LECTURA_TARJETA", "Tarjeta_no_Operador/Cliente");
                return;
            }
            /*------------------------------------------------------------------------------------------*/
            /*---------------------------------->Usuario Valido <----------------------------------------*/
            else
            {
                Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                bool OK = false;

                /* Pregunta si es maquina Cashless */
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) < 4 ||
                    Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
                {
#ifdef DEBUG_RFID
                    Serial.println("Maquina Cashless identificada");
#endif

                    // if(AFT.GET_STATUS_TRANSFER()!=TransaccionCashless::TRANS_IDLE)
                    //     OK = false;
                    
                    // else
                    //     OK = true;
                        
                    if(AFT.GET_STATUS_TRANSFER()==TransaccionCashless::TRANS_IDLE && !Variables_globales.Get_Variable_Global(Status_Games_Machine))
                        OK = true;
                    else
                        OK = false;
                    
                    /* Si tiene  transacciones pendientes  no permite transacciones */
//                     if (Variables_globales.Get_Variable_Global(Event_Dowmload_Cashless_Pending) || Variables_globales.Get_Variable_Global(Event_Load_Cashless_Pending) || !transaccionesPendientes.empty())
//                     {
// #ifdef DEBUG_RFID
//                         Serial.println("Transferencia pendiente!");
// #endif
//                         OK = false;
//                     }
//                     else
//                     {
// #ifdef DEBUG_RFID
//                         Serial.println("No existen transferencias pendientes!");
// #endif
//                         OK = true;
//                     }
                }
                else /* Si no es maquina Cashless no valida transacciones pendientes para lectura de tarjetas */
                {
#ifdef DEBUG_RFID
                    Serial.println("Maquinas Solo fidelizacion");
#endif
                    OK = true;
                }

                if (OK)
                {
                    Menssage_TFT("Lectura de tarjeta exitosa");
                    Info_Cashless.Log(RTC, "LECTURA_TARJETA", "OK");
                    Cliente_VS_Operador(buffer1, buffer2);
                }
                else
                {

                    if (Variables_globales.Get_Variable_Global(Enable_Cashless))
                    {

                        if(AFT.GET_STATUS_TRANSFER()!=TransaccionCashless::TRANS_IDLE)
                        {
                            Status_Barra(302);
                            Report_Http_Code(TRANSFER_PENDING, "Lectura de tarjeta rechazada por transaccion en progreso o pendiente");
                            return;
                        }

                        else if (!transaccionesPendientes.empty())
                        {
                            Menssage_TFT("transaccion pendiente en la maquina\n Por favor espere..");
                            Report_Http_Code(TRANSFER_PENDING, "Lectura de tarjeta rechazada por Transaccion pendiente por consulta en maquina");
                            Status_Barra(302);
                            return;
                        }

                        else if (Variables_globales.Get_Variable_Global(Status_Games_Machine))
                        {
                            Menssage_TFT("lectura de Tarjeta rechazada por maquina en juego");
                            Report_Http_Code(MAQUINA_EN_JUEGO, "lectura de tarjeta rechazada por maquina en juego");
                            Status_Barra(302);
                            Info_Cashless.Log(RTC, "Rechaza_lectura_de_tarjeta_por_Maquina_en_juego");
                            return;
                        }
                        else
                        {
                            Menssage_TFT("transaccion pendiente de envio\n Por favor espere..");
                            /* El servidor aun no recibe el ack */
                            Report_Http_Code(TRANSFER_PENDING, "lectura de tarjeta rechazada por transaccion pendiente de recepcion");
                            Status_Barra(302);
                            Info_Cashless.Log(RTC, "Rechaza_lectura_por_transaccion_pendiente_de_recepcion");
                            return;
                        }
                    }
                    else
                    {
                        Report_Http_Code(MAQUINA_EN_JUEGO, "Rechaza lectura de tarjeta por maquina en juego");
                        Status_Barra(302);
                        Info_Cashless.Log(RTC, "Rechaza_lectura_de_tarjeta_por_Maquina_en_juego");
                    }
                }
            }
            /*------------------------------------------------------------------------------------------*/
        }
}
/*--------------------------------------------------------------------------------*/
/**********************************************************************************/
/*                              Player Tracking                                   */
/**********************************************************************************/

#ifdef OG
void Cliente_VS_Operador(byte MEMORIA[],byte INFO[])
{

    if (MEMORIA[0] == 'O')
    {
        contadores.Close_ID_Operador(); /* Borra ID operador anterior */
        contadores.Dele_Operador_INFO_Operador(); /* ID*/
        Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO,false);
        Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO,true); /* Inicia timmer */
        New_Timer_Final = New_Timmer_Inicial;
        startTime = currentTime; /* Reset TimeOut_Player Tracking */
            
        char ID_Temp_[8];
        ID_Temp_[0] = INFO[0];
        ID_Temp_[1] = INFO[1];
        ID_Temp_[2] = INFO[2];
        ID_Temp_[3] = INFO[3];
        ID_Temp_[4] = INFO[4];
        ID_Temp_[5] = INFO[5];
        ID_Temp_[6] = INFO[6];
        ID_Temp_[7] = INFO[7];
        
        contadores.ID_Consulta_INFO_Operador(ID_Temp_);
        Variables_globales.Set_Variable_Global(Consulta_Conexion_To_Host, true);

        if (Variables_globales.Get_Variable_Global(Conexion_RFID))
        {
            Status_Barra(TARJETA_OPERADOR_INSERT);
        }
        unsigned long Respuesta_Server = millis();
        int TIMEOUT_CONECT_SERVER = 6500;
        while (!Variables_globales.Get_Variable_Global(Conexion_To_Host) && millis() - Respuesta_Server < TIMEOUT_CONECT_SERVER)
        {
            #ifdef DEBUG_RFID
            Serial.println("Verificando Conexion to Host...");
            #endif
            vTaskDelay(300);
        }

        if (Variables_globales.Get_Variable_Global(Conexion_To_Host))
        {
            

            if (contadores.Set_Operador_ID_Temp(ID_Temp_))
            {
                Condicion_Cumpl=false; /* Reset Timeout*/
               

                contadores.Copy_Operator_In_();
                Variables_globales.Set_Variable_Global(Operador_Detected, true);
                Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                Update_Status_SD();
                Storage_Premios_OP(Archivo_CSV_Premios, Variables_globales.Get_Variable_Global(Enable_Storage), contadores.Get_Operador_ID());
            }
            else
            {
                Variables_globales.Set_Variable_Global(Operador_Detected, false);
                contadores.Close_ID_Operador(); /*Temporal y en Trama*/
                Status_Barra(ERROR_LECTURA);
                Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
            }
        }else{
            Variables_globales.Set_Variable_Global(Operador_Detected, false);
            Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
            Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
            contadores.Close_ID_Operador(); /*Temporal y en Trama*/
            #ifdef DEBUG_RFID
            Serial.println("Host Gmaster disconected");
            #endif
            Status_Barra(CONEXION_TO_HOTS_FAILED);
            delay(100);
            Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO,false); /* Inicia timmer */
        }

        Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
    }

    else if (MEMORIA[0] == 'C')
    {
        New_Timer_Final = New_Timmer_Inicial;
        startTime = currentTime;
        contadores.Dele_Operador_INFO_Operador(); /* ID*/
        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
        Variables_globales.Set_Variable_Global(Consulta_Conexion_To_Host, true);
       
        if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) > 3)
        {

            int contador = 0;
            for (int i = 0; i < 8; i++)
            {
                if (INFO[i] == contadores.Get_Client_ID()[i])
                {
                    contador++;
                }
            }
            if (contador >= 8) /* Cierra Sesion por Usuario*/
            {

                if(Close_Sesion_Player_Tracking())
                {
                    contador = 0;
                    Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                }else
                {
                    Status_Barra(ERROR_LECTURA);
                    delay(100);
                    contador = 0;
                    Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                }
                Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
            }
            else
            {
                Status_Barra(LECTURA_OK);
                unsigned long Respuesta_Server = millis();
                int TIMEOUT_CONECT_SERVER = 6500; //3500
                while (!Variables_globales.Get_Variable_Global(Conexion_To_Host) && millis() - Respuesta_Server < TIMEOUT_CONECT_SERVER)
                {
#ifdef DEBUG_RFID
                    Serial.println("Verificando Conexion to Host...");
#endif
                    vTaskDelay(300);
                }
                if (Variables_globales.Get_Variable_Global(Conexion_To_Host))
                {
/* Inicia Player Tracking  */
#ifdef DEBUG_RFID
                    Serial.println("Host Gmaster conected");
#endif
                    byte ID_Temp[8];
                    ID_Temp[0] = INFO[0];
                    ID_Temp[1] = INFO[1];
                    ID_Temp[2] = INFO[2];
                    ID_Temp[3] = INFO[3];
                    ID_Temp[4] = INFO[4];
                    ID_Temp[5] = INFO[5];
                    ID_Temp[6] = INFO[6];
                    ID_Temp[7] = INFO[7];


                    /* Inicia Player Tracking Sesion */
                    if (contadores.Set_Client_ID(ID_Temp))
                    {
                       
                        New_Timer_Final = New_Timmer_Inicial; /*RESET TIMEOUT*/
                        /* Guarda  Informacion cliente en memoria SD */
                        Update_Status_SD(); /* Actualiza Estado de Almacenamiento */
                        Storage_Cliente(Archivo_CSV_Sesiones, Variables_globales.Get_Variable_Global(Enable_Storage),contadores.Get_Client_ID());
                        /*  Maquina en juego  y Sesion Activa */
                        Variables_globales.Set_Variable_Global(Flag_Sesion_RFID, true);
                        Variables_globales.Set_Variable_Global(Flag_Contadores_Sesion_ON, true);
                       // Variables_globales.Set_Variable_Global(Flag_Maquina_En_Juego, true);
                        /* -Notificacion de inicio de sesion de juego-*/
                        Status_Barra(SESION_INICIADA);
                    }
                    else
                    {
                        Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                        Variables_globales.Set_Variable_Global(Flag_Sesion_RFID, false);
                        Status_Barra(ERROR_LECTURA);
                    }
                    delay(100); 
                    Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                }
                else
                {
                    Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                    Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                    #ifdef DEBUG_RFID
                    Serial.println("Host Gmaster disconected");
                    #endif
                    Status_Barra(CONEXION_TO_HOTS_FAILED);
                    delay(100);
                }  
            }
        }
        else
        {

            int contador = 0;
            for (int i = 0; i < 8; i++)
            {
                if (INFO[i] == contadores.Get_Client_ID()[i])
                {
                    contador++;
                }
            }
            if (contador >= 8) /* Cierra Sesion por Usuario*/
            {
                if (Close_Sesion_Player_Tracking())
                {
                    /*  Cierra Sesion de juego por cliente */
                    delay(10);
                    contador = 0;
                    Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                }
                else
                {
                    Status_Barra(ERROR_LECTURA);
                    delay(100);
                    contador = 0;
                    Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                }
            }
            else
            {
                Status_Barra(LECTURA_OK);
                unsigned long Respuesta_Server = millis();
                int TIMEOUT_CONECT_SERVER = 6500;
                while (!Variables_globales.Get_Variable_Global(Conexion_To_Host) && millis() - Respuesta_Server < TIMEOUT_CONECT_SERVER)
                {
#ifdef DEBUG_RFID
                    Serial.println("Verificando Conexion to Host...");
#endif
                    vTaskDelay(300);
                }
                if (Variables_globales.Get_Variable_Global(Conexion_To_Host))
                {
/* Inicia Player Tracking  */
#ifdef DEBUG_RFID
                    Serial.println("Host Gmaster conected");
#endif
                    byte ID_Temp[8];
                    ID_Temp[0] = INFO[0];
                    ID_Temp[1] = INFO[1];
                    ID_Temp[2] = INFO[2];
                    ID_Temp[3] = INFO[3];
                    ID_Temp[4] = INFO[4];
                    ID_Temp[5] = INFO[5];
                    ID_Temp[6] = INFO[6];
                    ID_Temp[7] = INFO[7];

                    /* Inicia Player Tracking Sesion */
                    if (contadores.Set_Client_ID(ID_Temp))
                    {

                        New_Timer_Final = New_Timmer_Inicial; /*RESET TIMEOUT*/
                        /* Guarda  Informacion cliente en memoria SD */
                        Update_Status_SD(); /* Actualiza Estado de Almacenamiento */
                        Storage_Cliente(Archivo_CSV_Sesiones, Variables_globales.Get_Variable_Global(Enable_Storage), contadores.Get_Client_ID());
                        /*  Maquina en juego  y Sesion Activa */
                        Variables_globales.Set_Variable_Global(Flag_Sesion_RFID, true);
                        Variables_globales.Set_Variable_Global(Flag_Contadores_Sesion_ON, true);
                        // Variables_globales.Set_Variable_Global(Flag_Maquina_En_Juego, true);
                        /* -Notificacion de inicio de sesion de juego- */
                        Status_Barra(SESION_INICIADA);
                    }
                    else
                    {
                        Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                        Variables_globales.Set_Variable_Global(Flag_Sesion_RFID, false);
                        Status_Barra(ERROR_LECTURA);
                    }
                    delay(100);
                    Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                }
                else
                {
                    Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                    Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                    Serial.println("Host Gmaster disconected");
#endif
                    Status_Barra(CONEXION_TO_HOTS_FAILED);
                    delay(100);
                }
            }
            // /* ---------------------------->Clashless<--------------------------------------------------------------------*/
            // Status_Barra(LECTURA_OK);
            // unsigned long Respuesta_Server = millis();
            // int TIMEOUT_CONECT_SERVER = 3000;
            // while (!Variables_globales.Get_Variable_Global(Conexion_To_Host) && millis() - Respuesta_Server < TIMEOUT_CONECT_SERVER)
            // {
            //     #ifdef DEBUG_RFID
            //     Serial.println("Verificando Conexion to Host...");
            //     #endif
            //     vTaskDelay(300);
            // }

            // if (Variables_globales.Get_Variable_Global(Conexion_To_Host))
            // {
            //     byte ID_Temp_Cashless[8];
            //     ID_Temp_Cashless[0] = INFO[0];
            //     ID_Temp_Cashless[1] = INFO[1];
            //     ID_Temp_Cashless[2] = INFO[2];
            //     ID_Temp_Cashless[3] = INFO[3];
            //     ID_Temp_Cashless[4] = INFO[4];
            //     ID_Temp_Cashless[5] = INFO[5];
            //     ID_Temp_Cashless[6] = INFO[6];
            //     ID_Temp_Cashless[7] = INFO[7];

            //     if(contadores.Set_ID_Cliente_Temp(ID_Temp_Cashless))
            //     {
            //         Variables_globales.Set_Variable_Global(Consulta_Info_Cliente, true);
            //     }
            // }else{
            //     Status_Barra(CONEXION_TO_HOTS_FAILED);
            //     delay(100);
            //     Variables_globales.Set_Variable_Global(Handle_RFID_Lector,false);
            // }
            // Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
        }
    }else{
        Status_Barra(ERROR_LECTURA);
        Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
    }
}
#endif
/*-----------------------------------------------------------Descoment to reset */

bool Verify_Current_Credit_Cashless(char Buffer_Current_Credit[])
{
    Creditos_Machine();
    delay(350);
    int Creditos_Actuales_Maquina = contadores.Get_Contadores_Int(24);
    Creditos_Machine();
    delay(350);
    Creditos_Actuales_Maquina = contadores.Get_Contadores_Int(24);

    if(Creditos_Actuales_Maquina>10)
        return true; /* No puede Cerrar Sesion Usuario normal */
    else
        return false;
}

void Report_Http_Code(int Code_Http, String Msg, bool Status)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        int Code;

        int httpCode;
        bool Status_Code;
        char IP_Server[4];
        char Current_IP[4];

        memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));
        memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));
        std::string Ip = IP_toString_(IP_Server);
        String Ip_Server = String(Ip.c_str());
        std::string Ip_Local = IP_toString_(Current_IP);
        String Ip_Local_Device = String(Ip_Local.c_str());
        String Puerto = "9595";

        StaticJsonDocument<500> jsonDocument;

        jsonDocument["IsSuccess"] = Status;
        jsonDocument["Message"] = Msg + " " + String(Code_Http);
        jsonDocument["Ip"] = Ip_Local_Device;

        String Json;
        serializeJson(jsonDocument, Json); /* Serializa Data */

        // String fwurl = "http://"+Ip_Server+":"+Puerto+"/api/Cashless/MensajeMonitor?"+"ip="+Ip_Local_Device+"&"+"mensaje="+Mensaje+"&"+"exitoso="+Status;
        String fwurl = "http://" + Ip_Server + ":" + Puerto + "/api/Cashless/MensajeMonitor";
        // Serial.println(fwurl);
        // http://192.168.5.180:9595/api/Cashless/MensajeMonitor?ip=192.168.5.180&mensaje=mensaje de pruebas&exitoso=False

        WiFiClient client;
        HTTPClient https;
        https.setTimeout(5000);

        if (https.begin(client, fwurl))
        {
            https.addHeader("Content-Type", "application/json");
            httpCode = https.POST(Json);
            //  Serial.println(httpCode);
            https.end();
        }
    }
}

void Cliente_VS_Operador(byte MEMORIA[],byte INFO[])
{

    if (MEMORIA[0] == 'O')
    {
        /* Bloquea lector */
        Info_Cashless.Lock_Reader();

        char ID_Temp_[8];
        ID_Temp_[0] = INFO[0];
        ID_Temp_[1] = INFO[1];
        ID_Temp_[2] = INFO[2];
        ID_Temp_[3] = INFO[3];
        ID_Temp_[4] = INFO[4];
        ID_Temp_[5] = INFO[5];
        ID_Temp_[6] = INFO[6];
        ID_Temp_[7] = INFO[7];

        

        /* Maquina Cashless */
        if (Variables_globales.Get_Variable_Global(Enable_Cashless) && Configuracion.Get_Configuracion(Tipo_Maquina, 0) < 4||Variables_globales.Get_Variable_Global(Enable_Cashless) && Configuracion.Get_Configuracion(Tipo_Maquina, 0)==17)
        {

            contadores.Close_ID_Operador();           /* Borra ID operador anterior */
            contadores.Dele_Operador_INFO_Operador(); /* ID*/
            Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO, false);
            Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO, true); /* Inicia timmer */
            New_Timer_Final = New_Timmer_Inicial;
            startTime = currentTime; /* Reset TimeOut_Player Tracking */

            char ID_Temp_[8];
          
            ID_Temp_[0] = INFO[0];
            ID_Temp_[1] = INFO[1];
            ID_Temp_[2] = INFO[2];
            ID_Temp_[3] = INFO[3];
            ID_Temp_[4] = INFO[4];
            ID_Temp_[5] = INFO[5];
            ID_Temp_[6] = INFO[6];
            ID_Temp_[7] = INFO[7];
            
            Status_Barra(TARJETA_OPERADOR_INSERT); /* Notifica tarjeta operador */

            Status_Barra(301); /*Notificacion de espera...*/
            int Client_Id_Int=contadores.Get_Client_ID_Int_(INFO);
            if (Info_Cashless.Await_Conexion(Client_Id_Int, 'O'))
            {
                Info_Cashless.Log(RTC, "CONSULTA_OPERADOR_FIDELIZACION-" + String(Client_Id_Int), "True");
                if (contadores.Set_Operador_ID_Temp(ID_Temp_))
                {
                    Condicion_Cumpl = false; /* Reset Timeout*/

                    contadores.Copy_Operator_In_();
                    Variables_globales.Set_Variable_Global(Operador_Detected, true);
                    Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                    Update_Status_SD();
                    Storage_Premios_OP(Archivo_CSV_Premios, Variables_globales.Get_Variable_Global(Enable_Storage), contadores.Get_Operador_ID());
                    if(Info_Cashless.Type_Sesion() != PLAYER_CASHLESS_SESION)
                        Info_Cashless.Unlock_Reader();
                }
                else
                {
                    Variables_globales.Set_Variable_Global(Operador_Detected, false);
                    contadores.Close_ID_Operador(); /*Temporal y en Trama*/
                    Status_Barra(ERROR_LECTURA);
                    Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                    Info_Cashless.Unlock_Reader();
                }
            }
            else
            {
                Variables_globales.Set_Variable_Global(Operador_Detected, false);
                Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                contadores.Close_ID_Operador(); /*Temporal y en Trama*/
#ifdef DEBUG_RFID
                Serial.println("Host Gmaster disconected");
#endif
                Status_Barra(CONEXION_TO_HOTS_FAILED);
                delay(100);
                Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO, false); /* Inicia timmer */
                Info_Cashless.Unlock_Reader();
                Info_Cashless.Log(RTC, "CONSULTA_OPERADOR_FIDELIZACION-" + String(Client_Id_Int), "False");
            }

        }
        else
        {

            
            contadores.Close_ID_Operador();           /* Borra ID operador anterior */
            contadores.Dele_Operador_INFO_Operador(); /* ID*/
            Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO, false);
            Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO, true); /* Inicia timmer */
            New_Timer_Final = New_Timmer_Inicial;
            startTime = currentTime; /* Reset TimeOut_Player Tracking */

            char ID_Temp_[8];
            ID_Temp_[0] = INFO[0];
            ID_Temp_[1] = INFO[1];
            ID_Temp_[2] = INFO[2];
            ID_Temp_[3] = INFO[3];
            ID_Temp_[4] = INFO[4];
            ID_Temp_[5] = INFO[5];
            ID_Temp_[6] = INFO[6];
            ID_Temp_[7] = INFO[7];

            // for(int i=0; i<8; i++)
            // {
            //     Serial.println(INFO[i]);
            // }
           
            Status_Barra(TARJETA_OPERADOR_INSERT); /* Notifica tarjeta operador */

            Status_Barra(301); /*Notificacion de espera...*/
            int Client_Id_Int=contadores.Get_Client_ID_Int_(INFO);
            // Serial.println(Client_Id_Int);
            if (Info_Cashless.Await_Conexion(Client_Id_Int, 'O'))
            {
                Info_Cashless.Log(RTC, "CONSULTA_OPERADOR_FIDELIZACION-" + String(Client_Id_Int), "True");
                if (contadores.Set_Operador_ID_Temp(ID_Temp_))
                {
                    Condicion_Cumpl = false; /* Reset Timeout*/

                    contadores.Copy_Operator_In_();
                    Variables_globales.Set_Variable_Global(Operador_Detected, true);
                    Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                    Update_Status_SD();
                    Storage_Premios_OP(Archivo_CSV_Premios, Variables_globales.Get_Variable_Global(Enable_Storage), contadores.Get_Operador_ID());
                    Info_Cashless.Unlock_Reader();
                }
                else
                {
                    Variables_globales.Set_Variable_Global(Operador_Detected, false);
                    contadores.Close_ID_Operador(); /*Temporal y en Trama*/
                    Status_Barra(ERROR_LECTURA);
                    Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                    Info_Cashless.Unlock_Reader();
                    Reset_Handle_LED();
                }
            }
            else
            {
                Variables_globales.Set_Variable_Global(Operador_Detected, false);
                Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                contadores.Close_ID_Operador(); /*Temporal y en Trama*/
#ifdef DEBUG_RFID
                Serial.println("Host Gmaster disconected");
#endif
                Status_Barra(CONEXION_TO_HOTS_FAILED);
                delay(100);
                Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO, false); /* Inicia timmer */
                Info_Cashless.Unlock_Reader();
                Info_Cashless.Log(RTC, "CONSULTA_OPERADOR_FIDELIZACION-" + String(Client_Id_Int), "False");
            }
        }
        // Info_Cashless.Lock_Reader();
    }

    else if (MEMORIA[0] == 'C')
    {
        /* Bloqueo lector */
        Info_Cashless.Lock_Reader();
            
        New_Timer_Final = New_Timmer_Inicial;
        startTime = currentTime;
        contadores.Dele_Operador_INFO_Operador(); /* ID*/
        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
        
        Status_Barra(LECTURA_OK); /* Lectura OK */

        Status_Barra(301); /*Notificacion de espera...*/
       
        if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) < 4 ||Configuracion.Get_Configuracion(Tipo_Maquina, 0)==17) /* Maquinas Cashless */
        {

            int contador = 0;
            for (int i = 0; i < 8; i++)
            {
                if (INFO[i] == contadores.Get_Client_ID()[i])
                {
                    contador++;

                }
            }

            if (contador >= 8) /* Cierra Sesion por Usuario*/
            {
                bool Handle=false;
                
                if (Variables_globales.Get_Variable_Global(Enable_Cashless))
                {
                    /* Reset Timeout Player Tracking y Cashless Auto*/
                    condicionCumplida=false;
                    /* Pregunta si no existe transacciones en maquina  &&  La Maq EFT no esta en condicion de pago */
                    if (Info_Cashless.Get_Status_Transfer() == TRANSFER_IDLE && !Info_Cashless.Get_Status_Handpay_EFT())
                    {
                        int ClientID = contadores.Get_Client_ID_Transaccion_Int();
                        switch (Info_Cashless.Type_Sesion())
                        {
                        case PLAYER_TRACKING_SESION: /*  Solicitud manual de cierre  Sesion Player Tracking  */

                            // if (contadores.Get_Client_ID_Int() > 0 && contadores.Get_Client_ID_Transaccion_Int() > 0 && Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
                            // {
                            //     switch (Info_Cashless.Info_Client_Download(contadores.Get_Client_ID_Transaccion(), RTC, "D", Cashless.Get_Trans_ID_Int()))
                            //     {
                            //     case REQUEST_SUCCESSFULLY_RECEIVED:
                            //         Solicitud_Descarga_Cashless();
                            //         break;

                            //     case NOT_CONEXION_WITH_SERVER:
                            //         Status_Barra(ERROR_LECTURA);
                            //         Info_Cashless.Unlock_Reader(); /* Habilita lector */
                            //         Handle = true;
                            //         Reset_Handle_LED();
                            //         break;

                            //     case PROBLEM_WITH_THE_SERVER:
                            //         Status_Barra(ERROR_LECTURA);
                            //         Info_Cashless.Unlock_Reader(); /* Habilita lector */
                            //         Handle = true;
                            //         Reset_Handle_LED();
                            //         break;

                            //     case HTTP_CODE_UNAUTHORIZED:
                            //         Handle = true;
                            //         Status_Barra(ERROR_LECTURA);
                            //         Info_Cashless.Unlock_Reader(); /* Habilita lector */
                            //         Reset_Handle_LED();
                            //         Report_Http_Code(HTTP_CODE_UNAUTHORIZED, "Token de autenticacion no valido Codigo HTTP");
                            //         break;

                            //     default:
                            //         Status_Barra(ERROR_LECTURA);
                            //         Info_Cashless.Unlock_Reader(); /* Habilita lector */
                            //         Handle = true;
                            //         Reset_Handle_LED();
                            //         break;
                            //     }
                            // }
                            // else
                            // {

                            Info_Cashless.Close_Player_Tracking_Sesion(true);
                            Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
                            contadores.Close_ID_Client_Transaccion();
                            Info_Cashless.Unlock_Reader(); /* Habilita lector */
                            Handle = true;
                            Report_Http_Code(TERMINA_SESION_MANUAL, "Sesion fidelizacion terminada por Usuario: ", true);
                            // }
                            DisplayTFT.Cierra_Sesion_Player_Tracking_TFT_Globus_IM();
                            break;

                        case PLAYER_CASHLESS_SESION: /* Solicitud de cierre Sesion Player Cashless */
                            /* Descarga maquina*/

                            switch (Info_Cashless.Info_Client_Download(contadores.Get_Client_ID_Transaccion(), RTC, "D", Cashless.Get_Trans_ID_Int(), ClientID))
                            {
                            case REQUEST_SUCCESSFULLY_RECEIVED:
                                Info_Cashless.Log(RTC, "ENVIA_TRANSACCION_CASHLESS_DESCARGA_MAQUINA");
                                Objeto_Transfer_Download["Operacion"]="Solicitud Descarga por Usuario";
                                Solicitud_Descarga_Cashless();
                                Menssage_TFT("Comprobando saldo\nPor favor espere...",DisplayTFT.configtft.timeoutMensajesCONFIG,true);
                                break;

                            case NOT_CONEXION_WITH_SERVER:
                                Info_Cashless.Log(RTC, "ERROR_EN_SOLICITUD_DESCARGA");
                                Status_Barra(ERROR_LECTURA);
                                Info_Cashless.Unlock_Reader(); /* Habilita lector */
                                Handle = true;
                                Reset_Handle_LED();
                                Menssage_TFT("Sin conexión con el servidor\nIntente nuevamente...",DisplayTFT.configtft.timeoutMensajesCONFIG,true);
                                break;

                            case PROBLEM_WITH_THE_SERVER:
                                Info_Cashless.Log(RTC, "ERROR_EN_SOLICITUD_DE_DESCARGA");
                                Status_Barra(ERROR_LECTURA);
                                Info_Cashless.Unlock_Reader(); /* Habilita lector */
                                Handle = true;
                                Reset_Handle_LED();
                                break;

                            case HTTP_CODE_UNAUTHORIZED:
                                Info_Cashless.Log(RTC, "TOKEN_NO_AUTORIZADO");
                                Handle = true;
                                Status_Barra(ERROR_LECTURA);
                                Info_Cashless.Unlock_Reader(); /* Habilita lector */
                                Reset_Handle_LED();
                                Report_Http_Code(HTTP_CODE_UNAUTHORIZED, "Token de autenticacion no valido Codigo HTTP");
                                break;

                            default:
                                Info_Cashless.Log(RTC, "ERROR_SOLICITUD_NO_IDENTIFICADO");
                                Status_Barra(ERROR_LECTURA);
                                Info_Cashless.Unlock_Reader(); /* Habilita lector */
                                Handle = true;
                                Reset_Handle_LED();
                                break;
                            }
                            break;

                        default: /*  Solicitud manual de cierre  Sesion Player Tracking  */
                            Status_Barra(ERROR_LECTURA);
                            Info_Cashless.Unlock_Reader(); /* Habilita lector */
                            Reset_Handle_LED();
                            break;
                        }
                    }
                    else
                    {

                        if (Info_Cashless.Get_Status_Transfer() != TRANSFER_IDLE)
                            Report_Http_Code(TRANSFER_PENDING, " La tarjeta esta en proceso de descarga ");
                        if (Info_Cashless.Get_Status_Handpay_EFT())
                            Report_Http_Code(DESCARGA_EFT_BLOQUEADA, "Maquina en condicion de pago no puede realizar descarga EFT:");

                        Status_Barra(302);
                        Info_Cashless.Unlock_Reader();
                    }
                }else{
                    Info_Cashless.Close_Player_Tracking_Sesion(true);
                    Info_Cashless.Unlock_Reader();  /* Habilita lector */
                    //Close_Player_TFT(0,0,0,1);
                    DisplayTFT.Cierra_Sesion_Player_Tracking_TFT_Globus_IM();

                    Info_Cashless.Log(RTC, "CIERRE_MANUAL_SESION_POR_USUARIO");
                }
               // Info_Cashless.Unlock_Reader();  /* Habilita lector */
            }
            else
            {
                bool Handle=false;

                byte ID_Temp[8];
                ID_Temp[0] = INFO[0];
                ID_Temp[1] = INFO[1];
                ID_Temp[2] = INFO[2];
                ID_Temp[3] = INFO[3];
                ID_Temp[4] = INFO[4];
                ID_Temp[5] = INFO[5];
                ID_Temp[6] = INFO[6];
                ID_Temp[7] = INFO[7];

                if (Variables_globales.Get_Variable_Global(Enable_Cashless))
                {

                    condicionCumplida=false;

                    /* Logica: Si Existe una  Sesion Player Cashless Activa anterior y un usuario  se identifica
                    Se verifica si los creditos son <10. Si es asi, Cierra la sesión anterior espera hasta completar y inicia una lasesion del cliente nuevo.
                    Por otro lado, si la sesión anterior tiene creditos  >=10 Permite el cierre solo por Operador. */

                    bool Error=false;
                    unsigned long Timout_Break;
                    int Stop_Transaccion = 6500; // 3500


                   // bool Error=false;
                    unsigned long Timout_Break_Controller;
                    int Stop_Transaccion_Controller = 6500; // 3500

                    if (Info_Cashless.Type_Sesion() == PLAYER_CASHLESS_SESION)
                    {
                        if (Info_Cashless.Get_Status_Transfer() == TRANSFER_IDLE && !Info_Cashless.Get_Status_Handpay_EFT())
                        {
                            if (!Verify_Current_Credit_Cashless(Buffer_Cashless.Get_RX_AFT(Buffer_RX_Cashless)))
                            {
                                int ClientID = contadores.Get_Client_ID_Transaccion_Int();
                                switch (Info_Cashless.Info_Client_Download(contadores.Get_Client_ID_Transaccion(), RTC, "D", Cashless.Get_Trans_ID_Int(), ClientID))
                                {
                                case REQUEST_SUCCESSFULLY_RECEIVED:
                                    Error = true;
                                    Info_Cashless.Log(RTC, "SOLICITUD_CIERRE_USUARIO_ANTERIOR");
                                    Objeto_Transfer_Download["Operacion"] = "Solicitud Descarga por Usuario anterior";
                                    Info_Cashless.Log(RTC, "ENVIA_TRANSACCION_CASHLESS_DESCARGA_MAQUINA");
                                    Solicitud_Descarga_Cashless();

                                    break;

                                case HTTP_CODE_UNAUTHORIZED:
                                    Info_Cashless.Log(RTC, "TOKEN_NO_AUTORIZADO");
                                    Error = true;
                                    Status_Barra(ERROR_LECTURA);
                                    Handle = true;
                                    Reset_Handle_LED();
                                    Report_Http_Code(HTTP_CODE_UNAUTHORIZED, "Token de autenticacion no valido Codigo HTTP");
                                    break;

                                default:

                                    Info_Cashless.Log(RTC, "ERROR_EN_SOLICITUD_NO_IDENTIFICADO");
                                    Error = true;
                                    Status_Barra(ERROR_LECTURA);
                                    Handle = true;
                                    Reset_Handle_LED();
                                    break;
                                }
                            }
                            else
                            {
                                Error = true;
                                Status_Barra(ERROR_LECTURA);
                                Reset_Handle_LED();
                                Handle = true;
                            }
                        }
                        else
                        {
                            if (Info_Cashless.Get_Status_Transfer() != TRANSFER_IDLE)
                                Report_Http_Code(TRANSFER_PENDING, " La tarjeta esta en proceso de descarga ");
                            if (Info_Cashless.Get_Status_Handpay_EFT())
                                Report_Http_Code(DESCARGA_EFT_BLOQUEADA, "Maquina en condicion de pago no puede realizar descarga EFT:");

                            Status_Barra(302);
                            Handle = true;
                        }

                        if (Handle)
                            Info_Cashless.Unlock_Reader();
                    }

                    if (!Error)
                    {

                        Error = false;
                        
                        /* Si no Existen transacciones pendientes por confirmar y  la maquina no tiene transacciones pendientes tramita transaccion */
                        Variables_globales.Set_Variable_Global(Hand_pay_is_pending, false);

                        if (transaccionesPendientes.empty() && !Variables_globales.Get_Variable_Global(Event_Dowmload_Cashless_Pending) && !Variables_globales.Get_Variable_Global(Event_Load_Cashless_Pending))
                        {
                            bool Handl = true;
                            char ID_Temp_Tarjeta[8];
                            /*  Abre nueva Sesion */
                            contadores.Set_Client_ID_Transaccion(ID_Temp); /* Guarda ID de cliente */


                            if(contadores.Get_Client_ID_Transaccion_Int()<=0)
                            {
                                Report_Http_Code(INVALIDE_CLIENT,"Error procesando id de cliente en tarjeta: "+String(contadores.Get_Client_ID_Transaccion_Int()),false);
                                Info_Cashless.Unlock_Reader();
                                Status_Barra(CONEXION_TO_HOTS_FAILED);
                                return;
                            }    
                           
                            switch (Info_Cashless.Info_Client(contadores.Get_Client_ID_Transaccion(), RTC, LOAD_TRANSACTION, Cashless.Get_Trans_ID_Int()))
                            {
                            case REQUEST_SUCCESSFULLY_RECEIVED:
                                Info_Cashless.Log(RTC,"ENVIA_TRANSACCION_CASHLESS_CARGA_MAQUINA");
                                Solicitud_Carga_Cashless();
                                Menssage_TFT("Comprobando saldo\nPor favor espere...",DisplayTFT.configtft.timeoutMensajesCONFIG,false);
                                break;

                            case INSUFFICIENT_BALANCE:
                                Info_Cashless.Log(RTC,"ENVIA_TRANSACCION_CASHLESS_CARGA_MAQUINA");
                                Solicitud_Carga_Cashless(); /* Carga en 0 */
                                break;

                            case TRANS_ID_NO_MACTH:
                                Objeto_Transfer["IsSuccess"] = false;
                                Objeto_Transfer["Trans_Estado"] = TRANS_ID_NO_MACTH;
                                hayTransaccionesPendientes = true;
                               // Status_Barra(ERROR_LECTURA);
                                Handle = true;
                               // Reset_Handle_LED();
                               
                                ID_Temp_Tarjeta[0] = INFO[0];
                                ID_Temp_Tarjeta[1] = INFO[1];
                                ID_Temp_Tarjeta[2] = INFO[2];
                                ID_Temp_Tarjeta[3] = INFO[3];
                                ID_Temp_Tarjeta[4] = INFO[4];
                                ID_Temp_Tarjeta[5] = INFO[5];
                                ID_Temp_Tarjeta[6] = INFO[6];
                                ID_Temp_Tarjeta[7] = INFO[7];

                                if (Variables_globales.Get_Variable_Global(Enable_Fidelizacion_After_Cashless_Fail))
                                {
                                    if (Info_Cashless.Await_Conexion(contadores.Get_Client_ID_Int_(INFO), 'C'))
                                    {
                                        Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
                                        // Info_Cashless.Saves_Current_Player_Sesion(INFO,PLAYER_TRACKING_SESION);
                                        Info_Cashless.Init_Player_Tracking_Sesion(ID_Temp);
                                        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                                        Info_Cashless.Unlock_Reader();
                                        Report_Http_Code(TRANS_ID_NO_MACTH, "Inicia Player tracking numero de transaccion de solicitud diferente al recibido", false);
                                        // contadores.Close_ID_Client_Transaccion();

                                        DisplayTFT.Actualiza_Puntos_TFT_Globus_IM(DisplayTFT.info.Usuario, DisplayTFT.info.Casino, NivelUsuarioInt(DisplayTFT.info.Nivel_Usuario), DisplayTFT.info.Total_Fide, DisplayTFT.info.Total_Bole);

                                        DisplayTFT.Actualiza_Menu_TFT_Globus_IM();
                                    }
                                    else
                                    {

                                        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                                        Serial.println("Host Gmaster disconected");
#endif
                                        Info_Cashless.Unlock_Reader();
                                        Status_Barra(CONEXION_TO_HOTS_FAILED);
                                        delay(100);
                                    }
                                }
                                else
                                {
                                    Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                                    Serial.println("Host Gmaster disconected");
#endif
                                    Info_Cashless.Unlock_Reader();
                                    Status_Barra(CONEXION_TO_HOTS_FAILED);
                                    delay(100);
                                }
                                break;

                            case CLIENT_NOT_MACTH:
                                Objeto_Transfer["IsSuccess"] = false;
                                Objeto_Transfer["Trans_Estado"] = CLIENT_NOT_MACTH;
                                hayTransaccionesPendientes = true;
                                //Status_Barra(ERROR_LECTURA);
                                Handle = true;
                               // Reset_Handle_LED();

                               

                                
                                ID_Temp_Tarjeta[0] = INFO[0];
                                ID_Temp_Tarjeta[1] = INFO[1];
                                ID_Temp_Tarjeta[2] = INFO[2];
                                ID_Temp_Tarjeta[3] = INFO[3];
                                ID_Temp_Tarjeta[4] = INFO[4];
                                ID_Temp_Tarjeta[5] = INFO[5];
                                ID_Temp_Tarjeta[6] = INFO[6];
                                ID_Temp_Tarjeta[7] = INFO[7];

                                if (Variables_globales.Get_Variable_Global(Enable_Fidelizacion_After_Cashless_Fail))
                                {
                                    if (Info_Cashless.Await_Conexion(contadores.Get_Client_ID_Int_(INFO), 'C'))
                                    {

                                        Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
                                        // Info_Cashless.Saves_Current_Player_Sesion(INFO,PLAYER_TRACKING_SESION);
                                        Info_Cashless.Init_Player_Tracking_Sesion(ID_Temp);
                                        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                                        Info_Cashless.Unlock_Reader();
                                        Report_Http_Code(CLIENT_NOT_MACTH, "Inicia Player tracking cliente de solicitud  diferente al recibido", false);
                                        // contadores.Close_ID_Client_Transaccion();
                                        DisplayTFT.Actualiza_Puntos_TFT_Globus_IM(DisplayTFT.info.Usuario,DisplayTFT.info.Casino,NivelUsuarioInt(DisplayTFT.info.Nivel_Usuario),DisplayTFT.info.Total_Fide,DisplayTFT.info.Total_Bole);
                                        DisplayTFT.Actualiza_Menu_TFT_Globus_IM();
                                    }
                                    else
                                    {

                                        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                                        Serial.println("Host Gmaster disconected");
#endif
                                        Info_Cashless.Unlock_Reader();
                                        Status_Barra(CONEXION_TO_HOTS_FAILED);
                                        delay(100);
                                    }
                                }
                                else
                                {
                                    Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                                    Serial.println("Host Gmaster disconected");
#endif
                                    Info_Cashless.Unlock_Reader();
                                    Status_Barra(CONEXION_TO_HOTS_FAILED);
                                    delay(100);
                                }

                                break;

                            case INVALID_BALANCE:
                                Objeto_Transfer["IsSuccess"] = false;
                                Objeto_Transfer["Trans_Estado"] = INVALID_BALANCE;
                                hayTransaccionesPendientes = true;
                                // Variables_globales.Set_Variable_Global(Flag_Sesion_Cashless, true);
                                // Info_Cashless.Init_Player_Tracking_Sesion(contadores.Get_Client_ID_Transaccion());
                               // Status_Barra(ERROR_LECTURA);
                                Handle = true;
                                //  Reset_Handle_LED();

                               

                                ID_Temp_Tarjeta[0] = INFO[0];
                                ID_Temp_Tarjeta[1] = INFO[1];
                                ID_Temp_Tarjeta[2] = INFO[2];
                                ID_Temp_Tarjeta[3] = INFO[3];
                                ID_Temp_Tarjeta[4] = INFO[4];
                                ID_Temp_Tarjeta[5] = INFO[5];
                                ID_Temp_Tarjeta[6] = INFO[6];
                                ID_Temp_Tarjeta[7] = INFO[7];

                                if (Variables_globales.Get_Variable_Global(Enable_Fidelizacion_After_Cashless_Fail))
                                {
                                    if (Info_Cashless.Await_Conexion(contadores.Get_Client_ID_Int_(INFO), 'C'))
                                    {
                                        Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
                                        // Info_Cashless.Saves_Current_Player_Sesion(INFO,PLAYER_TRACKING_SESION);
                                        Info_Cashless.Init_Player_Tracking_Sesion(ID_Temp);
                                        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                                        Info_Cashless.Unlock_Reader();
                                        Report_Http_Code(INVALID_BALANCE, "Inicia Player tracking El monto de transaccion no es un  entero de 32bits", false);
                                        // contadores.Close_ID_Client_Transaccion();
                                        DisplayTFT.Actualiza_Puntos_TFT_Globus_IM(DisplayTFT.info.Usuario,DisplayTFT.info.Casino,NivelUsuarioInt(DisplayTFT.info.Nivel_Usuario),DisplayTFT.info.Total_Fide,DisplayTFT.info.Total_Bole);

                                        DisplayTFT.Actualiza_Menu_TFT_Globus_IM();
                                    }
                                    else
                                    {

                                        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                                        Serial.println("Host Gmaster disconected");
#endif
                                        Info_Cashless.Unlock_Reader();
                                        Status_Barra(CONEXION_TO_HOTS_FAILED);
                                        delay(100);
                                    }
                                }
                                else
                                {
                                    Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                                    Serial.println("Host Gmaster disconected");
#endif
                                    Info_Cashless.Unlock_Reader();
                                    Status_Barra(CONEXION_TO_HOTS_FAILED);
                                    delay(100);
                                }

                                break;

                            case TYPE_TRANS_NOT_MACTH:
                                Objeto_Transfer["IsSuccess"] = false;
                                Objeto_Transfer["Trans_Estado"] = TYPE_TRANS_NOT_MACTH;
                                hayTransaccionesPendientes = true;
                                Handle = true;
                               // Status_Barra(ERROR_LECTURA);
                               // Reset_Handle_LED();

                               

                                ID_Temp_Tarjeta[0] = INFO[0];
                                ID_Temp_Tarjeta[1] = INFO[1];
                                ID_Temp_Tarjeta[2] = INFO[2];
                                ID_Temp_Tarjeta[3] = INFO[3];
                                ID_Temp_Tarjeta[4] = INFO[4];
                                ID_Temp_Tarjeta[5] = INFO[5];
                                ID_Temp_Tarjeta[6] = INFO[6];
                                ID_Temp_Tarjeta[7] = INFO[7];

                                if (Variables_globales.Get_Variable_Global(Enable_Fidelizacion_After_Cashless_Fail))
                                {
                                    if (Info_Cashless.Await_Conexion(contadores.Get_Client_ID_Int_(INFO), 'C'))
                                    {
                                        Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
                                        // Info_Cashless.Saves_Current_Player_Sesion(INFO,PLAYER_TRACKING_SESION);
                                        Info_Cashless.Init_Player_Tracking_Sesion(ID_Temp);
                                        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                                        Info_Cashless.Unlock_Reader();
                                        Report_Http_Code(TYPE_TRANS_NOT_MACTH, "Inicia Player tracking El tipo de transaccion de solicitud es diferente al recibido", false);
                                        // contadores.Close_ID_Client_Transaccion();
                                        DisplayTFT.Actualiza_Puntos_TFT_Globus_IM(DisplayTFT.info.Usuario,DisplayTFT.info.Casino,NivelUsuarioInt(DisplayTFT.info.Nivel_Usuario),DisplayTFT.info.Total_Fide,DisplayTFT.info.Total_Bole);

                                        DisplayTFT.Actualiza_Menu_TFT_Globus_IM();
                                    }
                                    else
                                    {

                                        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                                        Serial.println("Host Gmaster disconected");
#endif
                                        Info_Cashless.Unlock_Reader();
                                        Status_Barra(CONEXION_TO_HOTS_FAILED);
                                        delay(100);
                                    }
                                }
                                else
                                {
                                    Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                                    Serial.println("Host Gmaster disconected");
#endif
                                    Info_Cashless.Unlock_Reader();
                                    Status_Barra(CONEXION_TO_HOTS_FAILED);
                                    delay(100);
                                }

                                break;

                            case PROBLEM_WITH_THE_SERVER: /* CLIENTE BLOQUEADO  */
                                // Status_Barra(ERROR_LECTURA);
                                Handle = true;
                                // Reset_Handle_LED();

                                

                                ID_Temp_Tarjeta[0] = INFO[0];
                                ID_Temp_Tarjeta[1] = INFO[1];
                                ID_Temp_Tarjeta[2] = INFO[2];
                                ID_Temp_Tarjeta[3] = INFO[3];
                                ID_Temp_Tarjeta[4] = INFO[4];
                                ID_Temp_Tarjeta[5] = INFO[5];
                                ID_Temp_Tarjeta[6] = INFO[6];
                                ID_Temp_Tarjeta[7] = INFO[7];

                                if (Variables_globales.Get_Variable_Global(Enable_Fidelizacion_After_Cashless_Fail))
                                {
                                    if (Info_Cashless.Await_Conexion(contadores.Get_Client_ID_Int_(INFO), 'C'))
                                    {
                                        Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
                                        // Info_Cashless.Saves_Current_Player_Sesion(INFO,PLAYER_TRACKING_SESION);
                                        Info_Cashless.Init_Player_Tracking_Sesion(ID_Temp);
                                        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                                        Info_Cashless.Unlock_Reader();
                                        Report_Http_Code(PROBLEM_WITH_THE_SERVER, "Inicia Player tracking Error en procesamiento de solicitud ", false);
                                        // contadores.Close_ID_Client_Transaccion();
                                        DisplayTFT.Actualiza_Puntos_TFT_Globus_IM(DisplayTFT.info.Usuario,DisplayTFT.info.Casino,NivelUsuarioInt(DisplayTFT.info.Nivel_Usuario),DisplayTFT.info.Total_Fide,DisplayTFT.info.Total_Bole);
                                        DisplayTFT.Actualiza_Menu_TFT_Globus_IM();
                                    }
                                    else
                                    {

                                        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                                        Serial.println("Host Gmaster disconected");
#endif
                                        Info_Cashless.Unlock_Reader();
                                        Status_Barra(CONEXION_TO_HOTS_FAILED);
                                        delay(100);
                                    }
                                }
                                else
                                {
                                    Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                                    Serial.println("Host Gmaster disconected");
#endif
                                    Info_Cashless.Unlock_Reader();
                                    Status_Barra(CONEXION_TO_HOTS_FAILED);
                                    delay(100);
                                }

                                break;

                            case NOT_CONEXION_WITH_SERVER: /* NO CONEXION CON EL SERVIDOR */
                                Status_Barra(CONEXION_TO_HOTS_FAILED);
                                Handle = true;
                                Reset_Handle_LED();
                                Menssage_TFT("Sin conexion con el servidor\nIntente nuevamente...",DisplayTFT.configtft.timeoutMensajesCONFIG,true);
                                break;

                            case NOT_COMMUNICATION_WITH_THE_MACHINE:
                                Objeto_Transfer["IsSuccess"] = false;
                                Objeto_Transfer["Trans_Estado"] = NOT_COMMUNICATION_WITH_THE_MACHINE;
                                hayTransaccionesPendientes = true;
                                Status_Barra(CONEXION_TO_HOTS_FAILED);
                                Handle = true;
                                Reset_Handle_LED();
                                break;
                            case NOT_WIFI_CONNECTION:
                                Objeto_Transfer["IsSuccess"] = false;
                                Objeto_Transfer["Trans_Estado"] = NOT_WIFI_CONNECTION;
                                hayTransaccionesPendientes = true;
                                Status_Barra(CONEXION_TO_HOTS_FAILED);
                                Handle = true;
                                Reset_Handle_LED();
                                break;

                            case TYPE_MACHINE_NOT_MACTH:
                                Objeto_Transfer["IsSuccess"] = false;
                                Objeto_Transfer["Trans_Estado"] = TYPE_MACHINE_NOT_MACTH;
                                hayTransaccionesPendientes = true;
                               // Status_Barra(ERROR_LECTURA);
                                Handle = true;
                                // Reset_Handle_LED();

                                

                                ID_Temp_Tarjeta[0] = INFO[0];
                                ID_Temp_Tarjeta[1] = INFO[1];
                                ID_Temp_Tarjeta[2] = INFO[2];
                                ID_Temp_Tarjeta[3] = INFO[3];
                                ID_Temp_Tarjeta[4] = INFO[4];
                                ID_Temp_Tarjeta[5] = INFO[5];
                                ID_Temp_Tarjeta[6] = INFO[6];
                                ID_Temp_Tarjeta[7] = INFO[7];

                                if (Variables_globales.Get_Variable_Global(Enable_Fidelizacion_After_Cashless_Fail))
                                {
                                    if (Info_Cashless.Await_Conexion(contadores.Get_Client_ID_Int_(INFO), 'C'))
                                    {
                                        Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
                                        // Info_Cashless.Saves_Current_Player_Sesion(INFO,PLAYER_TRACKING_SESION);
                                        Info_Cashless.Init_Player_Tracking_Sesion(ID_Temp);
                                        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                                        Info_Cashless.Unlock_Reader();
                                        Report_Http_Code(TYPE_MACHINE_NOT_MACTH, "Inicia Player tracking El tipo de maquina de solicitud es diferente al recibido", false);
                                        // contadores.Close_ID_Client_Transaccion();
                                        DisplayTFT.Actualiza_Puntos_TFT_Globus_IM(DisplayTFT.info.Usuario,DisplayTFT.info.Casino,NivelUsuarioInt(DisplayTFT.info.Nivel_Usuario),DisplayTFT.info.Total_Fide,DisplayTFT.info.Total_Bole);
                                        DisplayTFT.Actualiza_Menu_TFT_Globus_IM();
                                    }
                                    else
                                    {

                                        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                                        Serial.println("Host Gmaster disconected");
#endif
                                        Info_Cashless.Unlock_Reader();
                                        Status_Barra(CONEXION_TO_HOTS_FAILED);
                                        delay(100);
                                    }
                                }
                                else
                                {
                                    Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                                    Serial.println("Host Gmaster disconected");
#endif
                                    Info_Cashless.Unlock_Reader();
                                    Status_Barra(CONEXION_TO_HOTS_FAILED);
                                    delay(100);
                                }

                                break;

                            case HTTP_CODE_UNAUTHORIZED:
                                Handle = true;
                                // Status_Barra(ERROR_LECTURA);
                                // Reset_Handle_LED();

                                
                                ID_Temp_Tarjeta[0] = INFO[0];
                                ID_Temp_Tarjeta[1] = INFO[1];
                                ID_Temp_Tarjeta[2] = INFO[2];
                                ID_Temp_Tarjeta[3] = INFO[3];
                                ID_Temp_Tarjeta[4] = INFO[4];
                                ID_Temp_Tarjeta[5] = INFO[5];
                                ID_Temp_Tarjeta[6] = INFO[6];
                                ID_Temp_Tarjeta[7] = INFO[7];

                                if (Variables_globales.Get_Variable_Global(Enable_Fidelizacion_After_Cashless_Fail))
                                {
                                    if (Info_Cashless.Await_Conexion(contadores.Get_Client_ID_Int_(INFO), 'C'))
                                    {
                                        Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);

                                        // Info_Cashless.Saves_Current_Player_Sesion(INFO,PLAYER_TRACKING_SESION);
                                        Info_Cashless.Init_Player_Tracking_Sesion(ID_Temp);
                                        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                                        Info_Cashless.Unlock_Reader();
                                        // contadores.Close_ID_Client_Transaccion();
                                        DisplayTFT.Actualiza_Puntos_TFT_Globus_IM(DisplayTFT.info.Usuario,DisplayTFT.info.Casino,NivelUsuarioInt(DisplayTFT.info.Nivel_Usuario),DisplayTFT.info.Total_Fide,DisplayTFT.info.Total_Bole);
                                        DisplayTFT.Actualiza_Menu_TFT_Globus_IM();


                                    }
                                    else
                                    {

                                        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                                        Serial.println("Host Gmaster disconected");
#endif
                                        Info_Cashless.Unlock_Reader();
                                        Status_Barra(CONEXION_TO_HOTS_FAILED);
                                        delay(100);
                                    }
                                }
                                else
                                {
                                    Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                                    Serial.println("Host Gmaster disconected");
#endif
                                    Info_Cashless.Unlock_Reader();
                                    Status_Barra(CONEXION_TO_HOTS_FAILED);
                                    delay(100);
                                }

                                Report_Http_Code(HTTP_CODE_UNAUTHORIZED,"Token de autenticacion no valido Codigo HTTP");
                                break;

                            default:
                                Objeto_Transfer["IsSuccess"] = false;
                                Objeto_Transfer["Trans_Estado"] = NOT_COMMUNICATION_WITH_THE_MACHINE;
                                hayTransaccionesPendientes = true;
                               // Status_Barra(ERROR_LECTURA);
                                Handle = true;
                                // Reset_Handle_LED();

                                if (Variables_globales.Get_Variable_Global(Enable_Fidelizacion_After_Cashless_Fail))
                                {
                                    if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                                    {

                                        ID_Temp_Tarjeta[0] = INFO[0];
                                        ID_Temp_Tarjeta[1] = INFO[1];
                                        ID_Temp_Tarjeta[2] = INFO[2];
                                        ID_Temp_Tarjeta[3] = INFO[3];
                                        ID_Temp_Tarjeta[4] = INFO[4];
                                        ID_Temp_Tarjeta[5] = INFO[5];
                                        ID_Temp_Tarjeta[6] = INFO[6];
                                        ID_Temp_Tarjeta[7] = INFO[7];

                                        if (Info_Cashless.Await_Conexion(contadores.Get_Client_ID_Int_(INFO), 'C'))
                                        {
                                            Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
                                            // Info_Cashless.Saves_Current_Player_Sesion(INFO,PLAYER_TRACKING_SESION);
                                            Info_Cashless.Init_Player_Tracking_Sesion(ID_Temp);
                                            Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                                            Info_Cashless.Unlock_Reader();
                                            Report_Http_Code(ERROR_NOT_IDENTIFY, "Inicia Player tracking Error no codificado", false);
                                            // contadores.Close_ID_Client_Transaccion();
                                            DisplayTFT.Actualiza_Puntos_TFT_Globus_IM(DisplayTFT.info.Usuario,DisplayTFT.info.Casino,NivelUsuarioInt(DisplayTFT.info.Nivel_Usuario),DisplayTFT.info.Total_Fide,DisplayTFT.info.Total_Bole);

                                            DisplayTFT.Actualiza_Menu_TFT_Globus_IM();
                                        }
                                        else
                                        {

                                            Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                                            Serial.println("Host Gmaster disconected");
#endif
                                            Info_Cashless.Unlock_Reader();
                                            Status_Barra(CONEXION_TO_HOTS_FAILED);
                                            delay(100);
                                        }
                                    }
                                }
                                else
                                {
                                    Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                                    Serial.println("Host Gmaster disconected");
#endif
                                    Info_Cashless.Unlock_Reader();
                                    Status_Barra(CONEXION_TO_HOTS_FAILED);
                                    delay(100);
                                }

                                break;
                            }
                            /* Si Existe un Ack error D Habilita lector */
                            if (Handle)
                            {
                                Info_Cashless.Unlock_Reader();
                                Handle = false;
                            }
                        }
                        else
                        {/*s*/
                            
                            if(transaccionesPendientes.empty())
                                Report_Http_Code(TRANSFER_PENDING," Transaccion pendiente por consulta en maquina ");
                            else{
                                /* Pendiente por Reportar transaccion */
                               // String  transaccion = transaccionesPendientes.front(); /* Toma la primera transferencia */
                                Report_Http_Code(TRANSFER_PENDING,"Rechazada por transaccion pendiente de recepcion");
                            }   
                            Status_Barra(302);
                            Handle = true;
                        }
                    }

                    if(Handle)
                    {
                        Info_Cashless.Unlock_Reader();
                        Handle=false;
                    } 
                }
                else
                {
                    bool Handl = true;
                    char ID_Temp_Tarjeta[8];
                    ID_Temp_Tarjeta[0] = INFO[0];
                    ID_Temp_Tarjeta[1] = INFO[1];
                    ID_Temp_Tarjeta[2] = INFO[2];
                    ID_Temp_Tarjeta[3] = INFO[3];
                    ID_Temp_Tarjeta[4] = INFO[4];
                    ID_Temp_Tarjeta[5] = INFO[5];
                    ID_Temp_Tarjeta[6] = INFO[6];
                    ID_Temp_Tarjeta[7] = INFO[7];

                    char User[9];
                    User[0] = INFO[0];
                    User[1] = INFO[1];
                    User[2] = INFO[2];
                    User[3] = INFO[3];
                    User[4] = INFO[4];
                    User[5] = INFO[5];
                    User[6] = INFO[6];
                    User[7] = INFO[7];
                    User[8] = '\0'; // Cierre del string

                    //int Client_Id_Int = atoi(User); // Conversión a entero
                    int Client_Id_Int=contadores.Get_Client_ID_Int_(INFO);
                    if (Info_Cashless.Await_Conexion(Client_Id_Int, 'C'))
                    {
                        
                        //Info_Cashless.Saves_Current_Player_Sesion(INFO,PLAYER_TRACKING_SESION);
                        Info_Cashless.Init_Player_Tracking_Sesion(ID_Temp);
                        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                        Info_Cashless.Unlock_Reader();

                        DisplayTFT.Actualiza_Puntos_TFT_Globus_IM(DisplayTFT.info.Usuario,DisplayTFT.info.Casino,NivelUsuarioInt(DisplayTFT.info.Nivel_Usuario),DisplayTFT.info.Total_Fide,DisplayTFT.info.Total_Bole);

                        Info_Cashless.Log(RTC,"CONSULTA_USUARIO_FIDELIZACION-"+String(Client_Id_Int),"True");
                    }
                    else
                    {

                        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                        Serial.println("Host Gmaster disconected");
#endif  
                        Info_Cashless.Unlock_Reader();
                        
                        Status_Barra(CONEXION_TO_HOTS_FAILED);
                        delay(100);
                        Info_Cashless.Log(RTC,"CONSULTA_USUARIO_FIDELIZACION-"+String(Client_Id_Int),"False");
                    }

                    //                     contadores.ID_Consulta_INFO_Client(ID_Temp_Tarjeta); /* Setea Id cliente + tipo C */
                    //                     Variables_globales.Set_Variable_Global(Consulta_Conexion_To_Host, true);

                    //                     bool Handl=true;
                    //                     unsigned long Respuesta_Server = millis();
                    //                     int TIMEOUT_CONECT_SERVER = 6500; // 3500
                    //                     while (!Variables_globales.Get_Variable_Global(Conexion_To_Host) && millis() - Respuesta_Server < TIMEOUT_CONECT_SERVER)
                    //                     {
                    // #ifdef DEBUG_RFID
                    //                         Serial.println("Verificando Conexion to Host...");
                    // #endif
                    //                         vTaskDelay(300);
                    //                     }
                    //                     if (Variables_globales.Get_Variable_Global(Conexion_To_Host))
                    //                     {
                    //                         Info_Cashless.Init_Player_Tracking_Sesion(ID_Temp);
                    //                         Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                    //                         Info_Cashless.Unlock_Reader();
                    //                     }
                    //                     else
                    //                     {
                    //                         Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                    // #ifdef DEBUG_RFID
                    //                         Serial.println("Host Gmaster disconected");
                    // #endif
                    //                         Info_Cashless.Unlock_Reader();
                    //                         Status_Barra(CONEXION_TO_HOTS_FAILED);
                    //                         delay(100);
                    //                     }

                    if (Handl)
                        Info_Cashless.Unlock_Reader();
                }
            }
        }else{
            
            byte ID_Temp[8];
            ID_Temp[0] = INFO[0];
            ID_Temp[1] = INFO[1];
            ID_Temp[2] = INFO[2];
            ID_Temp[3] = INFO[3];
            ID_Temp[4] = INFO[4];
            ID_Temp[5] = INFO[5];
            ID_Temp[6] = INFO[6];
            ID_Temp[7] = INFO[7];

            int contador = 0;
            for (int i = 0; i < 8; i++)
            {
                if (INFO[i] == contadores.Get_Client_ID()[i])
                {
                    contador++;
                }
            }

            if(contador>=8)
            {
                Info_Cashless.Close_Player_Tracking_Sesion(true);
                Report_Http_Code(TERMINA_SESION_MANUAL, "Sesion terminada por Usuario: " , true);
                Info_Cashless.Unlock_Reader();

                if(Variables_globales.Get_Variable_Global(Conexion_TFT_Display)&& Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
                {
                    Menssage_TFT("Cerrando su sesion.....\nUn momento por favor",DisplayTFT.configtft.timeoutMensajesCONFIG,false);
                    DisplayTFT.info.Actualiza_Puntos_Finales=true;
                }
                    
                Info_Cashless.Log(RTC, "CIERRE_MANUAL_SESION_POR_USUARIO");
            }
            else
            {

                char ID_Temp_Tarjeta[8];
                ID_Temp_Tarjeta[0] = INFO[0];
                ID_Temp_Tarjeta[1] = INFO[1];
                ID_Temp_Tarjeta[2] = INFO[2];
                ID_Temp_Tarjeta[3] = INFO[3];
                ID_Temp_Tarjeta[4] = INFO[4];
                ID_Temp_Tarjeta[5] = INFO[5];
                ID_Temp_Tarjeta[6] = INFO[6];
                ID_Temp_Tarjeta[7] = INFO[7];
                //                 contadores.ID_Consulta_INFO_Client(ID_Temp_Tarjeta); /* Setea Id cliente + tipo C */
                //                 Variables_globales.Set_Variable_Global(Consulta_Conexion_To_Host, true);

                //                 unsigned long Respuesta_Server = millis();
                //                 int TIMEOUT_CONECT_SERVER = 6500; // 3500
                //                 while (!Variables_globales.Get_Variable_Global(Conexion_To_Host) && millis() - Respuesta_Server < TIMEOUT_CONECT_SERVER)
                //                 {
                // #ifdef DEBUG_RFID
                //                     Serial.println("Verificando Conexion to Host...");
                // #endif
                //                     vTaskDelay(300);
                //                 }
                //                 if (Variables_globales.Get_Variable_Global(Conexion_To_Host))
                //                 {
                //                     Info_Cashless.Init_Player_Tracking_Sesion(ID_Temp);
                //                     Info_Cashless.Unlock_Reader();
                //                     Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                //                 }
                //                 else
                //                 {
                //                     Info_Cashless.Unlock_Reader();
                //                     Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                // #ifdef DEBUG_RFID
                //                     Serial.println("Host Gmaster disconected");
                // #endif
                //                     Status_Barra(CONEXION_TO_HOTS_FAILED);
                //                     delay(100);
                //                 }

                char User[9];
                User[0] = INFO[0];
                User[1] = INFO[1];
                User[2] = INFO[2];
                User[3] = INFO[3];
                User[4] = INFO[4];
                User[5] = INFO[5];
                User[6] = INFO[6];
                User[7] = INFO[7];
                User[8] = '\0'; // Cierre del string

                //int Client_Id_Int = atoi(User); // Conversión a entero
                int Client_Id_Int=contadores.Get_Client_ID_Int_(INFO);

                Menssage_TFT("Estableciendo comunicación...\nUn momento por favor",DisplayTFT.configtft.timeoutMensajesCONFIG,false);

                if (Info_Cashless.Await_Conexion(Client_Id_Int, 'C'))
                {
                    Menssage_TFT("Cargando datos de sesion.....",DisplayTFT.configtft.timeoutMensajesCONFIG,false);
                    // Info_Cashless.Saves_Current_Player_Sesion(ID_Temp,PLAYER_TRACKING_SESION); /* Guarda sesion en memoria */
                    Info_Cashless.Init_Player_Tracking_Sesion(ID_Temp);
                    delay(500);
                    Info_Cashless.Unlock_Reader();
                    // Variables_globales.Set_Variable_Global(Conexion_To_Host, false);

                    
                    if(Variables_globales.Get_Variable_Global(Conexion_TFT_Display)&& Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
                        DisplayTFT.info.Actualiza_Puntos_Iniciales=true;

                    Info_Cashless.Log(RTC, "CONSULTA_USUARIO_FIDELIZACION-" + String(Client_Id_Int), "True");
                }
                else
                {

                    
                    Info_Cashless.Unlock_Reader();
                    // Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                    Serial.println("Host Gmaster disconected");
#endif
                    Status_Barra(CONEXION_TO_HOTS_FAILED);
                    delay(100);
                    Info_Cashless.Unlock_Reader();
                    Menssage_TFT("No se recibio respuesta.\nPor favor intente de nuevo",DisplayTFT.configtft.timeoutMensajesCONFIG,true);
                    Info_Cashless.Log(RTC, "CONSULTA_USUARIO_FIDELIZACION-" + String(Client_Id_Int), "False");
                }
            }

            Info_Cashless.Unlock_Reader();
        }
        
    }else
    {
        Status_Barra(ERROR_LECTURA);
        Report_Http_Code(TARJETA_NO_IDENTIFICADA,"Tarjeta no identificada no es cliente no operador: ");
        Info_Cashless.Unlock_Reader();
        Info_Cashless.Log(RTC,"LECTURA_TARJETA"," No se identifico el tipo de tarjeta");
    }
        
}




// int Type_Operation(byte INFO[])
// {
//     int contador = 0;
//     for (int i = 0; i < 8; i++)
//     {
//         if (INFO[i] == contadores.Get_Client_ID()[i])
//         {
//             contador++;
//         }
//     }
//     if (contador >= 8) /* Cierra Sesion por Usuario*/
//     {
//         return DOWNLOAD_TRANSACTION;
//     }else{
//         return LOAD_TRANSACTION;
//     }
// }
// void Cliente_VS_Operador2(byte MEMORIA[],byte INFO[])
// {

//     if (MEMORIA[0] == 'O')
//     {
//         contadores.Close_ID_Operador(); /* Borra ID operador anterior */
//         contadores.Dele_Operador_INFO_Operador(); /* ID*/
//         Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO,false);
//         Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO,true); /* Inicia timmer */
//         New_Timer_Final = New_Timmer_Inicial;
//         startTime = currentTime; /* Reset TimeOut_Player Tracking */
            
//         char ID_Temp_[8];
//         ID_Temp_[0] = INFO[0];
//         ID_Temp_[1] = INFO[1];
//         ID_Temp_[2] = INFO[2];
//         ID_Temp_[3] = INFO[3];
//         ID_Temp_[4] = INFO[4];
//         ID_Temp_[5] = INFO[5];
//         ID_Temp_[6] = INFO[6];
//         ID_Temp_[7] = INFO[7];
        
//         contadores.ID_Consulta_INFO_Operador(ID_Temp_);
//         Variables_globales.Set_Variable_Global(Consulta_Conexion_To_Host, true);

//         if (Variables_globales.Get_Variable_Global(Conexion_RFID))
//         {
//             Status_Barra(TARJETA_OPERADOR_INSERT);
//         }
//         unsigned long Respuesta_Server = millis();
//         int TIMEOUT_CONECT_SERVER = 6500;
//         while (!Variables_globales.Get_Variable_Global(Conexion_To_Host) && millis() - Respuesta_Server < TIMEOUT_CONECT_SERVER)
//         {
//             #ifdef DEBUG_RFID
//             Serial.println("Verificando Conexion to Host...");
//             #endif
//             vTaskDelay(300);
//         }

//         if (Variables_globales.Get_Variable_Global(Conexion_To_Host))
//         {
            

//             if (contadores.Set_Operador_ID_Temp(ID_Temp_))
//             {
//                 Condicion_Cumpl=false; /* Reset Timeout*/
               

//                 contadores.Copy_Operator_In_();
//                 Variables_globales.Set_Variable_Global(Operador_Detected, true);
//                 Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
//                 Update_Status_SD();
//                 Storage_Premios_OP(Archivo_CSV_Premios, Variables_globales.Get_Variable_Global(Enable_Storage), contadores.Get_Operador_ID());
//             }
//             else
//             {
//                 Variables_globales.Set_Variable_Global(Operador_Detected, false);
//                 contadores.Close_ID_Operador(); /*Temporal y en Trama*/
//                 Status_Barra(ERROR_LECTURA);
//                 Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
//                 Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
//             }
//         }else{
//             Variables_globales.Set_Variable_Global(Operador_Detected, false);
//             Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
//             Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
//             contadores.Close_ID_Operador(); /*Temporal y en Trama*/
//             #ifdef DEBUG_RFID
//             Serial.println("Host Gmaster disconected");
//             #endif
//             Status_Barra(CONEXION_TO_HOTS_FAILED);
//             delay(100);
//             Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO,false); /* Inicia timmer */
//         }

//         Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
//     }

//     else if (MEMORIA[0] == 'C')
//     {
//         bool Creditos_Machine=false;
//         New_Timer_Final = New_Timmer_Inicial;
//         startTime = currentTime;
//         contadores.Dele_Operador_INFO_Operador(); /* ID*/
//         Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
//         Variables_globales.Set_Variable_Global(Consulta_Conexion_To_Host, true);
//         Status_Barra(LECTURA_OK);

//         byte ID_Temp[8];
//         switch (Type_Operation(INFO))
//         {
//         case DOWNLOAD_TRANSACTION:
            
//             ID_Temp[0] = INFO[0];
//             ID_Temp[1] = INFO[1];
//             ID_Temp[2] = INFO[2];
//             ID_Temp[3] = INFO[3];
//             ID_Temp[4] = INFO[4];
//             ID_Temp[5] = INFO[5];
//             ID_Temp[6] = INFO[6];
//             ID_Temp[7] = INFO[7];

//             switch (Info_Cashless.Info_Client(ID_Temp, RTC, DOWNLOAD_TRANSACTION, Cashless.Get_Trans_ID_Int()))
//             {
//             case 2:

//                 if (Creditos_Machine)
//                 {
//                     Solicitud_Descarga_Cashless();
//                     if (Close_Sesion_Player_Tracking())
//                     {
//                         /*  Cierra Sesion de juego por cliente */
//                         delay(500);
//                         Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
//                     }
//                     else
//                     {
//                         Status_Barra(ERROR_LECTURA);
//                         delay(100);
//                         Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
//                     }
//                     Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
//                 }
//                 else
//                 {
//                     if (Close_Sesion_Player_Tracking())
//                     {
//                         /*  Cierra Sesion de juego por cliente */
//                         delay(500);
//                         Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
//                     }
//                     else
//                     {
//                         Status_Barra(ERROR_LECTURA);
//                         delay(100);
//                         Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
//                     }
//                     Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
//                 }

//                 break;
            
            
//             default:
//                 break;
//             }
//             break;

//         case LOAD_TRANSACTION:
           
//             ID_Temp[0] = INFO[0];
//             ID_Temp[1] = INFO[1];
//             ID_Temp[2] = INFO[2];
//             ID_Temp[3] = INFO[3];
//             ID_Temp[4] = INFO[4];
//             ID_Temp[5] = INFO[5];
//             ID_Temp[6] = INFO[6];
//             ID_Temp[7] = INFO[7];
            
//             switch (Info_Cashless.Info_Client(ID_Temp, RTC,LOAD_TRANSACTION,Cashless.Get_Trans_ID_Int()))
//             {
//             case 1:
//                 if (contadores.Set_Client_ID(ID_Temp))
//                 {
//                     Solicitud_Carga_Cashless();
//                     New_Timer_Final = New_Timmer_Inicial; /*RESET TIMEOUT*/
//                     /* Guarda  Informacion cliente en memoria SD */
//                     Update_Status_SD(); /* Actualiza Estado de Almacenamiento */
//                     Storage_Cliente(Archivo_CSV_Sesiones, Variables_globales.Get_Variable_Global(Enable_Storage), contadores.Get_Client_ID());
//                     /*  Maquina en juego  y Sesion Activa */
//                     Variables_globales.Set_Variable_Global(Flag_Sesion_RFID, true);
//                     Variables_globales.Set_Variable_Global(Flag_Contadores_Sesion_ON, true);
//                     // Variables_globales.Set_Variable_Global(Flag_Maquina_En_Juego, true);
//                     /* -Notificacion de inicio de sesion de juego-*/
//                     Status_Barra(SESION_INICIADA);
//                 }
//                 break;
//             case SALDO_INSUFICIENTE:
//                 /* Inicia Player Tracking */
//                 if (contadores.Set_Client_ID(ID_Temp))
//                 {
//                     New_Timer_Final = New_Timmer_Inicial; /*RESET TIMEOUT*/
//                     /* Guarda  Informacion cliente en memoria SD */
//                     Update_Status_SD(); /* Actualiza Estado de Almacenamiento */
//                     Storage_Cliente(Archivo_CSV_Sesiones, Variables_globales.Get_Variable_Global(Enable_Storage), contadores.Get_Client_ID());
//                     /*  Maquina en juego  y Sesion Activa */
//                     Variables_globales.Set_Variable_Global(Flag_Sesion_RFID, true);
//                     Variables_globales.Set_Variable_Global(Flag_Contadores_Sesion_ON, true);
//                     // Variables_globales.Set_Variable_Global(Flag_Maquina_En_Juego, true);
//                     /* -Notificacion de inicio de sesion de juego-*/
//                     Status_Barra(SESION_INICIADA);
//                 }
//                 break;
//             default:
//                 break;
//             }
            
//             break;

//         default:
//             break;
//         }

        
//         Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);


// //         if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) > 3)
// //         {

// //             int contador = 0;
// //             for (int i = 0; i < 8; i++)
// //             {
// //                 if (INFO[i] == contadores.Get_Client_ID()[i])
// //                 {
// //                     contador++;
// //                 }
// //             }
// //             if (contador >= 8) /* Cierra Sesion por Usuario*/
// //             {

// //                 Info_Cashless.Info_Client(contadores.Get_Client_ID(), RTC,DOWNLOAD_TRANSACTION,Cashless.Get_Trans_ID_Int());

                
// //                 Solicitud_Descarga_Cashless();
// //                 if(Close_Sesion_Player_Tracking())
// //                 {
// //                     /*  Cierra Sesion de juego por cliente */
// //                     delay(500);
// //                     contador = 0;
// //                     Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
// //                 }else
// //                 {
// //                     Status_Barra(ERROR_LECTURA);
// //                     delay(100);
// //                     contador = 0;
// //                     Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
// //                 }
// //                 Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
// //             }
// //             else
// //             {
// //                 Status_Barra(LECTURA_OK);
// //                 unsigned long Respuesta_Server = millis();
// //                 int TIMEOUT_CONECT_SERVER = 6500; //3500
// //                 while (!Variables_globales.Get_Variable_Global(Conexion_To_Host) && millis() - Respuesta_Server < TIMEOUT_CONECT_SERVER)
// //                 {
// //                    #ifdef DEBUG_RFID
// //                     Serial.println("Verificando Conexion to Host...");
// //                    #endif
// //                     vTaskDelay(300);
// //                 }
// //                 if (Variables_globales.Get_Variable_Global(Conexion_To_Host))
// //                 {
// //                     /* Inicia Player Tracking  */
// //                     #ifdef DEBUG_RFID
// //                     Serial.println("Host Gmaster conected");
// //                     #endif
// //                     byte ID_Temp[8];
// //                     ID_Temp[0] = INFO[0];
// //                     ID_Temp[1] = INFO[1];
// //                     ID_Temp[2] = INFO[2];
// //                     ID_Temp[3] = INFO[3];
// //                     ID_Temp[4] = INFO[4];
// //                     ID_Temp[5] = INFO[5];
// //                     ID_Temp[6] = INFO[6];
// //                     ID_Temp[7] = INFO[7];

// //                   //  Info_Cashless.Info_Client(ID_Temp, RTC,LOAD_TRANSACTION,Cashless.Get_Trans_ID_Int());

// //                     /* Inicia Player Tracking Sesion */
// //                     if (contadores.Set_Client_ID(ID_Temp))
// //                     {
// //                         Solicitud_Carga_Cashless();
// //                         Info_Cashless.Info_Client(ID_Temp, RTC,DOWNLOAD_TRANSACTION,Cashless.Get_Trans_ID_Int());

// //                         New_Timer_Final = New_Timmer_Inicial; /*RESET TIMEOUT*/
// //                         /* Guarda  Informacion cliente en memoria SD */
// //                         Update_Status_SD(); /* Actualiza Estado de Almacenamiento */
// //                         Storage_Cliente(Archivo_CSV_Sesiones, Variables_globales.Get_Variable_Global(Enable_Storage),contadores.Get_Client_ID());
// //                         /*  Maquina en juego  y Sesion Activa */
// //                         Variables_globales.Set_Variable_Global(Flag_Sesion_RFID, true);
// //                         Variables_globales.Set_Variable_Global(Flag_Contadores_Sesion_ON, true);
// //                        // Variables_globales.Set_Variable_Global(Flag_Maquina_En_Juego, true);
// //                         /* -Notificacion de inicio de sesion de juego-*/
// //                         Status_Barra(SESION_INICIADA);
// //                     }else{
// //                         Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
// //                         Variables_globales.Set_Variable_Global(Flag_Sesion_RFID, false);
// //                         Status_Barra(ERROR_LECTURA);
// //                     }
// //                     delay(100); 
// //                     Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
// //                 }
// //                 else
// //                 {
// //                     Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
// //                     Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
// //                     #ifdef DEBUG_RFID
// //                     Serial.println("Host Gmaster disconected");
// //                     #endif
// //                     Status_Barra(CONEXION_TO_HOTS_FAILED);
// //                     delay(100);
// //                 }  
// //             }
// //         }
// //         else
// //         {

// //             int contador = 0;
// //             for (int i = 0; i < 8; i++)
// //             {
// //                 if (INFO[i] == contadores.Get_Client_ID()[i])
// //                 {
// //                     contador++;
// //                 }
// //             }
// //             if (contador >= 8) /* Cierra Sesion por Usuario*/
// //             {
// //                 if (Close_Sesion_Player_Tracking())
// //                 {
// //                     /*  Cierra Sesion de juego por cliente */
// //                     delay(10);
// //                     contador = 0;
// //                     Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
// //                 }
// //                 else
// //                 {
// //                     Status_Barra(ERROR_LECTURA);
// //                     delay(100);
// //                     contador = 0;
// //                     Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
// //                 }
// //             }
// //             else
// //             {
// //                 Status_Barra(LECTURA_OK);
// //                 unsigned long Respuesta_Server = millis();
// //                 int TIMEOUT_CONECT_SERVER = 6500;
// //                 while (!Variables_globales.Get_Variable_Global(Conexion_To_Host) && millis() - Respuesta_Server < TIMEOUT_CONECT_SERVER)
// //                 {
// // #ifdef DEBUG_RFID
// //                     Serial.println("Verificando Conexion to Host...");
// // #endif
// //                     vTaskDelay(300);
// //                 }
// //                 if (Variables_globales.Get_Variable_Global(Conexion_To_Host))
// //                 {
// // /* Inicia Player Tracking  */
// // #ifdef DEBUG_RFID
// //                     Serial.println("Host Gmaster conected");
// // #endif
// //                     byte ID_Temp[8];
// //                     ID_Temp[0] = INFO[0];
// //                     ID_Temp[1] = INFO[1];
// //                     ID_Temp[2] = INFO[2];
// //                     ID_Temp[3] = INFO[3];
// //                     ID_Temp[4] = INFO[4];
// //                     ID_Temp[5] = INFO[5];
// //                     ID_Temp[6] = INFO[6];
// //                     ID_Temp[7] = INFO[7];

// //                     /* Inicia Player Tracking Sesion */
// //                     if (contadores.Set_Client_ID(ID_Temp))
// //                     {

// //                         New_Timer_Final = New_Timmer_Inicial; /*RESET TIMEOUT*/
// //                         /* Guarda  Informacion cliente en memoria SD */
// //                         Update_Status_SD(); /* Actualiza Estado de Almacenamiento */
// //                         Storage_Cliente(Archivo_CSV_Sesiones, Variables_globales.Get_Variable_Global(Enable_Storage), contadores.Get_Client_ID());
// //                         /*  Maquina en juego  y Sesion Activa */
// //                         Variables_globales.Set_Variable_Global(Flag_Sesion_RFID, true);
// //                         Variables_globales.Set_Variable_Global(Flag_Contadores_Sesion_ON, true);
// //                         // Variables_globales.Set_Variable_Global(Flag_Maquina_En_Juego, true);
// //                         /* -Notificacion de inicio de sesion de juego- */
// //                         Status_Barra(SESION_INICIADA);
// //                     }
// //                     else
// //                     {
// //                         Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
// //                         Variables_globales.Set_Variable_Global(Flag_Sesion_RFID, false);
// //                         Status_Barra(ERROR_LECTURA);
// //                     }
// //                     delay(100);
// //                     Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
// //                 }
// //                 else
// //                 {
// //                     Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
// //                     Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
// // #ifdef DEBUG_RFID
// //                     Serial.println("Host Gmaster disconected");
// // #endif
// //                     Status_Barra(CONEXION_TO_HOTS_FAILED);
// //                     delay(100);
// //                 }
// //             }
//             // /* ---------------------------->Clashless<--------------------------------------------------------------------*/
//             // Status_Barra(LECTURA_OK);
//             // unsigned long Respuesta_Server = millis();
//             // int TIMEOUT_CONECT_SERVER = 3000;
//             // while (!Variables_globales.Get_Variable_Global(Conexion_To_Host) && millis() - Respuesta_Server < TIMEOUT_CONECT_SERVER)
//             // {
//             //     #ifdef DEBUG_RFID
//             //     Serial.println("Verificando Conexion to Host...");
//             //     #endif
//             //     vTaskDelay(300);
//             // }

//             // if (Variables_globales.Get_Variable_Global(Conexion_To_Host))
//             // {
//             //     byte ID_Temp_Cashless[8];
//             //     ID_Temp_Cashless[0] = INFO[0];
//             //     ID_Temp_Cashless[1] = INFO[1];
//             //     ID_Temp_Cashless[2] = INFO[2];
//             //     ID_Temp_Cashless[3] = INFO[3];
//             //     ID_Temp_Cashless[4] = INFO[4];
//             //     ID_Temp_Cashless[5] = INFO[5];
//             //     ID_Temp_Cashless[6] = INFO[6];
//             //     ID_Temp_Cashless[7] = INFO[7];

//             //     if(contadores.Set_ID_Cliente_Temp(ID_Temp_Cashless))
//             //     {
//             //         Variables_globales.Set_Variable_Global(Consulta_Info_Cliente, true);
//             //     }
//             // }else{
//             //     Status_Barra(CONEXION_TO_HOTS_FAILED);
//             //     delay(100);
//             //     Variables_globales.Set_Variable_Global(Handle_RFID_Lector,false);
//             // }
//             // Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
//         // }
//     }else{
//         Status_Barra(ERROR_LECTURA);
//         Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
//         Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
//     }
// }


int  Convert_Char_To_Int4(char buffer[])
{
    int resultado = ((buffer[0] - 48) * 10000000) + ((buffer[1] - 48) * 1000000) +
                    ((buffer[2] - 48) * 100000) + ((buffer[3] - 48) * 10000) +
                    ((buffer[4] - 48) * 1000) + ((buffer[5] - 48) * 100) +
                    ((buffer[6] - 48) * 10) + ((buffer[7] - 48) * 1);
    return resultado;
}


void Timer_Close_Player_Tracking(unsigned long Tiempo_Transcurrido, int Inactividad)
{
    int Tiempo_Restante=(Inactividad-Tiempo_Transcurrido)/ 1000;
    
    if(Tiempo_Restante<=20)
    {
        TimeOUT=millis();
        Activa_ALERT=true;
    }
    if(Tiempo_Restante<=20&&Tiempo_Restante>15)
    {
        Parpadeo=3000;
        if((TimeOUT-TimeIn)>=Parpadeo)
        {
           Status_Barra(TIMEOUT_SESION_ALERT);
           TimeIn=millis();
        }

    }else if(Tiempo_Restante<=15&&Tiempo_Restante>10){
        Parpadeo=1000;
        if((TimeOUT-TimeIn)>=Parpadeo)
        {
           Status_Barra(TIMEOUT_SESION_ALERT);
           TimeIn=millis();
        }
    }else if(Tiempo_Restante<=10&&Tiempo_Restante>5)
    {
        Parpadeo=800;
        if((TimeOUT-TimeIn)>=Parpadeo)
        {
           Status_Barra(TIMEOUT_SESION_ALERT);
           TimeIn=millis();
        }
    }
    else if(Tiempo_Restante<=5 &&Tiempo_Restante>3)
    {
        
        Parpadeo=600;
        if((TimeOUT-TimeIn)>=Parpadeo)
        {
           Status_Barra(TIMEOUT_SESION_ALERT);
           TimeIn=millis();
        }
    }
     else if(Tiempo_Restante<=3 &&Tiempo_Restante>2)
    {
        Parpadeo=350;
        if((TimeOUT-TimeIn)>=Parpadeo)
        {
           Status_Barra(TIMEOUT_SESION_ALERT);
           TimeIn=millis();
        }
    }
     else if(Tiempo_Restante<=1)
    {
        Parpadeo=100;
        if((TimeOUT-TimeIn)>=Parpadeo)
        {
           Status_Barra(TIMEOUT_SESION_ALERT);
           TimeIn=millis();
        }
    }

}


/* Funcion para cerrar sesion player tracking */
bool Close_Sesion_Player_Tracking(void)
{
    
    // switch (contadores.Get_Type_Sesion())
    // {
    // case PLAYER_TRACKING_SESION:
    //     //Serial.println("Id Borrado OK");
    //     Info_Cashless.Remove_Currrent_Player_Sesion();
    //     break;

    // default:
    //     break;
    // }

    contadores.Close_ID_Client();
    contadores.Close_ID_Client_Temp();
    Sesion_Cerrada_Color=true;
    Variables_globales.Set_Variable_Global(Flag_Sesion_RFID,false); 
    Variables_globales.Set_Variable_Global(Flag_Contadores_Sesion_OFF, true);
    Status_Barra(SESION_CERRADA);
    delay(10);
    Reset_Handle_LED();
    delay(50);
    return true;
}
/* Utiliza El ID Cashless  para iniciar player tracking cuando el saldo es insuficiente para carga cashless*/
bool Player_Tracking_Sesion(void)
{
    byte ID_Temp[8];

    for(int i=0; i<8;i++)
    {
        if(contadores.Get_Client_ID_Temp()[i]==NULL)
        {
            return false;
        }
    }
    int valida=0;
     for(int i=0; i<8;i++)
    {
        if(contadores.Get_Client_ID_Temp()[i]==48)
        {
            valida++;
        }
    }

    if(valida>=8)
    {
        return false;
    }
    ID_Temp[0] = contadores.Get_Client_ID_Temp()[0];
    ID_Temp[1] = contadores.Get_Client_ID_Temp()[1];
    ID_Temp[2] = contadores.Get_Client_ID_Temp()[2];
    ID_Temp[3] = contadores.Get_Client_ID_Temp()[3];
    ID_Temp[4] = contadores.Get_Client_ID_Temp()[4];
    ID_Temp[5] = contadores.Get_Client_ID_Temp()[5];
    ID_Temp[6] = contadores.Get_Client_ID_Temp()[6];
    ID_Temp[7] = contadores.Get_Client_ID_Temp()[7];

    if (contadores.Set_Client_ID(ID_Temp))
    {
        /* Close Player Tracking Sesion */
        New_Timer_Final = New_Timmer_Inicial;
        Variables_globales.Set_Variable_Global(Flag_Sesion_RFID, true);
        Variables_globales.Set_Variable_Global(Flag_Contadores_Sesion_ON, true);
        Variables_globales.Set_Variable_Global(Flag_Maquina_En_Juego, true);
        if(Variables_globales.Get_Variable_Global(Conexion_RFID))
        {
            Status_Barra(SESION_INICIADA);
        }  
    }
    return true;
}

void Consulta_Info_Cliente_Sistema(void)
{
    /*Consulta info cliente */
    /*Flag de consulta AFT */
    /*Serializacion Cashless*/
}
/*----------------------------------------> Utilidades <---------------------------------------------*/

void Sesion_Abierta_Color(int Figura)
{
    int R, G, B;
    int intensidad;
    int Style;
    int NUM;
    switch (Figura)
    {
    case 1:                                           /*Sesion de juego disponible*/
        Barra_Status_Sesion_Client.setBrightness(20); /* Configura Brillo*/
        Barra_Status_Sesion_Client.setPixelColor(0, 255, 0, 0);
        Barra_Status_Sesion_Client.setPixelColor(1, 255, 0, 0);
        Barra_Status_Sesion_Client.setPixelColor(2, 255, 0, 0);
        Barra_Status_Sesion_Client.setPixelColor(3, 255, 0, 0);
        Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
        break;

    case 2:                                           /*Random de colores*/
        Barra_Status_Sesion_Client.setBrightness(20); /* Configura Brillo*/
        for (int i = 0; i < 5; i++)
        {
            R = random(100);
            G = random(256);
            B = random(256);

            Barra_Status_Sesion_Client.setPixelColor(i, G, R, B);
            Barra_Status_Sesion_Client.show(); // Mostrar el cambio en cada iteración
        }
        break;

    case 3:                                           /*Desplazamiento Derecha-Izquierda*/
        Barra_Status_Sesion_Client.setBrightness(20); /* Configura Brillo*/
        R = random(100);
        G = random(256);
        B = random(256);

        for (int i = 0; i < 5; i++)
        {
            Barra_Status_Sesion_Client.setPixelColor(i, G, R, B);
            Barra_Status_Sesion_Client.show();
            delay(150);
        }

        for (int i = 3; i >= 0; i--)
        {
            Barra_Status_Sesion_Client.setPixelColor(i, G, R, B);
            Barra_Status_Sesion_Client.show();
            delay(150);
        }
        Barra_Status_Sesion_Client.setPixelColor(0, G, R, B);
        Barra_Status_Sesion_Client.setPixelColor(1, G, R, B);
        Barra_Status_Sesion_Client.setPixelColor(2, G, R, B);
        Barra_Status_Sesion_Client.setPixelColor(3, G, R, B);
        Barra_Status_Sesion_Client.show();
        break;

    case 4:                                           /*Solo un Bit*/
        Barra_Status_Sesion_Client.setBrightness(20); /* Configura Brillo*/
        R = random(100);
        G = random(256);
        B = random(256);

        for (int i = 0; i < 5; i++)
        {
            Barra_Status_Sesion_Client.setPixelColor(i, G, R, B);
            Barra_Status_Sesion_Client.show();
            delay(150);
            Barra_Status_Sesion_Client.setPixelColor(i, 0, 0, 0);
            Barra_Status_Sesion_Client.show();
        }

        for (int i = 3; i >= 0; i--)
        {
            Barra_Status_Sesion_Client.setPixelColor(i, G, R, B);
            Barra_Status_Sesion_Client.show();
            delay(150);
            Barra_Status_Sesion_Client.setPixelColor(i, 0, 0, 0);
            Barra_Status_Sesion_Client.show();
        }

        Barra_Status_Sesion_Client.setPixelColor(0, G, R, B);
        Barra_Status_Sesion_Client.setPixelColor(1, G, R, B);
        Barra_Status_Sesion_Client.setPixelColor(2, G, R, B);
        Barra_Status_Sesion_Client.setPixelColor(3, G, R, B);
        Barra_Status_Sesion_Client.show();
        break;

    case 5:                                           /* Aleatorio posición*/
        Barra_Status_Sesion_Client.setBrightness(20); /* Configura Brillo*/
        R = random(100);
        G = random(256);
        B = random(256);

        
        for (int i = 0; i < 5; i++)
        {
            int pos = random(0, 4);
            Barra_Status_Sesion_Client.setPixelColor(pos, G, R, B);
            Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
            delay(150);
            Barra_Status_Sesion_Client.setPixelColor(pos, 0, 0, 0);
            Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
        }

        Barra_Status_Sesion_Client.setPixelColor(0, G, R, B);
        Barra_Status_Sesion_Client.setPixelColor(1, G, R, B);
        Barra_Status_Sesion_Client.setPixelColor(2, G, R, B);
        Barra_Status_Sesion_Client.setPixelColor(3, G, R, B);
        Barra_Status_Sesion_Client.show();

        break;

    case 6: /*Cambia intensidad*/
        intensidad = random(10, 20);
        Style = random(2, 5);
        Barra_Status_Sesion_Client.setBrightness(intensidad); /* Configura Brillo*/
        Barra_Status_Sesion_Client.show();                    // Actualizamos la tira de LED

        switch (Style)
        {
        case 2:
            for (int i = 0; i < 5; i++)
            {
                R = random(100);
                G = random(256);
                B = random(256);

                
                Barra_Status_Sesion_Client.setPixelColor(i, G, R, B);

                Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
                intensidad = random(10, 20);
                Barra_Status_Sesion_Client.setBrightness(intensidad); /* Configura Brillo*/
            }
            Barra_Status_Sesion_Client.setBrightness(20); /* Configura Brillo*/
            break;
        case 3:

            R = random(100);
            G = random(256);
            B = random(256);

            for (int i = 0; i < 5; i++)
            {
                Barra_Status_Sesion_Client.setPixelColor(i, G, R, B);
                Barra_Status_Sesion_Client.show();
                delay(150);
                intensidad = random(10, 20);
                Barra_Status_Sesion_Client.setBrightness(intensidad); /* Configura Brillo*/
            }

            for (int i = 3; i >= 0; i--)
            {
                Barra_Status_Sesion_Client.setPixelColor(i, G, R, B);
                Barra_Status_Sesion_Client.show();
                delay(150);
                intensidad = random(10, 20);
                Barra_Status_Sesion_Client.setBrightness(intensidad); /* Configura Brillo*/
            }
            Barra_Status_Sesion_Client.setBrightness(20); /* Configura Brillo*/
            break;
        case 4:

            R = random(100);
            G = random(256);
            B = random(256);

            for (int i = 0; i < 5; i++)
            {
                Barra_Status_Sesion_Client.setPixelColor(i, G, R, B);
                Barra_Status_Sesion_Client.show();
                delay(150);
                Barra_Status_Sesion_Client.setPixelColor(i, 0, 0, 0);
                Barra_Status_Sesion_Client.show();
            }

            for (int i = 3; i >= 0; i--)
            {
                Barra_Status_Sesion_Client.setPixelColor(i, G, R, B);
                Barra_Status_Sesion_Client.show();
                delay(150);
                Barra_Status_Sesion_Client.setPixelColor(i, 0, 0, 0);
                Barra_Status_Sesion_Client.show();
            }
            break;

        case 5:

            R = random(256);
            G = random(256);
            B = random(256);
            for (int i = 0; i < 5; i++)
            {
                int pos = random(0, 4);
                Barra_Status_Sesion_Client.setPixelColor(pos, G, R, B);
                Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
                delay(150);
                Barra_Status_Sesion_Client.setPixelColor(pos, 0, 0, 0);
                Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
            }
            break;

        default:
            Barra_Status_Sesion_Client.setBrightness(20); /* Configura Brillo*/
            Barra_Status_Sesion_Client.setPixelColor(0, 255, 0, 0);
            Barra_Status_Sesion_Client.setPixelColor(1, 255, 0, 0);
            Barra_Status_Sesion_Client.setPixelColor(2, 255, 0, 0);
            Barra_Status_Sesion_Client.setPixelColor(3, 255, 0, 0);
            Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
            break;
        }

        Barra_Status_Sesion_Client.setBrightness(20); /* Configura Brillo*/
        intensidad = 20;
        break;

    case 7:
        Barra_Status_Sesion_Client.setBrightness(20); /* Configura Brillo*/
        Barra_Status_Sesion_Client.setPixelColor(0, 255, 0, 0);
        Barra_Status_Sesion_Client.setPixelColor(1, 255, 0, 0);
        Barra_Status_Sesion_Client.setPixelColor(2, 255, 0, 0);
        Barra_Status_Sesion_Client.setPixelColor(3, 255, 0, 0);
        Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
        break;
    case 8:

        Barra_Status_Sesion_Client.setBrightness(20); /* Configura Brillo*/
        Barra_Status_Sesion_Client.setPixelColor(0, 255, 0, 0);
        Barra_Status_Sesion_Client.setPixelColor(1, 255, 0, 0);
        Barra_Status_Sesion_Client.setPixelColor(2, 255, 0, 0);
        Barra_Status_Sesion_Client.setPixelColor(3, 255, 0, 0);
        Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
        break;
    default:
        Barra_Status_Sesion_Client.setBrightness(20); /* Configura Brillo*/
        Barra_Status_Sesion_Client.setPixelColor(0, 255, 0, 0);
        Barra_Status_Sesion_Client.setPixelColor(1, 255, 0, 0);
        Barra_Status_Sesion_Client.setPixelColor(2, 255, 0, 0);
        Barra_Status_Sesion_Client.setPixelColor(3, 255, 0, 0);
        Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
        break;
    }
}

void RESET_Handle(void)
{
   // SPI.begin();
    mfrc522.PCD_Reset();
    pcf8574.digitalWrite(P0,LOW);
    delay(10);
    mfrc522.PCD_Init();
}

int Pines=0;
int Host_=0;
void Reset_Handle_LED(void)
{
    Handle_LED=false;
}

void Status_Barra(int Status)
{


    

    int R = 0;
    int G = 0;
    int B = 0;
    int LEDD2 = false;
    int Brig=20;
    if (Variables_globales.Get_Variable_Global(Conexion_RFID))
    {
        switch (Status)
        {
        case Reset_Exitoso:
            Handle_LED = true;
            for (int i = 0; i < 15; i++)
            {
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                Barra_Status_Sesion_Client.clear();
                Barra_Status_Sesion_Client.show();
                delay(40);
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                delay(50);
            }

            Handle_LED = false;
            customTone(5, 1);
            delayMicroseconds(3350);
            customTone(1, 3);
            delayMicroseconds(2000);
            break;
        case ERROR_RESET_HANDPAY:
            Handle_LED = true;
            for (int i = 0; i < 20; i++)
            {
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                Barra_Status_Sesion_Client.clear();
                Barra_Status_Sesion_Client.show();
                delay(40);
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                delay(50);
            }
            Handle_LED = false;

            customTone(5, 1);
            delayMicroseconds(100);
            customTone(1, 3);
            delayMicroseconds(3000);
            customTone(5, 1);
            delayMicroseconds(100);
            customTone(1, 3);
            delayMicroseconds(3000);

            break;

        case GMASTER_CONFIRMA_RESET:
            Handle_LED = true;
            Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
            Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
            Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
            Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
            Barra_Status_Sesion_Client.show();
            Handle_LED = false;
            break;
        case SESION_INICIADA:
            Handle_LED = true;
            for (int i = 0; i < 20; i++)
            {

                if (i < 7)
                {
                    Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                    Barra_Status_Sesion_Client.show();
                    Barra_Status_Sesion_Client.clear();
                    Barra_Status_Sesion_Client.show();
                    delay(40);
                    Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                    Barra_Status_Sesion_Client.show();
                    delay(50);
                }
                else
                {
                    R = random(1, 255);
                    G = random(1, 255);
                    B = random(1, 255);

                    Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(G, R, B)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(G, R, B)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(G, R, B)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(G, R, B)); // rojo
                    Barra_Status_Sesion_Client.show();
                    Barra_Status_Sesion_Client.clear();
                    Barra_Status_Sesion_Client.show();
                    delay(40);
                    Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(G, R, B)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(G, R, B)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(G, R, B)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(G, R, B)); // rojo
                    Barra_Status_Sesion_Client.show();
                    delay(50);
                }
            }

            Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
            Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
            Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
            Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
            Handle_LED = false;

            customTone(5, 1);
            delayMicroseconds(3350);
            customTone(1, 3);
            delayMicroseconds(2000);

            delay(10);

            for (int i = 0; i < 4; i++)
            {
                customTone(5, 1);
                delayMicroseconds(1000);
            }
            customTone(5, 1);
            delayMicroseconds(200);
            customTone(1, 3);
            delayMicroseconds(800);
            customTone(5, 1);
            delayMicroseconds(3000);

            break;
        case SESION_TERMINADA:

            break;

        case TIMEOUT_SESION_ALERT:

            if (!LEDD2)
            {

                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                LEDD2 = true;
                customTone(5, 1);
            }
            else
            {
                // Apagar todos los LEDs
                Barra_Status_Sesion_Client.clear();
                Barra_Status_Sesion_Client.show();
                LEDD2 = false;
            }
            break;
        case ERROR_LECTURA:
            for (int i = 0; i < 20; i++)
            {
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                Barra_Status_Sesion_Client.clear();
                Barra_Status_Sesion_Client.show();
                delay(40);
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                delay(50);
            }

            customTone(5, 1);
            delayMicroseconds(100);
            customTone(1, 3);
            delayMicroseconds(2000);

            customTone(5, 1);
            delayMicroseconds(100);
            customTone(1, 3);
            delayMicroseconds(2000);

            break;

        case INICIO_MODULO:

            Barra_Status_Sesion_Client.begin();
            Barra_Status_Sesion_Client.clear();
            Barra_Status_Sesion_Client.setBrightness(20);
            Barra_Status_Sesion_Client.show();

            Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
            Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
            Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
            Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
            Barra_Status_Sesion_Client.show();
            break;
        case TIMEOUT_CONTEO:
            break;
        case SESION_CERRADA:
            Handle_LED = true;
            Barra_Status_Sesion_Client.clear();
            Barra_Status_Sesion_Client.show();
            for (int i = 0; i < 20; i++)
            {
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(255, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(255, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(255, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(255, 255, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                Barra_Status_Sesion_Client.clear();
                Barra_Status_Sesion_Client.show();
                delay(40);
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(255, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(255, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(255, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(255, 255, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                delay(50);
            }
            Handle_LED = false;

            customTone(5, 1);
            delayMicroseconds(100);
            customTone(1, 3);
            delayMicroseconds(2000);
            delay(10);

            break;

        case MODULO_OK:
            //Barra_Status_Sesion_Client.begin();
            Barra_Status_Sesion_Client.clear();
            Barra_Status_Sesion_Client.setBrightness(20);
            Barra_Status_Sesion_Client.show();

            for (int i = 0; i < 10; i++)
            {
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                Barra_Status_Sesion_Client.clear();
                Barra_Status_Sesion_Client.show();
                delay(50);
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                delay(50);
            }

            customTone(5, 1);
            delayMicroseconds(3350);
            customTone(1, 3);
            delayMicroseconds(2000);

            delay(100);
            break;
        case LECTURA_OK:

            for (int i = 0; i < 10; i++)
            {
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                Barra_Status_Sesion_Client.clear();
                Barra_Status_Sesion_Client.show();
                delay(40);
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                delay(40);
            }
            break;
        case MODULO_KO:

            Barra_Status_Sesion_Client.begin();
            Barra_Status_Sesion_Client.clear();
            Barra_Status_Sesion_Client.setBrightness(20);
            Barra_Status_Sesion_Client.show();

            for (int i = 0; i < 10; i++)
            {
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                Barra_Status_Sesion_Client.clear();
                Barra_Status_Sesion_Client.show();
                delay(50);
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                delay(50);
            }

            for (int i = 0; i < 10; i++)
            {
                customTone(1, 1);
                delay(100);
                customTone(5, 1);
            }
            break;

        case CONEXION_TO_HOTS_FAILED:
            Handle_LED = true;
            for (int i = 0; i < 3; i++)
            {

                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                Barra_Status_Sesion_Client.clear();
                Barra_Status_Sesion_Client.show();
                delay(50);
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 255, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                delay(50);

                customTone(5, 1);
                delayMicroseconds(100);
                customTone(1, 3);
                delayMicroseconds(500);
            }
            Handle_LED = false;
            break;

        case CLIENTE_NO_BD:
            Handle_LED = true;
            for (int i = 0; i < 10; i++)
            {
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                Barra_Status_Sesion_Client.clear();
                Barra_Status_Sesion_Client.show();
                delay(100);
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                delay(100);
            }
            Handle_LED = false;

            customTone(5, 1);
            delayMicroseconds(100);
            customTone(1, 3);
            delayMicroseconds(500);
            break;

        case NO_HAY_COMUNICACION:
            Pines++;
            if (millis() - Encendido >= 1000)
            {

                if (!LED)
                {
                    Host_++;
                    Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                    Barra_Status_Sesion_Client.show();
                    LED = true;
                    if (Host_ < 8)
                    {
                        customTone(5, 1);
                    }
                }
                else
                {
                    // Apagar todos los LEDs
                    Barra_Status_Sesion_Client.clear();
                    Barra_Status_Sesion_Client.show();
                    LED = false;
                }

                Encendido = millis();
            }
            break;

        case TARJETA_OPERADOR_INSERT:
            Handle_LED = true;
            for (int i = 0; i < 10; i++)
            {
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                Barra_Status_Sesion_Client.show();
                Barra_Status_Sesion_Client.clear();
                Barra_Status_Sesion_Client.show();
                delay(40);
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                Barra_Status_Sesion_Client.show();
                delay(50);
            }
            customTone(5, 1);
            delayMicroseconds(3350);
            customTone(1, 3);
            delayMicroseconds(2000);
            Handle_LED = false;
            break;

        case UPDATING_SYS:
            Handle_LED = true;
            if (millis() - Encendido >= 150)
            {

                if (!LED)
                {
                    Host_++;
                    Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 255, 255)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 255, 255)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 255, 255)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 255, 255)); // rojo
                    Barra_Status_Sesion_Client.show();
                    LED = true;
                }
                else
                {
                    // Apagar todos los LEDs
                    Barra_Status_Sesion_Client.clear();
                    Barra_Status_Sesion_Client.show();
                    LED = false;
                }

                Encendido = millis();
            }
            Handle_LED = false;
            break;

        case NOT_AP_MODE:
            Handle_LED = true;
            if (millis() - Encendido >= 1000)
            {

                if (!LED)
                {
                    Host_++;
                    Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                    Barra_Status_Sesion_Client.show();
                    LED = true;
                }
                else
                {
                    // Apagar todos los LEDs
                    Barra_Status_Sesion_Client.clear();
                    Barra_Status_Sesion_Client.show();
                    LED = false;
                }

                Encendido = millis();
            }
            Handle_LED = false;
            break;

        case CONFIG_EXITOSA:
            Handle_LED = true;
            for (int i; i < 3; i++)
            {
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                Barra_Status_Sesion_Client.show();
                Barra_Status_Sesion_Client.clear();
                Barra_Status_Sesion_Client.show();
                delay(50);
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 0, 255)); // rojo
                Barra_Status_Sesion_Client.show();
                delay(50);
                customTone(5, 1);
                delayMicroseconds(3350);
                customTone(5, 1);
                delayMicroseconds(2000);
                customTone(5, 1);
            }
            Handle_LED = false;
            break;

        case CARGA_CASHLESS_EXITOSA:
            Handle_LED = true;
            for (int i = 0; i < 8; i++)
            {
                Brig = random(20, 100);
                customTone(5, 1);

                R = random(1, 255);
                G = random(1, 255);
                B = random(1, 255);

                if(i==0)
                {
                    Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(R, 216, 230)); // azul claro
                    Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(173, 216, 230)); // azul claro
                    Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(173, 216, 230)); // azul claro
                    Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(173, 216, 230)); // azul claro
                    Barra_Status_Sesion_Client.setBrightness(Brig);
                    Barra_Status_Sesion_Client.show();
                }

                else if(i==1)
                {
                    Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(173, 216, 230)); // azul claro
                    Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(173,  G, 230)); // azul claro
                    Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(173, 216, 230)); // azul claro
                    Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(173, 216, 230)); // azul claro
                    Barra_Status_Sesion_Client.setBrightness(Brig);
                    Barra_Status_Sesion_Client.show();
                }


                else if(i==2)
                {
                    Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(173, 216, 230)); // azul claro
                    Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(173, 216, 230)); // azul claro
                    Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(173, 216, B));   // azul claro
                    Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(173, 216, 230)); // azul claro
                    Barra_Status_Sesion_Client.setBrightness(Brig);
                    Barra_Status_Sesion_Client.show();
                }else{
                    Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(173, 216, 230)); // azul claro
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(173, 216, 230)); // azul claro
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(173, 216, 230)); // azul claro
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(173, 216, 230)); // azul claro
                Barra_Status_Sesion_Client.setBrightness(Brig);
                Barra_Status_Sesion_Client.show();
                delay(50);
                Brig = random(20, 100);
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 0, 0)); // azul claro
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 0, 0)); // azul claro
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 0, 0)); // azul claro
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 0, 0)); // azul claro
                Barra_Status_Sesion_Client.setBrightness(Brig);
                Barra_Status_Sesion_Client.show();
                }
            }
            Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(0, 255, 0)); // azul claro
            Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(0, 255, 0)); // azul claro
            Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(0, 255, 0)); // azul claro
            Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(0, 255, 0)); // azul

            Brig=20;
            Barra_Status_Sesion_Client.setBrightness(20);
            Barra_Status_Sesion_Client.show();
            Handle_LED = false;
            break;
        case 300:
            Handle_LED = true;
            Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
            Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
            Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
            Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
            Barra_Status_Sesion_Client.show();
            customTone(5, 1);
            delayMicroseconds(1350);
            Handle_LED = false;
        break;

        case 301:
            Handle_LED = true;
            Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
            Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
            Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
            Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
            Barra_Status_Sesion_Client.show();
            break;


        case 302:
            Handle_LED = true;
            for (int i = 0; i < 3; i++)
            {

                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                Barra_Status_Sesion_Client.clear();
                Barra_Status_Sesion_Client.show();
                delay(50);
                Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(60, 255, 0)); // rojo
                Barra_Status_Sesion_Client.show();
                delay(50);

                customTone(5, 1);
                delayMicroseconds(100);
                customTone(1, 3);
                delayMicroseconds(500);
            }
            Handle_LED = false;
            break;

        case 500:
            Handle_LED=true; 
            Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
            Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
            Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
            Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(255, 0, 0)); // rojo
            Barra_Status_Sesion_Client.show();
            Handle_LED=false; 
        break;

        case WIFI_CONEXION_FAILED:

            Pines++;
            if (millis() - Encendido >= 500)
            {

                if (!LED)
                {
                    Host_++;
                    Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(255, 255, 255)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(255, 255, 255)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(255, 255, 255)); // rojo
                    Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(255, 255, 255)); // rojo
                    Barra_Status_Sesion_Client.show();
                    LED = true;
                    // if (Host_ < 8)
                    // {
                    //     customTone(5, 1);
                    // }
                }
                else
                {

                    Host_++;
                    // Apagar todos los LEDs
                    Barra_Status_Sesion_Client.clear();
                    Barra_Status_Sesion_Client.show();
                    LED = false;

                    // if (Host_ < 8)
                    // {
                    //     customTone(5, 1);
                    // }

                }

                Encendido = millis();
            }

            break;

        default:
            break;
        }
    }
}

std::string IP_toString_(char IP_Char[])
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


String IP_toString_String(char IP_Char[])
{
    String ipString = "";

    for (int i = 0; i < 4; ++i) {
        ipString += String(static_cast<uint8_t>(IP_Char[i])); // Convertir a uint8_t para evitar problemas con valores negativos
        if (i < 3) {
            ipString += ".";
        }
    }

    return ipString;
}


uint32_t ConvertirBytesASCIIaEntero(byte Id_Client[8])
{
    char buffer[9];
    for (int i = 0; i < 8; i++)
    {
        buffer[i] = (char)Id_Client[i];
    }
    buffer[8] = '\0'; // terminador de cadena

    return strtoul(buffer, nullptr, 10); // base 10
}

bool Cashless_API::ExitsCahlessID(void)
{
    NVS.begin("Config_ESP32", false);

    if (NVS.isKey("CashlessID"))
    {
        NVS.end();
        return true;
    }
    else
    {
        NVS.end();
        return false;
    }
}

bool Cashless_API::RemoveCashlessID()
{

    NVS.begin("Config_ESP32", false);
    if (NVS.isKey("CashlessID"))
    {
        NVS.remove("CashlessID");
        NVS.end();
        return true;
    }
    else
    {
        NVS.end();
        return true;
    }
}

int Cashless_API::GetCashlessID(int Parameter)
{
    int CashlessID;
    NVS.begin("Config_ESP32", false);

    CashlessID = NVS.getInt("CashlessID", Parameter);

    NVS.end();

    return CashlessID;
}

bool Cashless_API::SetCashlessID(String Type_Transaction)
{

    bool IsSuccess = false;

    if (Type_Transaction == "C")
    {
        if (Objeto_Transfer["Cashless_ID"].isNull() || !Objeto_Transfer.containsKey("Cashless_ID"))
        {
            size_t len;
            NVS.begin("Config_ESP32", false);
            len = NVS.putInt("CashlessID", 0);
            return false;
        }
        else
        {
            int CashlessID = Objeto_Transfer["Cashless_ID"];
            size_t len;
            NVS.begin("Config_ESP32", false);
            len = NVS.putInt("CashlessID", CashlessID);
            NVS.end();
        }
    }
    else
    {
        if (Objeto_Transfer_Download["Cashless_ID"].isNull() || !Objeto_Transfer_Download.containsKey("Cashless_ID"))
        {
            size_t len;
            NVS.begin("Config_ESP32", false);
            len = NVS.putInt("CashlessID", 0);
            return false;
        }
        else
        {
            int CashlessID = Objeto_Transfer_Download["Cashless_ID"];
            size_t len;
            NVS.begin("Config_ESP32", false);
            len = NVS.putInt("CashlessID", CashlessID);
            NVS.end();
        }
    }

    return true;
}

int Cashless_API::Info_Client(byte Id_Client[], ESP32Time RTC, String Type_Transaction, uint32_t Transaction_ID)
{

    
    int Code;

    int httpCode;

    char IP_Server[4];
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));

    std::string Ip=IP_toString_(IP_Server);
    String Ip_Server=String(Ip.c_str());
    String Puerto="9595";
    String fwurl = "http://"+Ip_Server+":"+Puerto+"/api/Cashless/Saldo";



    WiFiClient client;
    HTTPClient https;


    
    /* Crea Objeto*/

    StaticJsonDocument<450> jsonDocument;
    jsonDocument.clear();
    char Current_IP[4];
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    

    jsonDocument["IsSuccess"] = true;
    jsonDocument["Cliente_ID"] = contadores.Get_Client_ID_Transaccion_Int();
    jsonDocument["Trans_Tipo"] = Type_Transaction;
    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Trans_ID"] = Transaction_ID;
    jsonDocument["Ip"] = IP_toString_(Current_IP);
    jsonDocument["MAC"] = WiFi.macAddress();
    jsonDocument["Id_Maquina"] = 0;
    jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
    
    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 0:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 1:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 2:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;
    case 3:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;

    case 17:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;

    default:
        jsonDocument["Tipo_Maq"] = "";
        break;
    }


    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    https.setTimeout(15000);

    // Info_Cashless.Log(RTC,"ENVIA_SOLICITUD_INFO_CLIENTE_CARGA ",Json);
    
    if (https.begin(client, fwurl))
    {
        
        https.addHeader("Content-Type", "application/json");
        https.addHeader("hash",Info_Cashless.Get_Hash_Valido());
        https.addHeader("gmsec","GMaster");
        https.addHeader("Authorization", "Bearer " + Info_Cashless.Get_Token_Valido());
        
        httpCode = https.POST(Json);

        //Serial.println(httpCode);
         
        if (httpCode == HTTP_CODE_OK)
        {
            
            String Response = https.getString();

            #ifdef Debug_HTTPS
            Serial.println(Response);
            #endif
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, Response);

            if (error)
            {
#ifdef Debug_HTTPS
                Serial.println("Error Json Contadores ");
#endif
            }
            else
            {
                    bool IsSuccess = doc["IsSuccess"];
                    int Current_Cliente_ID_Server_Int = doc["Cliente_ID"];
                    String Type_Trans_Server = doc["Trans_Tipo"];
                    uint32_t Cashable_Server = doc["Saldo_Canjeable"];
                    uint32_t Restricted_Server = doc["Saldo_Restringido"];
                    uint32_t Non_Restricted_Server = doc["Saldo_No_Restringido"];
                    uint32_t Trans_ID_Server = doc["Trans_ID"];
                    String Data_Time_Response_Server = doc["Fecha_Hora"];
                    String Msgg=doc["Message"];
                    String Nombre_Cliente=doc["Cliente_Nombre"];

                    DisplayTFT.Set_Nombre_Cliente(Nombre_Cliente);
                    int Cashless_ID=0;
                    String Cashless_Estado=doc["Cashless_Estado"];
                    String Ip_Tarjeta=doc["Ip"];
                    String Key=doc["Key"];
                    String Mac=doc["MAC"];
                    String Trans_Estado=doc["Trans_Estado"];
                    String Tipo_Maq=doc["Tipo_Maq"];

                    if (Variables_globales.Get_Variable_Global(Conexion_TFT_Display) && Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
                    {
                        JsonVariant info = doc["InformacionPuntosPantalla"];

                        if (info.isNull())
                        {
                            Serial.println("ERROR: InformacionPuntosPantalla es NULL");
                        }
                        else
                        {
                            if (!info["Cliente_Nombre"].is<String>() ||
                                !info["Casino"].is<String>() ||
                                !info["Deno_Cashless"].is<float>() ||
                                !info["Total_Fide"].is<float>() ||
                                !info["Total_Bole"].is<float>() ||
                                !info["Nivel_Usuario"].is<String>() ||
                                !info["Actual_Fide"].is<float>() ||
                                !info["Actual_Bole"].is<float>())
                            {
                                Serial.println("ERROR: Faltan datos obligatorios o tipos incorrectos");
                            }
                            else
                            {
                                DisplayTFT.info.Usuario = info["Cliente_Nombre"].as<String>();
                                DisplayTFT.info.Casino = info["Casino"].as<String>();

                                DisplayTFT.info.DenoCashless = info["Deno_Cashless"].as<float>();
                                DisplayTFT.info.Total_Fide = info["Total_Fide"].as<float>();
                                DisplayTFT.info.Total_Bole = info["Total_Bole"].as<float>();
                                DisplayTFT.info.Nivel_Usuario = info["Nivel_Usuario"].as<String>();
                                DisplayTFT.info.Actual_Fide = info["Actual_Fide"].as<float>();
                                DisplayTFT.info.Actual_Bole = info["Actual_Bole"].as<float>();
                                
                                // Serial.println(DisplayTFT.info.Usuario);
                                // Serial.println(DisplayTFT.info.Casino);
                                // Serial.println(DisplayTFT.info.Total_Fide);
                                //Serial.println(DisplayTFT.info.Total_Bole);
                                // Serial.println(DisplayTFT.info.Nivel_Usuario);
                                // Serial.println(DisplayTFT.info.Actual_Fide);
                                // Serial.println(DisplayTFT.info.Actual_Bole);
                            }
                        }
                    }

                    if (Msgg.indexOf("Ya existe una transacción en proceso") != -1)
                    {
                        //Serial.println(" Existe palabra.....");
                    }
                    else
                    {
                        //Serial.println(Msgg);
                        Cashless_ID = doc["Cashless_ID"];
                        Objeto_Transfer["Cashless_ID"] = Cashless_ID;
                    }

                    int Year, Month, Day, Hour, Minutes, Seconds;
                    sscanf(Data_Time_Response_Server.c_str(), "%d-%d-%d %d:%d:%d", &Year, &Month, &Day, &Hour, &Minutes, &Seconds);

                    Objeto_Transfer["IsSuccess"] = false;
                    Objeto_Transfer["Cliente_ID"] = Current_Cliente_ID_Server_Int;
                    Objeto_Transfer["Trans_Tipo"] = Type_Trans_Server;
                    Objeto_Transfer["Saldo_Canjeable"] = Cashable_Server;
                    Objeto_Transfer["Saldo_Restringido"] = Restricted_Server;
                    Objeto_Transfer["Saldo_No_Restringido"] = Non_Restricted_Server;
                    Objeto_Transfer["Trans_ID"] = Trans_ID_Server;
                    Objeto_Transfer["Fecha_Hora"] = Data_Time_Response_Server;
                    Objeto_Transfer["Message"] = Msgg;
                    Objeto_Transfer["Cliente_Nombre"] = Nombre_Cliente;
                    
                    Objeto_Transfer["Cashless_Estado"] = Cashless_Estado;
                    Objeto_Transfer["Ip"] = Ip_Tarjeta;
                    Objeto_Transfer["Key"] = Key;
                    Objeto_Transfer["MAC"] = Mac;
                    Objeto_Transfer["Trans_Estado"] = Trans_Estado;
                    Objeto_Transfer["Tipo_Maq"]=Tipo_Maq;

                    String Tipo_Maq_Local="";
                    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
                    {
                    case 0:
                        Tipo_Maq_Local="AFT";
                        break;
                    case 1:
                        Tipo_Maq_Local="AFT";
                        break;
                    case 2:
                        Tipo_Maq_Local="EFT";
                        break;
                    case 3:
                        Tipo_Maq_Local="AFT";
                        break;

                    case 5:
                        Tipo_Maq_Local="AFT";
                        break;

                    case 17:
                        Tipo_Maq_Local = "EFT";
                    break;

                    default:
                        Tipo_Maq_Local="";
                        break;
                    }

                    // Info_Cashless.Log(RTC,"SOLICITUD_INFO_CLIENTE_CARGA_RECIBIDA ",String(Response));

                    if (IsSuccess)
                    {

                        if(WiFi.status()!=WL_CONNECTED)
                        {
                            // Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(NOT_WIFI_CONNECTION,HEX),archivo,ERROR_);
                            Code= NOT_WIFI_CONNECTION; /* Envia ACK */
                        }
                        else if(Tipo_Maq!=Tipo_Maq_Local)
                        {
                            // Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(TYPE_MACHINE_NOT_MACTH,HEX),archivo,ERROR_);
                            Code=TYPE_MACHINE_NOT_MACTH; /* Envia ACK */
                        }
                        else if(!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                        {
                            // Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(NOT_COMMUNICATION_WITH_THE_MACHINE,HEX),archivo,ERROR_);
                            Code=NOT_COMMUNICATION_WITH_THE_MACHINE; /* Envia ACK */
                        }
                       
                        else if (Current_Cliente_ID_Server_Int != contadores.Get_Client_ID_Transaccion_Int())
                        {
                            // Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(CLIENT_NOT_MACTH,HEX),archivo,ERROR_);
                            Code = CLIENT_NOT_MACTH; /* Envia ACK */
                        }

                        else if (Type_Trans_Server != Type_Transaction)
                        {
                            // Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(TYPE_TRANS_NOT_MACTH,HEX),archivo,ERROR_);
                            Code = TYPE_TRANS_NOT_MACTH; /* Envia ACK */
                        }

                        else if (Trans_ID_Server != Transaction_ID)  
                        {
                            // Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(TRANS_ID_NO_MACTH,HEX),archivo,ERROR_);
                            Code = TRANS_ID_NO_MACTH; /* Envia ACK */
                        }

                        else if (!doc["Saldo_Canjeable"].is<uint32_t>() || !doc["Saldo_Restringido"].is<uint32_t>() || !doc["Saldo_No_Restringido"].is<uint32_t>())
                        {
                            // Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(INVALID_BALANCE,HEX),archivo,ERROR_);
                            Code = INVALID_BALANCE; /* Envia ACK */
                        }
                            

                        else if (Cashable_Server == 0 && Restricted_Server == 0 && Non_Restricted_Server == 0)
                        {
                            if(Cashless.Set_Amount_To_Load(Cashable_Server,Restricted_Server,Non_Restricted_Server))
                                Code = REQUEST_SUCCESSFULLY_RECEIVED; /* OK */
                            else
                                {
                                    // Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(INVALID_BALANCE,HEX),archivo,ERROR_);
                                    Code=INVALID_BALANCE; /* Envia ACK */
                                }
                               
                        }
                        else
                        {
                            if(Cashless.Set_Amount_To_Load(Cashable_Server,Restricted_Server,Non_Restricted_Server))
                                Code = REQUEST_SUCCESSFULLY_RECEIVED; /* OK */
                            else
                                Code=INVALID_BALANCE; /* Envia ACK */       
                        }
                    }
                    else
                    {
                        // Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(PROBLEM_WITH_THE_SERVER,HEX),archivo,ERROR_);
                        Code = PROBLEM_WITH_THE_SERVER; /* Error en servidor HTTP */
                    }
            }

            doc.clear();
        }
        else
        {
            // Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(httpCode),archivo,ERROR_);

            if(httpCode==401)
                Code=401;
            else
                Code=NOT_CONEXION_WITH_SERVER; /* No se logro establecer comunicación con el servidor HTTP */

            
        }
        https.end();

      //  Serial.println(Code);
        return Code;
    }
    // Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(NOT_CONEXION_WITH_SERVER,HEX),archivo,ERROR_);
    Code=NOT_CONEXION_WITH_SERVER;
    return Code;
}


int Cashless_API::Info_Client_Download(byte Id_Client[], ESP32Time RTC, String Type_Transaction, uint32_t Transaction_ID,int ClientID,int Operacion)
{

    int Code;
    int httpCode;
    
    char IP_Server[4];
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));

    std::string Ip=IP_toString_(IP_Server);
    String Ip_Server=String(Ip.c_str());
    String Puerto="9595";
    String fwurl = "http://"+Ip_Server+":"+Puerto+"/api/Cashless/Saldo";

    WiFiClient client;
    HTTPClient https;
    
    /* Crea Objeto*/

    StaticJsonDocument<450> jsonDocument;
    jsonDocument.clear();
    char Current_IP[4];
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    
    jsonDocument["IsSuccess"] = true;
    jsonDocument["Cliente_ID"] = ClientID;
    jsonDocument["Trans_Tipo"] = Type_Transaction;
    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Trans_ID"] = Transaction_ID;
    jsonDocument["Ip"] = IP_toString_(Current_IP);
    jsonDocument["MAC"] = WiFi.macAddress();
    jsonDocument["Id_Maquina"] = 0;
    jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();

    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 0:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 1:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 2:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;
    case 3:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;

    case 17:
        jsonDocument["Tipo_Maq"] = "EFT";
    break;
    
    default:
        jsonDocument["Tipo_Maq"] = "";
        break;
    }

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    
    https.setTimeout(15000);

   // Info_Cashless.Log(RTC,"ENVIA_SOLICITUD_INFO_CLIENTE_DESCARGA ",Json);

    if (https.begin(client, fwurl))
    {
       
        https.addHeader("Content-Type", "application/json");
        https.addHeader("hash",Info_Cashless.Get_Hash_Valido());
        https.addHeader("gmsec","GMaster");
        https.addHeader("Authorization", "Bearer " + Info_Cashless.Get_Token_Valido()); // Agrega el token de autorización

        httpCode = https.POST(Json);

        // Serial.println(httpCode);
        if (httpCode == HTTP_CODE_OK)
        {

            String Response = https.getString();
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, Response);
            if (error)
            {
#ifdef Debug_HTTPS
                Serial.println("Error Json Contadores ");
#endif
            }
            else
            {
                    bool IsSuccess = doc["IsSuccess"];
                    int Current_Cliente_ID_Server_Int = doc["Cliente_ID"];
                    String Type_Trans_Server = doc["Trans_Tipo"];
                    uint32_t Cashable_Server = doc["Saldo_Canjeable"];
                    uint32_t Restricted_Server = doc["Saldo_Restringido"];
                    uint32_t Non_Restricted_Server = doc["Saldo_No_Restringido"];
                    uint32_t Trans_ID_Server = doc["Trans_ID"];
                    String Data_Time_Response_Server = doc["Fecha_Hora"];
                    String Msgg=doc["Message"];
                    String Nombre_Cliente=doc["Cliente_Nombre"];
                    int Cashless_ID=0; 
                    String Cashless_Estado=doc["Cashless_Estado"];
                    String Ip_Tarjeta=doc["Ip"];
                    String Key=doc["Key"];
                    String Mac=doc["MAC"];
                    int Trans_Estado;

                    if (Variables_globales.Get_Variable_Global(Conexion_TFT_Display) && Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
                    {
                        JsonVariant info = doc["InformacionPuntosPantalla"];

                        if (info.isNull())
                        {
                            Serial.println("ERROR: InformacionPuntosPantalla es NULL");
                        }
                        else
                        {
                            if (!info["Cliente_Nombre"].is<String>() ||
                                !info["Casino"].is<String>() ||
                                !info["Deno_Cashless"].is<float>() ||
                                !info["Total_Fide"].is<float>() ||
                                !info["Total_Bole"].is<float>() ||
                                !info["Nivel_Usuario"].is<String>() ||
                                !info["Actual_Fide"].is<float>() ||
                                !info["Actual_Bole"].is<float>())
                            {
                                Serial.println("ERROR: Faltan datos obligatorios o tipos incorrectos");
                            }
                            else
                            {
                                DisplayTFT.info.Usuario = info["Cliente_Nombre"].as<String>();
                                DisplayTFT.info.Casino = info["Casino"].as<String>();

                                DisplayTFT.info.DenoCashless = info["Deno_Cashless"].as<float>();
                                DisplayTFT.info.Total_Fide = info["Total_Fide"].as<float>();
                                DisplayTFT.info.Total_Bole = info["Total_Bole"].as<float>();
                                DisplayTFT.info.Nivel_Usuario = info["Nivel_Usuario"].as<String>();
                                DisplayTFT.info.Actual_Fide = info["Actual_Fide"].as<float>();
                                DisplayTFT.info.Actual_Bole = info["Actual_Bole"].as<float>();

                                // Serial.println(DisplayTFT.info.Usuario);
                                // Serial.println(DisplayTFT.info.Casino);
                                // Serial.println(DisplayTFT.info.Total_Fide);
                                // Serial.println(DisplayTFT.info.Total_Bole);
                                // Serial.println(DisplayTFT.info.Nivel_Usuario);
                                // Serial.println(DisplayTFT.info.Actual_Fide);
                                // Serial.println(DisplayTFT.info.Actual_Bole);
                            }
                        }
                    }

                    if (Msgg.indexOf("Ya existe una transacción en proceso") != -1)
                    {
                        //Serial.println(" Existe palabra.....");
                    }
                    else
                    {
                        //Serial.println(Msgg);
                        Cashless_ID =doc["Cashless_ID"];
                        Objeto_Transfer_Download["Cashless_ID"] = Cashless_ID;
                    }

                    if(!doc.containsKey("Trans_Estado")||doc["Trans_Estado"].isNull()||doc["Trans_Estado"]=="null")
                        Trans_Estado=-1;
                    else 
                        Trans_Estado=doc["Trans_Estado"];


                    String Tipo_Maq=doc["Tipo_Maq"];

                    bool test=false;
                    if(!doc.containsKey("Ip")||!doc.containsKey("Cliente_ID")||!doc.containsKey("Key")||!doc.containsKey("MAC")||Current_Cliente_ID_Server_Int<=0)
                        test=true;


                    //Info_Cashless.Log(RTC,"SOLICITUD_INFO_CLIENTE_DESCARGA_RECIBIDA",String(Response));
                   
                    int Year, Month, Day, Hour, Minutes, Seconds;
                    sscanf(Data_Time_Response_Server.c_str(), "%d-%d-%d %d:%d:%d", &Year, &Month, &Day, &Hour, &Minutes, &Seconds);
                    
                    Objeto_Transfer_Download["IsSuccess"] = false; /* Default*/
                    Objeto_Transfer_Download["Cliente_ID"] = Current_Cliente_ID_Server_Int;
                    Objeto_Transfer_Download["Trans_Tipo"] = Type_Trans_Server;
                    Objeto_Transfer_Download["Saldo_Canjeable"] = Cashable_Server;
                    Objeto_Transfer_Download["Saldo_Restringido"] = Restricted_Server;
                    Objeto_Transfer_Download["Saldo_No_Restringido"] = Non_Restricted_Server;
                    Objeto_Transfer_Download["Trans_ID"] = Trans_ID_Server;
                    Objeto_Transfer_Download["Fecha_Hora"] = Data_Time_Response_Server;
                    Objeto_Transfer_Download["Message"] = Msgg;
                    Objeto_Transfer_Download["Cliente_Nombre"] = Nombre_Cliente;
                    //Objeto_Transfer_Download["Cashless_ID"] = Cashless_ID;
                    Objeto_Transfer_Download["Cashless_Estado"] = Cashless_Estado;
                    Objeto_Transfer_Download["Ip"] = Ip_Tarjeta;
                    Objeto_Transfer_Download["Key"] = Key;
                    Objeto_Transfer_Download["MAC"] = Mac;
                    Objeto_Transfer_Download["Trans_Estado"] = Trans_Estado;
                    Objeto_Transfer_Download["Tipo_Maq"]=Tipo_Maq;

                    if (IsSuccess)
                    {

                        if (Objeto_Transfer_Download["Cliente_ID"]<=0||test)
                        {
                            // Info_Cashless.Log(RTC, "ERROR_SOLICITUD_INFO_CLIENTE_DESCARGA", String(PROBLEM_WITH_THE_SERVER, HEX),archivo,ERROR_);
                            Code = PROBLEM_WITH_THE_SERVER; /* Error en servidor HTTP */
                        }
                        else
                            Code = REQUEST_SUCCESSFULLY_RECEIVED; /* OK */
                    }
                    else
                    {
                        /* Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_DESCARGA",String(PROBLEM_WITH_THE_SERVER,HEX),archivo,ERROR_); */
                        Code = PROBLEM_WITH_THE_SERVER; /* Error en servidor HTTP */
                    }
            }

            doc.clear();
        }
        else
        {
            if(httpCode==HTTP_CODE_UNAUTHORIZED)
                Code=HTTP_CODE_UNAUTHORIZED;
            else
                Code=NOT_CONEXION_WITH_SERVER; /* No se logro establecer comunicación con el servidor HTTP */

            // Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_DESCARGA",String(httpCode),archivo,ERROR_);
            
        }
        https.end();
        return Code;
    }

    // Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_DESCARGA",String(NOT_CONEXION_WITH_SERVER,HEX),archivo,ERROR_);
    Code=NOT_CONEXION_WITH_SERVER;
    return Code;
}

uint32_t Convert_5BCD_To_Int(char Buffer_5BCD[],int filtro) {
    uint32_t creditNumber=0;
    char str[11];
    switch (filtro)
    {
    case 1:
        for (int i = 0; i < 5; i++)
        {
           creditNumber = creditNumber * 100 + ((Buffer_5BCD[i] >> 4) * 10) + (Buffer_5BCD[i] & 0x0F);
        }

        // Dividir por 100 para obtener el valor en pesos
        creditNumber /= 100;
        return creditNumber;
        break;


    case 2:
        creditNumber += Buffer_5BCD[16];         // Decenas de millar
        creditNumber += Buffer_5BCD[15] * 10;    // Unidades de millar
        creditNumber += Buffer_5BCD[14] * 100;   // Centenas
        creditNumber += Buffer_5BCD[13] * 1000;  // Decenas
        creditNumber += Buffer_5BCD[12] * 10000; // Unidades
            return creditNumber;
        break;

    case 3:
        creditNumber += Buffer_5BCD[21];         // Decenas de millar
        creditNumber += Buffer_5BCD[20] * 10;    // Unidades de millar
        creditNumber += Buffer_5BCD[19] * 100;   // Centenas
        creditNumber += Buffer_5BCD[18] * 1000;  // Decenas
        creditNumber += Buffer_5BCD[17] * 10000; // Unidades
            return creditNumber;
        break;
    
    default:
        return creditNumber;
        break;
    }
}

/* Actualiza objeto  para reporte de transacciones de carga */
bool Cashless_API::Status_Transfer(int Code, char Buffer_Transfer[], ESP32Time RTC)
{

    char Current_IP[4];
    String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));


    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17) /* Maquinas EFT */
    {
        /* Actualiza Objeto Carga */
        if (Code == 0x00)
            Objeto_Transfer["IsSuccess"] = true;
        else
            Objeto_Transfer["IsSuccess"] = false;
    }
    else
    {
        /* Actualiza Objeto Carga */
        if (Code == 0x00 || Code == 0x01)
            Objeto_Transfer["IsSuccess"] = true;
        else
            Objeto_Transfer["IsSuccess"] = false;
    }


    Objeto_Transfer["Ip"] = IP_toString_(Current_IP);
    Objeto_Transfer["MAC"] = WiFi.macAddress();
    Objeto_Transfer["Id_Maquina"] = 0;
    Objeto_Transfer["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();

    if(ExitsCahlessID())
        Objeto_Transfer["Cashless_ID"]=GetCashlessID(0);

    Objeto_Transfer["Cliente_ID"] = contadores.Get_Client_ID_Transaccion_Int();
    Objeto_Transfer["Trans_Tipo"] = "C";

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17) /* Maquinas EFT */
    {
        Objeto_Transfer["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 4);
        Objeto_Transfer["Saldo_Restringido"] = 0;
        Objeto_Transfer["Saldo_No_Restringido"] = 0;
        Objeto_Transfer["Trans_ID"] = Cashless.Get_Trans_ID_EFT();
        Objeto_Transfer["Tipo_Maq"] = "EFT";
    }
    else /* Maquinas AFT */
    {
        Objeto_Transfer["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 7);
        Objeto_Transfer["Saldo_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, RESTRICTED, 12);
        Objeto_Transfer["Saldo_No_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, NON_RESTRICTED, 17);
        Objeto_Transfer["Trans_ID"] = Cashless.Get_Trans_ID_Int();
        Objeto_Transfer["Tipo_Maq"] = "AFT";
    }

    Objeto_Transfer["Fecha_Hora"] = DataTime;
    Objeto_Transfer["Trans_Estado"] = Code;

    String Json;
    serializeJson(Objeto_Transfer, Json); /* Serializa Data */

    // Info_Cashless.Log(RTC,"OBJETO_TRANSFER_ACTUALIZADO",Json);

    Info_Cashless.Reader_Lock(false);
    Info_Cashless.Reader_Lock(false);
    Info_Cashless.Reader_Lock(false);
    Info_Cashless.Reader_Lock(false);
    
    hayTransaccionesPendientes = true;
    // Serial.println(Json);
    if (Code == 0x00 || Code == 0x01)
    {
        Saves_Current_Player_Sesion(contadores.Get_Client_ID_Transaccion(), Info_Cashless.Type_Sesion());

        /* Aqui iniciar sesion en pantalla */

        // uint32_t Canjeable = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 7);
        // uint32_t Saldo_No_Restringido = Cashless.BCDtoUint32_Pos(Buffer_Transfer, NON_RESTRICTED, 17);
        // uint32_t Saldo_Restringido = Cashless.BCDtoUint32_Pos(Buffer_Transfer, RESTRICTED, 12);

        if (Variables_globales.Get_Variable_Global(Conexion_TFT_Display) && Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
        {
            if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
            {
                DisplayTFT.info.Saldo_Canjeable = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 4);
                DisplayTFT.info.Saldo_No_Restrindigo = 0;
                DisplayTFT.info.Saldo_Restringido = 0;
            }
            else
            {

                DisplayTFT.info.Saldo_Canjeable = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 7);
                DisplayTFT.info.Saldo_No_Restrindigo = Cashless.BCDtoUint32_Pos(Buffer_Transfer, NON_RESTRICTED, 17);
                DisplayTFT.info.Saldo_Restringido = Cashless.BCDtoUint32_Pos(Buffer_Transfer, RESTRICTED, 12);
            }

            DisplayTFT.info.Actualiza_Saldos_Iniciales = true;
        }

        // DisplayTFT.Actualiza_Saldos_TFT_Globus_IM(DisplayTFT.info.Casino, Canjeable, Saldo_No_Restringido, Saldo_Restringido);
        
        // DisplayTFT.Actualiza_Puntos_TFT_Globus_IM(DisplayTFT.info.Usuario,DisplayTFT.info.Casino,NivelUsuarioInt(DisplayTFT.info.Nivel_Usuario),DisplayTFT.info.Total_Fide,DisplayTFT.info.Actual_Bole,DisplayTFT.info.Actual_Fide,DisplayTFT.info.Actual_Bole);
    }

    

    return true;
}

/*Actualiza objeto  para reporte de  estado  pendiente de transaccion*/
bool Cashless_API::Ack_Transfer_Pending(int Code, char Buffer_Transfer[], ESP32Time RTC,String Type_Transaccion)
{

    if (Type_Transaccion == LOAD_TRANSACTION)
    {
        String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

        if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2|| Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
        {

            /* Actualiza Objeto Carga */
            Objeto_Transfer["IsSuccess"] = false;
            Objeto_Transfer["Cliente_ID"] = contadores.Get_Client_ID_Transaccion_Int();
            Objeto_Transfer["Trans_Tipo"] = "C";
            Objeto_Transfer["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 5);
            Objeto_Transfer["Saldo_Restringido"] = 0;
            Objeto_Transfer["Saldo_No_Restringido"] = 0;
            Objeto_Transfer["Trans_ID"] = Cashless.Get_Trans_ID_EFT();
            Objeto_Transfer["Fecha_Hora"] = DataTime;
            Objeto_Transfer["Trans_Estado"] = Code;
            Objeto_Transfer["Tipo_Maq"] ="EFT";
        }
        else
        {
            /* Actualiza Objeto Carga */
            Objeto_Transfer["IsSuccess"] = false;
            Objeto_Transfer["Cliente_ID"] = contadores.Get_Client_ID_Transaccion_Int();
            Objeto_Transfer["Trans_Tipo"] = "C";
            Objeto_Transfer["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 7);
            Objeto_Transfer["Saldo_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, RESTRICTED, 12);
            Objeto_Transfer["Saldo_No_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, NON_RESTRICTED, 17);
            Objeto_Transfer["Trans_ID"] = Cashless.Get_Trans_ID_Int();
            Objeto_Transfer["Fecha_Hora"] = DataTime;
            Objeto_Transfer["Trans_Estado"] = Code;
            Objeto_Transfer["Tipo_Maq"] ="AFT";
        }

        Transfer_Pending_Load = true;
    }
    else
    {

        char Current_IP[4];
        memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

        Objeto_Transfer_Download["Ip"] = IP_toString_(Current_IP);
        Objeto_Transfer_Download["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
        Objeto_Transfer_Download["MAC"] = WiFi.macAddress();

        if(ExitsCahlessID())
            Objeto_Transfer_Download["Cashless_ID"]=GetCashlessID(0);

        if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
        {
            String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());
            Objeto_Transfer_Download["IsSuccess"] = false;
            Objeto_Transfer_Download["Cliente_ID"] = contadores.Get_Client_ID_Transaccion_Int();
            Objeto_Transfer_Download["Trans_Tipo"] = "D";
            Objeto_Transfer_Download["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 5);
            Objeto_Transfer_Download["Saldo_Restringido"] = 0;
            Objeto_Transfer_Download["Saldo_No_Restringido"] = 0;
            Objeto_Transfer_Download["Trans_ID"] = Cashless.Get_Trans_ID_EFT();
            Objeto_Transfer_Download["Fecha_Hora"] = DataTime;
            Objeto_Transfer_Download["Trans_Estado"] = Code;
            Objeto_Transfer_Download["Tipo_Maq"]= "EFT";

        }
        else
        {
            String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());
            Objeto_Transfer_Download["IsSuccess"] = false;
            Objeto_Transfer_Download["Cliente_ID"] = contadores.Get_Client_ID_Transaccion_Int();
            Objeto_Transfer_Download["Trans_Tipo"] = "D";
            Objeto_Transfer_Download["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 12);
            Objeto_Transfer_Download["Saldo_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, RESTRICTED, 17);
            Objeto_Transfer_Download["Saldo_No_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, NON_RESTRICTED, 22);
            Objeto_Transfer_Download["Trans_ID"] = Cashless.Get_Trans_ID_Int();
            Objeto_Transfer_Download["Fecha_Hora"] = DataTime;
            Objeto_Transfer_Download["Trans_Estado"] = Code;
            Objeto_Transfer_Download["Tipo_Maq"]= "AFT";
        }

        Transfer_Pending_Download=true;
    }
    return true;
}

bool Cashless_API::Ack_Transfer_Pending_Pendiente(int Code, char Buffer_Transfer[], ESP32Time RTC,String Type_Transaccion, char Saldos[])
{

    if (Type_Transaccion == LOAD_TRANSACTION)
    {
        String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

        if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2|| Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
        {

            /* Actualiza Objeto Carga */
            Objeto_Transfer["IsSuccess"] = false;
            Objeto_Transfer["Cliente_ID"] = contadores.Get_Client_ID_Transaccion_Int();
            Objeto_Transfer["Trans_Tipo"] = "C";
            Objeto_Transfer["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Saldos, CASHABLES, 0);
            Objeto_Transfer["Saldo_Restringido"] = 0;
            Objeto_Transfer["Saldo_No_Restringido"] = 0;
            Objeto_Transfer["Trans_ID"] = Cashless.Get_Trans_ID_EFT();
            Objeto_Transfer["Fecha_Hora"] = DataTime;
            Objeto_Transfer["Trans_Estado"] = Code;
            Objeto_Transfer["Tipo_Maq"] ="EFT";
        }
        else
        {
            /* Actualiza Objeto Carga */
            Objeto_Transfer["IsSuccess"] = false;
            Objeto_Transfer["Cliente_ID"] = contadores.Get_Client_ID_Transaccion_Int();
            Objeto_Transfer["Trans_Tipo"] = "C";
            Objeto_Transfer["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Saldos, CASHABLES, 0);
            Objeto_Transfer["Saldo_Restringido"] = Cashless.BCDtoUint32_Pos(Saldos, RESTRICTED, 5);
            Objeto_Transfer["Saldo_No_Restringido"] = Cashless.BCDtoUint32_Pos(Saldos, NON_RESTRICTED, 10);
            Objeto_Transfer["Trans_ID"] = Cashless.Get_Trans_ID_Int();
            Objeto_Transfer["Fecha_Hora"] = DataTime;
            Objeto_Transfer["Trans_Estado"] = Code;
            Objeto_Transfer["Tipo_Maq"] ="AFT";
        }

        Transfer_Pending_Load = true;
    }
    else
    {

        char Current_IP[4];
        memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

        Objeto_Transfer_Download["Ip"] = IP_toString_(Current_IP);
        Objeto_Transfer_Download["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
        Objeto_Transfer_Download["MAC"] = WiFi.macAddress();

        if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
        {
            String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());
            Objeto_Transfer_Download["IsSuccess"] = false;
            Objeto_Transfer_Download["Cliente_ID"] = contadores.Get_Client_ID_Transaccion_Int();
            Objeto_Transfer_Download["Trans_Tipo"] = "D";
            Objeto_Transfer_Download["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Saldos, CASHABLES, 0);
            Objeto_Transfer_Download["Saldo_Restringido"] = 0;
            Objeto_Transfer_Download["Saldo_No_Restringido"] = 0;
            Objeto_Transfer_Download["Trans_ID"] = Cashless.Get_Trans_ID_EFT();
            Objeto_Transfer_Download["Fecha_Hora"] = DataTime;
            Objeto_Transfer_Download["Trans_Estado"] = Code;
            Objeto_Transfer_Download["Tipo_Maq"]= "EFT";

        }
        else
        {
            String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());
            Objeto_Transfer_Download["IsSuccess"] = false;
            Objeto_Transfer_Download["Cliente_ID"] = contadores.Get_Client_ID_Transaccion_Int();
            Objeto_Transfer_Download["Trans_Tipo"] = "D";
            Objeto_Transfer_Download["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Saldos, CASHABLES, 0);
            Objeto_Transfer_Download["Saldo_Restringido"] = Cashless.BCDtoUint32_Pos(Saldos, RESTRICTED, 5);
            Objeto_Transfer_Download["Saldo_No_Restringido"] = Cashless.BCDtoUint32_Pos(Saldos, NON_RESTRICTED, 10);
            Objeto_Transfer_Download["Trans_ID"] = Cashless.Get_Trans_ID_Int();
            Objeto_Transfer_Download["Fecha_Hora"] = DataTime;
            Objeto_Transfer_Download["Trans_Estado"] = Code;
            Objeto_Transfer_Download["Tipo_Maq"]= "AFT";
        }

        Transfer_Pending_Download=true;
    }
    return true;
}

/* Actualiza objeto  para reporte de transacciones de descarga  */
bool Cashless_API::Status_Transfer_Download(int Code, char Buffer_Transfer[], ESP32Time RTC,int Id_Client)
{

    String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

    char Current_IP[4];
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
    {
        if (Code == 0x00)
            Objeto_Transfer_Download["IsSuccess"] = true;
        else
            Objeto_Transfer_Download["IsSuccess"] = false;
    }
    else
    {
        if (Code == 0x00 || Code == 0x01)
            Objeto_Transfer_Download["IsSuccess"] = true;
        else
            Objeto_Transfer_Download["IsSuccess"] = false;
    }

    Objeto_Transfer_Download["Ip"] = IP_toString_(Current_IP);
    Objeto_Transfer_Download["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
    Objeto_Transfer_Download["MAC"] = WiFi.macAddress();

    Objeto_Transfer_Download["Cliente_ID"] = Id_Client;
    Objeto_Transfer_Download["Trans_Tipo"] = "D";


    if(ExitsCahlessID())
        Objeto_Transfer_Download["Cashless_ID"]=GetCashlessID(0);

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
    {
        Objeto_Transfer_Download["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 5);
        Objeto_Transfer_Download["Saldo_Restringido"] = 0;
        Objeto_Transfer_Download["Saldo_No_Restringido"] = 0;
        Objeto_Transfer_Download["Trans_ID"] = Cashless.Get_Trans_ID_EFT();
        Objeto_Transfer_Download["Fecha_Hora"] = DataTime;
        Objeto_Transfer_Download["Trans_Estado"] = Code;
        Objeto_Transfer_Download["Tipo_Maq"] = "EFT";
    }
    else
    {
        Objeto_Transfer_Download["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 12);
        Objeto_Transfer_Download["Saldo_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, RESTRICTED, 17);
        Objeto_Transfer_Download["Saldo_No_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, NON_RESTRICTED, 22);
        Objeto_Transfer_Download["Trans_ID"] = Cashless.Get_Trans_ID_Int();
        Objeto_Transfer_Download["Fecha_Hora"] = DataTime;
        Objeto_Transfer_Download["Trans_Estado"] = Code;
        Objeto_Transfer_Download["Tipo_Maq"] = "AFT";
    }

    Info_Cashless.Reader_Lock(false);
    Info_Cashless.Reader_Lock(false);
    Info_Cashless.Reader_Lock(false);
    Info_Cashless.Reader_Lock(false);
    Sesion_Anterior = true;
    String Json;
    serializeJson(Objeto_Transfer_Download, Json); /* Serializa Data */

    // Info_Cashless.Log(RTC,"OBJETO_TRANSFER_DOWNLOAD_ACTUALIZADO",Json);

    hayTransaccionesPendientes_Download = true;
    // Serial.println(Json);
    if (Code == 0x00 || Code == 0x01)
    {
        Info_Cashless.Remove_Currrent_Player_Sesion();

        // uint32_t Saldo_Canjeable;
        // uint32_t Saldo_No_Restringido;
        // uint32_t Saldo_Restrringido;

        if (Variables_globales.Get_Variable_Global(Conexion_TFT_Display) && Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
        {

            if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
            {
                // Saldo_Canjeable=Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 5);
                // Saldo_No_Restringido=Cashless.BCDtoUint32_Pos(Buffer_Transfer, RESTRICTED, 17);
                // Saldo_Restrringido=Cashless.BCDtoUint32_Pos(Buffer_Transfer, NON_RESTRICTED, 22);
                DisplayTFT.info.Saldo_Canjeable = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 5);
                DisplayTFT.info.Saldo_Restringido = 0;
                DisplayTFT.info.Saldo_No_Restrindigo = 0;
            }
            else
            {
                DisplayTFT.info.Saldo_Canjeable = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 12);
                DisplayTFT.info.Saldo_Restringido = Cashless.BCDtoUint32_Pos(Buffer_Transfer, RESTRICTED, 17);
                DisplayTFT.info.Saldo_No_Restrindigo = Cashless.BCDtoUint32_Pos(Buffer_Transfer, NON_RESTRICTED, 22);
            }

            DisplayTFT.info.Actualiza_Saldos_Finales=true;
        }

        //DisplayTFT.Cierra_Sesion_Player_Cashless_TFT_Globus_IM(DisplayTFT.info.Casino,Saldo_Canjeable,Saldo_No_Restringido,Saldo_Restrringido);
        // Close_Player_TFT(Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 12), Cashless.BCDtoUint32_Pos(Buffer_Transfer, NON_RESTRICTED, 22), Cashless.BCDtoUint32_Pos(Buffer_Transfer, RESTRICTED, 17),0);
    }

    return true;
}

bool Cashless_API::Status_Transfer_Download_Pendiente_AFT(int Code, char Buffer_Transfer[], ESP32Time RTC, int Id_Client, char Saldos[15])
{

    String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

    char Current_IP[4];
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
    {
        if (Code == 0x00)
            Objeto_Transfer_Download["IsSuccess"] = true;
        else
            Objeto_Transfer_Download["IsSuccess"] = false;
    }
    else
    {
        if (Code == 0x00 || Code == 0x01)
            Objeto_Transfer_Download["IsSuccess"] = true;
        else
            Objeto_Transfer_Download["IsSuccess"] = false;
    }

    Objeto_Transfer_Download["Ip"] = IP_toString_(Current_IP);
    Objeto_Transfer_Download["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
    Objeto_Transfer_Download["MAC"] = WiFi.macAddress();

    Objeto_Transfer_Download["Cliente_ID"] = Id_Client;
    Objeto_Transfer_Download["Trans_Tipo"] = "D";

    if (ExitsCahlessID())
        Objeto_Transfer_Download["Cashless_ID"] = GetCashlessID(0);

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
    {
        Objeto_Transfer_Download["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Saldos, CASHABLES, 0);
        Objeto_Transfer_Download["Saldo_Restringido"] = 0;
        Objeto_Transfer_Download["Saldo_No_Restringido"] = 0;
        Objeto_Transfer_Download["Trans_ID"] = Cashless.Get_Trans_ID_EFT();
        Objeto_Transfer_Download["Fecha_Hora"] = DataTime;
        Objeto_Transfer_Download["Trans_Estado"] = Code;
        Objeto_Transfer_Download["Tipo_Maq"] = "EFT";
    }
    else
    {
        Objeto_Transfer_Download["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Saldos, CASHABLES, 0);
        Objeto_Transfer_Download["Saldo_Restringido"] = Cashless.BCDtoUint32_Pos(Saldos, RESTRICTED, 5);
        Objeto_Transfer_Download["Saldo_No_Restringido"] = Cashless.BCDtoUint32_Pos(Saldos, NON_RESTRICTED, 10);
        Objeto_Transfer_Download["Trans_ID"] = Cashless.Get_Trans_ID_Int();
        Objeto_Transfer_Download["Fecha_Hora"] = DataTime;
        Objeto_Transfer_Download["Trans_Estado"] = Code;
        Objeto_Transfer_Download["Tipo_Maq"] = "AFT";
    }

    Info_Cashless.Reader_Lock(false);
    Info_Cashless.Reader_Lock(false);
    Info_Cashless.Reader_Lock(false);
    Info_Cashless.Reader_Lock(false);
    Sesion_Anterior = true;
    String Json;
    serializeJson(Objeto_Transfer_Download, Json); /* Serializa Data */

    // Info_Cashless.Log(RTC,"OBJETO_TRANSFER_DOWNLOAD_ACTUALIZADO",Json);

    hayTransaccionesPendientes_Download = true;
    // Serial.println(Json);
    if (Code == 0x00 || Code == 0x01)
    {
        Info_Cashless.Remove_Currrent_Player_Sesion();

        if (Variables_globales.Get_Variable_Global(Conexion_TFT_Display) && Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
        {

            if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
            {

                DisplayTFT.info.Saldo_Canjeable = Cashless.BCDtoUint32_Pos(Saldos, CASHABLES, 0);
                DisplayTFT.info.Saldo_No_Restrindigo = 0;
                DisplayTFT.info.Saldo_Restringido = 0;
            }
            else
            {

                DisplayTFT.info.Saldo_Canjeable = Cashless.BCDtoUint32_Pos(Saldos, CASHABLES, 0);
                DisplayTFT.info.Saldo_No_Restrindigo = Cashless.BCDtoUint32_Pos(Saldos, NON_RESTRICTED, 10);
                DisplayTFT.info.Saldo_Restringido = Cashless.BCDtoUint32_Pos(Saldos, RESTRICTED, 5);
            }
            DisplayTFT.info.Actualiza_Saldos_Finales=true;
        }

    }

    return true;
}

bool Cashless_API::Status_Transfer_Download_Pendiente(int Code, char Buffer_Transfer[], ESP32Time RTC,int Id_Client)
{

    String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

    char Current_IP[4];
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
    {
        if (Code == 0x00)
            Objeto_Transfer_Download["IsSuccess"] = true;
        else
            Objeto_Transfer_Download["IsSuccess"] = false;
    }
    else
    {
        if (Code == 0x00 || Code == 0x01)
            Objeto_Transfer_Download["IsSuccess"] = true;
        else
            Objeto_Transfer_Download["IsSuccess"] = false;
    }

    Objeto_Transfer_Download["Ip"] = IP_toString_(Current_IP);
    Objeto_Transfer_Download["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
    Objeto_Transfer_Download["MAC"] = WiFi.macAddress();

    Objeto_Transfer_Download["Cliente_ID"] = Id_Client;
    Objeto_Transfer_Download["Trans_Tipo"] = "D";

    if(ExitsCahlessID())
        Objeto_Transfer_Download["Cashless_ID"]=GetCashlessID(0);

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
    {
        Objeto_Transfer_Download["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 5);
        Objeto_Transfer_Download["Saldo_Restringido"] = 0;
        Objeto_Transfer_Download["Saldo_No_Restringido"] = 0;
        Objeto_Transfer_Download["Trans_ID"] = Cashless.Get_Trans_ID_EFT();
        Objeto_Transfer_Download["Fecha_Hora"] = DataTime;
        Objeto_Transfer_Download["Trans_Estado"] = Code;
        Objeto_Transfer_Download["Tipo_Maq"] = "EFT";
    }
    else
    {
        Objeto_Transfer_Download["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 7);
        Objeto_Transfer_Download["Saldo_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, RESTRICTED, 12);
        Objeto_Transfer_Download["Saldo_No_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, NON_RESTRICTED, 17);
        Objeto_Transfer_Download["Trans_ID"] = Cashless.Get_Trans_ID_Int();
        Objeto_Transfer_Download["Fecha_Hora"] = DataTime;
        Objeto_Transfer_Download["Trans_Estado"] = Code;
        Objeto_Transfer_Download["Tipo_Maq"] = "AFT";
    }

    Info_Cashless.Reader_Lock(false);
    Info_Cashless.Reader_Lock(false);
    Info_Cashless.Reader_Lock(false);
    Info_Cashless.Reader_Lock(false);
    Sesion_Anterior = true;
    String Json;
    serializeJson(Objeto_Transfer_Download, Json); /* Serializa Data */

    // Info_Cashless.Log(RTC,"OBJETO_TRANSFER_DOWNLOAD_ACTUALIZADO",Json);

    hayTransaccionesPendientes_Download = true;
    // Serial.println(Json);
    if (Code == 0x00 || Code == 0x01)
        Info_Cashless.Remove_Currrent_Player_Sesion();
    return true;
}

/* Valida ID para descarga cashless  por operador */
bool Cashless_API::Valida_Operador_Cashless(char ID_Tarjeta_Operador[])
{
    int Code = false;

    if (WiFi.status() == WL_CONNECTED && Variables_globales.Get_Variable_Global(Token_Cashless_Solicitud))
    {
        StaticJsonDocument<800> jsonDocument;
        jsonDocument.clear();

        char Current_IP[4];
        memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

        // jsonDocument["IsSuccess"] = true;
        jsonDocument["IsSuccess"] = true;
        jsonDocument["Ip"] = IP_toString_(Current_IP);
        jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();
        jsonDocument["Tarjeta_Id"] = contadores.Get_Operador_ID_Int(ID_Tarjeta_Operador);
        jsonDocument["Tarjeta_Tipo"] = "O";

        int httpCode;

        char IP_Server[4];
        memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));

        std::string Ip = IP_toString_(IP_Server);
        String Ip_Server = String(Ip.c_str());
        String Puerto = "9595";
        String fwurl = "http://" + Ip_Server + ":" + Puerto + "/api/Cashless/Tarjeta";

        WiFiClient client;

        HTTPClient https;
        https.setTimeout(15000); /* 10 seg */
        String Json;
        serializeJson(jsonDocument, Json); /* Serializa Data */

        if (https.begin(client, fwurl))
        {
            https.addHeader("Content-Type", "application/json");
            https.addHeader("hash", Info_Cashless.Get_Hash_Valido());
            https.addHeader("gmsec", "GMaster");
            https.addHeader("Authorization", "Bearer " + Info_Cashless.Get_Token_Valido()); // Agrega el token de autorización

            httpCode = https.POST(Json);

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
                }
                else
                {
                    bool IsSuccess = doc["IsSuccess"];
                    if (IsSuccess)
                    {
                        // Serial.println("Recibida por el Servidor");
                        Code = true;
                    }

                    else
                    {
                        Info_Cashless.Log(RTC, "ERROR_SOLICITUD_VALIDACION_OPERADOR", "Return false");
                        Code = false;
                    }
                }

                doc.clear();
            }
            else
            {
                Info_Cashless.Log(RTC, "ERROR_SOLICITUD_VALIDACION_OPERADOR", String(httpCode));
                Code = false;
            }
            https.end();
            return Code;
        }
        else
            return Code;
    }
    else
        return Code;
}

/* Retorna  Estado de transferencia de carga  */
String Cashless_API::Get_Current_Status_Transfer()
{
    String Json;
    serializeJson(Objeto_Transfer, Json); /* Serializa Data */
    Objeto_Transfer.clear(); /*  Limpia  Objeto Ack Transferencias carga */
    //Buffer_Cashless.Init_Buffer_Transfer_AFT(true);
    return Json;
}

/* Retorna Estado de  transferencia de descarga */
String Cashless_API::Get_Current_Status_Transfer_Download()
{
    String Json;
    serializeJson(Objeto_Transfer_Download, Json); /* Serializa Data */
    Objeto_Transfer_Download.clear(); /*  Limpia  Objeto Ack Transferencias carga */
    Buffer_Cashless.Clear_Buffer(Buffer_RX_Cashless); /* Inicializa Buffer Creditos*/
    
    //Buffer_Cashless.Init_Buffer_Transfer_AFT(true);
    return Json;
}

/* Retorna Ack de transferencia pendiente carga */
String Cashless_API::Get_Current_Pending_Ack_Load(void)
{
    String Json;
    serializeJson(Objeto_Transfer, Json); /* Serializa Data */
    return Json;
}

/* Retorna Ack de Transferencia pendiente de descarga */
String Cashless_API::Get_Current_Pending_Ack_Download(void)
{
    String Json;
    serializeJson(Objeto_Transfer_Download, Json); /* Serializa Data */
    return Json;
}

/* Envia Ack Transfer */

bool Cashless_API::enviarTransaccion(const String &json)
{
    int httpCode;
    int Code=false;

    char IP_Server[4];
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));

    std::string Ip=IP_toString_(IP_Server);
    String Ip_Server=String(Ip.c_str());
    // String Puerto="22141";
    // String fwurl = "http://"+Ip_Server+":"+Puerto+"/api/Cashless/Ack";

    String Puerto="9595";
    String fwurl = "http://"+Ip_Server+":"+Puerto+"/api/Cashless/Ack";

    WiFiClient client;

    HTTPClient https;

    StaticJsonDocument<200> docp;
    StaticJsonDocument<200> filtercp;
    docp.clear();
    filtercp.clear();
    filtercp["Trans_Estado"] = true; // Especificar la clave que deseas deserializar
    // Deserializar el JSON con el filtro
    DeserializationError error = deserializeJson(docp, json, DeserializationOption::Filter(filtercp));


    int code = docp["Trans_Estado"];

    switch (code)
    {
    case 0x40:
        https.setTimeout(5000); /* 5seg Solicitudes pendientes*/
        break;
    default:
        https.setTimeout(10000); /* 10 Segundos solicitudes transacciones final */
        break;
    }

    if (https.begin(client, fwurl))
    {
        https.addHeader("Content-Type", "application/json");
        https.addHeader("hash",Info_Cashless.Get_Hash_Valido());
        https.addHeader("gmsec","GMaster");
        https.addHeader("Authorization", "Bearer " + Info_Cashless.Get_Token_Valido()); // Agrega el token de autorización
        httpCode = https.POST(json);

        if (httpCode == HTTP_CODE_OK)
        {
            String Response = https.getString();
            StaticJsonDocument<1024>
                doc,
                filter;
            DeserializationError error = deserializeJson(doc, Response);
            if (error)
            {
                #ifdef Debug_HTTPS
                                Serial.println("Error Json Contadores ");
                #endif
                CodeHttp=2;
            }
            else
            {
                bool IsSuccess = doc["IsSuccess"];
                String Msgg=doc["Message"];

                if(IsSuccess)
                {
                    Code=true;
                }
                    
                else
                {
                    Code=false;
                    CodeHttp=1;
                }
                   
            }

            doc.clear();
        }
        else
        {
            CodeHttp=httpCode;
            Code=false;
        }
        https.end();
        return Code;
    }
    CodeHttp=0;
    return Code;
}





void Cashless_API:: guardarTransacciones()
{
    File file = SPIFFS.open(transaccionesFile, "w");
    if (file) {
        for (const auto& transaccion : transaccionesPendientes) {
            file.println(transaccion);
        }
        file.close();
    }
}

// Intentar enviar todas las transacciones pendientes

void Cashless_API:: intentarEnviarTransacciones() {
    bool cambios = false; // Indica si hubo cambios en la lista de transacciones pendientes.

    if(!transaccionesPendientes.empty())
    {
        
        String  transaccion = transaccionesPendientes.front(); /* Toma la primera transferencia */

        if(enviarTransaccion(transaccion))/* Intenta enviarla */
        {
            transaccionesPendientes.erase(transaccionesPendientes.begin()); // Eliminar la primera transacción
            cambios=true;

            StaticJsonDocument<200> filter;
            StaticJsonDocument<200> doc;
            doc.clear();
            filter.clear();
            // Crear un filtro para incluir solo la clave "IsSuccess"
            filter["IsSuccess"] = true;

            DeserializationError error = deserializeJson(doc, transaccion, DeserializationOption::Filter(filter));
            if (!error)
            {
                bool isSuccess = doc["IsSuccess"];

                if (isSuccess)
                {
                    Updated_Cashless_Counters(transaccion); /* Transaccion OK  envia trama contadores cashless */
                }
            }
        }else{
            switch (CodeHttp)
            {
            case 1:
                Info_Cashless.Log(RTC,"ERROR_RECEPCION_ACK","Return_false");
                Info_Cashless.Log(RTC,"ERROR_ACK_NO_RECIBIDO",transaccion);
                break;

            case 0:
                Info_Cashless.Log(RTC,"ERROR_RECEPCION_ACK",String(NOT_CONEXION_WITH_SERVER,HEX));
                break;

            case 2:

                Info_Cashless.Log(RTC,"ERROR_JSON_DATA","No_fue_posible_convertir_json_enviarTransaccion()");
                break;
            
            default:
                Info_Cashless.Log(RTC,"ERROR_RECEPCION_ACK",String(CodeHttp));
                break;
            }
        }

        if (cambios)
        {
            guardarTransacciones();

            if(AFT.GET_STATUS_TRANSFER()==TransaccionCashless::TRANS_FINALIZADA)
                AFT.STATUS_TRANSFER(TransaccionCashless::TRANS_IDLE);
        }
    }
    // auto it = transaccionesPendientes.begin();
    // while (it != transaccionesPendientes.end()) {
    //     if (enviarTransaccion(*it)) {
    //         it = transaccionesPendientes.erase(it); // Eliminar transacción enviada
    //         cambios = true; // Marcar que hubo cambios
    //     } else {
    //         ++it; // Continuar con la siguiente transacción
    //     }
    // }
    // Guardar las transacciones pendientes en el archivo solo si hubo cambios
    
}

// Función para manejar una nueva transferencia
void Cashless_API::nuevaTransferencia(const String& json) {
    if (!enviarTransaccion(json)) {
        Reset_Timer_Tranfer_Pending=false;
        transaccionesPendientes.push_back(json); // Agregar a la lista si no se puede enviar
        guardarTransacciones(); // Guardar en el archivo
        Transaccion_Finalizada();

    }else{

        StaticJsonDocument<200> filter;
        StaticJsonDocument<200> doc;
        doc.clear();
        filter.clear();
        // Crear un filtro para incluir solo la clave "IsSuccess"
        filter["IsSuccess"] = true;

        DeserializationError error = deserializeJson(doc, json, DeserializationOption::Filter(filter));
        if (!error)
        {   bool isSuccess = doc["IsSuccess"];
            
            if(isSuccess)
            {

                
                Updated_Cashless_Counters(json); /* Transaccion OK  envia trama contadores */
               
                
            }     
        }
        Transaccion_Finalizada();
        if(AFT.GET_STATUS_TRANSFER()==TransaccionCashless::TRANS_FINALIZADA)
                    AFT.STATUS_TRANSFER(TransaccionCashless::TRANS_IDLE);
    }
}

/* Metodo para reporte de ACK'S  para */
void Cashless_API::Reporting_Pending_Transfers(unsigned long TimeOut)
{

    if (hayTransaccionesPendientes)
    {
        nuevaTransferencia(Get_Current_Status_Transfer());
        /// Info_Cashless.Log(RTC,"ENVIA_TRANSACCION_CARGA-API");
        hayTransaccionesPendientes = false;
    }
    if (Transfer_Pending_Load)
    {
        enviarTransaccion(Get_Current_Pending_Ack_Load());
        Transfer_Pending_Load = false;
    }

    if (Transfer_Pending_Download)
    {
        enviarTransaccion(Get_Current_Pending_Ack_Download());
        // Info_Cashless.Log(RTC,"ENVIA_TRANSACCION_DESCARGA-API");
        Transfer_Pending_Download = false;
    }

    if (hayTransaccionesPendientes_Download)
    {
        nuevaTransferencia(Get_Current_Status_Transfer_Download());
        hayTransaccionesPendientes_Download = false;
        Info_Cashless.Set_Status_Transfer(TRANSFER_DONE);
        delay(50);
        Info_Cashless.Set_Status_Transfer(TRANSFER_IDLE);
    }

    /* Envia Transacciones Cashless pendientes de Recepcion */

    if (!transaccionesPendientes.empty())
    {
        Tiempo_Inicio_Intento = millis();

        if ((Tiempo_Inicio_Intento - tiempoUltimoReintento) >= intervaloReintento)
        {

            if (Reset_Timer_Tranfer_Pending)
            {
                intentarEnviarTransacciones();
            }
            Reset_Timer_Tranfer_Pending = true;
            tiempoUltimoReintento = Tiempo_Inicio_Intento;
        }
    }

    if (Hay_BA_Pendientes)
    {
        Hay_BA_Pendientes = false;

        Serial.println(AFT.GetStatusBA());
        //enviarTransaccion(AFT.GetStatusBA());
    }
}

/* Carga en RAM  las transacciones pendientes por enviar al Sistema Gmaster despues de un reinicio
o falla en la transmisión */
void Cashless_API::Load_Pending_Transactions()
{
    if (SPIFFS.exists(transaccionesFile))
    {
        File file = SPIFFS.open(transaccionesFile, "r");
        if (file)
        {
           
            while (file.available())
            {
                String line = file.readStringUntil('\n');
                //Serial.println(line);
                transaccionesPendientes.push_back(line);
            }
            file.close();
        }
    }else{

        File file = SPIFFS.open(transaccionesFile, "a");
        if (file)
        {
            file.close();
        }
    }
}


/* Inicializa Sistema de archivos en el procesador para guardar las transacciones pendientes.*/
bool Cashless_API::Inicialize_File_System(void)
{
    if(Variables_globales.Get_Variable_Global(Default_Formatt))
    {
        for(int i=0; i<5; i++)
        {
            if(SPIFFS.format())
                break;
        }

        NVS.begin("Config_ESP32", false);
        NVS.putBool("Spiffs",false);
        NVS.end();
    }
    if(SPIFFS.begin(Variables_globales.Get_Variable_Global(Default_Formatt)))
        return true;
    else
        return false;
}


/* Metodo para Inicializar Sesion Player Trancking */
bool Cashless_API::Init_Player_Tracking_Sesion(byte Id_Client[])
{

    if (contadores.Set_Client_ID(Id_Client))
    {
        New_Timer_Final = New_Timmer_Inicial; /*RESET TIMEOUT*/
        /* Guarda  Informacion cliente en memoria SD */
        Update_Status_SD(); /* Actualiza Estado de Almacenamiento */
        Storage_Cliente(Archivo_CSV_Sesiones, Variables_globales.Get_Variable_Global(Enable_Storage), contadores.Get_Client_ID());
        /*  Maquina en juego  y Sesion Activa */
        Variables_globales.Set_Variable_Global(Flag_Sesion_RFID, true);
        Variables_globales.Set_Variable_Global(Flag_Contadores_Sesion_ON, true);
        // Variables_globales.Set_Variable_Global(Flag_Maquina_En_Juego, true);
        /* -Notificacion de inicio de sesion de juego-*/

        if(Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless))
            Status_Barra(CARGA_CASHLESS_EXITOSA);
        else
            Status_Barra(SESION_INICIADA);


        // int Sesion=0;
        // Serial.println(Info_Cashless.Type_Sesion());
        // switch (Info_Cashless.Type_Sesion())
        // {
        // case 22:
        //     Sesion=PLAYER_CASHLESS_SESION;
        //     break;

        // default:
        //     Sesion=PLAYER_TRACKING_SESION;
        //     break;
        // }



        // Serial.println(Sesion);
        

        if(contadores.Verify_Client_ID(contadores.Get_Client_ID()) && Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
            return true;
        else
            return false;
    }
    else
    {
        contadores.Close_ID_Client();
        Variables_globales.Set_Variable_Global(Flag_Sesion_RFID, false);
        Status_Barra(ERROR_LECTURA);
        return false;
    }
}

/*Metodo para cerrar sesion Player Tracking */
bool Cashless_API::Close_Player_Tracking_Sesion(bool Enable)
{

    /* Borra sesion de fidelización de la memoria */
    // switch (contadores.Get_Type_Sesion())
    // {
    // case PLAYER_TRACKING_SESION:
    //     //Serial.println("Id Borrado OK");
    //     Remove_Currrent_Player_Sesion();
    //     break;

    // default:
    //     break;
    // }

    contadores.Close_ID_Client();
    contadores.Close_ID_Client_Temp();
    Sesion_Cerrada_Color=true;
    Variables_globales.Set_Variable_Global(Flag_Sesion_RFID,false); 
    Variables_globales.Set_Variable_Global(Flag_Contadores_Sesion_OFF, true);
    Status_Barra(SESION_CERRADA);
    delay(10);
    Reset_Handle_LED();
    delay(50);

    //Close_Player_TFT(0,0,1,1);

    if(!Enable)
    {
        Status_Barra(ERROR_LECTURA);
        return false;
    }
    else
        return true;
}

bool Cashless_API::Reader_Lock(bool Status_Lock)
{
    Variables_globales.Set_Variable_Global(Handle_RFID_Lector,Status_Lock);

    if(Variables_globales.Get_Variable_Global(Handle_RFID_Lector)==Status_Lock)
        return true;
    else
        return false;
}


void Unlock_Reader_Timer(void* arg)
{
    Info_Cashless.Unlock_Reader();
    if(!Info_Cashless.Get_Status_Reader())
        // Serial.println("Lector Desbloqueado OK");
    Handle_LED = false;
}

/* Metodo para actualizar contadores despues de una transaccion Exitosa */
bool Cashless_API::Updated_Cashless_Counters(String Type_Transaccion)
{
    bool Code=false;
    int httpCode;

    char IP_Server[4];
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));

    std::string Ip=IP_toString_(IP_Server);
    String Ip_Server=String(Ip.c_str());
    String Puerto="9595";
    String fwurl = "http://"+Ip_Server+":"+Puerto+"/api/Cashless/Contadores";


    WiFiClient client;
    HTTPClient https;

    /* Crea Objeto*/
    
    StaticJsonDocument<200> doc;
    StaticJsonDocument<200> filter;
    doc.clear();
    filter.clear();
    filter["Trans_Tipo"] = true; // Especificar la clave que deseas deserializar
    // Deserializar el JSON con el filtro
    DeserializationError error = deserializeJson(doc, Type_Transaccion, DeserializationOption::Filter(filter));


    String Trans_Tipo = doc["Trans_Tipo"];
    //Serial.println(Trans_Tipo);

    esp_task_wdt_init(1000000, true);
    esp_task_wdt_add(NULL);

    if(Trans_Tipo==LOAD_TRANSACTION)
    {
        Actualiza_Cashless_Entradas();

        unsigned long Respuesta_Server = millis();
        int TIMEOUT_CONECT_SERVER = 5500; // Espera 3.5 seg para Encuestar contadores
        while (millis() - Respuesta_Server < TIMEOUT_CONECT_SERVER)
        {

            esp_task_wdt_reset();
#ifdef DEBUG_RFID
            Serial.println("Encuestando contadores.....");
#endif
            vTaskDelay(300);
            // if(Flag_Entradas_Cashless_OK)
            //     break;
        }

        Flag_Entradas_Cashless_OK=false;

        // Serial.println(" Actualiza Cashless Entradas");
    }else if(Trans_Tipo==DOWNLOAD_TRANSACTION)
    {
        Actualiza_Cashless_Salidas();

        unsigned long Respuesta_Server = millis();
        int TIMEOUT_CONECT_SERVER = 5500; // 3500 Espera 3.5 seg para Encuestar contadores
        while (millis() - Respuesta_Server < TIMEOUT_CONECT_SERVER)
        {
#ifdef DEBUG_RFID
            Serial.println("Encuestando contadores.....");
#endif
            esp_task_wdt_reset();
            vTaskDelay(300);
            // if(Flag_Salidas_Cashless_OK)
            //     break;
        }
        Flag_Salidas_Cashless_OK=false;
    }

   
    StaticJsonDocument<500> jsonDocument;
    jsonDocument.clear();
    char Current_IP[4];
    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    if (!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
        jsonDocument["IsSuccess"] = false;
    else
        jsonDocument["IsSuccess"] = true;

    jsonDocument["Entrada_Canjeable"] = contadores.Get_Contadores_String(Casheable_In);
    jsonDocument["Entrada_Restringida"] = contadores.Get_Contadores_String(Casheable_Restricted_In);
    jsonDocument["Entrada_No_Restringida"] = contadores.Get_Contadores_String(Casheable_NONrestricted_In);
    

    jsonDocument["Salida_Canjeable"] = contadores.Get_Contadores_String(Casheable_Out);
    jsonDocument["Salida_Restringida"] = contadores.Get_Contadores_String(Casheable_Restricted_Out);
    jsonDocument["Salida_No_Restringida"] = contadores.Get_Contadores_String(Casheable_NONrestricted_Out);

    jsonDocument["Ip"] = IP_toString_(Current_IP);
    jsonDocument["MAC"] = WiFi.macAddress();
    jsonDocument["Id_Maquina"] = 0;
    jsonDocument["Fecha_Hora"] = DataTime;
    jsonDocument["Key"] = Buffer_Cashless.Get_Key_Register_AFT_String();

    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 0:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 1:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;
    case 2:
        jsonDocument["Tipo_Maq"] = "EFT";
        break;
    case 3:
        jsonDocument["Tipo_Maq"] = "AFT";
        break;

    case 17:
        jsonDocument["Tipo_Maq"] = "EFT";
    break;
    
    default:
        jsonDocument["Tipo_Maq"] = "";
        break;
    }
    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
   // Serial.println(Json);
    
    https.setTimeout(10000); /* 10seg */
    if (https.begin(client, fwurl))
    {
       https.addHeader("Content-Type", "application/json");
        https.addHeader("hash",Info_Cashless.Get_Hash_Valido());
        https.addHeader("gmsec","GMaster");
        https.addHeader("Authorization", "Bearer " + Info_Cashless.Get_Token_Valido()); // Agrega el token de autorización
        httpCode = https.POST(Json);

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
                Serial.println("Error Json Contadores ");
#endif
            }
            else
            {
                bool IsSuccess = doc["IsSuccess"];

                if(IsSuccess)
                {
                    Code=true;
                    Info_Cashless.Log(RTC,"ACTUALIZA_CONTADORES_CASHLESS","OK",archivo);
                }
                else
                {
                    Info_Cashless.Log(RTC,"ACTUALIZA_CONTADORES_CASHLESS","ERROR_ACTUALIZACION",archivo,ERROR_);
                    Code = false;
                }
                
            }

            doc.clear();
        }
        else
        {
            Info_Cashless.Log(RTC,"ACTUALIZA_CONTADORES_CASHLESS","ERROR:"+String(httpCode),archivo,ERROR_);
            Code=false;
        }
        https.end();
           
        return Code;
    }
    return false;
}

void Cashless_API::Log(ESP32Time RTC,String Msg,String Data, const char *Txt,int Nivel_log)
{
    // String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

    char buffer[25];
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
            RTC.getYear(),
            RTC.getMonth() + 1,
            RTC.getDay(),
            RTC.getHour(true),
            RTC.getMinute(),
            RTC.getSecond());

    String DataTime(buffer);

    String Nivel;
    switch (Nivel_log) {
    case INFO_:  Nivel = "INFO "; break;
    case DEBUG_: Nivel = "DEBUG"; break;
    case ERROR_: Nivel = "ERROR"; break;
    case WARN_:  Nivel = "WARN "; break;
    case FATAL_: Nivel = "FATAL"; break;
    default:     Nivel = "INFO "; break;
    }
    
    String Ms=DataTime+" "+"["+Nivel+"]"+" "+" ["+Msg+"]"+" "+Data;
    Data.replace("\n","");
    Msg.replace("\n","");
    Ms.replace("\n","");
    //Ms.replace(" ", "");
    Ms.replace("\n", "");
    Ms.replace("\t", "");
    Erro_Log(Ms,Txt);
}

/* Metodo para crear, eliminar y establecer tiempo de logs */
bool Cashless_API::CreaLog(const char *Archivo, bool Minutes_Days, uint32_t Min_Day)
{
    if (Variables_globales.Get_Variable_Global(SD_INSERT) && Variables_globales.Get_Variable_Global(Sincronizacion_RTC))
    {
        if (!SD.exists(Archivo))
        {
            File f = SD.open(Archivo, FILE_WRITE);
            if (f)
            {
                /* Fecha creacion de archivo log */
                NVS.begin("Config_ESP32", false);
                NVS.putULong("Fecha_log", RTC.getEpoch());
                NVS.end();
                f.close();
                
                return true;
            }
        }
        else
        {
            Verifica_Log(Archivo, Minutes_Days, Min_Day);
            return true;
        }
    }
    return false;
}

bool Cashless_API::Verifica_Log(const char *Archivo, bool Minutes_Days, uint32_t Min_Day)
{

    NVS.begin("Config_ESP32", false);
    time_t fecha_creacion = NVS.getULong("Fecha_log", 0);
    NVS.end();

    if (fecha_creacion == 0)
    {
        NVS.begin("Config_ESP32", false);
        NVS.putULong("Fecha_log", RTC.getEpoch());
        NVS.end();

        Serial.println("Guarda fecha actual como Inicio de archivo");
        return false;
    }

    time_t ahora = RTC.getEpoch();

    /* Minutes_Days=true valida en minutos */
    /* Minutes_Days=false valida en dias */
    double Contador;

    if (Minutes_Days)
        Contador = (double)(ahora - fecha_creacion) / 60.0;
    else
        Contador = (double)(ahora - fecha_creacion) / (60 * 60 * 24);

    if (Contador > Min_Day)
    {
        if (SD.remove(Archivo))
        {

            NVS.begin("Config_ESP32", false);
            NVS.remove("Fecha_log");
            NVS.end();
            delay(10);

            /* Crea archivo con fecha nuevamente */
            File f = SD.open(Archivo, FILE_WRITE);
            if (f)
            {
                /* Fecha creacion de archivo log */
                NVS.begin("Config_ESP32", false);
                NVS.putULong("Fecha_log", RTC.getEpoch());
                NVS.end();
                f.close();
                return true;
            }
        }
    }
    else
    {
        time_t fecha_eliminacion = fecha_creacion + (Min_Day * 60 * 60 * 24);
        struct tm *tm_info;
        char buffer[80];

        tm_info = localtime(&fecha_eliminacion);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
        Serial.print("📅 Fecha de eliminación: ");
        Serial.println(buffer);
        String fechaString = "Fecha de eliminacion log: " + String(buffer);
        // Serial.println("OK");
        Report_Http_Code(FECHA_ELIMINA_LOG, fechaString, true);
    }

    return false;
}

bool Cashless_API::Guarda_Tiempo_Log(bool Minutes_Days, uint32_t Min_Day)
{

    NVS.begin("Config_ESP32", false);
    NVS.putULong("TimeBackup", Min_Day);
    // NVS.putBool("Min_Day", Minutes_Days);

    /* Por defecto  el log se guarda 15 dias */
    // bool Min = NVS.getBool("Min_Day", false);
    uint32_t Time = NVS.getULong("TimeBackup", 15);
    NVS.end();

    if (Time == Min_Day)
        return true;

    else
        return false;
}
/* Retorna el tipo de sesion a  cerrar
retorna (21) = PLAYER_TRACKING_SESION
retorna (22) = PLAYER_CASHLESS_SESION
*/
int Cashless_API::Type_Sesion(int Flag, bool Status)
{

    /* WRITE */
    if (Flag == PLAYER_CASHLESS_SESION)
    {
        Variables_globales.Set_Variable_Global(Flag_Sesion_Cashless, Status);
    }

    /* READ */

    // if(!Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless) && Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless))
    //     return PLAYER_TRACKING_SESION;
    // else if(Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless))
    //     return PLAYER_CASHLESS_SESION;
    // else{
    //     return PLAYER_TRACKING_SESION;
    // }

    if (Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless))
        return PLAYER_CASHLESS_SESION;
    else
        return PLAYER_TRACKING_SESION;
}

/* Retorna 1 Maquina no comunica 
Retorna 2 Ya se intento recuperar la sesion 
Retorna 3 Error de recuperacion */
int Cashless_API::Recovery_Player_Sesion(byte Id_Client_Recovery[], int Type_Sesion, bool Handle_Cashless)
{
    
    if(Variables_globales.Get_Variable_Global(Comunicacion_Maq)) /* Comunicacion OK*/
    {

        // if(contadores.Verify_Tarjeta(Id_Client_Recovery)&&!Handle_Cashless)
        // {
        //     switch (Type_Sesion)
        //     {
        //     case PLAYER_TRACKING_SESION:

        //         contadores.Set_Client_ID(Id_Client_Recovery);
        //         Info_Cashless.Init_Player_Tracking_Sesion(Id_Client_Recovery);
        //         Info_Cashless.Unlock_Reader();
        //         break;
            
        //     default:
        //         break;
        //     }

        //     return 2;
        // }

        if(contadores.Verify_Tarjeta(Id_Client_Recovery) && Handle_Cashless) /* Existe ID */
        {

            //Serial.println("OK");
            bool Test=false;
            switch (Type_Sesion)
            {
            case PLAYER_CASHLESS_SESION:
                
               // Info_Cashless.Lock_Reader();
                // Status_Barra(301);
                 //Serial.println("Se identifico Sesion Cashless");
                // Serial.println("Recuperando sesion......");

                if (contadores.Set_Client_ID(Id_Client_Recovery) && contadores.Set_Client_ID_Transaccion(Id_Client_Recovery))
                {
                    
                    Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, true);
                    Status_Barra(CARGA_CASHLESS_EXITOSA);
                    Variables_globales.Set_Variable_Global(Flag_Sesion_RFID, true);
                    //Serial.println("Cashless Iniciado....");
                    

                    /*Verifica si existe carga cashless pendiente por consultar */
                    NVS.begin("Config_ESP32", false);
                    
                    bool Test_Pen_Data = NVS.getBool("Cash_Pending", false);
                    bool Donmload_Critical = NVS.getBool("Cash_Pen_Dow", false);


                    NVS.end();
                    if (Test_Pen_Data)
                    {
                        Test=true;
                        Status_Barra(301); /* Lector ocupado por transaccion */
                      //  Serial.println("Tarea de trasacción pendiente habilitada");

                        if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)==2||Configuracion.Get_Configuracion(Tipo_Maquina, 0)==17)
                            Variables_globales.Set_Variable_Global(Event_Load_Cashless_Pending, true);
                        //Info_Cashless.Lock_Reader();
                    }

                    if (Donmload_Critical)
                    {
                        Test = true;
                        Status_Barra(301); /* Lector ocupado por transaccion */
                                           //  Serial.println("Tarea de trasacción pendiente habilitada");
                        // Variables_globales.Set_Variable_Global(Event_Dowmload_Cashless_Pending, true);
                        // Info_Cashless.Lock_Reader();
                        /* AFT */
                        if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 3||Configuracion.Get_Configuracion(Tipo_Maquina, 0)==1)
                        {
                            AFT.STATUS_TRANSFER(TransaccionCashless::TRANS_PENDIENTE);
                            AFT.Set_Evento_Controller_Machine_Download(true);
                        }
                            
                        else /*EFT */
                            Variables_globales.Set_Variable_Global(Event_Dowmload_Cashless_Pending, true);
                    }

                    Info_Cashless.Log(RTC,"SESION_CASHLESS","RECUPERA_SESION_POR_REINICIO",archivo,WARN_);
                }
                // if(!Test)
                //     Info_Cashless.Unlock_Reader();
                break;

            case PLAYER_TRACKING_SESION:
                Info_Cashless.Unlock_Reader();
                // Serial.println("Se identifico un ID despues del Reinicio");
                //Serial.println("Se identifico Sesion Player Tracking...");
                // Serial.println("Borra Datos para no recuperar Player Tracking....");
                Info_Cashless.Remove_Currrent_Player_Sesion();
                // Info_Cashless.Reader_Lock(true);

                // if (contadores.Set_Client_ID(Id_Client_Recovery))
                // {
                //     if(Handle_Cashless)
                //     {
                //         Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, true);
                //         contadores.Set_Client_ID_Transaccion(Id_Client_Recovery);
                //     }
                //     else
                //         Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
                //     Status_Barra(SESION_INICIADA);
                //     Variables_globales.Set_Variable_Global(Flag_Sesion_RFID, true);
                // }
                // Info_Cashless.Reader_Lock(false);
                // break;
               
                break;
            default:
                Info_Cashless.Remove_Currrent_Player_Sesion();
                break;
            }

            return 2;
        }else
        {
            DisplayTFT.Home_TFT_Globus_IM();
            AFT.Get_Client_Transfer_Critical();
            //Serial.println("NO existe Sesion ");
            return 2;
        }
            
    }else{
        return 1;
    }  
}

void Cashless_API::Saves_Current_Player_Sesion(byte Id_Client_Recovery[],int Type_Sesion)
{

    byte Array[8];

    Array[0]=Id_Client_Recovery[0];
    Array[1]=Id_Client_Recovery[1];
    Array[2]=Id_Client_Recovery[2];
    Array[3]=Id_Client_Recovery[3];
    Array[4]=Id_Client_Recovery[4];
    Array[5]=Id_Client_Recovery[5];
    Array[6]=Id_Client_Recovery[6];
    Array[7]=Id_Client_Recovery[7];

    NVS.begin("Config_ESP32", false);
    NVS.putBytes("Id_Client",Id_Client_Recovery,sizeof(Array));
    NVS.putInt("Sesion_Type",Type_Sesion);
    NVS.end();
}

void Cashless_API::Remove_Currrent_Player_Sesion(void)
{
    NVS.begin("Config_ESP32", false);
    byte Null[8]={'0', '0', '0', '0','0','0','0','0'};
    int Default_Type_Sesion= SESION_DEFAULT;
    NVS.putBytes("Id_Client",Null,sizeof(Null));
    NVS.putInt("Sesion_Type",Default_Type_Sesion);
    NVS.end();
}

bool Cashless_API::Set_Transfer_Pending(bool Status,String Type_Transaccion,char Buffer_Transfer_Amount[])
{
    if(Type_Transaccion=="C")
    {
        
        Objeto_Transfer["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer_Amount, CASHABLES, 7);
        Objeto_Transfer["Saldo_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer_Amount, RESTRICTED, 12);
        Objeto_Transfer["Saldo_No_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer_Amount, NON_RESTRICTED, 17);
        Enable_Event_Pendeing_Transfer_Load=Status;
    }
    if(Type_Transaccion=="D")

        Objeto_Transfer_Download["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer_Amount, CASHABLES, 12);
        Objeto_Transfer_Download["Saldo_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer_Amount, RESTRICTED, 17);
        Objeto_Transfer_Download["Saldo_No_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer_Amount, NON_RESTRICTED, 22);

        Enable_Event_Pending_Transfer_Download = Status;

    if(Type_Transaccion=="CC")
    {
        Enable_Event_Pendeing_Transfer_Load=Status;
    }
    if(Type_Transaccion=="DD")
    {   
        Enable_Event_Pending_Transfer_Download=Status;
    }
    

    return true;
}

bool Cashless_API::Get_Status_Event_Pending_Transfer(String Type_Transaccion)
{
    if(Type_Transaccion=="C")
       return Enable_Event_Pendeing_Transfer_Load;
    if(Type_Transaccion=="D")
       return  Enable_Event_Pending_Transfer_Download;

    return false;
}

/* Solicitud de descarga por evento 6A 
Procesa evento 6A dependiento del  estado (Enable)

Enable -> True  Procesa evento 6A de requerimiento AFT/EFT y realiza descarga Cashless 
Enable -> False No procesa evento 6A y espera cobro por  asistente de pagos.
*/
bool Cashless_API::Requerimiento_AFT_6A(int Evento, bool Enable)
{
    if (Enable && Variables_globales.Get_Variable_Global(Enable_Cashless) && Evento == 0x6A && Info_Cashless.Type_Sesion() == PLAYER_CASHLESS_SESION && Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
    {
        Info_Cashless.Lock_Reader();
        int ClientID=contadores.Get_Client_ID_Transaccion_Int();
        switch (Info_Cashless.Info_Client_Download(contadores.Get_Client_ID_Transaccion(), RTC, "D", Cashless.Get_Trans_ID_Int(),ClientID))
        {
        case REQUEST_SUCCESSFULLY_RECEIVED:
            Solicitud_Descarga_Cashless();
            return true;
            break;

        case HTTP_CODE_UNAUTHORIZED:
            Status_Barra(ERROR_LECTURA);
            Info_Cashless.Unlock_Reader(); /* Habilita lector */
            Report_Http_Code(HTTP_CODE_UNAUTHORIZED, "Token de autenticacion no valido Codigo HTTP");
            return false;
            break;

        default:
            Status_Barra(ERROR_LECTURA);
            Info_Cashless.Unlock_Reader();
            return false;
            break;
        }

    }
    return false;
    
}


/* Problema identificado el Ack*/
bool Cashless_API::Update_Ack_Evento_69(String Type_Transaccion,int Code)
{
    String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

    if (Type_Transaccion == "C")
    {
        /* Actualiza Objeto Carga */
        if (Code == 0x00 || Code == 0x01)
            Objeto_Transfer["IsSuccess"] = true;
        else
            Objeto_Transfer["IsSuccess"] = false;

        Objeto_Transfer["Cliente_ID"] = contadores.Get_Client_ID_Transaccion_Int();
        Objeto_Transfer["Trans_Tipo"] = "C";
        // Objeto_Transfer["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 7);
        // Objeto_Transfer["Saldo_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, RESTRICTED, 12);
        // Objeto_Transfer["Saldo_No_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, NON_RESTRICTED, 17);
        Objeto_Transfer["Trans_ID"] = Cashless.Get_Trans_ID_Int();
        Objeto_Transfer["Fecha_Hora"] = DataTime;
        Objeto_Transfer["Trans_Estado"] = Code;

        String Json;
        serializeJson(Objeto_Transfer, Json); /* Serializa Data */
        hayTransaccionesPendientes = true;
        Serial.println(Json);
        if (Code == 0x00 || Code == 0x01)
            Saves_Current_Player_Sesion(contadores.Get_Client_ID_Transaccion(), Info_Cashless.Type_Sesion());
    }

    if(Type_Transaccion=="D")
    {

        if (Code == 0x00 || Code == 0x01)
            Objeto_Transfer_Download["IsSuccess"] = true;
        else
            Objeto_Transfer_Download["IsSuccess"] = false;

        Objeto_Transfer_Download["Cliente_ID"] = contadores.Get_Client_ID_Transaccion_Int();
        Objeto_Transfer_Download["Trans_Tipo"] = "D";
        // Objeto_Transfer_Download["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 12);
        // Objeto_Transfer_Download["Saldo_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, RESTRICTED, 17);
        // Objeto_Transfer_Download["Saldo_No_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, NON_RESTRICTED, 22);
        Objeto_Transfer_Download["Trans_ID"] = Cashless.Get_Trans_ID_Int();
        Objeto_Transfer_Download["Fecha_Hora"] = DataTime;
        Objeto_Transfer_Download["Trans_Estado"] = Code;

        Sesion_Anterior = true;
        String Json;
        serializeJson(Objeto_Transfer_Download, Json); /* Serializa Data */
        hayTransaccionesPendientes_Download = true;
        Serial.println(Json);
        if (Code == 0x00 || Code == 0x01)
            Info_Cashless.Remove_Currrent_Player_Sesion();
    }
        
    return true;
}

bool Cashless_API::Set_Controller_Transfer_Load(bool Enable)
{

    Enable_Transfer_Load=Enable;

    if(Enable_Transfer_Load==Enable)
        return true;
    else
        return false;
}


bool Cashless_API::Set_Controller_Transfer_Download(bool Enable)
{
    Enable_Transfer_Download=Enable;

    if(Enable_Transfer_Download==Enable)
        return true;
    else
        return false;
}


bool  Cashless_API::Get_Controller_Transfer_Load(void)
{
    return Enable_Transfer_Load;
}

bool Cashless_API::Get_Controller_Transfer_Download(void)
{
    return Enable_Transfer_Download;
}

/* Inicializa  Timer para recuperar estado de lector RFID Despues de una transaccion */

// void TimeOut_Token_Cash(void *arg)
// {
//     Info_Cashless.Solicitud_Token_Cashless(); /* Solicita  Token Para  Cashless*/
//     delay(1);
//     if (Variables_globales.Get_Variable_Global(Token_Cashless_Solicitud)) /* Valida Si se genero un Token Valido */
//         esp_timer_stop(Token_Cash); /* Si el token es OK  detiene el timer */
// }

bool Cashless_API::Init_Timer_Lector(void)
{

  esp_timer_create_args_t timer_args = {
      .callback = &Unlock_Reader_Timer,
      .arg = NULL,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "read_timer"
  };
  esp_timer_create(&timer_args, &readTimer);



   esp_timer_create_args_t timer_args_r = {
        .callback = &New_Token,
        .name = "Token Timer"
    };

    // Crear el temporizador una sola vez
    esp_timer_create(&timer_args_r, &token_timer);


    esp_timer_create_args_t timer_args_r_Cashless = {
        .callback = &Break_Cashless_Pending,
        .name = "Cashless_Pending"
    };

    // Crear el temporizador una sola vez
    esp_timer_create(&timer_args_r_Cashless, &Cashless_Pending);

    return true;



}

/* Configura Timer Para desbloqueo de lector RFID*/
bool Cashless_API::Transaccion_Finalizada(void)
{
    esp_timer_stop(readTimer);
    // Configura el temporizador para reactivar las lecturas después de 10 segundos
    //esp_timer_start_once(readTimer, 10000000); // 10 segundos en microsegundos
    esp_timer_start_once(readTimer, 7000000); // 5 segundos en microsegundos
    Status_Barra(301);
    return true;
}

/* Retorna estado de lector RFID 
true-> Lector Habilitado para lectura
false-> Deshabilitado (Ocupado)
*/
bool Cashless_API::Get_Status_Reader(void)
{
    return Handle_Lector_RFID;
}


/* Bloquea Modulo lector RFID con  efecto inmediato hasta que se llame al metodo Unlock_Reader o  Transaccion_Finalizada. 
Si se llama al metodo Transaccion_Finalizada el lector sera desbloqueado despues del tiempo  seteado en esp_timer_start_once(readTimer, 7000000); // 7 segundos en microsegundos
*/
bool Cashless_API::Lock_Reader(void)
{
    Handle_Lector_RFID=true;

    if(Handle_Lector_RFID==true)
        return true;
    else    
        return false;
}


/* Desbloquea Modulo lector RFID con efecto inmediato */
bool Cashless_API::Unlock_Reader(void)
{
    Handle_Lector_RFID=false;

    if(!Handle_Lector_RFID)
        return true;
    else    
        return false;
}


/* Genera Clave de acceso */
char* Cashless_API::Key_Generator(void)
{
    char Current_IP[4];
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));
    std::string Ip_string=IP_toString_(Current_IP);
    String Ip_local= Ip_string.c_str();
    String Mac_ESP=WiFi.macAddress();
    String Output=Ip_local+"^"+Mac_ESP;
   
    Buffer.Set_Key_API(Output);
    delay(1);
    return Buffer.Get_Buffer_Key();
}

void Cashless_API::Genera_Token_Cashless(void)
{
    if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
    {
        if (!Reset_Time)
            TimeOut_Token_Final = TimeOut_Token_Inicial;

        // if (Variables_globales.Get_Variable_Global(Sincronizacion_RTC))
        // {
        TimeOut_Token_Inicial = millis();
        if ((TimeOut_Token_Inicial - TimeOut_Token_Final) >= TimeOut_Ejecuta && !Variables_globales.Get_Variable_Global(Token_Cashless_Solicitud) || !Reset_Time)
        {
            Solicitud_Token_Cashless();
            TimeOut_Token_Final = TimeOut_Token_Inicial;
            Reset_Time = true;
        }
    }

    // }
}


void Cashless_API::Solicitud_Token_Cashless(void)
{
    
    if (WiFi.status() == WL_CONNECTED)
    {
        char Current_IP[4];
        memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));
        std::string Ip_string = IP_toString_(Current_IP);
        String Ip_local = Ip_string.c_str();
        String Mac_ESP = WiFi.macAddress();

        char IP_Server[4];
        memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));

        std::string Ip = IP_toString_(IP_Server);
        String Ip_Server = String(Ip.c_str());
        String Puerto = "9595";
        String fwurl = "http://" + Ip_Server + ":" + Puerto + "/api/Cashless/Token";

        String Token = "";
        WiFiClient client;
        int httpCode;
        HTTPClient https;

        https.setTimeout(10000); /* 10 seg */
        Key_Generator();

        if (https.begin(client, fwurl))
        {
            https.addHeader("Content-Type", "application/json");
            https.addHeader("ip", Ip_local);
            https.addHeader("mac", Mac_ESP);
            httpCode = https.POST((uint8_t*)Buffer.Get_Buffer_Key(),258);
            
            //Serial.println(httpCode);
            if (httpCode == HTTP_CODE_OK)
            {
                String Response = https.getString();
                StaticJsonDocument<1024>
                    doc,
                    filter;

                DeserializationError error = deserializeJson(doc, Response);
                if (error)
                {
#ifdef Debug_HTTPS
                    Serial.println("Error Json Contadores ");
#endif
                }
                else
                {
                   // Serial.println(Response);
                    bool IsSuccess = doc["IsSuccess"];
                
                    if (IsSuccess)
                    {

                        String Token = doc["Data"]["access_token"];
                        String Hash = doc["Data"]["hash"];
                        int hour = doc["Data"]["Time_Expires"];
                        int minutes = doc["Data"]["Minutes"];
                        int seconds = doc["Data"]["Seconds"];
                        int Token_Expires_Day = doc["Data"]["Day"];
                        int Token_Expires_Month = doc["Data"]["Month"];
                        int Token_Expires_Year = doc["Data"]["Year"];
                        uint64_t tiempoExpiracion_ms= doc["Data"]["Expiration_time"];
                        
                        if (Set_Token_Valido(Token,Hash))
                        {
                            Variables_globales.Set_Variable_Global(Token_Cashless_Solicitud, true);
                            Token_Expiration(tiempoExpiracion_ms);
                        }
                            
                        else
                            Variables_globales.Set_Variable_Global(Token_Cashless_Solicitud, false);
                    }
                    else
                    {
                        Set_Token_Valido("","");
                        Variables_globales.Set_Variable_Global(Token_Cashless_Solicitud, false);
                    }
                }

                doc.clear();
            }
            https.end();
        }
        else
        {
            Set_Token_Valido("","");
            Variables_globales.Set_Variable_Global(Token_Cashless_Solicitud, false);
        }
    }   
}



String Cashless_API::Get_Token_Valido(void)
{
    return Token_Cashless;
}

/* Retorna Hash */
String Cashless_API::Get_Hash_Valido(void)
{
    return Hash_Cashless;
}

/* Guarda token en memoria RAM*/
bool Cashless_API::Set_Token_Valido(String Token_Valido,String Hash_Valido)
{
    if (Token_Valido != "" && Hash_Valido!="")
    {
        Hash_Cashless=Hash_Valido;
        Token_Cashless = Token_Valido;
        return true;
    }
    else
        return false;
}

bool Cashless_API::Solicitud_Token_Cashless_BA(unsigned long timeout)
{
    bool Issucess=false;
    char Current_IP[4];
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));
    std::string Ip_string = IP_toString_(Current_IP);
    String Ip_local = Ip_string.c_str();
    String Mac_ESP = WiFi.macAddress();

    char IP_Server[4];
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));

    std::string Ip = IP_toString_(IP_Server);
    String Ip_Server = String(Ip.c_str());
    //String Puerto = "9595";
    String fwurl = "http://"+ipDest.toString()+":22141/api/MachineToken/token";
   
    Serial.println(fwurl);

    String Token = "";
    WiFiClient client;
    int httpCode;
    HTTPClient https;

    https.setTimeout(timeout); /* 10 seg */
    Key_Generator();

    if (https.begin(client, fwurl))
    {
        https.addHeader("Content-Type", "application/json");
        https.addHeader("ip", Ip_local);
        https.addHeader("mac", Mac_ESP);
        httpCode = https.POST((uint8_t *)Buffer.Get_Buffer_Key(), 258);

        Serial.println(httpCode);
        if (httpCode == HTTP_CODE_OK)
        {
            String Response = https.getString();
            StaticJsonDocument<1024>
                doc,
                filter;

            DeserializationError error = deserializeJson(doc, Response);
            if (error)
            {
#ifdef Debug_HTTPS
                Serial.println("Error Json Contadores ");
#endif
            }
            else
            {
                Serial.println(Response);
                bool IsSuccess = doc["IsSuccess"];

                if (IsSuccess)
                {

                    String Token = doc["Data"]["access_token"];
                    String Hash = doc["Data"]["hash"];
                    int hour = doc["Data"]["Time_Expires"];
                    int minutes = doc["Data"]["Minutes"];
                    int seconds = doc["Data"]["Seconds"];
                    int Token_Expires_Day = doc["Data"]["Day"];
                    int Token_Expires_Month = doc["Data"]["Month"];
                    int Token_Expires_Year = doc["Data"]["Year"];
                    uint64_t tiempoExpiracion_ms = doc["Data"]["Expiration_time"];

                    if (Set_Token_Valido_BA(Token, Hash))
                    {
                        // Variables_globales.Set_Variable_Global(Token_Cashless_Solicitud, true);
                        // Token_Expiration(tiempoExpiracion_ms);
                        Issucess=true;
                    }

                    else
                        Issucess=false;
                }
                else
                {
                    Set_Token_Valido_BA("", "");
                    Issucess=false;
                }
            }

            doc.clear();
        }
        https.end();
    }
    else
    {
        Set_Token_Valido_BA("", "");
        Issucess=false;
    }

    return Issucess;
}

String Cashless_API::Get_Token_Valido_BA(void)
{
    return Token_Cashless_BA;
}

/* Retorna Hash */
String Cashless_API::Get_Hash_Valido_BA(void)
{
    return Hash_Cashless_BA;
}

/* Guarda token en memoria RAM*/
bool Cashless_API::Set_Token_Valido_BA(String Token_Valido,String Hash_Valido)
{
    if (Token_Valido != "" && Hash_Valido!="")
    {
        Hash_Cashless_BA=Hash_Valido;
        Token_Cashless_BA = Token_Valido;
        return true;
    }
    else
        return false;
}


bool Cashless_API::Set_Flag_Token(bool Status_Flag)
{
    Reset_Time=Status_Flag;

    if(Reset_Time==Status_Flag)
        return true;
    else
        return false;
}

void New_Token(void*arg)
{
    Variables_globales.Set_Variable_Global(Token_Cashless_Solicitud,false);
    Info_Cashless.Set_Flag_Token(false);
}

void Cashless_API::Token_Expiration(uint64_t tiempoExpiracion_ms)
{
    // Detiene el temporizador 
    esp_timer_stop(token_timer);
    esp_timer_start_once(token_timer, tiempoExpiracion_ms* 1000);
}


void Break_Cashless_Pending(void*arg)
{
    Variables_globales.Set_Variable_Global(Event_Load_Cashless_Pending,false);
    Info_Cashless.Unlock_Reader();

    
}

void Cashless_API::Break_Timer_Transfer_Pending(void)
{
    esp_timer_stop(Cashless_Pending);
    Variables_globales.Set_Variable_Global(Event_Load_Cashless_Pending,false);
    Info_Cashless.Unlock_Reader();
    Serial.println("Se ejecuto");
}
void Cashless_API::Init_Timer_Transfer_Pending(uint64_t Tiempo_ms)
{
    esp_timer_stop(Cashless_Pending);
    esp_timer_start_once(Cashless_Pending, Tiempo_ms);
}



/* Verifica conexión con Gmaster antes de realizar transacciones fidelización o Cashless 

Entradas:
Id Cliente/Operador: 00000001
Typo de tarjeta: C || O
Tiempo de espera de confirmación: 10sg

Salida:
    True = Gmaster Conectado.
    False=Gmaster Desconectado.
*/
bool Cashless_API::Await_Conexion(int ClienteID,char Type_Client,int Timeout)
{
    bool Code=false;
    int httpCode;
    WiFiClient client;
    HTTPClient https;

    char IP_Server[4];
    char Current_IP[4];
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));

    std::string Ip=IP_toString_(IP_Server);
    std::string maquinaIp=IP_toString_(Current_IP);
    String Ip_Server=String(Ip.c_str());

    String MaquinaIp=String(maquinaIp.c_str());
    String Puerto="9595";
    String fwurl = "http://"+Ip_Server+":"+Puerto+"/api/Fidelizacion/VerificaConexion"+"?"+"tarjetaId=" + String(ClienteID)+"&"+"maquinaIp="
    +MaquinaIp+"&"+"tarjetaTipo="+String(Type_Client);
    
    https.setTimeout(Timeout); /* 10seg */
    if (https.begin(client, fwurl))
    {
        httpCode = https.GET();

        // Serial.println(httpCode);
        if (httpCode == HTTP_CODE_OK)
        {

            String Response = https.getString();
            StaticJsonDocument<800>
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

                if (IsSuccess)
                {
                    Code = true;
                    JsonVariant Data = doc["Data"];

                    if (Data.isNull())
                    {
                        Serial.println("ERROR: InformacionPuntosPantalla es NULL");
                        // return;
                    }
                    else
                    {
                        if (!Data["Cliente_Nombre"].is<String>() ||
                            !Data["Casino"].is<String>() ||
                            !Data["Deno_Cashless"].is<float>() ||
                            !Data["Total_Fide"].is<float>() ||
                            !Data["Total_Bole"].is<float>() ||
                            !Data["Nivel_Usuario"].is<String>() ||
                            !Data["Actual_Fide"].is<float>() ||
                            !Data["Actual_Bole"].is<float>())
                        {
                            Serial.println("Error En procesamiento de datos para actualizacion de pantalla");
                        }
                        else
                        {
                            DisplayTFT.info.Usuario = Data["Cliente_Nombre"].as<String>();
                            DisplayTFT.info.Casino = Data["Casino"].as<String>();

                            DisplayTFT.info.DenoCashless = Data["Deno_Cashless"].as<float>();
                            DisplayTFT.info.Total_Fide = Data["Total_Fide"].as<float>();
                            DisplayTFT.info.Total_Bole = Data["Total_Bole"].as<float>();
                            DisplayTFT.info.Nivel_Usuario = Data["Nivel_Usuario"].as<String>();
                            DisplayTFT.info.Actual_Fide = Data["Actual_Fide"].as<float>();
                            DisplayTFT.info.Actual_Bole = Data["Actual_Bole"].as<float>();
                        }
                    }
                }

                else
                    Code=false;
            }

            doc.clear();
        }
        else
        {

            Code=false;
        }
        https.end();

        return Code;
    }
    return false;
}

bool Cashless_API::Bloquea_Descarga_EFT(int Evento, int Tipo_Maq, int ClientID, bool Flag_Sesion)
{
    /* Si el tipo de maquina es EFT y existe una sesion cashless*/
    if (Tipo_Maq == 2 && ClientID > 0 && Flag_Sesion || Tipo_Maq == 17 && ClientID > 0 && Flag_Sesion) /* EFT O EFT+ */
    {
        switch (Evento)
        {

        case 0x51: /* Bloquea descarga Cashless */

            Variables_globales.Set_Variable_Global(Hand_pay_is_pending, true);

            return true;
            break;
        case 0x52:
            Variables_globales.Set_Variable_Global(Hand_pay_is_pending, false);
            return true;
            break;

        default:
            return false;
            break;
        }
    }
    return false;
}

// Constructor para inicializar el estado
Cashless_API::Cashless_API() {
    estado_transfer = TRANSFER_IDLE;
}

bool Cashless_API::Estado_Juego_Maquina(int Evento)
{
    switch (Evento)
    {
    case 0x7E:
        Variables_globales.Set_Variable_Global(Flag_Maquina_Juego_Evento,true);
        break;

    case 0x7F:
        Variables_globales.Set_Variable_Global(Flag_Maquina_Juego_Evento,false);
        break;
    
    default:
        break;
    }

    if (Variables_globales.Get_Variable_Global(Enable_Cashless))
    {
        if (Evento == 0x7E)
        {
            Variables_globales.Set_Variable_Global(Status_Games_Machine, true);

            return true;
        }
        else if (Evento == 0x7F)
        {
            Variables_globales.Set_Variable_Global(Status_Games_Machine, false);
            return false;
        }
        else
            return false;
    }
    else
    {
        Variables_globales.Set_Variable_Global(Status_Games_Machine, false);
        return false;
    }
}
/* Si la maquina es EFT  y esta en condicion de pago retorna  True */
bool Cashless_API::Get_Status_Handpay_EFT(void)
{
    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
    {
        if (Variables_globales.Get_Variable_Global(Hand_pay_is_pending))
            return true;
        else
            return false;
    }
    else
        return false;
}

void Cashless_API::Set_Status_Transfer(EstadoTransfer nuevo_estado) {
    if (estado_transfer != nuevo_estado) {
        // Opcional: log o depuración del cambio
        //Serial.printf("[STATUS] Cambio de estado: %d -> %d\n", estado_transfer, nuevo_estado);
    }
    estado_transfer = nuevo_estado;
}

EstadoTransfer Cashless_API::Get_Status_Transfer() const {
    return estado_transfer;
}


unsigned long close_request_time=0;
unsigned long ultimaHoraCorte = 0;
unsigned long inicioSesionEpoch = -1; // Guardará el epoch cuando inicia la sesión
const unsigned long DURACION_SESION = 55 * 60;

/*Apertura y cierre de sesiones sin tarjeta
Se consume metodo Web/API /api/Fidelizacion/ProcesarSesionAcumulada reporta
los contadores iniciales de apertura y finales de cierre*/
void Cashless_API::Count_Player_Sesions(bool Billete_In, bool Flag_Premio)
{
    static bool Sesion_Activa = false;
    static bool Sesion_Sin_Tarjeta_Activa = false;
    static bool Sesion_Con_Tarjeta = false;
    static bool prev_Id_Exist = false;

    static bool Pending_close = false;
    static bool prev_premio_flag = false;

    static bool Reset_Timeout=false;

    static bool Exe = false;

    static bool CreditosOk = false;
    static bool Bill_InS = false;
    static int sesiones_sin_tarjeta = 0;
    bool Id_Exist = Variables_globales.Get_Variable_Global(Flag_Sesion_RFID);
    bool Status_Game_Machine_ = Variables_globales.Get_Variable_Global(Flag_Maquina_En_Juego);
    int Creditos = contadores.Get_Contadores_Int(24);

    bool Premio_finalizado = (prev_premio_flag == true && Flag_Premio == false);
    prev_premio_flag = Flag_Premio;

    /*------------------> Corte cada Hora <---------------------------------- */
    if (Sesion_Activa && Sesion_Sin_Tarjeta_Activa)
    {

        unsigned long ahora = RTC.getEpoch();
        int hora = RTC.getHour(true);
        int minuto = RTC.getMinute();
        int segundo = RTC.getSecond();

        if (hora != inicioSesionEpoch && inicioSesionEpoch != -1)
        {
            sesiones_sin_tarjeta++;
#ifdef DEBUG_SESIONES_A
            Serial.printf("💳❓ Sesión SIN tarjeta terminada por corte de tiempo 🕒🔴: %s\n", RTC.getTimeDate().c_str());
#endif
            Sesion_Sin_Tarjeta_Activa = false;
            Sesion_Activa = false;
#ifdef DEBUG_SESIONES_A
            Serial.printf("📊 Sesiones sin tarjeta acumuladas: %d\n", sesiones_sin_tarjeta);
#endif
            Variables_globales.Set_Variable_Global(Flag_Sesion_Sin_Tarjeta,false);
            Info_Cashless.Nueva_Sesion(Info_Cashless.Get_Info_Sesion_Unknown(false,false));
            Info_Cashless.Log(RTC, "CIERRE_DE_SESION_SIN_TARJETA", "TIEMPO_DE_CORTE_ALCANZADO" + String(Creditos) + "ESTADO_MAQUINA: " + String(Status_Game_Machine_));
            Pending_close = false;

            /* --------------------------> Abre nueva sesion <------------------------------ */
            inicioSesionEpoch = RTC.getHour(true);
            Sesion_Activa = true;
            Sesion_Sin_Tarjeta_Activa = true;

            #ifdef DEBUG_SESIONES_A
            Serial.printf("💳❓ Inicio sesión SIN tarjeta por corte horario 🟢⏰: %s\n", RTC.getTimeDate().c_str());
            #endif

            Variables_globales.Set_Variable_Global(Flag_Sesion_Sin_Tarjeta,true);
            Info_Cashless.Nueva_Sesion(Info_Cashless.Get_Info_Sesion_Unknown(true,true));
            Info_Cashless.Log(RTC, "INICIA_SESION_SIN_TARJETA", "INICIA_NUEVA_SESION_SIN_TARJETA");
            Exe = false;
            tiempo_inicio_sesion = millis();
            /*-------------------------------------------------------------------------------*/
        }
    }
    /*------------------------------------------------------------------------*/
    
    if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
    {

        if (Bill_InS && Creditos >= CREDIT_MINIMUM)
            CreditosOk = true;

        if (millis() - Timeout_Task_PlayerInicial > TIME_EXECUTE || Billete_In)
        {

            if ((Status_Game_Machine_ && !Sesion_Activa && !Sesion_Con_Tarjeta) && Creditos > CREDIT_MINIMUM || (Billete_In && !Sesion_Activa && !Sesion_Con_Tarjeta))
            {

                Variables_globales.Set_Variable_Global(Flag_Sesion_Sin_Tarjeta,true);

                if (!Id_Exist)
                {
                    
                    inicioSesionEpoch = RTC.getHour(true);
                    
                    Sesion_Activa = true;
                    Sesion_Sin_Tarjeta_Activa = true;

                    if (Billete_In)
                    {
#ifdef DEBUG_SESIONES_A
                        Serial.printf("💳❓ Inicio sesión SIN tarjeta por billete insertado 💸🟢: %s\n", RTC.getTimeDate().c_str());
#endif
                        Bill_InS = true;
                    }
                    else
                    {
#ifdef DEBUG_SESIONES_A
                        Serial.printf("💳❓ Inicio sesión SIN tarjeta por coin in 🕹️🟢: %s\n", RTC.getTimeDate().c_str());
#endif
                        CreditosOk = true;
                    }

                    #ifdef DEBUG_SESIONES_A
                    Serial.printf("🕒 Hora Inicio:%02d:%02d:%02d\n",
                                  inicioSesionEpoch, RTC.getHour(true), RTC.getMinute(), RTC.getSecond());
                    #endif

                    Info_Cashless.Nueva_Sesion(Info_Cashless.Get_Info_Sesion_Unknown(true,true));
                    Info_Cashless.Log(RTC, "INICIA_SESION_SIN_TARJETA", "INICIA_NUEVA_SESION_SIN_TARJETA");
                    Exe = false;
                    tiempo_inicio_sesion = millis();
                    //Menssage_TFT("Sesion sin tarjeta iniciada con exito!",2500);
                }
            }

            if (Sesion_Activa && Id_Exist && Sesion_Sin_Tarjeta_Activa)
            {

                Variables_globales.Set_Variable_Global(Flag_Sesion_Sin_Tarjeta,false);

                CreditosOk = false;
                sesiones_sin_tarjeta++;
                Sesion_Sin_Tarjeta_Activa = false;

#ifdef DEBUG_SESIONES_A
                Serial.printf("💳❓ Sesión SIN tarjeta finalizada por inserción RFID 💳🔴: %s\n", RTC.getTimeDate().c_str());
#endif
                Sesion_Activa = false;
                Sesion_Con_Tarjeta = true;
#ifdef DEBUG_SESIONES_A
                Serial.printf("📊 Sesiones sin tarjeta acumuladas: %d\n", sesiones_sin_tarjeta);
#endif
                Exe = false;

                Info_Cashless.Nueva_Sesion(Info_Cashless.Get_Info_Sesion_Unknown(false,false));
                Info_Cashless.Log(RTC, "CIERRE_DE_SESION_SIN_TARJETA", "SE_INICIO_UNA_SESION_CON_TARJETA");
            }

            if (!Sesion_Sin_Tarjeta_Activa && !Id_Exist && !Sesion_Activa && Sesion_Con_Tarjeta)
            {

                if (Creditos > 10 && Status_Game_Machine_)
                {

                    Variables_globales.Set_Variable_Global(Flag_Sesion_Sin_Tarjeta,true);

                    inicioSesionEpoch = RTC.getHour(true);
                    CreditosOk = false;

                    Sesion_Activa = true;
                    Sesion_Sin_Tarjeta_Activa = true;
#ifdef DEBUG_SESIONES_A
                    Serial.printf("Cambio a sesión SIN tarjeta (tarjeta retirada): %s\n", RTC.getTimeDate().c_str());
#endif
                    Sesion_Con_Tarjeta = false;
                    Exe = false;
                    Info_Cashless.Nueva_Sesion(Info_Cashless.Get_Info_Sesion_Unknown(true,true));
                    Info_Cashless.Log(RTC, "INICIA_SESION_SIN_TARJETA_", "CAMBIO_DE_SESION_CON_TARJETA_A_SIN_TARJETA");
                    #ifdef DEBUG_SESIONES_A
                    Serial.printf("🕒 Hora Inicio:%02d:%02d:%02d\n",
                                  inicioSesionEpoch, RTC.getHour(true), RTC.getMinute(), RTC.getSecond());
                    #endif
                }
                else
                {

                    

                    Sesion_Sin_Tarjeta_Activa = false;
                    Sesion_Activa = false;
#ifdef DEBUG_SESIONES_A
                    Serial.printf("💳 Sesion con tarjeta finalizada! no abre sesion sin tarjeta ❌: %s\n", RTC.getTimeDate().c_str());
#endif
                    Sesion_Con_Tarjeta = false;
                    Info_Cashless.Log(RTC, "SESION_CON_TARJETA_TERMINADA", "CREDITOS: " + String(Creditos) + "  ESTADO_MAQUINA: " + String(Status_Game_Machine_));
                    Variables_globales.Set_Variable_Global(Flag_Sesion_Sin_Tarjeta,false);
                }
            }

            if (!Pending_close && Sesion_Activa && !Status_Game_Machine_ && Creditos < CREDIT_MINIMUM && Sesion_Sin_Tarjeta_Activa && !Billete_In && CreditosOk)
            {

                if (Reset_Timeout)
                {
                    Reset_Timeout = false;
                    tiempo_inicio_sesion = millis();
                }

                if ((millis() - tiempo_inicio_sesion) > (Inactividad_Usuario_Player_Tracking-OFFSET_PLAYER_TRACKING))
                {

#ifdef DEBUG_SESIONES_A
                    Serial.println("💳❓ Cierre de sesion sin tarjeta pendiente ⏳");
#endif
                    Pending_close = true;
                    Exe = false;
                    close_request_time = millis();
                }
            }
            else
                Reset_Timeout = true;

            prev_Id_Exist = Id_Exist;
            Timeout_Task_PlayerInicial = millis();
        }

        if (Pending_close && !Exe && Sesion_Activa && Sesion_Sin_Tarjeta_Activa)
        {
            if (Premio_finalizado || millis() - close_request_time > PAYOUT_TIMEOUT||Creditos<CREDIT_MINIMUM)
            {

                CreditosOk = false;
                Exe = true;
                if (Premio_finalizado)
                {
#ifdef DEBUG_SESIONES_A
                    Serial.println("Premio Enviado");
#endif
                }

                sesiones_sin_tarjeta++;
#ifdef DEBUG_SESIONES_A
                Serial.printf("💳❓ Sesión SIN tarjeta terminada por inactividad sin créditos 💤: %s\n", RTC.getTimeDate().c_str());
#endif
                Sesion_Sin_Tarjeta_Activa = false;
                Sesion_Activa = false;
#ifdef DEBUG_SESIONES_A
                Serial.printf("📊 Sesiones sin tarjeta acumuladas: %d\n", sesiones_sin_tarjeta);
#endif
                Info_Cashless.Nueva_Sesion(Info_Cashless.Get_Info_Sesion_Unknown(false,false));
                Info_Cashless.Log(RTC, "CIERRE_DE_SESION_SIN_TARJETA", "CIERRE_DE_SESION_POR_INACTIVIDAD " + String(Creditos) + "ESTADO_MAQUINA: " + String(Status_Game_Machine_));
                Pending_close = false;
                Variables_globales.Set_Variable_Global(Flag_Sesion_Sin_Tarjeta,false);
            }
        }
        else
        {
            if (Pending_close && !Sesion_Activa && !Sesion_Sin_Tarjeta_Activa)
            {
                Pending_close = false;
            }
        }
    }
    else
    {

        if (Sesion_Activa && !Id_Exist && Sesion_Sin_Tarjeta_Activa)
        {
            sesiones_sin_tarjeta++;
            Sesion_Sin_Tarjeta_Activa = false;
#ifdef DEBUG_SESIONES_A
            Serial.printf("💳❓ Sesión SIN tarjeta finalizada por perdida DE comunicacion con la MET 🔌❌: %s\n", RTC.getTimeDate().c_str());
#endif
            Sesion_Activa = false;
            Sesion_Con_Tarjeta = false;
#ifdef DEBUG_SESIONES_A
            Serial.printf("📊 Sesiones sin tarjeta acumuladas: %d\n", sesiones_sin_tarjeta);
#endif
            Info_Cashless.Nueva_Sesion(Info_Cashless.Get_Info_Sesion_Unknown(false,false));
            Info_Cashless.Log(RTC, "CIERRE_DE_SESION_SIN_TARJETA", "PERDIDA_COMUNICACION_CON_LA_MET");
            Exe = false;
            Variables_globales.Set_Variable_Global(Flag_Sesion_Sin_Tarjeta,false);
        }
    }
}

void Cashless_API:: guardarSesiones(void)
{
    File file = SPIFFS.open(SesionesFile, "w");
    if (file) {
        #ifdef DEBUG_SESIONES_A
        Serial.println("Abrio archivo OK");
        #endif
        for (const auto& Sesiones : SesionesPendientes) {
            file.println(Sesiones);
        }
        file.close();
    }else{
        #ifdef DEBUG_SESIONES_A
        Serial.println("No abrio el archivo");
        #endif
    }
}

bool Cashless_API::haySesionesEnArchivo(void)
{
    File file = SPIFFS.open(SesionesFile, "r");
    if (!file) return false;
    bool tieneDatos = file.available();
    file.close();
    return tieneDatos;
}

void Cashless_API::Nueva_Sesion(const String &json)
{

    if (!SesionesPendientes.empty() || haySesionesEnArchivo())
    {

        #ifdef DEBUG_SESIONES_A
        Serial.println("Guarda nueva sesion en memoria por datos pendientes ✅");
        #endif
        SesionesPendientes.push_back(json);
        guardarSesiones(); // Guardan Sesiones en el archivo
    }
    else
    {
        if (!Envia_Sesiones_Unknown(json))
        {
            #ifdef DEBUG_SESIONES_A
            Serial.println("Nueva sesion no enviada ❌");
            #endif
            SesionesPendientes.push_back(json);
            guardarSesiones(); // Guardan Sesiones en el archivo
        }
        else
        {
            #ifdef DEBUG_SESIONES_A
            Serial.println("Nueva sesion reportada...✅");
            #endif
        }
    }
}

void Cashless_API::Intenta_Enviar_Sesiones_Task(void)
{
    Timeout_Sesiones_Pendientes_Inicial = millis();

    if ((Timeout_Sesiones_Pendientes_Inicial - Timeout_Sesiones_Pendientes_Final) >= Timeout_Sesiones_Pendientes)
    {

        if (xSemaphoreTake(mutexSesiones, portMAX_DELAY) == pdTRUE)
        {
            if (!SesionesPendientes.empty())
            {
                bool Cambios = false;
                String Sesiones = SesionesPendientes.front(); /* Toma la primera transferencia */
                if (Envia_Sesiones_Unknown(Sesiones))         /* Intenta enviarla */
                {
                    SesionesPendientes.erase(SesionesPendientes.begin());
                    Cambios = true;
                }

                if (Cambios)
                    guardarSesiones();
            }
            xSemaphoreGive(mutexSesiones);
        }
        Timeout_Sesiones_Pendientes_Final = Timeout_Sesiones_Pendientes_Inicial;
    }
}



String Cashless_API::FormatearBytesComoHex(char* data, size_t length)
{
  String logg;

  for (size_t i = 0; i < length; i++)
  {
    if (i > 0)
      logg += "|";


    String temp = String(data[i], HEX);
    temp.toUpperCase();

    if (data[i] < 0x10)
      logg += "0x0" + temp;
    else
      logg += "0x" + temp;
  }

  logg.replace(" ", "");
  return logg;
}

String Cashless_API::Get_Info_Sesion_Unknown(bool Estado_Sesion,bool flag_contador)
{

    String Output;
    String Msg = "";
    bool IsSuccess = false;
    char IP_Server[4];
    char Current_IP[4];

    String Fecha = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay());
    String Hora = String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));
    std::string Ip = IP_toString_(IP_Server);
    String Ip_Server = String(Ip.c_str());
    std::string Ip_Local = IP_toString_(Current_IP);
    String Ip_Local_Device = String(Ip_Local.c_str());

    StaticJsonDocument<1024> jsonDocument;
    jsonDocument.clear();

    if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
        IsSuccess = true;
    else
        IsSuccess = false;

    jsonDocument["IsSuccess"] = IsSuccess;

    if (Estado_Sesion)
        Msg = "Sesion sin Tarjeta iniciada  🟢";
    else
        Msg = "Sesion sin tarjeta finalizada 🔴";

    jsonDocument["Message"] = Msg;
    jsonDocument["Estado_Sesion"] = Estado_Sesion;
    jsonDocument["Ip"] = Ip_Local_Device;
    jsonDocument["Fecha"] = Fecha;
    jsonDocument["Hora"] = Hora;

    jsonDocument["Total_Cancel_Credit"] = contadores.Get_Contadores_Int(Total_Cancel_Credit);
    jsonDocument["Coin_In"] = contadores.Get_Contadores_Int(Coin_In);
    jsonDocument["Coin_Out"] = contadores.Get_Contadores_Int(Coin_Out);
    jsonDocument["Total_Drop"] = contadores.Get_Contadores_Int(Total_Drop);
    jsonDocument["Jackpot"] = contadores.Get_Contadores_Int(Jackpot);

    jsonDocument["Cancel_Credit_Handpay"] = contadores.Get_Contadores_Int(Cancel_Credit_Hand_Pay);

    if(flag_contador)
        jsonDocument["Bill_Amount"] = Copia_Bill_Amount_Sesiones;
    else
        jsonDocument["Bill_Amount"] = contadores.Get_Contadores_Int(Bill_Amount);
        
    jsonDocument["Total_Juegos"] = contadores.Get_Contadores_Int(Games_Played);

    serializeJson(jsonDocument, Output); /* Serializa Data */

    return Output;
}

bool Cashless_API::Envia_Sesiones_Unknown(const String &json)
{

    bool Code = false;

    if (WiFi.status() == WL_CONNECTED)
    {
        char IP_Server[4];
        int httpCode;

        memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));
        std::string Ip = IP_toString_(IP_Server);
        String Ip_Server = String(Ip.c_str());
        String Puerto = "9595";
        String fwurl = "http://" + Ip_Server + ":" + Puerto + "/api/Fidelizacion/ProcesarSesionAcumulada";
        

        #ifdef DEBUG_SESIONES_A
        Serial.print("🌐 Url servidor: ");
        Serial.println(fwurl);
        #endif

        WiFiClient client;
        HTTPClient https;
        https.setTimeout(5000);
        #ifdef DEBUG_SESIONES_A
        Serial.println("------------------------> Datos de sesion <----------------------------");
        Serial.println(json);
        Serial.println("-----------------------------------------------------------------------");
        #endif
        if (https.begin(client, fwurl))
        {

            https.addHeader("Content-Type", "application/json");
            httpCode = https.POST(json);
          

            #ifdef DEBUG_SESIONES_A
            Serial.print("🌐 Codigo Http: ");
            Serial.println(httpCode);
            #endif


            if (httpCode == HTTP_CODE_OK)
            {

                String Response = https.getString();

                #ifdef DEBUG_SESIONES_A
                Serial.println("--------------------> Respuesta de solicitud 🌐  <---------------------");
                Serial.println(Response);
                Serial.println("------------------------------------------------------------------------");
                #endif
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
        }
    }
    return Code;
}

void Cashless_API::Load_Sesiones_Unknown_Pendientes(void)
{
    if (SPIFFS.exists(SesionesFile))
    {
        File file = SPIFFS.open(SesionesFile, "r");
        if (file)
        {
           
            while (file.available())
            {
                String line = file.readStringUntil('\n');
                Serial.println(line);
                SesionesPendientes.push_back(line);
            }
            file.close();
        }
    }else{

        File file = SPIFFS.open(SesionesFile, "a");
        if (file)
        {
            file.close();
        }
    }
}

void Cashless_API::Task_Sesiones_Unknow(int timeout)
{
    if (Variables_globales.Get_Variable_Global(Flag_Bill_Insert_Sesiones))
    {
        if (Variables_globales.Get_Variable_Global(Flag_Bill_Insert_Sesiones))
            Variables_globales.Set_Variable_Global(Flag_Bill_Insert_Sesiones, false);
        Info_Cashless.Count_Player_Sesions(true);
    }

    if (Variables_globales.Get_Variable_Global(Flag_Cancel_Sesiones))
    {
        if (Variables_globales.Get_Variable_Global(Flag_Cancel_Sesiones))
            Variables_globales.Set_Variable_Global(Flag_Cancel_Sesiones, false);
        Info_Cashless.Count_Player_Sesions(false, true);
    }

    Info_Cashless.Intenta_Enviar_Sesiones_Task();
    Info_Cashless.Count_Player_Sesions();
}

void Cashless_API::Test_Txt(String Msg)
{

    File file = SPIFFS.open(transaccionesFile, "w");
    if (file)
    {
        file.println(Msg);
        file.close();
    }
}



void Cashless_API::Test_Counter(char* Contador,long&valor_test)
{

    if(valor_test==0)
    {
        valor_test = atol(Contador);
    }
    valor_test=valor_test+100;

    snprintf(Contador, 9, "%08ld", valor_test);

    Serial.println(Contador);
}

/*Calcula contador billetero antes de iniciar la sesion sin tarjeta*/
void Cashless_API::Parametros_iniciales_Sesion(char buffer[])
{

    int contador_nuevo = ((buffer[0] - 48) * 10000000) + ((buffer[1] - 48) * 1000000) +
                  ((buffer[2] - 48) * 100000) + ((buffer[3] - 48) * 10000) +
                  ((buffer[4] - 48) * 1000) + ((buffer[5] - 48) * 100) +
                  ((buffer[6] - 48) * 10) + ((buffer[7] - 48) * 1);


    //Serial.println(contador_nuevo);
  
    if (!Variables_globales.Get_Variable_Global(Flag_Sesion_Sin_Tarjeta) && !Sesion_Anterior_Existe && contador_nuevo > contadores.Get_Contadores_Int(Bill_Amount))
    {
        // Serial.println("-------------->Set<--------------");
        // Serial.println(contador_nuevo);
        // Primera vez que sube el contador tras reinicio
        Copia_Bill_Amount_Sesiones = contadores.Get_Contadores_Int(Bill_Amount);
        Sesion_Anterior_Existe = true;
    }
    if (!Variables_globales.Get_Variable_Global(Flag_Sesion_Sin_Tarjeta) && Sesion_Anterior_Existe && contador_nuevo > contadores.Get_Contadores_Int(Bill_Amount))
    {
        // Nueva sesión luego de una sesión cerrada
        Copia_Bill_Amount_Sesiones = contadores.Get_Contadores_Int(Bill_Amount);
    }
}
// void Cashless_API::Start_Game()
// {

// }