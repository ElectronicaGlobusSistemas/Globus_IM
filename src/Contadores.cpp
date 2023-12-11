#include <iostream>
#include "Contadores.h"
#include "Arduino.h"
#include "Json_Datos.h"
using namespace std;
#include "AutoUpdate.h"
#include "WiFi.h"
#include <HTTPClient.h>
#include <HTTPUpdate.h>

extern AutoUpdate UpdateOTA;
extern uint8_t Version_Firmware_[];
int Convert_Char_To_Int(char buffer[]);

char *Contadores_SAS::Get_Contadores_Char(int Filtro_Contador)
{

  delay(1);
  switch (Filtro_Contador)
  {
  case 1:
    return Total_Cancel_Credit_;
    break;  
  case 2:
    return Coin_In_;
    break;
  case 3:
    return Coin_Out_;
    break;
  case 4:
    return Jackpot_;
    break;
  case 5:
    return Total_Drop_;
    break;
  case 6:
    return Cancel_Credit_Hand_Pay_;
    break;
  case 7:
    return Bill_Amount_;
    break;
  case 8:
    return Casheable_In_;
    break;
  case 9:
    return Casheable_Restricted_In_;
    break;
  case 10:
    return Casheable_NONrestricted_In_;
    break;
  case 11:
    return Casheable_Out_;
    break;
  case 12:
    return Casheable_Restricted_Out_;
    break;
  case 13:
    return Casheable_NONrestricted_Out_;
    break;
  case 14:
    return Games_Played_;
    break;
  case 15:
    return Physical_Coin_In_;
    break;
  case 16:
    return Physical_Coin_Out_;
    break;
  case 17:
    return Total_Coin_Drop_;
    break;
  case 18:
    return Machine_Paid_Progresive_Payout_;
    break;
  case 19:
    return Machine_Paid_External_Bonus_Payout_;
    break;
  case 20:
    return Attendant_Paid_Progresive_Payout_;
    break;
  case 21:
    return Attendant_Paid_External_Bonus_Payout_;
    break;
  case 22:
    return Ticket_In_;
    break;
  case 23:
    return Ticket_Out_;
    break;
  case 24:
    return Current_Credits_;
    break;
  case 25:
    return Door_Open_;
    break;
  case 26:
    return Games_Since_Last_Power_Up_;
    break;
  case 27:
    return Informacion_Maquina_;
    break;
  case 28:
    return ROM_Signature_;
    break;
  case 29:
    return Billetes_2k_;
    break;
  case 30:
    return Billetes_5k_;
    break;
  case 31:
    return Billetes_10k_;
    break;
  case 32:
    return Billetes_20k_;
    break;
  case 33:
    return Billetes_50k_;
    break;
  case 34:
    return Premio_1B_;
    break;
  case 50:
    return Serie_Trama_;
    break;
  case 53:
    return Copia_Cancel_Credit_;
    break;

  case 54:
    return Copia_Bill_Amount_;
    break;

  default:
    return 0x00;
    break;
  }
}

int Contadores_SAS::Get_Contadores_Int(int Filtro_Contador)
{
  int res;
  switch (Filtro_Contador) // Selecciona Contador Especifico.
  {
  case 1: // Bloque de instrucciones 1;
    res = Convert_Char_To_Int(Total_Cancel_Credit_);
    return res;
    break;
  case 2: // Bloque de instrucciones 2;
    res = Convert_Char_To_Int(Coin_In_);
    return res;
    break;
  case Coin_Out: // Bloque de instrucciones 3;
    res = Convert_Char_To_Int(Coin_Out_);
    return res;
    break;
  case 4: // Bloque de instrucciones 4;
    res = Convert_Char_To_Int(Jackpot_);
    return res;
    break;
  case 5: // Bloque de instrucciones 5;
    res = Convert_Char_To_Int(Total_Drop_);
    return res;
    break;
  case 6: // Bloque de instrucciones 6;
    res = Convert_Char_To_Int(Cancel_Credit_Hand_Pay_);
    return res;
    break;
  case 7: // Bloque de instrucciones 7;
    res = Convert_Char_To_Int(Bill_Amount_);
    return res;
    break;
  case 8: // Bloque de instrucciones 8;
    res = Convert_Char_To_Int(Casheable_In_);
    return res;
    break;
  case 9: // Bloque de instrucciones 9;
    res = Convert_Char_To_Int(Casheable_Restricted_In_);
    return res;
    break;
  case 10: // Bloque de instrucciones 10;
    res = Convert_Char_To_Int(Casheable_NONrestricted_In_);
    return res;
    break;
  case 11: // Bloque de instrucciones 11;
    res = Convert_Char_To_Int(Casheable_Out_);
    return res;
    break;
  case 12: // Bloque de instrucciones 12;
    res = Convert_Char_To_Int(Casheable_Restricted_Out_);
    return res;
    break;
  case 13: // Bloque de instrucciones 13;
    res = Convert_Char_To_Int(Casheable_NONrestricted_Out_);
    return res;
    break;
  case 14: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Games_Played_);
    return res;
    break;
  case 15: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Physical_Coin_In_);
    return res;
    break;
  case 16: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Physical_Coin_Out_);
    return res;
    break;
  case 17: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Total_Coin_Drop_);
    return res;
    break;
  case 18: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Machine_Paid_Progresive_Payout_);
    return res;
    break;
  case 19: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Machine_Paid_External_Bonus_Payout_);
    return res;
    break;
  case 20: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Attendant_Paid_Progresive_Payout_);
    return res;
    break;
  case 21: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Attendant_Paid_External_Bonus_Payout_);
    return res;
    break;
  case 22: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Ticket_In_);
    return res;
    break;
  case 23: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Ticket_Out_);
    return res;
    break;
  case 24: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Current_Credits_);
    return res;
    break;
  case 25: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Door_Open_);
    return res;
    break;
  case 26: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Games_Since_Last_Power_Up_);
    return res;
    break;
  case 29: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Billetes_2k_);
    return res;
    break;
  case 30: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Billetes_5k_);
    return res;
    break;
  case 31: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Billetes_10k_);
    return res;
    break;
  case 32: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Billetes_20k_);
    return res;
    break;
  case 33: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Billetes_50k_);
    return res;
    break;
  case 34: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Premio_1B_);
    return res;
    break;
  case 50: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Serie_Trama_);
    return res;
    break;
  case 53: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Copia_Cancel_Credit_);
    return res;
    break;
  }
  return 0;
}

