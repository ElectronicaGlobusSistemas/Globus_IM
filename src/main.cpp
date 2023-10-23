#include <Arduino.h>
#include "Errores.h"
#include "Bootloader.h"

/* Definir las clases que haran uso de los metodos */
#include "Preferences.h"
Preferences NVS;

#include "Configuracion.h"
Configuracion_ESP32 Configuracion;

#include "ESP32Time.h"
#include "time.h"
ESP32Time RTC; // Objeto contiene hora y fecha

#include "Buffers.h"
Buffers Buffer;            // Objeto de buffer de mensajes servidor
Contadores_SAS contadores; // Objeto contiene contadores maquina

#include "Buffer_Cashless.h"
Buffer_RX_AFT Buffer_Cashless;
//#include "Clase_Variables_Globales.h"
Variables_Globales Variables_globales; // Objeto contiene Variables Globales

Eventos_SAS eventos; // Objeto contiene eventos maquina
#include "Tabla_Eventos.h"
Tabla_Eventos Tabla_Evento;
/* Definir los metodos que haran uso de las clases */

#include "RS232.h"
#include "Comunicaciones.h"

#include <stdio.h>
#include <string.h>
#include "Config_Perifericos.h"

/*--------------------------------------->Debug Comunicación Maquina <------------------------------*/
//#define Debug_Comunicacion_MQ
//#define Debug_Memoria_SD

//#define Debug_FTP
//#define Debug_Status_SD
//#define Debug_Escritura
//#define Info_SD
/*--------------------------------------------------------------------------------------------------*/
unsigned long Timeout_Close_Operador=0;
unsigned long Timeout_Close_Final=0;

/* Reset Timer Close Operador */
int Condicion_Cumpl=false;
int Timeout_OK_Data=40000;


unsigned long Contador_RESET_HANDPAY=0;
unsigned long Contador_RESET_HANDPAY_FINAL=0;

unsigned long tiempo_inicial, tiempo_final = 0;
int bandera2 = 0;

/*Variables Para Task Comunicación Maquina*/
unsigned long Bandera_RS232=0;
unsigned long Bandera_RS232_F=0;

unsigned long Msg_RS232=0;
unsigned long Msg_RS232_F=0;
int Timeout_Msg=8000;

unsigned long Msg_RS232_=0;
unsigned long Msg_RS232_F_=0;
int Timeout_Msg_=8000;

unsigned long Timeout_RS232=20000; //9000 //15000

unsigned long Excepcion=0;
bool Fallo_Comunicacion=false;
static void Check_Comunicacion_Maq(void *parameter);
extern bool Enable_Status;
extern int Intento_Connect_SD; // Variable Contadora de Intentos de Conexión SD.
extern int Contador_Escrituras;

unsigned long Timer_SD_CHECK=0;
unsigned long Timer_SD_Previous=0;
unsigned long SD_CHECK_Timer=10000;
bool Handle_SD=false;
bool extern Datos_OK;
bool Carga_Datos_Iniciales=false;
extern bool Counter_Final;
int Conta_Ejecuta=0;
extern bool Ultimo_Counter_;
bool Ejecuta_Instruccions_=false;
unsigned long Time_I=0;
unsigned long Time_P=0;
int LongT=8000; //8000
extern bool VERIFY_WIRE_CONNECTION;

extern unsigned long New_Timer_Final;
extern unsigned long New_Timmer_Inicial;


unsigned long currentTime;
int Inactividad_R=90000;
bool condicionCumplida = false;
unsigned long startTime = 0;

unsigned long Sample_Time=0;
unsigned long Sample_Time_=0;  
int Time_Ejecutions=2000;


extern bool Activa_ALERT;
extern int Compuesta;
int Valida =0;
void check_SD(void);
void TimeOut_Player_Tracking_Sesion(void);
void TimeOut_Marca_Operador(void);
static void Task_RFID(void *parameter);
bool Comunica_=false;
extern int Host_;
extern unsigned long TimeOut_Automatico;
extern unsigned long TimeOut_Automatico_Inicial;

int Total_SD;
int Usado_SD;        
int Libre_SD;

unsigned long TimeOut_Verify_RFID=0;
unsigned long TimeOut_Verify_RFID_Final=0;
int Time_Stop_Verify=40000;

