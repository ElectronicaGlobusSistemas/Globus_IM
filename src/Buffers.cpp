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
/*                BUFFER DE RECEPCION DE DATOS SERVIDOR TCP                       */
/**********************************************************************************/

bool Buffers::Set_buffer_recepcion_TCP(String buffer)
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
/*                BUFFER DE RECEPCION DE DATOS SERVIDOR UDP                       */
/**********************************************************************************/

bool Buffers::Set_buffer_recepcion_UDP(char buffer[])
{
    if (Metodo_CRC.Verifica_CRC_Wifi(buffer))
    {
        if (Set_buffer_recepcion_desencriptado(buffer))
        {
            return true;
        }
        return false;
    }
    memcpy(buffer_recepcion, buffer, 258);
    return false;
}

/**********************************************************************************/
/*                       BUFFER DE CONTADORES ACCOUTING                           */
/**********************************************************************************/

bool Buffers::Set_buffer_contadores_ACC(int Com, Contadores_SAS contadores, ESP32Time RTC, Variables_Globales Variables_globales)
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

    //--------------------------------------------------------------------------------------------------
    // SERIE TRAMA
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Serie_Trama), sizeof(res) / sizeof(res[0]));
    pos = 208;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 216

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

    String Hora = RTC.getTime();
    String Fecha = RTC.getDate();
    String Mes;
    int month = RTC.getMonth();
    switch (month)
    {
    case 0:
        Mes = "01";
        break;
    case 1:
        Mes = "02";
        break;
    case 2:
        Mes = "03";
        break;
    case 3:
        Mes = "04";
        break;
    case 4:
        Mes = "05";
        break;
    case 5:
        Mes = "06";
        break;
    case 6:
        Mes = "07";
        break;
    case 7:
        Mes = "08";
        break;
    case 8:
        Mes = "09";
        break;
    case 9:
        Mes = "10";
        break;
    case 10:
        Mes = "11";
        break;
    case 11:
        Mes = "12";
        break;
    default:
        break;
    }

    // Hora y fecha
    req[226] = Hora[0];
    req[227] = Hora[1];
    req[228] = '|'; // 228
    req[229] = Hora[3];
    req[230] = Hora[4];
    req[231] = '|'; // 231
    req[232] = Hora[6];
    req[233] = Hora[7];
    req[234] = '|'; // 234
    req[235] = Fecha[9];
    req[236] = Fecha[10];
    req[237] = '|'; // 237
    req[238] = Mes[0];
    req[239] = Mes[1];
    req[240] = '|'; // 240
    req[241] = Fecha[14];
    req[242] = Fecha[15];
    req[243] = '|'; // 243

    if (Variables_globales.Get_Variable_Global(Flag_Maquina_En_Juego))
        req[244] = '1';
    else
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

    // CRC
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

/**********************************************************************************/
/*                           BUFFER DE ID MAQUINA                                 */
/**********************************************************************************/

bool Buffers::Set_buffer_id_maq(int Com, Contadores_SAS contadores)
{
    char req[258] = {};
    bzero(req, 258);
    char res[20];
    bzero(res, 20);
    char char_dato[5];
    bzero(char_dato, 5);
    int32_t Aux1;
    int dato;

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

    memcpy(res, contadores.Get_Contadores_Char(Informacion_Maquina), sizeof(res) / sizeof(res[0]));

    for (int i = 0; i < 20; i++)
    {
        Serial.print(res[i]);
    }
    Serial.println();

    req[4] = res[0];
    req[5] = res[1];
    req[6] = '|';

    req[7] = res[2];
    req[8] = res[3];
    req[9] = res[4];
    req[10] = '|';

    int j = 0;

    for (int i = 5; i < 10; i++)
    {
        String string_dato = String(res[i], HEX);
        char_dato[j] = string_dato.toInt();
        j++;
    }

    dato = (char_dato[0] - (char_dato[0] % 10)) / 10;
    req[11] = dato + '0';
    dato = char_dato[0] % 10;
    req[12] = dato + '0';
    req[13] = '|';

    dato = (char_dato[1] - (char_dato[1] % 10)) / 10;
    req[14] = dato + '0';
    dato = char_dato[1] % 10;
    req[15] = dato + '0';
    req[16] = '|';

    dato = (char_dato[2] - (char_dato[2] % 10)) / 10;
    req[17] = dato + '0';
    dato = char_dato[2] % 10;
    req[18] = dato + '0';
    req[19] = '|';

    dato = (char_dato[3] - (char_dato[3] % 10)) / 10;
    req[20] = dato + '0';
    dato = char_dato[3] % 10;
    req[21] = dato + '0';
    dato = (char_dato[4] - (char_dato[4] % 10)) / 10;
    req[22] = dato + '0';
    dato = char_dato[4] % 10;
    req[23] = dato + '0';
    req[24] = '|';

    req[25] = res[10];
    req[26] = res[11];
    req[27] = res[12];
    req[28] = res[13];
    req[29] = res[14];
    req[30] = res[15];
    req[31] = '|';

    req[32] = res[16];
    req[33] = res[17];
    req[34] = res[18];
    req[35] = res[19];
    req[36] = '|';

    req[37] = '0';
    req[38] = '0';
    req[39] = '|';

    for (int i = 40; i < 258; i++)
    {
        req[i] = '0';
    }

    memcpy(buffer_id_maq, req, 258);

    for (int indice = 0; indice < 256; indice++)
    {
        Serial.print(buffer_id_maq[indice]);
    }
    Serial.println();

    if (Set_buffer_id_maq_encriptado())
    {
        if (Set_buffer_id_maq_CRC())
        {
            return true;
        }
        return false;
    }
    return false;
}

