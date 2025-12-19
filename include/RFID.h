
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

#define WIFI_CONEXION_FAILED    38 


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
#define COMANDO_NO_IDENTIFICADO       (0x59)



#define NOT_RED                       (0x37)
#define AUTHENTICATION_ERROR          (0x38)
#define READ_ERROR                    (0x39)
#define TARJETA_NO_IDENTIFICADA       (0x40)
#define CLOSE_PLAYER_TRACKING         (0x41)
#define READER_KO                     (0x42)
#define READER_OK                     (0x43)
#define RSET_IP                       (0x44)
#define TERMINA_SESION_CREDITOS       (0x45)
#define TERMINA_SESION_MANUAL         (0x46)
#define TERMINA_SESION_DESDE_SERVER   (0x47)
#define TERMINA_SESION_POR_TARJETA_OPERADOR   (0x48)

#define DESCARGA_POR_PERADOR          (0x50)
#define ERROR_NOT_IDENTIFY            (0x49)

#define TRANSFER_PENDING              (0x51)
#define MAQUINA_EN_JUEGO              (0x52)
#define INVALIDE_CLIENT               (0x53)

#define DESCARGA_USUARIO              (0x54)
#define DESCARGA_AUTOMATICO           (0x55)
#define DESCARGA_EFT_BLOQUEADA        (0x56)
#define FECHA_ELIMINA_LOG             (0x57)
#define TFT_NOT_INIT                  (0x58)
#define TRANSACCION_PENDIENTE_POR_CONSULTA (0x60)
 


#define INFO_  0
#define DEBUG_ 1
#define ERROR_ 2
#define WARN_  3
#define FATAL_ 4
/*----------------------------- Funciones Utilidades <-----------------------------------------*/
void Init_RFID(void);
void Lee_Tarjeta(void);
void Player_Tracking_(void);
void Sesion_Abierta_Color(int Figura);
void Clear_Barra(void);
void RESET_Handle(void);
void Cliente_VS_Operador(byte MEMORIA[],byte INFO[]);
//void Cliente_VS_Operador(int TIPO_TARJETA,uint32_t ID);
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
void Reset_Down_Mode(void);
void check_Status_Reader_Polling(void);

void Report_Http_Code(int Code_Http, String Msg="", bool Status=false);



std::string IP_toString_(char IP_Char[]);
String IP_toString_String(char IP_Char[]);

enum EstadoTransfer {
    TRANSFER_IDLE,
    TRANSFER_IN_PROGRESS,
    TRANSFER_DONE
};

enum Estado_Lector
{
    ESTADO_INACTIVO,
    ESTADO_SESION_ACTIVA,
    ESTADO_SIN_COMUNICACION,
    ESTADO_SIN_CONEXION_RFID,
    ESTADO_UPDATING,
    ESTADO_AP_MODE,
    ESTADO_LISTO_PARA_LEER
};

class Cashless_API
{

private:

    bool Enable_Event_Pending_Transfer_Download=false;
    bool Enable_Event_Pendeing_Transfer_Load=false;


    bool Enable_Transfer_Load=false;
    bool Enable_Transfer_Download=false;

    bool Handle_Lector_RFID=false;

    unsigned long TimeOut_RT=0;
    unsigned long TimeOut_RTF=0;

    String Token_Cashless="";
    String Hash_Cashless="";

    String Token_Cashless_BA="";
    String Hash_Cashless_BA="";

    uint64_t tiempoExpiracion_ms_BA=0;


    unsigned long TimeOut_Token_Inicial;
    unsigned long TimeOut_Token_Final;
    int TimeOut_Ejecuta=15000;
    bool Reset_Time=false;
    EstadoTransfer estado_transfer = TRANSFER_IDLE;  // Variable miembro

    unsigned long Timeout_Sesiones_Pendientes_Inicial=0;
    unsigned long Timeout_Sesiones_Pendientes_Final=0;
    int Timeout_Sesiones_Pendientes=30000;

    
public:

    int Info_Client(byte Id_Client[], ESP32Time ,String Type_Transaction=LOAD_TRANSACTION, uint32_t Transaction_ID=0);
    int Info_Client_Download(byte Id_Client[], ESP32Time RTC, String Type_Transaction, uint32_t Transaction_ID,int ClientID,int Operacion=DESCARGA_USUARIO);
    bool Status_Transfer(int Code,char Buffer_Transfer[],ESP32Time);
    bool Status_Transfer_Download(int Code,char Buffer_Transfer[],ESP32Time RTC,int Id_Client);
    void Reporting_Pending_Transfers(unsigned long TimeOut);
    String Get_Current_Status_Transfer(void);
    String Get_Current_Status_Transfer_Download(void);
    void Load_Pending_Transactions(void);
    bool Inicialize_File_System(void);
    bool Init_Player_Tracking_Sesion(byte Id_Client[]);
    bool Close_Player_Tracking_Sesion(bool Enable);
    bool Reader_Lock(bool Status);
    int Type_Sesion(int Flag=SESION_DEFAULT,bool Status=false);
    void Procesar_Lectura_Tarjeta(void);
    void Agragar_Transaccion(String Json);
    
