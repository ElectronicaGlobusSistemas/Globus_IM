#include <Arduino.h>
#include "mbedtls/aes.h"
#include <iostream>
#include "Stream.h"

char *key = "Charles Dickens.";
unsigned char bufferEncriptado[256];
unsigned char bufferDesencriptado[256];
extern char buffer_envio[258];

void Encripta_Bloque(int val);
void Desencripta_Bloque(int val);
void Encripta_Mensaje_Servidor();

using namespace std;

/*****************************************************************************************************************************/
/*****************************************************************************************************************************/

void Desencripta_Mensaje_Servidor(String buffer)
{

    int len = buffer.length();

    std::copy(std::begin(buffer), std::end(buffer), std::begin(bufferEncriptado));

    //    decrypt(bufferEncriptado, key, bufferDesencriptado);
    Desencripta_Bloque(8);

    Serial.println("\nDeciphered text:");
    for (int i = 0; i < len; i++)
    {
        char str[3];
        sprintf(str, "%02x", (int)bufferDesencriptado[i]);
        Serial.print(str);
    }
    Serial.println(" ");
}

/*****************************************************************************************************************************/
/*****************************************************************************************************************************/

void Encripta_Mensaje_Servidor()
{

    //    int len = buffer.length();

    //    std::copy(std::begin(buffer), std::end(buffer), std::begin(bufferEncriptado));
    memcpy(bufferDesencriptado, buffer_envio, 256);
    Serial.println("\n\nDeciphered text:");
    // for (int i = 0; i < 256; i++)
    // {
    //     Serial.print((char)bufferDesencriptado[i]);
    // }

    //    decrypt(bufferEncriptado, key, bufferDesencriptado);
    Encripta_Bloque(8);

    Serial.println("\nCiphered text:");
    memcpy(buffer_envio, bufferEncriptado, 256);
    Serial.println(" ");

    Desencripta_Bloque(8);
    Serial.println("\nDeciphered text:");
    for (int i = 0; i < 256; i++)
    {
        Serial.print(bufferDesencriptado[i], HEX);
    }
    Serial.println(" ");
}

/*****************************************************************************************************************************/
/*****************************************************************************************************************************/

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

/*****************************************************************************************************************************/
/*****************************************************************************************************************************/

void Encripta_Bloque(int val)
{
    char buffer_encripcion[16];
    unsigned char buffer_encriptado[16];
    int j = 0;

    for (int k = 0; k < val; k++)
    {
        for (int i = 0; i < 16; i++)
        {
            buffer_encripcion[i] = bufferDesencriptado[j];
            j++;
        }

        encrypt(buffer_encripcion, key, buffer_encriptado);

        j = j - 16;
        for (int i = 0; i < 16; i++)
        {
            bufferEncriptado[j] = buffer_encriptado[i];
            j++;
        }
    }
}

/*****************************************************************************************************************************/
/*****************************************************************************************************************************/

void Desencripta_Bloque(int val)
{
    unsigned char buffer_desencripcion[16];
    unsigned char buffer_desencriptado[16];
    int j = 0;

    for (int k = 0; k < val; k++)
    {
        for (int i = 0; i < 16; i++)
        {
            buffer_desencripcion[i] = bufferEncriptado[j];
            j++;
        }

        decrypt(buffer_desencripcion, key, buffer_desencriptado);

        j = j - 16;
        for (int i = 0; i < 16; i++)
        {
            bufferDesencriptado[j] = buffer_desencriptado[i];
            j++;
        }
    }
}