bool Buffers::Set_buffer_id_maq_encriptado(void)
{
    memcpy(buffer_id_maq_encriptado, Metodo_AES.Encripta_Mensaje_Servidor(buffer_id_maq), 258);
    return true;
}

bool Buffers::Set_buffer_id_maq_CRC(void)
{
    memcpy(buffer_id_maq_final, Metodo_CRC.Calcula_CRC_Wifi(buffer_id_maq_encriptado), 258);
    return true;
}

char *Buffers::Get_buffer_id_maq(void)
{
    return buffer_id_maq_final;
}

/**********************************************************************************/
/*                           BUFFER DE EVENTOS SAS                                */
/**********************************************************************************/

bool Buffers::Set_buffer_eventos(int Com, Eventos_SAS eventos, ESP32Time RTC)
{
    char req[258] = {};
    bzero(req, 258);
    char res = {'0'};
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

    req[4] = '0';
    req[5] = '1';
    req[6] = '|';

    String string_dato = String(eventos.Get_evento(), HEX);

    // char char_dato = string_dato.toInt();

    // int dato = (char_dato - (char_dato % 10)) / 10;
    // req[7] = dato + '0';
    // dato = char_dato % 10;
    // req[8] = dato + '0';

    (string_dato[0] > 96) ? req[7] = string_dato[0] - 32 : req[7] = string_dato[0];
    (string_dato[1] > 96) ? req[8] = string_dato[1] - 32 : req[8] = string_dato[1];

    req[9] = '|';

    String Hora = RTC.getTime();
    String Fecha = RTC.getDate();
    String Mes;
    int month = RTC.getMonth();
    switch (month)
    {
    case 0:
        Mes = "01";
        break;
    case 1:
        Mes = "02";
        break;
    case 2:
        Mes = "03";
        break;
    case 3:
        Mes = "04";
        break;
    case 4:
        Mes = "05";
        break;
    case 5:
        Mes = "06";
        break;
    case 6:
        Mes = "07";
        break;
    case 7:
        Mes = "08";
        break;
    case 8:
        Mes = "09";
        break;
    case 9:
        Mes = "10";
        break;
    case 10:
        Mes = "11";
        break;
    case 11:
        Mes = "12";
        break;
    default:
        break;
    }

    // Hora y fecha
    req[10] = Hora[0];
    req[11] = Hora[1];
    req[12] = '|';
    req[13] = Hora[3];
    req[14] = Hora[4];
    req[15] = '|';
    req[16] = Hora[6];
    req[17] = Hora[7];
    req[18] = '|';
    req[19] = Fecha[9];
    req[20] = Fecha[10];
    req[21] = '|';
    req[22] = Mes[0];
    req[23] = Mes[1];
    req[24] = '|';
    req[25] = Fecha[14];
    req[26] = Fecha[15];
    req[27] = '|';

    for (int i = 28; i < 258; i++)
    {
        req[i] = '0';
    }

    memcpy(buffer_eventos, req, 258);

    for (int indice = 0; indice < 256; indice++)
    {
        Serial.print(buffer_eventos[indice]);
    }
    Serial.println();

    if (Set_buffer_eventos_encriptado())
    {
        if (Set_buffer_eventos_CRC())
        {
            return true;
        }
        return false;
    }
    return false;
}

