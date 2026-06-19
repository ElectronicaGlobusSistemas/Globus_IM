#include <Arduino.h>


#define TIMEOUT_ACK_TRANSACCION  20000
#define TIMEOUT_EVENT_TRANSACCION 10000
#define TIMEOUT_RESULT_TRANSACCION 10000
#define MAX_USAGE_PERCENT 40
/*5 Minutos*/
#define TIMEOUT_RESULT_TRANSACCION_BA 100000
#define TIMEOUT_RESULT_ 100000

#define BA_OK 0x00
#define BA_PENDING 0x40
#define BA_ERROR  0xFF
#define BA_NO_INICIADO  0xAA
#define DWN_OK 0x00
#define DWN_PENDING 0x40
#define ERROR_DWN 0xFF


#define Tipo_UP 0x00
#define Tipo_DWN 0x01

enum EstadoAFTMAQUINA
  {
    AFT_OK = 0,

    ERROR_DENO_CASHLESS_CERO,
    ERROR_LIMITE_TRANSACCION,
    ERROR_EVENTO_MAQUINA,
    ERROR_MAQUINA_EN_JUEGO,

    ERROR_GAMELOCK,
    ERROR_HOST_CASHOUT,

    ERROR_TRANSFER_FROM_DISABLED,
    ERROR_TRANSFER_TO_DISABLED,
    ERROR_BONUS_AWARD_DISABLED,

    ERROR_AFT_NO_REGISTRADO,
    ERROR_INHOUSE_DISABLED,
    ERROR_BONUS_TRANSFER_DISABLED,
    ERROR_DEBIT_TRANSFER_DISABLED,
    ERROR_AFT_GENERAL_DISABLED,

    ERROR_COMUNICACION_MAQ,
    ERROR_WIFI_DESCONECTADO,
    ERROR_FTP_MODE,
    ERROR_BOOTLOADER_MODE,
    
  };

class TransaccionCashless
{
private:
  bool Evento_Controller_Machine=false;
  bool Evento_Controller_Machine_Download=false;
  bool Evento_Controller_Machine_BA=false;
  unsigned long Start_Transfer_Timestamp_Load=0;
  unsigned long Start_Transfer_Timestamp_Download=0;
  //unsigned long TIMEOUT_TASK=10000;
  

  bool Evento_Controller_Machine_BUDA_In=false;
  bool Evento_Controller_Machine_BUDA_Out=false;

  unsigned long TimeoutExc=0;
  unsigned long TimeoutExc_Final=0;
  int IntervExc=20500;

  int IdBA;
  String Guid;
  
  volatile bool SolicitudTokenBA=false;

public:

  enum EstadoTransaccion {
    TRANS_IDLE = 0,          // En espera: sin transacción activa
    TRANS_RECIBIDA = 1,      // Solicitud de carga recibida
    TRANS_EN_PROGRESO = 2,   // Se está procesando la transacción
    TRANS_PENDIENTE = 3,    // Pendiente de consultada estado
    TRANS_TIMEOUT= 4,       // TIMEOUT de consulta expirado 
    TRANS_FINALIZADA = 5    // Finalizada

  };
 volatile EstadoTransaccion estado;  // <-- variable que guarda el estado actual



  enum EstadoBA {
    BA_IDLE = 0,          // En espera: sin transacción activa
    BA_RECIBIDA = 1,      // Solicitud de carga recibida
    BA_EN_PROGRESO = 2,   // Se está procesando la transacción
    BA_PENDIENTE = 3,    // Pendiente de consultada estado
    BA_TIMEOUT= 4,       // TIMEOUT de consulta espirado 
    BA_FINALIZADA = 5    // Finalizada
    


  };

  

  typedef struct 
  {
    char TransaccionID_TX[8];
    char TransaccionID_RX[8];
    char Size_TransaccionID_TX;
    char Size_TransaccionID_RX;
    uint8_t Transfer_type=0xFF;
  } Informacion_Transaccion;
  
  
  Informacion_Transaccion Info_Current_Transfer;
  

  typedef struct
  {
    char assets[9];
    int count;
    int ID;
    String GUID;

    float Deno_Contabilidad_BUDA;
    float Deno_Cashless_BUDA;

    char Amount[4];

    bool pendiente;

    int Coin_In_AFTER;
    int Coin_In_Before;
    int Credit_AFTER;
    int Credit_Before;

    bool Upgrade_Outputs=false;
    bool Ack_Upgrade_Outputs=false;
    bool Ack_Pendiente_Recv=false;
    bool Flag_Attend_Transaccion_BA=false;

  } SolicitudBA;


  SolicitudBA solicitudBA;
  EstadoBA Status=BA_IDLE;  // <-- variable que guarda el estado actual

