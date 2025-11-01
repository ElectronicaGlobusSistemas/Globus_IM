/*****************************************************
 *  Autor: Globus Sistemas                           *
 *  Libreria Manejo Storage SD  y Servidor FTP       *                                                   *                                                   *
 ******************************************************/


//----------------------------------------> Variables Globales <----------------------------------------
bool Enable_Status;
unsigned long count2 = 0;
//#define Debug_FTP
//#define Debug_Status_SD
//#define Debug_Escritura
//#define Info_SD
#define FLASH_RESET_Pin  35
#define MCU_Status      2
#define WIFI_Status     15
#define Reset_Config    27
#define Hopper_Enable   14
#define MCU_Status_2    25
#define Unlock_Machine  26
//------------------------------------------------------------------------------------------------------

//------------------------------------------> Archivos Header <-----------------------------------------
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include "Memory_SD.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include "Buffers.h"
//#include "Clase_Variables_Globales.h"
#include "Errores.h"
#include "ESP32Time.h"
#include "time.h"
#include "ESP32FtpServer.h"
#include "nvs_flash.h"
#include <esp_task_wdt.h>
#include <Persistenca_Info.h>
#include <SdFat.h>
#include <CRC32.h>
#include "RFID.h"


SdFat sd;
SdCardFactory cardFactory;
SdCard* m_card = nullptr;
uint32_t cardSectorCount = 0;
uint8_t sectorBuffer[512];

FatVolume vol;    // Volumen FAT
//------------------------------------------------------------------------------------------------------
//--------------------------------------------> Objetos Locales <-----------------------------------------------
SemaphoreHandle_t sd_mutex;
File myFile;                //  Manejo de Archivos.
File file;                  //  Manejo de Archivos En Ftp Mode.
TaskHandle_t SD_CHECK;      //  Manejador de tareas
FtpServer ftpSrv;           //  Objeto servidor FTP
TaskHandle_t Ftp_SERVER;    //  Manejador de tareas
int Contador_Escrituras=0;
int Contador_Dias=0;


#define SD_CS_PIN 5  // Cambia este pin según tu conexión
#define SPI_CLOCK SD_SCK_MHZ(10)
#define MAX_LOG_LEN      100
#define MAX_FILENAME_LEN 64


typedef struct {    
  char mensaje[MAX_LOG_LEN];
  char archivo[MAX_FILENAME_LEN];
  uint16_t fragmento_id;  // ID del fragmento
  bool es_ultimo;         // Flag para indicar si es el último fragmento
} MensajeLog;

QueueHandle_t cola_logs;
QueueHandle_t cola_contadores;

Persistenca_Info Backup;

char Archivo_CSV_Contadores_copy[200];
char Archivo_CSV_Eventos_copy[200];
char Archivo_LOG_copy[200];
char Archivo_CSV_Sesiones_copy[200];
char Archivo_CSV_Premios_copy[200];
String Estructura_CSV_Sesiones[2];
String Estructura_CSV_Premios[2];

extern char Archivo_CSV_Contadores[200];
extern char Archivo_CSV_Eventos[200];
extern char Archivo_LOG[200];

//int Sd_Mont=false;
extern bool Archivos_Ready;
int Valida_Archivos_Eliminados=0;
int Mes_Limite=4;
bool Borrado_completado=false;


unsigned long Timeout_FTP=0;
unsigned long Timeout_FTP_Previous=0;
unsigned long Deshabilita_FTP=600000;


unsigned long InicialTime = 0;
unsigned long FinalTime = 0;
int Conteo = 0;
bool SD_State = LOW;
bool Set_WATCHDOG=false;

//------------------------------------------------------------------------------------------------------

//------------------------------------------> Objetos Extern <------------------------------------------
extern Variables_Globales Variables_globales;
extern ESP32Time RTC;
//------------------------------------------------------------------------------------------------------

//--------------------------------------> Variables externas <------------------------------------------
extern char Fallo[64];
extern int month_copy;
extern int year_copy;

extern bool Formateo;
extern int Result_Formatt;
//------------------------------------------------------------------------------------------------------
extern Buffers Buffer;            // Objeto de buffer de mensajes servidor
//---------------------------------------> Inicializa SD <----------------------------------------------
int Intento_Connect_SD = 0; // Variable Contadora de Intentos de Conexión SD.

#include "Configuracion.h"
extern Configuracion_ESP32 Configuracion;
extern Cashless_API Info_Cashless;
extern Eventos_SAS eventos; // Objeto contiene eventos maquina
//--------------------------------------> Bus SPI <-----------------------------------------------------
//extern SPIClass spiRFID;
/**********************************************************************************/
/*                              Inicializa Modulo SD                              */
/**********************************************************************************/
bool formatearSD(void) {
  // Inicializa la tarjeta SD
  m_card = cardFactory.newCard(SdSpiConfig(SD_CS_PIN, SHARED_SPI, SPI_CLOCK));
  if (!m_card || m_card->errorCode()) {
    Serial.println("Error: no se pudo inicializar la tarjeta SD.");
    return false;
  }

  // // Verifica tamaño
  // cardSectorCount = m_card->sectorCount();
  // if (!cardSectorCount) {
  //   Serial.println("Error: no se pudo obtener el tamaño de la tarjeta.");
  //   return false;
  // }

  // // Validar que tenga tamaño mínimo requerido para FAT32
  // if (cardSectorCount <= 4194304) { // 2 GiB mínimo recomendado para FAT32
  //   Serial.println("Error: la tarjeta es muy pequeña para FAT32.");
  //   return false;
  // }

  Serial.println("Formateando en FAT32...");
  FatFormatter fatFormatter;
  if (!fatFormatter.format(m_card, sectorBuffer)) {
    Serial.println("Error al formatear en FAT32.");
    return false;
  }

  Serial.println("Formateo FAT32 completado correctamente.");
  return true;
}

