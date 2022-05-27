#include "MetodoAES.h"

/*****************************************************************************************************************************/
/*****************************************************************************************************************************/

void MetodoAES::encrypt(char *plainText, char *key, unsigned char *outputBuffer)
{
    mbedtls_aes_context aes;

    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, (const unsigned char *)key, strlen(key) * 8);
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, (const unsigned char *)plainText, outputBuffer);
    mbedtls_aes_free(&aes);
}

/*****************************************************************************************************************************/

void MetodoAES::decrypt(unsigned char *chipherText, char *key, unsigned char *outputBuffer)
{
    mbedtls_aes_context aes;

    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, (const unsigned char *)key, strlen(key) * 8);
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, (const unsigned char *)chipherText, outputBuffer);
    mbedtls_aes_free(&aes);
}

/*****************************************************************************************************************************/
/*****************************************************************************************************************************/

void MetodoAES::Encripta_Bloque(int val)
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

void MetodoAES::Desencripta_Bloque(int val)
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

/*****************************************************************************************************************************/
/*****************************************************************************************************************************/

char *MetodoAES::Encripta_Mensaje_Servidor(char buffer[])
{
    memcpy(bufferDesencriptado, buffer, 256);
    Encripta_Bloque(8);
    return bufferEncriptado;
}

/*****************************************************************************************************************************/

char *MetodoAES::Desencripta_Mensaje_Servidor(String buffer)
{
    int len = buffer.length();
    std::copy(std::begin(buffer), std::end(buffer), std::begin(bufferEncriptado));
    Desencripta_Bloque(8);
    return bufferDesencriptado;
}
