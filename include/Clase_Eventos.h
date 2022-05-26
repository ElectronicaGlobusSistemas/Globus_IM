/*
        Clase  Para la consulta de contadores en Maquina.
                        ***Metodos***
                    ->Metodo Get_Contadores
                    ->Metodo Set_Contadores
*/

#include <iostream>
using namespace std;
//------------------------------------Definición de Eventos-------------------------------------------------------------
#define Puerta_Abierta                      1 // 0x11
#define Puerta_Cerrada                      2 // 0x12
#define Maquina_Encendida                   3 // 0x17
#define Maquina_Apagada                     4 // 0x18
#define                                     5 // 0x26
#define Operator_Changed_ Options           6 // 0x3C
#define Billete Aceptado                    7 // 0x4F
#define Hanpay_Pendiente                    8 // 0x51
#define Hanpay_Reset                        9 // 0x52
#define Boton_Cashout_Presionado            10// 0x66
#define Transferencia_AFT_Completa          11// 0x69
#define Requerimiento_host_Cashout_AFT12    12// 0x6A
#define Juego_Iniciado                      13// 0x7E
#define Juego_Terminado                     8 // 0x7E

//---------------------------------------->Clase Manejo de Contadores<-------------------------------------------------
class Contadores_SAS
{

private: // Variables Privadas Para contadores
  int Total_Cancel_Credit_;
  int Coin_In_;
  int Coin_Out_;
  int Jackpot_;
  int Total_Drop_;
  int Cancel_Credit_Hand_Pay_;
  int Bill_Amount_;
  int Casheable_In_;
  int Casheable_Restricted_In_;
  int Casheable_NONrestricted_In_;
  int Casheable_Out_;
  int Casheable_Restricted_Out_;
  int Casheable_NONrestricted_Out_;
  int Games_Played_;
  int Vacio;

public:                                                               // Metodos Publicos
  int Get_Eventos(int Filtro_Contador);                        // Metodo Get Eventos
  bool Set_Eventos(int Filtro_Contador, int Data_Contador); // Metodo Set Eventos
};
//---------------------------------------->Metodo Para Obtener Eventos<---------------------------------------------
int Contadores_SAS::Get_Eventos(int Filtro_Evento)
{
  switch (Filtro_Evento) // Selecciona Evento Especifico.
  {

  case 1: // Bloque de instrucciones 1;
    return Total_Cancel_Credit_;
    break;
  case 2: // Bloque de instrucciones 2;
    return Coin_In_;
    break;
  case 3: // Bloque de instrucciones 3;
    return Coin_Out_;
    break;
  case 4: // Bloque de instrucciones 4;
    return Jackpot_;
    break;
  case 5: // Bloque de instrucciones 5;
    return Total_Drop_;
    break;
  case 6: // Bloque de instrucciones 6;
    return Cancel_Credit_Hand_Pay_;
    break;
  case 7: // Bloque de instrucciones 7;
    return Bill_Amount_;
    break;
  case 8: // Bloque de instrucciones 8;
    return Casheable_In_;
    break;
  case 9: // Bloque de instrucciones 9;
    return Casheable_Restricted_In_;
    break;
  case 10: // Bloque de instrucciones 10;
    return Casheable_NONrestricted_In_;
    break;
  case 11: // Bloque de instrucciones 11;
    return Casheable_Out_;
    break;
  case 12: // Bloque de instrucciones 12;
    return Casheable_Restricted_Out_;
    break;
  case 13: // Bloque de instrucciones 13;
    return Casheable_NONrestricted_Out_;
    break;
  case 14: // Bloque de instrucciones 14;
    return Games_Played_;
    break;
  }
}
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------->Metodo Para Guardar Eventos<---------------------------------------------
bool Contadores_SAS::Set_Eventos(int Filtro_Evento, int Data_Evento)
{
  switch (Filtro_Evento) // Selecciona Evento Especifico.
  {
  case 1: // Bloque de instrucciones 1;
    Total_Cancel_Credit_ = Data_Contador;
    return true;
    break;
  case 2: // Bloque de instrucciones 2;
    Coin_In_ = Data_Contador;
    return true;
    break;
  case 3: // Bloque de instrucciones 3;
    Coin_Out_ = Data_Contador;
    return true;
    break;
  case 4: // Bloque de instrucciones 4;
    Jackpot_ = Data_Contador;
    return true;
    break;
  case 5: // Bloque de instrucciones 5;
    Total_Drop_ = Data_Contador;
    return true;
    break;
  case 6: // Bloque de instrucciones 6;
    Cancel_Credit_Hand_Pay_ = Data_Contador;
    return true;
    break;
  case 7: // Bloque de instrucciones 7;
    Bill_Amount_ = Data_Contador;
    return true;
    break;
  case 8: // Bloque de instrucciones 8;
    Casheable_In_ = Data_Contador;
    return true;
    break;
  case 9: // Bloque de instrucciones 9;
    Casheable_Restricted_In_ = Data_Contador;
    return true;
    break;
  case 10: // Bloque de instrucciones 10;
    Casheable_NONrestricted_In_ = Data_Contador;
    return true;
    break;
  case 11: // Bloque de instrucciones 11;
    Casheable_Out_ = Data_Contador;
    return true;
    break;

  case 12: // Bloque de instrucciones 12;
    Casheable_Restricted_Out_ = Data_Contador;
    return true;
    break;
  case 13: // Bloque de instrucciones 13;
    Casheable_NONrestricted_Out_ = Data_Contador;
    return true;
    break;
  case 14: // Bloque de instrucciones 13;
    Games_Played_ = Data_Contador;
    return true;
    break;
  }
  return false;
}
//---------------------------------------------------------------------------------------------------------------------