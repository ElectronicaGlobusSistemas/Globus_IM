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

#include "Preferences.h"
extern Preferences NVS;

esp_timer_handle_t readTimer;
esp_timer_handle_t Token_Cash;
// Vector para almacenar las transacciones pendientes
std::vector<String> transaccionesPendientes;

// Flag para verificar si hay transacciones pendientes
bool hayTransaccionesPendientes = false;
bool hayTransaccionesPendientes_Download = false;
bool Transfer_Pending_Load=false;
bool Transfer_Pending_Download=false;


bool Update_In_Cashless=false;
bool Update_Out_Cashless=false;
// Archivo para almacenar transacciones pendientes
const char* transaccionesFile = "/transacciones.txt";
extern const char* archivo;
//const char* LogError = "/Loggin.txt";
// Intervalo de reintento (60 segundos)
const unsigned long intervaloReintento = 30000;
bool  Reset_Timer_Tranfer_Pending=false;
unsigned long tiempoUltimoReintento = 0;
unsigned long Tiempo_Inicio_Intento=0;

int CodeHttp=0;

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


//#define DEBUG_RFID
extern unsigned long New_Timer_Final;
extern unsigned long New_Timmer_Inicial;

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
char Buffer_Info_Lector[200];


DynamicJsonDocument Objeto_Transfer(800);
DynamicJsonDocument Objeto_Transfer_Download(800);
/*-----------------------------------------------------------------------------------------------*/
/* ------------------------------> Instancias <-------------------------------------------------- */
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

/**********************************************************************************/
/*                              CONECTA MÓDULO DESPUES DE INICIO                  */
/**********************************************************************************/
void Check_RFID(void)
{
    scanner.Init();
   
    for (uint8_t index = 0; index < num_addresses; index++)
	{
		results[index] = scanner.Check(addresses[index]);
	}
	
	for (uint8_t index = 0; index < num_addresses; index++)
	{
		if (results[index])
		{
			//Serial.print("Found device ");
			//Serial.print(index);
			//Serial.print(" at address ");
			//Serial.println(addresses[index], HEX);
            VERIFY_WIRE_CONNECTION=true;
		}
	}
    if (VERIFY_WIRE_CONNECTION)
    {
        pcf8574.begin(); // Inicializa Expansor I2C
        pcf8574.pinMode(P1, OUTPUT);
        pcf8574.pinMode(P0, INPUT);
        pcf8574.pinMode(P2, OUTPUT);
        
        

        mfrc522.PCD_Init(); // Inicializa Módulo RFID
        Status_Barra(INICIO_MODULO);
        delay(100);
        mfrc522.PCD_Init();                              // Inicializa Módulo RFID
        mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_43dB); // Activa  antena con ganancia 33dB RxGain_33dB
        byte gain = mfrc522.PCD_GetAntennaGain();        // Obtiene  configuracion de antena  para verifica conexión  de modulo RFID

        if (gain >= 0 && gain <= 1000)
        {
            /* --------------------------- >Configura RIFD <------------------------------------*/
            mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_43dB); /* Configura Gancia en la antena*/
#ifdef Init_RFID_
            Serial.println("Modulo RFID Inicializado....");
#endif
            Variables_globales.Set_Variable_Global(Conexion_RFID, true); /*Modulo OK*/
            Status_Barra(MODULO_OK);
            Variables_globales.Set_Variable_Global(Verify_Modulo_RFID,true);
        }
        else
        {
#ifdef Init_RFID_
            Serial.println("Error Inicializando Modulo");
#endif
            Variables_globales.Set_Variable_Global(Conexion_RFID, false);
            Status_Barra(MODULO_KO);
            Intentos_Conect_RFID++;
        }
    }else{
        Serial.println("Modulo RFID no Conectado...");
        Intentos_Conect_RFID++;
    }
}

/**********************************************************************************/
/*                              Inicializa Modulo RFID                            */
/**********************************************************************************/
void Init_RFID(void)
{
    /*---------------> SCAN I2C <----------------------------*/
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
			//Serial.print("Found device ");
			//Serial.print(index);
			//Serial.print(" at address ");
			//Serial.println(addresses[index], HEX);
            VERIFY_WIRE_CONNECTION=true;
		}
	} 
    /*-------------------------------------------------------*/
    if (VERIFY_WIRE_CONNECTION)
    {
        pcf8574.begin(); // Inicializa Expansor I2C
        pcf8574.pinMode(P1, OUTPUT);
        pcf8574.pinMode(P0, INPUT);
        pcf8574.pinMode(P2, OUTPUT);
        
        

        mfrc522.PCD_Init(); // Inicializa Módulo RFID
        Status_Barra(INICIO_MODULO);
        delay(100);
        mfrc522.PCD_Init();                              // Inicializa Módulo RFID
        mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_43dB); // Activa  antena con ganancia 33dB RxGain_33dB
        byte gain = mfrc522.PCD_GetAntennaGain();        // Obtiene  configuracion de antena  para verifica conexión  de modulo RFID

        if (gain >= 0 && gain <= 125)
        {
            
            /* --------------------------- >Configura RIFD <------------------------------------*/
            mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_43dB); /* Configura Gancia en la antena*/
#ifdef Init_RFID_
            Serial.println("Modulo RFID Inicializado....");
