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
//#include "mySD.h"
#include <WiFiClient.h>

//----------------------------------------------------------------------------------------

//----------------------------> Hardware SD <----------------------------------------------
#define SD_ChipSelect   5            // Selector de Esclavo SPI SS1_SPI.
#define SD_Status       4            // Indicador  de Estado de Memoria SD.
#define MOSI 23                      // Pin de Datos In           
#define MISO 19                      // Pin de Datos Out
#define CLK  18                      // Señal de reloj
//-----------------------------------------------------------------------------------------
//----------------------------> Prototipo de Funciones <-----------------------------------

#define FORMAT_NOT_STARTED     0   // No existe proceso de formateo Iniciado
#define FORMAT_IN_PROGRESS     1   // Formateando tarjeta SD...
#define FORMAT_SUCCESS         2   // Tarjeta formateada con éxito
#define FORMAT_FAILED          3   // Falla formateando tarjeta SD



void Erro_Log_Write(String Datos, String archivo);
void Queue_SD(void);
void Init_SD(void);
static void Task_Verifica_Conexion_SD(void *parameter);
void Create_ARCHIVE_Txt( char* ARCHIVO);
void Write_Data_File_Txt(String Datos, char* ARCHIVO);
void Create_ARCHIVE_Excel(char* ARCHIVO, String  Encabezado);
void Write_Data_File(String Datos,char *ARCHIVO,bool select);
void Write_Data_File2(String Datos, String archivo, bool select, String Encabezado);
void Write_Data_File3(String Datos,char *archivo,bool select, String Encabezado);
void Read_File(const char* ARCHIVO);
bool Remove_Archive(char* ARCHIVO);
void Nueva_Carpeta(char* Carpeta);
bool Init_FTP_SERVER(void);
void RESET_SD(void);
static void Rum_FTP_SERVER(void *parameter);
void SD_Status_Rum(void);
void FreeSpace_SD(void);
void LOG_ESP(char *ARCHIVO,bool Enable);
bool Libera_Memoria(int Total_Memoria_MB, int Espacio_Usado_MB);
void Create_ARCHIVE_Excel_Eventos(char *ARCHIVO, String Encabezado);
void FLASH_RESET(void);
void Rum_FTP_Server(void);
void Ftp_handle(void);
void Storage_Cliente(String archivo, bool Enable,byte *Buffer);
void Storage_Premios_OP(String archivo, bool Enable, byte *Buffer);
void Update_Status_SD(void);
void RESET_SD_2(bool Select);
void Prueba_LOG(char *ARCHIVO,bool Enable,String Datos);
void LOG_ESP_Descrip(char *ARCHIVO,bool Enable,String Mensaje);
void Erro_Log(String Datos, String archivo);
bool VerificaArchivo(const char* archivo, String DataTime);
void FtpFast(void);
bool formatearSD(void);
void Evento_Formateo_SD(void);
//------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------
#endif
