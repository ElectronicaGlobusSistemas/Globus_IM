#include "Buffers.h"
#include <WiFi.h>
#include "Preferences.h"
#include "Configuracion.h"
#include "Buffer_Cashless.h"
#include "Json_Datos.h"

extern Configuracion_ESP32 Configuracion;
extern Preferences NVS;
using namespace std;
extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales
extern Buffer_RX_AFT Buffer_Cashless;
char convert(uint8_t dato);
int Firts_cancel_credit=0;

extern unsigned char Tabla_Eventos_[ 999 ][ 8 ]; /* Cola de Eventos*/
extern unsigned short Num_Eventos;
extern unsigned short Ptr_Eventos_Marca_Temp, Ptr_Eventos_Marca;
void Almacena_Evento(unsigned char Evento,ESP32Time RTC);
char Hex_Ascci_H(char Hex_Val);
char Hex_Ascci_L(char Hex_Val);
void  Escribe_Tiempo_Eventos(unsigned short N_Evento, ESP32Time RTC);

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


bool Buffers::Verifica_buffer_Mecanicas(char buffer[], int len)
{
    //    MetodoCRC CRC_Mecanicas;
    if (Metodo_CRC.Verifica_CRC_Mecanicas(buffer, len))
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


bool Calcula_Cancel_Credit_first(bool Calcula_Contador)
{
    extern Contadores_SAS contadores;
    int Cancel_Credit_Poker, Coin_In_Poker, Coin_Out_Poker, Drop_Poker, Creditos_Poker, Residuo;
    int uni, dec, cen, unimil, decmil, centmil, unimill, decmill;
    char Contador_Cancel_Credit_Poker[9];
    bzero(Contador_Cancel_Credit_Poker, 9);

    if (Calcula_Contador)
    {
        Coin_In_Poker = contadores.Get_Contadores_Int(Coin_In);
        Coin_Out_Poker = contadores.Get_Contadores_Int(Coin_Out);
        Drop_Poker = contadores.Get_Contadores_Int(Total_Drop);
        Creditos_Poker = contadores.Get_Contadores_Int(Current_Credits);

        Cancel_Credit_Poker = ((Drop_Poker - Coin_In_Poker) + Coin_Out_Poker) - Creditos_Poker;
        if (Cancel_Credit_Poker <= 0)
            return false;
    }
    Serial.print("contador cancel credit int poker es: ");
    Serial.println(Cancel_Credit_Poker);

    decmill = Cancel_Credit_Poker / 10000000;
    Contador_Cancel_Credit_Poker[0] = decmill + 48;
    Residuo = Cancel_Credit_Poker % 10000000;

    unimill = Residuo / 1000000;
    Contador_Cancel_Credit_Poker[1] = unimill + 48;
    Residuo = Cancel_Credit_Poker % 1000000;

    centmil = Residuo / 100000;
    Contador_Cancel_Credit_Poker[2] = centmil + 48;
    Residuo = Cancel_Credit_Poker % 100000;

    decmil = Residuo / 10000;
    Contador_Cancel_Credit_Poker[3] = decmil + 48;
    Residuo = Cancel_Credit_Poker % 10000;

    unimil = Residuo / 1000;
    Contador_Cancel_Credit_Poker[4] = unimil + 48;
    Residuo = Cancel_Credit_Poker % 1000;

    cen = Residuo / 100;
    Contador_Cancel_Credit_Poker[5] = cen + 48;
    Residuo = Cancel_Credit_Poker % 100;

    dec = Residuo / 10;
    Contador_Cancel_Credit_Poker[6] = dec + 48;
    Residuo = Cancel_Credit_Poker % 10;

    uni = Residuo;
    Contador_Cancel_Credit_Poker[7] = uni + 48;

    Serial.print("contador cancel credit char poker es: ");
    Serial.println(Contador_Cancel_Credit_Poker);
    if (contadores.Set_Contadores(Total_Cancel_Credit, Contador_Cancel_Credit_Poker) && contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_Cancel_Credit_Poker))
        return true;
    else
        return false;
}

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
    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)==6){
        if(Firts_cancel_credit==0)
        {
            Calcula_Cancel_Credit_first(true);
            Serial.println("Primer");
            Firts_cancel_credit=1;
            
            bzero(res, 8);
            memcpy(res, contadores.Get_Contadores_Char(Total_Cancel_Credit), sizeof(res) / sizeof(res[0]));
            pos = 4;
            for (int i = 0; i < 8; i++) // Total Cancel Credit
            {
                req[pos] = res[i];
                pos++;
            }
            req[pos] = '|'; // 12
        }else{
            bzero(res, 8);
            memcpy(res, contadores.Get_Contadores_Char(Total_Cancel_Credit), sizeof(res) / sizeof(res[0]));
            pos = 4;
            for (int i = 0; i < 8; i++) // Total Cancel Credit
            {
                req[pos] = res[i];
                pos++;
            }
            req[pos] = '|'; // 12
        }
       
    }else{

    bzero(res, 8);
    memcpy(res, contadores.Get_Contadores_Char(Total_Cancel_Credit), sizeof(res) / sizeof(res[0]));
    pos = 4;
    for (int i = 0; i < 8; i++) // Total Cancel Credit
    {
        req[pos] = res[i];
        pos++;
    }
    req[pos] = '|'; // 12

    }
    

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
    req[189] = '0';
    req[190] = '0';
    req[191] = '0';
    req[192] = '0'; // 192

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
const unsigned char Tabla_Ascii[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
unsigned char Convierte_Hex_Ascii_Centenas(unsigned char Hex_Val) {
    unsigned char Centenas;

    Centenas = Hex_Val / 100;
    Centenas = Tabla_Ascii[ Centenas ];
    return ( Centenas);
}

/******************************************************************************/

unsigned char Convierte_Hex_Ascii_Decenas(unsigned char Hex_Val) {
    unsigned char Decenas, Temp;

    Temp = Hex_Val % 100;
    Decenas = Temp / 10;
    Decenas = Tabla_Ascii[ Decenas ];
    return ( Decenas);
}

/******************************************************************************/

unsigned char Convierte_Hex_Ascii_Unidades(unsigned char Hex_Val) {
    unsigned char Unidades;

    Unidades = Hex_Val % 10;
    Unidades = Tabla_Ascii[ Unidades ];
    return ( Unidades);
}


/**********************************************************************************/
/*                           BUFFER DE EVENTOS SAS                                */
/**********************************************************************************/

bool Buffers::Set_buffer_eventos(int Com, Eventos_SAS eventos, ESP32Time RTC)
{

    /*Cola-Variables*/

    unsigned char i, j, Dato, Index;

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

    Ptr_Eventos_Marca_Temp = Ptr_Eventos_Marca;
    Index = 7;

    for (i = 0; i < 10; i++)
    {
        if (Tabla_Eventos_[Ptr_Eventos_Marca_Temp][7] == 0x00)
        {
            for (j = 0; j < 7; j++)
            {

                if(j==0)
                {
                    Dato = Tabla_Eventos_[Ptr_Eventos_Marca_Temp][j];
                    req[Index] = Hex_Ascci_H(Dato);
                    Index++;
                    req[Index] = Hex_Ascci_L(Dato);
                    Index++;
                    req[Index] = '|';
                    Index++;
                }
                else
                {
                    Dato = Tabla_Eventos_[Ptr_Eventos_Marca_Temp][j];
                    String Dato2=String(Dato,DEC);

                    if(Dato2[1]==NULL)
                    {
                        req[Index] = '0';
                        Index++;
                        req[Index] = Dato2[0];
                        Index++;
                        req[Index] = '|';
                        Index++;
                        
                    }else{
                        req[Index] = Dato2[0];
                        Index++;
                        req[Index] = Dato2[1];
                        Index++;
                        req[Index] = '|';
                        Index++;
                    }
                }
            }
        }
        else if (Tabla_Eventos_[Ptr_Eventos_Marca_Temp][7] == 0xFF)
        {
            break;
        }
        Ptr_Eventos_Marca_Temp++;
    }

    if(i!=0)
    {
        req[4]=Convierte_Hex_Ascii_Decenas(i);
        req[5]=Convierte_Hex_Ascii_Unidades(i);
        req[6]='|';
    }

    if(i==0)
    {
        return false;
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
/*                           BUFFER DE INFO TARJETA                               */
/**********************************************************************************/

bool Buffers::Set_buffer_info_tarjeta(int Com)

{
    /* ID Comando */
    char req[258] = {};
    bzero(req, 258);
    char res[2] = {};
    bzero(res, 2);
    int32_t Aux1;
    String Ver;
    String mac;
    String Fecha_Bootlader;
    String Espacio_Memoria_Libre = Variables_globales.Get_Variables_Global_String(Espacio_Libre_SD);
    String Espacio_Memoria_Usado=Variables_globales.Get_Variables_Global_String(Espacio_Usado_SD);
    String Max_Storage_SD=Variables_globales.Get_Variables_Global_String(Size_SD);
    String GPU_Temperature=Variables_globales.Get_Variables_Global_String(Temperatura_procesador);
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
    
    
    req[4]='|';

    /* MAC */
     mac = WiFi.macAddress();
    /**/
    req[5] = mac[0];
    req[6] = mac[1];
    req[7] = mac[2];
    req[8] = mac[3];
    req[9] = mac[4];
    req[10] = mac[5];
    req[11] = mac[6];
    req[12] = mac[7];
    req[13] = mac[8];
    req[14] = mac[9];
    req[15] = mac[10];
    req[16] = mac[11];
    req[17] = mac[12];
    req[18] = mac[13];
    req[19] = mac[14];
    req[20] = mac[15];
    req[21] = mac[16];
    req[22] = '|';

    /*Tipo maquina*/
    char tipoMq;
    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 1:
        tipoMq='1';
        break;
    case 2:
        tipoMq='2';
        break;
    case 3:
        tipoMq='3';
        break;
    case 4:
        tipoMq='4';
        break;
    case 5:
        tipoMq='5';
        break;
    case 6:
        tipoMq='6';
        break;
    case 7:
        tipoMq='7';
        break;
    
    case 8:
        tipoMq='8';
        break;
    case 9:
        tipoMq='9';
        break;
    }
    req[23]= tipoMq;
    req[24]='|';

    /*Version de firmware*/
    NVS.begin("Config_ESP32", false);
    size_t Version_length = NVS.getBytesLength("Ver_Fir");
    uint8_t Version_FIRMWARE[Version_length];
    NVS.getBytes("Ver_Fir", Version_FIRMWARE, sizeof(Version_FIRMWARE));
    req[25]=convert(int(Version_FIRMWARE[0]));
    req[26]=convert(Version_FIRMWARE[1]);
    req[27]=convert(int(Version_FIRMWARE[2]));
    req[28]=convert(int(Version_FIRMWARE[3]));
    
    Ver= String(int(Version_FIRMWARE[0]))+String(int(Version_FIRMWARE[1]))+String(int(Version_FIRMWARE[2]))+String(int(Version_FIRMWARE[3]));
    
    req[29]='|';
    NVS.end();
    /*Fecha ultimo bootloader*/
    NVS.begin("Config_ESP32", false);
    size_t Fecha_len = NVS.getBytesLength("Fecha_Boot");
    uint8_t Datos_Fecha_B[Fecha_len];
    NVS.getBytes("Fecha_Boot", Datos_Fecha_B, sizeof(Datos_Fecha_B));
    NVS.end();

    req[30]=char(Datos_Fecha_B[6]);
    req[31]=char(Datos_Fecha_B[7]);
    req[32]='|';
    req[33]=char(Datos_Fecha_B[8]);
    req[34]=char(Datos_Fecha_B[9]);
    req[35]='|';
    req[36]=char(Datos_Fecha_B[10]);
    req[37]=char(Datos_Fecha_B[11]);
    req[38]='|';
    Fecha_Bootlader=String(char(Datos_Fecha_B[6]))+String(char(Datos_Fecha_B[7]))+String(char(Datos_Fecha_B[8]))+String(char(Datos_Fecha_B[9]))+String(char(Datos_Fecha_B[10]))+String(char(Datos_Fecha_B[11]));
   // Serial.println(Fecha_Bootlader);
    /*Status SD*/

    req[39]=convert(Variables_globales.Get_Variable_Global(Estado_Escritura));
    req[40]='|';
    /*Información de memoria*/
    
    if(Variables_globales.Get_Variable_Global(Enable_SD))
    {
        /*Espacio libre SD */
       
        req[41] = Espacio_Memoria_Libre[0];
        req[42] = Espacio_Memoria_Libre[1];
        req[43] = Espacio_Memoria_Libre[2];
        req[44] = Espacio_Memoria_Libre[3];
        /* Espacio Usado SD */
        
        req[45]=Espacio_Memoria_Usado[0];
        req[46]=Espacio_Memoria_Usado[1];
        req[47]=Espacio_Memoria_Usado[2];
        req[48]=Espacio_Memoria_Usado[3];
        /* Tamaño de memoria */
        
        req[49]=Max_Storage_SD[0];
        req[50]=Max_Storage_SD[1];
        req[51]=Max_Storage_SD[2];
        req[52]=Max_Storage_SD[3];

    }else
    {
        req[41] = '0';
        req[42] = '0';
        req[43] = '0';
        req[44] = '0';
        req[45] = '0';
        req[46] = '0';
        req[47] = '0';
        req[48] = '0';
        req[49] = '0';
        req[50] = '0';
        req[51] = '0';
        req[52] = '0';
    }
    req[53]='|';
    /*MODO FTP */
    req[54]=convert(Variables_globales.Get_Variable_Global(Ftp_Mode));
    req[55]='|';

    /*Temperatura procesador*/
    
    req[56]=GPU_Temperature[0];
    req[57]=GPU_Temperature[1];
    req[58]=GPU_Temperature[2];
    req[59]='|';
    /*Datos Restantes en 0 */
    /*
   for (int i = 60; i < 258; i++)
    {
        req[i] = '0';
    }
   */
  String json;
    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)==6)
    {
        json=Buffer_Json_info_IM(mac,Configuracion.Get_Configuracion(Tipo_Maquina, 0),Ver,Fecha_Bootlader,Variables_globales.Get_Variable_Global(Estado_Escritura),Espacio_Memoria_Libre,Espacio_Memoria_Usado,Max_Storage_SD,Variables_globales.Get_Variable_Global(Ftp_Mode),GPU_Temperature,String(digitalRead(14)));
    }else{
        json=Buffer_Json_info_IM(mac,Configuracion.Get_Configuracion(Tipo_Maquina, 0),Ver,Fecha_Bootlader,Variables_globales.Get_Variable_Global(Estado_Escritura),Espacio_Memoria_Libre,Espacio_Memoria_Usado,Max_Storage_SD,Variables_globales.Get_Variable_Global(Ftp_Mode),GPU_Temperature,"NO");
    }
   
    
    int pos=5;
    for(int i=0; i<json.length();i++)
    {
        req[pos]=json[i];
        pos++;
    }
    req[pos]='|';    
    for (int i = pos+2; i < 258; i++)
    {
        req[i] = '0';
    }

   // Serial.println(Espacio_Memoria_Libre);
   // Serial.println(Espacio_Memoria_Usado);
   // Serial.println(Max_Storage_SD);

    memcpy(buffer_info_tarjeta, req, 258);

    for (int indice = 0; indice < 256; indice++)
    {
        Serial.print(buffer_info_tarjeta[indice]);
    }
    Serial.println();
    //Reset Variables 
    if (Set_buffer_info_tarjeta_encriptado())
    {
        if (Set_buffer_info_tarjeta_CRC())
        {
            return true;
        }
        return false;
    }
    return false;
}

