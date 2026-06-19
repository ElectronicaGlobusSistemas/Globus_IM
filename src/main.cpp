#include <Arduino.h>
#include "Errores.h"
#include "Bootloader.h"
#include "TITO.h"
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
Transsaccion_Cashless Cashless;
//#include "Clase_Variables_Globales.h"
Variables_Globales Variables_globales; // Objeto contiene Variables Globales

TITO Tito;
#include "API_Gmaster.h"

API_Gmaster Api_G;
Eventos_SAS eventos; // Objeto contiene eventos maquina
#include "Tabla_Eventos.h"
Tabla_Eventos Tabla_Evento;
/* Definir los metodos que haran uso de las clases */

#include "RS232.h"
#include "Comunicaciones.h"

#include <stdio.h>
#include <string.h>
#include "Config_Perifericos.h"
#include "API_Accounting.h"
#include <MFRC522.h>
#include "Pantalla_TFT.h"

#include "Persistenca_Info.h"
#include "ScanWiFi.h"
#include "lwip/stats.h"


API_Accounting Accounting;
extern Persistenca_Info Backup;


extern Fidelizacion Fideliza;

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
bool Condicion_Cumpl=false;
int Timeout_OK_Data=60000;


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

unsigned long Timeout_RS232=30000; //9000 //15000

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
int LongT=4000; //8000
extern bool VERIFY_WIRE_CONNECTION;

extern unsigned long New_Timer_Final;
extern unsigned long New_Timmer_Inicial;
extern std::vector<String> transaccionesPendientes;

unsigned long currentTime;
//int Inactividad_R=90000;
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

int Inactividad_Usuario_Player_Tracking;
int Tiempo_Transmision_En_Juego;
int Tiempo_Transmision_No_Juego;
int Tiempo_Inactividad_Maquina;

unsigned long  Timer_Error_Wifi_Inicial=0;
unsigned long  Timer_Error_Wifi_Previous=0;


int Exec_Timer=30000;

int buttonPressCount = 0;  // Contador de pulsaciones
unsigned long lastButtonClickTime = 0;

//SPIClass spiRFID(VSPI);

const char* archivo = "/LogESP.txt";


const char* archivo_Fide = "/LogFide.txt";
const char* archivo_Cashless= "/LogCashless.txt";
const char* archivo_Acounting= "/LogAcounting.txt";


extern MFRC522 mfrc522;   // Create MFRC522 instance.

unsigned long Timeout_inicial=0;
unsigned long Timeout_final=0;
int Timeout_Transfer=50000;
bool condicion_transfer =false;

extern DynamicJsonDocument Objeto_Transfer_Download;
uint32_t Dia_Guarda_Logs=15;
TaskHandle_t Check_Comunication_Maq;


extern SemaphoreHandle_t sd_mutex;


bool Formateo=false;
int Result_Formatt=0;


extern volatile bool Flag_Recupera;



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
        6000, /*8000*/
        NULL,
        configMAX_PRIORITIES - 15,
        &Check_Comunication_Maq,
        1); // Core donde se ejecutara la tarea
  }

  if(Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
    DisplayTFT.Init_Handle();

  if (Variables_globales.Get_Variable_Global(Enable_Cashless) || Variables_globales.Get_Variable_Global(Handle_Premios_SAS) || Variables_globales.Get_Variable_Global(Enable_Tito_Ticket))
  {

    if (Info_Cashless.Inicialize_File_System())
    {
      Serial.println("📂 Sistema de archivos iniciado ✅");
      /* --------------------------> Transacciones Cashless <---------------------------*/
      if (Variables_globales.Get_Variable_Global(Enable_Cashless))
        Info_Cashless.Load_Pending_Transactions(); /* Carga en RAM transacciones Cashless pendientes */
      /*--------------------------------------------------------------------------------*/

      /* ------------------------------> Transacciones Premios SAS <---------------------*/
      if (Variables_globales.Get_Variable_Global(Handle_Premios_SAS))
        Accounting.Load_Premios_SAS(); /* Carga en RAM Premios SAS pendientes */
      /*--------------------------------------------------------------------------------*/

      /* -----------------------------> Transacciones Tito <----------------------------*/
      if (Variables_globales.Get_Variable_Global(Enable_Tito_Ticket))
      {
        Tito.Load_Pending_Ticket_Transactions();
        Tito.Set_Confirma_Ticket(false);
      }

      /*------------------------> Carga Sesiones Sin tarjetas <-------------------------*/
      Info_Cashless.Load_Sesiones_Unknown_Pendientes();
      /*--------------------------------------------------------------------------------*/
    }
    else
    {
      Serial.println("📂 No se inicio el sistema de archivos ❌");
    }
  }
  else
  {

    if (Info_Cashless.Inicialize_File_System())
    {
      Serial.println("📂 Sistema de archivos iniciado ✅");
      Info_Cashless.Load_Sesiones_Unknown_Pendientes();
    }
      
    else
    {
      Serial.println("📂 No se inicio el sistema de archivos ❌");
    }
      
  }

  if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) < 4 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
  {
    Timeout_RS232 = 50000;
  }

  // /*----------------------------------------------------------------------------------*/
  // if(Info_Cashless.Inicialize_File_System())
  // {
  //   Serial.println("Sistema de archivos iniciado");
  //   Info_Cashless.Load_Pending_Transactions();

  //   /* -------------> Premios SAS <---------------------------------------------------*/
  //   if(Variables_globales.Get_Variable_Global(Handle_Premios_SAS))
  //     Accounting.Load_Premios_SAS();
  //   /*--------------------------------------------------------------------------------*/
  // }
  // else
  //   Serial.println("No se inicio el sistema de archivos");

  Cashless.Init_API_Server();                       /* Inicializa Server Globus IM ESP32 */
  Buffer_Cashless.Init_Buffer_Transfer_AFT(true);   /* Inicializa Buffer de transferencias AFT*/
  Buffer_Cashless.Clear_Buffer(Buffer_RX_Cashless); /* Inicializa Buffer Creditos*/
  Cashless.Set_Amount_To_Load(0, 0, 0);             /* Setea Valores de carga en 0 */
  Info_Cashless.Init_Timer_Lector();
  // Info_Cashless.Log(RTC,"INICIO_OPERACION_DISPOSITIVO_GLOBUS_IM_ESP32");
  sd_mutex = xSemaphoreCreateMutex();

  if (AFT.IsCashlessMachine())
  {
    if (AFT.checkLogSAS())
      Variables_globales.Set_Variable_Global(Flag_Log_SAS, true);
    else
      Variables_globales.Set_Variable_Global(Flag_Log_SAS, false);
  }else
    Variables_globales.Set_Variable_Global(Flag_Log_SAS, false);

  // ScanWiFi();
  Info_Cashless.Log(RTC, "DISPOSITIVO_INICIADO");


  
  //Api_G.Inicializa_Cola_Tramas(true);

  //Tito.Request_Transfer_Tito_Out();
  // ConsultarBanners();

  // SPIFFS.remove("/fifo.txt");
  // SPIFFS.remove("/fifo.tmp");

