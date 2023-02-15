//Eventos SAS.
#include "Stream.h"
#include <iostream>
using namespace std;
#include "string.h"

#define Puerta_Abierta                              0x11
#define Puerta_Cerrada                              0x12
#define Maquina_Encendida                           0x17
#define Maquina_Apagada                             0x18
#define Billete_Aceptado                            0x4F
#define Hanpay_Pendiente                            0x51
#define Hanpay_Reset                                0x52
#define Cashout_Presionado                          0x66
#define Transferencia_AFT_Completa                  0x69
#define Requerimiento_AFT_Host_Cashout              0x6A
#define Requerimiento_AFT_Solicitud_Registro        0x6C
#define Requerimiento_AFT_Interrogacion_Registro    0x6D
#define Requerimiento_AFT_Cancelar_Registro         0x6E
#define Juego_Seleccionado                          0x8C
#define Ingreso_Pantalla_Contadores                 0x82
#define Salida_Pantalla_Contadores                  0x83
#define Caja_Dinero_removida                        0x1B
#define Caja_dinero_Instalada                       0x1C
#define Validador_billetes_Reset                    0x7B
#define CMOS_Ram_Error_Data_Recovered               0x31
#define CMOS_Ram_Error_No_Data_Recovered            0x32
#define CMOS_Ram_Error_Bad_device                   0x33
#define Billete_falso                               0x2C
#define EEPROM_Error_data                           0x34
#define EEPROM_Error_Bad_device                     0x35
#define EEPROM_Error_Diferent_checksum_vchanged     0x36
#define EEPROM_Error_Bad_checksum_compare           0x37
#define Partitioned_EEPROM_Error_Bad_checksum_v     0x38
#define Partitioned_EEPROM_Error_Bad_checksum_c     0x39
#define Memory_Error_Reset                          0x3A

class Tabla_Eventos
{
private:
    String Puerta_Abierta_="";
    String Puerta_Cerrada_="";
    String Maquina_Encendida_="";
    String Maquina_Apagada_="";
    String Billete_Aceptado_="";
    String Hanpay_Pendiente_="";
    String Hanpay_Reset_="";
    String Cashout_Presionado_="";
    String Transferencia_AFT_Completa_="";
    String Requerimiento_AFT_Host_Cashout_="";
    String Requerimiento_AFT_Solicitud_Registro_="";
    String Requerimiento_AFT_Interrogacion_Registro_="";
    String Requerimiento_AFT_Cancelar_Registro_="";
    String Evento_No_Identificado_="";
    String Juego_Seleccionado_="";
    String Ingreso_Pantalla_Contadores_="";
    String Salida_Pantalla_Contadores_="";
    String Caja_Dinero_removida_="";
    String Caja_dinero_Instalada_="";
    String Validador_billetes_Reset_="";
    String CMOS_Ram_Error_Data_Recovered_="";
    String CMOS_Ram_Error_No_Data_Recovered_="";
    String CMOS_Ram_Error_Bad_device_="";
    String Billete_falso_="";
    String EEPROM_Error_data_="";
    String EEPROM_Error_Bad_device_="";
    String EEPROM_Error_Diferent_checksum_vchanged_="";
    String EEPROM_Error_Bad_checksum_compare_="";
    String Partitioned_EEPROM_Error_Bad_checksum_v_="";
    String Partitioned_EEPROM_Error_Bad_checksum_c_="";
    String Memory_Error_Reset_="";


public:
   String Get_Descrip_Eventos(int Eventoss);
    void  Init_Tabla_Eventos(void);
};

