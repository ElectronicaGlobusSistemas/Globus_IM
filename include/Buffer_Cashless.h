#include "Stream.h"
#include <iostream>
using namespace std;

#define  Info_MQ_AFT 1




class Buffer_RX_AFT
{

private: // Variables Privadas Buffer
  char Info_MQ_AFT_[38];
  


public:
  
  bool Set_RX_AFT(int Filtro_buffer, char[]);  // Metodo Set buff
  char *Get_RX_AFT(int Filtro_buffer);
};