/*****************************************************
 *  Autor: Globus Sistemas                           *
 *  Libreria Manejo Storage SD  y Servidor FTP       *                                                   *                                                   *
 ******************************************************/


//----------------------------------------> Variables Globales <----------------------------------------
bool Enable_Status;
unsigned long count = 0;
#define FTP_DEBUG
extern char Archivo_LOG[100];
//------------------------------------------------------------------------------------------------------

//------------------------------------------> Archivos Header <-----------------------------------------
#include <SPI.h>
#include <mySD.h>
#include "Memory_SD.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include "Buffers.h"
//#include "Clase_Variables_Globales.h"
#include "Errores.h"
#include "ESP32Time.h"
#include "time.h"


//------------------------------------------------------------------------------------------------------
//--------------------------------------------> Objetos Locales <-----------------------------------------------

File myFile;                //  Manejo de Archivos.
File file;                  //  Manejo de Archivos En Ftp Mode.
Sd2Card card;               //  Memoria SD.
SdFile root;                //  Listado de Archivos
SdVolume volume;
TaskHandle_t SD_CHECK;      //  Manejador de tareas
FtpServer ftpSrv;           //  Objeto servidor FTP
TaskHandle_t Ftp_SERVER;    //  Manejador de tareas
WiFiServer ftpServer( FTP_CTRL_PORT );
WiFiServer dataServer( FTP_DATA_PORT_PASV );
int Contador_Escrituras=0;
int Contador_Dias=1;
char Archivo_CSV_Contadores_copy[100];
char Archivo_CSV_Eventos_copy[100];
char Archivo_LOG_copy[100];
int Sd_Mont=0;
extern bool Archivos_Ready;
int Valida_Archivos_Eliminados=1;
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

void Init_SD(void)
{
  //Variables_globales.Set_Variable_Global(Ftp_Mode,true);
  //Variables_globales.Set_Variable_Global(Bootloader_Mode,true);
  //Variables_globales.Set_Variable_Global(Bootloader_Mode,true);

  SD.begin(SD_ChipSelect); // Intento Conectar SD 
  xTaskCreatePinnedToCore(
      Task_Verifica_Conexion_SD, //  Función a implementar la tarea.
      "SD_Check",                //  Nombre de la tarea
      10000,                     //  Tamaño de stack en palabras (memoria)
      NULL,                      //  Entrada de parametros
      5,                         //  Prioridad de la tarea.
      &SD_CHECK,                 //  Manejador de la tarea.
      1);                        //  Core donde se ejecutara.
      int T=100;

}
//------------------------------------------------------------------------------------------------------
//---------------------------------------> Inicializa Servidor FTP <------------------------------------
void Init_FTP_SERVER()
{
  xTaskCreatePinnedToCore(
      Rum_FTP_SERVER,             //  Función a implementar la tarea.
      "Ftp_SERVER",               //  Nombre de la tarea.
      10000,                      //  Tamaño de stack en palabras (memoria)
      NULL,                       //  Entrada de parametros.
      configMAX_PRIORITIES - 10,  //  Prioridad de la tarea.
      &Ftp_SERVER,                //  Manejador de la tarea.
      0);                         //  Core donde se ejecutara.
}
//------------------------------------------------------------------------------------------------------
//---------------------------------> Aquí Tarea Control Servidor FTP <----------------------------------
static void Rum_FTP_SERVER(void *parameter)
{
  //SD.open("./");
  RESET_SD();
  ftpSrv.begin("esp32", "esp32"); // Usuario y Contraseña..
  vTaskSuspend(Ftp_SERVER);     //  Tarea Suspendida Control por manager.
  //ftpSrv.begin("esp32", "esp32"); // Usuario y Contraseña..
  unsigned long InicialTime = 0;
  unsigned long FinalTime = 0;
  int Conteo = 0;
  bool SD_State = LOW;
  Serial.println("Modo FTP Activado");
  for (;;)
  {
    Conteo++;
    InicialTime = millis();
    if ((InicialTime - FinalTime) > 100)
    {
      if (Enable_Status == true)
      {
        FinalTime = InicialTime;
        SD_State = !SD_State;
        digitalWrite(SD_Status, !SD_State);
      }
    }
    else if (Conteo == 10)
    {
      Conteo = 0;
    }
    
    ftpSrv.handleFTP(); // Verifica Mensajes y Transferencias FTP.
    vTaskDelay(10);
  }
  vTaskDelay(10);
}

