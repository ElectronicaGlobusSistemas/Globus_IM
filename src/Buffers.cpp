#include "Buffers.h"

using namespace std;

/**********************************************************************************/
/*                              BUFFERS DE ACK                                    */
/**********************************************************************************/

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
    memcpy(buffer_ACK_encriptado, Metodo_AES.Encripta_Mensaje_Servidor(buffer_ACK_inicio), 258);
    return true;
}

bool Buffers::Set_buffer_ACK_CRC(void)
{
    memcpy(buffer_ACK_final, Metodo_CRC.Calcula_CRC_Wifi(buffer_ACK_encriptado), 258);
    return true;
}

char *Buffers::Get_buffer_ACK(void)
{
    return buffer_ACK_final;
}

/**********************************************************************************/
/*                    BUFFER DE RECEPCION DE DATOS MAQUINA                        */
/**********************************************************************************/

bool Buffers::Verifica_buffer_Maq(char buffer[], int len)
{
    //    MetodoCRC CRC_Maq;
    if (Metodo_CRC.Verifica_CRC_Maq(buffer, len))
    {
        return true;
    }
    return false;
}

/**********************************************************************************/
/*                  BUFFER DE RECEPCION DE DATOS SERVIDOR                         */
/**********************************************************************************/

bool Buffers::Set_buffer_recepcion(String buffer)
{
    if (Metodo_CRC.Verifica_CRC_Wifi(buffer))
    {
        if (Set_buffer_recepcion_desencriptado(buffer))
        {
            return true;
        }
        return false;
    }
    std::copy(std::begin(buffer), std::end(buffer), std::begin(buffer_recepcion));
    return false;
}

bool Buffers::Set_buffer_recepcion_desencriptado(String buffer)
{
    memcpy(buffer_recepcion, Metodo_AES.Desencripta_Mensaje_Servidor(buffer), 258);
    return true;
}

char *Buffers::Get_buffer_recepcion(void)
{
    return buffer_recepcion;
}

/**********************************************************************************/
/*                           BUFFER DE CONTADORES                                 */
/**********************************************************************************/