string Contadores_SAS::Get_Contadores_String(int Filtro_Contador)
{
  int res = 0;
  switch (Filtro_Contador) // Selecciona Contador Especifico.
  {
  case 1: // Bloque de instrucciones 1;
    res = Convert_Char_To_Int(Total_Cancel_Credit_);
    return std::to_string(res);
    break;
  case 2: // Bloque de instrucciones 2;
    res = Convert_Char_To_Int(Coin_In_);
    return std::to_string(res);
    break;
  case 3: // Bloque de instrucciones 3;
    res = Convert_Char_To_Int(Coin_Out_);
    return std::to_string(res);
    break;
  case 4: // Bloque de instrucciones 4;
    res = Convert_Char_To_Int(Jackpot_);
    return std::to_string(res);
    break;
  case 5: // Bloque de instrucciones 5;
    res = Convert_Char_To_Int(Total_Drop_);
    return std::to_string(res);
    break;
  case 6: // Bloque de instrucciones 6;
    res = Convert_Char_To_Int(Cancel_Credit_Hand_Pay_);
    return std::to_string(res);
    break;
  case 7: // Bloque de instrucciones 7;
    res = Convert_Char_To_Int(Bill_Amount_);
    return std::to_string(res);
    break;
  case 8: // Bloque de instrucciones 8;
    res = Convert_Char_To_Int(Casheable_In_);
    return std::to_string(res);
    break;
  case 9: // Bloque de instrucciones 9;
    res = Convert_Char_To_Int(Casheable_Restricted_In_);
    return std::to_string(res);
    break;
  case 10: // Bloque de instrucciones 10;
    res = Convert_Char_To_Int(Casheable_NONrestricted_In_);
    return std::to_string(res);
    break;
  case 11: // Bloque de instrucciones 11;
    res = Convert_Char_To_Int(Casheable_Out_);
    return std::to_string(res);
    break;
  case 12: // Bloque de instrucciones 12;
    res = Convert_Char_To_Int(Casheable_Restricted_Out_);
    return std::to_string(res);
    break;
  case 13: // Bloque de instrucciones 13;
    res = Convert_Char_To_Int(Casheable_NONrestricted_Out_);
    return std::to_string(res);
    break;
  case 14: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Games_Played_);
    return std::to_string(res);
    break;
  case 15: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Physical_Coin_In_);
    return std::to_string(res);
    break;
  case 16: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Physical_Coin_Out_);
    return std::to_string(res);
    break;
  case 17: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Total_Coin_Drop_);
    return std::to_string(res);
    break;
  case 18: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Machine_Paid_Progresive_Payout_);
    return std::to_string(res);
    break;
  case 19: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Machine_Paid_External_Bonus_Payout_);
    return std::to_string(res);
    break;
  case 20: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Attendant_Paid_Progresive_Payout_);
    return std::to_string(res);
    break;
  case 21: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Attendant_Paid_External_Bonus_Payout_);
    return std::to_string(res);
    break;
  case 22: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Ticket_In_);
    return std::to_string(res);
    break;
  case 23: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Ticket_Out_);
    return std::to_string(res);
    break;
  case 24: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Current_Credits_);
    return std::to_string(res);
    break;
  case 25: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Door_Open_);
    return std::to_string(res);
    break;
  case 26: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Games_Since_Last_Power_Up_);
    return std::to_string(res);
    break;
  case 29: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Billetes_2k_);
    return std::to_string(res);
    break;
  case 30: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Billetes_5k_);
    return std::to_string(res);
    break;
  case 31: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Billetes_10k_);
    return std::to_string(res);
    break;
  case 32: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Billetes_20k_);
    return std::to_string(res);
    break;
  case 33: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Billetes_50k_);
    return std::to_string(res);
    break;
  case 34: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Premio_1B_);
    return std::to_string(res);
    break;
  case 50: // Bloque de instrucciones 14;
    res = Convert_Char_To_Int(Serie_Trama_);
    return std::to_string(res);
    break;
  default:
    return std::to_string(0);
    break;
  }
}

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------->Metodo Para Guardar Contadores<---------------------------------------------
bool Contadores_SAS::Set_Contadores(int Filtro_Contador, char Data_Contador[])
{
  switch (Filtro_Contador) // Selecciona Contador Especifico.
  {
  case 1: // Bloque de instrucciones 1;
    memcpy(Total_Cancel_Credit_, Data_Contador, sizeof(Total_Cancel_Credit_) / sizeof(Total_Cancel_Credit_[0]));
    return true;
    break;
  case 2: // Bloque de instrucciones 2;
    memcpy(Coin_In_, Data_Contador, sizeof(Coin_In_) / sizeof(Coin_In_[0]));
    return true;
    break;
  case 3: // Bloque de instrucciones 3;
    memcpy(Coin_Out_, Data_Contador, sizeof(Coin_Out_) / sizeof(Coin_Out_[0]));
    return true;
    break;
  case 4: // Bloque de instrucciones 4;
    memcpy(Jackpot_, Data_Contador, sizeof(Jackpot_) / sizeof(Jackpot_[0]));
    return true;
    break;
  case 5: // Bloque de instrucciones 5;
    memcpy(Total_Drop_, Data_Contador, sizeof(Total_Drop_) / sizeof(Total_Drop_[0]));
    return true;
    break;
  case 6: // Bloque de instrucciones 6;
    memcpy(Cancel_Credit_Hand_Pay_, Data_Contador, sizeof(Cancel_Credit_Hand_Pay_) / sizeof(Cancel_Credit_Hand_Pay_[0]));
    return true;
    break;
  case 7: // Bloque de instrucciones 7;
    memcpy(Bill_Amount_, Data_Contador, sizeof(Bill_Amount_) / sizeof(Bill_Amount_[0]));
    return true;
    break;
  case 8: // Bloque de instrucciones 8;
    memcpy(Casheable_In_, Data_Contador, sizeof(Casheable_In_) / sizeof(Casheable_In_[0]));
    return true;
    break;
  case 9: // Bloque de instrucciones 9;
    memcpy(Casheable_Restricted_In_, Data_Contador, sizeof(Casheable_Restricted_In_) / sizeof(Casheable_Restricted_In_[0]));
    return true;
    break;
  case 10: // Bloque de instrucciones 10;
    memcpy(Casheable_NONrestricted_In_, Data_Contador, sizeof(Casheable_NONrestricted_In_) / sizeof(Casheable_NONrestricted_In_[0]));
    return true;
    break;
  case 11: // Bloque de instrucciones 11;
    memcpy(Casheable_Out_, Data_Contador, sizeof(Casheable_Out_) / sizeof(Casheable_Out_[0]));
    return true;
    break;
  case 12: // Bloque de instrucciones 12;
    memcpy(Casheable_Restricted_Out_, Data_Contador, sizeof(Casheable_Restricted_Out_) / sizeof(Casheable_Restricted_Out_[0]));
    return true;
    break;
  case 13: // Bloque de instrucciones 13;
    memcpy(Casheable_NONrestricted_Out_, Data_Contador, sizeof(Casheable_NONrestricted_Out_) / sizeof(Casheable_NONrestricted_Out_[0]));
    return true;
    break;
  case 14: // Bloque de instrucciones 13;
    memcpy(Games_Played_, Data_Contador, sizeof(Games_Played_) / sizeof(Games_Played_[0]));
    return true;
    break;
  case 15: // Bloque de instrucciones 13;
    memcpy(Physical_Coin_In_, Data_Contador, sizeof(Physical_Coin_In_) / sizeof(Physical_Coin_In_[0]));
    return true;
    break;
  case 16: // Bloque de instrucciones 13;
    memcpy(Physical_Coin_Out_, Data_Contador, sizeof(Physical_Coin_Out_) / sizeof(Physical_Coin_Out_[0]));
    return true;
    break;
  case 17: // Bloque de instrucciones 13;
    memcpy(Total_Coin_Drop_, Data_Contador, sizeof(Total_Coin_Drop_) / sizeof(Total_Coin_Drop_[0]));
    return true;
    break;
  case 18: // Bloque de instrucciones 13;
    memcpy(Machine_Paid_Progresive_Payout_, Data_Contador, sizeof(Machine_Paid_Progresive_Payout_) / sizeof(Machine_Paid_Progresive_Payout_[0]));
    return true;
    break;
  case 19: // Bloque de instrucciones 13;
    memcpy(Machine_Paid_External_Bonus_Payout_, Data_Contador, sizeof(Machine_Paid_External_Bonus_Payout_) / sizeof(Machine_Paid_External_Bonus_Payout_[0]));
    return true;
    break;
  case 20: // Bloque de instrucciones 13;
    memcpy(Attendant_Paid_Progresive_Payout_, Data_Contador, sizeof(Attendant_Paid_Progresive_Payout_) / sizeof(Attendant_Paid_Progresive_Payout_[0]));
    return true;
    break;
  case 21: // Bloque de instrucciones 13;
    memcpy(Attendant_Paid_External_Bonus_Payout_, Data_Contador, sizeof(Attendant_Paid_External_Bonus_Payout_) / sizeof(Attendant_Paid_External_Bonus_Payout_[0]));
    return true;
    break;
  case 22: // Bloque de instrucciones 13;
    memcpy(Ticket_In_, Data_Contador, sizeof(Ticket_In_) / sizeof(Ticket_In_[0]));
    return true;
    break;
  case 23: // Bloque de instrucciones 13;
    memcpy(Ticket_Out_, Data_Contador, sizeof(Ticket_Out_) / sizeof(Ticket_Out_[0]));
    return true;
    break;
  case 24: // Bloque de instrucciones 13;
    memcpy(Current_Credits_, Data_Contador, sizeof(Current_Credits_) / sizeof(Current_Credits_[0]));
    return true;
    break;
  case 25: // Bloque de instrucciones 13;
    memcpy(Door_Open_, Data_Contador, sizeof(Door_Open_) / sizeof(Door_Open_[0]));
    return true;
    break;
  case 26: // Bloque de instrucciones 13;
    memcpy(Games_Since_Last_Power_Up_, Data_Contador, sizeof(Games_Since_Last_Power_Up_) / sizeof(Games_Since_Last_Power_Up_[0]));
    return true;
    break;
  case 27: // Bloque de instrucciones 13;
    memcpy(Informacion_Maquina_, Data_Contador, sizeof(Informacion_Maquina_) / sizeof(Informacion_Maquina_[0]));
    return true;
    break;
  case 28: // Bloque de instrucciones 13;
    memcpy(ROM_Signature_, Data_Contador, sizeof(ROM_Signature_) / sizeof(ROM_Signature_[0]));
    return true;
    break;
  case 29: // Bloque de instrucciones 13;
    memcpy(Billetes_2k_, Data_Contador, sizeof(Billetes_2k_) / sizeof(Billetes_2k_[0]));
    return true;
    break;
  case 30: // Bloque de instrucciones 13;
    memcpy(Billetes_5k_, Data_Contador, sizeof(Billetes_5k_) / sizeof(Billetes_5k_[0]));
    return true;
    break;
  case 31: // Bloque de instrucciones 13;
    memcpy(Billetes_10k_, Data_Contador, sizeof(Billetes_10k_) / sizeof(Billetes_10k_[0]));
    return true;
    break;
  case 32: // Bloque de instrucciones 13;
    memcpy(Billetes_20k_, Data_Contador, sizeof(Billetes_20k_) / sizeof(Billetes_20k_[0]));
    return true;
    break;
  case 33: // Bloque de instrucciones 13;
    memcpy(Billetes_50k_, Data_Contador, sizeof(Billetes_50k_) / sizeof(Billetes_50k_[0]));
    return true;
    break;
  case 34: // Bloque de instrucciones 13;
    memcpy(Premio_1B_, Data_Contador, sizeof(Premio_1B_) / sizeof(Premio_1B_[0]));
    return true;
    break;
  case 50: // Bloque de instrucciones 13;
    memcpy(Serie_Trama_, Data_Contador, sizeof(Serie_Trama_) / sizeof(Serie_Trama_[0]));
    return true;
    break;
  
  case 53: // Bloque de instrucciones 13;
    memcpy(Copia_Cancel_Credit_, Data_Contador, sizeof(Copia_Cancel_Credit_) / sizeof(Copia_Cancel_Credit_[0]));
    return true;
    break;

   case 54: // Bloque de instrucciones 13;
    memcpy(Copia_Bill_Amount_, Data_Contador, sizeof(Copia_Bill_Amount_) / sizeof(Copia_Bill_Amount_[0]));
    return true;
    break;

   case 55:
   memcpy(Copia_Bill_Amount_2_, Data_Contador, sizeof(Copia_Bill_Amount_2_) / sizeof(Copia_Bill_Amount_2_[0]));
   return true;
   break;

   

  }
  return false;
}

