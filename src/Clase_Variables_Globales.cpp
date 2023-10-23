#include <iostream>
#include "Clase_Variables_Globales.h"

extern bool Bootloader_Enable;
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
    case Version_Firmware:
        return Version_Firmware_;
        break;
    case Enable_SD:
        return Enable_SD_;
        break;
    case Consulta_Info_Cashless_OK:
        return Consulta_info_Cashless_OK_;
        break;
    case SD_INSERT:
        return SD_INSERT_;
        break;
    case Flag_Memoria_SD_Full: 
        return Flag_Memoria_SD_Full_;
        break;

    case Flag_Creditos_D_P:
        return Flag_Creditos_D_Premio_;
        break;

    case Flag_Trama_Completa:
        return Flag_Trama_Completa_;
        break;
     
    case Flag_Crea_Archivos:
        return Flag_Crea_Archivos_;
        break;

    case Flag_Archivos_OK:
        return Flag_Archivos_OK_;
        break;

    case Flag_Sesion_RFID:
        return Flag_Sesion_RFID_;
        break;
    case Conexion_RFID:
        return Conexion_RFID_;
        break;
    case Flag_Contadores_Sesion_ON:
        return Flag_Contadores_Sesion_ON_;
        break;
    case Flag_Contadores_Sesion_OFF:
        return Flag_Contadores_Sesion_OFF_;
        break;
    case Flag_Cashout_Button:
        return Flag_Cashout_Button_;
        break;  
    
    case Operador_Detected:
        return Operador_Detected_;
        break; 

    case Trama_Pendiente:
         return Trama_Pendiente_;
        break;

    case Consulta_Info_Cliente:
         return Consulta_Info_Cliente_;
        break;

    case Conexion_To_Host:
        return Conexion_To_Host_;
        break;

    case Consulta_Conexion_To_Host:
        return Consulta_Conexion_To_Host_;
        break;

    case Handle_RFID_Lector:
        return Handle_RFID_Lector_;
        break;

    case Manual_Reset:
        return Manual_Reset_;
        break;
    case Manual_Detected:
        return Manual_Detected_;
        break;
    
    case Billete_Insert:
        return Billete_Insert_;
        break;

    case Reset_Handpay_in_Process:
        return Reset_Handpay_in_Process_;
        break;

    case Type_Hanpay_Reset:
        return Type_Hanpay_Reset_;
        break;

    case Falla_MicroSD:
        return Falla_MicroSD_;
        break;

    case Verify_Modulo_RFID:
        return Verify_Modulo_RFID_;
        break;

    case Handle_Task_WIFI_CONEXION:
        return Handle_Task_WIFI_;
        break;

    case MARCA_OPERADOR_VALIDO:
        return MARCA_OPERADOR_VALIDO_;
        break;

    case Consulta_Info_Lector_Rfid:
        return Consulta_Info_Lector_Rfid_;
        break;

    case Excepcion_WIFI:
        return Excepcion_WIFI_;
        break;


    case ACK_Valido:
        return ACK_Valido_;
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
        break;
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
        break;
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
    case Enable_SD:
        Enable_SD_=Change_estado;
        break;
    case Consulta_Info_Cashless_OK:
        Consulta_info_Cashless_OK_=Change_estado;
        break;
    case SD_INSERT:
        SD_INSERT_=Change_estado;
        break;
    case Flag_Memoria_SD_Full:
        Flag_Memoria_SD_Full_=Change_estado;
        break;

    case Flag_Creditos_D_P:
        Flag_Creditos_D_Premio_=Change_estado;
        break;

    case Flag_Trama_Completa:
        Flag_Trama_Completa_=Change_estado;
        break;

    case Flag_Crea_Archivos:
        Flag_Crea_Archivos_=Change_estado;
        break;

    case Flag_Archivos_OK:
        Flag_Archivos_OK_=Change_estado;
        break;

    case Flag_Sesion_RFID:
        Flag_Sesion_RFID_ = Change_estado;
        break;
    case Conexion_RFID:
        Conexion_RFID_ = Change_estado;
        break;
    case Flag_Contadores_Sesion_ON:
        Flag_Contadores_Sesion_ON_ = Change_estado;
        break;
    case Flag_Contadores_Sesion_OFF:
        Flag_Contadores_Sesion_OFF_ = Change_estado;
        break;
    case Flag_Cashout_Button:
        Flag_Cashout_Button_ = Change_estado;
        break;

    case Operador_Detected:
         Operador_Detected_ = Change_estado;
        break;

    case Trama_Pendiente:
        Trama_Pendiente_ = Change_estado;
        break;

    case Consulta_Info_Cliente:
        Consulta_Info_Cliente_=Change_estado;
        break;

    case Conexion_To_Host:
        Conexion_To_Host_=Change_estado;
        break;

    case Consulta_Conexion_To_Host:
        Consulta_Conexion_To_Host_=Change_estado;
        break;
    case Handle_RFID_Lector:
        Handle_RFID_Lector_=Change_estado;
        break;


    case Manual_Reset:
        Manual_Reset_=Change_estado;
        break;
    case Manual_Detected:
        Manual_Detected_=Change_estado;
        break;
    case Billete_Insert:
        Billete_Insert_=Change_estado;
        break;

    case Reset_Handpay_in_Process:
        Reset_Handpay_in_Process_=Change_estado;
        break;

    case Type_Hanpay_Reset:
        Type_Hanpay_Reset_=Change_estado;
        break;

    case Falla_MicroSD:
        Falla_MicroSD_=Change_estado;
        break;

    case Verify_Modulo_RFID:
        Verify_Modulo_RFID_=Change_estado;
        break;

    case Handle_Task_WIFI_CONEXION:
        Handle_Task_WIFI_=Change_estado;
        break;

    case MARCA_OPERADOR_VALIDO:
        MARCA_OPERADOR_VALIDO_=Change_estado;
        break;
    case Consulta_Info_Lector_Rfid:
        Consulta_Info_Lector_Rfid_=Change_estado;
        break;

    case Excepcion_WIFI:
        Excepcion_WIFI_=Change_estado;
        break;

    case ACK_Valido:
        ACK_Valido_=Change_estado;
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
    Version_Firmware_=0.0; // Version  de firmware de tarjeta.
    Espacio_Libre_SD_="0000";
    Enable_SD_=false;
    Espacio_Usado_SD_="0000";
    Size_SD_="0000";
    Consulta_info_Cashless_OK_=false;
    Bootloader_Enable=false;
    SD_INSERT_=false;
    Flag_Memoria_SD_Full_=false;
    Reset_Handay_OK_=0x04;
    Flag_Creditos_D_Premio_=false;
    Flag_Trama_Completa_=false;
    Flag_Type_excepcion_=0;
    Flag_Crea_Archivos_=false;
    Flag_Archivos_OK_=false;


    Flag_Sesion_RFID_=false;
    Conexion_RFID_=false;
    Sig_Estado_RFID_=0;
    Flag_Contadores_Sesion_ON_=false;
    Flag_Contadores_Sesion_OFF_=false;
    Flag_Cashout_Button_=false;
    Operador_Detected_=false;
    Trama_Pendiente_=false;
    Consulta_Info_Cliente_=false;
    Conexion_To_Host_=false;
    Consulta_Conexion_To_Host_=false;
    Handle_RFID_Lector_=false;

    Manual_Reset_=false;
    Manual_Detected_=false;
    Encabezado_Archivo_Sesiones_="Hora,ID Cliente";
    Encabezado_Archivo_Premios_="Hora,ID Operador";
    Reset_Handpay_in_Process_=false;
    Type_Hanpay_Reset_=false;
    Falla_MicroSD_=false;
    Verify_Modulo_RFID_=false;
    Handle_Task_WIFI_=false;
    MARCA_OPERADOR_VALIDO_=false;
    Consulta_Info_Lector_Rfid_=false;
    Excepcion_WIFI_=false;
    ACK_Valido_=false;
    
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

    case Encabezado_Archivo_Sesiones:
        return Encabezado_Archivo_Sesiones_;
        break;

    case Encabezado_Archivo_Premios:
    return Encabezado_Archivo_Premios_;
    break;
    }
}

