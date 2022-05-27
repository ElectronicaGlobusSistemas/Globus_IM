/*
        Clase  Para la consulta de contadores en Maquina.
                        ***Metodos***
                    ->Metodo Get_Contadores
                    ->Metodo Set_Contadores
*/
#include "Stream.h"
#include <iostream>
using namespace std;

#define Total_Cancel_Credit 1
#define Coin_In 2
#define Coin_Out 3
#define Jackpot 4
#define Total_Drop 5
#define Cancel_Credit_Hand_Pay 6
#define Bill_Amount 7
#define Casheable_In 8
#define Casheable_Restricted_In 9
#define Casheable_NONrestricted_In 10
#define Casheable_Out 11
#define Casheable_Restricted_Out 12
#define Casheable_NONrestricted_Out 13
#define Games_Played 14

//---------------------------------------->Clase Manejo de Contadores<-------------------------------------------------
class Contadores_SAS
{

private: // Variables Privadas Para contadores
  char Total_Cancel_Credit_[9];
  char Coin_In_[9];
  char Coin_Out_[9];
  char Jackpot_[9];
  char Total_Drop_[9];
  char Cancel_Credit_Hand_Pay_[9];
  char Bill_Amount_[9];
  char Casheable_In_[9];
  char Casheable_Restricted_In_[9];
  char Casheable_NONrestricted_In_[9];
  char Casheable_Out_[9];
  char Casheable_Restricted_Out_[9];
  char Casheable_NONrestricted_Out_[9];
  char Games_Played_[9];
  char Vacio;

public:
  char *Get_Contadores_Char(int Filtro_Contador);    // Metodos Publicos
  int Get_Contadores_Int(int Filtro_Contador);       // Metodo Get Contadores
  string Get_Contadores_String(int Filtro_Contador); // Metodo Get Contadores
  bool Set_Contadores(int Filtro_Contador, char[]);  // Metodo Set Contadores
};