void Init_SD(void)
{
//   spiRFID.begin(18,19,23,SD_ChipSelect);
//  // spiRFID.setClockDivider(SPI_CLOCK_DIV128); /*10000000*/
//   spiRFID.setFrequency(500000);

 // formatearSD();
  
  if(/*SD.begin( SD_ChipSelect, spiRFID, 500000)*/ SD.begin(SD_ChipSelect,SPI))
  {
    Serial.println("💾 Memoria SD Inicializada... ✅");
    Variables_globales.Set_Variable_Global(SD_INSERT,true);
    digitalWrite(SD_Status,HIGH);

    
    
    
    //Backup.Init_Archive_Backup();
    // File root = SD.open("/");
    // while (File file = root.openNextFile())
    // {
    //   Serial.print("Borrando archivo: ");
    //   Serial.println(file.name());
    //   file.close();
    //   SD.remove(file.name());
    // }
    // Serial.println("Todos los archivos han sido borrados.");

    // root.close();

    

    cola_logs = xQueueCreate(5, sizeof(MensajeLog));

    // const char* criticalFiles[] = {
    //     "/Contadores-692025.csv"
    // };
    // bool IsSuccess =Cheack_SD_CriticalMemory(criticalFiles,1);

    // if(IsSuccess)
    //   Report_Http_Code(READER_KO, "Verificacion de memoria y archivos OK", false);
    // else
    //   Report_Http_Code(READER_KO, "Fallo de integridad de memoria critica", false);

    // if(Check_Card())
    // {
    //   Serial.println("💾 Verificacion de integridad de memoria critica ✅");
      
    //   //const char *criticalFiles[1]={"/Contadores-1792025.CSV"};
    //   Info_Cashless.Log(RTC,"VERIFICANDO_MEMORIA_CRITICA","VERIFICANDO_INTEGRIDAD_DE_ARCHIVOS..");
      
    //   // if (Cheack_SD_CriticalMemory(criticalFiles, 1))
    //   // {
    //   //   Serial.println("Verificacion exitosa ✅");
    //   //   Info_Cashless.Log(RTC, "VERIFICANDO_MEMORIA_CRITICA", "VERIFICACION_EXITOSA");
    //   // }
    //   // else
    //   // {
    //   //   Serial.println("Verificacion fallida ❌");
    //   //   Info_Cashless.Log(RTC, "VERIFICANDO_MEMORIA_CRITICA", "VERIFICACION_FALLIDA");
    //   // }
    // }
    // else
    // {
    //   Serial.println("💾 Verificacion de integridad de memoria critica ❌");

    //   if (eventos.Set_evento(ERROR_VALIDACION_MEMORIA_SD))
    //   {
    //     Variables_globales.Set_Variable_Global(Dato_Evento_Valido, true);
    //   }

    //   Info_Cashless.Log(RTC, "VERIFICANDO_MEMORIA_CRITICA", "VERIFICACION_FALLIDA");
    // }
  }else
  {

    Serial.println("💾 Memoria SD no insertada... ❌");
    Variables_globales.Set_Variable_Global(SD_INSERT,false);
    digitalWrite(SD_Status,LOW);


    /* Evento memoria removida */
    // if (eventos.Set_evento(MEMORIA_NO_INSERTADA))
    // {
    //   Variables_globales.Set_Variable_Global(Dato_Evento_Valido, true);
    // }
  }
}
//------------------------------------------------------------------------------------------------------
//---------------------------------------> Inicializa Servidor FTP <------------------------------------
bool Init_FTP_SERVER()
{
  if (Variables_globales.Get_Variable_Global(SD_INSERT) == true && WiFi.status() == WL_CONNECTED)
  {
    RESET_SD();
    ftpSrv.begin("GlobusAmin", "Globussistemas23", "SuperGlobusAdmin", "SuperG2023");
    return true;
  }
  else
    return false;
}
//------------------------------------------------------------------------------------------------------
//---------------------------------> Aquí Tarea Control Servidor FTP <----------------------------------
/* Cola de eventos SD Controla Escritura de logs en orden de llegada de las peticiones */
void Queue_SD(void)
{

  if (Variables_globales.Get_Variable_Global(SD_INSERT) && !Variables_globales.Get_Variable_Global(Ftp_Mode))
  {
    MensajeLog msg;
    static String mensaje_completo = "";  // Variable para almacenar el mensaje completo
    if (xQueueReceive(cola_logs, &msg, 0) == pdTRUE)
    {
      mensaje_completo += String(msg.mensaje);
      // Serial.println(msg.mensaje);
      // Serial.println("");
      if(msg.es_ultimo)
      {
        mensaje_completo.replace("/LogESP.txt", "");
        Erro_Log_Write(mensaje_completo, String(msg.archivo)); // o tu versión interna que accede a la SD
        mensaje_completo = "";
      }
    }
  }
}

void Rum_FTP_Server(void)
{
  if(Set_WATCHDOG==false)
  {
    esp_task_wdt_init(10000, true);
    Set_WATCHDOG=true;
  }

   esp_task_wdt_reset(); /* Reset Timer Lista Larga de archivos*/
    Conteo++;
    InicialTime = millis();
    if ((InicialTime - FinalTime) >= 100)
    {
      if (Enable_Status == true)
      {
        FinalTime = InicialTime;
        SD_State = !SD_State;
        digitalWrite(SD_Status, SD_State);
      }
    }

    if(xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(200)))
    {
      ftpSrv.handleFTP(); // Verifica Mensajes y Transferencias FTP.
      xSemaphoreGive(sd_mutex);
    }
      
    esp_task_wdt_reset(); /* Reset Timer Lista Larga de archivos*/
    vTaskDelay(10);
}

void FtpFast(void)
{

  if (Variables_globales.Get_Variable_Global(Ftp_Mode))
  {
    esp_task_wdt_reset();

    // if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(200)))
    // {
      ftpSrv.handleFTP(); // Verifica Mensajes y Transferencias FTP.
    //   xSemaphoreGive(sd_mutex);
    // }
    esp_task_wdt_reset();
  }
}

void Ftp_handle(void)
{
  esp_task_wdt_reset(); /* Reset Timer Lista Larga de archivos*/
  if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(200)))
  {
    ftpSrv.handleFTP(); // Verifica Mensajes y Transferencias FTP.
    xSemaphoreGive(sd_mutex);
  }
  esp_task_wdt_reset();
}