#endif
            Variables_globales.Set_Variable_Global(Conexion_RFID, true); /*Modulo OK*/
            Status_Barra(MODULO_OK);
            Variables_globales.Set_Variable_Global(Verify_Modulo_RFID,true);
        }
        else
        {
#ifdef Init_RFID_
            Serial.println("Error Inicializando Modulo");
#endif
            Variables_globales.Set_Variable_Global(Conexion_RFID, false);
            Status_Barra(MODULO_KO);
        }
    }else{
        Serial.println("Modulo RFID no Conectado...");
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


/* Metodo para leer tarjetas RFID usuario - operador  */
void Lee_Tarjeta()
{
    Verify_Status_RFID=millis();

    Start_Cambio_Color = millis();
    byte block;
    byte len;
    byte block_2;
    MFRC522::StatusCode status;

  //  Check_RFID_Real_Time();
    
    
        /* Habilitado */

        /*----------------------------------------> Indicador estados del lector <---------------------------------------------*/
        if (!Variables_globales.Get_Variable_Global(Conexion_RFID) || !Variables_globales.Get_Variable_Global(Comunicacion_Maq) || Variables_globales.Get_Variable_Global(Flag_Sesion_RFID) ||Variables_globales.Get_Variable_Global(Updating_System) || Variables_globales.Get_Variable_Global(Access_Point_Mode))
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
                if (!Variables_globales.Get_Variable_Global(Comunicacion_Maq) && Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9 && !Variables_globales.Get_Variable_Global(Updating_System)&&!Variables_globales.Get_Variable_Global(Access_Point_Mode) &&Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 15)
                {

                    TimeOut_Automatico_Inicial = millis();

                    if ((TimeOut_Automatico_Inicial - TimeOut_Automatico) >= Close_Automatico && Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
                    {

                        if(Info_Cashless.Type_Sesion() != PLAYER_CASHLESS_SESION)
                            Close_Sesion_Player_Tracking();
                        TimeOut_Automatico = TimeOut_Automatico_Inicial;
                    }
                    if (!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                    {
                        Status_Barra(NO_HAY_COMUNICACION);
                    }
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

            }
        }
        /*-----------------------------------------------------------------------------------------------------------------*/
        if (Variables_globales.Get_Variable_Global(Conexion_RFID) && Variables_globales.Get_Variable_Global(Comunicacion_Maq) && Info_Cashless.Get_Status_Reader()==false&& !Variables_globales.Get_Variable_Global(Updating_System) && !Variables_globales.Get_Variable_Global(Access_Point_Mode))
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
            if ((!mfrc522.PICC_IsNewCardPresent()))
            {
                return;
            }

            if (!mfrc522.PICC_ReadCardSerial())
            {
                return;
            }

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

#ifdef DEBUG_RFID
                Serial.print(F("Authentication failed: "));
                Serial.println(mfrc522.GetStatusCodeName(status));
#endif
                //Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
               
                RESET_Handle();
                return;
            }

            status = mfrc522.MIFARE_Read(block, buffer1, &len);
            if (status != MFRC522::STATUS_OK)
            {

#ifdef DEBUG_RFID
                Serial.print(F("Reading failed: "));
                Serial.println(mfrc522.GetStatusCodeName(status));
#endif
               // Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                RESET_Handle();
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

            /*-----------------------------> Verifica Usuario Valido <--------------------------------*/
            if (!contadores.Verity_ID_NOT_NULL(buffer2, 'M'))
            {
               // Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                Status_Barra(ERROR_LECTURA);
                return;
            }
            else if (!contadores.Verify_Client_ID(buffer2))
            {
               // Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                Status_Barra(ERROR_LECTURA);
                return;
            }
            else if (!contadores.Verity_ID_NOT_NULL(buffer1, 'C'))
            {
               // Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                Status_Barra(ERROR_LECTURA);
                return;
            }
            /*------------------------------------------------------------------------------------------*/
            /*---------------------------------->Usuario Valido <----------------------------------------*/
            else
            {
                Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                Cliente_VS_Operador(buffer1, buffer2);
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
        if (Variables_globales.Get_Variable_Global(Enable_Cashless) && Configuracion.Get_Configuracion(Tipo_Maquina, 0) < 4)
        {
            if (Variables_globales.Get_Variable_Global(Excepcion_51) && !Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss)) /* Maquina en estado de reset handpay */
            {

                Variables_globales.Set_Variable_Global(Requerimiento_Operador,true);
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
                }
            }
            else
            {
                if (Info_Cashless.Type_Sesion() == PLAYER_CASHLESS_SESION) /* Pregunta si tiene sesión Cashless */
                {

                    char ID_Temp_[8];
                    ID_Temp_[0] = INFO[0];
                    ID_Temp_[1] = INFO[1];
                    ID_Temp_[2] = INFO[2];
                    ID_Temp_[3] = INFO[3];
                    ID_Temp_[4] = INFO[4];
                    ID_Temp_[5] = INFO[5];
                    ID_Temp_[6] = INFO[6];
                    ID_Temp_[7] = INFO[7];

                    Status_Barra(TARJETA_OPERADOR_INSERT);

                    if (Info_Cashless.Valida_Operador_Cashless(ID_Temp_))
                    {
                        switch (Info_Cashless.Info_Client_Download(contadores.Get_Client_ID_Transaccion(), RTC, "D", Cashless.Get_Trans_ID_Int()))
                        {
                        case REQUEST_SUCCESSFULLY_RECEIVED:
                            Solicitud_Descarga_Cashless();
                            break;

                        default:
                            Status_Barra(ERROR_LECTURA);
                            Info_Cashless.Unlock_Reader();
                            break;
                        }
                    }
                    else
                    {
                        Status_Barra(ERROR_LECTURA);
                        Info_Cashless.Unlock_Reader();
                    }
                }
                else
                {

                    Variables_globales.Set_Variable_Global(Excepcion_51,false);
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
                    }
                }
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
        
        Status_Barra(LECTURA_OK);

        Status_Barra(301); /*Notificacion de espera...*/
       
        if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) < 4)
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
                    switch (Info_Cashless.Type_Sesion())
                    {
                    case PLAYER_TRACKING_SESION: /*  Solicitud manual de cierre  Sesion Player Tracking  */

                        Info_Cashless.Close_Player_Tracking_Sesion(true);
                        Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
                        contadores.Close_ID_Client_Transaccion();
                        Handle=true;
                        break;

                    case PLAYER_CASHLESS_SESION: /* Solicitud de cierre Sesion Player Cashless */
                        /* Descarga maquina*/
                        switch (Info_Cashless.Info_Client_Download(contadores.Get_Client_ID_Transaccion(),RTC,"D",Cashless.Get_Trans_ID_Int()))
                        {
                        case REQUEST_SUCCESSFULLY_RECEIVED:
                            Solicitud_Descarga_Cashless();
                            break;

                        default:
                            Status_Barra(ERROR_LECTURA);
                            Info_Cashless.Unlock_Reader(); /* Habilita lector */
                            Handle=true;
                            break;
                        }
                        break;

                    default: /*  Solicitud manual de cierre  Sesion Player Tracking  */
                        Status_Barra(ERROR_LECTURA);
                        Handle=true;
                        break;
                    }
                }else{
                    Info_Cashless.Close_Player_Tracking_Sesion(true);
                    Handle=true;
                }

                if(Handle)
                    Info_Cashless.Unlock_Reader();  /* Habilita lector */
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

                    /* Logica: Si Existe una  Sesion Player Cashless Activa anterior y un usuario  se identifica
                    Se verifica si los creditos son <10. Si es asi, Cierra la sesión anterior espera hasta completar y inicia una lasesion del cliente nuevo.
                    Por otro lado, si la sesión anterior tiene creditos  >=10 Permite el cierre solo por Operador. */

                    bool Error=false;
                    unsigned long Timout_Break;
                    int Stop_Transaccion = 6500; // 3500


                   // bool Error=false;
                    unsigned long Timout_Break_Controller;
                    int Stop_Transaccion_Controller = 6500; // 3500

                    if(Info_Cashless.Type_Sesion()==PLAYER_CASHLESS_SESION)
                    {
                        
                        if (!Verify_Current_Credit_Cashless(Buffer_Cashless.Get_RX_AFT(Buffer_RX_Cashless)))
                        {
                            switch (Info_Cashless.Info_Client_Download(contadores.Get_Client_ID_Transaccion(), RTC, "D", Cashless.Get_Trans_ID_Int()))
                            {
                            case REQUEST_SUCCESSFULLY_RECEIVED:
                                Error=true;
                                Solicitud_Descarga_Cashless();
                                
                                break;
                            default:
                                Error = true;
                                Status_Barra(ERROR_LECTURA);
                                Handle=true;
                                break;
                            }
                        }else
                        {
                            Error=true;
                            Status_Barra(ERROR_LECTURA);
                            Handle=true;
                        }

                        if(Handle)
                            Info_Cashless.Unlock_Reader();
                    }

                    if (!Error)
                    {

                        Error = false;

                        if (transaccionesPendientes.empty())
                        {
                            /*  Abre nueva Sesion */
                            contadores.Set_Client_ID_Transaccion(ID_Temp); /* Guarda ID de cliente */
                            switch (Info_Cashless.Info_Client(contadores.Get_Client_ID_Transaccion(), RTC, LOAD_TRANSACTION, Cashless.Get_Trans_ID_Int()))
                            {
                            case REQUEST_SUCCESSFULLY_RECEIVED:
                                Solicitud_Carga_Cashless();
                                break;

                            case INSUFFICIENT_BALANCE:
                                Solicitud_Carga_Cashless();
                                break;

                            case TRANS_ID_NO_MACTH:
                                Objeto_Transfer["IsSuccess"] = false;
                                Objeto_Transfer["Trans_Estado"] = TRANS_ID_NO_MACTH;
                                hayTransaccionesPendientes = true;
                                Status_Barra(ERROR_LECTURA);
                                Handle = true;
                                break;

                            case CLIENT_NOT_MACTH:
                                Objeto_Transfer["IsSuccess"] = false;
                                Objeto_Transfer["Trans_Estado"] = CLIENT_NOT_MACTH;
                                hayTransaccionesPendientes = true;
                                Status_Barra(ERROR_LECTURA);
                                Handle = true;
                                break;

                            case INVALID_BALANCE:
                                Objeto_Transfer["IsSuccess"] = false;
                                Objeto_Transfer["Trans_Estado"] = INVALID_BALANCE;
                                hayTransaccionesPendientes = true;
                                // Variables_globales.Set_Variable_Global(Flag_Sesion_Cashless, true);
                                // Info_Cashless.Init_Player_Tracking_Sesion(contadores.Get_Client_ID_Transaccion());
                                Status_Barra(ERROR_LECTURA);
                                Handle = true;
                                break;

                            case TYPE_TRANS_NOT_MACTH:
                                Objeto_Transfer["IsSuccess"] = false;
                                Objeto_Transfer["Trans_Estado"] = TYPE_TRANS_NOT_MACTH;
                                hayTransaccionesPendientes = true;
                                Handle = true;
                                Status_Barra(ERROR_LECTURA);
                                break;

                            case PROBLEM_WITH_THE_SERVER: /* CLIENTE BLOQUEADO */
                                Status_Barra(ERROR_LECTURA);
                                Handle = true;
                                break;

                            case NOT_CONEXION_WITH_SERVER: /* NO CONEXION CON EL SERVIDOR */
                                Status_Barra(CONEXION_TO_HOTS_FAILED);
                                Handle = true;
                                break;

                            case NOT_COMMUNICATION_WITH_THE_MACHINE:
                                Objeto_Transfer["IsSuccess"] = false;
                                Objeto_Transfer["Trans_Estado"] = NOT_COMMUNICATION_WITH_THE_MACHINE;
                                hayTransaccionesPendientes = true;
                                Status_Barra(CONEXION_TO_HOTS_FAILED);
                                Handle = true;
                                break;
                            case NOT_WIFI_CONNECTION:
                                Objeto_Transfer["IsSuccess"] = false;
                                Objeto_Transfer["Trans_Estado"] = NOT_WIFI_CONNECTION;
                                hayTransaccionesPendientes = true;
                                Status_Barra(CONEXION_TO_HOTS_FAILED);
                                Handle = true;
                                break;

                            case TYPE_MACHINE_NOT_MACTH:
                                Objeto_Transfer["IsSuccess"] = false;
                                Objeto_Transfer["Trans_Estado"] = TYPE_MACHINE_NOT_MACTH;
                                hayTransaccionesPendientes = true;
                                Status_Barra(ERROR_LECTURA);
                                Handle = true;
                                break;

                            default:
                                Objeto_Transfer["IsSuccess"] = false;
                                Objeto_Transfer["Trans_Estado"] = NOT_COMMUNICATION_WITH_THE_MACHINE;
                                hayTransaccionesPendientes = true;
                                Status_Barra(ERROR_LECTURA);
                                Handle = true;
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
                        {
                            /* Pendiente por Reportar transaccion */
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

                    char ID_Temp_Tarjeta[8];
                    ID_Temp_Tarjeta[0] = INFO[0];
                    ID_Temp_Tarjeta[1] = INFO[1];
                    ID_Temp_Tarjeta[2] = INFO[2];
                    ID_Temp_Tarjeta[3] = INFO[3];
                    ID_Temp_Tarjeta[4] = INFO[4];
                    ID_Temp_Tarjeta[5] = INFO[5];
                    ID_Temp_Tarjeta[6] = INFO[6];
                    ID_Temp_Tarjeta[7] = INFO[7];

                    contadores.ID_Consulta_INFO_Client(ID_Temp_Tarjeta); /* Setea Id cliente + tipo C */
                    Variables_globales.Set_Variable_Global(Consulta_Conexion_To_Host, true);
                    
                    bool Handl=true;
                    unsigned long Respuesta_Server = millis();
                    int TIMEOUT_CONECT_SERVER = 6500; // 3500
                    while (!Variables_globales.Get_Variable_Global(Conexion_To_Host) && millis() - Respuesta_Server < TIMEOUT_CONECT_SERVER)
                    {
#ifdef DEBUG_RFID
                        Serial.println("Verificando Conexion to Host...");
#endif
                        vTaskDelay(300);
                    }
                    if (Variables_globales.Get_Variable_Global(Conexion_To_Host))
                    {
                        Info_Cashless.Init_Player_Tracking_Sesion(ID_Temp);
                        Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                        Info_Cashless.Unlock_Reader();
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

                    if(Handl)
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
                Info_Cashless.Unlock_Reader();
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
                contadores.ID_Consulta_INFO_Client(ID_Temp_Tarjeta); /* Setea Id cliente + tipo C */
                Variables_globales.Set_Variable_Global(Consulta_Conexion_To_Host, true);
                
                unsigned long Respuesta_Server = millis();
                int TIMEOUT_CONECT_SERVER = 6500; // 3500
                while (!Variables_globales.Get_Variable_Global(Conexion_To_Host) && millis() - Respuesta_Server < TIMEOUT_CONECT_SERVER)
                {
#ifdef DEBUG_RFID
                    Serial.println("Verificando Conexion to Host...");
#endif
                    vTaskDelay(300);
                }
                if (Variables_globales.Get_Variable_Global(Conexion_To_Host))
                {
                    Info_Cashless.Init_Player_Tracking_Sesion(ID_Temp);
                    Info_Cashless.Unlock_Reader();
                    Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
                }
                else
                {
                    Info_Cashless.Unlock_Reader();
                    Variables_globales.Set_Variable_Global(Conexion_To_Host, false);
#ifdef DEBUG_RFID
                    Serial.println("Host Gmaster disconected");
#endif
                    Status_Barra(CONEXION_TO_HOTS_FAILED);
                    delay(100);
                }
            }

            Info_Cashless.Unlock_Reader();
        }
        
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
    int R,G,B;
    int intensidad;
    int Style; 
    int NUM;
    switch (Figura)
    {
    case 1:/*Sesion de juego disponible*/
        Barra_Status_Sesion_Client.setBrightness(20); /* Configura Brillo*/
        Barra_Status_Sesion_Client.setPixelColor(0, 255, 0, 0); 
        Barra_Status_Sesion_Client.setPixelColor(1, 255, 0, 0); 
        Barra_Status_Sesion_Client.setPixelColor(2, 255, 0, 0); 
        Barra_Status_Sesion_Client.setPixelColor(3, 255, 0, 0); 
        Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
    break;

    case 2: /*Random de colores*/
        Barra_Status_Sesion_Client.setBrightness(20); /* Configura Brillo*/
        for (int i = 0; i < 5; i++)
        {
                R = random(256);                                  
                G = random(256);                                  
                B = random(256);
                if(R!=255 && G!=0 && B!=0)
                {
                    Barra_Status_Sesion_Client.setPixelColor(i, R, G, B);
                }                                  
        }
        Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
    break;
   
    case 3: /*Desplazamiento Derecha-Izquierda*/
        Barra_Status_Sesion_Client.setBrightness(20); /* Configura Brillo*/
        R = random(256);                                  
        G = random(256);                                  
        B = random(256);

        for(int i=0; i<5;i++)
        {
            Barra_Status_Sesion_Client.setPixelColor(i,R,G,B);
            Barra_Status_Sesion_Client.show();
            delay(150);
        }

        for(int i=3; i>=0;i--)
        {
            Barra_Status_Sesion_Client.setPixelColor(i,R,G,B);
            Barra_Status_Sesion_Client.show();
            delay(150); 
        }

    break;
    
    case 4:/*Solo un Bit*/
        Barra_Status_Sesion_Client.setBrightness(20); /* Configura Brillo*/
        R = random(256);
        G = random(256);
        B = random(256);

        for (int i = 0; i < 5; i++)
        {
                Barra_Status_Sesion_Client.setPixelColor(i, R, G, B);
                Barra_Status_Sesion_Client.show();
                delay(150);
                Barra_Status_Sesion_Client.setPixelColor(i,0,0,0);
                Barra_Status_Sesion_Client.show();
        }

        for (int i = 3; i >= 0; i--)
        {
                Barra_Status_Sesion_Client.setPixelColor(i, R, G, B);
                Barra_Status_Sesion_Client.show();
                delay(150);
                Barra_Status_Sesion_Client.setPixelColor(i,0,0,0);
                Barra_Status_Sesion_Client.show();
        }
    break;


    case 5:/* Aleatorio posición*/
        Barra_Status_Sesion_Client.setBrightness(20); /* Configura Brillo*/
        R = random(256);
        G = random(256);
        B = random(256);
        for(int i=0; i<5; i++)
        {
            int pos = random(0, 4);
            Barra_Status_Sesion_Client.setPixelColor(pos, R, G, B);
            Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
            delay(150);
            Barra_Status_Sesion_Client.setPixelColor(pos, 0, 0, 0);
            Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
        }
        
    break;

    case 6: /*Cambia intensidad*/
         intensidad =random(10,20);
         Style =random(2,5);
         Barra_Status_Sesion_Client.setBrightness(intensidad); /* Configura Brillo*/
         Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED

         switch (Style)
         {
         case 2:
            for (int i = 0; i < 5; i++)
            {
                R = random(256);
                G = random(256);
                B = random(256);
                if (R != 255 && G != 0 && B != 0)
                {
                    Barra_Status_Sesion_Client.setPixelColor(i, R, G, B);
                }
            }
            Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED

            break;
         case 3:

            R = random(256);
            G = random(256);
            B = random(256);

            for (int i = 0; i < 5; i++)
            {
                Barra_Status_Sesion_Client.setPixelColor(i, R, G, B);
                Barra_Status_Sesion_Client.show();
                delay(150);
            }

            for (int i = 3; i >= 0; i--)
            {
                Barra_Status_Sesion_Client.setPixelColor(i, R, G, B);
                Barra_Status_Sesion_Client.show();
                delay(150);
            }

            break;
        case 4:

            R = random(256);
            G = random(256);
            B = random(256);

            for (int i = 0; i < 5; i++)
            {
                Barra_Status_Sesion_Client.setPixelColor(i, R, G, B);
                Barra_Status_Sesion_Client.show();
                delay(150);
                Barra_Status_Sesion_Client.setPixelColor(i, 0, 0, 0);
                Barra_Status_Sesion_Client.show();
            }

            for (int i = 3; i >= 0; i--)
            {
                Barra_Status_Sesion_Client.setPixelColor(i, R, G, B);
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
                Barra_Status_Sesion_Client.setPixelColor(pos, R, G, B);
                Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
                delay(150);
                Barra_Status_Sesion_Client.setPixelColor(pos, 0, 0, 0);
                Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
            }
        break;
        
        }
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
    mfrc522.PCD_Reset();
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
            Barra_Status_Sesion_Client.begin();
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
            if (millis() - Encendido >= 1000)
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

    StaticJsonDocument<800> jsonDocument;
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

    // case 5:
    //     jsonDocument["Tipo_Maq"] = "AFT"; /* Eliminar*/
    //     break;
    default:
        jsonDocument["Tipo_Maq"] = "";
        break;
    }


    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    https.setTimeout(15000);
    
    if (https.begin(client, fwurl))
    {
        
        https.addHeader("Content-Type", "application/json");
        https.addHeader("hash",Info_Cashless.Get_Hash_Valido());
        https.addHeader("gmsec","GMaster");
        https.addHeader("Authorization", "Bearer " + Info_Cashless.Get_Token_Valido());
        
        httpCode = https.POST(Json);

      //  Serial.println(httpCode);
         
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
                    int Cashless_ID=doc["Cashless_ID"];
                    String Cashless_Estado=doc["Cashless_Estado"];
                    String Ip_Tarjeta=doc["Ip"];
                    String Key=doc["Key"];
                    String Mac=doc["MAC"];
                    String Trans_Estado=doc["Trans_Estado"];
                    String Tipo_Maq=doc["Tipo_Maq"];
    
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
                    Objeto_Transfer["Cashless_ID"] = Cashless_ID;
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

                    default:
                        Tipo_Maq_Local="";
                        break;
                    }

                    if (IsSuccess)
                    {

                        if(WiFi.status()!=WL_CONNECTED)
                        {
                            Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(NOT_WIFI_CONNECTION,HEX));
                            Code= NOT_WIFI_CONNECTION; /* Envia ACK */
                        }
                        else if(Tipo_Maq!=Tipo_Maq_Local)
                        {
                            Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(TYPE_MACHINE_NOT_MACTH,HEX));
                            Code=TYPE_MACHINE_NOT_MACTH; /* Envia ACK */
                        }
                        else if(!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                        {
                            Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(NOT_COMMUNICATION_WITH_THE_MACHINE,HEX));
                            Code=NOT_COMMUNICATION_WITH_THE_MACHINE; /* Envia ACK */
                        }
                       
                        else if (Current_Cliente_ID_Server_Int != contadores.Get_Client_ID_Transaccion_Int())
                        {
                            Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(CLIENT_NOT_MACTH,HEX));
                            Code = CLIENT_NOT_MACTH; /* Envia ACK */
                        }

                        else if (Type_Trans_Server != Type_Transaction)
                        {
                            Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(TYPE_TRANS_NOT_MACTH,HEX));
                            Code = TYPE_TRANS_NOT_MACTH; /* Envia ACK */
                        }

                        else if (Trans_ID_Server != Transaction_ID)  
                        {
                            Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(TRANS_ID_NO_MACTH,HEX));
                            Code = TRANS_ID_NO_MACTH; /* Envia ACK */
                        }

                        else if (!doc["Saldo_Canjeable"].is<uint32_t>() || !doc["Saldo_Restringido"].is<uint32_t>() || !doc["Saldo_No_Restringido"].is<uint32_t>())
                        {
                            Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(INVALID_BALANCE,HEX));
                            Code = INVALID_BALANCE; /* Envia ACK */
                        }
                            

                        else if (Cashable_Server == 0 && Restricted_Server == 0 && Non_Restricted_Server == 0)
                        {
                            if(Cashless.Set_Amount_To_Load(Cashable_Server,Restricted_Server,Non_Restricted_Server))
                                Code = REQUEST_SUCCESSFULLY_RECEIVED; /* OK */
                            else
                                {
                                    Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(INVALID_BALANCE,HEX));
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
                        Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(PROBLEM_WITH_THE_SERVER,HEX));
                        Code = PROBLEM_WITH_THE_SERVER; /* Error en servidor HTTP */
                    }
            }

            doc.clear();
        }
        else
        {
            Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(httpCode));
            Code=NOT_CONEXION_WITH_SERVER; /* No se logro establecer comunicación con el servidor HTTP */
        }
        https.end();

      //  Serial.println(Code);
        return Code;
    }
    Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_CARGA",String(NOT_CONEXION_WITH_SERVER,HEX));
    Code=NOT_CONEXION_WITH_SERVER;
    return Code;
}

