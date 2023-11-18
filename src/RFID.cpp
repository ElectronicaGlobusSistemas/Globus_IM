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

//#define DEBUG_RFID
extern unsigned long New_Timer_Final;
extern unsigned long New_Timmer_Inicial;
/* ------------------------------> Variables externas <-------------------------------------------*/
 extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales
 extern Buffers Buffer;            // Objeto de buffer de mensajes servidor

extern Contadores_SAS contadores; // Objeto contiene contadores maquina
 extern Configuracion_ESP32 Configuracion;
extern ESP32Time RTC;

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
        if (!Variables_globales.Get_Variable_Global(Conexion_RFID) || !Variables_globales.Get_Variable_Global(Comunicacion_Maq) || Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
        {
            if (!Handle_LED)
            {
                /*---------------------------------------> Sesion Iniciada <-------------------------------------------------------*/
                if (Variables_globales.Get_Variable_Global(Flag_Sesion_RFID) || !Variables_globales.Get_Variable_Global(Conexion_RFID) && Variables_globales.Get_Variable_Global(Comunicacion_Maq))
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
                /*-----------------------------------------------------------------------------------------------------------------*/

                /*-------------------------------------> NO HAY COMUNICACION <-----------------------------------------------------*/
                if (!Variables_globales.Get_Variable_Global(Comunicacion_Maq) && Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9)
                {

                    TimeOut_Automatico_Inicial = millis();

                    if ((TimeOut_Automatico_Inicial - TimeOut_Automatico) >= Close_Automatico && Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
                    {
                        Close_Sesion_Player_Tracking();
                        TimeOut_Automatico = TimeOut_Automatico_Inicial;
                    }
                    if (!Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
                    {
                        Status_Barra(NO_HAY_COMUNICACION);
                    }
                }
                /*-----------------------------------------------------------------------------------------------------------------*/
                if(!Variables_globales.Get_Variable_Global(Conexion_RFID))
                {
                    Barra_Status_Sesion_Client.clear();
                    Barra_Status_Sesion_Client.setPixelColor(0, Barra_Status_Sesion_Client.Color(255, 0, 255)); // gris
                    Barra_Status_Sesion_Client.setPixelColor(1, Barra_Status_Sesion_Client.Color(255, 0, 255)); // gris
                    Barra_Status_Sesion_Client.setPixelColor(2, Barra_Status_Sesion_Client.Color(255, 0, 255)); // gris
                    Barra_Status_Sesion_Client.setPixelColor(3, Barra_Status_Sesion_Client.Color(255, 0, 255)); // gris
                    Barra_Status_Sesion_Client.show();
                }

            }
        }
        /*-----------------------------------------------------------------------------------------------------------------*/
        if (Variables_globales.Get_Variable_Global(Conexion_RFID) && Variables_globales.Get_Variable_Global(Comunicacion_Maq) && !Variables_globales.Get_Variable_Global(Handle_RFID_Lector))
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
                Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
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
                Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                RESET_Handle();
                return;
            }
            Variables_globales.Set_Variable_Global(Handle_RFID_Lector, true);

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
                Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                Status_Barra(ERROR_LECTURA);
                return;
            }
            else if (!contadores.Verify_Client_ID(buffer2))
            {
                Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                Status_Barra(ERROR_LECTURA);
                return;
            }
            else if (!contadores.Verity_ID_NOT_NULL(buffer1, 'C'))
            {
                Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
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
        int TIMEOUT_CONECT_SERVER = 3500;
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
                    /*  Cierra Sesion de juego por cliente */
                    delay(10);
                    contador = 0;
                    Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
                }else
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
                int TIMEOUT_CONECT_SERVER = 3500;
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
                    }else{
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
                int TIMEOUT_CONECT_SERVER = 3500;
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
   int  LEDD2=false;
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

         for(int i=0; i<4;i++)
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
                    if(Host_<8)
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


        default:
        break;
        }
    }
}