  bool Set_Evento_Controller_Machine_Load(bool Enable);
  bool Set_Evento_Controller_Machine_Download(bool Enable);

  bool Set_Evento_Controller_Machine_Load_Especial(bool Enable);
  bool Set_Evento_Controller_Machine_Download_Especial(bool Enable);


  bool Await_Transfer_Command_Load(unsigned long Timeout);
  void Init_Transfer_Parameter_Load();
  void Envia_Transaccion_AFT_Load();
  bool Await_Transaccion_AFT_Load(unsigned long timeout);
  void Estatus_Transaccion_AFT_Load();
  void Envia_Comando_AFT_Stutus();
  void Error_Transaccion_AFT_Load(int Code);
  void Task_Transaccion_AFT_Load(bool Transfer_In_Progress);

void Task_Transaccion_AFT_Load_Especial(bool Transfer_In_Progress);
void Task_Transaccion_AFT_Download_Especial(bool Transfer_In_Progress);

  bool Get_Evento_Controller_Machine_Load();
  bool Get_Evento_Controller_Machine_Download();

  bool Get_Evento_Controller_Machine_Load_Especial();
  bool Get_Evento_Controller_Machine_Download_Especial();

  void Task_Procesa_BA(unsigned long Timeout=15000);

  bool Upgrade_Outpus_Machine(unsigned long Timeout=5000);

  void checkAndCleanLogSAS(void);

  bool Set_ProcesarSolicitudBA(bool Enable)
  {
    SolicitudTokenBA = Enable;

    if (SolicitudTokenBA == Enable)
      return true;
    else
      return false;
  }

  bool Get_ProcesarSolicitudBA()
  {
    return SolicitudTokenBA;
  }

  void Backup_TransaccionBA(bool STATUS);
  void Backup_Transaccion_Output(bool STATUS);

  void Save_Transaccion(bool);
  bool Await_Event_AFT(unsigned long timeout);
  bool Save_Client_Transfer_Critical();
  bool Delete_Client_Transfer_Critical(void);
  void AFT_FUNDS_TRANSFER_STATUS();
  bool Get_Response_AFT(unsigned long timeout);
  bool Get_Client_Transfer_Critical();

  void Save_Status_Descarga(bool Status);

  void Init_Transfer_Parameter_Download(void);
  bool Get_Creditos_AFT_Download(unsigned long timeout);
  void Envia_Transaccion_AFT_Download(void);
  bool Await_Transaccion_AFT_Download(unsigned long timeout);
  void Error_Transaccion_AFT_Download(int Code);
  void Estatus_Transaccion_AFT_Download(void);


  void Estatus_Transaccion_AFT_Download_Especial(void);

  void Task_Transaccion_AFT_Download(bool Transfer_In_Progress);


  void Estatus_Transaccion_AFT_Especial(void);

  void Error_Transaccion_AFT_Load_Especial(int Code);


  void Error_Transaccion_AFT_Download_Especial(int Code);

  void BackupSaldos(char BufferAFT[128]);
  bool GetBackupSaldos(char saldos[15]);
  void ClearBackupSaldos();
  bool Exits_BackupSaldos(void);
  bool Await_ACK_BA(unsigned long timeout);
  // Constructor inicializa en IDLE
  TransaccionCashless() : estado(TRANS_IDLE) {};
  
  String GetStatusBA();
  bool STATUS_TRANSFER(int Status)
  {
    if (Status < TRANS_IDLE || Status > TRANS_FINALIZADA)
    {
      return false; // Estado inválido
    }
    estado = static_cast<EstadoTransaccion>(Status);

    switch (estado)
    {
    case TRANS_IDLE:
      // printf("🔵 Estado cambiado a: IDLE (En espera)\n");
      break;
    case TRANS_RECIBIDA:
      // printf("📩 Estado cambiado a: RECIBIDA\n");
      break;
    case TRANS_EN_PROGRESO:
      // printf("⏳ Estado cambiado a: EN PROGRESO\n");
      break;
    case TRANS_PENDIENTE:
      // printf("🟡 Estado cambiado a: PENDIENTE\n");
      break;
    case  TRANS_FINALIZADA:
      // printf("🏁 Estado cambiado a: FINALIZADA\n");
      break;

    case TRANS_TIMEOUT:
      // printf("⏰ Estado cambiado a: TIMEOUT EXPIRADO\n");
      break;
    default:
      // printf("❓ ESTADO DESCONOCIDO\n");
      break;
    }

    return true;
  }

  bool Set_Controller_Transaccion_BA(int NewStatus)
  {
    Evento_Controller_Machine_BA=NewStatus;

    if(Evento_Controller_Machine_BA==NewStatus)
      return true;
    else
      return false;
  }