String Tabla_Eventos::Get_Descrip_Eventos(int Eventoss)
{
    switch (Eventoss)
    {
    case 0x11:
        return Puerta_Abierta_;
        break;
    case 0x12:
        return Puerta_Cerrada_;
        break;
    case 0x17:
        return Maquina_Encendida_;
        break;
    case 0x18:
        return Maquina_Apagada_;
        break;
    case 0x4F:
        return Billete_Aceptado_;
        break;
    case 0x51:
        return Hanpay_Pendiente_;
        break;
    case 0x52:
        return Hanpay_Reset_;
        break;
    case 0x66:
        return Cashout_Presionado_;
        break;
    case 0x69:
        return Transferencia_AFT_Completa_;
        break;
    case 0x6A:
        return Requerimiento_AFT_Host_Cashout_;
        break;
    case 0x6C:
        return Requerimiento_AFT_Solicitud_Registro_;
        break;
    case 0x6D:
        return Requerimiento_AFT_Interrogacion_Registro_;
        break;
    case 0x6E:
        return Requerimiento_AFT_Cancelar_Registro_;
        break;

    case Juego_Seleccionado:
        return Juego_Seleccionado_;
        break;
    case Ingreso_Pantalla_Contadores:
        return Ingreso_Pantalla_Contadores_;
        break;

    case Salida_Pantalla_Contadores:
        return Salida_Pantalla_Contadores_;
        break;
    case Caja_Dinero_removida:
        return Caja_Dinero_removida_;
        break;
    case Caja_dinero_Instalada:
        return Caja_dinero_Instalada_;
        break;
    case Validador_billetes_Reset:
        return Validador_billetes_Reset_;
        break;
    case CMOS_Ram_Error_Data_Recovered: /*CMOS RAM ERROR (data recovered EEPROM)*/
        return CMOS_Ram_Error_Data_Recovered_;
        break;
    case CMOS_Ram_Error_No_Data_Recovered: /*CMOS RAM ERROR (no data recovered EEPROM)*/
        return CMOS_Ram_Error_No_Data_Recovered_;
        break;
    case CMOS_Ram_Error_Bad_device: /*CMOS RAM ERROR(bad device)*/
        return CMOS_Ram_Error_Bad_device_;
        break;
    case Billete_falso: /* Billete falso  detectado*/
        return Billete_falso_;
        break;
    case EEPROM_Error_data: /*EEPROM error (data error)*/
        return EEPROM_Error_data_;
        break;
    case EEPROM_Error_Bad_device: /*EEPROM error (bad device)*/
        return EEPROM_Error_Bad_device_;
        break;
    case EEPROM_Error_Diferent_checksum_vchanged: /*EEPROM error (diffent checksum version changed)*/
        return EEPROM_Error_Diferent_checksum_vchanged_;
        break;
    case EEPROM_Error_Bad_checksum_compare: /*EPROM error bad checksum compare*/
        return EEPROM_Error_Bad_checksum_compare_;
        break;
    case Partitioned_EEPROM_Error_Bad_checksum_v: /*Partitioned EEPROM error (checksum version changed)*/
        return Partitioned_EEPROM_Error_Bad_checksum_v_;
        break;
    case Partitioned_EEPROM_Error_Bad_checksum_c: /*Partitioned EEPROM error (bad checksum compare)*/
        return Partitioned_EEPROM_Error_Bad_checksum_c_;
        break;
    case Memory_Error_Reset: /*Memory error reset (operador used self test)*/
        return Memory_Error_Reset_;
        break;
    default:
        return Evento_No_Identificado_;
        break;
    }
}

void Tabla_Eventos::Init_Tabla_Eventos(void)
{
    Puerta_Abierta_ = "Puerta Abierta";
    Puerta_Cerrada_ = "Puerta Cerrada";
    Maquina_Encendida_ = "Maquina Encendida";
    Maquina_Apagada_ = "Puerta Cerrada";
    Billete_Aceptado_ = "Billete Acetado";
    Hanpay_Pendiente_ = "Hanpay Pendiente";
    Hanpay_Reset_ = "Hanpay_Reset";
    Cashout_Presionado_ = "Cashout Presionado";
    Transferencia_AFT_Completa_ = "Transferencia AFT Completa";
    Requerimiento_AFT_Host_Cashout_ = "Requerimiento AFT Host-Cashout";
    Requerimiento_AFT_Solicitud_Registro_ = "Requerimiento AFT Solicitud Registro";
    Requerimiento_AFT_Interrogacion_Registro_ = "Requerimiento AFT Interrogacion Registro";
    Requerimiento_AFT_Cancelar_Registro_ = "Requerimiento AFT Cancelar Registro";
    Evento_No_Identificado_="Evento No Identificado";
    Juego_Seleccionado_="Juego Seleccionado";
    Ingreso_Pantalla_Contadores_="Ingreso a pantalla de contadores o menu asistente";
    Salida_Pantalla_Contadores_="Salida de pantalla de contadores o menu de asistente";
    Caja_Dinero_removida_="Caja del dinero removida";
    Caja_dinero_Instalada_="Caja del dinero instalada";
    Validador_billetes_Reset_="Validador de billetes Totales han sido reseteados";
    CMOS_Ram_Error_Data_Recovered_="Datos de error de RAM CMOS recuperados";
    CMOS_Ram_Error_No_Data_Recovered_="Error de RAM CMOS sin datos recuperados";
    CMOS_Ram_Error_Bad_device_="CMOS RAM error mal dispositivo";
    Billete_falso_="Ingreso de billete falso";
    EEPROM_Error_data_="Datos de error de EEPROM";
    EEPROM_Error_Bad_device_="Error de EEPROM dispositivo malo";
    EEPROM_Error_Diferent_checksum_vchanged_="Error de EEPROM version de checksum cambiada";
    EEPROM_Error_Bad_checksum_compare_="Error de EEPROM checksum incorrecta";
    Partitioned_EEPROM_Error_Bad_checksum_v_="Error de EEPROM particionado de versión de checksum incorrecta";
    Partitioned_EEPROM_Error_Bad_checksum_c_="Comparación de checksum incorrecta Error de EEPROM particionado";
    Memory_Error_Reset_="Error reinicio de memoria";
}
