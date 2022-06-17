#include <Arduino.h>
#include <stdio.h>

#define Direccion_IP 1
#define Direccion_IP_GW 2
#define Direccion_SN_MASK 3
#define Direccion_IP_Server 4
#define Puerto_Server 5
#define Nombre_Maquina 6
#define SSID 7
#define Password 8
#define Tipo_Conexion 9

class Configuracion_ESP32
{
private:
    char Direccion_IP_[4];
    char Direccion_IP_GW_[4];
    char Direccion_SN_MASK_[4];
    char Direccion_IP_Server_[4];
    uint16_t Puerto_Server_;
    String Nombre_Maquina_;
    String SSID_;
    String Password_;
    bool Tipo_Conexion_;

public:
    bool Set_Configuracion_ESP32(int, char[]);
    bool Set_Configuracion_ESP32(int, uint16_t);
    bool Set_Configuracion_ESP32(int, String);
    bool Set_Configuracion_ESP32(int, bool);
    char *Get_Configuracion(int, char);
    uint16_t Get_Configuracion(int, int);
    String Get_Configuracion(int, String);
    bool Get_Configuracion(int);
};