//   const char* testLine = 
// "{\"Cashless_ID\":0,\"IsSuccess\":false,\"Cliente_ID\":13152,\"Trans_Tipo\":\"D\","
// "\"Saldo_Canjeable\":0,\"Saldo_Restringido\":0,\"Saldo_No_Restringido\":0,"
// "\"Trans_ID\":0,\"Fecha_Hora\":\"2026-1-22 15:17:56\",\"Message\":\"null\","
// "\"Cliente_Nombre\":\"Jose manuel\",\"Cashless_Estado\":\"0\",\"Ip\":\"192.168.5.116\","
// "\"Key\":\"10001000841fe8998681804121126\",\"MAC\":\"84:1F:E8:09:98:68\","
// "\"Trans_Estado\":129,\"Tipo_Maq\":\"AFT\"}";

//   File file = SPIFFS.open("/transacciones.txt", FILE_APPEND);
//   for (int i = 0; i < 11; i++)
//   {
//     file.println(testLine);
//   }
//   file.close();

}


unsigned long INT1=0;
int Muestreo=500;
bool Verifica=false;
bool test=false;

// #include <Arduino.h>

// extern "C" {
// #include <fcntl.h>
// #include <errno.h>
// }

// int countOpenSockets() {
//     int count = 0;

//     // En ESP32 normalmente FD de 0 a 63
//     for (int fd = 0; fd < 64; fd++) {
//         errno = 0;
//         int flags = fcntl(fd, F_GETFL, 0);

//         if (flags != -1 || errno != EBADF) {
//             count++;
//         }
//     }

//     return count;
// }

// void printOpenSockets() {
//     int sockets = countOpenSockets();
//     Serial.printf("Sockets abiertos actualmente: %d\n", sockets);
// }

// void listOpenSockets() {
//     Serial.println("----- FDs abiertos -----");
//     for (int fd = 0; fd < 64; fd++) {
//         errno = 0;
//         int flags = fcntl(fd, F_GETFL, 0);

//         if (flags != -1 || errno != EBADF) {
//             Serial.printf("FD %02d abierto\n", fd);
//         }
//     }
//     Serial.println("------------------------");
// }


