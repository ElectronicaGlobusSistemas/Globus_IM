#include <iostream>
#include "Clase_Variables_Globales.h"




bool Variables_Globales::Get_Variable_Global(int Filtro)
{
    switch (Filtro)
    {
    case 1:
        return Ftp_Mode;
        break;
    
    case 2:
        return Bootloader_Mode;
        break;
    }
}
void Variables_Globales::Set_Variable_Global(int Filtro, bool Change_estado)
{
    switch (Filtro)
    {
    case 1:
        Ftp_Mode = Change_estado;
        break;
    case 2:
        Bootloader_Mode = Change_estado;
        break;
    }
}

void Variables_Globales::Init_Variables_Globales() // Constructor explicito 
{
    Ftp_Mode = false;
    Bootloader_Mode = false;
}
