#include "Internet.h"

#define ID_Contadores_Accounting 3
#define ID_Billetes 4
#define ID_Eventos 5
#define ID_Maquina 11
#define ID_ROM_Signature 12
#define ID_Contadores_Cashless 308

int Contador_Transmision = 0;
int Contador_Transmision_Contadores = 0;

extern bool flag_sincronizacion_RTC;
extern bool flag_serie_trama_contadores;
extern bool flag_evento_valido_recibido;

void Task_Procesa_Comandos(void *parameter);
void Task_Maneja_Transmision(void *parameter);
void Transmite_Configuracion(void);
void Transmision_Controlada_Contadores(void);

void init_Comunicaciones()
{
    xTaskCreatePinnedToCore(
        Task_Procesa_Comandos,
        "Procesa comandos server",
        5000,
        NULL,
        configMAX_PRIORITIES - 5,
        NULL,
        0); // Core donde se ejecutara la tarea
    xTaskCreatePinnedToCore(
        Task_Maneja_Transmision,
        "Maneja la transmision con server",
        5000,
        NULL,
        configMAX_PRIORITIES - 15,
        NULL,
        0); // Core donde se ejecutara la tarea
}
/*****************************************************************************************/
/********************************** TRANSMITE ACK ****************************************/
/*****************************************************************************************/

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
        Serial.println("Set buffer general ERROR");
}

/*****************************************************************************************/
/************************** TRANSMITE CONTADORES ACCOUNTING ******************************/
/*****************************************************************************************/

void Transmite_Contadores_Accounting(void)
{
    if (Buffer.Set_buffer_contadores_ACC(ID_Contadores_Accounting, contadores, RTC))
    {
        if (flag_serie_trama_contadores)
        {
            contadores.Incrementa_Serie_Trama();
        }
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
        Serial.println("Set buffer general ERROR");
}

/*****************************************************************************************/
/************************** TRANSMITE CONTADORES CASHLESS ********************************/
/*****************************************************************************************/

void Transmite_Contadores_Cashless(void)
{
    if (Buffer.Set_buffer_contadores_CASH(ID_Contadores_Cashless, contadores))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_contadores_CASH(), 258);
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
        Serial.println("Set buffer general ERROR");
}

/*****************************************************************************************/
/************************** TRANSMITE CONTADORES BILLETES ********************************/
/*****************************************************************************************/

void Transmite_Billetes(void)
{
    if (Buffer.Set_buffer_billetes(ID_Billetes, contadores))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_billetes(), 258);
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
        Serial.println("Set buffer general ERROR");
}

/*****************************************************************************************/
/******************************* SINCRONIZA RELOJ RTC ************************************/
/*****************************************************************************************/

bool Sincroniza_Reloj_RTC(char res[])
{
    int hour, minutes, seconds, day, month, year;

    hour = ((res[4] - 48) * 10) + (res[5] - 48);
    minutes = ((res[6] - 48) * 10) + (res[7] - 48);
    seconds = ((res[8] - 48) * 10) + (res[9] - 48);
    day = ((res[10] - 48) * 10) + (res[11] - 48);
    month = ((res[12] - 48) * 10) + (res[13] - 48);
    year = ((res[14] - 48) * 10) + (res[15] - 48);
    year = year + 2000;

    RTC.setTime(seconds, minutes, hour, day, month, year);
    return true;
}

/*****************************************************************************************/
/************************** TRANSMITE INFORMACION MAQUINA ********************************/
/*****************************************************************************************/

void Transmite_ID_Maquina(void)
{
    if (Buffer.Set_buffer_id_maq(ID_Maquina, contadores))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_id_maq(), 258);
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
        Serial.println("Set buffer general ERROR");
}

/*****************************************************************************************/
/******************************* TRANSMITE EVENTOS SAS ***********************************/
/*****************************************************************************************/

void Transmite_Eventos(void)
{
    if (Buffer.Set_buffer_eventos(ID_Eventos, eventos, RTC))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_eventos(), 258);
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
        Serial.println("Set buffer general ERROR");
}

/*****************************************************************************************/
/******************************* TRANSMITE ROM SIGNATURE *********************************/
/*****************************************************************************************/

void Transmite_ROM_Signature(void)
{
    if (Buffer.Set_buffer_ROM_Singnature(ID_ROM_Signature, contadores))
    {
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_ROM_Singnature(), 258);
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
        Serial.println("Set buffer general ERROR");
}

/*****************************************************************************************/
/******************************* RESPONDE ECO BROADCAST **********************************/
/*****************************************************************************************/

