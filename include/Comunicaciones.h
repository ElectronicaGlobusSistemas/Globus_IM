#include "Internet.h"
#include "Memory_SD.h"

#define ID_Contadores_Accounting 3
#define ID_Billetes 4
#define ID_Eventos 5
#define ID_Maquina 11
#define ID_ROM_Signature 12
#define ID_Contadores_Cashless 308

int Contador_Transmision = 0;
int Contador_Transmision_Contadores = 0;

int Contador_Cancel_Credit_Ant = 0;
int Contador_Cancel_Credit_Act = 0;
bool flag_premio_pagado_cashout = false;

int Contador_Bill_In_Ant = 0;
int Contador_Bill_In_Act = 0;
bool flag_billete_insertado = false;

int Contador_Coin_In_Ant = 0;
int Contador_Coin_In_Act = 0;
bool flag_maquina_en_juego = false;

extern bool flag_ultimo_contador_Ok;

void Task_Procesa_Comandos(void *parameter);
void Task_Maneja_Transmision(void *parameter);
void Verifica_Cambio_Contadores(void);
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
/********************************* TRANSMITE A SERVIDOR **********************************/
/*****************************************************************************************/

void Transmite_A_Servidor(char buffer[], int len)
{
    Serial.print("Tamaño a enviar: ");
    Serial.println(len);
    //    Serial.println("buffer enviado: ");
    //    Serial.println(buffer);
    int length_ = 0;
    if (Configuracion.Get_Configuracion(Tipo_Conexion))
        length_ = clientTCP.write(buffer, len);
    else
    {
        memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));
        IPAddress serverIP(IP_Server[0], IP_Server[1], IP_Server[2], IP_Server[3]);
        uint16_t serverPort = Configuracion.Get_Configuracion(Puerto_Server, 0);

        clientUDP.beginPacket(serverIP, serverPort);
        length_ = clientUDP.write((const uint8_t *)buffer, len);
        clientUDP.endPacket();
    }
    Serial.print("Bytes enviados: ");
    Serial.println(length_);
    Serial.println("--------------------------------------------------------------------");
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
        Transmite_A_Servidor(res, len);
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
        if (Variables_globales.Get_Variable_Global(Serializacion_Serie_Trama))
        {
            contadores.Incrementa_Serie_Trama();
        }
        char res[258] = {};
        bzero(res, 258); // Pone el buffer en 0
        memcpy(res, Buffer.Get_buffer_contadores_ACC(), 258);
        Serial.println("Set buffer general OK");
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
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
        Transmite_A_Servidor(res, len);
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
        Transmite_A_Servidor(res, len);
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
    if ((hour == RTC.getHour(true)) && (minutes == RTC.getMinute()) && (day == RTC.getDay()) && ((month - 1) == RTC.getMonth()) && (year == RTC.getYear()))
    {
        Serial.println("RTC sincronizado con exito!");
        return true;
    }
    else
        return false;
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
        Transmite_A_Servidor(res, len);
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
        Transmite_A_Servidor(res, len);
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
        Transmite_A_Servidor(res, len);
    }
    else
        Serial.println("Set buffer general ERROR");
}

/*****************************************************************************************/
/******************************* TRANSMITE ROM SIGNATURE *********************************/
/*****************************************************************************************/

