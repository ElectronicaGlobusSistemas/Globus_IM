#include "Internet.h"

#define     ID_Eventos              5

Buffers transimite_ACK;

void Transmite_Confirmacion(char High, char Low)
{
    if (transimite_ACK.Set_buffer_ACK(ID_Eventos, High, Low))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, transimite_ACK.Get_buffer_ACK(), 258);
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