//------------------------------------------------------------------------------------------------------
//---------------------------------> Aquí Tarea para Verifcar Conexión de SD <--------------------------
static void Task_Verifica_Conexion_SD(void *parameter)
{ int Contador=0;
  Serial.println("Verificador de Memoria SD Activado");
  int Intento_Connect_SD = 0; // Variable Contadora de Intentos de Conexión SD.
  bool state_;
  uint32_t chipId = 0;
  long State_volume=0;
  for (;;)
  {
      State_volume++;
  //  state_= SD.begin(SD_ChipSelect);
    //Serial.println(state_);
    bool state_2=volume.init(card);
  //  Serial.println(state_2);
    if (!card.init(SPI_FULL_SPEED,SD_ChipSelect) && !SD.begin())          // SD Desconectada...
    {
      Serial.println("Memoria SD Desconectada..");
                 // Apaga  Indicador LED SD Status.
      Enable_Status = false;// Desactiva Parpadeo de LED SD Status en Modo FTP Server
      Sd_Mont=false;
      if(!card.init(SPI_FULL_SPEED,SD_ChipSelect))
      {
        // Intenta Conectar Despues de Fallo.
        Serial.print("Fallo en Conexión SD"); // Mensaje de Fallo.
        Serial.print(" Intento #: ");         // Mensaje de Fallo.
        Serial.println(Intento_Connect_SD);   // Imprime conteo de Fallos.
        Intento_Connect_SD++;
        digitalWrite(SD_Status, LOW); // Enciende Indicador LED SD Status.
      }
    }
    else 
    {
      char info_MCU_Temp[258];
      bzero(info_MCU_Temp, 258);
      card.readEnd();
      //SD.begin();
      SD.begin(SD_ChipSelect);
      Intento_Connect_SD = 0;        // Reset Contador de Fallos.
      Serial.println("SD OK");       // Mensaje de Conexión SD.
      digitalWrite(SD_Status, HIGH); // Enciende Indicador LED SD Status.
      Enable_Status = true;          // Habilita  El Parpadeo de LED SD Status en Modo FTP Server
      Sd_Mont=true;
     // log_e("Error Inicializando SD: ", ERROR_INICIALIZANDO_SD);
     // LOG_ESP(Archivo_LOG,Variables_globales.Get_Variable_Global(Enable_Storage));
      if(Contador_Escrituras>0 && Variables_globales.Get_Variable_Global(Enable_Storage)==true)
      {
        Variables_globales.Set_Variable_Global(Estado_Escritura,true);
      }else
      {
        Variables_globales.Set_Variable_Global(Estado_Escritura,false);
      }
     // Serial.println("Memoria SD Conectada");
      if(Variables_globales.Get_Variable_Global(Ftp_Mode)==false)
      {
        volume.init(card);
        FreeSpace_SD();
      }
      
      
      uint8_t Temperatura_Procesador= temperatureRead();
      
     // Serial.print("Temperatura de Procesador: "+String(Temperatura_Procesador));
    //  Serial.println(" C");
      
      for (int i = 0; i < 17; i = i + 8)
      {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
      }
      
    //  Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    //  Serial.printf("This chip has %d cores\n", ESP.getChipCores());
    //  Serial.print("Chip ID: ");
    //  Serial.println(chipId);
      //info_MCU_Temp[0]=char(ESP.getChipModel());     //Modelo chip
      info_MCU_Temp[1]=ESP.getChipCores();          // Cores de Procesador
      info_MCU_Temp[2]=chipId;                     // ID chip
      info_MCU_Temp[3]=Temperatura_Procesador;    //Temperatura °C
      Buffer.Set_buffer_info_MCU(info_MCU_Temp); //Guarda Buffer de Datos;

    }
    vTaskDelay(10000); // Pausa Tarea 10000ms
  }
  vTaskDelay(10);
}
//------------------------------------------------------------------------------------------------------
//---------------------> Función Para Crear Archivos Txt sin Formato <---------------------------------
void FreeSpace_SD(void)
{
  char info_MicroSD_Temp[258];
  bzero(info_MicroSD_Temp, 258);
  Serial.println("Información de Memoria SD ");
  Serial.println();
  /*
  Serial.print("Card type:         ");
  switch (card.type())
  {
  case SD_CARD_TYPE_SD1:
    Serial.println("SD1");
    break;
  case SD_CARD_TYPE_SD2:
    Serial.println("SD2");
    break;
  case SD_CARD_TYPE_SDHC:
    Serial.println("SDHC");
    break;
  default:
    Serial.println("Unknown");
  }
  */
  if (Variables_globales.Get_Variable_Global(Estado_Escritura))
  {
    Serial.println("Estado: Escritura");
  }
  else
  {
    if (Variables_globales.Get_Variable_Global(Ftp_Mode))
    {
      Serial.println("Estado: FTP");
    }
    else
    {
      Serial.println("Estado: Deshabilitada");
    }
  }
  
  // Serial.print(F("Card Size MB:   "));
  // Serial.println(0.000512 * card.cardSize()); // size Memory
  float clusterSize = 0.000512 * volume.blocksPerCluster();

  // Serial.print(F("Total space MB: ")); // size Total Memory
  // Serial.println(clusterSize * volume.clusterCount());
  // Serial.print(F("Free Space MB:  ")); // size Disponible.
  // Serial.println(clusterSize * volume.freeClusterCount());
  Libera_Memoria(clusterSize * volume.clusterCount(), clusterSize * volume.freeClusterCount());

  info_MicroSD_Temp[0]=card.type(); // Tipo de Memoria
  info_MicroSD_Temp[1]=clusterSize * volume.clusterCount(); // Total Memoria
  info_MicroSD_Temp[2]=clusterSize * volume.freeClusterCount(); //  Espacio libre
  info_MicroSD_Temp[3]=Variables_globales.Get_Variable_Global(Estado_Escritura); // Estado Escritura
  info_MicroSD_Temp[4]=Variables_globales.Get_Variable_Global(Ftp_Mode); // Modo FTP
  Buffer.Set_buffer_info_MicroSD(info_MicroSD_Temp); // Guarda Buffer.
}

void Create_ARCHIVE_Txt(char *ARCHIVO)
{
  if (!SD.exists(ARCHIVO)) // Pregunta si el archivo Existe.
  {
    myFile = SD.open(ARCHIVO, FILE_WRITE); // Abre Archivo.
    if (!myFile)
    {
      Serial.println("No se pudo Crear Archivo LOG");
      Variables_globales.Set_Variable_Global(Fallo_Archivo_LOG,true);
    }
    else
    {
      Serial.println("Archivo LOG Creado: " + String(ARCHIVO));
      myFile.close();
      Variables_globales.Set_Variable_Global(Fallo_Archivo_LOG,false);
      Variables_globales.Set_Variable_Global(Archivo_CSV_OK, true);
    }
  }
  else if (SD.exists(ARCHIVO))
  {
    Serial.println("El Archivo Existia.. Continua Guardando en: " + String(ARCHIVO));
    Variables_globales.Set_Variable_Global(Fallo_Archivo_LOG,false);
    Variables_globales.Set_Variable_Global(Archivo_CSV_OK, true);
  }
}
//------------------------------------------------------------------------------------------------------
//-----------------------------> Función Para guardar Eventos En SD <-----------------------------------
void Write_Data_File_Txt(String Datos, char *ARCHIVO)
{
  if(Sd_Mont==1)
  {
    myFile = SD.open(ARCHIVO, FILE_WRITE);
    if (!myFile)
    {
      Serial.println("Error al Escribir en Archivo: " + (String)ARCHIVO);
      Contador_Escrituras = 0;
    }
    else
    {
      myFile.println(Datos);
      myFile.close();
      Serial.println("Evento Guardado en SD");
      Contador_Escrituras++;
    }
  }
}

void LOG_ESP(char *ARCHIVO,bool Enable)
{
  if(Sd_Mont==1)
  {
    if (Enable == true)
    {
      myFile = SD.open(ARCHIVO, FILE_WRITE);
      if (!myFile)
      {
        Serial.println("Error al Escribir en Archivo: " + (String)ARCHIVO);
        Contador_Escrituras = 0;
      }
      else
      {
        String Datos = String(Fallo);
        myFile.println(RTC.getTime() + " Error: " + Datos);
        myFile.close();
        Serial.println("LOG Guardado");
        Contador_Escrituras++;
      }
    }
    else
    {
      Serial.println("Guardado Deshabilitado");
    }
  }
}

