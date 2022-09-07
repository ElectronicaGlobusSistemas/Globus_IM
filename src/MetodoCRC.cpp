#include "MetodoCRC.h"

bool MetodoCRC::Verifica_CRC_Wifi(String str_CRC)
{
    int len, len_CRC;
    uint16_t CRC_Recibido, CRC_Temp, crcval;
    char *indice;

    uint8_t *data = (uint8_t *)&str_CRC[0];
    len_CRC = str_CRC.length();
    //  Serial.println(len_CRC);
    len = (len_CRC - 2);
    //    Serial.print("\nCRC-16/KERMIT:\t");

    //                            buf Polynome  Init   XorOut  RefIn RefOut
    //                            len  0x1021  0x0000  0x0000  true  true
    // Serial.println(crc16(data, len, 0x1021, 0x0000, 0x0000, true, true), HEX);
    crcval = crc16(data, len, 0x1021, 0x0000, 0x0000, true, true);
    //    Serial.println(crcval, HEX);

    indice = &str_CRC[len_CRC - 1]; // CRC recibido parte alta
    CRC_Temp = *indice;
    CRC_Temp = CRC_Temp << 8;
    // Serial.println(CRC_Temp, HEX);

    indice = &str_CRC[len_CRC - 2]; // CRC recibido parte baja
    CRC_Recibido = *indice;
    // Serial.println(CRC_Recibido, HEX);

    CRC_Recibido = CRC_Temp | CRC_Recibido; // CRC_Recibido contiene el CRC recibido en la trama de contadores maquina

    //    Serial.print("CRC-16/MAQUINA:\t");
    //    Serial.println(CRC_Recibido, HEX);

    //    Serial.println("\nComparando CRC...");

    if (CRC_Recibido == crcval)
    {
        //        Serial.println("\nCRC OK...");
        return true;
    }
    else
    {
        //        Serial.println("\nCRC Failed...");
        return false;
    }
}

bool MetodoCRC::Verifica_CRC_Maq(char str_CRC[], int len_CRC)
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
        //        Serial.println("CRC OK...");
        return true;
    }
    else
    {
        //        Serial.println("\nCRC Failed...");
        return false;
    }
}

bool MetodoCRC::Verifica_CRC_Mecanicas(char *str_CRC, unsigned char len_CRC)
{
    unsigned short i;
    uint16_t CRC_Recibido, CRC_Temp;
    unsigned short crcval = 0xffff;
    char *indice;

    indice = &str_CRC[len_CRC - 2]; // CRC recibido parte alta
    CRC_Temp = *indice;
    CRC_Temp = CRC_Temp << 8;

    indice = &str_CRC[len_CRC - 1]; // CRC recibido parte baja
    CRC_Recibido = *indice;

    CRC_Recibido = CRC_Temp | CRC_Recibido; // CRC_Recibido contiene el CRC recibido en la trama de contadores maquina

    int len = len_CRC - 2;

    while (len--)
    {
        crcval = (crcval ^ *str_CRC++);
        for (i = 0; i < 8; i++)
        {
            if (crcval & 1)
            {
                crcval = (crcval >> 1) ^ 0xA001;
            }
            else
            {
                crcval = (crcval >> 1);
            }
        }
    }

    if (CRC_Recibido == crcval)
        return true;
    else
        return false;
}

char *MetodoCRC::Calcula_CRC_Wifi(char buffer[])
{
    uint16_t crcval, CRC_Temp, CRC_RES;
    uint8_t *data = (uint8_t *)&buffer[0];
    crcval = crc16(data, 256, 0x1021, 0x0000, 0x0000, true, true);
    CRC_Temp = (crcval & 0b1111111100000000);
    CRC_RES = (CRC_Temp >> 8);
    buffer[257] = CRC_RES;
    CRC_RES = (crcval & 0b0000000011111111);
    buffer[256] = CRC_RES;
    return buffer;
}

unsigned short MetodoCRC::Calcula_CRC_Mecanicas(char buffer[])
{
    unsigned short crc = 0xffff, i;
    int len = 33;

    while (len--)
    {
        crc = (crc ^ *buffer++);
        for (i = 0; i < 8; i++)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ 0xA001;
            }
            else
            {
                crc = (crc >> 1);
            }
        }
    }
    
    return crc;
}