//------------------------------------------------------------------------------------------------------
//---------------------------------> Aquí Tarea para Verifcar Conexión de SD <--------------------------
static void Task_Verifica_Conexion_SD(void *parameter)
{ int Contador=0;
  #ifdef Info_SD
  Serial.println("Verificador de Memoria SD Activado");
  #endif
 
  for (;;)
  {
    uint8_t Temperatura_Procesador_GPU = temperatureRead();
    Variables_globales.Set_Variable_Global_String(Temperatura_procesador, String(Temperatura_Procesador_GPU));
    if (!SD.begin(SD_ChipSelect)) // SD Desconectada...
    {
      #ifdef Info_SD
      Serial.println("Memoria SD Desconectada..");
      #endif
      Variables_globales.Set_Variable_Global(SD_INSERT,false);
      if(!SD.begin(SD_ChipSelect))
      {
      // Apaga  Indicador LED SD Status.
      Enable_Status = false; // Desactiva Parpadeo de LED SD Status en Modo FTP Server


        // Intenta Conectar Despues de Fallo.
        #ifdef Info_SD
        Serial.print("Fallo en Conexión SD"); // Mensaje de Fallo.
        Serial.print(" Intento #: ");         // Mensaje de Fallo.
        Serial.println(Intento_Connect_SD);   // Imprime conteo de Fallos.
        #endif
        Intento_Connect_SD++;
        
        digitalWrite(SD_Status, LOW); // Apaga Indicador LED SD Status.
        Variables_globales.Set_Variable_Global(Enable_SD, false);
        Variables_globales.Set_Variable_Global_String(Espacio_Libre_SD, "0000");
        Variables_globales.Set_Variable_Global_String(Espacio_Usado_SD, "0000");
        Variables_globales.Set_Variable_Global_String(Size_SD, "0000");
        //Sd_Mont = false;
        Variables_globales.Set_Variable_Global(SD_INSERT,false);
        
      }
    }
    else
    {
      
      #ifdef Info_SD
      Serial.println("SD OK"); // Mensaje de Conexión SD.
      #endif
      FreeSpace_SD();
      Variables_globales.Set_Variable_Global(Enable_SD, true);
      digitalWrite(SD_Status, HIGH); // Enciende Indicador LED SD Status.
      Enable_Status = true;          // Habilita  El Parpadeo de LED SD Status en Modo FTP Server
      //Sd_Mont = true;
      Variables_globales.Set_Variable_Global(SD_INSERT,true);
      // log_e("Error Inicializando SD: ", ERROR_INICIALIZANDO_SD);
      // LOG_ESP(Archivo_LOG,Variables_globales.Get_Variable_Global(Enable_Storage));
      if (Contador_Escrituras > 0 && Variables_globales.Get_Variable_Global(Enable_Storage) == true)
      {
        Variables_globales.Set_Variable_Global(Estado_Escritura, true);
      }
      else
      {
        Variables_globales.Set_Variable_Global(Estado_Escritura, false);
      }
      // Serial.println("Memoria SD Conectada");
      if (Variables_globales.Get_Variable_Global(Ftp_Mode) == false)
      {

        if (Variables_globales.Get_Variable_Global(Comunicacion_Maq) && Variables_globales.Get_Variable_Global(Sincronizacion_RTC) && Variables_globales.Get_Variable_Global(Flag_Crea_Archivos) && !Variables_globales.Get_Variable_Global(Ftp_Mode) && Variables_globales.Get_Variable_Global(SD_INSERT))
        {
          Serial.println("Creado Archivos......");
          /*-----------------------> Crea Archivos fecha actual<----------------------------------------*/
          Create_ARCHIVE_Excel(Archivo_CSV_Contadores, Variables_globales.Get_Encabezado_Maquina(Encabezado_Maquina_Generica));
          delay(100);
          Create_ARCHIVE_Excel_Eventos(Archivo_CSV_Eventos, Variables_globales.Get_Encabezado_Maquina(Encabezado_Maquina_Eventos));
          delay(100);
          Create_ARCHIVE_Txt(Archivo_LOG);
          delay(100);
          Serial.println("OK Archivos Listos..");
          Variables_globales.Set_Variable_Global(Flag_Crea_Archivos, false);
          Variables_globales.Set_Variable_Global(Flag_Archivos_OK,true);
          /*--------------------------------------------------------------------------------------------*/
        }

        int Total_SD= SD.totalBytes() / (1024 * 1024);
        int Usado_SD=SD.usedBytes() / (1024 * 1024);
        int Libre_SD=Total_SD-Usado_SD;
        #ifdef Info_SD
        Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
        Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
        Serial.println("Free Space: " +String(Libre_SD))+"MB";
        #endif
        Variables_globales.Set_Variable_Global_String(Espacio_Libre_SD,String(Libre_SD)); // Guarda  espacio libre de memoria
        Variables_globales.Set_Variable_Global_String(Espacio_Usado_SD,String(Usado_SD));
        Variables_globales.Set_Variable_Global_String(Size_SD,String(Total_SD));

        if(Libera_Memoria(Total_SD, Usado_SD))
        {
          #ifdef Debug_Escritura
          Serial.println("Alerta Memoria llena Borrando Datos...");
          #endif
          /*Memoria llena*/
          Variables_globales.Set_Variable_Global(Flag_Memoria_SD_Full,true);
        }else{
          /*Memoria libre Espacio usado <80% de memoria total*/
          Variables_globales.Set_Variable_Global(Flag_Memoria_SD_Full,false);
        }
      }
      
    }
    vTaskDelay(9000); // Pausa Tarea 10000ms
  }
  vTaskDelay(10);
}
//------------------------------------------------------------------------------------------------------
//---------------------> Función Para Crear Archivos Txt sin Formato <---------------------------------
void FreeSpace_SD(void)
{
  
  if (Variables_globales.Get_Variable_Global(Estado_Escritura))
  {
    #ifdef Debug_Status_SD
    Serial.println("Estado: Escritura");
    #endif
  }
  else
  {
    if (Variables_globales.Get_Variable_Global(Ftp_Mode))
    {
      #ifdef Debug_Status_SD
      Serial.println("Estado: FTP");
      #endif
    }
    else
    {
      #ifdef Debug_Status_SD
      Serial.println("Estado: Deshabilitada");
      #endif
    }
  }
}

void Create_ARCHIVE_Txt(char *ARCHIVO)
{
  if (!SD.exists("/"+String(ARCHIVO))) // Pregunta si el archivo Existe.
  {
    File myFile___ = SD.open("/"+String(ARCHIVO), FILE_WRITE); // Si no Existe lo abre
    if (!myFile___)
    {
      #ifdef Debug_Status_SD
      Serial.println("No se pudo Crear Archivo LOG");
      #endif
   //   Variables_globales.Set_Variable_Global(Fallo_Archivo_LOG,true);
    }
    else
    {
      #ifdef Debug_Status_SD
      Serial.println("Archivo LOG Creado: " + String(ARCHIVO));
      #endif

      myFile___.flush();
      myFile___.close();
      Variables_globales.Set_Variable_Global(Fallo_Archivo_LOG,false);
      Variables_globales.Set_Variable_Global(Archivo_CSV_OK, true);
      Variables_globales.Set_Variable_Global(Flag_Archivos_OK, true);
    }
  }
  else if (SD.exists("/"+String(ARCHIVO)))
  {
    #ifdef Debug_Status_SD
    Serial.println("El Archivo Existia.. Continua Guardando en: " + String(ARCHIVO));
    #endif
  //  Variables_globales.Set_Variable_Global(Fallo_Archivo_LOG,false);
  //  Variables_globales.Set_Variable_Global(Archivo_CSV_OK, true);
  Variables_globales.Set_Variable_Global(Flag_Archivos_OK, true);
  }
}
//------------------------------------------------------------------------------------------------------
//-----------------------------> Función Para guardar Eventos En SD <-----------------------------------
void Write_Data_File_Txt(String Datos, char *ARCHIVO)
{
  if (Variables_globales.Get_Variable_Global(SD_INSERT))
  {

    if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(200)))
    {
      File myFile_txt = SD.open("/" + String(ARCHIVO), FILE_APPEND);
      if (!myFile_txt)
      {
        #ifdef Debug_Escritura
        Serial.println("Error al Escribir en Archivo: " + (String)ARCHIVO);
        #endif
        Contador_Escrituras = 0;
      }
      else
      {
        myFile_txt.println(Datos);
        myFile_txt.flush();
        myFile_txt.close();
        #ifdef Debug_Escritura
        Serial.println("Evento Guardado en SD");
        #endif
        Contador_Escrituras++;
      }
      xSemaphoreGive(sd_mutex);
    }
  }
}

void LOG_ESP(char *ARCHIVO, bool Enable)
{

  if (Variables_globales.Get_Variable_Global(SD_INSERT))
  {
    if (Enable == true)
    {

      if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(200)))
      {
        File myFile2;
        myFile2 = SD.open("/" + String(ARCHIVO), FILE_APPEND);
        if (!myFile2)
        {
#ifdef Debug_Escritura
          Serial.println("Error al Escribir en Archivo: " + (String)ARCHIVO);
#endif
          Contador_Escrituras = 0;
        }
        else
        {
          String Datos = String(Fallo);
          myFile2.println(RTC.getTime() + " Error: " + Datos);
          myFile.flush();
          myFile2.close();
#ifdef Debug_Escritura
          Serial.println("LOG Guardado");
#endif
          Contador_Escrituras++;
        }
        xSemaphoreGive(sd_mutex);
      }
    }
    else
    {
#ifdef Debug_Escritura
      Serial.println("Guardado Deshabilitado");
#endif
    }
  }
}

