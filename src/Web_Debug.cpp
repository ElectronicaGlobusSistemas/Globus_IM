#include "Arduino.h"
#include "Web_Config.h"
#include "Clase_Variables_Globales.h"
#include "Contadores.h"
#include "ESP32Time.h"
#include "time.h"





extern Contadores_SAS contadores; // Objeto contiene contadores maquina
extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales
extern ESP32Time RTC; // Objeto contiene hora y fecha


bool Web_Config::MaquinaDisponible(void)
{
    bool IsSuccess = false;

    if (!Variables_globales.Get_Variable_Global(Flag_Sesion_Cashless) && !Variables_globales.Get_Variable_Global(Flag_Sesion_RFID) && !Variables_globales.Get_Variable_Global(Flag_Maquina_En_Juego) && contadores.Get_Contadores_Int(Current_Credits) <=10)
        IsSuccess = true;
    else
        IsSuccess = false;

    return IsSuccess;
}


void Web_Config::Rutine_handle(void)
{
    int hora = RTC.getHour(true);
    int minuto = RTC.getMinute(); 

}