//-------------------------------------------------------------------------------------------------------
//---------------------------> Función Para Guardar Contadores y Eventos en SD <-------------------------
void Create_ARCHIVE_Excel(char *ARCHIVO, String Encabezado)
{
  if (!SD.exists(ARCHIVO)) // Si el archivo no existe lo Crea con encabezado para Excel!!
  {
    myFile = SD.open(ARCHIVO, FILE_WRITE);

    if (!myFile)
    {
      Serial.println("No Fue Posible Crear el Archivo");
      Variables_globales.Set_Variable_Global(Fallo_Archivo_COM,true);
    }
    else
    {
      myFile.println(Encabezado);
      myFile.close();
      Serial.println("Archivo: " + (String)ARCHIVO + " Creado con Encabezado");
      Variables_globales.Set_Variable_Global(Fallo_Archivo_COM,false);
      Variables_globales.Set_Variable_Global(Archivo_CSV_OK, true);
    }
  }
  else if (SD.exists(ARCHIVO))
  {
    Serial.println("El Archivo Existia.. Continua Guardando en: " + (String)ARCHIVO + ".csv");
     Variables_globales.Set_Variable_Global(Fallo_Archivo_COM,false);
    Variables_globales.Set_Variable_Global(Archivo_CSV_OK, true);
  }
}
void Create_ARCHIVE_Excel_Eventos(char *ARCHIVO, String Encabezado)
{
  if (!SD.exists(ARCHIVO)) // Si el archivo no existe lo Crea con encabezado para Excel!!
  {
    myFile = SD.open(ARCHIVO, FILE_WRITE);

    if (!myFile)
    {
      Serial.println("No Fue Posible Crear el Archivo");
      Variables_globales.Set_Variable_Global(Fallo_Archivo_EVEN,true);
    }
    else
    {
      myFile.println(Encabezado);
      myFile.close();
      Serial.println("Archivo: " + (String)ARCHIVO + " Creado con Encabezado");
      Variables_globales.Set_Variable_Global(Fallo_Archivo_EVEN,false);
      Variables_globales.Set_Variable_Global(Archivo_CSV_OK, true);
    }
  }
  else if (SD.exists(ARCHIVO))
  {
    Serial.println("El Archivo Existia.. Continua Guardando en: " + (String)ARCHIVO + ".csv");
    Variables_globales.Set_Variable_Global(Fallo_Archivo_EVEN,false);
    Variables_globales.Set_Variable_Global(Archivo_CSV_OK, true);
  }
}
//--------------------------------------------------------------------------------------------------------
//---------------------------> Función para Escribir En Archivos CSV <------------------------------------
void Write_Data_File(String Datos, char *ARCHIVO, bool select)
{
  myFile = SD.open(ARCHIVO, FILE_WRITE);
  if (select == false)
  {
    if (!myFile)
    {
      Serial.println("Error al escribir en SD");
    }
    else
    {
      myFile.print(Datos);
      myFile.print(",");
      Serial.println("Dato: " + Datos + " Guardado en SD");
      myFile.close();
    }
  }
  else
  {
    if (!myFile)
    {
      Serial.println("Error al escribir en SD");
    }
    else
    {
      myFile.println(Datos);
      Serial.println("Dato: " + Datos + " Guardado en SD");
      myFile.close();
    }
  }
}
//--------------------------------------------------------------------------------------------------------
//---------------------------> Función para Escribir En Archivos CSV <------------------------------------
void Write_Data_File2(String Datos, char *archivo, bool select, String Encabezado)
{
  if(Sd_Mont==1)
  {
    myFile = SD.open(archivo, FILE_WRITE);
    if (!myFile)
    {
      Serial.println("Error No se pudo Abrir el  Archivo: " + (String)archivo);
      Contador_Escrituras = 0;
    }
    else if (myFile) //  else por else if
    {
      myFile.println(Datos);

      // myFile.println();
      myFile.close();
      Serial.println("Contadores Guardados en SD");
      Contador_Escrituras++;
    }
  }
}
//--------------------------------------------------------------------------------------------------------
//-----------------------------------> Función Para Reset SD <--------------------------------------------
void RESET_SD(void)
{
  SD.end();
  SD.begin(SD_ChipSelect); // Intento Conectar SD
}
//--------------------------------------------------------------------------------------------------------

//------------------------------> Función Lectura de Archivo Formato String <-----------------------------
void Read_File(const char *ARCHIVO)
{
  myFile = SD.open(ARCHIVO);
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
    Serial.print("Error en lectura de archivo ");
    Serial.println(ARCHIVO);
  }
}
//--------------------------------------------------------------------------------------------------------

//------------------------------> Función Para Borrar Archivos <------------------------------------------
void Remove_Archive(char *ARCHIVO)
{
  if (SD.exists(ARCHIVO))
  {
    SD.remove(ARCHIVO);
    Serial.print("\nSe ha Eliminado ");
    Serial.print(ARCHIVO);
    Serial.println(" Exitosamente.");
  }
  else
  {
    Serial.println("\nNo existe archivo: " + (String)ARCHIVO);
  }
}