void LOG_ESP_Descrip(char *ARCHIVO,bool Enable,String Mensaje)
{


  if(Variables_globales.Get_Variable_Global(SD_INSERT))
  {
    if (Enable == true)
    {

      if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(200)))
      {
        File myFile2;
        myFile2 = SD.open("/" + String(ARCHIVO), FILE_APPEND);
        if (!myFile2)
        {
#ifdef Debug_Escritura
          Serial.println("Error al Escribir en Archivo: " + (String)ARCHIVO);
#endif
          Contador_Escrituras = 0;
        }
        else
        {
          myFile2.println(RTC.getTime() + " Error: " + Mensaje);
          myFile.flush();
          myFile2.close();
#ifdef Debug_Escritura
          Serial.println("LOG Guardado");
#endif
          Contador_Escrituras++;
        }
        xSemaphoreGive(sd_mutex);
      }
    }
    else
    {
      #ifdef Debug_Escritura
      Serial.println("Guardado Deshabilitado");
      #endif
    }
  }
}
//-------------------------------------------------------------------------------------------------------
//-----------------------> Función Para Crear archivo de contadores con encabezado <---------------------
void Create_ARCHIVE_Excel(char *ARCHIVO, String Encabezado)
{

  if (!SD.exists("/" + String(ARCHIVO))) // Si el archivo no existe lo Crea con encabezado para Excel!!
  {
    File myFile_ = SD.open("/" + String(ARCHIVO), FILE_WRITE);

    if (!myFile_)
    {
#ifdef Debug_Escritura
      Serial.println("No Fue Posible Crear el Archivo");
#endif
      //  Variables_globales.Set_Variable_Global(Fallo_Archivo_COM,true);
    }
    else
    {
      myFile_.println(Encabezado);
      myFile_.flush();
      myFile_.close();
#ifdef Debug_Escritura
      Serial.println("Archivo: " + (String)ARCHIVO + " Creado con Encabezado");
#endif
      //  Variables_globales.Set_Variable_Global(Fallo_Archivo_COM,false);
      //  Variables_globales.Set_Variable_Global(Archivo_CSV_OK, true);
      Variables_globales.Set_Variable_Global(Flag_Archivos_OK, true);
    }
  }
  else if (SD.exists("/" + String(ARCHIVO)))
  {
    // #ifdef Debug_Escritura
    Serial.println("El Archivo Existia.. Continua Guardando en: " + (String)ARCHIVO);
    // #endif
    // Variables_globales.Set_Variable_Global(Fallo_Archivo_COM,false);
    // Variables_globales.Set_Variable_Global(Archivo_CSV_OK, true);
    Variables_globales.Set_Variable_Global(Flag_Archivos_OK, true);
  }
}
//-----------------------> Funcion para Crear Archivo de eventos con encabezado <------------------------
void Create_ARCHIVE_Excel_Eventos(char *ARCHIVO, String Encabezado)
{
  if (!SD.exists("/"+String(ARCHIVO))) // Si el archivo no existe lo Crea con encabezado para Excel!!
  {
   File myFile__ = SD.open("/"+String(ARCHIVO), FILE_WRITE);

    if (!myFile__)
    {
      #ifdef Debug_Escritura
      Serial.println("No Fue Posible Crear el Archivo");
      #endif
     // Variables_globales.Set_Variable_Global(Fallo_Archivo_EVEN,true);
    }
    else
    {
      myFile__.println(Encabezado);
      myFile__.flush();
      myFile__.close();
      #ifdef Debug_Escritura
      Serial.println("Archivo: " + (String)ARCHIVO + " Creado con Encabezado");
      #endif
    //  Variables_globales.Set_Variable_Global(Fallo_Archivo_EVEN,false);
    //  Variables_globales.Set_Variable_Global(Archivo_CSV_OK, true);
    }
  }
  else if (SD.exists("/"+String(ARCHIVO)))
  {
    //#ifdef Debug_Escritura
    Serial.println("El Archivo Existia.. Continua Guardando en: " + (String)ARCHIVO + ".csv");
    //endif
  //  Variables_globales.Set_Variable_Global(Fallo_Archivo_EVEN,false);
  //  Variables_globales.Set_Variable_Global(Archivo_CSV_OK, true);
  }
}
//--------------------------------------------------------------------------------------------------------
//---------------------------> Función para Escribir En Archivos CSV <------------------------------------
void Write_Data_File(String Datos, char *ARCHIVO, bool select)
{
  File myFile_ = SD.open("/" + String(ARCHIVO), FILE_WRITE);
  if (select == false)
  {
    if (!myFile_)
    {
#ifdef Debug_Escritura
      Serial.println("Error al escribir en SD");
#endif
    }
    else
    {
      myFile_.print(Datos);
      myFile_.print(",");
#ifdef Debug_Escritura
      Serial.println("Dato: " + Datos + " Guardado en SD");
#endif
      myFile_.flush();
      myFile_.close();
    }
  }
  else
  {
    if (!myFile_)
    {
#ifdef Debug_Escritura
      Serial.println("Error al escribir en SD");
#endif
    }
    else
    {
      myFile_.println(Datos);
#ifdef Debug_Escritura
      Serial.println("Dato: " + Datos + " Guardado en SD");
#endif
      myFile_.flush();
      myFile_.close();
    }
  }
}
//--------------------------------------------------------------------------------------------------------
//---------------------------> Función para Escribir En Archivos CSV <------------------------------------

int Cuenta_Fallos=0;

void Write_Data_File2(String Datos, String archivo, bool select, String Encabezado)
{
  if (Variables_globales.Get_Variable_Global(SD_INSERT))
  {
    if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(100)))
    {
      File myFileLocal = SD.open("/" + archivo, FILE_APPEND);
      if (!myFileLocal)
      {
        Cuenta_Fallos++;
        if (Cuenta_Fallos >= 2)
        {
          Variables_globales.Set_Variable_Global(Falla_MicroSD, true);
          Cuenta_Fallos = 0;
        }
        // #ifdef Debug_Escritura
        Serial.println("Error No se pudo Abrir el  Archivo: " + (String)archivo);
        // #endif
        Contador_Escrituras = 0;
      }
      else if (myFileLocal) //  else por else if
      {
        digitalWrite(SD_Status, LOW);
        myFileLocal.println(Datos);
        myFileLocal.flush();
        myFileLocal.close();
        #ifdef Debug_Escritura
        Serial.println("Contadores Guardados en SD");
        #endif
        Contador_Escrituras++;
        digitalWrite(SD_Status, HIGH);
      }

      xSemaphoreGive(sd_mutex);
    }
    else
    {
      #ifdef Debug_Escritura
      Serial.println("Recurso SD OCUPADO");
      #endif
    }
  }
}

