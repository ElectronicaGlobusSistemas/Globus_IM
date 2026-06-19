#include "Arduino.h"
#define SINCRO    10
#define REESTART_TFT          11
#define DESINCRO  12
#define PING                  3
#define CONFIG                13
#define DOWNLOAD              14
#define REMOVE_IMG_TFT        15
#define UPDATE_TFT            16

#define BANNER_TFT            17


#define UPDATE_OTA            30

#ifndef PANTALLA_TFT_H
#define PANTALLA_TFT_H

// #define  PLAYER_TRACKING_SESION 0
// #define  PLAYER_CASHLESS_SESION 1
// #define  PLAYER_UNKNOW_SESION   2

#define TFT_M5STACK_DIAL        1
#define TFT_48S3_43             0
#define TFT_UNKNOW              100

#define MAX_LIMITE_CONEXION_TFT  5

#define MAX_INTENTOS_CONEXION_TFT     100



#define BUSY_TFT    0
#define REQUEST_OK  1
#define ERROR_REQUEST 2



enum EventType
{
    EVENT_ACTUALIZAR_INFO_INICIAL_SESION_CASHLESS,
    EVENT_ACTUALIZAR_INFO_FINAL_SESION_CASHLESS,
    EVENT_ACTUALIZAR_INFO_INICIAL_SESION_FIDELIZACION,
    EVENT_ACTUALIZAR_INFO_FINAL_SESION_FIDELIZACION,
    EVENT_MENSAJES,
    EVENT_ACTUALIZA_PUNTOS,
    EVENT_RECUPERA_SESION,


    EVENT_ON_START_OTA,
    EVENT_ON_PROGRESS,
    EVENT_ON_END,
    EVENT_ON_ERROR,

    CALL_OPERATOR_EVENT,

    EVENT_RESET_ESPNOW,
    EVENT_CONEXION_WIFI_ESTAB,
    EVENT_RECOVERY_SD,
    EVENT_LOGIN_TFT

};

struct ButtonEvent
{
    EventType type;
};


enum EstadoSistema
{
    ESTADO_OK = 0,
    ERROR_WIFI,
    ERROR_MAQUINA,
    ERROR_SERVIDOR,
    ERROR_LECTOR,
    LECTOR_BLOQUEADO,
    FTP_MODE
};


enum LoginTFT
{
    STATE_LOGIN_IDLE = 0,
    STATE_LOGIN_IN_PROGRESS,
    STATE_LOGIN_OK,
    STATE_LOGIN_FAIL,
    STATE_BLOCKED
};


class Pantalla_TFT
{
private:
    String Nombre_Cliente = "";
    float DenoCashless=100;



    unsigned long TimeoutTaskTFT=0;
    unsigned long TimeoutTaskTFTF=0;
    int IntvTask =500;

    unsigned long Timeout_Reconexion_TFT=0;
    unsigned long Current_Intento_Conexion_TFT=0;

    volatile unsigned long Intentos_Conexion=0;


public:

    struct DatosUsuario
    {
        String Usuario; // Cliente_Nombre
        String Casino;  // Casino

        float DenoCashless; // Deno_Cashless

        float Total_Fide;       // Total_Fide
        float Total_Bole;       // Total_Bole
        String Nivel_Usuario; // Nivel_Usuario

        float Actual_Fide; // Actual_Fide
        float Actual_Bole; // Actual_Bole

        uint32_t Saldo_Canjeable=0;
        uint32_t Saldo_Restringido=0;
        uint32_t Saldo_No_Restrindigo=0;
        
        bool  Actualiza_Saldos_Iniciales=false;
        bool  Actualiza_Saldos_Finales=false;
        bool Actualiza_Puntos_Iniciales=false;
        bool Actualiza_Puntos_Finales=false;

        String Mensaje="";
        bool Ocultar=true;

        bool Sesion_Server=false;
        String Version_Firmware_tft="";

        int Progress=0;

        //bool  volatile OTARun=false;

        int Evento;
        String Message;

        String Id_Usuario_TFT="";
        String Contrasena_TFT="";
        int Id_TFT_Temporal=0;


        String Usuario_Espnow="";
        String Contrasena_Espnow="";
        
    };

    DatosUsuario info;

    struct ConfigTFT
    {
        uint32_t timeoutSaldosCONFIG = 3000;
        uint32_t timeoutImagenesCONFIG = 30000;
        uint32_t timeoutCarrucel_MensajesCONFIG = 10000;
        uint32_t timeoutMensajesCONFIG = 1500;
        uint32_t timeoutMensajesCliente=3000;

        unsigned long Timeout_Conexion;

        int Contador_Conexion=0;

    };

    ConfigTFT configtft;

    void Set_Saldos_TFT(uint32_t SaldoCanjeable, uint32_t SaldoRestringido, uint32_t SaldoNoRestringido);

