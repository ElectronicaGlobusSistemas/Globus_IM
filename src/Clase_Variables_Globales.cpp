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
    case Enable_Storage:
        return Enable_Storage_;
        break;
    case Archivo_CSV_OK:
        return Archivo_CSV_OK_;
        break;

    case Archivo_LOG_OK:
        return Archivo_LOG_OK_;
        break;
    }
}
void Variables_Globales::Set_Variable_Global(int Filtro, bool Change_estado)
{
    switch (Filtro)
    {
    case Ftp_Mode:
        Ftp_Mode_ = Change_estado;
        break;
    case Bootloader_Mode:
        Bootloader_Mode_ = Change_estado;
        break;
    case Enable_Storage:
        Enable_Storage_= Change_estado;
        break;

    case Archivo_CSV_OK:
        Archivo_CSV_OK_=Change_estado;
        break;

    case Archivo_LOG_OK:
        Archivo_LOG_OK_=Change_estado;
        break;
    }
}

void Variables_Globales::Init_Variables_Globales() // Constructor explicito 
{
    Ftp_Mode_ = false;
    Bootloader_Mode_ = false;
    Enable_Storage_=true;
    Archivo_CSV_OK_=false;
    Archivo_LOG_OK_=false;

}