    bool Updated_Cashless_Counters(String Type_Transaccion);
    bool Envia_Sesiones_Unknown(bool Opcion);
    bool enviarTransaccion(const String &json);


    
    bool Ack_Transfer_Pending_Pendiente(int Code, char Buffer_Transfer[], ESP32Time RTC,String Type_Transaccion,char Saldos[15]);
    bool Status_Transfer_Download_Pendiente_AFT(int Code, char Buffer_Transfer[], ESP32Time RTC,int Id_Client, char Saldos[15]);
    void Log(ESP32Time RTC,String Msg,String Data="",const char *Txt="/LogESP.txt",int Nivel_log=INFO_);
    void guardarTransacciones();
    void intentarEnviarTransacciones();
    void nuevaTransferencia(const String& json);
    bool Valida_Transmision(void);

    bool Estado_Juego_Maquina(int Evento);
    int Recovery_Player_Sesion(byte Id_Client_Recovery[],int Type_Sesion,bool Handle_Cashless);
    void Saves_Current_Player_Sesion(byte Id_Client_Recovery[],int Type_Sesion);
    void Remove_Currrent_Player_Sesion(void);

    void Count_Player_Sesions(bool Billete_In=false, bool Flag_Premio=false);
    
    bool Ack_Transfer_Pending(int Code, char Buffer_Transfer[], ESP32Time RTC,String Type_Transaccion);
    String Get_Current_Pending_Ack_Load(void);
    String Get_Current_Pending_Ack_Download(void);

    bool Valida_Operador_Cashless(char ID_Tarjeta_Operador[]);
    
    void Test_Txt(String Msg);
    /* Actualiza Objeto  Transfer Load and Download */
    bool Update_Ack_Evento_69(String Type_Transaccion,int Code=0x00);
    /* Habilita procesamiento de evento 68 (Transferencia completa) */
    bool Set_Transfer_Pending(bool Status,String Type_Transaccion,char Buffer_Transfer_Amount[]);
    bool Status_Transfer_Download_Pendiente(int Code, char Buffer_Transfer[], ESP32Time RTC,int Id_Client);
    bool Get_Status_Event_Pending_Transfer(String Type_Transaccion);


    bool Set_Controller_Transfer_Load(bool Enable);
    bool Get_Controller_Transfer_Load(void);
    bool Set_Controller_Transfer_Download(bool Enable);
    bool Get_Controller_Transfer_Download(void);


    bool Requerimiento_AFT_6A(int Evento, bool Enable);

    bool Init_Timer_Lector(void);
    bool Transaccion_Finalizada(void);

    bool  Get_Status_Reader(void);
    
    bool  Lock_Reader(void);
    bool  Unlock_Reader(void);


    void Solicitud_Token_Cashless(void);
    char*  Key_Generator(void);

    String Get_Token_Valido(void);
    String Get_Hash_Valido(void);
    bool  Set_Token_Valido(String Token_Valido,String Hash_Valido);

    void Genera_Token_Cashless(void);

    bool Set_Flag_Token(bool Status_Flag);
    void Token_Expiration(uint64_t Expiracion_Time);

    void Init_Timer_Transfer_Pending(uint64_t Tiempo_ms);
    void Break_Timer_Transfer_Pending(void);


    bool Await_Conexion(int ClienteID,char Type_Client,int Timeout=15000);

    void Set_Status_Transfer(EstadoTransfer nuevo_estado);
    EstadoTransfer Get_Status_Transfer() const;
    Cashless_API();
    bool Bloquea_Descarga_EFT(int Evento,int Tipo_Maq, int ClientID=0,bool Flag_Sesion=false);
    bool Get_Status_Handpay_EFT(void);
    bool CreaLog(const char *Archivo,bool Minutes_Days=false,uint32_t Min_Day=15);
    bool Verifica_Log(const char *Archivo, bool Minutes_Days=false,uint32_t Min_Day=15);
    bool Guarda_Tiempo_Log(bool Minutes_Days, uint32_t Min_Day);

    void guardarSesiones(void);
    bool haySesionesEnArchivo(void);
    void Nueva_Sesion(const String &json);
    void Intenta_Enviar_Sesiones_Task(void);
    String  Get_Info_Sesion_Unknown(bool Estado_Sesion,bool flag_contador);
    bool Envia_Sesiones_Unknown(const String &json);
    void Load_Sesiones_Unknown_Pendientes(void);
    void Task_Sesiones_Unknow(int timeout=5000);
    String FormatearBytesComoHex(char* data, size_t length);
    void Parametros_iniciales_Sesion(char buffer[]);
    void Test_Counter(char* Contador,long&valor_test);


    bool ExitsCahlessID(void);
    bool RemoveCashlessID();
    int GetCashlessID(int Parameter);
    bool SetCashlessID(String Type_Transaction="C");

    bool Solicitud_Token_Cashless_BA(unsigned long timeout=500);
    String Get_Token_Valido_BA(void);
    String Get_Hash_Valido_BA(void);
    bool Set_Token_Valido_BA(String Token_Valido,String Hash_Valido);

};
void New_Token(void*arg);
void Resurrect_reader(void);
void Break_Cashless_Pending(void*arg);

#endif // RFID_H