void Libera_Memoria(float Total_Memoria_MB, float Free_Space)
{
    
    float Limite_MB = Total_Memoria_MB * 0.8;
    float Espacio_Usado_MB = Total_Memoria_MB - Free_Space;
    if(Espacio_Usado_MB<0 ||Espacio_Usado_MB>Total_Memoria_MB+100.0 || Free_Space<0 || Free_Space>Total_Memoria_MB+100.0)
    {
      Serial.println("Error en Volumen de Memoria");
    }
    else
    {
      Serial.println("Memoria Usada MB:" + String(Espacio_Usado_MB));
      if (Espacio_Usado_MB >= Limite_MB || Variables_globales.Get_Variable_Global(Libera_Memoria_OK) == true) // 80%  del espacio de Memoria Utilizada.
      {
        Variables_globales.Set_Variable_Global(Libera_Memoria_OK, true);
        Contador_Dias++;
        int Eliminar_Mes = month_copy - 4;
        int Eliminar_year = year_copy;
        if (Eliminar_Mes < 0)
        {
          Eliminar_Mes = Eliminar_Mes + 12;
          Eliminar_year = Eliminar_year - 1;
        }
        if (Eliminar_Mes == 0)
        {
          Eliminar_Mes = 1;
        }
        String Name_Archivo_Contadores = String(Contador_Dias) + String(Eliminar_Mes) + String(Eliminar_year) + ".CSV";
        String Name_Archivo_Eventos = String(Contador_Dias) + String(Eliminar_Mes) + String(Eliminar_year) + "1" + ".CSV";
        String Name_Archivo_LOG = String(Contador_Dias) + String(Eliminar_Mes) + String(Eliminar_year) + ".TXT";
        strcpy(Archivo_CSV_Contadores_copy, Name_Archivo_Contadores.c_str());
        strcpy(Archivo_CSV_Eventos_copy, Name_Archivo_Eventos.c_str());
        strcpy(Archivo_LOG_copy, Name_Archivo_LOG.c_str());

        if (SD.exists(Archivo_CSV_Contadores_copy))
        {
          SD.remove(Archivo_CSV_Contadores_copy);
          Serial.print("\nSe ha Eliminado ");
          Serial.print(Archivo_CSV_Contadores_copy);
          Serial.println(" Exitosamente.");
          if(!SD.exists(Archivo_CSV_Contadores_copy))
          {
            Valida_Archivos_Eliminados++;
          }
        }
        if (SD.exists(Archivo_CSV_Eventos_copy))
        {
          SD.remove(Archivo_CSV_Eventos_copy);
          Serial.print("\nSe ha Eliminado ");
          Serial.print(Archivo_CSV_Eventos_copy);
          Serial.println(" Exitosamente.");
          if(!SD.exists(Archivo_CSV_Eventos_copy))
          {
            Valida_Archivos_Eliminados++;
          }
        }
        if (SD.exists(Archivo_LOG_copy))
        {
          SD.remove(Archivo_LOG_copy);
          Serial.print("\nSe ha Eliminado ");
          Serial.print(Archivo_LOG_copy);
          Serial.println(" Exitosamente.");
          if(!SD.exists(Archivo_LOG_copy))
          {
            Valida_Archivos_Eliminados++;
          }
        }

        if (Contador_Dias == 30)
        {
          Contador_Dias = 1;
          if(Valida_Archivos_Eliminados==90)
          {
            Valida_Archivos_Eliminados=1;
            Variables_globales.Set_Variable_Global(Libera_Memoria_OK, false);
            Serial.println("Delete 30 Archivos");
          }     
        }
      }
      else
      {
        Serial.println("Memoria Libre: " + String(Free_Space));
      }
    }
}
//--------------------------------------------------------------------------------------------------------

//------------------------------> Función Para Crear Carpetas <-------------------------------------------
void Nueva_Carpeta(char *Carpeta)
{
  SD.mkdir(Carpeta);
}
//--------------------------------------------------------------------------------------------------------

FtpServer::FtpServer()
{
}

void FtpServer::begin(String uname, String pword)
{
  // Tells the ftp server to begin listening for incoming connection
  _FTP_USER = uname;
  _FTP_PASS = pword;

  /*
     // print the type of card
   Serial.print("\nCard type: ");
   switch(card.type()) {
     case SD_CARD_TYPE_SD1:
       Serial.println("SD1");
       break;
     case SD_CARD_TYPE_SD2:
       Serial.println("SD2");
       break;
     case SD_CARD_TYPE_SDHC:
       Serial.println("SDHC");
       break;
     default:
       Serial.println("Unknown");
   }
 */

  ftpServer.begin();
  delay(10);
  dataServer.begin();
  delay(10);
  millisTimeOut = (uint32_t)FTP_TIME_OUT * 60 * 1000;
  millisDelay = 0;
  cmdStatus = 0;
  iniVariables();
}

void FtpServer::iniVariables()
{
  // Default for data port
  dataPort = FTP_DATA_PORT_PASV;
  // Default Data connection is Active
  dataPassiveConn = true;
  // Set the root directory
  strcpy(cwdName, "/");
  rnfrCmd = false;
  transferStatus = 0;
}

int FtpServer::handleFTP()
{
  if ((int32_t)(millisDelay - millis()) > 0)
    return 0;

  if (ftpServer.hasClient())
  {
    //  if (ftpServer.available()) {
    client.stop();
    client = ftpServer.available();
  }

  if (cmdStatus == 0)
  {
    if (client.connected())
      disconnectClient();
    cmdStatus = 1;
  }
  else if (cmdStatus == 1) // Ftp server waiting for connection
  {
    abortTransfer();
    iniVariables();
#ifdef FTP_DEBUG
    Serial.println("Ftp server waiting for connection on port " + String(FTP_CTRL_PORT));
#endif
    cmdStatus = 2;
  }
  else if (cmdStatus == 2) // Ftp server idle
  {

    if (client.connected()) // A client connected
    {
      clientConnected();
      millisEndConnection = millis() + 10 * 1000; // wait client id during 10 s.
      cmdStatus = 3;
    }
  }
  else if (readChar() > 0) // got response
  {
    if (cmdStatus == 3) // Ftp server waiting for user identity
      if (userIdentity())
        cmdStatus = 4;
      else
        cmdStatus = 0;
    else if (cmdStatus == 4) // Ftp server waiting for user registration
      if (userPassword())
      {
        cmdStatus = 5;
        millisEndConnection = millis() + millisTimeOut;
      }
      else
        cmdStatus = 0;
    else if (cmdStatus == 5) // Ftp server waiting for user command
    {
      if (!processCommand())
        cmdStatus = 0;
      else
        millisEndConnection = millis() + millisTimeOut;
    }
  }
  else if (!client.connected() || !client)
  {
    cmdStatus = 1;
#ifdef FTP_DEBUG
    Serial.println("client disconnected");
    //---->>> add 
  Variables_globales.Set_Variable_Global(Ftp_Mode,false);
  if(!Variables_globales.Get_Variable_Global(Ftp_Mode))
  {
    vTaskSuspend(Ftp_SERVER); //  Suspende Modo FTP
    RESET_SD(); // Reset SD para Modo Storage Data
    if(Variables_globales.Get_Variable_Global(Fallo_Archivo_COM) == false || Variables_globales.Get_Variable_Global(Fallo_Archivo_EVEN)== false || Variables_globales.Get_Variable_Global(Fallo_Archivo_LOG) == false)
    {
      if(Variables_globales.Get_Variable_Global(Sincronizacion_RTC)==true)
      {
        Variables_globales.Set_Variable_Global(Enable_Storage, true);
      }
    }
  }
#endif
  }

  if (transferStatus == 1) // Retrieve data
  {
    if (!doRetrieve())
      transferStatus = 0;
  }
  else if (transferStatus == 2) // Store data
  {
    if (!doStore())
      transferStatus = 0;
  }
  else if (cmdStatus > 2 && !((int32_t)(millisEndConnection - millis()) > 0))
  {
    client.println("530 Timeout");
    millisDelay = millis() + 200; // delay of 200 ms
    cmdStatus = 0;
  }

  return transferStatus != 0 || cmdStatus != 0;
}

