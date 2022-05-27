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
            }
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
        continue;
    }
}