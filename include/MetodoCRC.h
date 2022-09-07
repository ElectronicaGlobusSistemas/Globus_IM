#include "CRC.h"
#include <iostream>

class MetodoCRC
{
private:
    // int len, len_CRC;
    // uint16_t CRC_Recibido, CRC_Temp, crcval;
    // char *indice;

public:
    bool Verifica_CRC_Wifi(String str_CRC);
    bool Verifica_CRC_Maq(char str_CRC[], int len_CRC);
    bool Verifica_CRC_Mecanicas(char *str_CRC, unsigned char len_CRC);
    char *Calcula_CRC_Wifi(char buffer[]);
    bool Calcula_CRC_Maq(char buffer[]);
};