//---------------------------------------------------------------------------------------------------------------------
//--------------------------------->Metodo Para Aumentar serie trama contadores<---------------------------------------
bool Contadores_SAS::Incrementa_Serie_Trama()
{
  char res[8] = {};
  bzero(res, 8);
  int unidades, descenas, centenas, uni_mil, desc_mil, cent_mil, uni_millon, desc_millon, resultado = 0;

  Serie_Trama_Int++;

  desc_millon = (Serie_Trama_Int - (Serie_Trama_Int % 10000000)) / 10000000;
  res[0] = desc_millon + '0';
  resultado = Serie_Trama_Int % 10000000;

  uni_millon = (resultado - (resultado % 1000000)) / 1000000;
  res[1] = uni_millon + '0';
  resultado = resultado % 1000000;

  cent_mil = (resultado - (resultado % 100000)) / 100000;
  res[2] = cent_mil + '0';
  resultado = resultado % 10000;

  desc_mil = (resultado - (resultado % 10000)) / 10000;
  res[3] = desc_mil + '0';
  resultado = resultado % 10000;

  uni_mil = (resultado - (resultado % 1000)) / 1000;
  res[4] = uni_mil + '0';
  resultado = resultado % 1000;

  centenas = (resultado - (resultado % 100)) / 100;
  res[5] = centenas + '0';
  resultado = resultado % 100;

  descenas = (resultado - (resultado % 10)) / 10;
  res[6] = descenas + '0';

  unidades = resultado % 10;
  res[7] = unidades + '0';

  if (Set_Contadores(Serie_Trama, res))
  {
    return true;
  }
  else
  {
    return false;
  }
}

