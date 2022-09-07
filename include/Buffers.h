#include "Stream.h"
#include "MetodoAES.h"
#include "MetodoCRC.h"
#include "ESP32Time.h"
#include "Contadores.h"
#include "Eventos.h"
#include "Clase_Variables_Globales.h"

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

    /* Buffer id maquina */
    char buffer_id_maq[258];
    char buffer_id_maq_encriptado[258];
    char buffer_id_maq_final[258];

    /* Buffer de eventos */
    char buffer_eventos[258];
    char buffer_eventos_encriptado[258];
    char buffer_eventos_final[258];

    /* Buffer ROM Signature */
    char buffer_ROM_Singnature[258];
    char buffer_ROM_Singnature_encriptado[258];
    char buffer_ROM_Singnature_final[258];

    /* Buffer de contadores cashless */
    char buffer_contadores_CASH[258];
    char buffer_contadores_CASH_encriptado[258];
    char buffer_contadores_CASH_final[258];

    /* Buffer de billetes */
    char buffer_billetes[258];
    char buffer_billetes_encriptado[258];
    char buffer_billetes_final[258];

    /* Buffer de Información */
    char buffer_info_MicroSD[258];
    char buffer_info_MCU[258];

    /* Buffer tarjeta mecanica */
    char buffer_tarjeta_mecanica[64];
    char buffer_tarjeta_mecanica_final[64];

    MetodoAES Metodo_AES;
    MetodoCRC Metodo_CRC;

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
    bool Verifica_buffer_Mecanicas(char[], int);

    /* Buffer de Recepcion Servidor TCP */
    bool Set_buffer_recepcion_TCP(String);
    bool Set_buffer_recepcion_desencriptado(String);
    char *Get_buffer_recepcion(void);

    /* Buffer de Recepcion Servidor UDP */
    bool Set_buffer_recepcion_UDP(char[]);

    /* Buffer de contadores */
    bool Set_buffer_contadores_ACC(int, Contadores_SAS, ESP32Time, Variables_Globales);
    bool Set_buffer_contadores_ACC_encriptado(void);
    bool Set_buffer_contadores_ACC_CRC(void);
    char *Get_buffer_contadores_ACC(void);

    /* Buffer id maq */
    bool Set_buffer_id_maq(int, Contadores_SAS);
    bool Set_buffer_id_maq_encriptado(void);
    bool Set_buffer_id_maq_CRC(void);
    char *Get_buffer_id_maq(void);

    /* Buffer eventos */
    bool Set_buffer_eventos(int, Eventos_SAS, ESP32Time);
    bool Set_buffer_eventos_encriptado(void);
    bool Set_buffer_eventos_CRC(void);
    char *Get_buffer_eventos(void);

    /* Buffer ROM Singnature */
    bool Set_buffer_ROM_Singnature(int, Contadores_SAS);
    bool Set_buffer_ROM_Singnature_encriptado(void);
    bool Set_buffer_ROM_Singnature_CRC(void);
    char *Get_buffer_ROM_Singnature(void);

    /* Buffer de contadores */
    bool Set_buffer_contadores_CASH(int, Contadores_SAS);
    bool Set_buffer_contadores_CASH_encriptado(void);
    bool Set_buffer_contadores_CASH_CRC(void);
    char *Get_buffer_contadores_CASH(void);

    /* Buffer de contadores */
    bool Set_buffer_billetes(int, Contadores_SAS);
    bool Set_buffer_billetes_encriptado(void);
    bool Set_buffer_billetes_CRC(void);
    char *Get_buffer_billetes(void);

    bool Set_buffer_info_MicroSD(char *buffer);
    bool Set_buffer_info_MCU(char *buffer);
    char *Get_buffer_info_MCU(void);
    char *Get_buffer_info_MicroSD(void);

    bool Set_buffer_tarjeta_mecanica(char buffer[]);
    bool Set_buffer_tarjeta_CRC(void);
    char *Get_buffer_tarjeta_mecanica(void);
};