#include "Configuracion.h"

bool Configuracion_ESP32::Set_Configuracion_ESP32(int expression, char data[])
{
    switch (expression)
    {
    case 1:
        memcpy(Direccion_IP_, data, sizeof(Direccion_IP_) / sizeof(Direccion_IP_[0]));
        return true;
        break;
    case 2:
        memcpy(Direccion_IP_GW_, data, sizeof(Direccion_IP_GW_) / sizeof(Direccion_IP_GW_[0]));
        return true;
        break;
    case 3:
        memcpy(Direccion_SN_MASK_, data, sizeof(Direccion_SN_MASK_) / sizeof(Direccion_SN_MASK_[0]));
        return true;
        break;
    case 4:
        memcpy(Direccion_IP_Server_, data, sizeof(Direccion_IP_Server_) / sizeof(Direccion_IP_Server_[0]));
        return true;
        break;

    default:
        return false;
        break;
    }
}

bool Configuracion_ESP32::Set_Configuracion_ESP32(int expression, uint16_t dato)
{
    switch (expression)
    {
    case 5:
        Puerto_Server_ = dato;
        return true;
        break;
    case 10:
        Tipo_Maquina_ = dato;
        return true;
        break;
    case 11:
        Puerto_AP_=dato;
        return true;
        break;

    default:
        return false;
        break;
    }
}

bool Configuracion_ESP32::Set_Configuracion_ESP32(int expression, String dato)
{
    switch (expression)
    {
    case 6:
        Nombre_Maquina_ = dato;
        return true;
        break;
    case 7:
        SSID_ = dato;
        return true;
        break;
    case 8:
        Password_ = dato;
        return true;
        break;
    case Id_Maquina:
        Id_Maquina_=dato;
        return true;
        break;


    default:
        return false;
        break;
    }
}

bool Configuracion_ESP32::Set_Configuracion_ESP32(int expression, bool state)
{
    switch (expression)
    {
    case 9:
        Tipo_Conexion_ = state;
        return true;
        break;

    default:
        return false;
        break;
    }
}

char *Configuracion_ESP32::Get_Configuracion(int expression, char)
{
    switch (expression)
    {
    case 1:
        return Direccion_IP_;
        break;
    case 2:
        return Direccion_IP_GW_;
        break;
    case 3:
        return Direccion_SN_MASK_;
        break;
    case 4:
        return Direccion_IP_Server_;
        break;

    default:
        return 0x00;
        break;
    }
}

uint16_t Configuracion_ESP32::Get_Configuracion(int expression, int)
{
    switch (expression)
    {
    case 5:
        return Puerto_Server_;
        break;
    case 10:
        return Tipo_Maquina_;
        break;

    case 11:
        return Puerto_AP_;
        break;

    default:
        return 0;
        break;
    }
}

String Configuracion_ESP32::Get_Configuracion(int expression, String)
{
    switch (expression)
    {
    case 6:
        return Nombre_Maquina_;
        break;
    case 7:
        return SSID_;
        break;
    case 8:
        return Password_;
        break;

    case Id_Maquina:
        return Id_Maquina_;
        break;

    default:
        return "Hola Mundo";
        break;
    }
    return "Hola Mundo";
}

bool Configuracion_ESP32::Get_Configuracion(int expression)
{
    switch (expression)
    {
    case 9:
        return Tipo_Conexion_;
        break;

    default:
        return NULL;
        break;
    }
}

bool Configuracion_ESP32::Set_Configuracion_ESP32_ES(int expression,String dato [])
{
    switch (expression)
    {
    case Controlador_P:
        Controlador_Principal_=dato[0];
        return true;
        break;
    
    case Metodo_Sincro_RTC:
        Metodo_RTC_=dato[1];
        return true;
        break;

    case Metodo_Conta:
        Metodo_Contadores_=dato[2];
        return true;
        break;

    case Metodo_Event:
        Metodo_Eventos_=dato[3];
        return true;
        break;


    case Metodo_Access_T:
        Metodo_Token_=dato[4];
        return true;
        break;

    default:
        return false;
        break;
    }
}

String Configuracion_ESP32::Get_Configuracion_ES(int expression, String)
{
    switch (expression)
    {
    case Controlador_P:
        return Controlador_Principal_;
        break;
    
    case Metodo_Sincro_RTC:
        return Metodo_RTC_;
        break;

    case Metodo_Conta:
        return Metodo_Contadores_;
        break;

    case Metodo_Event:
        return Metodo_Eventos_;
        break;


    case Metodo_Access_T:
        return Metodo_Token_;
        break;


     default:
        return "Hola Mundo";
        break;
    }
}