void Erro_Log(String Datos, String archivo) {
    if (Variables_globales.Get_Variable_Global(SD_INSERT) && !Variables_globales.Get_Variable_Global(Ftp_Mode)) {

        MensajeLog log;
        size_t total_len = Datos.length();  // Longitud total del mensaje
        size_t fragment_size = MAX_LOG_LEN;  // Tamaño de cada fragmento

        // Si el mensaje es más corto o igual que el tamaño máximo permitido
        if (total_len <= fragment_size) {
            // Enviar el mensaje entero en un solo fragmento
            memset(log.mensaje, 0, MAX_LOG_LEN);
                        

            strncpy(log.mensaje, Datos.c_str(), total_len);
            log.mensaje[total_len] = '\0';  // Asegurarse de que la cadena termine correctamente

            // Copiar el nombre del archivo
            strncpy(log.archivo, archivo.c_str(), MAX_FILENAME_LEN - 1);
            log.archivo[MAX_FILENAME_LEN - 1] = '\0';

            // Configurar el fragmento
            log.fragmento_id = 0;
            log.es_ultimo = true;  // El único fragmento es el último

            // Enviar el fragmento a la cola
            if (xQueueSend(cola_logs, &log, 50) != pdTRUE) {
                #ifdef Debug_Escritura
                Serial.println("No se envió log a la cola");
                #endif
            } else {
                #ifdef Debug_Escritura
                Serial.println("Mensaje completo enviado a la cola");
                #endif
            }
        }
        else {
            // Si el mensaje es más largo que el tamaño máximo permitido, lo fragmentamos
            size_t start_pos = 0;
            uint16_t fragment_id = 0;

            // Calculamos cuántos fragmentos necesitamos enviar
            size_t num_fragments = (total_len + fragment_size - 1) / fragment_size;

            // Copiar el nombre del archivo
            
           
            // Usamos un bucle for para fragmentar y enviar los datos
            for (size_t i = 0; i < num_fragments; i++) {
                size_t remaining_len = total_len - start_pos;
                size_t len_to_copy = (remaining_len < fragment_size) ? remaining_len : fragment_size;
                memset(log.mensaje, 0, MAX_LOG_LEN);
                // Copiar el fragmento de datos
                strncpy(log.mensaje, Datos.c_str() + start_pos, len_to_copy);
                log.mensaje[len_to_copy] = '\0'; // Asegurarse de que la cadena termine correctamente

                //Serial.println(log.mensaje);
                //Serial.println(archivo.c_str());
                // Configurar el fragmento
                log.fragmento_id = fragment_id;
                log.es_ultimo = (start_pos + len_to_copy) >= total_len; // Si es el último fragmento

                strncpy(log.archivo, archivo.c_str(), MAX_FILENAME_LEN - 1);
                log.archivo[MAX_FILENAME_LEN - 1] = '\0';

                // Enviar el fragmento a la cola
                if (xQueueSend(cola_logs, &log, 50) != pdTRUE) {
                    #ifdef Debug_Escritura
                    Serial.println("No se envió log a la cola");
                    #endif
                } else {
                    #ifdef Debug_Escritura
                    Serial.println("Fragmento de log enviado a la cola");
                    #endif
                }

                // Actualizar la posición de inicio y el ID del fragmento
                start_pos += len_to_copy;
                fragment_id++;
            }
        }
    }
}

void Erro_Log_Write(String Datos, String archivo)
{
  if (Variables_globales.Get_Variable_Global(SD_INSERT) && !Variables_globales.Get_Variable_Global(Ftp_Mode))
  {

    Variables_globales.Set_Variable_Global(Flag_Log, true);

    if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(100)))
    {

      File myFileLocal = SD.open(archivo, FILE_APPEND);
      if (!myFileLocal)
      {
        #ifdef Debug_Escritura
        Serial.println("Error No se pudo Abrir el  Archivo: " + (String)archivo);
        #endif
      }
      else //  else por else if
      {
        myFileLocal.println(Datos);
        myFileLocal.flush();
        myFileLocal.close();
        #ifdef Debug_Escritura
        Serial.println("Log Capturado");
        #endif
      }
      xSemaphoreGive(sd_mutex); // Libera el acceso

      delay(10);
    }
    else
    {
      #ifdef Debug_Escritura
      Serial.println("Recurso SD OCUPADO");
      #endif
    }

    Variables_globales.Set_Variable_Global(Flag_Log, false);
  }
}

bool VerificaArchivo(const char* archivo, String DataTime)
{
  if(SD.exists(archivo))
  {
    //Serial.print("Archivo ya existe: ");
    //Serial.println(archivo);
    return true;
  }else{

    myFile=SD.open(archivo,FILE_WRITE);

    if(myFile)
    {
      myFile.println(DataTime);
      //Serial.println("Archivo Creado...");
      myFile.close();

      return true;
    }else
    {
      return false;
    }
  }
  return false;
}

//--------------------------------------------------------------------------------------------------------
//---------------------------> Funcion para  guardar operador  reset handpay <----------------------------
void Storage_Premios_OP(String archivo, bool Enable, byte *Buffer)
{
  String Datos2;
  if (Enable)
  {
    if (Variables_globales.Get_Variable_Global(SD_INSERT))
    {

      if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(200)))
      {
        File myFileOp = SD.open("/" + archivo, FILE_APPEND);
        if (!myFileOp)
        {
#ifdef Debug_Escritura
          Serial.println("Error No se pudo Abrir el  Archivo: " + (String)archivo);
#endif
          Contador_Escrituras = 0;
        }
        else if (myFileOp) //  else por else if
        {

          /* Fecha */
          Estructura_CSV_Premios[0] = RTC.getTime() + ",";
          /* Operador */
          int BT11 = (int)Buffer[0] - 48;
          int BT22 = (int)Buffer[1] - 48;
          int BT33 = (int)Buffer[2] - 48;
          int BT44 = (int)Buffer[3] - 48;
          int BT55 = (int)Buffer[4] - 48;
          int BT66 = (int)Buffer[5] - 48;
          int BT77 = (int)Buffer[6] - 48;
          int BT88 = (int)Buffer[7] - 48;
          Estructura_CSV_Premios[1] = String(BT11) + String(BT22) + String(BT33) + String(BT44) + String(BT55) + String(BT66) + String(BT77) + String(BT88);

          /* Almacena estructura en archivo CSV*/
          Datos2 = Estructura_CSV_Premios[0] + Estructura_CSV_Premios[1];
          myFileOp.println(Datos2);
          myFileOp.flush();
          myFileOp.close();

          for (int i = 0; i <= Datos2.length(); i++)
          {
            Datos2.remove(i);
          }
          for (int i = 0; i < 2; i++)
          {
            Estructura_CSV_Premios[i] = "N/A";
          }

#ifdef Debug_Escritura
          Serial.println("Premio Guardado en SD");
#endif
          Contador_Escrituras++;
        }
        xSemaphoreGive(sd_mutex);
      }
    }
  }else{
    if(Variables_globales.Get_Variable_Global(SD_INSERT) == 1)
    {
      for (int i = 0; i <= Datos2.length(); i++)
      {
        Datos2.remove(i);
      }
      for (int i = 0; i < 2; i++)
      {
        if (i == 1)
        {
          Estructura_CSV_Premios[i] = "N/A";
        }
        else
        {
          Estructura_CSV_Premios[i] = "N/A,";
        }
      }
    }
  }
}
//---------------------------> Funcion para guardar inicios de sesion clientes <--------------------------
void Storage_Cliente(String archivo, bool Enable, byte *Buffer)
{
  String Datos;
  if (Enable)
  {
    if (Variables_globales.Get_Variable_Global(SD_INSERT))
    {

      if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(200)))
      {

        File myFile_ = SD.open("/" + archivo, FILE_APPEND);
        if (!myFile_)
        {
#ifdef Debug_Escritura
          Serial.println("Error No se pudo Abrir el  Archivo: " + (String)archivo);
#endif
          Contador_Escrituras = 0;
        }
        else if (myFile_) //  else por else if
        {

          /* Fecha y Cliente */
          Estructura_CSV_Sesiones[0] = RTC.getTime() + ",";

          int BT1 = (int)Buffer[0] - 48;
          int BT2 = (int)Buffer[1] - 48;
          int BT3 = (int)Buffer[2] - 48;
          int BT4 = (int)Buffer[3] - 48;
          int BT5 = (int)Buffer[4] - 48;
          int BT6 = (int)Buffer[5] - 48;
          int BT7 = (int)Buffer[6] - 48;
          int BT8 = (int)Buffer[7] - 48;

          Estructura_CSV_Sesiones[1] = String(BT1) + String(BT2) + String(BT3) + String(BT4) + String(BT5) + String(BT6) + String(BT7) + String(BT8);
          Datos = Estructura_CSV_Sesiones[0] + Estructura_CSV_Sesiones[1];
          myFile_.println(Datos);
          myFile_.flush();
          myFile_.close();

          for (int i = 0; i <= Datos.length(); i++)
          {
            Datos.remove(i);
          }
          for (int i = 0; i < 2; i++)
          {
            if (i == 1)
            {
              Estructura_CSV_Sesiones[i] = "N/A";
            }
            else
            {
              Estructura_CSV_Sesiones[i] = "N/A,";
            }
          }

#ifdef Debug_Escritura
          Serial.println("Cliente Guardado en SD");
#endif
          Contador_Escrituras++;
        }
        xSemaphoreGive(sd_mutex);
      }
    }
  }
  else
  {

    if (Variables_globales.Get_Variable_Global(SD_INSERT))
    {
      for (int i = 0; i <= Datos.length(); i++)
      {
        Datos.remove(i);
      }
      for (int i = 0; i < 2; i++)
      {
        if (i == 1)
        {
          Estructura_CSV_Sesiones[i] = "N/A";
        }
        else
        {
          Estructura_CSV_Sesiones[i] = "N/A,";
        }
      }
    }
  }
}
//---------------------------> Funcion para actualizar estado de memoria <--------------------------------
void Update_Status_SD(void)
  {
    if (Variables_globales.Get_Variable_Global(Ftp_Mode) == true || Variables_globales.Get_Variable_Global(Flag_Memoria_SD_Full) == true || Variables_globales.Get_Variable_Global(SD_INSERT) == false)
    {
      Variables_globales.Set_Variable_Global(Enable_Storage, false); // Deshabilita  Guardado SD.
    }
    else
    {
      if (Variables_globales.Get_Variable_Global(Sincronizacion_RTC) == true && Variables_globales.Get_Variable_Global(Ftp_Mode) == false && Variables_globales.Get_Variable_Global(Flag_Memoria_SD_Full) == false && Variables_globales.Get_Variable_Global(SD_INSERT) == true && Variables_globales.Get_Variable_Global(Flag_Archivos_OK)==true)
      {
        Variables_globales.Set_Variable_Global(Enable_Storage, true);
      }
    }
  }