bool Configura_Tipo_Maquina(char res[])
{
    uint16_t ID_Maq = Configuracion.Get_Configuracion(Tipo_Maquina, 0);
    uint16_t ID_Maq_Server = res[4] - 48;

    if (ID_Maq != ID_Maq_Server)
    {
        Serial.println("Tipo de maquina diferente");
        NVS.begin("Config_ESP32", false);
        NVS.putUInt("TYPE_MAQ", ID_Maq_Server);
        uint16_t tipo_maq = NVS.getUInt("TYPE_MAQ", 0);
        NVS.end();
        if (tipo_maq == ID_Maq_Server)
        {
            Configuracion.Set_Configuracion_ESP32(Tipo_Maquina, tipo_maq);
            return true;
        }
        else
            return false;
    }
    else
    {
        Serial.println("Tipo de maquina igual");
        return true;
    }
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
    Serial.println(WiFi.localIP());

    // Socket de conexion
    Serial.print("Socket: ");
    string socket;
    if (Configuracion.Get_Configuracion(Tipo_Conexion))
    {
        Serial.println(clientTCP.remotePort());
        socket = std::to_string(clientTCP.remotePort());
    }
    else
    {
        Serial.println(clientUDP.remotePort());
        socket = std::to_string(clientUDP.remotePort());
    }

    // Mascara subred ESP32
    Serial.print("Mascara Subred: ");
    Serial.println(WiFi.subnetMask());

    // Puerta de enlace ESP32
    Serial.print("Puerta de enlace: ");
    Serial.println(WiFi.gatewayIP());

    // Nombre de la maquina
    Serial.print("Nombre de MAQ: ");
    String Name = Configuracion.Get_Configuracion(Nombre_Maquina, "Nombre_Maq");
    Serial.println(Name);

    Serial.print("**************************************************************************************");
    Serial.println();

    char IP[4];
    bzero(IP, 4);
    memcpy(IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(IP) / sizeof(IP[0]));

    char dir_ip[12] = {};
    int j = 0;
    for (int i = 0; i < 4; i++)
    {
        int octeto = (int)IP[i];
        dir_ip[j] = ((octeto - (octeto % 100)) / 100) + '0';
        j++;
        int resultado = octeto % 100;
        dir_ip[j] = ((resultado - (resultado % 10)) / 10) + '0';
        j++;
        dir_ip[j] = (resultado % 10) + '0';
        j++;
    }

    char IP_GW[4];
    bzero(IP_GW, 4);
    memcpy(IP_GW, Configuracion.Get_Configuracion(Direccion_IP_GW, 'x'), sizeof(IP_GW) / sizeof(IP_GW[0]));

    char dir_ip_gw[12] = {};
    j = 0;
    for (int i = 0; i < 4; i++)
    {
        int octeto = (int)IP_GW[i];
        dir_ip_gw[j] = ((octeto - (octeto % 100)) / 100) + '0';
        j++;
        int resultado = octeto % 100;
        dir_ip_gw[j] = ((resultado - (resultado % 10)) / 10) + '0';
        j++;
        dir_ip_gw[j] = (resultado % 10) + '0';
        j++;
    }

    char SN_MASK[4];
    bzero(SN_MASK, 4);
    memcpy(SN_MASK, Configuracion.Get_Configuracion(Direccion_SN_MASK, 'x'), sizeof(SN_MASK) / sizeof(SN_MASK[0]));

    char sn_mask[12] = {};
    j = 0;
    for (int i = 0; i < 4; i++)
    {
        int octeto = (int)SN_MASK[i];
        sn_mask[j] = ((octeto - (octeto % 100)) / 100) + '0';
        j++;
        int resultado = octeto % 100;
        sn_mask[j] = ((resultado - (resultado % 10)) / 10) + '0';
        j++;
        sn_mask[j] = (resultado % 10) + '0';
        j++;
    }

    res[0] = 'L';
    res[1] = '|';
    res[2] = '0';
    res[3] = '1';
    res[4] = '|';
    // MAC
    res[5] = mac[0];
    res[6] = mac[1];
    res[7] = mac[2];
    res[8] = mac[3];
    res[9] = mac[4];
    res[10] = mac[5];
    res[11] = mac[6];
    res[12] = mac[7];
    res[13] = mac[8];
    res[14] = mac[9];
    res[15] = mac[10];
    res[16] = mac[11];
    res[17] = mac[12];
    res[18] = mac[13];
    res[19] = mac[14];
    res[20] = mac[15];
    res[21] = mac[16];
    res[22] = '|';
    // PORT
    res[23] = '0';
    res[24] = socket[0];
    res[25] = socket[1];
    res[26] = socket[2];
    res[27] = socket[3];
    res[28] = '|';
    // IP
    res[29] = dir_ip[0];
    res[30] = dir_ip[1];
    res[31] = dir_ip[2];
    res[32] = '.';
    res[33] = dir_ip[3];
    res[34] = dir_ip[4];
    res[35] = dir_ip[5];
    res[36] = '.';
    res[37] = dir_ip[6];
    res[38] = dir_ip[7];
    res[39] = dir_ip[8];
    res[40] = '.';
    res[41] = dir_ip[9];
    res[42] = dir_ip[10];
    res[43] = dir_ip[11];
    res[44] = '|';
    // MASCARA
    res[45] = sn_mask[0];
    res[46] = sn_mask[1];
    res[47] = sn_mask[2];
    res[48] = '.';
    res[49] = sn_mask[3];
    res[50] = sn_mask[4];
    res[51] = sn_mask[5];
    res[52] = '.';
    res[53] = sn_mask[6];
    res[54] = sn_mask[7];
    res[55] = sn_mask[8];
    res[56] = '.';
    res[57] = sn_mask[9];
    res[58] = sn_mask[10];
    res[59] = sn_mask[11];
    res[60] = '|';
    // IP Enlace
    res[61] = dir_ip_gw[0];
    res[62] = dir_ip_gw[1];
    res[63] = dir_ip_gw[2];
    res[64] = '.';
    res[65] = dir_ip_gw[3];
    res[66] = dir_ip_gw[4];
    res[67] = dir_ip_gw[5];
    res[68] = '.';
    res[69] = dir_ip_gw[6];
    res[70] = dir_ip_gw[7];
    res[71] = dir_ip_gw[8];
    res[72] = '.';
    res[73] = dir_ip_gw[9];
    res[74] = dir_ip_gw[10];
    res[75] = dir_ip_gw[11];
    res[76] = '|';
    // Nombre MAQ
    res[77] = Name[0];
    res[78] = Name[1];
    res[79] = Name[2];
    res[80] = Name[3];
    res[81] = Name[4];
    res[82] = Name[5];
    res[83] = Name[6];
    res[84] = Name[7];
    res[85] = Name[8];
    res[86] = Name[9];
    res[87] = Name[10];
    res[88] = Name[11];
    res[89] = Name[12];
    res[90] = Name[13];
    res[91] = Name[14];
    res[92] = Name[15];

    Serial.println("Set buffer general OK");
    int len = sizeof(res);
    Transmite_A_Servidor(res, len);
    ;
}

