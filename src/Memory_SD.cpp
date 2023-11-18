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
//------------------------------------------------------------------------------------------------------
//--------------------------------------------> Objetos Locales <-----------------------------------------------

File myFile;                //  Manejo de Archivos.
File file;                  //  Manejo de Archivos En Ftp Mode.
TaskHandle_t SD_CHECK;      //  Manejador de tareas
FtpServer ftpSrv;           //  Objeto servidor FTP
TaskHandle_t Ftp_SERVER;    //  Manejador de tareas
int Contador_Escrituras=0;
int Contador_Dias=0;

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
//------------------------------------------------------------------------------------------------------
extern Buffers Buffer;            // Objeto de buffer de mensajes servidor
//---------------------------------------> Inicializa SD <----------------------------------------------
int Intento_Connect_SD = 0; // Variable Contadora de Intentos de Conexión SD.

#include "Configuracion.h"
extern Configuracion_ESP32 Configuracion;

//--------------------------------------> Bus SPI <-----------------------------------------------------
SPIClass spiRFID(VSPI);

/**********************************************************************************/
/*                              Inicializa Modulo SD                              */
/**********************************************************************************/
void Init_SD(void)
{
  
  spiRFID.begin(18,19,23,SD_ChipSelect);
 // spiRFID.setClockDivider(SPI_CLOCK_DIV128);
  spiRFID.setFrequency(500000);
  if(SD.begin( SD_ChipSelect, spiRFID, 500000))
  {
    Serial.println("Memoria SD Inicializada...");
    Variables_globales.Set_Variable_Global(SD_INSERT,true);
    digitalWrite(SD_Status,HIGH);
  }else
  {
    Serial.println("Memoria SD no insertada...");
    Variables_globales.Set_Variable_Global(SD_INSERT,false);
    digitalWrite(SD_Status,LOW);
  }
}
//------------------------------------------------------------------------------------------------------
//---------------------------------------> Inicializa Servidor FTP <------------------------------------
void Init_FTP_SERVER()
{
  if (Variables_globales.Get_Variable_Global(SD_INSERT) == true && WiFi.status() == WL_CONNECTED)
  {
    RESET_SD();
    ftpSrv.begin("GlobusAmin", "Globussistemas23","SuperGlobusAdmin","SuperG2023");
  }
}
//------------------------------------------------------------------------------------------------------
//---------------------------------> Aquí Tarea Control Servidor FTP <----------------------------------

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
    ftpSrv.handleFTP(); // Verifica Mensajes y Transferencias FTP.
    esp_task_wdt_reset(); /* Reset Timer Lista Larga de archivos*/
    vTaskDelay(10);
}

void Ftp_handle(void)
{
  esp_task_wdt_reset(); /* Reset Timer Lista Larga de archivos*/
  ftpSrv.handleFTP(); // Verifica Mensajes y Transferencias FTP.
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
    myFile = SD.open("/"+String(ARCHIVO), FILE_WRITE); // Si no Existe lo abre
    if (!myFile)
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
      myFile.close();
      Variables_globales.Set_Variable_Global(Fallo_Archivo_LOG,false);
      Variables_globales.Set_Variable_Global(Archivo_CSV_OK, true);
    }
  }
  else if (SD.exists("/"+String(ARCHIVO)))
  {
    #ifdef Debug_Status_SD
    Serial.println("El Archivo Existia.. Continua Guardando en: " + String(ARCHIVO));
    #endif
  //  Variables_globales.Set_Variable_Global(Fallo_Archivo_LOG,false);
  //  Variables_globales.Set_Variable_Global(Archivo_CSV_OK, true);
  }
}
//------------------------------------------------------------------------------------------------------
//-----------------------------> Función Para guardar Eventos En SD <-----------------------------------
void Write_Data_File_Txt(String Datos, char *ARCHIVO)
{
  if(Variables_globales.Get_Variable_Global(SD_INSERT)==1)
  {
    myFile = SD.open("/"+String(ARCHIVO), FILE_APPEND);
    if (!myFile)
    {
      #ifdef Debug_Escritura
      Serial.println("Error al Escribir en Archivo: " + (String)ARCHIVO);
      #endif
      Contador_Escrituras = 0;
    }
    else
    {
      myFile.println(Datos);
      myFile.close();
      #ifdef Debug_Escritura
      Serial.println("Evento Guardado en SD");
      #endif
      Contador_Escrituras++;
    }
  }
}