unsigned long TimeOut_Conect_RFID=0;
unsigned long TimeOut_Conect_Final=0;
int Time_Stop_Conect=10000;
int Intentos_Conect_RFID=0;
void setup()
{
  Variables_globales.Init_Variables_Globales();
  Tabla_Evento.Init_Tabla_Eventos();
  Init_Config(); // Config Perifericos

  /*--------------------> Verifica la Comunicación Maquina<------------------------- */
  if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9)
  {
    xTaskCreatePinnedToCore(
        Check_Comunicacion_Maq,
        "verificaComunica",
        8000,
        NULL,
        configMAX_PRIORITIES - 15,
        NULL,
        1); // Core donde se ejecutara la tarea
  }
  /*----------------------------------------------------------------------------------*/
}
unsigned long INT1=0;
int Muestreo=500;
void loop()
{
  Time_I=millis();
  TimeOut_Conect_RFID=millis();


  TimeOut_Marca_Operador();
  /*------------------------> Verifica Inactividad de Cliente <---------------------------*/
  TimeOut_Player_Tracking_Sesion();
  /*--------------------------------------------------------------------------------------*/

  /*---------------------> Lectura Tarjetas  <--------------------------------------------*/
  Lee_Tarjeta();
  /*------------------------> Despierta lector de inactividad <---------------------------*/
  if (Time_I - Time_P >= LongT)
  {
    if (Variables_globales.Get_Variable_Global(Conexion_RFID))
    {
      RESET_Handle(); /* Reset Handle*/
    }
      Time_P = millis();
  }
  /*--------------------------------------------------------------------------------------*/
  
  /*---------------------> Ejecuta Servidor FTP & Funciones de Memoria <------------------*/
  check_SD();
  /*--------------------------------------------------------------------------------------*/

  /*---------------------> Reset Handpay Maquinas No SAS <--------------------------------*/

  if (Variables_globales.Get_Variable_Global(Type_Hanpay_Reset))
  {
      if (millis() - Sample_Time_ >= Time_Ejecutions)
      {
      RESET_HANDPAY_NOT_SAS();
      Sample_Time_ = millis();
      }
  }

  /*--------------------------------------------------------------------------------------*/
  if (!Variables_globales.Get_Variable_Global(Verify_Modulo_RFID))
  {
      if ((TimeOut_Conect_RFID - TimeOut_Conect_Final) >= Time_Stop_Conect && !Variables_globales.Get_Variable_Global(Conexion_RFID) && Intentos_Conect_RFID < 2)
      {

      Check_RFID();
      TimeOut_Conect_Final = TimeOut_Conect_RFID;
      }
      if(Intentos_Conect_RFID>4)
      {
        Variables_globales.Set_Variable_Global(Verify_Modulo_RFID,true);
        TimeOut_Conect_Final = TimeOut_Conect_RFID;
      }
  }
}

static void Check_Comunicacion_Maq(void *parameter)
{
    for (;;)
    {
      Bandera_RS232 = millis();
      Msg_RS232 = millis();

      if (Bandera_RS232 - Bandera_RS232_F >= Timeout_RS232)
      {
        Variables_globales.Set_Variable_Global(Comunicacion_Maq, false);
        if(Comunica_==false)
        {
          condicionCumplida=false;
          Contador_Transmision_Contadores = 0;
          Contador_Maquina_En_Juego = 0;
          Fallo_Comunicacion = true; /*SI FALLO LA CONMUNICACIÓN*/
          Datos_OK = false;
          Conta_Ejecuta = 0;
          Counter_Final = false;
          Ultimo_Counter_ = false;
          Ejecuta_Instruccions_ = false;
          Comunica_=true;
          TimeOut_Automatico=TimeOut_Automatico_Inicial;
        }
        if (Msg_RS232 - Msg_RS232_F >= Timeout_Msg)
        {
          #ifdef Debug_Comunicacion_MQ
                    Serial.println("No  Hay Comunicación con la maquina");
          #endif
          condicionCumplida=false;
          Contador_Transmision_Contadores = 0;
          Contador_Maquina_En_Juego = 0;
          Fallo_Comunicacion = true; /*SI FALLO LA CONMUNICACIÓN*/
          Datos_OK = false;
          Conta_Ejecuta = 0;
          Counter_Final = false;
          Ultimo_Counter_ = false;
          Ejecuta_Instruccions_ = false;
          Msg_RS232_F_ = millis();
          TimeOut_Automatico=TimeOut_Automatico_Inicial;
          Msg_RS232_F = Msg_RS232;
          
        }
      }
      else
      {
        /*-----------------------------> Falla la comunicación <---------------------------*/
        if (Fallo_Comunicacion == true)
        {
          if (Ejecuta_Instruccions_ == false)
          {
            Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 1);
            Ejecuta_Instruccions_ = true;
            Encuesta_Creditos_Premio();
            delay(300);
          }

          else if (Ejecuta_Instruccions_ == true)
          {
            if (Counter_Final)
            {
              Fallo_Comunicacion = false;
             // Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
              // Conta_Ejecuta=0;
              Counter_Final = false;
              Ultimo_Counter_ = false;
              Ultimo_Counter_ = false;
              Ejecuta_Instruccions_ = false;
              Carga_Datos_Iniciales = true;
              #ifdef Debug_Comunicacion_MQ
              Serial.println("Comunicación con la maquina OK");
              #endif
              Host_=0;
              /*COMUNICA OK*/
              Comunica_=false;
            }
          }
        }
        /*----------------------------------------------------------------------------*/

        /*----------------------------> Primera conexión <----------------------------*/
        else if (Datos_OK == true) /*Primera Conexión*/
        {
          if (Datos_OK == true && Carga_Datos_Iniciales == false)
          {
            // Conta_Ejecuta++;
            if (!Ejecuta_Instruccions_)
            {
              Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 1);

              Encuesta_Creditos_Premio();
              delay(300);
              Ejecuta_Instruccions_ = true;
            }

            if (Ejecuta_Instruccions_)
            {
              if (Counter_Final == true) /*Ultimo_Counter_*/
              {
               // Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
                Counter_Final = false;
                Carga_Datos_Iniciales = true;
                Conta_Ejecuta = 0;
                Ultimo_Counter_ = false;
                Ejecuta_Instruccions_ = false;
                #ifdef Debug_Comunicacion_MQ
                Serial.println("Comunicación con la maquina OK");
                #endif
                Host_=0;
                Comunica_=false;
                TimeOut_Automatico=TimeOut_Automatico_Inicial;
                Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
                Msg_RS232_F = Msg_RS232;
              }
            }
          }

          if (millis() - Msg_RS232_F_ >= Timeout_Msg_ && Datos_OK == true)
          {
            #ifdef Debug_Comunicacion_MQ
                        Serial.println("Comunicación con la maquina OK");
            #endif
            Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
            Msg_RS232_F_ = millis();
            Host_=0;
            Compuesta=0;
            TimeOut_Automatico=TimeOut_Automatico_Inicial;
            Msg_RS232_F = Msg_RS232;
            
          }
        }
        /*--------------------------------------------------------------------------------*/
      }
      vTaskDelay(250);
    }
    vTaskDelay(10);
}

