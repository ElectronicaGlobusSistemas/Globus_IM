
#define TAMANO_HISTORIAL 255

class Eventos_SAS
{
private:

    char Evento_SAS;

    unsigned long Last_Time_Event_Generate=0;
    int Last_Event_Gerate=0x00;
    unsigned long Current_Time_Generate_Event;
    unsigned long TimeOut_Exec=30000; /* 1 Minuto 60000  default 30000*/

    int Current_Total_Event=0; /* Cantidad actual de eventos */
    int HistorialEventos[TAMANO_HISTORIAL];  /* Historial de eventos */

public:
    bool Set_evento(char evento);
    char Get_evento(void);

    bool Ignore_Event(int Current_Event);

    bool Ignore_Event_Plus(int Current_Evento);
    void TimeOut_Capture_Event(void);
    bool Set_Timer_Ignore_Event(unsigned long Time);
    void TimeOut_Capture_Event_Plus(void);
};