//---------------------------------------------------------------------------------------------------------------------

int Convert_Char_To_Int(char buffer[])
{
  int resultado = ((buffer[0] - 48) * 10000000) + ((buffer[1] - 48) * 1000000) +
                  ((buffer[2] - 48) * 100000) + ((buffer[3] - 48) * 10000) +
                  ((buffer[4] - 48) * 1000) + ((buffer[5] - 48) * 100) +
                  ((buffer[6] - 48) * 10) + ((buffer[7] - 48) * 1);
  return resultado;
}



bool Contadores_SAS::Set_ID_Cliente_Temp(byte Cliente_t[])
{
  for (int i = 0; i < 8; i++)
  {
   ID_Client_Temp[i]='0';
  }
  ID_Client_Temp[0]=Cliente_t[0];
  ID_Client_Temp[1]=Cliente_t[1];
  ID_Client_Temp[2]=Cliente_t[2];
  ID_Client_Temp[3]=Cliente_t[3];
  ID_Client_Temp[4]=Cliente_t[4];
  ID_Client_Temp[5]=Cliente_t[5];
  ID_Client_Temp[6]=Cliente_t[6];
  ID_Client_Temp[7]=Cliente_t[7];
  
  for (int i = 0; i < 8; i++)
  {
      if(ID_Client_Temp[i]==NULL)
      {
        return false;
      }
  }
  return true;
}

