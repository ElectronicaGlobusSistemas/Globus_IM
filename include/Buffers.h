#include "Stream.h"
#include "MetodoAES.h"
#include "MetodoCRC.h"
#include "ESP32Time.h"
#include "Contadores.h"

class Buffers
{
private:
    /* Buffer de ACK */
    char buffer_ACK_inicio[258];
    char buffer_ACK_encriptado[258];
    char buffer_ACK_final[258];

    /* Buffer de Recepcion Servidor */
    char buffer_recepcion[258];

    /* Buffer de contadores */
    char buffer_contadores_ACC[258];
    char buffer_contadores_ACC_encriptado[258];
    char buffer_contadores_ACC_final[258];

    /* Buffer de contadores */
    char buffer_id_maq[258];
    char buffer_id_maq_encriptado[258];
    char buffer_id_maq_final[258];

    MetodoAES Metodo_AES;
    MetodoCRC Metodo_CRC; 
//    Contadores_SAS Metodo_Contadores;

public:
    /* Constructor inicializa las variables en cero */
    Buffers()
    {
        bzero(buffer_ACK_inicio, 258);
        bzero(buffer_ACK_encriptado, 258);
        bzero(buffer_ACK_final, 258);
    }

    /* Buffer de ACK */
    bool Set_buffer_ACK(int, char, char);
    bool Set_buffer_ACK_encriptado(void);
    bool Set_buffer_ACK_CRC(void);
    char *Get_buffer_ACK(void);

    /* Buffer de Maquina */
    bool Verifica_buffer_Maq(char[], int);

    /* Buffer de Recepcion Servidor */
    bool Set_buffer_recepcion(String);
    bool Set_buffer_recepcion_desencriptado(String);
    char *Get_buffer_recepcion(void);

    /* Buffer de contadores */
    bool Set_buffer_contadores_ACC(int, Contadores_SAS, ESP32Time);
    bool Set_buffer_contadores_ACC_encriptado(void);
    bool Set_buffer_contadores_ACC_CRC(void);
    char *Get_buffer_contadores_ACC(void);

    /* Buffer id maq */
    bool Set_buffer_id_maq(int, Contadores_SAS);
    bool Set_buffer_id_maq_encriptado(void);
    bool Set_buffer_id_maq_CRC(void);
    char *Get_buffer_id_maq(void);
};