#include "Stream.h"
#include <iostream>
using namespace std;

#define Ftp_Mode            1
#define Bootloader_Mode     2
#define Enable_Storage      3
#define Archivo_CSV_OK      10
#define Archivo_LOG_OK      11

class Variables_Globales
{
private: 
bool Ftp_Mode_;
bool Bootloader_Mode_;
bool Enable_Storage_;
int  Number_Data_Storage_;
bool Archivo_CSV_OK_;
bool Archivo_LOG_OK_;

public:
void Init_Variables_Globales(); 
void Set_Variable_Global(int Filtro, bool Change_estado);
bool Get_Variable_Global(int Filtro);
};



