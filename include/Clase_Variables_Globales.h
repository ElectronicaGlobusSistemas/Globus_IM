#include "Stream.h"
#include <iostream>
using namespace std;

int Ftp_Mode_=1;
int Bootloader_Mode_=2;

class Variables_Globales
{
private: 
bool Ftp_Mode;
bool Bootloader_Mode;
public:
void Init_Variables_Globales(); 
void Set_Variable_Global(int Filtro, bool Change_estado);
bool Get_Variable_Global(int Filtro);

};



