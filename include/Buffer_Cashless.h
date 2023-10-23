#include "Stream.h"
#include <iostream>
using namespace std;

#define  Info_MQ_AFT 1
#define   Interroga_Registro 2
#define  Buffer_registro_Mq_ 3
#define Asset_Pos_Id_        4
#define Buffer_RX_AFT_       5


class Buffer_RX_AFT
{

private: // Variables Privadas Buffer
  char Info_MQ_AFT_[38];
  char Interrog_registro_ [258];
  char Buffer_registro_Mq [128];
  char Asset_Pos_Id[ 8 ];
  char Buffer_RX_AFT_MQ[128];
  

public:
  
  bool Set_RX_AFT(int Filtro_buffer, char[]);  // Metodo Set buff
  char *Get_RX_AFT(int Filtro_buffer);
  bool Clear_Buffer(int Filtro_Buffer);
  bool Set_RX_AFT_2(int Filtro_buffer);
};


