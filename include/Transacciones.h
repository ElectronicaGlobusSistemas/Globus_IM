#include <Arduino.h>


#define TIMEOUT_ACK_TRANSACCION  20000
#define TIMEOUT_EVENT_TRANSACCION 10000
#define TIMEOUT_RESULT_TRANSACCION 10000

/*5 Minutos*/
#define TIMEOUT_RESULT_TRANSACCION_BA 300000

#define BA_OK 0x00
#define BA_PENDING 0x40
#define BA_ERROR  0xFF
#define BA_NO_INICIADO  0xAA
#define DWN_OK 0x00
#define DWN_PENDING 0x40
#define ERROR_DWN 0xFF


#define Tipo_UP 0x00
#define Tipo_DWN 0x01



class TransaccionCashless
{
private:
  bool Evento_Controller_Machine=false;
  bool Evento_Controller_Machine_Download=false;
  bool Evento_Controller_Machine_BA=false;
  unsigned long Start_Transfer_Timestamp_Load=0;
  unsigned long Start_Transfer_Timestamp_Download=0;
  //unsigned long TIMEOUT_TASK=10000;
  
  unsigned long TimeoutExc=0;
  unsigned long TimeoutExc_Final=0;
  int IntervExc=10000;

  int IdBA;
  String Guid;
  
  volatile bool SolicitudTokenBA=false;

public:

  enum EstadoTransaccion {
    TRANS_IDLE = 0,          // En espera: sin transacción activa
    TRANS_RECIBIDA = 1,      // Solicitud de carga recibida
    TRANS_EN_PROGRESO = 2,   // Se está procesando la transacción
    TRANS_PENDIENTE = 3,    // Pendiente de consultada estado
    TRANS_TIMEOUT= 4,       // TIMEOUT de consulta espirado 
    TRANS_FINALIZADA = 5    // Finalizada

  };
  EstadoTransaccion estado;  // <-- variable que guarda el estado actual



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
    char assets[9];
    int count;
    int ID;
    String GUID;
    bool pendiente;
  } SolicitudBA;


  SolicitudBA solicitudBA;
  EstadoBA Status=BA_IDLE;  // <-- variable que guarda el estado actual

  bool Set_Evento_Controller_Machine_Load(bool Enable);
  bool Set_Evento_Controller_Machine_Download(bool Enable);

  bool Await_Transfer_Command_Load(unsigned long Timeout);
  void Init_Transfer_Parameter_Load();
  void Envia_Transaccion_AFT_Load();
  bool Await_Transaccion_AFT_Load(unsigned long timeout);
  void Estatus_Transaccion_AFT_Load();
  void Envia_Comando_AFT_Stutus();
  void Error_Transaccion_AFT_Load(int Code);
  void Task_Transaccion_AFT_Load(bool Transfer_In_Progress);
  bool Get_Evento_Controller_Machine_Load();
  bool Get_Evento_Controller_Machine_Download();

  void Task_Procesa_BA(unsigned long Timeout=5000);

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

  void Task_Transaccion_AFT_Download(bool Transfer_In_Progress);

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
      printf("🔵 Estado cambiado a: IDLE (En espera)\n");
      break;
    case TRANS_RECIBIDA:
      printf("📩 Estado cambiado a: RECIBIDA\n");
      break;
    case TRANS_EN_PROGRESO:
      printf("⏳ Estado cambiado a: EN PROGRESO\n");
      break;
    case TRANS_PENDIENTE:
      printf("🟡 Estado cambiado a: PENDIENTE\n");
      break;
    case  TRANS_FINALIZADA:
      printf("🏁 Estado cambiado a: FINALIZADA\n");
      break;

    case TRANS_TIMEOUT:
      printf("⏰ Estado cambiado a: TIMEOUT EXPIRADO\n");
      break;
    default:
      printf("❓ ESTADO DESCONOCIDO\n");
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
      printf("📩 Estado cambiado a: TRANSACCION BA RECIBIDA\n");
      break;
    case TRANS_EN_PROGRESO:
      printf("⏳ Estado cambiado a: TRANSACCION BA EN PROGRESO\n");
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

  void Tracking_BA(int Id,String GuidString);
  void Task_Controller_BA(bool InProgress);
  bool Validar_Estado_AFT_OK(void);
  bool Estatus_Transaccion_AFT_Download_AFTER_BA(void);
  bool Await_ACK_Transaccion_BA(unsigned long timeout);
  void Init_Transaccion_Parameter_BA(void);
  void Envia_Transaccion_BA();
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


