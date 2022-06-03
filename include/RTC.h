#include <Arduino.h>
#include <ESP32Time.h>
#include <iostream>
#include <sstream>

String Get_Hora(void);
String Get_Fecha(void);
void Sincroniza_Hora(int Segundos,int Minutos, int Hora, int Dia, int Mes, int Year);