void loop()
{

  if(Handle_Bootloader())
    return;
  

  // Tito.Request_Handle_Tito();
  if (!Verifica)
  {
    if (Info_Cashless.Recovery_Player_Sesion(contadores.Get_Client_Recovery(), contadores.Get_Type_Sesion(), Variables_globales.Get_Variable_Global(Enable_Cashless)) == 2)
      Verifica = true;
  }

  if (Variables_globales.Get_Variable_Global(Enable_Cashless) || Variables_globales.Get_Variable_Global(Handle_Premios_SAS))
    Info_Cashless.Genera_Token_Cashless();

  // Cashless.Registra_Maquina_Auto();

  if (Variables_globales.Get_Variable_Global(Enable_Cashless))
    Info_Cashless.Reporting_Pending_Transfers(25000);

  // if(Variables_globales.Get_Variable_Global(Enable_Tito_Ticket))
  //   Tito.Available_Ticket_Transfer();

  eventos.TimeOut_Capture_Event();
  Time_I = millis();
  TimeOut_Conect_RFID = millis();
  Timer_Error_Wifi_Inicial = millis();
  // Mensajes_RFID();
  TimeOut_Marca_Operador();
  /*------------------------> Verifica Inactividad de Cliente <---------------------------*/
  TimeOut_Player_Tracking_Sesion();
  /*--------------------------------------------------------------------------------------*/
  /*102*/
  /*---------------------> Lectura Tarjetas  <--------------------------------------------*/

  /*------------------------> Despierta lector de inactividad <---------------------------*/
  if (Time_I - Time_P >= LongT)
  {

    // printOpenSockets();

    if (Variables_globales.Get_Variable_Global(Informacion_Rendimiento))
    {
      Serial.printf(
          "[SYS] Heap:%u Min:%u MaxBlk:%u | Tasks:%u | CPU:%uMHz | Up:%lus\n",
          ESP.getFreeHeap(),
          ESP.getMinFreeHeap(),
          ESP.getMaxAllocHeap(),
          uxTaskGetNumberOfTasks(),
          ESP.getCpuFreqMHz(),
          millis() / 1000);
    }

    // listOpenSockets();
    if (Variables_globales.Get_Variable_Global(Conexion_RFID))
    {
      // mfrc522.PICC_IsNewCardPresent();
    }
    Time_P = millis();
  }

  Lee_Tarjeta();
  /*--------------------------------------------------------------------------------------*/

  /*---------------------> Ejecuta Servidor FTP & Funciones de Memoria <------------------*/

  if(!Variables_globales.Get_Variable_Global(Flag_Update_OTA))
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

  /*---------------------------> Conecta módulo RFID <------------------------------------*/

  if (!Variables_globales.Get_Variable_Global(Verify_Modulo_RFID))
  {
    if ((TimeOut_Conect_RFID - TimeOut_Conect_Final) >= Time_Stop_Conect && !Variables_globales.Get_Variable_Global(Conexion_RFID) && Intentos_Conect_RFID < 2)
    {

      Check_RFID();
      TimeOut_Conect_Final = TimeOut_Conect_RFID;
    }
    if (Intentos_Conect_RFID > 4)
    {
      Variables_globales.Set_Variable_Global(Verify_Modulo_RFID, true);
      TimeOut_Conect_Final = TimeOut_Conect_RFID;
    }
  }

  /* -----------------------------> Premios SAS <---------------------------------------------------- */
  if (Variables_globales.Get_Variable_Global(Handle_Premios_SAS))
    Accounting.Report_Handpay_Informations_SAS(Variables_globales.Get_Variable_Global(Token_Cashless_Solicitud));
  /*--------------------------------------------------------------------------------------------------*/

  Resurrect_reader();
  // check_Status_Reader_Polling();
  //  if(Variables_globales.Get_Variable_Global(Flag_Sesion_RFID))
  //    Prueba_TFT();
  //  if(Variables_globales.Get_Variable_Global(Comunicacion_Maq)&&!test)
  //  {
  //    Backup.enviarInformacionMaquina(Backup.Test());
  //    test=true;
  //  }

  // Backup.Task_Info();
  FtpFast();

  if (Variables_globales.Get_Variable_Global(Sincronizacion_RTC) && Variables_globales.Get_Variable_Global(Flag_Sesiones_Acumuladas_Sin_Tarjeta) && !Variables_globales.Get_Variable_Global(Ftp_Mode) && !Variables_globales.Get_Variable_Global(Updating_System))
    Info_Cashless.Task_Sesiones_Unknow();

  // if(!Variables_globales.Get_Variable_Global(Ftp_Mode))
  //   Check_TFT_Reconnect();

  AFT.BackupBA();
  AFT.Task_Procesa_BA();

  if(!AFT.solicitudBA.Flag_Attend_Transaccion_BA && Variables_globales.Get_Variable_Global(Comunicacion_Maq))
  {
    AFT.solicitudBA.Flag_Attend_Transaccion_BA=true;

    if(AFT.solicitudBA.Ack_Pendiente_Recv)
      Variables_globales.Set_Variable_Global(Flag_BA_Controller,true);

  }
  DisplayTFT.Task_Handle_TFT_Display();

  Info_Cashless.Simulador_Cashless_Automatico(Variables_globales.Get_Variable_Global(Flag_Simulador_Cashless));

  
}

