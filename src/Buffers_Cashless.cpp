#include <iostream>
#include "Buffer_Cashless.h"
#include "Arduino.h"
#include "WiFi.h"

using namespace std;

bool Buffer_RX_AFT::Set_RX_AFT(int Filtro_buffer, char Buffer[])
{
 
  switch (Filtro_buffer) // Selecciona Contador Especifico.
  {
  case 1: // Bloque de instrucciones 1;
    memcpy(Info_MQ_AFT_,Buffer, sizeof(Info_MQ_AFT_) / sizeof(Info_MQ_AFT_[0]));
    return true;
    break;

  case 2:
    memcpy(Interrog_registro_, Buffer, sizeof(Interrog_registro_) / sizeof(Interrog_registro_[0]));
    return true;
    break;

  case 3: 
    memcpy(Buffer_registro_Mq, Buffer, sizeof(Buffer_registro_Mq) / sizeof(Buffer_registro_Mq[0]));
      return true;
      break;

  case 4:
    Asset_Pos_Id[0]=Buffer[4];
    Asset_Pos_Id[1]=Buffer[5];
    Asset_Pos_Id[2]=Buffer[6];
    Asset_Pos_Id[3]=Buffer[7];
    Asset_Pos_Id[4]=Buffer[28];
    Asset_Pos_Id[5]=Buffer[29];
    Asset_Pos_Id[6]=Buffer[30];
    Asset_Pos_Id[7]=Buffer[31];
    return true;
    break;

  case 5:
    memcpy(Buffer_RX_AFT_MQ, Buffer, sizeof(Buffer_RX_AFT_MQ) / sizeof(Buffer_RX_AFT_MQ[0]));
    return true;
    break;
  }
}

char* Buffer_RX_AFT::Get_RX_AFT(int Filtro_buffer)
{
  switch (Filtro_buffer) // Selecciona Contador Especifico.
  {
  case 1: // Bloque de instrucciones 1;
    return Info_MQ_AFT_;
    break;

  case 2:
    return Interrog_registro_;
    break;

  case 3: 
    return Buffer_registro_Mq;
    break;

  case 4:
    return Asset_Pos_Id;
    break;

  case 5:
    return Buffer_RX_AFT_MQ;
    break;
  }
}

bool Buffer_RX_AFT::Clear_Buffer(int Filtro_buffer)
{

  switch (Filtro_buffer) // Selecciona Contador Especifico.
  {
 

  case 3: 
    for (int i=0; i<128; i++)
    {
      Buffer_registro_Mq[i]=0xAA;
    }
    return true;
    break;
  }

}

void macStringToByteArray(String mac, char* bytes) {
  for (int i = 0; i < 6; i++) {
    bytes[i] = strtoul(mac.substring(i*3, i*3+2).c_str(), NULL, 16);
  }
}


bool Buffer_RX_AFT::Set_RX_AFT_2(int Filtro_buffer)
{
   String MAC;
  char Mac_ESP[6];

  switch (Filtro_buffer)
  {
  case 6:
     
      MAC=WiFi.macAddress();
      macStringToByteArray(MAC,Mac_ESP);
      /*Asset*/
      Buffer_registro_Mq[0]=Asset_Pos_Id[0];
      Buffer_registro_Mq[1]=Asset_Pos_Id[1];
      Buffer_registro_Mq[2]=Asset_Pos_Id[2];
      Buffer_registro_Mq[3]=Asset_Pos_Id[3];
      /*POS ID*/
      Buffer_registro_Mq[4]=Asset_Pos_Id[4];
      Buffer_registro_Mq[5]=Asset_Pos_Id[5];
      Buffer_registro_Mq[6]=Asset_Pos_Id[6];
      Buffer_registro_Mq[7]=Asset_Pos_Id[7];
      /* Dirrecicon MAC */
      Buffer_registro_Mq[8]=Mac_ESP[0];
      Buffer_registro_Mq[9]=Mac_ESP[1];
      Buffer_registro_Mq[10]=Mac_ESP[2];
      Buffer_registro_Mq[11]=Mac_ESP[3];
      Buffer_registro_Mq[12]=Mac_ESP[4];
      Buffer_registro_Mq[13]=Mac_ESP[5];

      Buffer_registro_Mq[14]=0xC1;
      Buffer_registro_Mq[15]=0xC1;
      Buffer_registro_Mq[16]=0xC1;
      Buffer_registro_Mq[17]=0xC1;
      Buffer_registro_Mq[18]=0xC1;
      Buffer_registro_Mq[19]=0xC1;

      /*--->Fecha Actual<---*/
      return true;
      break;
  
  default:
    return false;
    break;
  }
}
