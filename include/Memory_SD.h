/**********************************
 * @file Memory_SD.h              *
 * @author Globus Sistemas        *
 * @brief                         *
 * @version 0.1                   *
 * @date 2022-06-11               *
 * @copyright Copyright (c) 2022  *
 **********************************/


#ifndef EXAMPLE_H
#define EXAMPLE_H
//------------------------------------>  Archivos Header <--------------------------------
#include "mySD.h"
#include <WiFiClient.h>

//----------------------------------------------------------------------------------------

#define FTP_SERVER_VERSION "2022"    // Version de  Libreria.
#define FTP_CTRL_PORT    21          // Puerto del Servidor.  
#define FTP_DATA_PORT_PASV 50009     // Modo Pasivo.
#define FTP_TIME_OUT  5              // timeout  Para desconectar el Cliente por inactividad. 
#define FTP_CMD_SIZE 255 + 8         // Tamaño Maximo de Comandos.
#define FTP_CWD_SIZE 255 + 8         // Tamaño Maximo de Nombres de Carpetas.
#define FTP_FIL_SIZE 255             // Tamaño Maximo de Nombres de Archivos.
#define FTP_BUF_SIZE (8192*1)-1      // Tamaño Maximo del Buffer de Escritura y Lectura.
//----------------------------> Hardware SD <----------------------------------------------
#define SD_ChipSelect   5            // Selector de Esclavo SPI SS1_SPI.
#define SD_Status       4            // Indicador  de Estado de Memoria SD.
#define MOSI 23                      // Pin de Datos In           
#define MISO 19                      // Pin de Datos Out
#define CLK  18                      // Señal de reloj
//-----------------------------------------------------------------------------------------
//----------------------------> Prototipo de Funciones <-----------------------------------
void Init_SD(void);
static void Task_Verifica_Conexion_SD(void *parameter);
void Create_ARCHIVE_Txt( char* ARCHIVO);
void Write_Data_File_Txt(String Datos, char* ARCHIVO);
void Create_ARCHIVE_Excel(char* ARCHIVO, String  Encabezado);
void Write_Data_File(String Datos,char *ARCHIVO,bool select);
void Write_Data_File2(String Datos,char *archivo,bool select, String Encabezado);
void Write_Data_File3(String Datos,char *archivo,bool select, String Encabezado);
void Read_File(const char* ARCHIVO);
void Remove_Archive(char* ARCHIVO);
void Nueva_Carpeta(char* Carpeta);
void Init_FTP_SERVER();
void RESET_SD(void);
static void Rum_FTP_SERVER(void *parameter);
void SD_Status_Rum(void);
void FreeSpace_SD(void);
void LOG_ESP(char *ARCHIVO,bool Enable);
void Libera_Memoria(float Total_Memoria_MB, float Free_Space);

//------------------------------------------------------------------------------------------

//-------------------------------> Clase  Server FTP <--------------------------------------
class FtpServer
{
public:
  FtpServer();
  void begin(String uname, String pword);
  int handleFTP();
private:
  void iniVariables();
  void clientConnected();
  void disconnectClient();
  boolean userIdentity();
  boolean userPassword();
  boolean processCommand();
  boolean dataConnect();
  boolean doRetrieve();
  boolean doStore();
  void closeTransfer();
  void abortTransfer();
  boolean makePath(char *fullname);
  boolean makePath(char *fullName, char *param);
  uint8_t getDateTime(uint16_t *pyear, uint8_t *pmonth, uint8_t *pday,
                      uint8_t *phour, uint8_t *pminute, uint8_t *second);
  char *makeDateTimeStr(char *tstr, uint16_t date, uint16_t time);
  int8_t readChar();

  IPAddress dataIp; // IP address of client for data
  WiFiClient client;
  WiFiClient data;
  Sd2Card card;
  boolean dataPassiveConn;
  uint16_t dataPort;
  char buf[FTP_BUF_SIZE];     // Buffer de Transferencia de Datos.
  char cmdLine[FTP_CMD_SIZE]; // where to store incoming char from client
  char cwdName[FTP_CWD_SIZE]; // Nombre de Carpeta Actual
  char command[5];            // Comandos enviados para el Cliente.
  boolean rnfrCmd;            // previous command was RNFR
  char *parameters;           // point to begin of parameters sent by client
  uint16_t iCL;               // pointer to cmdLine next incoming char
  int8_t cmdStatus,           // status of ftp command connexion
      transferStatus;         // Estado de la Transferencias FTP.
  uint32_t millisTimeOut,     // Tiempo de Espera  por inactividad
      millisDelay,
      millisEndConnection, //
      millisBeginTrans,    // store time of beginning of a transaction
      bytesTransfered;     //
  String _FTP_USER;
  String _FTP_PASS;
};
//------------------------------------------------------------------------------------------
#endif
