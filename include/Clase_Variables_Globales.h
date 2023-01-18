#include "Stream.h"
#include <iostream>
using namespace std;



#define Ftp_Mode                    1
#define Bootloader_Mode             2
#define Enable_Storage              3
#define Dato_Entrante_Valido        4
#define Dato_Entrante_No_Valido     5
#define Dato_Evento_Valido          6
#define Sincronizacion_RTC          7
#define Serializacion_Serie_Trama   8
#define Comunicacion_Maq            9
#define Archivo_CSV_OK              10
#define Archivo_LOG_OK              11
#define Primer_Cancel_Credit        12
#define Flag_Hopper_Enable          13
#define Flag_Maquina_En_Juego       14

#define Encabezado_Maquina_Generica 15
#define Encabezado_Maquina_Eventos  16
#define Estado_Escritura            17
#define Libera_Memoria_OK           18
#define Fallo_Archivo_COM           19
#define Fallo_Archivo_EVEN          20
#define Fallo_Archivo_LOG           21

#define Calc_Cancel_Credit          22
#define Version_Firmware            23
#define Espacio_Libre_SD            24
#define Enable_SD                   25
#define Espacio_Usado_SD            26
#define Size_SD                     27
#define Temperatura_procesador      28
#define Consulta_Info_Cashless_OK   29
#define SD_INSERT                   35

class Variables_Globales
{
private:
    bool Ftp_Mode_;
    bool Bootloader_Mode_;
    bool Dato_Entrante_Valido_;
    bool Dato_Entrante_No_Valido_;
    bool Dato_Evento_Valido_;
    bool Sincronizacion_RTC_;
    bool Serializacion_Serie_Trama_;
    bool Comunicacion_Maq_;
    bool Archivo_LOG_OK_;
    bool Archivo_CSV_OK_;
    bool Fallo_Archivo_COM_;
    bool Fallo_Archivo_EVEN_;
    bool Fallo_Archivo_LOG_;
    bool Enable_Storage_;
    String Encabezado_Maquina_Generica_;
    String Encabezado_Maquina_Eventos_;
    bool Estado_Escritura_;
    bool Libera_Memoria_OK_;
    bool Primer_Cancel_Credit_;
    bool Calcula_Cancel_Credit_;
    bool Flag_Hopper_Enable_;
    bool Flag_Maquina_En_Juego_;
    float Version_Firmware_;
    String Espacio_Libre_SD_;
    bool Enable_SD_;
    String Espacio_Usado_SD_;
    String Size_SD_;
    String Temperatura_procesador_;
    bool Consulta_info_Cashless_OK_;
    bool SD_INSERT_;

public:
    void Init_Variables_Globales();
    void Set_Variable_Global(int Filtro, bool Change_estado);
    bool Get_Variable_Global(int Filtro);
    String Get_Encabezado_Maquina(int Filtro);
    String Get_Variables_Global_String(int filtro);
    void Set_Variable_Global_String(int Filtro, String Dato);
};