//-----------------------------------> Función Para Reset SD <--------------------------------------------
void RESET_SD(void)
{
  SD.end();
  SD.begin(SD_ChipSelect); // Intento Conectar SD
}


//-----------------------------------> Funcion para inicio y reset <--------------------------------------
void RESET_SD_2(bool Select)
{
  if(Select)
  {
     SD.begin(SD_ChipSelect); // Intento Conectar SD
  }else{
     SD.end();
  }
 
 
}
//--------------------------------------------------------------------------------------------------------
//------------------------------> Función Lectura de Archivo Formato String <-----------------------------
void Read_File(const char *ARCHIVO)
{
  myFile = SD.open("/"+String(ARCHIVO));
  if (myFile)
  {
    while (myFile.available())
    {
      Serial.write(myFile.read());
    }
    myFile.close();
  }
  else
  {
    #ifdef Debug_Escritura
    Serial.print("Error en lectura de archivo ");
    Serial.println(ARCHIVO);
    #endif
  }
}
//--------------------------------------------------------------------------------------------------------
//------------------------------> Función Para Borrar Archivos <------------------------------------------
bool Remove_Archive(char *ARCHIVO)
{
  if (SD.exists("/"+String(ARCHIVO)))
  {
    SD.remove("/"+String(ARCHIVO));
    #ifdef Debug_Escritura
    Serial.print("\nSe ha Eliminado ");
    Serial.print(ARCHIVO);
    Serial.println(" Exitosamente.");
    #endif
    return 1;
  }
  else
  {
    #ifdef Debug_Escritura
    Serial.println("\nNo existe archivo: " + (String)ARCHIVO);
    #endif
  } 
  return 0;
}
/*Función para borrar datos de la memoria SD cuando el 80% de su capacidad esta  ocupada.
retorna [1]----> Memoria Llena
        [0]----> Memoria Libre*/