bool Contadores_SAS::Close_ID_Client_Temp(void)
{
  /* Elimina ID Cliente Cashless*/
  for (int i = 0; i < 8; i++)
  {
    ID_Client_Temp[i] = '0';
  }
  /*Verifica */
  for (int i = 0; i < 8; i++)
  {
    if (ID_Client_Temp[i] != 48)
    {
      return false;
    }
  }

  return true;
}

byte* Contadores_SAS::Get_Client_ID_Temp(void)
{
  return ID_Client_Temp;
}

bool  Contadores_SAS::Verify_Client(void)
{
  delay(50);
  unsigned int Verify=0;
  for (int i = 0; i < 8; i++)
  {
    if (ID_Client_Temp[i] == 48)
    {
      Verify++;
    }
  }
  
  /* Cliente 00000000*/
  if(Verify>=8)
  {
    return false; /* Buffer de tarjeta vacio*/
  }

  /*00000001*/
  else if(Verify<8)
  {
    return true; /* Buffer de tarjeta valido*/
  }
}

/* Verifica si el  buffer temporal de datos de la Tarjeta ID contiene un usuario no valido 00000000*/
bool  Contadores_SAS::Verify_Client_ID(byte Buffer_Tarjeta[])
{
  delay(50);
  unsigned int Verify_Cliente=0;
  for (int i = 0; i < 8; i++)
  {
    if (Buffer_Tarjeta[i] == 48)
    {
      Verify_Cliente++;
    }
  }
  
  /* Cliente 00000000*/
  if(Verify_Cliente>=8)
  {
    return false; /* Buffer de tarjeta vacio*/
  }

  /*00000001*/
  else if(Verify_Cliente<8)
  {
    return true; /* Buffer de tarjeta valido*/
  }
}
/* Verifica si  en las espacios 1 y 4 de la tarjeta RFID contiene datos Nulos*/
bool Contadores_SAS::Verity_ID_NOT_NULL(byte Buffer[],char Type)
{
  delay(50);

  if (Type == 'C' || Type == 'O')
  {
    if (Buffer[0] == NULL || Buffer[0] == 48)
    {
      return false;
    }

    return true;
  }
  else
  {
    for (int i = 0; i < 8; i++)
    {
      if (Buffer[i] == NULL)
      {
        return false;
      }
    }

    return true; /* Buffer de tarjeta valido*/
  }
}
bool Contadores_SAS::Set_Operador_ID_Temp_APP(char Operador_[])
{
   for (int i = 0; i < 8; i++)
  {
   ID_Operador_Temp[i]='0';
  }

  if(Operador_[4]==NULL||Operador_[5]==NULL||Operador_[6]==NULL||Operador_[7]==NULL||Operador_[8]==NULL||Operador_[9]==NULL||Operador_[10]==NULL||Operador_[11]==NULL)
  {
   for (int i = 0; i < 8; i++)
   {
     ID_Operador_Temp[i] = '0';
   }
   return false;
  }else{
   ID_Operador_Temp[0] = Operador_[4];
   ID_Operador_Temp[1] = Operador_[5];
   ID_Operador_Temp[2] = Operador_[6];
   ID_Operador_Temp[3] = Operador_[7];
   ID_Operador_Temp[4] = Operador_[8];
   ID_Operador_Temp[5] = Operador_[9];
   ID_Operador_Temp[6] = Operador_[10];
   ID_Operador_Temp[7] = Operador_[11];
  }
 
  return true;
}
bool Contadores_SAS:: Set_Operador_ID_Temp(char Operador_[])
{
  int contador=0;
  for (int i = 0; i < 8; i++)
  {
    ID_Operador_Temp[i] = '0';
  }
  /*Limpia y escribe  el ID operador */
  ID_Operador_Temp[0] = Operador_[0];
  ID_Operador_Temp[1] = Operador_[1];
  ID_Operador_Temp[2] = Operador_[2];
  ID_Operador_Temp[3] = Operador_[3];
  ID_Operador_Temp[4] = Operador_[4];
  ID_Operador_Temp[5] = Operador_[5];
  ID_Operador_Temp[6] = Operador_[6];
  ID_Operador_Temp[7] = Operador_[7];

  for(int i=0; i<8;i++)
  {
    if(ID_Operador_Temp[i]==Operador_[i])
    {
      contador++;
    }
  }
  
  if(contador>=7)
  {
    return true;
  }
  return false;
}