void FtpServer::clientConnected()
{
#ifdef FTP_DEBUG
  Serial.println("Client connected!");
  
#endif
  client.println("FTP SERVER");
  client.println("Globus IM ESP32");
  client.println("220 --   Version " + String(FTP_SERVER_VERSION) + "   --");
  iCL = 0;
}

void FtpServer::disconnectClient()
{
#ifdef FTP_DEBUG
  Serial.println(" Disconnecting client");
#endif
  abortTransfer();
  client.println("221 Goodbye");
  client.stop();

  //---->>> add 
  Variables_globales.Set_Variable_Global(Ftp_Mode,false);
  if(!Variables_globales.Get_Variable_Global(Ftp_Mode))
  {
    vTaskSuspend(Ftp_SERVER); //  Suspende Modo FTP
    RESET_SD(); // Reset SD para Modo Storage Data
    if(Variables_globales.Get_Variable_Global(Fallo_Archivo_COM) == false || Variables_globales.Get_Variable_Global(Fallo_Archivo_EVEN)== false || Variables_globales.Get_Variable_Global(Fallo_Archivo_LOG) == false)
    {
      if(Variables_globales.Get_Variable_Global(Sincronizacion_RTC)==true)
      {
        Variables_globales.Set_Variable_Global(Enable_Storage, true);
      }
    }
  }
}

boolean FtpServer::userIdentity()
{
  if (strcmp(command, "USER"))
    client.println("500 Syntax error");
  if (strcmp(parameters, _FTP_USER.c_str()))
    client.println("530 user not found");
  else
  {
    client.println("331 OK. Password required");
    strcpy(cwdName, "/");
    return true;
  }
  millisDelay = millis() + 100; // delay of 100 ms
  return false;
}

boolean FtpServer::userPassword()
{
  if (strcmp(command, "PASS"))
    client.println("500 Syntax error");
  else if (strcmp(parameters, _FTP_PASS.c_str()))
    client.println("530 ");
  else
  {
#ifdef FTP_DEBUG
    Serial.println("OK. Waiting for commands.");
#endif
    client.println("230 OK.");
    return true;
  }
  millisDelay = millis() + 100; // delay of 100 ms
  return false;
}