bool Libera_Memoria(int Total_Memoria_MB, int Espacio_Usado_MB)
{

    int Free_Space;
    int Limite_MB = Total_Memoria_MB * 0.8;
    Free_Space=Total_Memoria_MB-Espacio_Usado_MB;
    if(Espacio_Usado_MB<0 ||Espacio_Usado_MB>Total_Memoria_MB+300 || Free_Space<0 || Free_Space>Total_Memoria_MB+300||Total_Memoria_MB<0||Total_Memoria_MB==0)
    {
      #ifdef Debug_Escritura
      Serial.println("Error en Volumen de Memoria");
      #endif
    }
    else
    {
      
      if (Espacio_Usado_MB >= Limite_MB || Variables_globales.Get_Variable_Global(Libera_Memoria_OK) == true) // 80%  del espacio de Memoria Utilizada.
      {
        if(Variables_globales.Get_Variable_Global(Sincronizacion_RTC)&&Variables_globales.Get_Variable_Global(SD_INSERT)==true)
        {
          Borrado_completado=false;
          Variables_globales.Set_Variable_Global(Libera_Memoria_OK, true);
          Contador_Dias++;

           if(Contador_Dias>31)
          {
            Mes_Limite++;
            Contador_Dias=1;
          }
          /* 6-4*/
          int Eliminar_Mes = month_copy - Mes_Limite;
          int Eliminar_year = year_copy;
          int Limite_Mes=month_copy-3;
          if(Limite_Mes<=0)
          {
            Limite_Mes=Limite_Mes+12;
          }

          if (Eliminar_Mes <=0)
          {
            Eliminar_Mes = Eliminar_Mes + 12;
            Eliminar_year = Eliminar_year - 1;
          }
          
          if(Eliminar_Mes<=0)
          {
            Mes_Limite=4;
            Borrado_completado=true;
          }

          if (Eliminar_Mes != month_copy && Borrado_completado==false&& Eliminar_Mes>0 && Eliminar_Mes<13)
          {

            /*----------------------------------> ESTABLECE NOMBRE DE ARCHIVOS <----------------------------------------------------*/
            String Name_Archivo_Contadores = "Contadores-" + String(Contador_Dias) + String(Eliminar_Mes) + String(Eliminar_year) + ".CSV";
            String Name_Archivo_Eventos = "Eventos-" + String(Contador_Dias) + String(Eliminar_Mes) + String(Eliminar_year) + ".CSV";
            String Name_Archivo_LOG = "Log-" + String(Contador_Dias) + String(Eliminar_Mes) + String(Eliminar_year) + ".TXT";
            String Name_Archivo_Premios="Premios_Maquina-"+String(Contador_Dias)+ String(Eliminar_Mes)+String(Eliminar_year)+".CSV";
            String Name_Archivo_Sesiones="Sesiones_RFID-"+ String(Contador_Dias)+String(Eliminar_Mes)+String(Eliminar_year)+".CSV";

            String Log_Pulsos="Log-Pulsos"+ String(Contador_Dias)+String(Eliminar_Mes)+String(Eliminar_year)+".CSV";

            strcpy(Archivo_CSV_Contadores_copy, Name_Archivo_Contadores.c_str());
            strcpy(Archivo_CSV_Eventos_copy, Name_Archivo_Eventos.c_str());
            strcpy(Archivo_LOG_copy, Name_Archivo_LOG.c_str());
            strcpy(Archivo_CSV_Premios_copy, Name_Archivo_Premios.c_str());
            strcpy(Archivo_CSV_Sesiones_copy, Name_Archivo_Sesiones.c_str());
            /*----------------------------------------------------------------------------------------------------------------------*/
            /* -------------------------------------> Elimina Archivos <------------------------------------------------------------*/
            if (Remove_Archive(Archivo_CSV_Contadores_copy))
            {
              Valida_Archivos_Eliminados++;
            }
            if (Remove_Archive(Archivo_CSV_Eventos_copy))
            {
              Valida_Archivos_Eliminados++;
            }
            if (Remove_Archive(Archivo_LOG_copy))
            {
              Valida_Archivos_Eliminados++;
            }

            if(Remove_Archive(Archivo_CSV_Premios_copy))
            {
              Valida_Archivos_Eliminados++;
            }
            if(Remove_Archive(Archivo_CSV_Sesiones_copy))
            {
              Valida_Archivos_Eliminados++;
            }
            /*---------------------------------------------------------------------------------------------------------------*/
          }

          if (Espacio_Usado_MB <= (Limite_MB / 2)||Espacio_Usado_MB<=(Limite_MB*0.6))
          {
            Variables_globales.Set_Variable_Global(Libera_Memoria_OK, false);
            if(Variables_globales.Get_Variable_Global(Libera_Memoria_OK)==false)
            {
              Borrado_completado=true;
              Mes_Limite = 4;
              Valida_Archivos_Eliminados = 0;
              Contador_Dias = 0;
              #ifdef Debug_Escritura
              Serial.println("Memoria Liberada");
              #endif
              return 0;
            }
            return 0;
          }
          return 1;
        }
      }
      else
      {
       return 0;
      }
    }
    return 0;
}
//--------------------------------------------------------------------------------------------------------
//------------------------------> Función Para Crear Carpetas <-------------------------------------------
void Nueva_Carpeta(char *Carpeta)
{
  SD.mkdir(Carpeta);
}
//--------------------------------------------------------------------------------------------------------
void FLASH_RESET(void)
{
   bool RESET_FLASH = LOW;
    while (digitalRead(FLASH_RESET_Pin) == LOW)
    {
        if (millis() > 10000)
        {
          digitalWrite(WIFI_Status,  LOW);
          Serial.println ("FLASHOK..................");
          for (int i = 0; i < 50; i++)
          {
            nvs_flash_erase();
            nvs_flash_init();
            RESET_FLASH = !RESET_FLASH;
            digitalWrite(MCU_Status,   !RESET_FLASH);
            digitalWrite(SD_Status,    !RESET_FLASH);
            digitalWrite(MCU_Status_2, !RESET_FLASH);
            digitalWrite(WIFI_Status,  !RESET_FLASH);
            delay(100);
          }
          ESP.restart();
        }
    }
    return;
}

void Evento_Formateo_SD(void)
{
  if (Formateo)
  {
    Result_Formatt = FORMAT_IN_PROGRESS;
    if (formatearSD())
    {
      Result_Formatt = FORMAT_SUCCESS; /* Finalizado con Exito!*/
    }
    else
    {
      Result_Formatt = FORMAT_FAILED; /* Finalizado Con falla */
    }
    Formateo = false;
  }
}