/*****************************************************************************************/
/*************************** GUARDA LA NUEVA CONFIGURACION *******************************/
/*****************************************************************************************/

void Guarda_Configuracion_ESP32(void)
{
    char req[258] = {};
    bzero(req, 258); // Pone el buffer en 0
    memcpy(req, Buffer.Get_buffer_recepcion(), 258);

    char res[258] = {};
    bzero(res, 258); // Pone el buffer en 0

    bool cambios = false;

    if (req[91] != 0x0A || req[92] != 0x0D)
    {
        Serial.println();
        Serial.println("Error de Set All...");
        Serial.println(req[91], HEX);
        Serial.println(req[92], HEX);
        res[0] = 'E';
        res[1] = '0';
        res[2] = '0';
        res[3] = 0x0A;
        res[4] = 0x0D;

        for (int i = 5; i < 256; i++)
        {
            res[i] = '0';
        }

        Serial.println("Set buffer general OK");
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
    }
    else
    {
        int j = 0;
        bool diferencia = false;

        /*******************************************************************************************************/
        /*******************************************************************************************************/
        // Verifica MAC
        Serial.println();
        for (int i = 3; i < 20; i++)
        {
            Serial.print(req[i]);
        }
        String MAC = WiFi.macAddress();
        j = 0;
        for (int i = 3; i < 20; i++)
        {
            if (req[i] != MAC[j])
                return;
            j++;
        }

        Serial.println();
        Serial.println("Set all Ok...");

        /*******************************************************************************************************/
        /*******************************************************************************************************/
        // Verifica Puerto de conexion
        for (int i = 21; i < 26; i++)
        {
            Serial.print(req[i]);
        }
        Serial.println();
        uint16_t puerto = Configuracion.Get_Configuracion(Puerto_Server, 0);
        uint16_t puerto_server;
        puerto_server = ((int)req[22] - 48) * 1000;
        puerto_server += ((int)req[23] - 48) * 100;
        puerto_server += ((int)req[24] - 48) * 10;
        puerto_server += ((int)req[25] - 48);

        if (puerto != puerto_server)
        {
            NVS.begin("Config_ESP32", false);
            Serial.println("Si hay diferencias en Puerto de conexion...");
            NVS.putUInt("Socket", puerto_server);
            uint16_t port_server = NVS.getUInt("Socket", 0);
            Configuracion.Set_Configuracion_ESP32(Puerto_Server, port_server);
            cambios = true;
            NVS.end();
        }

        /*******************************************************************************************************/
        /*******************************************************************************************************/
        // Verifica direccion IP Local
        for (int i = 27; i < 42; i++)
        {
            Serial.print(req[i]);
        }
        Serial.println();
        char IP[4];
        bzero(IP, 4);
        memcpy(IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(IP) / sizeof(IP[0]));
        char dir_ip[12] = {};
        char dir_ip_server[12] = {};
        j = 0;
        for (int i = 0; i < 4; i++)
        {
            int octeto = (int)IP[i];
            dir_ip[j] = ((octeto - (octeto % 100)) / 100) + '0';
            j++;
            int resultado = octeto % 100;
            dir_ip[j] = ((resultado - (resultado % 10)) / 10) + '0';
            j++;
            dir_ip[j] = (resultado % 10) + '0';
            j++;
        }
        j = 0;
        for (int i = 27; i < 42; i++)
        {
            if (req[i] != '.')
            {
                dir_ip_server[j] = req[i];
                j++;
            }
        }
        for (int i = 0; i < 12; i++)
        {
            if (dir_ip[i] != dir_ip_server[i])
                diferencia = true;
        }
        if (diferencia)
        {
            NVS.begin("Config_ESP32", false);
            Serial.println("Si hay diferencias en IP...");
            uint8_t ip[4] = {};
            j = 0;
            for (int i = 0; i < 4; i++)
            {
                int numero = (dir_ip_server[j] - 48);
                int octeto = numero * 100;
                j++;
                numero = (dir_ip_server[j] - 48);
                octeto += (numero * 10);
                j++;
                numero = (dir_ip_server[j] - 48);
                octeto += numero;
                j++;
                ip[i] = octeto;
            }
            if (NVS.putBytes("Dir_IP", ip, sizeof(ip)) == 4)
            {
                size_t ip_len = NVS.getBytesLength("Dir_IP");
                char IP[ip_len];
                NVS.getBytes("Dir_IP", IP, ip_len);
                Configuracion.Set_Configuracion_ESP32(Direccion_IP, IP);
            }
            NVS.end();
            diferencia = false;
            cambios = true;
        }

        /*******************************************************************************************************/
        /*******************************************************************************************************/
        // Verifica MASK_SUBRED
        for (int i = 43; i < 58; i++)
        {
            Serial.print(req[i]);
        }
        Serial.println();
        char SN_MASK[4];
        bzero(SN_MASK, 4);
        memcpy(SN_MASK, Configuracion.Get_Configuracion(Direccion_SN_MASK, 'x'), sizeof(SN_MASK) / sizeof(SN_MASK[0]));
        char sn_mask[12] = {};
        char sn_mask_server[12] = {};
        j = 0;
        for (int i = 0; i < 4; i++)
        {
            int octeto = (int)SN_MASK[i];
            sn_mask[j] = ((octeto - (octeto % 100)) / 100) + '0';
            j++;
            int resultado = octeto % 100;
            sn_mask[j] = ((resultado - (resultado % 10)) / 10) + '0';
            j++;
            sn_mask[j] = (resultado % 10) + '0';
            j++;
        }
        j = 0;
        for (int i = 43; i < 58; i++)
        {
            if (req[i] != '.')
            {
                sn_mask_server[j] = req[i];
                j++;
            }
        }
        for (int i = 0; i < 12; i++)
        {
            if (sn_mask[i] != sn_mask_server[i])
                diferencia = true;
        }
        if (diferencia)
        {
            NVS.begin("Config_ESP32", false);
            Serial.println("Si hay diferencias en SN_MASK...");
            uint8_t sn_mask[4] = {};
            j = 0;
            for (int i = 0; i < 4; i++)
            {
                int numero = (sn_mask_server[j] - 48);
                int octeto = numero * 100;
                j++;
                numero = (sn_mask_server[j] - 48);
                octeto += (numero * 10);
                j++;
                numero = (sn_mask_server[j] - 48);
                octeto += numero;
                j++;
                sn_mask[i] = octeto;
            }
            if (NVS.putBytes("Dir_SN_MASK", sn_mask, sizeof(sn_mask)) == 4)
            {
                size_t sn_mask_len = NVS.getBytesLength("Dir_SN_MASK");
                char SN_MASK[sn_mask_len];
                NVS.getBytes("Dir_SN_MASK", SN_MASK, sn_mask_len);
                Configuracion.Set_Configuracion_ESP32(Direccion_SN_MASK, SN_MASK);
            }
            NVS.end();
            diferencia = false;
            cambios = true;
        }

        /*******************************************************************************************************/
        /*******************************************************************************************************/
        // Verifica IP_GW
        for (int i = 59; i < 74; i++)
        {
            Serial.print(req[i]);
        }
        Serial.println();
        char IP_GW[4];
        bzero(IP_GW, 4);
        memcpy(IP_GW, Configuracion.Get_Configuracion(Direccion_IP_GW, 'x'), sizeof(IP_GW) / sizeof(IP_GW[0]));
        char ip_gw[12] = {};
        char ip_gw_server[12] = {};
        j = 0;
        for (int i = 0; i < 4; i++)
        {
            int octeto = (int)IP_GW[i];
            ip_gw[j] = ((octeto - (octeto % 100)) / 100) + '0';
            j++;
            int resultado = octeto % 100;
            ip_gw[j] = ((resultado - (resultado % 10)) / 10) + '0';
            j++;
            ip_gw[j] = (resultado % 10) + '0';
            j++;
        }
        j = 0;
        for (int i = 59; i < 74; i++)
        {
            if (req[i] != '.')
            {
                ip_gw_server[j] = req[i];
                j++;
            }
        }
        for (int i = 0; i < 12; i++)
        {
            if (ip_gw[i] != ip_gw_server[i])
                diferencia = true;
        }
        if (diferencia)
        {
            NVS.begin("Config_ESP32", false);
            Serial.println("Si hay diferencias en IP_Enlace...");
            uint8_t ip_gw[4] = {};
            j = 0;
            for (int i = 0; i < 4; i++)
            {
                int numero = (ip_gw_server[j] - 48);
                int octeto = numero * 100;
                j++;
                numero = (ip_gw_server[j] - 48);
                octeto += (numero * 10);
                j++;
                numero = (ip_gw_server[j] - 48);
                octeto += numero;
                j++;
                ip_gw[i] = octeto;
            }
            if (NVS.putBytes("Dir_IP_GW", ip_gw, sizeof(ip_gw)) == 4)
            {
                size_t ip_gw_len = NVS.getBytesLength("Dir_IP_GW");
                char IP_GW[ip_gw_len];
                NVS.getBytes("Dir_IP_GW", IP_GW, ip_gw_len);
                Configuracion.Set_Configuracion_ESP32(Direccion_IP_GW, IP_GW);
            }
            NVS.end();
            diferencia = false;
            cambios = true;
        }

        /*******************************************************************************************************/
        /*******************************************************************************************************/
        // Verifica Nombre de MAQ
        char Nombre_Maq[16];
        bzero(Nombre_Maq, 16);
        char Nombre_Maq_Server[17];
        bzero(Nombre_Maq_Server, 17);
        diferencia = false;
        strcpy(Nombre_Maq, Configuracion.Get_Configuracion(Nombre_Maquina, "Nombre_Maq").c_str());
        j = 0;
        for (int i = 75; i < 91; i++)
        {
            Nombre_Maq_Server[j] = req[i];
            if (Nombre_Maq[j] != req[i])
                diferencia = true;
            j++;
        }
        if (diferencia)
        {
            NVS.begin("Config_ESP32", false);
            Serial.println("Si hay diferencias en NOMBRE_MAQ...");
            if (NVS.putString("Name_Maq", Nombre_Maq_Server) == 16)
            {
                String name = NVS.getString("Name_Maq");
                Configuracion.Set_Configuracion_ESP32(Nombre_Maquina, name);
            }
            NVS.end();
            diferencia = false;
            cambios = true;
        }

        /*******************************************************************************************************/
        /*******************************************************************************************************/

        res[0] = 'L';
        res[1] = '|';
        res[2] = '0';
        res[3] = '2';
        res[4] = '|';
        res[5] = 'O';
        res[6] = 'K';
        res[7] = '|';
        res[8] = 0x0A;
        res[9] = 0x0D;

        for (int i = 10; i < 256; i++)
        {
            res[i] = '0';
        }

        Serial.println("Set buffer general OK");
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
        if (cambios)
            ESP.restart();
    }
}

