#include "Internet.h"

#define ID_Contadores 3
#define ID_Eventos 5

void Transmite_Confirmacion(char High, char Low)
{
    if (Buffer.Set_buffer_ACK(ID_Eventos, High, Low))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_ACK(), 258);
        Serial.println("Set buffer general OK");
        int len = sizeof(res);
        Serial.print("Tamaño a enviar: ");
        Serial.println(len);
        Serial.println("buffer enviado: ");
        Serial.println(res);
        int length_ = client.write(res, len);
        Serial.print("Bytes enviados: ");
        Serial.println(length_);
        Serial.println("--------------------------------------------------------------------");
    }
    else
    {
        Serial.println("Set buffer general ERROR");
    }
}

void Transmite_Contadores()
{
    if (Buffer.Set_buffer_contadores_ACC(ID_Contadores, contadores))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_contadores_ACC(), 258);
        Serial.println("Set buffer general OK");
        int len = sizeof(res);
        Serial.print("Tamaño a enviar: ");
        Serial.println(len);
        Serial.println("buffer enviado: ");
        Serial.println(res);
        int length_ = client.write(res, len);
        Serial.print("Bytes enviados: ");
        Serial.println(length_);
        Serial.println("--------------------------------------------------------------------");
    }
    else
    {
        Serial.println("Set buffer general ERROR");
    }
}

void Transmite_Eco_Broadcast()
{
    char res[258] = {};
    bzero(res, 258); // Pone el buffer en 0

    res[0] = 'L';
    res[1] = '|';
    res[2] = '0';
    res[3] = '1';
    res[4] = '|';
    // MAC
    res[5] = '2';
    res[6] = '1';
    res[7] = '6';
    res[8] = '.';
    res[9] = '1';
    res[10] = '2';
    res[11] = '8';
    res[12] = '.';
    res[13] = '0';
    res[14] = '5';
    res[15] = '7';
    res[16] = '.';
    res[17] = '0';
    res[18] = '9';
    res[19] = '7';
    res[20] = '.';
    res[21] = '0';
    res[22] = '4';
    res[23] = '5';
    res[24] = '.';
    res[25] = '1';
    res[26] = '7';
    res[27] = '5';
    res[28] = '|';
    // PORT
    res[29] = '0';
    res[30] = '1';
    res[31] = '0';
    res[32] = '0';
    res[33] = '1';
    res[34] = '|';
    // IP
    res[35] = '1';
    res[36] = '9';
    res[37] = '2';
    res[38] = '.';
    res[39] = '1';
    res[40] = '6';
    res[41] = '8';
    res[42] = '.';
    res[43] = '0';
    res[44] = '0';
    res[45] = '5';
    res[46] = '.';
    res[47] = '1';
    res[48] = '5';
    res[49] = '5';
    res[50] = '|';
    // MASCARA
    res[51] = '2';
    res[52] = '5';
    res[53] = '5';
    res[54] = '.';
    res[55] = '2';
    res[56] = '5';
    res[57] = '5';
    res[58] = '.';
    res[59] = '2';
    res[60] = '5';
    res[61] = '5';
    res[62] = '.';
    res[63] = '0';
    res[64] = '0';
    res[65] = '0';
    res[66] = '|';
    // IP Enlace
    res[67] = '1';
    res[68] = '9';
    res[69] = '2';
    res[70] = '.';
    res[71] = '1';
    res[72] = '6';
    res[73] = '8';
    res[74] = '.';
    res[75] = '0';
    res[76] = '0';
    res[77] = '5';
    res[78] = '.';
    res[79] = '0';
    res[80] = '0';
    res[81] = '1';
    res[82] = '|';
    // Nombre MAQ
    res[83] = 'M';
    res[84] = 'a';
    res[85] = 'q';
    res[86] = '_';
    res[87] = 'P';
    res[88] = 'r';
    res[89] = 'u';
    res[90] = 'e';
    res[91] = 'b';
    res[92] = 'a';
    res[93] = '_';
    res[94] = 'E';
    res[95] = 'S';
    res[96] = 'P';
    res[97] = '3';
    res[98] = '2';

    String mac = WiFi.macAddress();
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.macAddress());

    Serial.println("Set buffer general OK");
    int len = sizeof(res);
    Serial.print("Tamaño a enviar: ");
    Serial.println(len);
    Serial.println("buffer enviado: ");
    Serial.println(res);
    int length_ = client.write(res, len);
    Serial.print("Bytes enviados: ");
    Serial.println(length_);
    Serial.println("--------------------------------------------------------------------");
}

int Comando_Recibido(void)
{
    int32_t Comando, Aux1, Aux2, Aux3, Aux4;

    char res[258] = {};
    bzero(res, 258); // Pone el buffer en 0
    memcpy(res, Buffer.Get_buffer_recepcion(), 258);

    Aux1 = res[0];
    Aux2 = res[1];
    Aux2 = Aux2 << 8;
    Aux3 = res[2];
    Aux3 = Aux3 << 16;
    Aux4 = res[3];
    Aux4 = Aux4 << 24;
    Comando = Aux4 | Aux3 | Aux2 | Aux1;
    return (Comando);
}

void Task_Procesa_Comandos(void *parameter)
{
    for (;;)
    {
        if (flag_dato_valido_recibido)
        {
            flag_dato_valido_recibido = false;

            switch (Comando_Recibido())
            {
            case 3:
                Serial.println("Solicitud de contadores SAS");
                Transmite_Contadores();
                break;

            default:
                Serial.println(Comando_Recibido());
                break;
            }
            Serial.println("--------------------------------------------------------------------------");
        }
        else if (flag_dato_no_valido_recibido)
        {
            flag_dato_no_valido_recibido = false;

            char res[258] = {};
            bzero(res, 258); // Pone el buffer en 0
            memcpy(res, Buffer.Get_buffer_recepcion(), 258);

            if (res[0] == 'S' && res[1] == 'B')
            {
                Serial.println("Solicitud de modo Bootloader");
                        }
            else if (res[0] == 'E' && res[1] == 'B')
            {
                Serial.println("Eco Broadcast");
                Transmite_Eco_Broadcast();
            }
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
        continue;
    }
}