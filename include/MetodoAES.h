#include <Arduino.h>
#include "mbedtls/aes.h"
#include <iostream>
#include "Stream.h"

class MetodoAES
{
private:
    char *key = "Charles Dickens.";
    char bufferEncriptado[256];
    char bufferDesencriptado[256];

public:
    char* Desencripta_Mensaje_Servidor(String buffer);
    char* Encripta_Mensaje_Servidor(char buffer[]);
    void encrypt(char *plainText, char *key, unsigned char *outputBuffer);
    void decrypt(unsigned char *chipherText, char *key, unsigned char *outputBuffer);
    void Encripta_Bloque(int val);
    void Desencripta_Bloque(int val);
};