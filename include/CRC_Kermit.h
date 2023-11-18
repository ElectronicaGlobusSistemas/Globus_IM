#include "CRC.h"
#include <iostream>

extern char buffer_envio[258];
extern char Data_TX_AFT[33];
extern char Data_TX[8];
bool Verifica_CRC_Wifi(String str_CRC)
{
  int len, len_CRC;
  uint16_t CRC_Recibido, CRC_Temp, crcval;
  char *indice;

  uint8_t *data = (uint8_t *)&str_CRC[0];
  len_CRC = str_CRC.length();
  //  Serial.println(len_CRC);
  len = (len_CRC - 2);
  Serial.print("\nCRC-16/KERMIT:\t");

  //                            buf Polynome  Init   XorOut  RefIn RefOut
  //                            len  0x1021  0x0000  0x0000  true  true
  // Serial.println(crc16(data, len, 0x1021, 0x0000, 0x0000, true, true), HEX);
  crcval = crc16(data, len, 0x1021, 0x0000, 0x0000, true, true);
  Serial.println(crcval, HEX);

  indice = &str_CRC[len_CRC - 1]; // CRC recibido parte alta
  CRC_Temp = *indice;
  CRC_Temp = CRC_Temp << 8;
  // Serial.println(CRC_Temp, HEX);

  indice = &str_CRC[len_CRC - 2]; // CRC recibido parte baja
  CRC_Recibido = *indice;
  // Serial.println(CRC_Recibido, HEX);

  CRC_Recibido = CRC_Temp | CRC_Recibido; // CRC_Recibido contiene el CRC recibido en la trama de contadores maquina

  Serial.print("CRC-16/MAQUINA:\t");
  Serial.println(CRC_Recibido, HEX);

  Serial.println("\nComparando CRC...");

  if (CRC_Recibido == crcval)
  {
    Serial.println("\nCRC OK...");
    return true;
  }
  else
  {
    Serial.println("\nCRC Failed...");
    return false;
  }
}

bool Verifica_CRC_Maq(char str_CRC[], int len_CRC)
{
  int len;
  uint16_t CRC_Recibido, CRC_Temp, crcval;
  char *indice;

  uint8_t *data = (uint8_t *)&str_CRC[0];
  //  Serial.println(len_CRC);
  len = (len_CRC - 2);
  //  Serial.print("\nCRC-16/KERMIT:\t");

  //                            buf Polynome  Init   XorOut  RefIn RefOut
  //                            len  0x1021  0x0000  0x0000  true  true
  // Serial.println(crc16(data, len, 0x1021, 0x0000, 0x0000, true, true), HEX);
  crcval = crc16(data, len, 0x1021, 0x0000, 0x0000, true, true);
  //  Serial.println(crcval, HEX);

  indice = &str_CRC[len_CRC - 1]; // CRC recibido parte alta
  CRC_Temp = *indice;
  CRC_Temp = CRC_Temp << 8;
  // Serial.println(CRC_Temp, HEX);

  indice = &str_CRC[len_CRC - 2]; // CRC recibido parte baja
  CRC_Recibido = *indice;
  // Serial.println(CRC_Recibido, HEX);

  CRC_Recibido = CRC_Temp | CRC_Recibido; // CRC_Recibido contiene el CRC recibido en la trama de contadores maquina

  //  Serial.print("CRC-16/MAQUINA:\t");
  //  Serial.println(CRC_Recibido, HEX);

  //  Serial.println("\nComparando CRC...");

  if (CRC_Recibido == crcval)
  {
    Serial.println("CRC OK...");
    return true;
  }
  else
  {
    //    Serial.println("\nCRC Failed...");
    return false;
  }
}

bool Calcula_CRC_Wifi()
{
  uint16_t crcval, CRC_Temp, CRC_RES;
  uint8_t *data = (uint8_t *)&buffer_envio[0];
  crcval = crc16(data, 256, 0x1021, 0x0000, 0x0000, true, true);
  Serial.println(crcval, HEX);
  CRC_Temp = ( crcval & 0b1111111100000000 );
  CRC_RES = ( CRC_Temp >> 8 );
  Serial.println(CRC_RES, HEX);
  buffer_envio[ 257 ] = CRC_RES;
  CRC_RES = ( crcval & 0b0000000011111111 );
  Serial.println(CRC_RES, HEX);
  buffer_envio[ 256 ] = CRC_RES;
  return true;
}


bool calcularCRC_Registro_AFT(void) {
  uint16_t crcval, CRC_Temp, CRC_RES;
  uint8_t *data = (uint8_t *)&Data_TX_AFT[0];
  crcval = crc16(data, 32, 0x1021, 0x0000, 0x0000, true, true);
  //Serial.println(crcval, HEX);
  CRC_Temp = (crcval & 0xFF00);
  CRC_RES = (CRC_Temp >> 8);
 // Serial.println(CRC_RES, HEX);
  Data_TX_AFT[33] = CRC_RES;
  CRC_RES = (crcval & 0x00FF);
  //Serial.println(CRC_RES, HEX);
  Data_TX_AFT[32] = CRC_RES;
  return true;
}


bool CalcularCRC_Datos(void) {
  uint16_t crcval, CRC_Temp, CRC_RES;
  uint8_t *data = (uint8_t *)&Data_TX[0];
  crcval = crc16(data, 7, 0x1021, 0x0000, 0x0000, true, true);
  //Serial.println(crcval, HEX);
  CRC_Temp = (crcval & 0xFF00);
  CRC_RES = (CRC_Temp >> 8);
 // Serial.println(CRC_RES, HEX);
  Data_TX[8] = CRC_RES;
  CRC_RES = (crcval & 0x00FF);
  //Serial.println(CRC_RES, HEX);
  Data_TX[7] = CRC_RES;
  return true;

}

bool  CalcularCRC_Datos_Generic(char Datos[], int Size,char CRC[]) {
  uint16_t crcval, CRC_Temp, CRC_RES;
  uint8_t *data = (uint8_t *)&Datos[0];
  crcval = crc16(data, Size, 0x1021, 0x0000, 0x0000, true, true);
  //Serial.println(crcval, HEX);
  CRC_Temp = (crcval & 0xFF00);
  CRC_RES = (CRC_Temp >> 8);
 // Serial.println(CRC_RES, HEX);
  CRC[0] = CRC_RES;
  CRC_RES = (crcval & 0x00FF);
  //Serial.println(CRC_RES, HEX);
  CRC[1] = CRC_RES;
  return true;
}