int Cashless_API::Info_Client_Download(byte Id_Client[], ESP32Time RTC, String Type_Transaction, uint32_t Transaction_ID)
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

    StaticJsonDocument<800> jsonDocument;
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
    
    default:
        jsonDocument["Tipo_Maq"] = "";
        break;
    }

    String Json;
    serializeJson(jsonDocument, Json); /* Serializa Data */
    
   
    
    https.setTimeout(15000);
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
                    int Cashless_ID=doc["Cashless_ID"];
                    String Cashless_Estado=doc["Cashless_Estado"];
                    String Ip_Tarjeta=doc["Ip"];
                    String Key=doc["Key"];
                    String Mac=doc["MAC"];
                    String Trans_Estado=doc["Trans_Estado"];
                    String Tipo_Maq=doc["Tipo_Maq"];

    
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
                    Objeto_Transfer_Download["Cashless_ID"] = Cashless_ID;
                    Objeto_Transfer_Download["Cashless_Estado"] = Cashless_Estado;
                    Objeto_Transfer_Download["Ip"] = Ip_Tarjeta;
                    Objeto_Transfer_Download["Key"] = Key;
                    Objeto_Transfer_Download["MAC"] = Mac;
                    Objeto_Transfer_Download["Trans_Estado"] = Trans_Estado;
                    Objeto_Transfer_Download["Tipo_Maq"]=Tipo_Maq;


                    if (IsSuccess)
                    {
                        Code = REQUEST_SUCCESSFULLY_RECEIVED; /* OK */
                    }
                    else
                    {
                        Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_DESCARGA",String(PROBLEM_WITH_THE_SERVER,HEX));
                        Code = PROBLEM_WITH_THE_SERVER; /* Error en servidor HTTP */
                    }
            }

            doc.clear();
        }
        else
        {
            Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_DESCARGA",String(httpCode));
            Code=NOT_CONEXION_WITH_SERVER; /* No se logro establecer comunicación con el servidor HTTP */
        }
        https.end();
        return Code;
    }

    Info_Cashless.Log(RTC,"ERROR_SOLICITUD_INFO_CLIENTE_DESCARGA",String(NOT_CONEXION_WITH_SERVER,HEX));
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

    String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

    /* Actualiza Objeto Carga */
    if (Code == 0x00 || Code == 0x01)
        Objeto_Transfer["IsSuccess"] = true;
    else
        Objeto_Transfer["IsSuccess"] = false;

    Objeto_Transfer["Cliente_ID"]= contadores.Get_Client_ID_Transaccion_Int();
    Objeto_Transfer["Trans_Tipo"] = "C";
    Objeto_Transfer["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 7);
    Objeto_Transfer["Saldo_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, RESTRICTED, 12);
    Objeto_Transfer["Saldo_No_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, NON_RESTRICTED, 17);
    Objeto_Transfer["Trans_ID"] = Cashless.Get_Trans_ID_Int();
    Objeto_Transfer["Fecha_Hora"] = DataTime;
    Objeto_Transfer["Trans_Estado"] = Code;

    String Json;
    serializeJson(Objeto_Transfer, Json); /* Serializa Data */
    Info_Cashless.Reader_Lock(false);
    Info_Cashless.Reader_Lock(false);
    Info_Cashless.Reader_Lock(false);
    Info_Cashless.Reader_Lock(false);
    hayTransaccionesPendientes = true;
    //Serial.println(Json);
    if(Code == 0x00 || Code == 0x01)
        Saves_Current_Player_Sesion(contadores.Get_Client_ID_Transaccion(),Info_Cashless.Type_Sesion());
    
    return true;
}

/*Actualiza objeto  para reporte de  estado  pendiente de transaccion*/
bool Cashless_API::Ack_Transfer_Pending(int Code, char Buffer_Transfer[], ESP32Time RTC,String Type_Transaccion)
{


    if (Type_Transaccion == LOAD_TRANSACTION)
    {
        String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

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
        Transfer_Pending_Load = true;
        
    }
    else
    {
        String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
        Objeto_Transfer_Download["IsSuccess"] = false;
        Objeto_Transfer_Download["Cliente_ID"] = contadores.Get_Client_ID_Transaccion_Int();
        Objeto_Transfer_Download["Trans_Tipo"] = "D";
        Objeto_Transfer_Download["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 12);
        Objeto_Transfer_Download["Saldo_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, RESTRICTED, 17);
        Objeto_Transfer_Download["Saldo_No_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, NON_RESTRICTED, 22);
        Objeto_Transfer_Download["Trans_ID"] = Cashless.Get_Trans_ID_Int();
        Objeto_Transfer_Download["Fecha_Hora"] = DataTime;
        Objeto_Transfer_Download["Trans_Estado"] = Code;
        Transfer_Pending_Download=true;
       
    }
    return true;
}

/* Actualiza objeto  para reporte de transacciones de descarga  */
bool Cashless_API::Status_Transfer_Download(int Code,char Buffer_Transfer[],ESP32Time RTC)
{

    String DataTime=String (RTC.getYear())+"-"+String(RTC.getMonth() + 1)+"-"+String(RTC.getDay())+" "+String(RTC.getHour(true))+":"+String (RTC.getMinute())+":"+String(RTC.getSecond());
    
    char Current_IP[4];
    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));

    if(Code==0x00||Code==0x01)
        Objeto_Transfer_Download["IsSuccess"]=true;
    else
        Objeto_Transfer_Download["IsSuccess"]=false;

    Objeto_Transfer_Download["Cliente_ID"] = contadores.Get_Client_ID_Transaccion_Int();
    Objeto_Transfer_Download["Trans_Tipo"] = "D";
    Objeto_Transfer_Download["Saldo_Canjeable"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, CASHABLES, 12);
    Objeto_Transfer_Download["Saldo_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, RESTRICTED, 17);
    Objeto_Transfer_Download["Saldo_No_Restringido"] = Cashless.BCDtoUint32_Pos(Buffer_Transfer, NON_RESTRICTED, 22);
    Objeto_Transfer_Download["Trans_ID"] = Cashless.Get_Trans_ID_Int();
    Objeto_Transfer_Download["Fecha_Hora"] = DataTime;
    Objeto_Transfer_Download["Trans_Estado"] = Code;
    Info_Cashless.Reader_Lock(false);
    Info_Cashless.Reader_Lock(false);
    Info_Cashless.Reader_Lock(false);
    Info_Cashless.Reader_Lock(false);
    Sesion_Anterior=true;
    String Json;
    serializeJson(Objeto_Transfer_Download, Json); /* Serializa Data */
    hayTransaccionesPendientes_Download=true;
   // Serial.println(Json);
    if(Code==0x00||Code==0x01)
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
    Buffer_Cashless.Init_Buffer_Transfer_AFT(true);
    return Json;
}

/* Retorna Estado de  transferencia de descarga */
String Cashless_API::Get_Current_Status_Transfer_Download()
{
    String Json;
    serializeJson(Objeto_Transfer_Download, Json); /* Serializa Data */
    Objeto_Transfer_Download.clear(); /*  Limpia  Objeto Ack Transferencias carga */
    Buffer_Cashless.Clear_Buffer(Buffer_RX_Cashless); /* Inicializa Buffer Creditos*/
    
    Buffer_Cashless.Init_Buffer_Transfer_AFT(true);
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
    String Puerto="9595";
    String fwurl = "http://"+Ip_Server+":"+Puerto+"/api/Cashless/Ack";

    WiFiClient client;

    HTTPClient https;
    https.setTimeout(20000);

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
    }
}

/* Metodo para reporte de ACK'S  para */
void Cashless_API::Reporting_Pending_Transfers(unsigned long TimeOut)
{

    if(hayTransaccionesPendientes)
    {
        nuevaTransferencia(Get_Current_Status_Transfer());
        hayTransaccionesPendientes=false;
    }
    if(Transfer_Pending_Load)
    {
        enviarTransaccion(Get_Current_Pending_Ack_Load());
        Transfer_Pending_Load=false;
    }

    if(Transfer_Pending_Download)
    {
        enviarTransaccion(Get_Current_Pending_Ack_Download());
        Transfer_Pending_Download=false;
    }


    if(hayTransaccionesPendientes_Download)
    {
        nuevaTransferencia(Get_Current_Status_Transfer_Download());
        hayTransaccionesPendientes_Download=false;
    }

    /* Envia Transacciones Cashless pendientes de Recepcion */

    if (!transaccionesPendientes.empty())
    {
        Tiempo_Inicio_Intento = millis();

        if ((Tiempo_Inicio_Intento - tiempoUltimoReintento) >= intervaloReintento)
        {
            
            if(Reset_Timer_Tranfer_Pending)
            {
                intentarEnviarTransacciones();
            }
            Reset_Timer_Tranfer_Pending=true;
            tiempoUltimoReintento = Tiempo_Inicio_Intento;
        }
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
        Serial.println("Hola");
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

    contadores.Close_ID_Client();
    contadores.Close_ID_Client_Temp();
    Sesion_Cerrada_Color=true;
    Variables_globales.Set_Variable_Global(Flag_Sesion_RFID,false); 
    Variables_globales.Set_Variable_Global(Flag_Contadores_Sesion_OFF, true);
    Status_Barra(SESION_CERRADA);
    delay(10);
    Reset_Handle_LED();
    delay(50);

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

    

    if(Trans_Tipo==LOAD_TRANSACTION)
    {
        Actualiza_Cashless_Entradas();

        unsigned long Respuesta_Server = millis();
        int TIMEOUT_CONECT_SERVER = 3500; // Espera 3.5 seg para Encuestar contadores
        while (!Flag_Entradas_Cashless_OK && millis() - Respuesta_Server < TIMEOUT_CONECT_SERVER)
        {
#ifdef DEBUG_RFID
            Serial.println("Encuestando contadores.....");
#endif
            vTaskDelay(300);
            if(Flag_Entradas_Cashless_OK)
                break;
        }

        Flag_Entradas_Cashless_OK=false;

        // Serial.println(" Actualiza Cashless Entradas");
    }else if(Trans_Tipo==DOWNLOAD_TRANSACTION)
    {
        Actualiza_Cashless_Salidas();

        unsigned long Respuesta_Server = millis();
        int TIMEOUT_CONECT_SERVER = 3500; // 3500 Espera 3.5 seg para Encuestar contadores
        while (!Flag_Salidas_Cashless_OK && millis() - Respuesta_Server < TIMEOUT_CONECT_SERVER)
        {
#ifdef DEBUG_RFID
            Serial.println("Encuestando contadores.....");
#endif
            
            vTaskDelay(300);
            if(Flag_Salidas_Cashless_OK)
                break;
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
                    Code=true;
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
        if(Code==true)
           
        return Code;
    }
    return false;
}

void Cashless_API::Log(ESP32Time RTC,String Msg,String Data)
{
    String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

    
    String Ms=DataTime+"|"+Msg+": "+Data;
    Data.replace("\n","");
    Msg.replace("\n","");
    Ms.replace("\n","");
    //Ms.replace(" ", "");
    Ms.replace("\n", "");
    Ms.replace("\t", "");
    Erro_Log(Ms,archivo);
}

/* Retorna el tipo de sesion a  cerrar
retorna (21) = PLAYER_TRACKING_SESION
retorna (22) = PLAYER_CASHLESS_SESION
*/
int  Cashless_API::Type_Sesion(int Flag,bool Status)
{

    /* WRITE */
    if(Flag==PLAYER_CASHLESS_SESION)
    {
        Variables_globales.Set_Variable_Global(Flag_Sesion_Cashless,Status);
    }

    /* READ */
    
    if(!Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless) && Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless))
        return PLAYER_TRACKING_SESION;
    else if(Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless))
        return PLAYER_CASHLESS_SESION;
    else{
        return PLAYER_TRACKING_SESION;
    }
}

/* Retorna 1 Maquina no comunica 
Retorna 2 Ya se intento recuperar la sesion 
Retorna 3 Error de recuperacion */
int Cashless_API::Recovery_Player_Sesion(byte Id_Client_Recovery[], int Type_Sesion, bool Handle_Cashless)
{
    
    if(Variables_globales.Get_Variable_Global(Comunicacion_Maq)) /* Comunicacion OK*/
    {
        if(contadores.Verify_Tarjeta(Id_Client_Recovery) && Handle_Cashless) /* Existe ID */
        {
            bool Test=false;
            switch (Type_Sesion)
            {
            case PLAYER_CASHLESS_SESION:
                
                Info_Cashless.Lock_Reader();
                // Status_Barra(301);
                // Serial.println("Se identifico Sesion Cashless");
                // Serial.println("Recuperando sesion......");

                if (contadores.Set_Client_ID(Id_Client_Recovery) && contadores.Set_Client_ID_Transaccion(Id_Client_Recovery))
                {
                    
                    Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, true);
                    Status_Barra(CARGA_CASHLESS_EXITOSA);
                    Variables_globales.Set_Variable_Global(Flag_Sesion_RFID, true);
                    // Serial.println("Cashless Iniciado....");
                    

                    /*Verifica si existe carga cashless pendiente por consultar */
                    NVS.begin("Config_ESP32", false);
                    bool Test_Pen_Data = NVS.getBool("Cash_Pending", false);
                    NVS.end();
                    if (Test_Pen_Data)
                    {
                        Test=true;
                        Status_Barra(301); /* Lector ocupado por transaccion */
                        Serial.println("Tarea de trasacción pendiente habilitada");
                        Variables_globales.Set_Variable_Global(Event_Load_Cashless_Pending, true);
                        Info_Cashless.Lock_Reader();
                    }else{
                        Serial.println("No tiene transacciones pendientes por consultar");
                    }
                }
                if(!Test)
                    Info_Cashless.Unlock_Reader();
                break;

            case PLAYER_TRACKING_SESION:

                // Serial.println("Se identifico un ID despues del Reinicio");
                // Serial.println("Se identifico Sesion Player Tracking...");
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
                break;
            }

            return 2;
        }else
        {
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
    if (Enable && Variables_globales.Get_Variable_Global(Enable_Cashless) && Evento == 0x6A && Info_Cashless.Type_Sesion() == PLAYER_CASHLESS_SESION)
    {
        Info_Cashless.Lock_Reader();

        switch (Info_Cashless.Info_Client_Download(contadores.Get_Client_ID_Transaccion(), RTC, "D", Cashless.Get_Trans_ID_Int()))
        {
        case REQUEST_SUCCESSFULLY_RECEIVED:
            Solicitud_Descarga_Cashless();
            return true;
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

    if(Handle_Lector_RFID)
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
