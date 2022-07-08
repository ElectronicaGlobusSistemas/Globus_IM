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
    case Estado_Escritura:
        return Estado_Escritura_;
        break;
    case Libera_Memoria_OK:
        return Libera_Memoria_OK_;
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

    case Estado_Escritura:
        Estado_Escritura_= Change_estado;
        break;
    case Libera_Memoria_OK:
        Libera_Memoria_OK_ = Change_estado;
        break;
    }
}

void Variables_Globales::Init_Variables_Globales() // Constructor explicito
{
    Ftp_Mode_ = false;
    Bootloader_Mode_ = false;
    Enable_Storage_=false;
    Archivo_CSV_OK_=false;
    Archivo_LOG_OK_=false;
    Dato_Entrante_Valido_ = false;
    Dato_Entrante_No_Valido_ = false;
    Dato_Evento_Valido_ = false;
    Sincronizacion_RTC_ = false;
    Serializacion_Serie_Trama_ = false;
    Comunicacion_Maq_ = false;
    Encabezado_Maquina_Generica_="Hora,Total Cancel Credit,Coin In,Coin Out,Jackpot,Total Drop, Cancel Credit Hand Pay,Bill Amount, Casheable In, Casheable Restricted In, Casheable Non Restricted In, Casheable Out, Casheable Restricted Out,Casheable Nonrestricted Out,Games Played,Coin In Fisico,Coin Out Fisico,Total Coin Drop,Machine Paid Progresive Payout,Machine Paid External Bonus Payout,Attendant Paid Progresive Payout,Attendant Paid External Payout,Ticket In,Ticket Out,Current Credits,Contador 1C - Door Open Metter,Contador 18 - Games Since Last Power Up";
    Encabezado_Maquina_Eventos_="Hora,Evento, Descripción";
    Estado_Escritura_=false;
    Libera_Memoria_OK_=false;
    
}




String Variables_Globales::Get_Encabezado_Maquina(int Filtro)
{
    switch (Filtro)
    {
    case Encabezado_Maquina_Generica:
        return Encabezado_Maquina_Generica_;
        break;

    case Encabezado_Maquina_Eventos:
        return Encabezado_Maquina_Eventos_;
        break;
    }
}