// Tabla de polinomio estándar CRC32 (IEEE 802.3)
const uint32_t crcTable[256] PROGMEM = {
  0x00000000L, 0x77073096L, 0xEE0E612CL, 0x990951BAL,
  0x076DC419L, 0x706AF48FL, 0xE963A535L, 0x9E6495A3L,
  0x0EDB8832L, 0x79DCB8A4L, 0xE0D5E91EL, 0x97D2D988L,
  0x09B64C2BL, 0x7EB17CBDL, 0xE7B82D07L, 0x90BF1D91L,
  0x1DB71064L, 0x6AB020F2L, 0xF3B97148L, 0x84BE41DEL,
  0x1ADAD47DL, 0x6DDDE4EBL, 0xF4D4B551L, 0x83D385C7L,
  0x136C9856L, 0x646BA8C0L, 0xFD62F97AL, 0x8A65C9ECL,
  0x14015C4FL, 0x63066CD9L, 0xFA0F3D63L, 0x8D080DF5L,
  0x3B6E20C8L, 0x4C69105EL, 0xD56041E4L, 0xA2677172L,
  0x3C03E4D1L, 0x4B04D447L, 0xD20D85FDL, 0xA50AB56BL,
  0x35B5A8FAL, 0x42B2986CL, 0xDBBBC9D6L, 0xACBCF940L,
  0x32D86CE3L, 0x45DF5C75L, 0xDCD60DCFL, 0xABD13D59L,
  0x26D930ACL, 0x51DE003AL, 0xC8D75180L, 0xBFD06116L,
  0x21B4F4B5L, 0x56B3C423L, 0xCFBA9599L, 0xB8BDA50FL,
  0x2802B89EL, 0x5F058808L, 0xC60CD9B2L, 0xB10BE924L,
  0x2F6F7C87L, 0x58684C11L, 0xC1611DABL, 0xB6662D3DL,
  0x76DC4190L, 0x01DB7106L, 0x98D220BCL, 0xEFD5102AL,
  0x71B18589L, 0x06B6B51FL, 0x9FBFE4A5L, 0xE8B8D433L,
  0x7807C9A2L, 0x0F00F934L, 0x9609A88EL, 0xE10E9818L,
  0x7F6A0DBBL, 0x086D3D2DL, 0x91646C97L, 0xE6635C01L,
  0x6B6B51F4L, 0x1C6C6162L, 0x856530D8L, 0xF262004EL,
  0x6C0695EDL, 0x1B01A57BL, 0x8208F4C1L, 0xF50FC457L,
  0x65B0D9C6L, 0x12B7E950L, 0x8BBEB8EAL, 0xFCB9887CL,
  0x62DD1DDFL, 0x15DA2D49L, 0x8CD37CF3L, 0xFBD44C65L,
  0x4DB26158L, 0x3AB551CEL, 0xA3BC0074L, 0xD4BB30E2L,
  0x4ADFA541L, 0x3DD895D7L, 0xA4D1C46DL, 0xD3D6F4FBL,
  0x4369E96AL, 0x346ED9FCL, 0xAD678846L, 0xDA60B8D0L,
  0x44042D73L, 0x33031DE5L, 0xAA0A4C5FL, 0xDD0D7CC9L,
  0x5005713CL, 0x270241AAL, 0xBE0B1010L, 0xC90C2086L,
  0x5768B525L, 0x206F85B3L, 0xB966D409L, 0xCE61E49FL,
  0x5EDEF90EL, 0x29D9C998L, 0xB0D09822L, 0xC7D7A8B4L,
  0x59B33D17L, 0x2EB40D81L, 0xB7BD5C3BL, 0xC0BA6CADL,
  0xEDB88320L, 0x9ABFB3B6L, 0x03B6E20CL, 0x74B1D29AL,
  0xEAD54739L, 0x9DD277AFL, 0x04DB2615L, 0x73DC1683L,
  0xE3630B12L, 0x94643B84L, 0x0D6D6A3EL, 0x7A6A5AA8L,
  0xE40ECF0BL, 0x9309FF9DL, 0x0A00AE27L, 0x7D079EB1L,
  0xF00F9344L, 0x8708A3D2L, 0x1E01F268L, 0x6906C2FEL,
  0xF762575DL, 0x806567CBL, 0x196C3671L, 0x6E6B06E7L,
  0xFED41B76L, 0x89D32BE0L, 0x10DA7A5AL, 0x67DD4ACCL,
  0xF9B9DF6FL, 0x8EBEEFF9L, 0x17B7BE43L, 0x60B08ED5L,
  0xD6D6A3E8L, 0xA1D1937EL, 0x38D8C2C4L, 0x4FDFF252L,
  0xD1BB67F1L, 0xA6BC5767L, 0x3FB506DDL, 0x48B2364BL,
  0xD80D2BDAL, 0xAF0A1B4CL, 0x36034AF6L, 0x41047A60L,
  0xDF60EFC3L, 0xA867DF55L, 0x316E8EEFL, 0x4669BE79L,
  0xCB61B38CL, 0xBC66831AL, 0x256FD2A0L, 0x5268E236L,
  0xCC0C7795L, 0xBB0B4703L, 0x220216B9L, 0x5505262FL,
  0xC5BA3BBEL, 0xB2BD0B28L, 0x2BB45A92L, 0x5CB36A04L,
  0xC2D7FFA7L, 0xB5D0CF31L, 0x2CD99E8BL, 0x5BDEAE1DL,
  0x9B64C2B0L, 0xEC63F226L, 0x756AA39CL, 0x026D930AL,
  0x9C0906A9L, 0xEB0E363FL, 0x72076785L, 0x05005713L,
  0x95BF4A82L, 0xE2B87A14L, 0x7BB12BAEL, 0x0CB61B38L,
  0x92D28E9BL, 0xE5D5BE0DL, 0x7CDCEFB7L, 0x0BDBDF21L,
  0x86D3D2D4L, 0xF1D4E242L, 0x68DDB3F8L, 0x1FDA836EL,
  0x81BE16CDL, 0xF6B9265BL, 0x6FB077E1L, 0x18B74777L,
  0x88085AE6L, 0xFF0F6A70L, 0x66063BCL, 0x11010B5L,
  0x8F659EFFL, 0xF862AE69L, 0x616BFFD3L, 0x166CCF45L,
  0xA00AE278L, 0xD70DD2EEL, 0x4E048354L, 0x3903B3C2L,
  0xA7672661L, 0xD06016F7L, 0x4969474DL, 0x3E6E77DBL,
  0xAED16A4AL, 0xD9D65ADCL, 0x40DF0B66L, 0x37D83BF0L,
  0xA9BCAE53L, 0xDEBB9EC5L, 0x47B2CF7FL, 0x30B5FFE9L,
  0xBDBDF21CL, 0xCABAC28AL, 0x53B39330L, 0x24B4A3A6L,
  0xBAD03605L, 0xCDD70693L, 0x54DE5729L, 0x23D967BFL,
  0xB3667A2EL, 0xC4614AB8L, 0x5D681B02L, 0x2A6F2B94L,
  0xB40BBE37L, 0xC30C8EA1L, 0x5A05DF1BL, 0x2D02EF8DL
};

// Calcula CRC32 de un buffer
uint32_t crc32_update(uint32_t crc, uint8_t data) {
    return (crc >> 8) ^ pgm_read_dword(&crcTable[(crc ^ data) & 0xFF]);
}

String addCRCToLine(String line)
{
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < line.length(); i++)
  {
    crc = crc32_update(crc, (uint8_t)line[i]);
  }
  crc = crc ^ 0xFFFFFFFF; // invertir bits finales

  char crcStr[20];
  sprintf(crcStr, ",CRC=%08X", crc); // siempre en 8 dígitos hexadecimales
  line += crcStr;
  return line;
}
// Calcula CRC32 de un archivo
uint32_t calculateFileCRC(const char* path) {
    File file = SD.open(path);
    if (!file) return 0;

    uint32_t crc = 0xFFFFFFFF;
    while (file.available()) {
        crc = crc32_update(crc, file.read());
    }
    file.close();

    return crc ^ 0xFFFFFFFF; // Invertido al final
}

bool validateLine(String line)
{
  int pos = line.lastIndexOf(",CRC=");
  if (pos == -1)
  {
    // Es encabezado u otra línea sin CRC → no marcar como error
    return true;
  }

  String dataPart = line.substring(0, pos);
  String crcStr = line.substring(pos + 5);
  uint32_t crcFile = strtoul(crcStr.c_str(), NULL, 16);

  uint32_t crcCalc = 0xFFFFFFFF;
  for (size_t i = 0; i < dataPart.length(); i++)
  {
    crcCalc = crc32_update(crcCalc, (uint8_t)dataPart[i]);


  }
  crcCalc ^= 0xFFFFFFFF;

  Serial.print(crcCalc);
  Serial.print(":");
  Serial.println(crcFile);

  return (crcCalc == crcFile);
}



bool validateFile(const char *path)
{
  bool Output;
  int Counter_Failed = 0;

  Serial.println("Verificando integridad de archivos....⏳");
  File file = SD.open(path);
  if (!file)
  {
    Serial.println("❌ No se pudo abrir el archivo");
    return false;
  }

  int lineNumber = 0;
  while (file.available())
  {
    String line = file.readStringUntil('\n');
    line.trim(); // eliminar saltos o espacios extras
    lineNumber++;

    if (line.length() == 0)
      continue; // saltar líneas vacías

    if (validateLine(line))
    {
      Serial.printf("✅ Línea %d OK\n", lineNumber);
    }
    else
    {
      Counter_Failed++;
      Serial.printf("❌ Línea %d con error de integridad\n", lineNumber);
    }
  }

  file.close();

  if (Counter_Failed > 0)
    return false;
  else
    return true;
}

bool Check_Card(void)
{
  bool IsSuccess = false;

  if (SD.cardType() == CARD_NONE || SD.cardType() == CARD_UNKNOWN)
  {
    Info_Cashless.Log(RTC,"VERIFICACION_MEMORIA_CRITICA","ERROR_DE_FORMATO_DE_TARJETA");
    return IsSuccess;
  }

  Info_Cashless.Log(RTC,"VERIFICACION_MEMORIA_CRITICA","EXITOSA");
  return true;
}

bool Cheack_SD_CriticalMemory(const char *criticalFiles[], int Number_CriticalFile)
{

  bool IsSuccess = true;

  for (int i = 0; i < Number_CriticalFile; i++)
  {
    if (!validateFile(criticalFiles[i]))
    {
      IsSuccess = false;
    }
  }

  return IsSuccess;
}