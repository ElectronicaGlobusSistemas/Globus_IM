//Eventos SAS.
#include "Stream.h"
#include <iostream>
using namespace std;
#include "string.h"

#define Puerta_Abierta                              0x11
#define Puerta_Cerrada                              0x12
#define Maquina_Encendida                           0x17
#define Maquina_Apagada                             0x18
#define Billete_Aceptado                            0x4F
#define Hanpay_Pendiente                            0x51
#define Hanpay_Reset                                0x52
#define Cashout_Presionado                          0x66
#define Transferencia_AFT_Completa                  0x69
#define Requerimiento_AFT_Host_Cashout              0x6A
#define Requerimiento_AFT_Solicitud_Registro        0x6C
#define Requerimiento_AFT_Interrogacion_Registro    0x6D
#define Requerimiento_AFT_Cancelar_Registro         0x6E
#define Juego_Seleccionado                          0x8C
#define Ingreso_Pantalla_Contadores                 0x82
#define Salida_Pantalla_Contadores                  0x83
#define Caja_Dinero_removida                        0x1B
#define Caja_dinero_Instalada                       0x1C
#define Validador_billetes_Reset                    0x7B

class Tabla_Eventos
{
private:
    String Puerta_Abierta_="";
    String Puerta_Cerrada_="";
    String Maquina_Encendida_="";
    String Maquina_Apagada_="";
    String Billete_Aceptado_="";
    String Hanpay_Pendiente_="";
    String Hanpay_Reset_="";
    String Cashout_Presionado_="";
    String Transferencia_AFT_Completa_="";
    String Requerimiento_AFT_Host_Cashout_="";
    String Requerimiento_AFT_Solicitud_Registro_="";
    String Requerimiento_AFT_Interrogacion_Registro_="";
    String Requerimiento_AFT_Cancelar_Registro_="";
    String Evento_No_Identificado_="";
    String Juego_Seleccionado_="";
    String Ingreso_Pantalla_Contadores_="";
    String Salida_Pantalla_Contadores_="";
    String Caja_Dinero_removida_="";
    String Caja_dinero_Instalada_="";
    String Validador_billetes_Reset_="";

public:
   String Get_Descrip_Eventos(int Eventoss);
    void  Init_Tabla_Eventos(void);
};

String Tabla_Eventos::Get_Descrip_Eventos(int Eventoss)
{
    switch (Eventoss)
    {
    case 0x11:
        return Puerta_Abierta_;
        break;
    case 0x12:
        return Puerta_Cerrada_;
        break;
    case 0x17:
        return Maquina_Encendida_;
        break;
    case 0x18:
        return Maquina_Apagada_;
        break;
    case 0x4F:
        return Billete_Aceptado_;
        break;
    case 0x51:
        return Hanpay_Pendiente_;
        break;
    case 0x52:
        return Hanpay_Reset_;
        break;
    case 0x66:
        return Cashout_Presionado_;
        break;
    case 0x69:
        return Transferencia_AFT_Completa_;
        break;
    case 0x6A:
        return Requerimiento_AFT_Host_Cashout_;
        break;
    case 0x6C:
        return Requerimiento_AFT_Solicitud_Registro_;
        break;
    case 0x6D:
        return Requerimiento_AFT_Interrogacion_Registro_;
        break;
    case 0x6E:
        return Requerimiento_AFT_Cancelar_Registro_;
        break;

    case Juego_Seleccionado:
        return Juego_Seleccionado_;
        break;
    case Ingreso_Pantalla_Contadores:
        return Ingreso_Pantalla_Contadores_;
        break;

    case Salida_Pantalla_Contadores:
        return Salida_Pantalla_Contadores_;
        break;
    case Caja_Dinero_removida:
        return Caja_Dinero_removida_;
        break;
    case Caja_dinero_Instalada:
        return Caja_dinero_Instalada_;
        break;
    case Validador_billetes_Reset:
        return Validador_billetes_Reset_;
        break;

    default:
        return Evento_No_Identificado_;
        break;
    }
}

void Tabla_Eventos::Init_Tabla_Eventos(void)
{
    Puerta_Abierta_ = "Puerta Abierta";
    Puerta_Cerrada_ = "Puerta Cerrada";
    Maquina_Encendida_ = "Maquina Encendida";
    Maquina_Apagada_ = "Puerta Cerrada";
    Billete_Aceptado_ = "Billete Acetado";
    Hanpay_Pendiente_ = "Hanpay Pendiente";
    Hanpay_Reset_ = "Hanpay_Reset";
    Cashout_Presionado_ = "Cashout Presionado";
    Transferencia_AFT_Completa_ = "Transferencia AFT Completa";
    Requerimiento_AFT_Host_Cashout_ = "Requerimiento AFT Host-Cashout";
    Requerimiento_AFT_Solicitud_Registro_ = "Requerimiento AFT Solicitud Registro";
    Requerimiento_AFT_Interrogacion_Registro_ = "Requerimiento AFT Interrogacion Registro";
    Requerimiento_AFT_Cancelar_Registro_ = "Requerimiento AFT Cancelar Registro";
    Evento_No_Identificado_="Evento No Identificado";
    Juego_Seleccionado_="Juego Seleccionado";
    Ingreso_Pantalla_Contadores_="Ingreso a pantalla de contadores o menu asistente";
    Salida_Pantalla_Contadores_="Salida de pantalla de contadores o menu de asistente";
    Caja_Dinero_removida_="Caja del dinero removida";
    Caja_dinero_Instalada_="Caja del dinero instalada";
    Validador_billetes_Reset_="Validador de billetes Totales han sido reseteados";
}