bool Contadores_SAS::ID_Consulta_INFO_Operador(char Operador[])
{
  int Val=0;
  for(int i=0; i<8;i++)
  {
    ID_Consulta_Info_Operador[i]=Operador[i];
  }

  for(int i=0; i<8;i++)
  {
    if(ID_Consulta_Info_Operador[i]==Operador[i])
    {
      Val++;
    }
  }

  if(Val>=8)
  {
    return true;
  }else{
    return false;
  }

  return false;
}

bool Contadores_SAS::Dele_Operador_INFO_Operador(void)
{
  for(int i=0; i<8; i++)
  {
    ID_Consulta_Info_Operador[i]='0';
  }

  for(int i=0; i<8; i++)
  {
    if(ID_Consulta_Info_Operador[i]!='0')
    {
      return false;
    }
  }

  return true;
}

byte* Contadores_SAS::Get_Operador_INFO_Operador(void)
{
  return ID_Consulta_Info_Operador;
}

/* Agrega ID de operador en trama 
return (false) Si no se agrego el ID  a la trama de contadores
return (true) Si  se agrego el ID a la trama de contadores
*/
bool Contadores_SAS:: Copy_Operator_In_(void)
{
  int contador=0;
  ID_Operador[0]=ID_Operador_Temp[0];
  ID_Operador[1]=ID_Operador_Temp[1];
  ID_Operador[2]=ID_Operador_Temp[2];
  ID_Operador[3]=ID_Operador_Temp[3];
  ID_Operador[4]=ID_Operador_Temp[4];
  ID_Operador[5]=ID_Operador_Temp[5];
  ID_Operador[6]=ID_Operador_Temp[6];
  ID_Operador[7]=ID_Operador_Temp[7];

  for(int i=0; i<8;i++)
  {
    if(ID_Operador[i]==ID_Operador_Temp[i])
    {
      contador++;
    }
  }
  
  if(contador>=7)
  {
    return true;
  }
  return false;
}

/* Limpia ID operador temporal  00000000 */
bool Contadores_SAS::Delete_Operator_ID_Temp(void)
{
  for (int i = 0; i < 8; i++)
  {
   ID_Operador_Temp[i] = '0';
  }

  if(ID_Operador_Temp[0]==48&&ID_Operador_Temp[1]==48&&ID_Operador_Temp[2]==48&&ID_Operador_Temp[3]==48&&ID_Operador_Temp[4]==48&&ID_Operador_Temp[5]==48&&ID_Operador_Temp[6]==48&&ID_Operador_Temp[7]==48)
  {
    return true;
  }else{
    return false;
  }
}

/* Verifica si el buffer es diferente de 00000000
Return (True) Buffer Cliente/Operador Cerrado
Return (False) Buffer Cliente/Operador / Abierto*/
bool Contadores_SAS::Verify_Close(byte Cliente_Operador[])
{

  int Valida=0;

  for(int i=0; i<8;i++)
  {
    if(Cliente_Operador[i]==48)
    {
      Valida++;
    }
  }
  
  if(Valida>=8)
  {
    return true;
  }else{
     return false;
  }

}

/* Agrega ID operador en trama  de contadores */
bool Contadores_SAS::Set_Operador_ID(char Operador[])
{
  for (int i = 0; i < 8; i++)
  {
   ID_Operador[i]='0';
  }

  if(Operador[4]==NULL||Operador[5]==NULL||Operador[6]==NULL||Operador[7]==NULL||Operador[8]==NULL||Operador[9]==NULL||Operador[10]==NULL||Operador[11]==NULL)
  {
   for (int i = 0; i < 8; i++)
   {
     ID_Operador[i] = '0';
   }
   return false;
  }else{
   ID_Operador[0] = Operador[4];
   ID_Operador[1] = Operador[5];
   ID_Operador[2] = Operador[6];
   ID_Operador[3] = Operador[7];
   ID_Operador[4] = Operador[8];
   ID_Operador[5] = Operador[9];
   ID_Operador[6] = Operador[10];
   ID_Operador[7] = Operador[11];
  }
 
  return true;
}

bool Contadores_SAS::Set_Operador_ID_RFID(char Operador[])
{
  for (int i = 0; i < 8; i++)
  {
   ID_Operador[i]='0';
  }

  if(Operador[0]==NULL||Operador[1]==NULL||Operador[2]==NULL||Operador[3]==NULL||Operador[4]==NULL||Operador[5]==NULL||Operador[6]==NULL||Operador[7]==NULL)
  {
   for (int i = 0; i < 8; i++)
   {
     ID_Operador[i] = '0';
   }
   return false;
  }else{
   ID_Operador[0] = Operador[0];
   ID_Operador[1] = Operador[1];
   ID_Operador[2] = Operador[2];
   ID_Operador[3] = Operador[3];
   ID_Operador[4] = Operador[4];
   ID_Operador[5] = Operador[5];
   ID_Operador[6] = Operador[6];
   ID_Operador[7] = Operador[7];
  }
  return true;
}
byte *Contadores_SAS::Get_Operador_ID(void)
{
return ID_Operador;
}

