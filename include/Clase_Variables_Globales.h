#include "Stream.h"
#include <iostream>
using namespace std;



#define Ftp_Mode                    1   /*Ftp activado[1] no activado [0]*/
#define Bootloader_Mode             2   /*Bootloader activado[1] no activado [0]*/
#define Enable_Storage              3   /*Escritura activada[1] no activada[0]*/
#define Dato_Entrante_Valido        4
#define Dato_Entrante_No_Valido     5  
#define Dato_Evento_Valido          6   /*Evento valido[1] no valido [0]*/
#define Sincronizacion_RTC          7   /*Sincronización de reloj*/
#define Serializacion_Serie_Trama   8   /*Contadores de tramas enviadas*/
#define Comunicacion_Maq            9   /* Comunicación maquina  Comunicando[1] no hay comunicación[0]*/
#define Archivo_CSV_OK              10 /*Archivos Eventos y Contadores creados*/
#define Archivo_LOG_OK              11 /* Archivo log creado*/
#define Primer_Cancel_Credit        12 /*Calculo primer encendido formula poker*/
#define Flag_Hopper_Enable          13 /*HIGH  señal en alto LOW señal en bajo*/
#define Flag_Maquina_En_Juego       14 /* Maquina en juego[1] Maquina no en  juego[0]*/

#define Encabezado_Maquina_Generica 15 /* Encabezado de archivo de contadores SAS*/
#define Encabezado_Maquina_Eventos  16 /*Descripción de eventos SAS*/
#define Estado_Escritura            17 /* encribiendo en archivos[1] no esta escribiendo [0] */
#define Libera_Memoria_OK           18 /* Liberando memoria[0] Memoria libre 1*/
#define Fallo_Archivo_COM           19 /* fallo creación archivo[1] no fallo[0]*/
#define Fallo_Archivo_EVEN          20 /* fallo creación archivo[1] no fallo[0]*/
#define Fallo_Archivo_LOG           21 /* fallo creación archivo[1] no fallo[0]*/

#define Calc_Cancel_Credit          22
#define Version_Firmware            23
#define Espacio_Libre_SD            24  /*Espacio libre de memoria SD*/
#define Enable_SD                   25  
#define Espacio_Usado_SD            26 /* Espacio usado de memoria SD*/
#define Size_SD                     27 /* Tamaño de memoria SD insertada*/
#define Temperatura_procesador      28 /*0°-150°*/
#define Consulta_Info_Cashless_OK   29
#define SD_INSERT                   35  /*Estado de memoria insertada [1] no insertada [0]*/
#define Flag_Memoria_SD_Full        36 /*Estado de memoria llena [1] o libre [0]*/
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
    bool Flag_Memoria_SD_Full_;
public:
    void Init_Variables_Globales();
    void Set_Variable_Global(int Filtro, bool Change_estado);
    bool Get_Variable_Global(int Filtro);
    String Get_Encabezado_Maquina(int Filtro);
    String Get_Variables_Global_String(int filtro);
    void Set_Variable_Global_String(int Filtro, String Dato);
};
