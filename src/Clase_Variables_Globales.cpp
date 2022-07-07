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
    case Primer_Cancel_Credit:
        return Primer_Cancel_Credit_;
        break;
    case Flag_Hopper_Enable:
        return Flag_Hopper_Enable_;
        break;
    case Flag_Maquina_En_Juego:
        return Flag_Maquina_En_Juego_;
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
        Enable_Storage_ = Change_estado;
        break;

    case Archivo_CSV_OK:
        Archivo_CSV_OK_ = Change_estado;
        break;

    case Archivo_LOG_OK:
        Archivo_LOG_OK_ = Change_estado;
    case Dato_Entrante_Valido:
        Dato_Entrante_Valido_ = Change_estado;
        break;
    case Dato_Entrante_No_Valido:
        Dato_Entrante_No_Valido_ = Change_estado;
        break;
    case Dato_Evento_Valido:
        Dato_Evento_Valido_ = Change_estado;
        break;
    case Sincronizacion_RTC:
        Sincronizacion_RTC_ = Change_estado;
        break;
    case Serializacion_Serie_Trama:
        Serializacion_Serie_Trama_ = Change_estado;
        break;
    case Comunicacion_Maq:
        Comunicacion_Maq_ = Change_estado;
        break;
    case Primer_Cancel_Credit:
        Primer_Cancel_Credit_ = Change_estado;
        break;
    case Flag_Hopper_Enable:
        Flag_Hopper_Enable_ = Change_estado;
        break;
    case Flag_Maquina_En_Juego:
        Flag_Maquina_En_Juego_ = Change_estado;
        break;
    }
}

void Variables_Globales::Init_Variables_Globales() // Constructor explicito
{
    Ftp_Mode_ = false;
    Bootloader_Mode_ = false;
    Enable_Storage_ = true;
    Archivo_CSV_OK_ = false;
    Archivo_LOG_OK_ = false;
    Dato_Entrante_Valido_ = false;
    Dato_Entrante_No_Valido_ = false;
    Dato_Evento_Valido_ = false;
    Sincronizacion_RTC_ = false;
    Serializacion_Serie_Trama_ = false;
    Comunicacion_Maq_ = false;
    Primer_Cancel_Credit_ = false;
    Flag_Hopper_Enable_ = false;
    Flag_Maquina_En_Juego_ = false;
}
