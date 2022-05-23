/*
        Clase  Para la consulta de contadores en Maquina.
                        ***Metodos***
                    ->Metodo Get_Contadores
                    ->Metodo Set_Contadores
*/

#include <iostream>
using namespace std;

#define Total_Cancel_Credit             1
#define Coin_In                         2
#define Coin_Out                        3
#define Jackpot                        4
#define Total_Drop                      5
#define Cancel_Credit_Hand_Pay          6
#define Bill_Amount                     7
#define Casheable_In                    8
#define Casheable_Restricted_In         9
#define Casheable_NONrestricted_In      10
#define Casheable_Out                   11
#define Casheable_Restricted_Out        12
#define Casheable_NONrestricted_Out     13
#define Games_Played                    14

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

public:                                                        // Metodos Publicos
  int Get_Contadores_Int(int Filtro_Contador);                     // Metodo Get Contadores
  string Get_Contadores_String(int Filtro_Contador);                     // Metodo Get Contadores
  bool Set_Contadores(int Filtro_Contador, int Data_Contador); // Metodo Set Contadores
};

// //---------------------------------------->Metodo Para Obtener Contadores<---------------------------------------------
// int Contadores_SAS::Get_Contadores(int Filtro_Contador)
// {
//   switch (Filtro_Contador) // Selecciona Contador Especifico.
//   {
//   case 1: // Bloque de instrucciones 1;
//     return Total_Cancel_Credit_;
//     break;
//   case 2: // Bloque de instrucciones 2;
//     return Coin_In_;
//     break;
//   case 3: // Bloque de instrucciones 3;
//     return Coin_Out_;
//     break;
//   case 4: // Bloque de instrucciones 4;
//     return Jackpot_;
//     break;
//   case 5: // Bloque de instrucciones 5;
//     return Total_Drop_;
//     break;
//   case 6: // Bloque de instrucciones 6;
//     return Cancel_Credit_Hand_Pay_;
//     break;
//   case 7: // Bloque de instrucciones 7;
//     return Bill_Amount_;
//     break;
//   case 8: // Bloque de instrucciones 8;
//     return Casheable_In_;
//     break;
//   case 9: // Bloque de instrucciones 9;
//     return Casheable_Restricted_In_;
//     break;
//   case 10: // Bloque de instrucciones 10;
//     return Casheable_NONrestricted_In_;
//     break;
//   case 11: // Bloque de instrucciones 11;
//     return Casheable_Out_;
//     break;
//   case 12: // Bloque de instrucciones 12;
//     return Casheable_Restricted_Out_;
//     break;
//   case 13: // Bloque de instrucciones 13;
//     return Casheable_NONrestricted_Out_;
//     break;
//   case 14: // Bloque de instrucciones 14;
//     return Games_Played_;
//     break;
//   }
//   return 0;
// }
// //---------------------------------------------------------------------------------------------------------------------
// //---------------------------------------->Metodo Para Guardar Contadores<---------------------------------------------
// bool Contadores_SAS::Set_Contadores(int Filtro_Contador, int Data_Contador)
// {
//   switch (Filtro_Contador) // Selecciona Contador Especifico.
//   {
//   case 1: // Bloque de instrucciones 1;
//     Total_Cancel_Credit_ = Data_Contador;
//     return true;
//     break;
//   case 2: // Bloque de instrucciones 2;
//     Coin_In_ = Data_Contador;
//     return true;
//     break;
//   case 3: // Bloque de instrucciones 3;
//     Coin_Out_ = Data_Contador;
//     return true;
//     break;
//   case 4: // Bloque de instrucciones 4;
//     Jackpot_ = Data_Contador;
//     return true;
//     break;
//   case 5: // Bloque de instrucciones 5;
//     Total_Drop_ = Data_Contador;
//     return true;
//     break;
//   case 6: // Bloque de instrucciones 6;
//     Cancel_Credit_Hand_Pay_ = Data_Contador;
//     return true;
//     break;
//   case 7: // Bloque de instrucciones 7;
//     Bill_Amount_ = Data_Contador;
//     return true;
//     break;
//   case 8: // Bloque de instrucciones 8;
//     Casheable_In_ = Data_Contador;
//     return true;
//     break;
//   case 9: // Bloque de instrucciones 9;
//     Casheable_Restricted_In_ = Data_Contador;
//     return true;
//     break;
//   case 10: // Bloque de instrucciones 10;
//     Casheable_NONrestricted_In_ = Data_Contador;
//     return true;
//     break;
//   case 11: // Bloque de instrucciones 11;
//     Casheable_Out_ = Data_Contador;
//     return true;
//     break;
//   case 12: // Bloque de instrucciones 12;
//     Casheable_Restricted_Out_ = Data_Contador;
//     return true;
//     break;
//   case 13: // Bloque de instrucciones 13;
//     Casheable_NONrestricted_Out_ = Data_Contador;
//     return true;
//     break;
//   case 14: // Bloque de instrucciones 13;
//     Games_Played_ = Data_Contador;
//     return true;
//     break;
//   }
//   return false;
// }
// //---------------------------------------------------------------------------------------------------------------------