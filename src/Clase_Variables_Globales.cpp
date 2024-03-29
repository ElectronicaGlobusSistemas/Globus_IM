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
    case Primer_Cancel_Credit:
        return Primer_Cancel_Credit_;
        break;
    case Calc_Cancel_Credit:
        return Calcula_Cancel_Credit_;
        break;
    case Flag_Hopper_Enable:
        return Flag_Hopper_Enable_;
        break;
    case Flag_Maquina_En_Juego:
        return Flag_Maquina_En_Juego_;
        break;

    case Fallo_Archivo_COM:
        return Fallo_Archivo_COM_;
        break;
    case Fallo_Archivo_EVEN:
        return Fallo_Archivo_EVEN_;
        break;
    case Fallo_Archivo_LOG:
        return Fallo_Archivo_LOG_;
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
    case Estado_Escritura:
        Estado_Escritura_ = Change_estado;
        break;
    case Libera_Memoria_OK:
        Libera_Memoria_OK_ = Change_estado;
    case Primer_Cancel_Credit:
        Primer_Cancel_Credit_ = Change_estado;
        break;
    case Calc_Cancel_Credit:
        Calcula_Cancel_Credit_ = Change_estado;
        break;
    case Flag_Hopper_Enable:
        Flag_Hopper_Enable_ = Change_estado;
        break;
    case Flag_Maquina_En_Juego:
        Flag_Maquina_En_Juego_ = Change_estado;
        break;

    case Fallo_Archivo_COM:
         Fallo_Archivo_COM_=Change_estado;
        break;
    case Fallo_Archivo_EVEN:
        Fallo_Archivo_EVEN_=Change_estado;
        break;
    case Fallo_Archivo_LOG:
        Fallo_Archivo_LOG_=Change_estado;
        break;
    }
}

void Variables_Globales::Init_Variables_Globales() // Constructor explicito
{
    Ftp_Mode_ = false;
    Bootloader_Mode_ = false;
    Enable_Storage_ = false;
    Archivo_CSV_OK_ = false;
    Archivo_LOG_OK_ = false;
    Enable_Storage_=false;
    Archivo_CSV_OK_=false;
    Archivo_LOG_OK_=false;

    Fallo_Archivo_COM_=true;
    Fallo_Archivo_EVEN_=true;
    Fallo_Archivo_LOG_=true;
    
    Dato_Entrante_Valido_ = false;
    Dato_Entrante_No_Valido_ = false;
    Dato_Evento_Valido_ = false;
    Sincronizacion_RTC_ = false;
    Serializacion_Serie_Trama_ = false;
    Comunicacion_Maq_ = false;
    Encabezado_Maquina_Generica_ = "Hora,Total Cancel Credit,Coin In,Coin Out,Jackpot,Total Drop, Cancel Credit Hand Pay,Bill Amount, Casheable In, Casheable Restricted In, Casheable Non Restricted In, Casheable Out, Casheable Restricted Out,Casheable Nonrestricted Out,Games Played,Coin In Fisico,Coin Out Fisico,Total Coin Drop,Machine Paid Progresive Payout,Machine Paid External Bonus Payout,Attendant Paid Progresive Payout,Attendant Paid External Payout,Ticket In,Ticket Out,Current Credits,Contador 1C - Door Open Metter,Contador 18 - Games Since Last Power Up";
    Encabezado_Maquina_Eventos_ = "Hora,Evento, Descripción";
    Estado_Escritura_ = false;
    Libera_Memoria_OK_ = false;
    Primer_Cancel_Credit_ = false;
    Calcula_Cancel_Credit_ = false;
    Flag_Hopper_Enable_ = false;
    Flag_Maquina_En_Juego_ = false;

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