bool Buffers::Set_buffer_contadores_ACC(int Com, Contadores_SAS contadores)
{
    int pos;
    char req[258] = {};
    char res[10] = {};
    bzero(req, 258);
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

    //--------------------------------------------------------------------------------------------------
    // TOTAL CANCEL CREDIT
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Total_Cancel_Credit), sizeof(res) / sizeof(res[0]));
    pos = 4;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 12

    //--------------------------------------------------------------------------------------------------
    // COIN IN
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Coin_In), sizeof(res) / sizeof(res[0]));
    pos = 13;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 21

    //--------------------------------------------------------------------------------------------------
    // COIN OUT
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Coin_Out), sizeof(res) / sizeof(res[0]));
    pos = 22;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 30

    //--------------------------------------------------------------------------------------------------
    // TOTAL DROP
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Total_Drop), sizeof(res) / sizeof(res[0]));
    pos = 31;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 39

    //--------------------------------------------------------------------------------------------------
    // JACKPOT
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Jackpot), sizeof(res) / sizeof(res[0]));
    pos = 40;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 48

    //--------------------------------------------------------------------------------------------------
    // PHYSICAL COIN IN
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Physical_Coin_In), sizeof(res) / sizeof(res[0]));
    pos = 49;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 57

    //--------------------------------------------------------------------------------------------------
    // PHYSICAL COIN OUT
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Physical_Coin_Out), sizeof(res) / sizeof(res[0]));
    pos = 58;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 66

    //--------------------------------------------------------------------------------------------------
    // TOTAL COIN DROP
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Total_Coin_Drop), sizeof(res) / sizeof(res[0]));
    pos = 67;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 75

    //--------------------------------------------------------------------------------------------------
    // MACHINE PAID PROGRESIVE
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Machine_Paid_Progresive_Payout), sizeof(res) / sizeof(res[0]));
    pos = 76;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 84

    //--------------------------------------------------------------------------------------------------
    // MACHINE PAID EXTERNAL BONUS
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Machine_Paid_External_Bonus_Payout), sizeof(res) / sizeof(res[0]));
    pos = 85;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 93

    //--------------------------------------------------------------------------------------------------
    // ATTENDANT PAID PROGRESIVE
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Attendant_Paid_Progresive_Payout), sizeof(res) / sizeof(res[0]));
    pos = 94;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 102

    //--------------------------------------------------------------------------------------------------
    // ATTENDANT PAID EXTERNAL BONUS
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Attendant_Paid_External_Bonus_Payout), sizeof(res) / sizeof(res[0]));
    pos = 103;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 111

    //--------------------------------------------------------------------------------------------------
    // TICKET IN
    //--------------------------------------------------------------------------------------------------
    bzero(res, 10);
    memcpy(res, contadores.Get_Contadores_Char(Ticket_In), sizeof(res) / sizeof(res[0]));
    pos = 112;
    for (int i = 0; i < 10; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 122

    //--------------------------------------------------------------------------------------------------
    // TICKET OUT
    //--------------------------------------------------------------------------------------------------
    bzero(res, 10);
    memcpy(res, contadores.Get_Contadores_Char(Ticket_Out), sizeof(res) / sizeof(res[0]));
    pos = 123;
    for (int i = 0; i < 10; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 133

    //--------------------------------------------------------------------------------------------------
    // CANCEL CREDIT HAND PAY
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Cancel_Credit_Hand_Pay), sizeof(res) / sizeof(res[0]));
    pos = 134;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 142

    //--------------------------------------------------------------------------------------------------
    // BILL AMOUNT
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Bill_Amount), sizeof(res) / sizeof(res[0]));
    pos = 143;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 151

    //--------------------------------------------------------------------------------------------------
    // GAMES SINCE LAST POWER UP
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Games_Since_Last_Power_Up), sizeof(res) / sizeof(res[0]));
    Serial.println(res);
    pos = 152;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 160

    //--------------------------------------------------------------------------------------------------
    // GAMES PLAYED
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Games_Played), sizeof(res) / sizeof(res[0]));
    pos = 161;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 169

    //--------------------------------------------------------------------------------------------------
    // DOOR OPEN
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Door_Open), sizeof(res) / sizeof(res[0]));
    pos = 170;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 178

    //--------------------------------------------------------------------------------------------------
    // CURRENT CREDITS
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Current_Credits), sizeof(res) / sizeof(res[0]));
    pos = 179;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 187

    // IP Tarjeta
    req[188] = '0';
    req[189] = '1';
    req[190] = '5';
    req[191] = '5';
    req[192] = '|'; // 192

    // Bytes libres
    req[193] = '0';
    req[194] = '0';
    req[195] = '0';
    req[196] = '0';
    req[197] = '0';
    req[198] = '0';
    req[199] = '0';
    req[200] = '0';
    req[201] = '0';
    req[202] = '0';
    req[203] = '0';
    req[204] = '0';
    req[205] = '0';
    req[206] = '0';
    req[207] = '|'; // 207

    // Serie Trama
    req[208] = '0';
    req[209] = '0';
    req[210] = '0';
    req[211] = '0';
    req[212] = '0';
    req[213] = '0';
    req[214] = '0';
    req[215] = '1';
    req[216] = '|'; // 216

    // ID Jugador
    req[217] = '0';
    req[218] = '0';
    req[219] = '0';
    req[220] = '0';
    req[221] = '0';
    req[222] = '0';
    req[223] = '0';
    req[224] = '0';
    req[225] = '|'; // 225

    // Hora y fecha
    req[226] = '1';
    req[227] = '1';
    req[228] = '|'; // 228
    req[229] = '0';
    req[230] = '1';
    req[231] = '|'; // 231
    req[232] = '0';
    req[233] = '1';
    req[234] = '|'; // 234
    req[235] = '2';
    req[236] = '7';
    req[237] = '|'; // 237
    req[238] = '0';
    req[239] = '5';
    req[240] = '|'; // 240
    req[241] = '2';
    req[242] = '2';
    req[243] = '|'; // 243
    req[244] = '0';
    req[245] = '|'; // 245
    req[246] = '0';
    req[247] = '|'; // 247

    // ID Operador
    req[248] = '0';
    req[249] = '0';
    req[250] = '0';
    req[251] = '0';
    req[252] = '0';
    req[253] = '0';
    req[254] = '0';
    req[255] = '0'; // 255
    req[256] = '9'; // 256
    req[257] = '9'; // 257

    memcpy(buffer_contadores_ACC, req, 258);

    // char res[9] = {};
    // bzero(res, 9);
    // memcpy(res, contadores.Get_Contadores_Char(Total_Drop), sizeof(res) / sizeof(res[0]));
    for (int indice3 = 0; indice3 < 256; indice3++)
    {
        Serial.print(buffer_contadores_ACC[indice3]);
    }
    Serial.println();

    if (Set_buffer_contadores_ACC_encriptado())
    {
        if (Set_buffer_contadores_ACC_CRC())
        {
            return true;
        }
        return false;
    }
    return false;
}

bool Buffers::Set_buffer_contadores_ACC_encriptado(void)
{
    memcpy(buffer_contadores_ACC_encriptado, Metodo_AES.Encripta_Mensaje_Servidor(buffer_contadores_ACC), 258);
    return true;
}

bool Buffers::Set_buffer_contadores_ACC_CRC(void)
{
    memcpy(buffer_contadores_ACC_final, Metodo_CRC.Calcula_CRC_Wifi(buffer_contadores_ACC_encriptado), 258);
    return true;
}

char *Buffers::Get_buffer_contadores_ACC(void)
{
    return buffer_contadores_ACC_final;
}