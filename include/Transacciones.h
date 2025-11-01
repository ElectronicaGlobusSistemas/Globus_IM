#include <Arduino.h>


#define TIMEOUT_ACK_TRANSACCION  20000
#define TIMEOUT_EVENT_TRANSACCION 10000
#define TIMEOUT_RESULT_TRANSACCION 10000


#define TIMEOUT_RESULT_TRANSACCION_BA 10000



class TransaccionCashless
{
private:
  bool Evento_Controller_Machine=false;
  bool Evento_Controller_Machine_Download=false;
  bool Evento_Controller_Machine_BA=false;
  unsigned long Start_Transfer_Timestamp_Load=0;
  unsigned long Start_Transfer_Timestamp_Download=0;
  //unsigned long TIMEOUT_TASK=10000;
 

  
  


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
  EstadoBA Status;  // <-- variable que guarda el estado actual

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
 

  bool STATUS_TRANSFER(int Status)
  {
    if (Status < TRANS_IDLE || Status > TRANS_FINALIZADA)
    {
      return false; // Estado inválido
    }
    estado = static_cast<EstadoTransaccion>(Status);

    // switch (estado)
    // {
    // case TRANS_IDLE:
    //   printf("🔵 Estado cambiado a: IDLE (En espera)\n");
    //   break;
    // case TRANS_RECIBIDA:
    //   printf("📩 Estado cambiado a: RECIBIDA\n");
    //   break;
    // case TRANS_EN_PROGRESO:
    //   printf("⏳ Estado cambiado a: EN PROGRESO\n");
    //   break;
    // case TRANS_PENDIENTE:
    //   printf("🟡 Estado cambiado a: PENDIENTE\n");
    //   break;
    // case  TRANS_FINALIZADA:
    //   printf("🏁 Estado cambiado a: FINALIZADA\n");
    //   break;

    // case TRANS_TIMEOUT:
    //   printf("⏰ Estado cambiado a: TIMEOUT EXPIRADO\n");
    //   break;
    // default:
    //   printf("❓ ESTADO DESCONOCIDO\n");
    //   break;
    // }

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
      printf("🔵 Estado cambiado a: TRASANSACCION BA IDLE (En espera)\n");
      break;
    case TRANS_RECIBIDA:
      printf("📩 Estado cambiado a: TRANSACCION BA RECIBIDA\n");
      break;
    case TRANS_EN_PROGRESO:
      printf("⏳ Estado cambiado a: TRANSACCION BA EN PROGRESO\n");
      break;
    case TRANS_PENDIENTE:
      printf("🟡 Estado cambiado a: TRANSACCION BA PENDIENTE\n");
      break;
    case  TRANS_FINALIZADA:
      printf("🏁 Estado cambiado a: TRANSACCION BA FINALIZADA\n");
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

  EstadoTransaccion GET_STATUS_TRANSFER() const {
      return estado;
  }


   EstadoBA GET_STATUS_BA() const {
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