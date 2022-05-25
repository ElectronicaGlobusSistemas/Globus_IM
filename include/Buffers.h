#include "Stream.h"
#include "MetodoAES.h"
#include "MetodoCRC.h"

class Buffers
{
    private:
    char buffer_ACK_inicio[258];
    char buffer_ACK_encriptado[258];
    char buffer_ACK_final[258];
    MetodoAES Metodo_AES;
    MetodoCRC Metodo_CRC;

    public:
    /* Buffer de ACK */
    bool Set_buffer_ACK(int, char, char);
    bool Set_buffer_ACK_encriptado(void);
    bool Set_buffer_ACK_CRC(void);
    char* Get_buffer_ACK(void);

    /* Buffer de Maquina */
    bool Verifica_buffer_Maq(char [], int);
};