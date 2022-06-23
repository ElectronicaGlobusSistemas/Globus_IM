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
    case Dato_Entrante_Valido:
        return Dato_Entrante_Valido_;
        break;
    case Dato_Entrante_No_Valido:
        return Dato_Entrante_No_Valido_;
        break;
    case Dato_Evento_Valido:
        return Dato_Evento_Valido_;
        break;
    case Sincronizacion_RTC:
        return Sincronizacion_RTC_;
        break;
    case Serializacion_Serie_Trama:
        return Serializacion_Serie_Trama_;
        break;
    case Comunicacion_Maq:
        return Comunicacion_Maq_;
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
    case 4:
        Dato_Entrante_Valido_ = Change_estado;
        break;
    case 5:
        Dato_Entrante_No_Valido_ = Change_estado;
        break;
    case 6:
        Dato_Evento_Valido_ = Change_estado;
        break;
    case 7:
        Sincronizacion_RTC_ = Change_estado;
        break;
    case 8:
        Serializacion_Serie_Trama_ = Change_estado;
        break;
    case 9:
        Comunicacion_Maq_ = Change_estado;
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
    Dato_Entrante_Valido_ = false;
    Dato_Entrante_No_Valido_ = false;
    Dato_Evento_Valido_ = false;
    Sincronizacion_RTC_ = false;
    Serializacion_Serie_Trama_ = false;
    Comunicacion_Maq_ = false;
}
