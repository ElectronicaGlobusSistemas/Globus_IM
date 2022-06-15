#include <iostream>
#include "Clase_Variables_Globales.h"




bool Variables_Globales::Get_Variable_Global(int Filtro)
{
    switch (Filtro)
    {
    case Ftp_Mode:
        return Ftp_Mode_;
        break;
    case Bootloader_Mode:
        return Bootloader_Mode_;
        break;
    }
}
void Variables_Globales::Set_Variable_Global(int Filtro, bool Change_estado)
{
    switch (Filtro)
    {
    case 1:
        Ftp_Mode_ = Change_estado;
        break;
    case 2:
        Bootloader_Mode_ = Change_estado;
        break;
    }
}

void Variables_Globales::Init_Variables_Globales() // Constructor explicito 
{
    Ftp_Mode_ = false;
    Bootloader_Mode_ = false;
}
