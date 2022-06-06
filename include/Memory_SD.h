#ifndef EXAMPLE_H
#define EXAMPLE_H

#define SD_ChipSelect   5
#define SD_Status       4 

//----------------------------> Prototipo de Funciones <-----------------------------------
void Init_SD(void);
static void Task_Verifica_Conexion_SD(void *parameter);
void Create_ARCHIVE_Txt( char* ARCHIVO);
void Write_Data_File_Txt(String Datos, char* ARCHIVO);
void Create_ARCHIVE_Excel(char* ARCHIVO, String  Encabezado);
void Write_Data_File(String Datos,char *ARCHIVO,bool select);
void Write_Data_File2(char* Datos,char *archivo,bool select, String Encabezado);
void Write_Data_File3(String Datos,char *archivo,bool select, String Encabezado);
void Read_File(const char* ARCHIVO);
void Remove_Archive(char* ARCHIVO);
void Nueva_Carpeta(char* Carpeta);
//------------------------------------------------------------------------------------------

#endif