  bool Get_Controller_Transaccion_BA()
  {
    return Evento_Controller_Machine_BA;
  }

  bool STATUS_BA(int Estado)
  {
    if (Estado < TRANS_IDLE || Estado > TRANS_FINALIZADA)
    {
      return false; // Estado inválido
    }
    Status = static_cast<EstadoBA>(Estado);

    switch (Status)
    {
    case TRANS_IDLE:
      //printf("🔵 Estado cambiado a: TRASANSACCION BA IDLE (En espera)\n");
      break;
    case TRANS_RECIBIDA:
      //printf("📩 Estado cambiado a: TRANSACCION BA RECIBIDA\n");
      break;
    case TRANS_EN_PROGRESO:
      //printf("⏳ Estado cambiado a: TRANSACCION BA EN PROGRESO\n");
      break;
    case TRANS_PENDIENTE:
      //printf("🟡 Estado cambiado a: TRANSACCION BA PENDIENTE\n");
      break;
    case  TRANS_FINALIZADA:
      //printf("🏁 Estado cambiado a: TRANSACCION BA FINALIZADA\n");
      break;

    case TRANS_TIMEOUT:
      //printf("⏰ Estado cambiado a: TIMEOUT EXPIRADO\n");
      break;
    default:
      //printf("❓ ESTADO DESCONOCIDO\n");
      break;
    }

    return true;
  }

  EstadoTransaccion GET_STATUS_TRANSFER() const {
      return estado;
  }

  EstadoBA GET_STATUS_BA() const
  {
    return Status;
  }

  const char *GET_STATUS_TRANSFER_Str() const
  {
    switch (estado)
    {
    case TRANS_IDLE:
      return "IDLE (En espera)";
    case TRANS_RECIBIDA:
      return "RECIBIDA";
    case TRANS_EN_PROGRESO:
      return "EN PROGRESO";
    case TRANS_FINALIZADA:
      return "FINALIZADA";

    case TRANS_TIMEOUT:
      return "TIMEOUT EXPIRADO";
    default:
      return "DESCONOCIDO";
    }
  }

  const char *GET_STATUS_BA_Str() const
  {
    switch (Status)
    {
    case TRANS_IDLE:
      return "IDLE (En espera)";
    case TRANS_RECIBIDA:
      return "RECIBIDA";
    case TRANS_EN_PROGRESO:
      return "EN PROGRESO";
    case TRANS_FINALIZADA:
      return "FINALIZADA";

    case TRANS_TIMEOUT:
      return "TIMEOUT EXPIRADO";
    default:
      return "DESCONOCIDO";
    }
  }


  int Procesa_Sesion_Duplicada(String Identificador);


  int Get_IdBA(void)
  {
    return IdBA;
  }

  String Get_Guid(void)
  {
    return Guid;
  }

  void UPDOWNBA(int Code, String Msg, char Buffer[],ESP32Time RTC,int TYPE);

  void BackupBA();


  bool Descarga_OK(unsigned long Timeout);

  bool Informacion_Maq_Before_Transfer(bool IsSuccess,bool Select);
  bool Informacion_Maq_After_Transfer(bool IsSuccess,bool Select);
  void Print_Snapshot(bool Select);

  bool Refresh_Maquina_Completa(unsigned long Timeout);


  bool Tracking_Dowmload_Transaccions(unsigned long Timeout);
  void Init_Refresh_Machine();

  bool Refresh_Cashless_Machine(bool Type_Transfer);

  int Validacion_Transaccion(int Type_Transaccion=0x01);
  bool Tracking_Contability_After_Tranfer(unsigned long Timeout, bool Type_Transfer);
  bool Tracking_Contability_Before_Tranfer(unsigned long Timeout, bool Type_Transfer);
  bool IsCashlessMachine();
  void Recv_Ack_Load(void);
  void LogSAS(const char *direccion, const char *evento, uint8_t *buffer, size_t length,bool Debug=false);
  bool checkLogSAS(void);
  void Tracking_BA(int Id,String GuidString);
  void Task_Controller_BA(bool InProgress);
  int Validar_Estado_AFT_OK(void);
  bool Estatus_Transaccion_AFT_Download_AFTER_BA(void);
  bool Await_ACK_Transaccion_BA(unsigned long timeout);
  void Init_Transaccion_Parameter_BA(void);
  void Envia_Transaccion_BA();


  bool Security_Key_Validation(unsigned long timeout);

  void Mantiene_Comunicacion(void);
  bool IsReadyForNewTransfer(void)
  {

    switch (estado)
    {
    case TRANS_IDLE:
      return true;
      break;

    default:
      return false;
      break;
    }
    return false;
  }
};