boolean FtpServer::processCommand()
{
  ///////////////////////////////////////
  //                                   //
  //      ACCESS CONTROL COMMANDS      //
  //                                   //
  ///////////////////////////////////////

  //
  //  CDUP - Change to Parent Directory
  //
  if (!strcmp(command, "CDUP"))
  {
    int todo;
    client.println("250 Ok. Current directory is \"" + String(cwdName) + "\"");
  }
  //
  //  CWD - Change Working Directory
  //
  else if (!strcmp(command, "CWD"))
  {
    if (strcmp(parameters, ".") == 0) // 'CWD .' is the same as PWD command
      client.println("257 \"" + String(cwdName) + "\" is your current directory");
    else
    {

#ifdef FTP_DEBUG
      Serial.print("CWD P=");
      Serial.print(parameters);
      Serial.print(" CWD=");
      Serial.println(cwdName);
#endif
      String dir;

      if (parameters[0] == '/')
      {
        dir = parameters;
      }
      else if (!strcmp(cwdName, "/")) // avoid "\\newdir"
      {
        dir = String("/") + parameters;
      }
      else
      {
        dir = String(cwdName) + "/" + parameters;
      }

      if (!SD.exists((char *)(dir.c_str())))
      {
        strcpy(cwdName, dir.c_str());
        client.println("250 CWD Ok. Current directory is \"" + String(dir) + "\"");
        Serial.println("250 CWD Ok. Current directory is \"" + String(dir) + "\"");
      }
      else
      {
        client.println("550 directory or file does not exist \"" + String(parameters) + "\"");
        Serial.println("550 directory or file does not exist \"" + String(parameters) + "\"");
      }
    }
  }
  //
  //  PWD - Print Directory
  //
  else if (!strcmp(command, "PWD"))
    client.println("257 \"" + String(cwdName) + "\" is your current directory");
  //
  //  QUIT
  //
  else if (!strcmp(command, "QUIT"))
  {
    disconnectClient();
    return false;
  }

  ///////////////////////////////////////
  //                                   //
  //    TRANSFER PARAMETER COMMANDS    //
  //                                   //
  ///////////////////////////////////////

  //
  //  MODE - Transfer Mode
  //
  else if (!strcmp(command, "MODE"))
  {
    if (!strcmp(parameters, "S"))
      client.println("200 S Ok");
    // else if( ! strcmp( parameters, "B" ))
    //  client.println( "200 B Ok\r\n";
    else
      client.println("504 Only S(tream) is suported");
  }
  //
  //  PASV - Passive Connection management
  //
  
  else if (!strcmp(command, "PASV"))
  {
    if (data.connected())
      data.stop();
    // dataServer.begin();
    // dataIp = Ethernet.localIP();
    dataIp = WiFi.localIP();
    dataPort = FTP_DATA_PORT_PASV;
// data.connect( dataIp, dataPort );
// data = dataServer.available();
#ifdef FTP_DEBUG
    Serial.println("Connection management set to passive");
    Serial.println("Data port set to " + String(dataPort));
#endif
    client.println("227 Entering Passive Mode (" + String(dataIp[0]) + "," + String(dataIp[1]) + "," + String(dataIp[2]) + "," + String(dataIp[3]) + "," + String(dataPort >> 8) + "," + String(dataPort & 255) + ").");
    dataPassiveConn = true;
  }
  //
  //  PORT - Data Port
  //
  else if (!strcmp(command, "PORT"))
  {
    if (data)
      data.stop();
    // get IP of data client
    dataIp[0] = atoi(parameters);
    char *p = strchr(parameters, ',');
    for (uint8_t i = 1; i < 4; i++)
    {
      dataIp[i] = atoi(++p);
      p = strchr(p, ',');
    }
    // get port of data client
    dataPort = 256 * atoi(++p);
    p = strchr(p, ',');
    dataPort += atoi(++p);
    if (p == NULL)
      client.println("501 Can't interpret parameters");
    else
    {

      client.println("200 PORT command successful");
      dataPassiveConn = false;
    }
  }
  //
  //  STRU - File Structure
  //
  else if (!strcmp(command, "STRU"))
  {
    if (!strcmp(parameters, "F"))
      client.println("200 F Ok");
    // else if( ! strcmp( parameters, "R" ))
    //  client.println( "200 B Ok\r\n";
    else
      client.println("504 Only F(ile) is suported");
  }
  //
  //  TYPE - Data Type
  //
  else if (!strcmp(command, "TYPE"))
  {
    if (!strcmp(parameters, "A"))
      client.println("200 TYPE is now ASII");
    else if (!strcmp(parameters, "I"))
      client.println("200 TYPE is now 8-bit binary");
    else
      client.println("504 Unknow TYPE");
  }

  ///////////////////////////////////////
  //                                   //
  //        FTP SERVICE COMMANDS       //
  //                                   //
  ///////////////////////////////////////

  //
  //  ABOR - Abort
  //
  else if (!strcmp(command, "ABOR"))
  {
    abortTransfer();
    client.println("226 Data connection closed");
  }
  //
  //  DELE - Delete a File
  //
  else if (!strcmp(command, "DELE"))
  {
    char path[FTP_CWD_SIZE];
    if (strlen(parameters) == 0)
      client.println("501 No file name");
    else if (makePath(path))
    {
      if (!SD.exists(path))
        client.println("550 File " + String(parameters) + " not found");
      else
      {
        if (SD.remove(path))
          client.println("250 Deleted " + String(parameters));
        else
          client.println("450 Can't delete " + String(parameters));
      }
    }
  }
  //
  //  LIST - List
  //
  else if (!strcmp(command, "LIST"))
  {
    if (!dataConnect())
      client.println("425 No data connection");
    else
    {
      client.println("150 Accepted data connection");
      uint16_t nm = 0;
      //      Dir dir=SD.openDir(cwdName);
      File dir = SD.open(cwdName);
      //      if( !SD.exists(cwdName))
      if ((!dir) || (!dir.isDirectory()))
        client.println("550 Can't open directory " + String(cwdName));
      else
      {
        File file = dir.openNextFile();
        while (file)
        {
          String fn, fs;
          fn = file.name();
// fn.remove(0, 1);
#ifdef FTP_DEBUG
          Serial.println("File Name = " + fn);
#endif
          fs = String(file.size());
          if (file.isDirectory())
          {
            data.println("01-01-2000  00:00AM <DIR> " + fn);
          }
          else
          {
            data.println("01-01-2000  00:00AM " + fs + " " + fn);
            //          data.println( " " + fn );
          }
          nm++;
          file = dir.openNextFile();
        }
        client.println("226 " + String(nm) + " matches total");
      }
      data.stop();
    }
  }
  //
  //  MLSD - Listing for Machine Processing (see RFC 3659)
  //
  else if (!strcmp(command, "MLSD"))
  {
    if (!dataConnect())
    {
      client.println("425 No data connection MLSD");
    }
    else
    {
      client.println("150 Accepted data connection");
      uint16_t nm = 0;
      //      Dir dir= SD.openDir(cwdName);
      File dir = SD.open(cwdName);
      // char dtStr[ 15 ];
      //  if(!SD.exists(cwdName))
      if ((!dir) || (!dir.isDirectory()))
        client.println("550 Can't open directory " + String(cwdName));
      //        client.println( "550 Can't open directory " +String(parameters) );
      else
      {
        //        while( dir.next())
        File file = dir.openNextFile();
        //        while( dir.openNextFile())
        while (file)
        {
          String fn, fs;
          fn = file.name();
          //          Serial.println(fn);
          fn.remove(0, strlen(cwdName));
          if (fn[0] == '/')
            fn.remove(0, 1);
          fs = String(file.size());
          if (file.isDirectory())
          {
            data.println("Type=dir;Size=" + fs + ";" + "modify=20000101000000;" + " " + fn);
            //            data.println( "Type=dir;modify=20000101000000; " + fn);
          }
          else
          {
            // data.println( "Type=file;Size=" + fs + ";"+"modify=20000101160656;" +" " + fn);
            data.println("Type=file;Size=" + fs + ";" + "modify=20000101000000;" + " " + fn);
          }
          nm++;
          file = dir.openNextFile();
        }
        client.println("226-options: -a -l");
        client.println("226 " + String(nm) + " matches total");
      }
      data.stop();
    }
  }
  //
  //  NLST - Name List
  //
  else if (!strcmp(command, "NLST"))
  {
    if (!dataConnect())
      client.println("425 No data connection");
    else
    {
      client.println("150 Accepted data connection");
      uint16_t nm = 0;
      //      Dir dir=SD.openDir(cwdName);
      File dir = SD.open(cwdName);
      if (!SD.exists(cwdName))
        client.println("550 Can't open directory " + String(parameters));
      else
      {
        File file = dir.openNextFile();
        //        while( dir.next())
        while (file)
        {
          //          data.println( dir.fileName());
          data.println(file.name());
          nm++;
          file = dir.openNextFile();
        }
        client.println("226 " + String(nm) + " matches total");
      }
      data.stop();
    }
  }
  //
  //  NOOP
  //
  else if (!strcmp(command, "NOOP"))
  {
    // dataPort = 0;
    client.println("200 Zzz...");
  }
  //
  //  RETR - Retrieve
  //
  else if (!strcmp(command, "RETR"))
  {
    char path[FTP_CWD_SIZE];
    if (strlen(parameters) == 0)
      client.println("501 No file name");
    else if (makePath(path))
    {
      file = SD.open(path, FILE_READ);
      if (!file)
        client.println("550 File " + String(parameters) + " not found");
      else if (!file)
        client.println("450 Can't open " + String(parameters));
      else if (!dataConnect())
        client.println("425 No data connection");
      else
      {
#ifdef FTP_DEBUG
        Serial.println("Sending " + String(parameters));
#endif
        client.println("150-Connected to port " + String(dataPort));
        client.println("150 " + String(file.size()) + " bytes to download");
        millisBeginTrans = millis();
        bytesTransfered = 0;
        transferStatus = 1;
      }
    }
  }
  //
  //  STOR - Store
  //
  else if (!strcmp(command, "STOR"))
  {
    char path[FTP_CWD_SIZE];
    if (strlen(parameters) == 0)
      client.println("501 No file name");
    else if (makePath(path))
    {
      file = SD.open(path, FILE_WRITE);
      if (!file)
        client.println("451 Can't open/create " + String(parameters));
      else if (!dataConnect())
      {
        client.println("425 No data connection");
        file.close();
      }
      else
      {
#ifdef FTP_DEBUG
        Serial.println("Receiving " + String(parameters));
#endif
        client.println("150 Connected to port " + String(dataPort));
        millisBeginTrans = millis();
        bytesTransfered = 0;
        transferStatus = 2;
      }
    }
  }
  //
  //  MKD - Make Directory
  //
  else if (!strcmp(command, "MKD"))
  {
#ifdef FTP_DEBUG
    Serial.print("MKD P=");
    Serial.print(parameters);
    Serial.print(" CWD=");
    Serial.println(cwdName);
#endif
    String dir;

    if (!strcmp(cwdName, "/")) // avoid "\\newdir"
    {
      dir = String("/") + parameters;
    }
    else
    {
      dir = String(cwdName) + "/" + parameters;
    }

#ifdef FTP_DEBUG
    Serial.print("try to create  ");
    Serial.println(dir);
#endif

    if (SD.mkdir((char *)(dir.c_str())))
    {
      client.println("257 \"" + String(parameters) + "\" - Directory successfully created");
    }
    else
    {
      client.println("502 Can't create \"" + String(parameters));
    }
  }
  //
  //  RMD - Remove a Directory
  //
  else if (!strcmp(command, "RMD"))
  {
#ifdef FTP_DEBUG
    Serial.print("RMD ");
    Serial.print(parameters);
    Serial.print(" CWD=");
    Serial.println(cwdName);
#endif
    String dir;

    if (!strcmp(cwdName, "/")) // avoid "\\newdir"
    {
      dir = String("/") + parameters;
    }
    else
    {
      dir = String(cwdName) + "/" + parameters;
    }

    if (SD.rmdir((char *)(dir.c_str())))
    {
      client.println("250 RMD command successful");
    }
    else
    {
      client.println("502 Can't delete \"" + String(parameters)); // not support on espyet
    }
  }
  //
  //  RNFR - Rename From
  //
  else if (!strcmp(command, "RNFR"))
  {
    buf[0] = 0;
    if (strlen(parameters) == 0)
      client.println("501 No file name");
    else if (makePath(buf))
    {
      if (!SD.exists(buf))
        client.println("550 File " + String(parameters) + " not found");
      else
      {
#ifdef FTP_DEBUG
        Serial.println("Renaming " + String(buf));
#endif
        client.println("350 RNFR accepted - file exists, ready for destination");
        rnfrCmd = true;
      }
    }
  }
  //
  //  RNTO - Rename To
  //
  else if (!strcmp(command, "RNTO"))
  {
    char path[FTP_CWD_SIZE];
    // char dir[ FTP_FIL_SIZE ];
    if (strlen(buf) == 0 || !rnfrCmd)
      client.println("503 Need RNFR before RNTO");
    else if (strlen(parameters) == 0)
      client.println("501 No file name");
    else if (makePath(path))
    {
      if (SD.exists(path))
        client.println("553 " + String(parameters) + " already exists");
      else
      {
      }
    }
    rnfrCmd = false;
  }

  ///////////////////////////////////////
  //                                   //
  //   EXTENSIONS COMMANDS (RFC 3659)  //
  //                                   //
  ///////////////////////////////////////

  //
  //  FEAT - New Features
  //
  else if (!strcmp(command, "FEAT"))
  {
    client.println("211-Extensions suported:");
    client.println(" MLSD");
    client.println("211 End.");
  }
  //
  //  MDTM - File Modification Time (see RFC 3659)
  //
  else if (!strcmp(command, "MDTM"))
  {
    client.println("550 Unable to retrieve time");
  }

  //
  //  SIZE - Size of the file
  //
  else if (!strcmp(command, "SIZE"))
  {
    char path[FTP_CWD_SIZE];
    if (strlen(parameters) == 0)
      client.println("501 No file name");
    else if (makePath(path))
    {
      file = SD.open(path, FILE_READ);
      if (!file)
        client.println("450 Can't open " + String(parameters));
      else
      {
        client.println("213 " + String(file.size()));
        file.close();
      }
    }
  }
  //
  //  SITE - System command
  //
  else if (!strcmp(command, "SITE"))
  {
    client.println("500 Unknow SITE command " + String(parameters));
  }
  //
  //  Unrecognized commands ...
  //
  else
    client.println("500 Unknow command");

  return true;
}

