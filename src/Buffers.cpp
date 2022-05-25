#include "Buffers.h"

using namespace std;

bool Buffers::Set_buffer_ACK(int Com, char High, char Low)
{
    char req[258] = {};
    int32_t Aux1;

    Aux1 = Com;
    Aux1 = (Aux1 & 0x000000FF);
    req[0] = Aux1;
    //------------------------------------------------------------------------------
    // Guarda el segundo byte
    Aux1 = Com;
    Aux1 = ((Aux1 & 0x0000FF00) >> 8);
    req[1] = Aux1;
    //------------------------------------------------------------------------------
    // Guarda el tercer byte
    Aux1 = Com;
    Aux1 = ((Aux1 & 0x00FF0000) >> 16);
    req[2] = Aux1;
    //------------------------------------------------------------------------------
    // Guarda el cuarto byte
    Aux1 = Com;
    Aux1 = ((Aux1 & 0xFF000000) >> 24);
    req[3] = Aux1;

    req[4] = {High};
    req[5] = {Low};
    req[6] = {'|'};

    memcpy(buffer_ACK_inicio, req, 258);
    
    if (Set_buffer_ACK_encriptado())
    {
        if (Set_buffer_ACK_CRC())
        {
            return true;
        }
        return false;
    }
    return false;
}

bool Buffers::Set_buffer_ACK_encriptado(void)
{
    MetodoAES AES_general;
    memcpy(buffer_ACK_encriptado, AES_general.Encripta_Mensaje_Servidor(buffer_ACK_inicio), 258);
    return true;
}

bool Buffers::Set_buffer_ACK_CRC(void)
{
    MetodoCRC CRC_general;
    memcpy(buffer_ACK_final, CRC_general.Calcula_CRC_Wifi(buffer_ACK_encriptado), 258);
    return true;
}

char *Buffers::Get_buffer_ACK(void)
{
    return buffer_ACK_final;
}

bool Verifica_buffer_Maq(char buffer[], int len)
{
    MetodoCRC CRC_Maq;
    if (CRC_Maq.Verifica_CRC_Maq(buffer, len));
    {
        return true;
    }
    return false;
}