bool Contadores_SAS::Close_ID_Operador(void)
{
  /* Cierra Sesión operador*/
  for (int i = 0; i < 8; i++)
  {
    ID_Operador[i] = '0';
  }

  for (int i = 0; i < 8; i++)
  {
    ID_Operador_Temp[i] = '0';
  }
  /*Verifica Cierre de Sesión Operador*/
  for (int i = 0; i < 8; i++)
  {
    if (ID_Operador[i] != 48)
    {
      return false;
    }
  }
  for (int i = 0; i < 8; i++)
  {
    if (ID_Operador_Temp[i] != 48)
    {
      return false;
    }
  }

  return true;
}

bool Contadores_SAS::Set_Client_ID(byte Cliente[])
{
  for (int i = 0; i < 8; i++)
  {
   ID_Client[i]='0';
  }
  ID_Client[0]=Cliente[0];
  ID_Client[1]=Cliente[1];
  ID_Client[2]=Cliente[2];
  ID_Client[3]=Cliente[3];
  ID_Client[4]=Cliente[4];
  ID_Client[5]=Cliente[5];
  ID_Client[6]=Cliente[6];
  ID_Client[7]=Cliente[7];
  
  for (int i = 0; i < 8; i++)
  {
      if(ID_Client[i]==NULL)
      {
        return false;
      }
  }
  return true;
}

byte *Contadores_SAS::Get_Client_ID(void)
{
return ID_Client;
}

bool Contadores_SAS::Close_ID_Client(void)
{
  /* Cierra Sesión Cliente*/
  for (int i = 0; i < 8; i++)
  {
    ID_Client[i] = '0';
  }
  /*Verifica Cierre de Sesión Cliente*/
  for (int i = 0; i < 8; i++)
  {
    if (ID_Client[i] != 48)
    {
      return false;
    }
  }

  return true;
}

/*Verifica si existe operador en trama de contadores 
Return (false) No existe operador
Return (True) Existe Operador
*/
bool Contadores_SAS::Verify_ID_Op(void)
{

  int Conteo=0;
  

  for(int i=0; i<8; i++)
  {
    if(ID_Operador[i]==48)
    {
      Conteo++;
    }
  }

  if(Conteo>=8) /*00000000*/
  {
    return false;
  }

  return true; /*00000001*/
  /* Existe ID */ 
}

byte Conve_Ascii_To_Hex_LL(char Val_Ascii) {
   
    if (Val_Ascii >= '0' && Val_Ascii <= '9') {
        return Val_Ascii - '0';
    }else {
        return 0;  // Carácter no válido
    }
}
byte Conve_Ascii_To_Hex_HH(char Val_Ascii) {
    return Conve_Ascii_To_Hex_LL(Val_Ascii) << 4;
}

int Type_Bonus(char res)
{
  if (res == '0')
  {
    return  0x00;
  }
  else if (res == '1')
  {
    return  0x01;
  }
  else if (res== '2')
  {
    return  0x02;
  }else{
    return  0x05;
  }
}


/*Configura Tipo y valor de bono Para Solicitud de carga*/
bool Contadores_SAS::Set_Meter_Legacy_Bonus_Awards(char res[])
{

  int Comp, Comp2;

  /* Tipo de bono */
  Type_Legacy_Bonus_Awards_ = Type_Bonus(res[12]);

  Comp = Conve_Ascii_To_Hex_LL(res[11]);
  Comp2 = Conve_Ascii_To_Hex_HH(res[10]);
  Legacy_Bonus_Awards_[3] = (Comp2 | Comp);

  Comp = Conve_Ascii_To_Hex_LL(res[9]);
  Comp2 = Conve_Ascii_To_Hex_HH(res[8]);
  Legacy_Bonus_Awards_[2] = (Comp2 | Comp);

  Comp = Conve_Ascii_To_Hex_LL(res[7]);
  Comp2 = Conve_Ascii_To_Hex_HH(res[6]);
  Legacy_Bonus_Awards_[1] = (Comp2 | Comp);

  Comp = Conve_Ascii_To_Hex_LL(res[5]);
  Comp2 = Conve_Ascii_To_Hex_HH(res[4]);
  Legacy_Bonus_Awards_[0] = (Comp2 | Comp);

  return true;
}

bool Contadores_SAS::Delete_Meter_Legacy_Bonus_Awards(void)
{
  Legacy_Bonus_Awards_[0] = '0';
  Legacy_Bonus_Awards_[1] = '0';
  Legacy_Bonus_Awards_[2] = '0';
  Legacy_Bonus_Awards_[3] = '0';
  Legacy_Bonus_Awards_[4] = '0';

  for (int i = 0; i < 5; i++)
  {
    if (Legacy_Bonus_Awards_[i] != '0')
      return true;
  }
  return false;
}
bool Contadores_SAS::Type_Legacy_Bonus_Awards(char res[])
{

  if (res[12] == '0')
  {
    Type_Legacy_Bonus_Awards_ = 0x00;
  }
  else if (res[12] == '1')
  {
    Type_Legacy_Bonus_Awards_ = 0x01;
  }
  else if (res[12] == '2')
  {
    Type_Legacy_Bonus_Awards_ = 0x02;
  }
  return true;
}
char *Contadores_SAS::Get_Amount_Legacy_Bonus_Awards(void)
{
  return Legacy_Bonus_Awards_;
}
int Contadores_SAS::Get_Type_Legacy_Bonus_Awards(void)