/* Verifica comunicacion maquina */
static void Check_Comunicacion_Maq(void *parameter)
{
  for (;;)
  {

    Bandera_RS232 = millis();
    Msg_RS232 = millis();

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 15) /* Mecanica 2 */
    {
      Timeout_RS232=180000; /* 3 minutos Para reportar A0 */
      if (Bandera_RS232 - Bandera_RS232_F >= Timeout_RS232)
      {
        Variables_globales.Set_Variable_Global(Comunicacion_Maq, false);
        if (Comunica_ == false)
        {
          condicionCumplida = false;
          //Contador_Transmision_Contadores = 0;
          Contador_Maquina_En_Juego = 0;
          Fallo_Comunicacion = true; /*SI FALLO LA CONMUNICACIÓN*/
          Datos_OK = false;
          Conta_Ejecuta = 0;
          Counter_Final = false;
          Ultimo_Counter_ = false;
          Ejecuta_Instruccions_ = false;
          Comunica_ = true;
          TimeOut_Automatico = TimeOut_Automatico_Inicial;
        }
        if (Msg_RS232 - Msg_RS232_F >= Timeout_Msg)
        {
#ifdef Debug_Comunicacion_MQ
          Serial.println("No  Hay Comunicación con la maquina");
#endif
          condicionCumplida = false;
          //Contador_Transmision_Contadores = 0;
          Contador_Maquina_En_Juego = 0;
          Fallo_Comunicacion = true; /*SI FALLO LA CONMUNICACIÓN*/
          Datos_OK = false;
          Conta_Ejecuta = 0;
          Counter_Final = false;
          Ultimo_Counter_ = false;
          Ejecuta_Instruccions_ = false;
          Msg_RS232_F_ = millis();
          TimeOut_Automatico = TimeOut_Automatico_Inicial;
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
            // Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 1);
            Ejecuta_Instruccions_ = true;
            // Encuesta_Creditos_Premio();
            // delay(300);
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
              Host_ = 0;
              /*COMUNICA OK*/
              Comunica_ = false;
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
              // Variables_globales.Set_Variable_Global_Int(Flag_Type_excepcion, 1);

              // // Encuesta_Creditos_Premio();
              // // delay(300);
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
                Host_ = 0;
                Comunica_ = false;
                TimeOut_Automatico = TimeOut_Automatico_Inicial;
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
            Host_ = 0;
            Compuesta = 0;
            TimeOut_Automatico = TimeOut_Automatico_Inicial;
            Msg_RS232_F = Msg_RS232;

           // Messag = false;
          }
        }
        /*--------------------------------------------------------------------------------*/
      }
    }
    else if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 15)
    {
      if (Bandera_RS232 - Bandera_RS232_F >= Timeout_RS232)
      {
        Variables_globales.Set_Variable_Global(Comunicacion_Maq, false);
        if (Comunica_ == false)
        {
          condicionCumplida = false;
          //Contador_Transmision_Contadores = 0;
          Contador_Maquina_En_Juego = 0;
          Fallo_Comunicacion = true; /*SI FALLO LA CONMUNICACIÓN*/
          Datos_OK = false;
          Conta_Ejecuta = 0;
          Counter_Final = false;
          Ultimo_Counter_ = false;
          Ejecuta_Instruccions_ = false;
          Comunica_ = true;
          TimeOut_Automatico = TimeOut_Automatico_Inicial;
        }
        if (Msg_RS232 - Msg_RS232_F >= Timeout_Msg)
        {
#ifdef Debug_Comunicacion_MQ
          Serial.println("No  Hay Comunicación con la maquina");
#endif
          condicionCumplida = false;
          //Contador_Transmision_Contadores = 0;
          Contador_Maquina_En_Juego = 0;
          Fallo_Comunicacion = true; /*SI FALLO LA CONMUNICACIÓN*/
          Datos_OK = false;
          Conta_Ejecuta = 0;
          Counter_Final = false;
          Ultimo_Counter_ = false;
          Ejecuta_Instruccions_ = false;
          Msg_RS232_F_ = millis();
          TimeOut_Automatico = TimeOut_Automatico_Inicial;
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
              Host_ = 0;
              /*COMUNICA OK*/
              Comunica_ = false;
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
                Host_ = 0;
                Comunica_ = false;
                TimeOut_Automatico = TimeOut_Automatico_Inicial;
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
            Host_ = 0;
            Compuesta = 0;
            TimeOut_Automatico = TimeOut_Automatico_Inicial;
            Msg_RS232_F = Msg_RS232;
          }
        }
        /*--------------------------------------------------------------------------------*/
      }
    }

    vTaskDelay(250);
  }
  vTaskDelay(10);
}

