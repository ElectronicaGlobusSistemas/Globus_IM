#include "Eventos.h"

bool Eventos_SAS::set_evento(char evento)
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
    case 0x18: // Maqina apagada
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
    case 0x7E: // Juego iniciado
        Evento_SAS = evento;
        return true;
        break;
    case 0x7F: // Juego temrinado
        Evento_SAS = evento;
        return true;
        break;
    default:
        Evento_SAS = 0x00;
        return false;
        break;
    }
}