bool Buffers::Set_buffer_info_tarjeta_encriptado(void)
{
    memcpy(buffer_info_tarjeta_encriptado, Metodo_AES.Encripta_Mensaje_Servidor(buffer_info_tarjeta), 258);
    return true;
}

bool Buffers::Set_buffer_info_tarjeta_CRC(void)
{
    memcpy(buffer_info_tarjeta_final, Metodo_CRC.Calcula_CRC_Wifi(buffer_info_tarjeta_encriptado), 258);
    return true;
}

char *Buffers::Get_buffer_info_tarjeta(void)
{
    return buffer_info_tarjeta_final;
}

/**********************************************************************************/
/*                           BUFFER INFO CASHLESS                                 */
/**********************************************************************************/
const unsigned char Tabla_Ascii_1[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
char Hex_Ascci_H(char Hex_Val) {
    char Val1, Val2;

    Val1 = Hex_Val; //Paso el valor del registro
    Val2 = (Val1 & 0b11110000); //Enmascaro  el nibble superior
    Val2 = Val2 >> 4;
    Val1 = Tabla_Ascii_1[ Val2 ];
    return ( Val1);
}

char Hex_Ascci_L(char Hex_Val) {
    char Val1, Val2;

    Val1 = Hex_Val; //Paso el valor del registro
    Val2 = (Val1 & 0b00001111); //Enmascaro  el nibble superior
    Val1 = Tabla_Ascii_1[ Val2 ];
    return ( Val1);
}

bool Buffers::Set_buffer_info_Cashless(int Com)

{
    /* ID Comando */
    char req[258] = {};
    bzero(req, 258);
    char res[2] = {};
    bzero(res, 2);
    int32_t Aux1;
    /* Reset Variables*/
    String AFT_Game_Lock="00";
    String Asset_Number="00000000";
    String Available_transfer="00";
    String Host_Cashout_Status="00";
    String AFT_Status="00";
    String Max_History_index="00";
    String Current_Casheable="0000000000";
    String Current_Restricted="0000000000";
    String Current_Nonrestricted="0000000000";
    String Transfer_Limit="0000000000";
    String Restricted_Expiration="00000000";
    String Restricted_Pool_ID = "0000";

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
    
    int m=4;
    String CASHLESS;
    for(int i=0; i<38;i++)
    {
       //req[m]=Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[i]);
        //m++;
        //req[m]=Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[i]);
        //m++;
        //Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[i]);
        //Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[i]);
        CASHLESS =CASHLESS+String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[i]))+String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[i]));
    }
    // GAME LOCK
    AFT_Game_Lock=
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[7]))+String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[7]));
    //ASET NUMBER
    Asset_Number=
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[3]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[3]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[4]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[4]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[5]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[5]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[6]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[6]));

    //Available Transfers
    Available_transfer=
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[8]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[8]));
    //Host Cashout Status
    Host_Cashout_Status=
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[9]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[9]));
    //AFT Status
    AFT_Status=
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[10]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[10]));
    // Max History Index
    Max_History_index=
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[11]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[11]));
    // Current Cashable
    Current_Casheable=
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[12]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[12]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[13]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[13]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[14]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[14]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[15]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[15]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[16]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[16]));
    //Current Restricted
    Current_Restricted=
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[17]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[17]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[18]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[18]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[19]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[19]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[20]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[20]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[21]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[21]));
   
    //Current Nonrestricted
    Current_Nonrestricted=
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[22]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[22]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[23]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[23]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[24]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[24]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[25]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[25]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[26]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[26]));
    //Transfer limit
    Transfer_Limit=
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[27]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[27]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[28]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[28]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[29]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[29]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[30]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[30]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[31]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[31]));
    //Restricted Expiration
    Restricted_Expiration=
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[32]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[32]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[33]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[33]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[34]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[34]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[35]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[35]));
    //Restricted Pool_ID
    Restricted_Pool_ID=
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[36]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[36]))+
    String(Hex_Ascci_H(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[37]))+
    String(Hex_Ascci_L(Buffer_Cashless.Get_RX_AFT(Info_MQ_AFT)[37]));
    //Convierte Datos en Entidad para transmitir
    String json= Buffer_Json_info_Cashless(AFT_Game_Lock,
    Asset_Number,
    Available_transfer,
    Host_Cashout_Status,
    AFT_Status,
    Max_History_index,
    Current_Casheable,
    Current_Restricted,
    Current_Nonrestricted,
    Transfer_Limit,
    Restricted_Expiration,
    Restricted_Pool_ID);

    req[4]='|';
    int pos2=5;
    for(int i=0; i<json.length();i++)
    {
        req[pos2]=json[i];
        pos2++;
    }
    req[pos2]='|';

   for (int i = pos2+2; i < 258; i++)
    {
        req[i] = '0';
    }
    memcpy(buffer_info_Cashless, req, 258);

    for (int indice = 0; indice < 256; indice++)
    {
        Serial.print(buffer_info_Cashless[indice]);
    }
    Serial.println();

    if (Set_buffer_info_Cashless_encriptado())
    {
        if (Set_buffer_info_Cashless_CRC())
        {
            return true;
        }
        return false;
    }
    return false;
}