{
  return Type_Legacy_Bonus_Awards_;
}


/* Genera  y retorna token  de acceso desde la API */
String Token_Generator(String Url)
{
  String payload;
  int httpCode;
  String fwurl = Url;
  #ifdef Debug_HTTPS
  Serial.println(fwurl);
  #endif
  WiFiClient *client = new WiFiClient;
  String Output="";
  if (client)
  {

    HTTPClient https;

    if (https.begin(*client, fwurl))
    {
      #ifdef Debug_HTTPS
      Serial.print("[HTTPS] GET...\n");
      #endif
      httpCode = https.GET();
      #ifdef Debug_HTTPS
      Serial.println(httpCode);
      #endif
      if (httpCode == HTTP_CODE_OK)
      {
        payload = https.getString();

        #ifdef Debug_HTTPS
        Serial.println(payload);
        #endif

        StaticJsonDocument<1024>
            doc,
            filter;
        DeserializationError error = deserializeJson(doc, payload);

        
        if (error)
        {
          #ifdef Debug_HTTPS
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
          #endif
          return  Output; /* Vacio*/
        }
        else
        {
          // Token Generado;
          #ifdef Debug_HTTPS
          Serial.println("Token Generador! OK");
          #endif
          String Token= doc["Data"]["access_token"];
          String Token_Expires= doc["Data"]["expires"];
          bool IsSuccess = doc["IsSuccess"];
          String Message = doc["Message"];
          int Evento =doc["Evento"];

          if(IsSuccess)
          {
            return Output=Token;
          }else{
             return Output;
          }
        }
        return Output;
      }
      else
      {
        #ifdef Debug_HTTPS
        Serial.print("error in downloading version file:");
        Serial.println(httpCode);
        #endif
        return Output; /* Vacio*/
      }
      https.end();
    }
    delete client;
    client->stop();
  }

  return Output; /* Vacio*/
}
/* 
(0)--> URL recibidas y Token Generado con exito ! 
(1)--> Falla deserializando json de solicitud 
(2)--> Token No generado 
(3)--> Proceso no identificado
(4)--> Comando no identificado no se detecto json en la solicitud 
*/
int Contadores_SAS::Init_Parameter_Update(char res[])
{

  char res2[255];
  int JsonCount = 0;

  // Extraer el fragmento de JSON
  for (int i = 4; i < 258; i++)
  {
    res2[JsonCount] = res[i];
    JsonCount++;
  }

  char *finJson = strchr(res2, '}');

  // Verificar si se encontró '}' y eliminar los ceros si es necesario
  if (finJson != nullptr)
  {
    // Calcular la longitud del contenido deseado
    size_t longitud = finJson - res2 + 1;

    // Crear un nuevo array del tamaño adecuado
   // Serial.println(longitud);
    char resFinal[longitud];

    // Copiar el contenido deseado al nuevo array
    memcpy(resFinal, res2, longitud);

    resFinal[longitud] = '\0';
   // Serial.println(resFinal);

    /* Prepara Json para deserialización */
    String Json = String(resFinal);
    Json.replace("\\\"", "\"");
    Json.trim(); /* Elimina espacios */

    StaticJsonDocument<1024>
    doc, 
    filter;

    DeserializationError error = deserializeJson(doc, Json);

    // Verificar errores de deserialización
    if (error)
    {
      #ifdef Debug_HTTPS
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      #endif
      return 1; /* Falla Json Solicitud */
    }
    else
    {
      /* Url Generica */
      String URL_Generic=doc["Url"];
      /* Url Api Genera token */
      String Api_Token=doc["Api_Token"];
      /*Url Descarga de archivo*/
      String Api_Bin=doc["Api_Bin"];
      /* URL Api de respuesta */
      String Api_Res=doc["Api_Respuesta"];
      /*Url Parametrizada para generar token */
      String Url_Completa=URL_Generic+Api_Token+"?"+ "Mac="+WiFi.macAddress();
      /* Version de actualización */
      String Version_Programa=doc["Version_Act"];
      /* URL Api version */
      String Api_Version=doc["Api_Version"];

      /* Token de acceso */
      String Token_Valido= Token_Generator(Url_Completa);

    
      if(Token_Valido!="")
      {
        UpdateOTA.Init_AutoUpdate(Version_Programa, URL_Generic,Api_Version,Api_Bin, Api_Res, Token_Valido, Version_Firmware_);
        UpdateOTA.Confirmacion_ACK_HTTPS(URL_OK,RES_URL);
        return 0;
      }else{
        return 2; /* Token no generado */
      }
      return 3; /* No se identifico proceso */
    }
  }
  return 4 /* Comando no identificado no  se detecto Json */;
}