/* Verifica y cierra sesion de usuario RFID por inactividad despues del tiempo configurado en memoria
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
// void TimeOut_Player_Tracking_Sesion(void)
// {

//   currentTime = millis();

//   if (Variables_globales.Get_Variable_Global(Flag_Maquina_En_Juego) == false && Variables_globales.Get_Variable_Global(Flag_Sesion_RFID) == true)
//   {
//     Creditos_Machine();
//     delay(250);
//     int Creditos_Actuales_Maquina = Convert_Char_To_Int10(contadores.Get_Contadores_Char(24));
//     // Serial.println(Creditos_Actuales_Maquina);
//     if (!condicionCumplida)
//     {
//       startTime = currentTime;
//       condicionCumplida = true;
//     }
//     if ((currentTime - startTime) >= Inactividad_Usuario_Player_Tracking && Creditos_Actuales_Maquina > 10)
//     {
//       startTime = currentTime;
//       condicionCumplida = false;
//     }
//     if ((currentTime - startTime) >= Inactividad_Usuario_Player_Tracking && Creditos_Actuales_Maquina < 10)
//     {
//       /* Tipo de maquina no cashless */
//       if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) > 3)
//       {
//         Info_Cashless.Lock_Reader();
//         Transmite_Contadores_Accounting();
//         Close_Sesion_Player_Tracking();
//         Info_Cashless.Unlock_Reader();
//       }
//       else
//       {
//         /* Tipo de maquina Cashless */
//         if (Variables_globales.Get_Variable_Global(Enable_Cashless))
//         {
//           /* Cashless  habilitado */
//           if (Info_Cashless.Type_Sesion() == PLAYER_CASHLESS_SESION)
//           {
//             Info_Cashless.Lock_Reader();
//             bool Handle = true;
//             if (Info_Cashless.Get_Status_Reader())
//               Status_Barra(301);

//             switch (Info_Cashless.Info_Client_Download(contadores.Get_Client_ID_Transaccion(), RTC, "D", Cashless.Get_Trans_ID_Int()))
//             {
//             case REQUEST_SUCCESSFULLY_RECEIVED:
//               Solicitud_Descarga_Cashless();
//               Handle = false;
//               break;

//             default:
//               Status_Barra(ERROR_LECTURA);
//               Info_Cashless.Unlock_Reader();
//               Handle = false;
//               break;
//             }
//             if (Handle)
//               Info_Cashless.Unlock_Reader();
//           }
//         }
//         else
//         {
//           Info_Cashless.Lock_Reader();
//           Transmite_Contadores_Accounting();
//           Close_Sesion_Player_Tracking();
//           Info_Cashless.Unlock_Reader();
//         }
//       }
//       Contador_Transmision_Contadores = 0;
//       New_Timer_Final = New_Timmer_Inicial;
//       startTime = currentTime;
//       condicionCumplida = false;
//     }
//   }
//   else
//   {
//     condicionCumplida = false;
//   }
// }

