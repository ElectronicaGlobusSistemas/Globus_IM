#include "Stream.h"
#include <iostream>
using namespace std;

#define Ftp_Mode 1
#define Bootloader_Mode 2

class Variables_Globales
{
private: 
bool Ftp_Mode_;
bool Bootloader_Mode_;
public:
void Init_Variables_Globales(); 
void Set_Variable_Global(int Filtro, bool Change_estado);
bool Get_Variable_Global(int Filtro);

};