void Transmite_Eco_Broadcast(void)
{
    char res[258] = {};
    bzero(res, 258); // Pone el buffer en 0

    // Direccion MAC ESP32
    Serial.print("Direccion MAC: ");
    Serial.println(WiFi.macAddress());
    String mac = WiFi.macAddress();

    // Direccion IP ESP32
    Serial.print("Direccion IP Local: ");
    uint32_t ip = WiFi.localIP();
    Serial.print("Direccion IP Local2: ");
    Serial.print(ip);

    // Socket de conexion
    Serial.print("Socket: ");
    Serial.println(client.remotePort());
    string socket = std::to_string(client.remotePort());

    // Mascara subred ESP32
    Serial.print("Mascara Subred: ");
    Serial.println(WiFi.subnetMask());

    // Puerta de enlace ESP32
    Serial.print("Puerta de enlace: ");
    Serial.println(WiFi.gatewayIP());

    res[0] = 'L';
    res[1] = '|';
    res[2] = '0';
    res[3] = '1';
    res[4] = '|';
    // MAC
    res[5] = '0';
    res[6] = mac[0];
    res[7] = mac[1];
    res[8] = mac[2];
    res[9] = '0';
    res[10] = mac[3];
    res[11] = mac[4];
    res[12] = mac[5];
    res[13] = '0';
    res[14] = mac[6];
    res[15] = mac[7];
    res[16] = mac[8];
    res[17] = '0';
    res[18] = mac[9];
    res[19] = mac[10];
    res[20] = mac[11];
    res[21] = '0';
    res[22] = mac[12];
    res[23] = mac[13];
    res[24] = mac[14];
    res[25] = '0';
    res[26] = mac[15];
    res[27] = mac[16];
    res[28] = '|';
    // PORT
    res[29] = '0';
    res[30] = socket[0];
    res[31] = socket[1];
    res[32] = socket[2];
    res[33] = socket[3];
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

/*****************************************************************************************/
/******************************* PROCESA COMANDO RECIBIDO ********************************/
/*****************************************************************************************/

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

/*****************************************************************************************/
/********************************** TAREA DE COMANDOS ************************************/
/*****************************************************************************************/

void Task_Procesa_Comandos(void *parameter)
{
    for (;;)
    {
        if (flag_dato_valido_recibido)
        {
            flag_dato_valido_recibido = false;

            char res[258] = {};
            bzero(res, 258); // Pone el buffer en 0
            memcpy(res, Buffer.Get_buffer_recepcion(), 258);

            switch (Comando_Recibido())
            {
            case 3:
                Serial.println("Solicitud de contadores SAS");
                Transmite_Contadores_Accounting();
                break;

            case 4:
                Serial.println("Solicitud de billetes SAS");
                Transmite_Billetes();
                
            case 7:
                Serial.println("Evento recibido con exito");
                break;

            case 8:
                Serial.println("Sincroniza reloj RTC");
                if (Sincroniza_Reloj_RTC(res))
                {
                    flag_sincronizacion_RTC = true;
                    Transmite_Confirmacion('A', '4');
                }
                else
                    Transmite_Confirmacion('A', '5');
                break;

            case 11:
                Serial.println("Solicitud de informacion MAQ");
                Transmite_ID_Maquina();
                break;

            case 12:
                Serial.println("Solicitud de ROM Signature Maq");
                Transmite_ROM_Signature();
                break;

            case 14:
                Serial.println("Solicitud Reset ESP32");
                Transmite_Confirmacion('A', 'F');
                ESP.restart();

            case 15:
                Serial.println("Solicitud de inactivar maquina");
                if (Inactiva_Maquina())
                    Transmite_Confirmacion('A', 'B');
                else
                    Transmite_Confirmacion('A', 'C');
                break;

            case 18:
                Serial.println("Solicitud de activar maquina");
                if (Activa_Maquina())
                    Transmite_Confirmacion('A', '9');
                else
                    Transmite_Confirmacion('A', 'A');
                break;

            case 181:
                Serial.println("Confirmacion contadores recibido con exito");
                //                Transmite_Confirmacion('A', '1');

            case 190:
                Serial.println("Serializa trama de contadores");
                if (!flag_serie_trama_contadores)
                {
                    if (contadores.Incrementa_Serie_Trama())
                    {
                        flag_serie_trama_contadores = true;
                        Transmite_Confirmacion('A', '1'); // Transmite ACK a Server
                    }
                }
                else
                    Transmite_Confirmacion('A', '1'); // Transmite ACK a Server
                break;

            case 308:
                Serial.println("Solicitud de contadores Cashless");
                Transmite_Contadores_Cashless();
                break;

            default:
                Serial.println(Comando_Recibido());
                for (int i = 0; i < 256; i++)
                {
                    Serial.print(res[i]);
                }
                Serial.println();
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
                Serial.println("Solicitud de Bootloader");
            else if (res[0] == 'E' && res[1] == 'B')
            {
                Serial.println("Eco Broadcast");
                Transmite_Eco_Broadcast();
            }
        }
        else if (flag_evento_valido_recibido)
        {
            Transmite_Eventos();
            flag_evento_valido_recibido = false;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
        continue;
    }
}

/*****************************************************************************************/
/******************************** TAREA DE TRANSMISION ***********************************/
/*****************************************************************************************/

void Task_Maneja_Transmision(void *parameter)
{
    for (;;)
    {
        Transmite_Configuracion();
        Transmision_Controlada_Contadores();
        vTaskDelay(30000 / portTICK_PERIOD_MS);
        continue;
    }
}

void Transmite_Configuracion(void)
{
    switch (Contador_Transmision)
    {
    case 1:
        if (!flag_sincronizacion_RTC)
            Transmite_Confirmacion('A', '3');
        break;

    case 2:
        if (!flag_serie_trama_contadores)
            Transmite_Confirmacion('B', 'C');

    default:
        Contador_Transmision = 0;
        break;
    }
    Contador_Transmision++;
}

void Transmision_Controlada_Contadores(void)
{
    Contador_Transmision_Contadores++;
    if (Contador_Transmision_Contadores >= 4)
    {
        Contador_Transmision_Contadores = 0;
        Transmite_Contadores_Accounting();
    }
}