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
    char req[258] = {};
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

    char res[8] = {};
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Total_Cancel_Credit), sizeof(res) / sizeof(res[0]));


    //    Contadores_SAS contadores;
    //    Serial.println(contadores.Get_Contadores_Int(Total_Cancel_Credit));

    //    Serial.print("Buffer copiado: ");
    //    Serial.println(j);
    // Total Cancel Credit
    req[0] = 3;
    req[1] = 0;
    req[2] = 0;
    req[3] = 0;

    req[4] = '9';
    req[5] = '0';
    req[6] = '0';
    req[7] = '4';
    req[8] = '1';
    req[9] = '4';
    req[10] = '7';
    req[11] = '2';
    req[12] = '|'; // 12
    //    Serial.println(j);
    // Coin In
    req[13] = '0';
    req[14] = '2';
    req[15] = '0';
    req[16] = '8';
    req[17] = '7';
    req[18] = '4';
    req[19] = '3';
    req[20] = '2';
    req[21] = '|'; // 21
                   //    Serial.println(j);
    // Coin Out
    req[22] = '0';
    req[23] = '1';
    req[24] = '5';
    req[25] = '2';
    req[26] = '0';
    req[27] = '0';
    req[28] = '5';
    req[29] = '1';
    req[30] = '|'; // 30
                   //    Serial.println(j);
    // Total Drop
    req[31] = '9';
    req[32] = '0';
    req[33] = '6';
    req[34] = '0';
    req[35] = '9';
    req[36] = '8';
    req[37] = '3';
    req[38] = '3';
    req[39] = '|'; // 39
                   //    Serial.println(j);
    // Jackpot
    req[40] = '0';
    req[41] = '0';
    req[42] = '0';
    req[43] = '0';
    req[44] = '0';
    req[45] = '0';
    req[46] = '0';
    req[47] = '0';
    req[48] = '|'; // 48
                   //    Serial.println(j);
    // Physical Coin In
    req[49] = '0';
    req[50] = '0';
    req[51] = '0';
    req[52] = '0';
    req[53] = '0';
    req[54] = '0';
    req[55] = '0';
    req[56] = '0';
    req[57] = '|'; // 57
                   //    Serial.println(j);
    // Physical Coin Out
    req[58] = '0';
    req[59] = '0';
    req[60] = '0';
    req[61] = '0';
    req[62] = '0';
    req[63] = '0';
    req[64] = '0';
    req[65] = '0';
    req[66] = '|'; // 66
                   //    Serial.println(j);
    // Total Coin Drop
    req[67] = '0';
    req[68] = '0';
    req[69] = '0';
    req[70] = '0';
    req[71] = '0';
    req[72] = '0';
    req[73] = '0';
    req[74] = '0';
    req[75] = '|'; // 75
                   //    Serial.println(j);
    // Machine Paid Progresive
    req[76] = '0';
    req[77] = '0';
    req[78] = '0';
    req[79] = '0';
    req[80] = '0';
    req[81] = '0';
    req[82] = '0';
    req[83] = '0';
    req[84] = '|'; // 84
                   //    Serial.println(j);
    // Machine Paid External Bonus
    req[85] = '0';
    req[86] = '0';
    req[87] = '0';
    req[88] = '0';
    req[89] = '0';
    req[90] = '0';
    req[91] = '0';
    req[92] = '0';
    req[93] = '|'; // 93
                   //    Serial.println(j);
    // Attendfant Paid  Progresive
    req[94] = '0';
    req[95] = '0';
    req[96] = '0';
    req[97] = '0';
    req[98] = '0';
    req[99] = '0';
    req[100] = '0';
    req[101] = '0';
    req[102] = '|'; // 102
                    //    Serial.println(j);
    // Attendfant Paid External Bonus
    req[103] = '0';
    req[104] = '0';
    req[105] = '0';
    req[106] = '0';
    req[107] = '0';
    req[108] = '0';
    req[109] = '0';
    req[110] = '0';
    req[111] = '|'; // 111
                    //    Serial.println(j);
    // Ticket Voucher In
    req[112] = '0';
    req[113] = '0';
    req[114] = '0';
    req[115] = '0';
    req[116] = '0';
    req[117] = '0';
    req[118] = '0';
    req[119] = '0';
    req[120] = '0';
    req[121] = '0';
    req[122] = '|'; // 122
                    //    Serial.println(j);
    // Ticket Voucher Out
    req[123] = '0';
    req[124] = '0';
    req[125] = '0';
    req[126] = '0';
    req[127] = '0';
    req[128] = '0';
    req[129] = '0';
    req[130] = '0';
    req[131] = '0';
    req[132] = '0';
    req[133] = '|'; // 133
                    //    Serial.println(j);
    // Cancel Credit Hand Paid
    req[134] = '0';
    req[135] = '1';
    req[136] = '6';
    req[137] = '3';
    req[138] = '1';
    req[139] = '5';
    req[140] = '9';
    req[141] = '0';
    req[142] = '|'; // 142
                    //    Serial.println(j);
    // Bill Amount
    req[143] = '0';
    req[144] = '0';
    req[145] = '4';
    req[146] = '7';
    req[147] = '8';
    req[148] = '2';
    req[149] = '0';
    req[150] = '0';
    req[151] = '|'; // 151
                    //    Serial.println(j);
    // Games Since Power up
    req[152] = '0';
    req[153] = '0';
    req[154] = '0';
    req[155] = '0';
    req[156] = '0';
    req[157] = '0';
    req[158] = '0';
    req[159] = '0';
    req[160] = '|'; // 160
                    //    Serial.println(j);
    // Total Games Played
    req[161] = '0';
    req[162] = '0';
    req[163] = '0';
    req[164] = '0';
    req[165] = '8';
    req[166] = '6';
    req[167] = '2';
    req[168] = '0';
    req[169] = '|'; // 169
                    //    Serial.println(j);
    // Total Door Open
    req[170] = '0';
    req[171] = '0';
    req[172] = '0';
    req[173] = '0';
    req[174] = '1';
    req[175] = '2';
    req[176] = '6';
    req[177] = '8';
    req[178] = '|'; // 178
                    //    Serial.println(j);
    // Current Credits
    req[179] = '0';
    req[180] = '0';
    req[181] = '0';
    req[182] = '0';
    req[183] = '0';
    req[184] = '9';
    req[185] = '8';
    req[186] = '0';
    req[187] = '|'; // 187
                    //    Serial.println(j);
    // IP Tarjeta
    req[188] = '0';
    req[189] = '1';
    req[190] = '5';
    req[191] = '5';
    req[192] = '|'; // 192
                    //    Serial.println(j);
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
                    //    Serial.println(j);
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
                    //    Serial.println(j);
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
                    //    Serial.println(j);
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
                    //    Serial.println(j);
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
    //    Serial.println(j);

    //    Serial.print("Buffer copiado: ");
    //    Serial.println(j);

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