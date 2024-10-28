#include "Eventos.h"
#include <Arduino.h>

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

        case 0x57: // Requerimiento TITO
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
        case 0x8C: // Juego Seleccionado
            Evento_SAS=evento;
            return true;
            break;
        case 0x82: // Ha ingresado a pantalla de contadores o menu de asistente.
            Evento_SAS = evento;
            return true;
            break;
        case 0x83: // Ha salido de pantalla de contadores o menu de asistente.
            Evento_SAS = evento;
            return true;
            break;
        case 0x1B: // la caja del dinero fue removida.
            Evento_SAS = evento;
            return true;
            break;
        case 0x1C: // la caja del dinero fue instalada.
            Evento_SAS = evento;
            return true;
            break;
        case 0x7B: // Validador de billetes Los totales han sido reseteados.
            Evento_SAS = evento;
            return true;
            break;
        case 0x31: /*CMOS RAM ERROR (data recovered EEPROM)*/
            Evento_SAS = evento;
            return true;
            break;
        case 0x32: /*CMOS RAM ERROR (no data recovered EEPROM)*/
            Evento_SAS = evento;
            return true;
            break;
        case 0x33: /*CMOS RAM ERROR(bad device)*/
            Evento_SAS = evento;
            return true;
            break;
        case 0x2C: /* Billete falso  detectado*/
            Evento_SAS = evento;
            return true;
            break;
        case 0x34: /*EEPROM error (data error)*/
            Evento_SAS = evento;
            return true;
            break;
        case 0x35: /*EEPROM error (bad device)*/
            Evento_SAS = evento;
            return true;
            break;
        case 0x36: /*EEPROM error (diffent checksum version changed)*/
            Evento_SAS = evento;
            return true;
            break;
        case 0x37: /*EPROM error bad checksum compare*/
            Evento_SAS = evento;
            return true;
            break;
        case 0x38: /*Partitioned EEPROM error (checksum version changed)*/
            Evento_SAS = evento;
            return true;
            break;
        case 0x39: /*Partitioned EEPROM error (bad checksum compare)*/
            Evento_SAS = evento;
            return true;
            break;
        case 0x3A: /*Memory error reset (operador used self test)*/
            Evento_SAS = evento;
            return true;
            break;
        
         case 0x7C: /*Memory error reset (operador used self test)*/
            Evento_SAS = evento;
            return true;
            break;

        case 0x01: /*ACK RECIBIDO*/
            Evento_SAS = evento;
            return true;
            break;

        case 0x3D:
            Evento_SAS=evento;
            return true;
            break;

        case 0x3E:
            Evento_SAS=evento;
            return true;
            break;

        case 0x67:
        //Serial.println(evento,HEX);
        Evento_SAS=evento;
        return true;
        break;

        default:

           // Serial.println(evento,HEX);
            Evento_SAS = 0x00;
            return false;
            break;
    }
}
char Eventos_SAS::Get_evento(void)
{
    return Evento_SAS;
}


/* Timer  para eventos repetidos */
void Eventos_SAS::TimeOut_Capture_Event(void)
{
    Current_Time_Generate_Event=millis();
   
}
/* Timer  para eventos repetidos */
void Eventos_SAS::TimeOut_Capture_Event_Plus(void)
{
    Current_Time_Generate_Event=millis();

    if(Current_Time_Generate_Event-Last_Time_Event_Generate>=TimeOut_Exec && Current_Total_Event>0)
    {
        /* Limpia el historial de eventos a comparar  si  ha pasado el intervalo de  tiempo */
        Current_Total_Event=0;
        Last_Time_Event_Generate=Current_Time_Generate_Event;
    }
}



/*  Verifica si el  nuevo evento es igual al anterior  durante TimeOut_Exec
retorna = true  Evento nuevo o tiempo de espera agotado
retorna = false Evento repetido durante el tiempo de espera  
*/
bool Eventos_SAS::Ignore_Event(int Event)
{
    if(Event!=Last_Event_Gerate || (Current_Time_Generate_Event-Last_Time_Event_Generate>=TimeOut_Exec))
    {
        Last_Event_Gerate=Event;
        Last_Time_Event_Generate=Current_Time_Generate_Event;

        return true;
    }else
        return false;
}

/* Establece timer para  ignorar eventos duplicados en ms */
bool Eventos_SAS::Set_Timer_Ignore_Event(unsigned long Time)
{
    TimeOut_Exec=Time;

    if(TimeOut_Exec==Time)
        return true;
    else
        return false;
}

/*  Ignora  eventos  repetidos en una lista durante TimeOut_Exec
retorna = true Evento nuevo en la lista o tiempo de espera agotado
retorna = false Evento repetido  durante el tiempo de espera
 */
bool Eventos_SAS::Ignore_Event_Plus(int Current_Evento)
{
    
    bool Evento_Duplicado=false;

    for(int i=0; i<Current_Total_Event; i++)
    {
        if(HistorialEventos[i]==Current_Evento)
        {
            Evento_Duplicado=true;
            break;
        }
    }

    if(!Evento_Duplicado||Current_Time_Generate_Event-Last_Time_Event_Generate>=TimeOut_Exec)
    {
        HistorialEventos[Current_Total_Event%TAMANO_HISTORIAL]=Current_Evento;
        Current_Total_Event++;
        Last_Time_Event_Generate=Current_Time_Generate_Event;
        return true;
    }else{
        return false;
    }
}