/* Funcion para cerrar Sesion Player Tracking Por inactividad*/
void TimeOut_Player_Tracking_Sesion(void)
{
    currentTime = millis();

    if (Variables_globales.Get_Variable_Global(Flag_Maquina_En_Juego) == false && Variables_globales.Get_Variable_Global(Flag_Sesion_RFID) == true)
    {
      int Creditos_Actuales_Maquina = Convert_Char_To_Int10(contadores.Get_Contadores_Char(24));
      // Serial.println(Creditos_Actuales_Maquina);
      if (!condicionCumplida)
      {
        startTime = currentTime;
        condicionCumplida = true;
      }
      if ((currentTime - startTime) >= Inactividad_R && Creditos_Actuales_Maquina > 10)
      {
        condicionCumplida=false;
      }
      if ((currentTime - startTime) >= Inactividad_R && Creditos_Actuales_Maquina < 10)
      {

        Close_Sesion_Player_Tracking();
        Contador_Transmision_Contadores = 0;
        New_Timer_Final = New_Timmer_Inicial;
        startTime = currentTime;
        condicionCumplida = false;
      }
    }
    else
    {

      condicionCumplida = false;
    }
}


void TimeOut_Marca_Operador(void)
{
    Timeout_Close_Operador = millis();

    if (Variables_globales.Get_Variable_Global(MARCA_OPERADOR_VALIDO) && contadores.Verify_ID_Op())
    {
      if (!Condicion_Cumpl)
      {
        Timeout_Close_Final = Timeout_Close_Operador; /* Reset Timeout marca premio*/
        Condicion_Cumpl = true;
      }
      /* DEBUG  ID OPERADOR */
      // Serial.println(contadores.Verify_ID_Op());

      if ((Timeout_Close_Operador - Timeout_Close_Final) >= Timeout_OK_Data)
      {
        /*---------------Bloquea lector ------------------------------- */
        Variables_globales.Set_Variable_Global(Handle_RFID_Lector, true);
        /*--------------------------------------------------------------*/
        if (contadores.Close_ID_Operador())
        {
          Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO, false);
        }
        Timeout_Close_Final = Timeout_Close_Operador; /* Reset Timeout marca premio*/
        /*-----------------------> Habilita Lector<--------------------- */
        Variables_globales.Set_Variable_Global(Handle_RFID_Lector, false);
        /*---------------------------------------------------------------*/
      }
    }
    else if (Variables_globales.Get_Variable_Global(MARCA_OPERADOR_VALIDO) && !contadores.Verify_ID_Op())
    {
      Condicion_Cumpl = false;
      Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO, false);
    }
    else
    {
      Condicion_Cumpl = false;
    }
}


