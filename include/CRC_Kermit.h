#include "CRC.h"
#include <iostream>

bool Verifica_CRC(char str_CRC[], int len_CRC)
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