String Variables_Globales::Get_Variables_Global_String(int filtro)
{
    switch (filtro)
    {
    case Espacio_Libre_SD:
        return Espacio_Libre_SD_;
        break;
    case Espacio_Usado_SD:
        return Espacio_Usado_SD_;
        break;
    case Size_SD:
        return Size_SD_;
        break;
    case Temperatura_procesador:
        return Temperatura_procesador_;
        break;
    }
}

void Variables_Globales::Set_Variable_Global_String(int Filtro, String Dato)
{

    switch (Filtro)
    {
    case Espacio_Libre_SD:
        Espacio_Libre_SD_=Dato;
        break;
    case Espacio_Usado_SD:
        Espacio_Usado_SD_=Dato;
        break;
    case Size_SD:
        Size_SD_=Dato;
        break;
    case Temperatura_procesador:
        Temperatura_procesador_=Dato;
        break;
    }
}

void Variables_Globales::Set_Variable_Global_Char(int filtro,char Dato)
{
    switch (filtro)
    {
    case Reset_Handay_OK:
        Reset_Handay_OK_=Dato;
        break;
    }
}

char Variables_Globales::Get_Variable_Global_Char(int filtro)
{
    switch (filtro)
    {
    case Reset_Handay_OK:
        return Reset_Handay_OK_;
        break;
    }
}

void Variables_Globales::Set_Variable_Global_Int(int filtro, int Change_estado)
{
    switch (filtro)
    {
    case Flag_Type_excepcion:
        Flag_Type_excepcion_ = Change_estado;
        break;

    case Sig_Estado_RFID:
        Sig_Estado_RFID_ = Change_estado;
        break;
    }


}
int Variables_Globales::Get_Variable_Global_Int(int filtro)
{
    switch (filtro)
    {
    case Flag_Type_excepcion:
        return Flag_Type_excepcion_;
        break;
    case Sig_Estado_RFID:
        return Sig_Estado_RFID_;
        break;
    }  
}