void check_SD(void)
{
  Timer_SD_CHECK = millis();

  if (Variables_globales.Get_Variable_Global(Ftp_Mode))
  {
    Rum_FTP_Server();
    Timer_SD_Previous = millis();
  }
  else
  {
    if ((Timer_SD_CHECK - Timer_SD_Previous) >= SD_CHECK_Timer)
    {

      FreeSpace_SD();
      uint8_t Temperatura_Procesador_GPU = temperatureRead();
      Variables_globales.Set_Variable_Global_String(Temperatura_procesador, String(Temperatura_Procesador_GPU));

      if (Variables_globales.Get_Variable_Global(SD_INSERT))
      {
        Variables_globales.Set_Variable_Global(Enable_SD, true);
        digitalWrite(SD_Status, HIGH); // Enciende Indicador LED SD Status.
        Enable_Status = true;          // Habilita  El Parpadeo de LED SD Status en Modo FTP Server
        Variables_globales.Set_Variable_Global(SD_INSERT, true);
        if (Contador_Escrituras > 0 && Variables_globales.Get_Variable_Global(Enable_Storage) == true)
        {
          Variables_globales.Set_Variable_Global(Estado_Escritura, true);
        }
        else
        {
          Variables_globales.Set_Variable_Global(Estado_Escritura, false);
        }
      }
      /*-----------------------> Agregar  Variables_globales.Get_Variable_Global(Comunicacion_Maq) */
      if (Variables_globales.Get_Variable_Global(Sincronizacion_RTC) && Variables_globales.Get_Variable_Global(Flag_Crea_Archivos) && !Variables_globales.Get_Variable_Global(Ftp_Mode) && Variables_globales.Get_Variable_Global(SD_INSERT))
      {
        /*-----------------------> Crea Archivos fecha actual<----------------------------------------*/
        delay(100);
        Create_ARCHIVE_Excel(Archivo_CSV_Contadores, Variables_globales.Get_Encabezado_Maquina(Encabezado_Maquina_Generica));
        delay(100);
        Create_ARCHIVE_Excel_Eventos(Archivo_CSV_Eventos, Variables_globales.Get_Encabezado_Maquina(Encabezado_Maquina_Eventos));
        delay(100);
        Create_ARCHIVE_Txt(Archivo_LOG);
        delay(100);
        Create_ARCHIVE_Excel(Archivo_CSV_Sesiones,Variables_globales.Get_Encabezado_Maquina(Encabezado_Archivo_Sesiones));
        delay(100);
        Create_ARCHIVE_Excel(Archivo_CSV_Premios,Variables_globales.Get_Encabezado_Maquina(Encabezado_Archivo_Premios));
        Serial.println("OK Archivos Listos..");
        Variables_globales.Set_Variable_Global(Flag_Archivos_OK, true);
        Total_SD = SD.totalBytes() / (1024 * 1024);
        Usado_SD = SD.usedBytes() / (1024 * 1024);         
        Libre_SD = Total_SD - Usado_SD;
        Variables_globales.Set_Variable_Global_String(Espacio_Libre_SD, String(Libre_SD)); // Guarda  espacio libre de memoria
        Variables_globales.Set_Variable_Global_String(Espacio_Usado_SD, String(Usado_SD));
        Variables_globales.Set_Variable_Global_String(Size_SD, String(Total_SD));
        Variables_globales.Set_Variable_Global(Flag_Crea_Archivos, false);
        /*--------------------------------------------------------------------------------------------*/
      }
      if (Variables_globales.Get_Variable_Global(SD_INSERT) && !Variables_globales.Get_Variable_Global(Ftp_Mode) && Variables_globales.Get_Variable_Global(Sincronizacion_RTC))
      {

        if (Libera_Memoria(Total_SD, Usado_SD))
        {
          Total_SD = SD.totalBytes() / (1024 * 1024);
          Usado_SD = SD.usedBytes() / (1024 * 1024);
          Libre_SD = Total_SD - Usado_SD;
          Variables_globales.Set_Variable_Global_String(Espacio_Libre_SD, String(Libre_SD)); // Guarda  espacio libre de memoria
          Variables_globales.Set_Variable_Global_String(Espacio_Usado_SD, String(Usado_SD));
          Variables_globales.Set_Variable_Global_String(Size_SD, String(Total_SD));
#ifdef Debug_Escritura
          Serial.println("Buscando archivos...");
          Serial.println("Alerta Memoria llena Borrando Datos...");
#endif
          /*Memoria llena*/
          Variables_globales.Set_Variable_Global(Flag_Memoria_SD_Full, true);
        }
        else
        {
          Variables_globales.Set_Variable_Global(Flag_Memoria_SD_Full, false);
        }
      }
      Timer_SD_Previous = millis();
    }
  }
}