void LOG_ESP(char *ARCHIVO,bool Enable)
{


  if(Variables_globales.Get_Variable_Global(SD_INSERT)==1)
  {
    if (Enable == true)
    {

      
      File myFile2;
      myFile2 = SD.open("/"+String(ARCHIVO), FILE_APPEND);
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
        myFile2.close();
        #ifdef Debug_Escritura
        Serial.println("LOG Guardado");
        #endif
        Contador_Escrituras++;
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
  if (!SD.exists("/"+String(ARCHIVO))) // Si el archivo no existe lo Crea con encabezado para Excel!!
  {
    myFile = SD.open("/"+String(ARCHIVO), FILE_WRITE);

    if (!myFile)
    {
      #ifdef Debug_Escritura
      Serial.println("No Fue Posible Crear el Archivo");
      #endif
    //  Variables_globales.Set_Variable_Global(Fallo_Archivo_COM,true);
    }
    else
    {
      myFile.println(Encabezado);
      myFile.close();
      #ifdef Debug_Escritura
      Serial.println("Archivo: " + (String)ARCHIVO + " Creado con Encabezado");
      #endif
    //  Variables_globales.Set_Variable_Global(Fallo_Archivo_COM,false);
    //  Variables_globales.Set_Variable_Global(Archivo_CSV_OK, true);
    }
  }
  else if (SD.exists("/"+String(ARCHIVO)))
  {
    #ifdef Debug_Escritura
    Serial.println("El Archivo Existia.. Continua Guardando en: " + (String)ARCHIVO);
    #endif
   // Variables_globales.Set_Variable_Global(Fallo_Archivo_COM,false);
   // Variables_globales.Set_Variable_Global(Archivo_CSV_OK, true);
  }
}
//-----------------------> Funcion para Crear Archivo de eventos con encabezado <------------------------
void Create_ARCHIVE_Excel_Eventos(char *ARCHIVO, String Encabezado)
{
  if (!SD.exists("/"+String(ARCHIVO))) // Si el archivo no existe lo Crea con encabezado para Excel!!
  {
    myFile = SD.open("/"+String(ARCHIVO), FILE_WRITE);

    if (!myFile)
    {
      #ifdef Debug_Escritura
      Serial.println("No Fue Posible Crear el Archivo");
      #endif
     // Variables_globales.Set_Variable_Global(Fallo_Archivo_EVEN,true);
    }
    else
    {
      myFile.println(Encabezado);
      myFile.close();
      #ifdef Debug_Escritura
      Serial.println("Archivo: " + (String)ARCHIVO + " Creado con Encabezado");
      #endif
    //  Variables_globales.Set_Variable_Global(Fallo_Archivo_EVEN,false);
    //  Variables_globales.Set_Variable_Global(Archivo_CSV_OK, true);
    }
  }
  else if (SD.exists("/"+String(ARCHIVO)))
  {
    #ifdef Debug_Escritura
    Serial.println("El Archivo Existia.. Continua Guardando en: " + (String)ARCHIVO + ".csv");
    #endif
  //  Variables_globales.Set_Variable_Global(Fallo_Archivo_EVEN,false);
  //  Variables_globales.Set_Variable_Global(Archivo_CSV_OK, true);
  }
}
//--------------------------------------------------------------------------------------------------------
//---------------------------> Función para Escribir En Archivos CSV <------------------------------------
void Write_Data_File(String Datos, char *ARCHIVO, bool select)
{
  myFile = SD.open("/"+String(ARCHIVO), FILE_WRITE);
  if (select == false)
  {
    if (!myFile)
    {
      #ifdef Debug_Escritura
      Serial.println("Error al escribir en SD");
      #endif
    }
    else
    {
      myFile.print(Datos);
      myFile.print(",");
      #ifdef Debug_Escritura
      Serial.println("Dato: " + Datos + " Guardado en SD");
      #endif
      myFile.close();
    }
  }
  else
  {
    if (!myFile)
    {
      #ifdef Debug_Escritura
      Serial.println("Error al escribir en SD");
      #endif
    }
    else
    {
      myFile.println(Datos);
      #ifdef Debug_Escritura
      Serial.println("Dato: " + Datos + " Guardado en SD");
      #endif
      myFile.close();
    }
  }
}
//--------------------------------------------------------------------------------------------------------
//---------------------------> Función para Escribir En Archivos CSV <------------------------------------

int Cuenta_Fallos=0;
void Write_Data_File2(String Datos, String archivo, bool select, String Encabezado)
{
  if(Variables_globales.Get_Variable_Global(SD_INSERT)==1)
  {
                       
    myFile = SD.open("/"+archivo, FILE_APPEND);
    if (!myFile)
    {
      Cuenta_Fallos++;
      if(Cuenta_Fallos>=2)
      {
        Variables_globales.Set_Variable_Global(Falla_MicroSD,true);
        Cuenta_Fallos=0;
      }
      #ifdef Debug_Escritura
      Serial.println("Error No se pudo Abrir el  Archivo: " + (String)archivo);
      #endif
      Contador_Escrituras = 0;
    }
    else if (myFile) //  else por else if
    {
      digitalWrite(SD_Status,LOW);
      myFile.println(Datos);
      myFile.close();
      #ifdef Debug_Escritura
      Serial.println("Contadores Guardados en SD");
      #endif
      Contador_Escrituras++;
      digitalWrite(SD_Status,HIGH);
    }
    
  }
}
//--------------------------------------------------------------------------------------------------------
//---------------------------> Funcion para  guardar operador  reset handpay <----------------------------
void Storage_Premios_OP(String archivo, bool Enable, byte *Buffer)
{
  String Datos2;
  if (Enable)
  {
    if (Variables_globales.Get_Variable_Global(SD_INSERT) == 1)
    {
      myFile = SD.open("/" + archivo, FILE_APPEND);
      if (!myFile)
      {
#ifdef Debug_Escritura
        Serial.println("Error No se pudo Abrir el  Archivo: " + (String)archivo);
#endif
        Contador_Escrituras = 0;
      }
      else if (myFile) //  else por else if
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
        myFile.println(Datos2);
        myFile.close();

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
void Storage_Cliente(String archivo, bool Enable,byte *Buffer)
{
  String Datos;
  if (Enable)
  {
    if (Variables_globales.Get_Variable_Global(SD_INSERT) == 1)
    {
      
      myFile = SD.open("/" + archivo, FILE_APPEND);
      if (!myFile)
      {
#ifdef Debug_Escritura
        Serial.println("Error No se pudo Abrir el  Archivo: " + (String)archivo);
#endif
        Contador_Escrituras = 0;
      }
      else if (myFile) //  else por else if
      {

        /* Fecha y Cliente */
        Estructura_CSV_Sesiones[0] = RTC.getTime() + ",";

        int BT1=(int)Buffer[0]-48;
        int BT2=(int)Buffer[1]-48;
        int BT3=(int)Buffer[2]-48;
        int BT4=(int)Buffer[3]-48;
        int BT5=(int)Buffer[4]-48;
        int BT6=(int)Buffer[5]-48;
        int BT7=(int)Buffer[6]-48;
        int BT8=(int)Buffer[7]-48;

        Estructura_CSV_Sesiones[1] = String(BT1)+String(BT2)+String(BT3)+String(BT4)+String(BT5)+String(BT6)+String(BT7)+String(BT8);
        Datos=Estructura_CSV_Sesiones[0]+Estructura_CSV_Sesiones[1];
        myFile.println(Datos);
        myFile.close();

        for (int i = 0; i <= Datos.length(); i++)
        {
          Datos.remove(i);
        }
        for (int i = 0; i < 2; i++)
        {
          if(i==1)
          {
               Estructura_CSV_Sesiones[i] = "N/A";
          }else{
               Estructura_CSV_Sesiones[i] = "N/A,";
          }
        }

#ifdef Debug_Escritura
        Serial.println("Cliente Guardado en SD");
#endif
        Contador_Escrituras++;
      }
    }
  }else{

    if (Variables_globales.Get_Variable_Global(SD_INSERT) == 1)
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