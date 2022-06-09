#include "Eventos.h"

bool Eventos_SAS::Set_evento(char evento)
{
    switch (evento)
    {
    case 0x11: // Puerta abierta
        Evento_SAS = evento;
        return true;
        break;
    case 0x12: // Puerta cerrada
        Evento_SAS = evento;
        return true;
        break;
    case 0x17: // Maquina encendida
        Evento_SAS = evento;
        return true;
        break;
    case 0x18: // Maquina apagada
        Evento_SAS = evento;
        return true;
        break;
    case 0x4F: // Billete aceptado
        Evento_SAS = evento;
        return true;
        break;
    case 0x51: // Handpay pendiente
        Evento_SAS = evento;
        return true;
        break;
    case 0x52: // Handpay reset
        Evento_SAS = evento;
        return true;
        break;
    case 0x66: // Cashout presionado
        Evento_SAS = evento;
        return true;
        break;
    case 0x69: // Transferencia AFT completa
        Evento_SAS = evento;
        return true;
        break;
    case 0x6A: // Requerimiento AFT host-cashout
        Evento_SAS = evento;
        return true;
        break;
    case 0x6C: // Requerimiento AFT Solicitud Registro
        Evento_SAS = evento;
        return true;
        break;
    case 0x6D: // Requerimiento AFT Interrogacion registro
        Evento_SAS = evento;
        return true;
        break;
    case 0x6E: // Requerimiento AFT Cancelar registro
        Evento_SAS = evento;
        return true;
        break;
    default:
        Evento_SAS = 0x00;
        return false;
        break;
    }
}

char Eventos_SAS::Get_evento(void)
{
    return Evento_SAS;
}