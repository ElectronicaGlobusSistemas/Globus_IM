#include <Arduino.h>
#include "mbedtls/aes.h"
#include <iostream>

#include "Stream.h"

char *key = "Charles Dickens.";

using namespace std;

void encrypt(char *plainText, char *key, unsigned char *outputBuffer)
{
    mbedtls_aes_context aes;

    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, (const unsigned char *)key, strlen(key) * 8);
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, (const unsigned char *)plainText, outputBuffer);
    mbedtls_aes_free(&aes);
}

void decrypt(unsigned char *chipherText, char *key, unsigned char *outputBuffer)
{
    mbedtls_aes_context aes;

    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, (const unsigned char *)key, strlen(key) * 8);
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, (const unsigned char *)chipherText, outputBuffer);
    mbedtls_aes_free(&aes);
}

void Desencripta_Mensaje_Servidor(String buffer)
{
    unsigned char bufferEncriptado[256];
    unsigned char bufferDesencriptado[256];

    int len = buffer.length();
    //    int len = sizeof(buffer);

    std::copy(std::begin(buffer), std::end(buffer), std::begin(bufferEncriptado));

    decrypt(bufferEncriptado, key, bufferDesencriptado);

    Serial.println("\nDeciphered text:");
    for (int i = 0; i < len; i++)
    {
        char str[3];
        sprintf(str, "%02x", (int)bufferDesencriptado[i]);
        Serial.print(str);
    }
    Serial.println(" ");
}