/*****************************************************************************************/
/*************************** INICIALIZA MODO BOOTLOADER **********************************/
/*****************************************************************************************/

bool Inicializa_modo_bootloader(void)
{
    char res[258] = {};
    bzero(res, 258); // Pone el buffer en 0

    Variables_globales.Set_Variable_Global(Bootloader_Mode, true);
    if (Variables_globales.Get_Variable_Global(Bootloader_Mode))
    {
        res[0] = 'L';
        res[1] = '|';
        res[2] = 'S';
        res[3] = 'B';
        res[4] = '|';
        res[5] = 'O';
        res[6] = 'K';

        for (int i = 7; i < 256; i++)
        {
            res[i] = '0';
        }

        Serial.println("Set buffer general OK");
        int len = sizeof(res);
        Transmite_A_Servidor(res, len);
        return true;
    }
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
        if (Variables_globales.Get_Variable_Global(Dato_Entrante_Valido))
        {
            Variables_globales.Set_Variable_Global(Dato_Entrante_Valido, false);
            //            flag_dato_valido_recibido = false;

            char res[258] = {};
            bzero(res, 258); // Pone el buffer en 0
            memcpy(res, Buffer.Get_buffer_recepcion(), 258);

            switch (Comando_Recibido())
            {
            case 3:
                Serial.println("Solicitud de contadores SAS");
                if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                    Transmite_Contadores_Accounting();
                else
                    Transmite_Confirmacion('A', '0');
                break;

            case 4:
                Serial.println("Solicitud de billetes SAS");
                if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                    Transmite_Billetes();
                else
                    Transmite_Confirmacion('A', '0');
                break;

            case 7:
                Serial.println("Evento recibido con exito");
                break;

            case 8:
                Serial.println("Sincroniza reloj RTC");
                if (Sincroniza_Reloj_RTC(res))
                {
                    Variables_globales.Set_Variable_Global(Sincronizacion_RTC, true);
                    Transmite_Confirmacion('A', '4');
                }
                else
                    Transmite_Confirmacion('A', '5');
                break;

            case 11:
                Serial.println("Solicitud de informacion MAQ");
                if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                    Transmite_ID_Maquina();
                else
                    Transmite_Confirmacion('A', '0');
                break;

            case 12:
                Serial.println("Solicitud de ROM Signature Maq");
                if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                    Transmite_ROM_Signature();
                else
                    Transmite_Confirmacion('A', '0');
                break;

            case 14:
                Serial.println("Solicitud Reset ESP32");
                Transmite_Confirmacion('A', 'F');
                ESP.restart();

            case 15:
                Serial.println("Solicitud de inactivar maquina");
                if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                {
                    if (Inactiva_Maquina())
                        Transmite_Confirmacion('A', 'B');
                    else
                        Transmite_Confirmacion('A', 'C');
                }
                else
                    Transmite_Confirmacion('A', '0');
                break;

            case 18:
                Serial.println("Solicitud de activar maquina");
                if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                {
                    if (Activa_Maquina())
                        Transmite_Confirmacion('A', '9');
                    else
                        Transmite_Confirmacion('A', 'A');
                }
                else
                    Transmite_Confirmacion('A', '0');
                break;

            case 181:
                Serial.println("Confirmacion contadores recibido con exito");
                //                Transmite_Confirmacion('A', '1');

            case 190:
                Serial.println("Serializa trama de contadores");
                if (!Variables_globales.Get_Variable_Global(Serializacion_Serie_Trama))
                {
                    if (contadores.Incrementa_Serie_Trama())
                    {
                        Variables_globales.Set_Variable_Global(Serializacion_Serie_Trama, true);
                        Transmite_Confirmacion('A', '1'); // Transmite ACK a Server
                    }
                }
                else
                    Transmite_Confirmacion('A', '1'); // Transmite ACK a Server
                break;

            case 200:
                Serial.println("Solicitud de configuracion maquina");
                if ((res[4] < 7) && Configura_Tipo_Maquina(res))
                    Transmite_Confirmacion('C', '9');
                else
                    Transmite_Confirmacion('C', 'A');
                break;

            case 308:
                Serial.println("Solicitud de contadores Cashless");
                if (Variables_globales.Get_Variable_Global(Comunicacion_Maq))
                    Transmite_Contadores_Cashless();
                else
                    Transmite_Confirmacion('A', '0');
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
        else if (Variables_globales.Get_Variable_Global(Dato_Entrante_No_Valido))
        {
            Variables_globales.Set_Variable_Global(Dato_Entrante_No_Valido, false);

            char res[258] = {};
            bzero(res, 258); // Pone el buffer en 0
            memcpy(res, Buffer.Get_buffer_recepcion(), 258);

            if (res[0] == 'S' && res[1] == 'B')
            {
                Serial.println("Solicitud de Bootloader");
                Inicializa_modo_bootloader();
            }
            else if (res[0] == 'E' && res[1] == 'B')
            {
                Serial.println("Eco Broadcast");
                Transmite_Eco_Broadcast();
            }
            else if (res[0] == 'S' && res[1] == 'A')
            {
                Serial.println("Guarda Configuracion");
                for (int i = 0; i < 117; i++)
                {
                    Serial.print(res[i]);
                }
                Guarda_Configuracion_ESP32();
            }
            else
            {
                for (int i = 0; i < 256; i++)
                {
                    Serial.print(res[i]);
                }
                Serial.println();
                Transmite_Confirmacion('C', 'R');
            }
        }
        else if (Variables_globales.Get_Variable_Global(Dato_Evento_Valido))
        {
            Transmite_Eventos();
            Variables_globales.Set_Variable_Global(Dato_Evento_Valido, false);

            char evento = eventos.Get_evento();
            Serial.print("Evento enviado....... ");
            Serial.println(evento, HEX);
            switch (evento)
            {
            case 0x51:
                Verifica_Premio_1B();
                break;

            default:
                break;
            }
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
        Verifica_Cambio_Contadores();
        Transmite_Configuracion();
        Transmision_Controlada_Contadores();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        continue;
    }
}

void Verifica_Cambio_Contadores(void)
{
    if (flag_ultimo_contador_Ok && Variables_globales.Get_Variable_Global(Comunicacion_Maq))
    {
        // DETECTA CAMBIO COIN IN - MAQUINA EN JUEGO
        Contador_Coin_In_Act = contadores.Get_Contadores_Int(Coin_In);
        if (Contador_Coin_In_Ant == 0)
            Contador_Coin_In_Ant = Contador_Coin_In_Act;
        else if (Contador_Coin_In_Act > Contador_Coin_In_Ant)
        {
            Contador_Coin_In_Ant = Contador_Coin_In_Act;
            flag_maquina_en_juego = true;
        }

        // DETECTA CAMBIO CANCEL CREDIT - PREMIO PAGADO
        Contador_Cancel_Credit_Act = contadores.Get_Contadores_Int(Cancel_Credit_Hand_Pay);
        if (Contador_Cancel_Credit_Ant == 0)
            Contador_Cancel_Credit_Ant = Contador_Cancel_Credit_Act;
        else if (Contador_Cancel_Credit_Act > Contador_Cancel_Credit_Ant)
        {
            Contador_Cancel_Credit_Ant = Contador_Cancel_Credit_Act;
            flag_premio_pagado_cashout = true;
        }

        // DETECTA CAMBIO BILLETERO - BILLETE INSERTADO
        Contador_Bill_In_Act = contadores.Get_Contadores_Int(Bill_Amount);
        if (Contador_Bill_In_Ant == 0)
            Contador_Bill_In_Ant = Contador_Bill_In_Act;
        else if (Contador_Bill_In_Act > Contador_Bill_In_Ant)
        {
            Contador_Bill_In_Ant = Contador_Bill_In_Act;
            flag_billete_insertado = true;
        }
    }
}

void Transmite_Configuracion(void)
{
    switch (Contador_Transmision)
    {
    case 10:
        if (!Variables_globales.Get_Variable_Global(Sincronizacion_RTC))
            Transmite_Confirmacion('A', '3');
        break;

    case 20:
        if (!Variables_globales.Get_Variable_Global(Comunicacion_Maq))
            Transmite_Confirmacion('A', '0');
        break;

    case 30:
        if (!Variables_globales.Get_Variable_Global(Serializacion_Serie_Trama))
        {
            Transmite_Confirmacion('B', 'C');
            Contador_Transmision = 0;
        }
    }
    Contador_Transmision++;
}

void Transmision_Controlada_Contadores(void)
{
    Contador_Transmision_Contadores++;

    if (Variables_globales.Get_Variable_Global(Comunicacion_Maq)) // Si hay comunicacion con la maquina...
    {
        // Si la maquina NO esta en juego, transmite cada 2 minutos, si el valor es 120
        if (!flag_maquina_en_juego && !flag_premio_pagado_cashout && !flag_billete_insertado)
        {
            if (Contador_Transmision_Contadores >= 120)
            {
                Serial.println("Contadores, maquina NO juego....");
                Contador_Transmision_Contadores = 0;
                Transmite_Contadores_Accounting();
            }
        }

        // Si la maquina SI esta en juego, transmite cada 30 segundos, si el valor es 30
        else if (flag_maquina_en_juego && !flag_premio_pagado_cashout && !flag_billete_insertado)
        {
            if (Contador_Transmision_Contadores >= 30)
            {
                Serial.println("Contadores, maquina SI juego....");
                flag_maquina_en_juego = false;
                Contador_Transmision_Contadores = 0;
                Transmite_Contadores_Accounting();
            }
        }

        // Si cambio el cancel credit, porque se pago un premio
        else if (flag_premio_pagado_cashout)
        {
            Serial.println("Contadores, premio pagado....");
            Transmite_Contadores_Accounting();
            flag_premio_pagado_cashout = false;
        }

        // Si cambio el billetero, porque se ingreso un nuevo billete
        else if (flag_billete_insertado)
        {
            Serial.println("Contadores, billete insertado....");
            Transmite_Contadores_Accounting();
            flag_billete_insertado = false;
        }
    }
    else
    {
        if (Contador_Transmision_Contadores >= 120)
        {
            Contador_Transmision_Contadores = 0;
            Transmite_Confirmacion('A', '0');
        }
    }
}
