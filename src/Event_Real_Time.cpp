#include "Event_Real_Time.h"
#include "Arduino.h"
#include "Clase_Variables_Globales.h"
#include "Eventos.h"


extern Eventos_SAS eventos; // Objeto contiene eventos maquina
extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales



/* Reporte de eventos  mecanicos 

Utiliza sensores digitales y logica Pull Up. Si  la señal se mantiene estable durante TimeOut_Signal despues del cambio 
el evento es reportado, esto con el objetivo de  eliminar  fluctuaciones  en las mediciones del sensor
*/

bool Event_Real_Time::Reset_Event_Class(void)
{
    bool Last_State_Stacker = false;
    bool Report_Event = false;
    unsigned long Start_Time_Event = 0;
    bool Last_State_Door = false;
    bool Report_Event_Door = false;
    unsigned long Start_Time_Event_Door = 0;


    if(!Report_Event &&!Report_Event_Door)
        return true;
    else
        return false;
}

void Event_Real_Time::Enable_Event_Real_Time(bool Enable)
{
    Variables_globales.Set_Variable_Global(Enable_Mechanical_Events,Enable);
}

void Event_Real_Time::EVENT_REAL_TIME(int Pin_Stacker, int Pin_Door_Machine, int TimeOut_Signal)
{

    /*--------------------------------> Reporta evento de stacker <-----------------------------------*/
    bool Current_State_Stacker = digitalRead(Pin_Stacker);
   // Serial.println(Current_State_Stacker );
    // Serial.println(Current_State_Stacker);
    // Si el estado actual es diferente al estado previo
    if (Current_State_Stacker != Last_State_Stacker && !Report_Event)
    {
        // Si han pasado al menos 10 segundos desde el último cambio de estado
        Start_Time_Event = millis();
        Report_Event = true;
    }

    if (Report_Event && millis() - Start_Time_Event >= TimeOut_Signal)
    {

        if (Current_State_Stacker)
        {
            eventos.Set_evento(REMOVE_STACKER);
            int Evento=eventos.Get_evento();
            if(eventos.Ignore_Event(Evento))
                Variables_globales.Set_Variable_Global(Dato_Evento_Valido, true);
        }
        else
        {
            eventos.Set_evento(STACKER_INSTALL);
            int Evento=eventos.Get_evento();
            if(eventos.Ignore_Event(Evento))
                Variables_globales.Set_Variable_Global(Dato_Evento_Valido, true);
        }
        Start_Time_Event = 0;                       // Actualiza el tiempo del último cambio de estado
        Last_State_Stacker = Current_State_Stacker; // Actualiza el estado previo del sensor
        Report_Event = false;
    }
    /*-----------------------------------------------------------------------------------------------*/

    /* -------------------------> Reporte de evento de Apertura de puerta <---------------------------*/
    bool Current_State_Door = digitalRead(Pin_Door_Machine);

    if (Current_State_Door != Last_State_Door && !Report_Event_Door)
    {
        // Si han pasado al menos 10 segundos desde el último cambio de estado
        Start_Time_Event_Door = millis();
        Report_Event_Door = true;
    }

    if (Report_Event_Door && millis() - Start_Time_Event_Door >= TimeOut_Signal)
    {

        if (Current_State_Door)
        {
            eventos.Set_evento(OPEN_DOOR);
            int Evento=eventos.Get_evento();
            if(eventos.Ignore_Event(Evento))
                Variables_globales.Set_Variable_Global(Dato_Evento_Valido, true);
        }
        else
        {
            eventos.Set_evento(CLOSE_DOOR);
            int Evento=eventos.Get_evento();
            if(eventos.Ignore_Event(Evento))
                Variables_globales.Set_Variable_Global(Dato_Evento_Valido, true);
        }
        Start_Time_Event_Door = 0;            // Actualiza el tiempo del último cambio de estado
        Last_State_Door = Current_State_Door; // Actualiza el estado previo del sensor
        Report_Event_Door = false;
    }
    /*-----------------------------------------------------------------------------------------*/
}