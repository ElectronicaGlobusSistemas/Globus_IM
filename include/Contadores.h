/*
        Clase  Para la consulta de contadores en Maquina.
                        ***Metodos***
                    ->Metodo Get_Contadores
                    ->Metodo Set_Contadores
*/
#include "Stream.h"
#include <iostream>
using namespace std;
#include "Arduino.h"
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
#define Physical_Coin_In 15
#define Physical_Coin_Out 16
#define Total_Coin_Drop 17
#define Machine_Paid_Progresive_Payout 18
#define Machine_Paid_External_Bonus_Payout 19
#define Attendant_Paid_Progresive_Payout 20
#define Attendant_Paid_External_Bonus_Payout 21
#define Ticket_In 22
#define Ticket_Out 23
#define Current_Credits 24
#define Door_Open 25
#define Games_Since_Last_Power_Up 26
#define Informacion_Maquina 27
#define ROM_Signature 28
#define Billetes_2k 29
#define Billetes_5k 30
#define Billetes_10k 31
#define Billetes_20k 32
#define Billetes_50k 33
#define Premio_1B 34
#define Serie_Trama 50
#define Copia_Cancel_Credit 53
#define Copia_Bill_Amount_2 55

#define Copia_Bill_Amount 54


#define Legacy_Bonus_Awards 55
//---------------------------------------->Clase Manejo de Contadores<-------------------------------------------------
class Contadores_SAS
{

private: // Variables Privadas Para contadores
  char Copia_Cancel_Credit_[9]={'0', '0', '0', '0', '0', '0', '0', '0'};
  char Total_Cancel_Credit_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Coin_In_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Coin_Out_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Jackpot_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Total_Drop_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Cancel_Credit_Hand_Pay_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Bill_Amount_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Casheable_In_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Casheable_Restricted_In_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Casheable_NONrestricted_In_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Casheable_Out_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Casheable_Restricted_Out_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Casheable_NONrestricted_Out_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Games_Played_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Physical_Coin_In_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Physical_Coin_Out_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Total_Coin_Drop_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Machine_Paid_Progresive_Payout_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Machine_Paid_External_Bonus_Payout_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Attendant_Paid_Progresive_Payout_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Attendant_Paid_External_Bonus_Payout_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Ticket_In_[11] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
  char Ticket_Out_[11] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
  char Current_Credits_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Door_Open_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Games_Since_Last_Power_Up_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Serie_Trama_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Informacion_Maquina_[20] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
  char ROM_Signature_[2] = {'0', '0'};
  char Billetes_2k_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Billetes_5k_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Billetes_10k_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Billetes_20k_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Billetes_50k_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Premio_1B_[11] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
  char Copia_Bill_Amount_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  char Copia_Bill_Amount_2_[9] = {'0', '0', '0', '0', '0', '0', '0', '0'};

  int Type_Legacy_Bonus_Awards_=0;
  char Legacy_Bonus_Awards_[4]={'0', '0', '0', '0'};

  byte ID_Operador[8]={'0', '0', '0', '0','0','0','0','0'};
  byte ID_Client[8]={'0', '0', '0', '0','0','0','0','0'};
  byte ID_Client_Temp[8]={'0', '0', '0', '0','0','0','0','0'};
  byte ID_Operador_Temp[8]={'0', '0', '0', '0','0','0','0','0'};
  byte ID_Consulta_Info_Operador[8]={'0', '0', '0', '0','0','0','0','0'};

  char Vacio;

  int Serie_Trama_Int = 0;

public:
  char *Get_Contadores_Char(int Filtro_Contador);    // Metodos Publicos
  int Get_Contadores_Int(int Filtro_Contador);       // Metodo Get Contadores
  string Get_Contadores_String(int Filtro_Contador); // Metodo Get Contadores
  bool Set_Contadores(int Filtro_Contador, char[]);  // Metodo Set Contadores
  bool Incrementa_Serie_Trama();                     // Metodo Set Contadores


  bool Set_Operador_ID(char Operador[]);
  byte* Get_Operador_ID(void);
  bool Close_ID_Operador(void);
  bool Set_Operador_ID_RFID(char Operador[]);

  bool Set_Client_ID(byte Cliente[]);
  byte* Get_Client_ID(void);
  bool Close_ID_Client(void);


  bool Set_ID_Cliente_Temp(byte Cliente[]);
  byte* Get_Client_ID_Temp(void);
  bool Close_ID_Client_Temp(void);
  bool Verify_Client(void);
  bool Verity_ID_NOT_NULL(byte Buffer[],char Type);
  bool Verify_Client_ID(byte Buffer_Tarjeta[]);

  bool Copy_Operator_In_(void);
  bool Set_Operador_ID_Temp(char Operador[]);
  bool Delete_Operator_ID_Temp();
  bool Set_Operador_ID_Temp_APP(char Operador_[]);
  bool Verify_Close(byte Cliente_Operador[]);
  bool Verify_ID_Op(void);
  bool ID_Consulta_INFO_Operador(char Operador[]);
  bool Dele_Operador_INFO_Operador(void);
  byte* Get_Operador_INFO_Operador(void);




  /* Bonus*/
  bool Set_Meter_Legacy_Bonus_Awards(char res[]);
  bool Delete_Meter_Legacy_Bonus_Awards(void);
  int Get_Type_Legacy_Bonus_Awards(void);
  bool Type_Legacy_Bonus_Awards(char res[]);
  char*  Get_Amount_Legacy_Bonus_Awards(void);
  bool Set_Type_Legacy_Bonus(char res[]);


  bool Init_Parameter_Update(char res[]);
};