bool Buffers::Set_buffer_info_Cashless_encriptado(void)
{
    memcpy(buffer_info_Cashless_encriptado, Metodo_AES.Encripta_Mensaje_Servidor(buffer_info_Cashless), 258);
    return true;
}

bool Buffers::Set_buffer_info_Cashless_CRC(void)
{
    memcpy(buffer_info_Cashless_final, Metodo_CRC.Calcula_CRC_Wifi(buffer_info_Cashless_encriptado), 258);
    return true;
}

char *Buffers::Get_buffer_info_Cashless(void)
{
    return buffer_info_Cashless_final;
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

/**********************************************************************************/
/*                        BUFFER DE TARJETA MICROSD                          */
/**********************************************************************************/

bool Buffers::Set_buffer_info_MicroSD(char *buffer)
{
    memcpy(buffer_info_MicroSD, buffer, 258);
    return true;
}

bool Buffers::Set_buffer_info_MCU(char *buffer)
{
    memcpy(buffer_info_MCU, buffer, 258);
    return true;
}

char *Buffers::Get_buffer_info_MicroSD(void)
{
    return buffer_info_MicroSD;
}

char *Buffers::Get_buffer_info_MCU(void)
{
    return buffer_info_MicroSD;
}

/**********************************************************************************/
/*                            BUFFER TARJETA MECANICA                             */
/**********************************************************************************/

bool Buffers::Set_buffer_tarjeta_mecanica(char buffer[])
{
    int pos;
    char req[35] = {};
    bzero(req, 35);

    req[0] = 0X02; // 1

    pos = 1;
    for (int i = 4; i < 12; i++) // Total Cancel Credit
    {
        req[pos] = buffer[i] - 0x30;
        pos++;
    }

    pos = 9;
    for (int i = 12; i < 20; i++) // Coin In
    {
        req[pos] = buffer[i] - 0x30;
        pos++;
    }

    pos = 17;
    for (int i = 20; i < 28; i++) // Coin In
    {
        req[pos] = buffer[i] - 0x30;
        pos++;
    }

    pos = 25;
    for (int i = 28; i < 36; i++) // Total Drop
    {
        req[pos] = buffer[i] - 0x30;
        pos++;
    }

    req[33] = 0x00;
    req[34] = 0x00;

    memcpy(buffer_tarjeta_mecanica, req, 35);

    if (Set_buffer_tarjeta_CRC())
    {
        return true;
    }
    return false;
}

bool Buffers::Set_buffer_tarjeta_CRC(void)
{
    unsigned short crc = Metodo_CRC.Calcula_CRC_Mecanicas(buffer_tarjeta_mecanica);

    memcpy(buffer_tarjeta_mecanica_final, buffer_tarjeta_mecanica, 35);

    unsigned char x;
    x = ((crc & 0b1111111100000000) >> 8);
    buffer_tarjeta_mecanica_final[33] = x;
    x = crc & 0b0000000011111111;
    buffer_tarjeta_mecanica_final[34] = x;

    return true;
}

char *Buffers::Get_buffer_tarjeta_mecanica(void)
{
    return buffer_tarjeta_mecanica_final;
}

char convert(uint8_t dato)
{
    char dato_out;
    if(dato==0)
    {
        dato_out='0';
        return dato_out;
    }
    else
    {
        switch (dato)
        {
        case 1:
            dato_out = '1';
            break;

        case 2:
            dato_out = '2';
            break;
        case 3:
            dato_out = '3';
            break;
        case 4:
            dato_out = '4';
            break;
        case 5:
            dato_out = '5';
            break;
        case 6:
            dato_out = '6';
            break;
        case 7:
            dato_out = '7';
            break;
        case 8:
            dato_out = '8';
            break;
        case 9:
            dato_out = '9';
            break;
        }

        return dato_out;
    }

    
}



void Almacena_Evento_En_RAM(unsigned short N_Evento,unsigned char Val_A_Guardar)
{
    Tabla_Eventos_[N_Evento][0]=Val_A_Guardar; /* Guarda Evento en Cola*/
    //Serial.println("Evento almacenado en RAM");
}

void Almacena_Evento(unsigned char Evento,ESP32Time RTC)
{
    Almacena_Evento_En_RAM(Num_Eventos, Evento);
    Escribe_Tiempo_Eventos(Num_Eventos,RTC);
    Num_Eventos++;
}

void  Escribe_Tiempo_Eventos(unsigned short N_Evento, ESP32Time RTC)
{
    /*Captura time*/

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


   int  hour = ((Hora[0] - 48) * 10) + (Hora[1] - 48);
   int  minutes = ((Hora[3] - 48) * 10) + (Hora[4] - 48);
   int  seconds = ((Hora[6] - 48) * 10) + (Hora[7] - 48);
   int  day = ((Fecha[9] - 48) * 10) + (Fecha[10] - 48);
   int  month2 = ((Mes[0] - 48) * 10) + (Mes[1] - 48);
   int  year = ((Fecha[14] - 48) * 10) + (Fecha[15] - 48);
 
   Tabla_Eventos_[N_Evento][1]=hour;
   Tabla_Eventos_[N_Evento][2]=minutes;
   Tabla_Eventos_[N_Evento][3]=seconds;
   Tabla_Eventos_[N_Evento][4]=day;
   Tabla_Eventos_[N_Evento][5]=month2;
   Tabla_Eventos_[N_Evento][6]=year;
   Tabla_Eventos_[N_Evento][7]=0x00;
}