    void Set_DenoCashless(float deno)
    {
        DenoCashless = deno;
    }

    float Get_DenoCashless(void)
    {
        return DenoCashless;
    }

    bool Set_Nombre_Cliente(String Name)
    {
        Nombre_Cliente = Name;

        if (Nombre_Cliente == Name)
            return true;
        else
            return false;
    }
    String Get_Nombre_Cliente(void)
    {
        return Nombre_Cliente;
    }

    bool Reset_Nombre_Cliente(void)
    {
        Nombre_Cliente="";

        if(Nombre_Cliente=="")
            return true;
            
        return false; 
    }


    bool Get_Info_Cliente(int Cliente);

    void Notify_Status_Machine_Now();
    EstadoSistema ObtenerEstadoSistema(void);

    bool Progreso(int Progress);
    bool Actualiza_Saldos_TFT_Globus_IM(String Casino, uint32_t Saldo_Canjeable, uint32_t Saldo_Sin_Restriccion, uint32_t Saldo_No_Canjeable, bool Sesion=false);

    bool Actualiza_Puntos_TFT_Globus_IM(String User_Name,  String Casino,int User_Level, float Total_Playertracking_Points, float Total_Points_Tickets, float Current_Playertracking_Points=0.0, float Current_Points_Tickets=0.0);

    bool Cierra_Sesion_Player_Cashless_TFT_Globus_IM(String Casino, uint32_t Saldo_Canjeable, uint32_t Saldo_Sin_Restriccion, uint32_t Saldo_No_Canjeable);

    bool Cierra_Sesion_Player_Tracking_TFT_Globus_IM(void);
    bool Home_TFT_Globus_IM(void);
    int NivelUsuario(String Nivel);
    void Task_Banner_TFT(unsigned long Timeout);

    void Task_Handle_TFT_Display(int Timeout=500);
    bool Actualiza_Menu_TFT_Globus_IM(int Option=25);


    bool Reconexion_TFT_(unsigned long Timeout=3000);

    void Init_Handle();
    void Notify_Now(EventType Notify);
    void handleTFT(void);

    bool Verifica_Conexion_ESPNow(unsigned long Timeout=10000);
    bool Reset_Pantalla_TFT(void);
    void Mensaje_TFT(String mensaje,bool Ocultar=true);
    bool Evento_Soporte(int Evento, String Message);
    bool Captura_Puntos_Sesion();

    int Recibe_Info_Login(String Usuario, String Contrasena, bool IsSuccess);

    bool Login_TFT(const String& Usuario,
                             const String& Contrasena,
                             const String& URL);

    bool SendLoginResponse(uint8_t *addr,
                       bool success,
                       int code,int HardCode);
};

bool Init_TFT_Display(bool EspNow=false, uint8_t MAC[6] = nullptr);
bool Stop_TFT_Display(void);
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len);
// bool Init_Player_TFT(String User_Name ="", float Total_Playertracking_Points=0, float Total_Points_Tickets=0, int User_Level=0,uint32_t Saldo_Canjeable=0, uint32_t Saldo_Sin_Restriccion=0,uint32_t Saldo_No_Canjeable=0,float Current_Playertracking_Points=0,float Current_Points_Tickets=0,int Tipo_Sesion=PLAYER_CASHLESS_SESION,String Casino="");

bool Await_ms(bool (*condicion)(), unsigned long timeout_ms);

void Prueba_TFT(void);
bool Send_TFT(uint8_t MAC[], uint8_t *Data, int len,int MaxIntentos=5);
// bool Close_Player_TFT(uint32_t Saldo_Canjeable=0, uint32_t Saldo_Sin_Restriccion=0, uint32_t Saldo_No_Canjeable=0, int Tipo_Sesion=PLAYER_TRACKING_SESION);
bool Get_Status_Sesion_Player(void);
bool Menssage_TFT(String Message,int timeout=1500,bool IsSuccess=true);
bool Config_Parameter_TFT(uint32_t timeoutSaldos=3000,uint32_t timeoutImagenes=30000,uint32_t timeoutCarrucel_Mensajes=10000,uint32_t timeoutMensajes=2500);

bool get_Flag_Sincro_TFT();
bool get_Flag_Config_TFT(void);
bool get_Flag_Descarga_TFT(void);
bool get_Flag_Borrar_TFT(void);
bool get_Flag_Update_TFT(void);
int get_Tipo_TFT(void);
void Task_Conexion_TFT(int Timeout);
void Init_Server(const char *ssid,const char *password);
bool Reset_TFT(void);
void RumUpdateTFT();
void Check_TFT_Reconnect(unsigned long Timeout=10000);
bool EnviarBannersPorEspNow();
bool ConsultarBanners();
#endif // PANTALLA_TFT_H


