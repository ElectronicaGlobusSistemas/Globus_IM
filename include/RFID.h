/**
 * @brief Globus Sistemas SAS
 *  Archivo contiene funciones Control RFID
 */

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