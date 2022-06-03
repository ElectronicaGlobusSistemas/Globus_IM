#include <iostream>
#include "Contadores.h"

using namespace std;

int Convert_Char_To_Int(char buffer[]);

char *Contadores_SAS::Get_Contadores_Char(int Filtro_Contador)
{
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
  default:
    return 0x00;
    break;
  }
}

int Contadores_SAS::Get_Contadores_Int(int Filtro_Contador)
{
  int res = 0;
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
  case 3: // Bloque de instrucciones 3;
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
  }
  return false;
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