void TimeOut_Player_Tracking_Sesion(void)
{
  currentTime = millis();
  static bool aviso20 = false;
  static bool aviso10 = false;
  static bool aviso5  = false;

  if (!Variables_globales.Get_Variable_Global(Flag_Maquina_En_Juego) && Variables_globales.Get_Variable_Global(Flag_Sesion_RFID) && Convert_Char_To_Int10(contadores.Get_Contadores_Char(24)) < 10)
  {

    if (!condicionCumplida)
    {
      startTime = currentTime;
      condicionCumplida = true;
      // Serial.println("Reinicia Tiempout por primera activacion ");
      // Reset avisos
      aviso20 = false;
      aviso10 = false;
      aviso5 = false;
    }

    

    if ((currentTime - startTime) >= Inactividad_Usuario_Player_Tracking)
    {
      Creditos_Machine();
      delay(250);
      // Verifica créditos varias veces para evitar inconsistencias
      bool consistentCreditos = true;
      int Creditos;

      for (int i = 0; i < 5; i++)
      {
        Creditos = Convert_Char_To_Int10(contadores.Get_Contadores_Char(24));

        if (Creditos >= 10)
        {
          consistentCreditos = false;
          break;
        }
      }

      if (consistentCreditos)
      {
        Serial.println();
        Serial.print("Creditos: ");
        Serial.println(Creditos = Convert_Char_To_Int10(contadores.Get_Contadores_Char(24)));
        Serial.println("Cierra sesion por inactividad ");
        Serial.print("Timeout: ");
        Serial.println(currentTime - startTime);

        /* Maquina  No Cashless */
        if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) > 3 && Configuracion.Get_Configuracion(Tipo_Maquina, 0)!=17)
        {
          Transmite_Contadores_Accounting();
          Close_Sesion_Player_Tracking();
          Report_Http_Code(TERMINA_SESION_CREDITOS, "Sesion terminada por creditos: "+String(Creditos), true);
          Info_Cashless.Log(RTC,"CIERRE_SESION_AUTOMATICO_FIDELIZACION_CREDITOS",String(Creditos));

          DisplayTFT.Notify_Now(EVENT_ACTUALIZAR_INFO_FINAL_SESION_FIDELIZACION);
          DisplayTFT.Mensaje_TFT(
              "Tiempo de inactividad superado\n Cerrando sesion...",
              true);

          if (Variables_globales.Get_Variable_Global(Conexion_TFT_Display) && Variables_globales.Get_Variable_Global(Status_Device_TFT_Display))
          {

            // Menssage_TFT("Cerrando sesion por inactividad de juego...",DisplayTFT.configtft.timeoutMensajesCONFIG,false);
            // DisplayTFT.info.Actualiza_Puntos_Finales = true;
          }
            
        }
        else
        {
          /* Maquina Cashless */
          if (Variables_globales.Get_Variable_Global(Enable_Cashless))
          {

            //if(!Variables_globales.Get_Variable_Global(Comunicacion_Maq))

            if (Info_Cashless.Type_Sesion() == PLAYER_CASHLESS_SESION && Variables_globales.Get_Variable_Global(Flag_Sesion_RFID) && transaccionesPendientes.empty() && AFT.GET_STATUS_TRANSFER() == TransaccionCashless::TRANS_IDLE && !Variables_globales.Get_Variable_Global(Status_Games_Machine) && contadores.Get_Client_ID_Transaccion_Int() > 0 && (Configuracion.Get_Configuracion(Tipo_Maquina, 0) <= 3) || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17 && !Variables_globales.Get_Variable_Global(Status_Games_Machine) && contadores.Get_Client_ID_Transaccion_Int() > 0 && AFT.GET_STATUS_TRANSFER() == TransaccionCashless::TRANS_IDLE && Variables_globales.Get_Variable_Global(Comunicacion_Maq))
            {

              //if(AFT.GET_STATUS_TRANSFER() == TransaccionCashless::TRANS_IDLE)
              //  AFT.STATUS_TRANSFER(TransaccionCashless::TRANS_RECIBIDA);
              DisplayTFT.info.Mensaje = " Timeout de inactividad alcanzado\nComprobando saldo\nPor favor espere...";
              DisplayTFT.info.Ocultar = false;
              DisplayTFT.Notify_Now(EVENT_MENSAJES);


              Info_Cashless.Log(RTC, "TRANSACCION_DESCARGA_AUTOMATICA","TIMEOUT_ALCANZADO_PROCESANDO_DESCARGA",archivo,WARN_);

              Info_Cashless.Lock_Reader();
              bool Handle = true;
              if (Info_Cashless.Get_Status_Reader())
                Status_Barra(301);
              int ClientID = contadores.Get_Client_ID_Transaccion_Int();
              switch (Info_Cashless.Info_Client_Download(contadores.Get_Client_ID_Transaccion(), RTC, "D", Cashless.Get_Trans_ID_Int(), ClientID))
              {
              case REQUEST_SUCCESSFULLY_RECEIVED:
                // Info_Cashless.Log(RTC,"ENVIA_TRANSACCION_CASHLESS_DESCARGA_AUTOMATICA_MAQUINA");
                Objeto_Transfer_Download["Operacion"] = "Descarga Automatica";
                Solicitud_Descarga_Cashless();
                Handle = false;
                break;

              case 401:
                // Info_Cashless.Log(RTC,"TOKEN_NO_AUTORIZADO");

                DisplayTFT.info.Mensaje = "Acceso no autorizado\nContacte al operador";
                DisplayTFT.info.Ocultar = true;
                DisplayTFT.Notify_Now(EVENT_MENSAJES);

                Status_Barra(ERROR_LECTURA);
                Info_Cashless.Unlock_Reader();
                Handle = false;
                Report_Http_Code(401, "Token de autenticacion no valido Codigo HTTP");
                break;

              default:
                // Info_Cashless.Log(RTC,"ERROR_SOLICITUD_DESCARGA_AUTOMATICA");

                DisplayTFT.info.Mensaje = "Error al conectar con el servidor\nPor favor intente nuevamente";
                DisplayTFT.info.Ocultar = true;
                DisplayTFT.Notify_Now(EVENT_MENSAJES);

                Status_Barra(ERROR_LECTURA);
                Info_Cashless.Unlock_Reader();
                Handle = false;

                Info_Cashless.Log(RTC, "TRANSACCION_DESCARGA_AUTOMATICA","ERROR_EN_SOLICITUD_DESCARGA",archivo,ERROR_);
                break;
              }
            }

            /* Cashless  habilitado */
            // if (Info_Cashless.Type_Sesion() == PLAYER_CASHLESS_SESION && Variables_globales.Get_Variable_Global(Flag_Sesion_RFID) && transaccionesPendientes.empty() && !Variables_globales.Get_Variable_Global(Event_Dowmload_Cashless_Pending) && !Variables_globales.Get_Variable_Global(Event_Load_Cashless_Pending) &&(Configuracion.Get_Configuracion(Tipo_Maquina, 0)<=3||Configuracion.Get_Configuracion(Tipo_Maquina, 0)==17) && !Variables_globales.Get_Variable_Global(Status_Games_Machine) && contadores.Get_Client_ID_Transaccion_Int()>0 && Info_Cashless.Get_Status_Transfer()==TRANSFER_IDLE && !Info_Cashless.Get_Status_Handpay_EFT())
            // {

            //   Info_Cashless.Lock_Reader();
            //   bool Handle = true;
            //   if (Info_Cashless.Get_Status_Reader())
            //     Status_Barra(301);
            //   int ClientID=contadores.Get_Client_ID_Transaccion_Int();
            //   switch (Info_Cashless.Info_Client_Download(contadores.Get_Client_ID_Transaccion(), RTC, "D", Cashless.Get_Trans_ID_Int(),ClientID))
            //   {
            //   case REQUEST_SUCCESSFULLY_RECEIVED:
            //     //Info_Cashless.Log(RTC,"ENVIA_TRANSACCION_CASHLESS_DESCARGA_AUTOMATICA_MAQUINA");
            //     Objeto_Transfer_Download["Operacion"]="Descarga Automatica";
            //     Solicitud_Descarga_Cashless();
            //     Handle = false;
            //     break;

            //   case 401:
            //     //Info_Cashless.Log(RTC,"TOKEN_NO_AUTORIZADO");
            //     Status_Barra(ERROR_LECTURA);
            //     Info_Cashless.Unlock_Reader();
            //     Handle = false;
            //     Report_Http_Code(401, "Token de autenticacion no valido Codigo HTTP");
            //     break;

            //   default:
            //     //Info_Cashless.Log(RTC,"ERROR_SOLICITUD_DESCARGA_AUTOMATICA");
            //     Status_Barra(ERROR_LECTURA);
            //     Info_Cashless.Unlock_Reader();
            //     Handle = false;
            //     break;
            //   }

            //   // if (Handle)
            //   // Info_Cashless.Unlock_Reader();
            // }
            else
            {

              if (!transaccionesPendientes.empty())
              {
                //Info_Cashless.Unlock_Reader();

                Report_Http_Code(TRANSFER_PENDING, "No es posible realizar  descarga automatica por creditos: " + String(Creditos) + " Transferencia pendiente por recepcion");
                Status_Barra(302);

                Info_Cashless.Log(RTC, "TRANSACCION_DESCARGA_AUTOMATICA", "TRANSACCION_CANCELADA_POR_TRANSFERENCIA_PENDIENTE_DE_RECEPCION:" + String(AFT.GET_STATUS_TRANSFER()));
                //return;
              }

              if (AFT.GET_STATUS_TRANSFER() != TransaccionCashless::TRANS_IDLE)
              {
                //Info_Cashless.Unlock_Reader();
                Report_Http_Code(TRANSFER_PENDING, "No es posible realizar  la descarga automatica transaccion en progreso estado:" + String(AFT.GET_STATUS_TRANSFER()));
                Info_Cashless.Log(RTC, "TRANSACCION_DESCARGA_AUTOMATICA", "CANCELADA_POR_TRANSACCION_EN_PROGRESO_ESTADO:" + String(AFT.GET_STATUS_TRANSFER()));

                //return;
              }

              if (Info_Cashless.Get_Status_Handpay_EFT())
              {
                Info_Cashless.Unlock_Reader();
                if (Info_Cashless.Get_Status_Handpay_EFT())
                {
                  Report_Http_Code(DESCARGA_EFT_BLOQUEADA, "Maquina en condicion de pago no puede realizar descarga EFT:");
                  Info_Cashless.Log(RTC,"TRANSACCION_DESCARGA_AUTOMATICA","NO_PUEDE_REALIZAR_DESCARGA_AUTOMATICA_MAQUINA_EFT_EN_CONDICION_DE_PAGO");
                }
                //return;
              }

              if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
              {
                Info_Cashless.Unlock_Reader();
                if(Variables_globales.Get_Variable_Global(Status_Games_Machine))
                {
                  Report_Http_Code(DESCARGA_EFT_BLOQUEADA, "Transaccion de descarga automatica cancelada por maquina en juego");
                  Info_Cashless.Log(RTC,"TRANSACCION_DESCARGA_AUTOMATICA","MAQUINA_EFT_EN_JUEGO");
                }
                //return;
              }

              if(contadores.Get_Client_ID_Transaccion_Int() <= 0)
              {

                Report_Http_Code(DESCARGA_EFT_BLOQUEADA, "Id de cliente no valido para realizar descarga: "+String(contadores.Get_Client_ID_Transaccion_Int()));
                Info_Cashless.Log(RTC,"TRANSACCION_DESCARGA_AUTOMATICA","ID CLIENTE NO VALIDO PARA REALIZAR DESCARGA"+String(contadores.Get_Client_ID_Transaccion_Int()));
                //return;
              }
              



              // if (Info_Cashless.Type_Sesion() != PLAYER_CASHLESS_SESION)
              // {

              //   Transmite_Contadores_Accounting();
              //   Close_Sesion_Player_Tracking();
              //   Report_Http_Code(TERMINA_SESION_CREDITOS, "Sesion fidelizacion terminada por creditos: " + String(Creditos), true);
              //   Info_Cashless.Log(RTC,"CIERRE_SESION_AUTOMATICO_FIDELIZACION_CREDITOS","CASHLESS HABILITADO");
                
              // }

              // if (!transaccionesPendientes.empty())
              // {
              //   Report_Http_Code(TRANSFER_PENDING, "No es posible realizar  descarga automatica por creditos: " + String(Creditos) + " transaccion pendiente en maquina");
              //   Status_Barra(302);
              // }

              if (Variables_globales.Get_Variable_Global(Event_Dowmload_Cashless_Pending) || Variables_globales.Get_Variable_Global(Event_Load_Cashless_Pending))
              {
                Info_Cashless.Unlock_Reader();
                /* Pendiente por Reportar transaccion */
                // String  transaccion = transaccionesPendientes.front(); /* Toma la primera transferencia */
                Report_Http_Code(TRANSFER_PENDING, "No es posible realizar  descarga automatica por creditos: " + String(Creditos) + " transaccion pendiente de recepcion");
                Status_Barra(302);
              }

              Info_Cashless.Unlock_Reader();
              // Transmite_Contadores_Accounting();
              // Close_Sesion_Player_Tracking();
              // Report_Http_Code(TERMINA_SESION_CREDITOS, "Sesion terminada por creditos: "+String(Creditos), true);
            }
          }
          else
          {
            // Info_Cashless.Lock_Reader();
            Transmite_Contadores_Accounting();
            Close_Sesion_Player_Tracking();
            Report_Http_Code(TERMINA_SESION_CREDITOS, "Sesion terminada por creditos: "+String(Creditos), true);
            // Info_Cashless.Unlock_Reader();
            Info_Cashless.Log(RTC,"CIERRE_SESION_AUTOMATICO_FIDELIZACION_CREDITOS",String(Creditos));

            DisplayTFT.info.Mensaje = " Timeout de inactividad alcanzado\nCerrando sesion\nPor favor espere...";
            DisplayTFT.info.Ocultar = true;
            DisplayTFT.Notify_Now(EVENT_MENSAJES);
          }
        }
      }
      //Contador_Transmision_Contadores = 0;
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

/* Elimina ID Operador si no se recibe  ACK  191 despues de 60s */
void TimeOut_Marca_Operador(void)
{
    Timeout_Close_Operador = millis();

    if (Variables_globales.Get_Variable_Global(MARCA_OPERADOR_VALIDO))
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
    else
    {
      Condicion_Cumpl = false;
    }
}

/*Ejecuta Servidor FTP & Funciones de Memoria*/
void check_SD(void)
{

  Timer_SD_CHECK = millis();

  if (!Variables_globales.Get_Variable_Global(Updating_System))
  {

    if (Variables_globales.Get_Variable_Global(Ftp_Mode))
    {
      Rum_FTP_Server();
      Timer_SD_Previous = millis();
    }
    else
    {

      Queue_SD();

      Evento_Formateo_SD();

      //Task_Conexion_TFT(3000);

      if ((Timer_SD_CHECK - Timer_SD_Previous) >= SD_CHECK_Timer)
      {

       // printOpenSockets();

        //  Envio_Contadores.Transmite_Eventos_API(0x11);
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

          if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(200)))
          {

            Info_Cashless.CreaLog(archivo, false, Dia_Guarda_Logs);
            /*-----------------------> Crea Archivos fecha actual<----------------------------------------*/
            String DataTime = String(RTC.getYear()) + "-" + String(RTC.getMonth() + 1) + "-" + String(RTC.getDay()) + " " + String(RTC.getHour(true)) + ":" + String(RTC.getMinute()) + ":" + String(RTC.getSecond());

            VerificaArchivo(archivo, DataTime);
            delay(10);
            Create_ARCHIVE_Excel(Archivo_CSV_Contadores, Variables_globales.Get_Encabezado_Maquina(Encabezado_Maquina_Generica));
            delay(10);
            Create_ARCHIVE_Excel_Eventos(Archivo_CSV_Eventos, Variables_globales.Get_Encabezado_Maquina(Encabezado_Maquina_Eventos));
            delay(10);
            Create_ARCHIVE_Txt(Archivo_LOG);
            delay(10);
            Create_ARCHIVE_Excel(Archivo_CSV_Sesiones, Variables_globales.Get_Encabezado_Maquina(Encabezado_Archivo_Sesiones));
            delay(10);
            Create_ARCHIVE_Excel(Archivo_CSV_Premios, Variables_globales.Get_Encabezado_Maquina(Encabezado_Archivo_Premios));
            Serial.println("📂 OK Archivos Listos.. ✅");
            // Variables_globales.Set_Variable_Global(Flag_Archivos_OK, true);
            Total_SD = SD.totalBytes() / (1024 * 1024);
            Usado_SD = SD.usedBytes() / (1024 * 1024);
            Libre_SD = Total_SD - Usado_SD;
            Variables_globales.Set_Variable_Global_String(Espacio_Libre_SD, String(Libre_SD)); // Guarda  espacio libre de memoria
            Variables_globales.Set_Variable_Global_String(Espacio_Usado_SD, String(Usado_SD));
            Variables_globales.Set_Variable_Global_String(Size_SD, String(Total_SD));
            Variables_globales.Set_Variable_Global(Flag_Crea_Archivos, false);

            xSemaphoreGive(sd_mutex);
          }

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

        AFT.checkAndCleanLogSAS();
        Timer_SD_Previous = millis();
      }
    }
  }
}