boolean FtpServer::dataConnect()
{
  unsigned long startTime = millis();
  // wait 5 seconds for a data connection
  if (!data.connected())
  {
    while (!dataServer.hasClient() && millis() - startTime < 10000)
    //    while (!dataServer.available() && millis() - startTime < 10000)
    {
      // delay(100);
      yield();
    }
    if (dataServer.hasClient())
    {
      //    if (dataServer.available()) {
      data.stop();
      data = dataServer.available();
#ifdef FTP_DEBUG
      Serial.println("ftpdataserver client....");
#endif
    }
  }

  return data.connected();
}

boolean FtpServer::doRetrieve()
{
  // int16_t nb = file.readBytes((uint8_t*) buf, FTP_BUF_SIZE );
  int16_t nb = file.readBytes(buf, FTP_BUF_SIZE);
  if (nb > 0)
  {
    data.write((uint8_t *)buf, nb);
    bytesTransfered += nb;
    return true;
  }
  closeTransfer();
  return false;
}

boolean FtpServer::doStore()
{
  if (data.connected())
  {
    unsigned long ms0 = millis();
    // Serial.print("Transfer=");
    int16_t nb = data.readBytes((uint8_t *)buf, FTP_BUF_SIZE);
    // unsigned long ms1 = millis();
    // Serial.print(ms1-ms0);
    if (nb > 0)
    {
      // Serial.println( millis() << " " << nb << endl;
      // Serial.print("SD=");
      size_t written = file.write((uint8_t *)buf, nb);
      /*
      unsigned long ms2 = millis();
      Serial.print(ms2-ms1);
      Serial.print("nb=");
      Serial.print(nb);
      Serial.print("w=");
      Serial.println(written);
      */
      bytesTransfered += nb;
    }
    else
    {
      Serial.println(".");
    }
    return true;
  }
  closeTransfer();
  return false;
}

