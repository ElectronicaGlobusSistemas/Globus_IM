
#include "ESP32Time.h"
#include <string>
/**
 * @brief Globus Sistemas SAS
 *  Archivo contiene funciones Control RFID
 */

#ifndef RFID_H
#define RFID_H

#define Reset_Exitoso 0
#define ERROR_RESET_HANDPAY 1
#define SESION_INICIADA     2
#define ERROR_LECTURA       3
#define SESION_TERMINADA    4
#define INICIO_MODULO       5
#define TIMEOUT_CONTEO      6
#define SESION_CERRADA      7
#define TARJETA_OPERADOR_INSERT 8
#define MODULO_OK               9
#define MODULO_KO               10
#define CONEXION_TO_HOTS_FAILED 11
#define LECTURA_OK              12
#define CLIENTE_NO_BD           13
#define NO_HAY_COMUNICACION     14
#define TIMEOUT_SESION_ALERT    15
#define PLAYER_TRACKING_CASHLESS 1
#define PLAYER_TRACKING_DEFAULT  2
#define WIFI_DISCONECTED         17
#define GMASTER_CONFIRMA_RESET  16
#define UPDATING_SYS            18
#define NOT_AP_MODE             19
#define CONFIG_EXITOSA          20

#define CARGA_CASHLESS_EXITOSA  37


#define PLAYER_TRACKING_SESION  21
#define PLAYER_CASHLESS_SESION  22
#define SESION_DEFAULT          36 /* No Change Player Cashless*/


#define LOAD_TRANSACTION               "C"
#define DOWNLOAD_TRANSACTION           "D"
#define INSUFFICIENT_BALANCE          (0x23) /* Valores C,R,NR en 0 */
#define TRANS_ID_NO_MACTH             (0x24) /* Transaccion ID de solicitud es diferente a la retornada por el Servidor*/
#define CLIENT_NOT_EXITS_DATA_BASE    (0x25) 
#define CLIENT_NOT_MACTH              (0x26) /* Id de cliente  retornado por el servidor diferente al de la solicitud */
#define INVALID_BALANCE               (0x27) /* alguno  o todos los parametros de  C,R,NR, no son enteros */
#define TYPE_TRANS_NOT_MACTH          (0x28) /* Tipo de transaccion de servidor es diferente a solicitud */
#define REQUEST_SUCCESSFULLY_RECEIVED (0x29) /* Este no es necesario porque  es un codigo correcto y ya el ACK enviaria la confirmacion */
#define PROBLEM_WITH_THE_SERVER       (0x30) /* Me retorno un "IsSuccess" falso */
#define NOT_CONEXION_WITH_SERVER      (0x31)
#define INCOMPLETE_PARAMETERS_FOR_THE_REQUEST (0x32) /*Falta alguna Key para la solicitud */
#define GAME_MACHINE_NOT_REGISTER     (0x33)
#define NOT_COMMUNICATION_WITH_THE_MACHINE    (0x34) /* No hay comunicacion con la maquina */
#define NOT_WIFI_CONNECTION           (0x35)
#define TYPE_MACHINE_NOT_MACTH        (0x36)

/*----------------------------- Funciones Utilidades <-----------------------------------------*/
void Init_RFID(void);
void Lee_Tarjeta(void);
void Player_Tracking_(void);
void Sesion_Abierta_Color(int Figura);
void Clear_Barra(void);
void RESET_Handle(void);
void Cliente_VS_Operador(byte MEMORIA[],byte INFO[]);
void Status_Barra(int Status);
static void Read_RFID(void *parameter); /* Lee y verifica  RFID*/
void Reset_Handle_LED(void);
void Check_RFID(void);
bool Player_Tracking_Sesion(void);
void Player_Tracking_Timeout(void);
void Reset_Timeout_Player_Tracking(void);
int  Convert_Char_To_Int4(char buffer[]);
bool  Close_Sesion_Player_Tracking(void);
void Timer_Close_Player_Tracking(unsigned long Tiempo_Transcurrido, int Inactividad);


std::string IP_toString_(char IP_Char[]);


class Cashless_API
{

private:

public:

    int Info_Client(byte Id_Client[], ESP32Time ,String Type_Transaction=LOAD_TRANSACTION, uint32_t Transaction_ID=0);
    int Info_Client_Download(byte Id_Client[], ESP32Time RTC, String Type_Transaction, uint32_t Transaction_ID);
    bool Status_Transfer(int Code,char Buffer_Transfer[],ESP32Time);
    bool Status_Transfer_Download(int Code,char Buffer_Transfer[],ESP32Time RTC);
    void Reporting_Pending_Transfers(void);
    String Get_Current_Status_Transfer(void);
    String Get_Current_Status_Transfer_Download(void);
    void Load_Pending_Transactions(void);
    bool Inicialize_File_System(void);
    bool Init_Player_Tracking_Sesion(byte Id_Client[]);
    bool Close_Player_Tracking_Sesion(bool Enable);
    bool Reader_Lock(bool Status);
    int Type_Sesion(int Flag=SESION_DEFAULT,bool Status=false);

   
    
    bool Updated_Cashless_Counters(String Type_Transaccion);

    bool enviarTransaccion(const String &json);

    void Log(ESP32Time RTC,String Msg,String Data);
    void guardarTransacciones();
    void intentarEnviarTransacciones();
    void nuevaTransferencia(const String& json);


    int Recovery_Player_Sesion(byte Id_Client_Recovery[],int Type_Sesion,bool Handle_Cashless);
    void Saves_Current_Player_Sesion(byte Id_Client_Recovery[],int Type_Sesion);
    void Remove_Currrent_Player_Sesion(void);


    bool Ack_Transfer_Pending(int Code, char Buffer_Transfer[], ESP32Time RTC,String Type_Transaccion);
    String Get_Current_Pending_Ack_Load(void);
    String Get_Current_Pending_Ack_Download(void);

    bool Valida_Operador_Cashless(byte ID_Tarjeta_Operador[]);
};


#endif // RFID_H