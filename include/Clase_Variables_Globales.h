#include "Stream.h"
#include <iostream>
using namespace std;

#define Ftp_Mode 1
#define Bootloader_Mode 2
#define Dato_Entrante_Valido 4
#define Dato_Entrante_No_Valido 5
#define Dato_Evento_Valido 6
#define Sincronizacion_RTC 7
#define Serializacion_Serie_Trama 8
#define Comunicacion_Maq 9

class Variables_Globales
{
private:
    bool Ftp_Mode_;
    bool Bootloader_Mode_;
    bool Dato_Entrante_Valido_;
    bool Dato_Entrante_No_Valido_;
    bool Dato_Evento_Valido_;
    bool Sincronizacion_RTC_;
    bool Serializacion_Serie_Trama_;
    bool Comunicacion_Maq_;

public:
    void Init_Variables_Globales();
    void Set_Variable_Global(int Filtro, bool Change_estado);
    bool Get_Variable_Global(int Filtro);
};
