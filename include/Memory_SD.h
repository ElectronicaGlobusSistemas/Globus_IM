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
void Init_FTP_SERVER();
void RESET_SD(void);
static void Rum_FTP_SERVER(void *parameter);
void SD_Status_Rum(void);
void FreeSpace_SD(void);
void LOG_ESP(char *ARCHIVO,bool Enable);
bool Libera_Memoria(int Total_Memoria_MB, int Espacio_Usado_MB);
void Create_ARCHIVE_Excel_Eventos(char *ARCHIVO, String Encabezado);
//------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------
#endif