bool Buffers::Set_buffer_eventos_encriptado(void)
{
    memcpy(buffer_eventos_encriptado, Metodo_AES.Encripta_Mensaje_Servidor(buffer_eventos), 258);
    return true;
}

bool Buffers::Set_buffer_eventos_CRC(void)
{
    memcpy(buffer_eventos_final, Metodo_CRC.Calcula_CRC_Wifi(buffer_eventos_encriptado), 258);
    return true;
}

char *Buffers::Get_buffer_eventos(void)
{
    return buffer_eventos_final;
}

/**********************************************************************************/
/*                           BUFFER DE ROM SIGNATURE                              */
/**********************************************************************************/

bool Buffers::Set_buffer_ROM_Singnature(int Com, Contadores_SAS contadores)
{
    char req[258] = {};
    bzero(req, 258);
    char res[2] = {};
    bzero(res, 2);
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

    memcpy(res, contadores.Get_Contadores_Char(ROM_Signature), 2);

    //    Serial.println(res[0], HEX);

    String string_dato = String(res[0], HEX);

    (string_dato[0] > 96) ? req[4] = string_dato[0] - 32 : req[4] = string_dato[0];
    (string_dato[1] > 96) ? req[5] = string_dato[1] - 32 : req[5] = string_dato[1];

    //    Serial.println(res[1], HEX);

    String string_dato1 = String(res[1], HEX);

    (string_dato1[0] > 96) ? req[6] = string_dato1[0] - 32 : req[6] = string_dato1[0];
    (string_dato1[1] > 96) ? req[7] = string_dato1[1] - 32 : req[7] = string_dato1[1];

    req[8] = '|';

    for (int i = 9; i < 258; i++)
    {
        req[i] = '0';
    }

    memcpy(buffer_ROM_Singnature, req, 258);

    for (int indice = 0; indice < 256; indice++)
    {
        Serial.print(buffer_ROM_Singnature[indice]);
    }
    Serial.println();

    if (Set_buffer_ROM_Singnature_encriptado())
    {
        if (Set_buffer_ROM_Singnature_CRC())
        {
            return true;
        }
        return false;
    }
    return false;
}

bool Buffers::Set_buffer_ROM_Singnature_encriptado(void)
{
    memcpy(buffer_ROM_Singnature_encriptado, Metodo_AES.Encripta_Mensaje_Servidor(buffer_ROM_Singnature), 258);
    return true;
}

bool Buffers::Set_buffer_ROM_Singnature_CRC(void)
{
    memcpy(buffer_ROM_Singnature_final, Metodo_CRC.Calcula_CRC_Wifi(buffer_ROM_Singnature_encriptado), 258);
    return true;
}

char *Buffers::Get_buffer_ROM_Singnature(void)
{
    return buffer_ROM_Singnature_final;
}

/**********************************************************************************/
/*                        BUFFER DE CONTADORES CASHLESS                           */
/**********************************************************************************/

bool Buffers::Set_buffer_contadores_CASH(int Com, Contadores_SAS contadores)
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
    // CASHEABLE IN
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Casheable_In), sizeof(res) / sizeof(res[0]));
    pos = 4;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 12

    //--------------------------------------------------------------------------------------------------
    // CASHEABLE RESTRICTED IN
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Casheable_Restricted_In), sizeof(res) / sizeof(res[0]));
    Serial.println(res);
    pos = 13;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 21

    //--------------------------------------------------------------------------------------------------
    // CASHEABLE NONRESTRICTED IN
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Casheable_NONrestricted_In), sizeof(res) / sizeof(res[0]));
    pos = 22;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 30

    //--------------------------------------------------------------------------------------------------
    // CASHEABLE OUT
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Casheable_Out), sizeof(res) / sizeof(res[0]));
    pos = 31;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 39

    //--------------------------------------------------------------------------------------------------
    // CASHEABLE RESTRICTED OUT
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Casheable_Restricted_Out), sizeof(res) / sizeof(res[0]));
    pos = 40;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 48

    //--------------------------------------------------------------------------------------------------
    // CASHEABLE NONRESTRICTED OUT
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Casheable_NONrestricted_Out), sizeof(res) / sizeof(res[0]));
    pos = 49;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 57

    //--------------------------------------------------------------------------------------------------
    // CANCEL CREDIT HAND PAY
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Cancel_Credit_Hand_Pay), sizeof(res) / sizeof(res[0]));
    pos = 58;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 66

    for (int i = 67; i < 258; i++)
    {
        req[i] = '0';
    }

    memcpy(buffer_contadores_CASH, req, 258);

    for (int indice3 = 0; indice3 < 256; indice3++)
    {
        Serial.print(buffer_contadores_CASH[indice3]);
    }
    Serial.println();

    if (Set_buffer_contadores_CASH_encriptado())
    {
        if (Set_buffer_contadores_CASH_CRC())
        {
            return true;
        }
        return false;
    }
    return false;
}

