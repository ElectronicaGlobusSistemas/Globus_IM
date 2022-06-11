#include <ESP32Time.h>
#include <iostream>
#include <sstream>
using namespace std;

ESP32Time rtc;

//--------------------------> Función Para solicitar Hora en MCU <--------------------------
String Get_Hora(void)
{
    //    String Hora_24=(String)rtc.getHour(true); // Hora
    //   String Minutos=(String)rtc.getMinute(); // Minutos
    //   String Segundos=(String)rtc.getSecond(); // Segundos
    //    String Horario=rtc.getAmPm(true); // PM/AM
    return rtc.getTime(); // Hora 10:45:05 AM
}
//------------------------------------------------------------------------------------------

//---------------------------> Función Para solicitar Fecha <-------------------------------
String Get_Fecha(void)
{
    String Dia = (String)rtc.getDay();   // Dia
    String Mes = (String)rtc.getMonth(); // Mes
    String Year = (String)rtc.getYear(); // año
    return (Dia + Mes + Year);           // 01302022 Nombre archivo Format
}
//------------------------------------------------------------------------------------------