void FtpServer::closeTransfer()
{
  uint32_t deltaT = (int32_t)(millis() - millisBeginTrans);
  if (deltaT > 0 && bytesTransfered > 0)
  {
    client.println("226-File successfully transferred");
    client.println("226 " + String(deltaT) + " ms, " + String(bytesTransfered / deltaT) + " kbytes/s");
  }
  else
    client.println("226 File successfully transferred");

  file.close();
  data.stop();
}

void FtpServer::abortTransfer()
{
  if (transferStatus > 0)
  {
    file.close();
    data.stop();
    client.println("426 Transfer aborted");
#ifdef FTP_DEBUG
    Serial.println("Transfer aborted!");
#endif
  }
  transferStatus = 0;
}

// Read a char from client connected to ftp server
//
//  update cmdLine and command buffers, iCL and parameters pointers
//
//  return:
//    -2 if buffer cmdLine is full
//    -1 if line not completed
//     0 if empty line received
//    length of cmdLine (positive) if no empty line received

int8_t FtpServer::readChar()
{
  int8_t rc = -1;

  if (client.available())
  {
    char c = client.read();
    // char c;
    // client.readBytes((uint8_t*) c, 1);
#ifdef FTP_DEBUG
    Serial.print(c);
#endif
    if (c == '\\')
    {
      c = '/';
    }
    if (c != '\r')
    {
      if (c != '\n')
      {
        if (iCL < FTP_CMD_SIZE)
          cmdLine[iCL++] = c;
        else
          rc = -2; //  Line too long
      }
      else
      {
        cmdLine[iCL] = 0;
        command[0] = 0;
        parameters = NULL;
        // empty line?
        if (iCL == 0)
          rc = 0;
        else
        {
          rc = iCL;
          // search for space between command and parameters
          parameters = strchr(cmdLine, ' ');
          if (parameters != NULL)
          {
            if (parameters - cmdLine > 4)
            {
              rc = -2; // Syntax error
            }
            else
            {
              strncpy(command, cmdLine, parameters - cmdLine);
              command[parameters - cmdLine] = 0;

              while (*(++parameters) == ' ')
                ;
            }
          }
          else if (strlen(cmdLine) > 4)
            rc = -2; // Syntax error.
          else
            strcpy(command, cmdLine);
          iCL = 0;
        }
      }
    }
    if (rc > 0)
    {
      for (uint8_t i = 0; i < strlen(command); i++)
      {
        command[i] = toupper(command[i]);
      }
    }
    if (rc == -2)
    {
      iCL = 0;
      client.println("500 Syntax error");
    }
  }
  return rc;
}

// Make complete path/name from cwdName and parameters
//
// 3 possible cases: parameters can be absolute path, relative path or only the name
//
// parameters:
//   fullName : where to store the path/name
//
// return:
//    true, if done

boolean FtpServer::makePath(char *fullName)
{
  return makePath(fullName, parameters);
}

boolean FtpServer::makePath(char *fullName, char *param)
{
  if (param == NULL)
    param = parameters;

  // Root or empty?
  if (strcmp(param, "/") == 0 || strlen(param) == 0)
  {
    strcpy(fullName, "/");
    return true;
  }
  // If relative path, concatenate with current dir
  if (param[0] != '/')
  {
    strcpy(fullName, cwdName);
    if (fullName[strlen(fullName) - 1] != '/')
      strncat(fullName, "/", FTP_CWD_SIZE);
    strncat(fullName, param, FTP_CWD_SIZE);
  }
  else
    strcpy(fullName, param);
  // If ends with '/', remove it
  uint16_t strl = strlen(fullName) - 1;
  if (fullName[strl] == '/' && strl > 1)
    fullName[strl] = 0;
  if (strlen(fullName) < FTP_CWD_SIZE)
    return true;

  client.println("500 Command line too long");
  return false;
}

// Calculate year, month, day, hour, minute and second
//   from first parameter sent by MDTM command (YYYYMMDDHHMMSS)
//
// parameters:
//   pyear, pmonth, pday, phour, pminute and psecond: pointer of
//     variables where to store data
//
// return:
//    0 if parameter is not YYYYMMDDHHMMSS
//    length of parameter + space

uint8_t FtpServer::getDateTime(uint16_t *pyear, uint8_t *pmonth, uint8_t *pday,
                               uint8_t *phour, uint8_t *pminute, uint8_t *psecond)
{
  char dt[15];

  // Date/time are expressed as a 14 digits long string
  //   terminated by a space and followed by name of file
  if (strlen(parameters) < 15 || parameters[14] != ' ')
    return 0;
  for (uint8_t i = 0; i < 14; i++)
    if (!isdigit(parameters[i]))
      return 0;

  strncpy(dt, parameters, 14);
  dt[14] = 0;
  *psecond = atoi(dt + 12);
  dt[12] = 0;
  *pminute = atoi(dt + 10);
  dt[10] = 0;
  *phour = atoi(dt + 8);
  dt[8] = 0;
  *pday = atoi(dt + 6);
  dt[6] = 0;
  *pmonth = atoi(dt + 4);
  dt[4] = 0;
  *pyear = atoi(dt);
  return 15;
}

// Create string YYYYMMDDHHMMSS from date and time
//
// parameters:
//    date, time
//    tstr: where to store the string. Must be at least 15 characters long
//
// return:
//    pointer to tstr

char *FtpServer::makeDateTimeStr(char *tstr, uint16_t date, uint16_t time)
{
  sprintf(tstr, "%04u%02u%02u%02u%02u%02u",
          ((date & 0xFE00) >> 9) + 1980, (date & 0x01E0) >> 5, date & 0x001F,
          (time & 0xF800) >> 11, (time & 0x07E0) >> 5, (time & 0x001F) << 1);
  return tstr;
}