bool Buffers::Set_buffer_contadores_CASH_encriptado(void)
{
    memcpy(buffer_contadores_CASH_encriptado, Metodo_AES.Encripta_Mensaje_Servidor(buffer_contadores_CASH), 258);
    return true;
}

bool Buffers::Set_buffer_contadores_CASH_CRC(void)
{
    memcpy(buffer_contadores_CASH_final, Metodo_CRC.Calcula_CRC_Wifi(buffer_contadores_CASH_encriptado), 258);
    return true;
}

char *Buffers::Get_buffer_contadores_CASH(void)
{
    return buffer_contadores_CASH_final;
}

/**********************************************************************************/
/*                        BUFFER DE BILLETESS                           */
/**********************************************************************************/

bool Buffers::Set_buffer_billetes(int Com, Contadores_SAS contadores)
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
    // BILLETES 2K
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Billetes_2k), sizeof(res) / sizeof(res[0]));
    pos = 4;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }

    req[pos] = '|'; // 12

    //--------------------------------------------------------------------------------------------------
    // BILLETES 5K
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Billetes_5k), sizeof(res) / sizeof(res[0]));
    Serial.println(res);
    pos = 13;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }

    req[pos] = '|'; // 21

    //--------------------------------------------------------------------------------------------------
    // BILLETES 10K
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Billetes_10k), sizeof(res) / sizeof(res[0]));
    pos = 22;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }

    req[pos] = '|'; // 30

    //--------------------------------------------------------------------------------------------------
    // BILLETES 20K
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Billetes_20k), sizeof(res) / sizeof(res[0]));
    pos = 31;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }

    req[pos] = '|'; // 39

    //--------------------------------------------------------------------------------------------------
    // BILLETES 50K
    //--------------------------------------------------------------------------------------------------
    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Billetes_50k), sizeof(res) / sizeof(res[0]));
    pos = 40;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }

    req[pos] = '|'; // 48

    pos++;

    for (int j = 0; j < 7; j++)
    {
        for (int i = 0; i < 8; i++)
        {
            req[pos] = '0';
            pos++;
        }
        req[pos] = '|';
        pos++;
    }

    for (int i = 112; i < 258; i++)
    {
        req[i] = '0';
    }

    memcpy(buffer_billetes, req, 258);

    for (int indice3 = 0; indice3 < 256; indice3++)
    {
        Serial.print(buffer_billetes[indice3]);
    }
    Serial.println();

    if (Set_buffer_billetes_encriptado())
    {
        if (Set_buffer_billetes_CRC())
        {
            return true;
        }
        return false;
    }
    return false;
}

bool Buffers::Set_buffer_billetes_encriptado(void)
{
    memcpy(buffer_billetes_encriptado, Metodo_AES.Encripta_Mensaje_Servidor(buffer_billetes), 258);
    return true;
}

bool Buffers::Set_buffer_billetes_CRC(void)
{
    memcpy(buffer_billetes_final, Metodo_CRC.Calcula_CRC_Wifi(buffer_billetes_encriptado), 258);
    return true;
}

char *Buffers::Get_buffer_billetes(void)
{
    return buffer_billetes_final;
}


bool Buffers ::Set_buffer_info_MicroSD(char* buffer)
{
    memcpy(buffer_info_MicroSD, buffer, 258);
    return true;
}
bool Buffers ::Set_buffer_info_MCU(char* buffer)
{
    memcpy(buffer_info_MCU, buffer, 258);
    return true;
}

char*Buffers ::Get_buffer_info_MicroSD(void)
{
    return buffer_info_MicroSD;
}
char*Buffers ::Get_buffer_info_MCU(void)
{
    return buffer_info_MicroSD;
}