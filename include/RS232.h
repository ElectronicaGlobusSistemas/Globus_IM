#include "driver/uart.h"
#include <string.h>
#include <iostream>
#include <stdio.h>
#include "Memory_SD.h"
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include "Utilidades_Maquina.h"
#include "CRC_Kermit.h"
#include "RFID.h"
#include <esp_task_wdt.h>
#include "API_Accounting.h"
#include "ArduinoJson.h"
#include "Persistenca_Info.h"

bool App=false;
extern bool Update_In_Cashless;
extern bool Update_Out_Cashless;
extern bool Update_In_Tito;
extern bool Update_Out_Tito;

extern Persistenca_Info Backup;

bool Handle_Maq_AFT_Plus=false;
bool Capture_Evento_Cashless=false;
bool Capture_Evento_Cashless_OK=false;
bool Not_Comunicacion_With_Machine=false;
bool Hadle_comunications=false;



extern bool Solicitud_Expiracion_Ticket;

extern DynamicJsonDocument Objeto_Ticket_In_Response;

bool Ack_Cashless_Transfer_Load=false;
bool Flag_Transfer_Ok=false;

bool Flag_Entradas_Cashless_OK=false;
bool Flag_Salidas_Cashless_OK=false;

bool Flag_Set_Ticket_Data=false;
bool Flag_Entradas_Tito_OK=false;
bool Flag_Salidas_Tito_OK=false;


bool Comando_Entradas_OK=false;
bool Comando_Salidas_OK=false;

bool Comando_Entradas_Tito_OK=false;
bool Comando_Salidas_Tito_OK=false;


bool Flag_Critial_Questions=false;

int Envio=0;
char Prueba_AFT[128];
char Data_TX_AFT[33];

char FF[63];
extern bool Condicion_Cumpl;
extern bool  condicionCumplida;
extern Cashless_API Info_Cashless;
extern API_Accounting Accounting;
extern bool hayTransaccionesPendientes;



bool Actualizacion_datos_Ok=false;
bool Credit_Handle=false;
/*-------------------> Cashless <------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------------*/

/*---------------------------------------->Debug<---------------------------------------------------------*/
//#define Debug_Contadores
//#define Debug_Encuestas
//#define Debug_Eventos
//#define Debug_ACK_MSG
//#define Debug_SD_CARD
//--------------------------------------> Define UART <-----------------------------------------------------
#define NUMERO_PORTA_SERIALE UART_NUM_2
#define BUF_SIZE (1024 * 2)
#define RD_BUF_SIZE (1024 * 2)
static QueueHandle_t uart2_queue;
/*------------------------------------> UART 1 <-----------------------------------------------------------*/
#define BUF_SIZE1 (1024 * 2)
#define RD_BUF_SIZE1 (1024 * 2)
static QueueHandle_t uart1_queue;
//----------------------------------------------------------------------------------------------------------
//--------------------------------------> TaskHandle_t <----------------------------------------------------

TaskHandle_t RecepcionRS232, Encuestas,RecepcionRS232_NUM1,Encuestas_NUM1;

//----------------------------------------------------------------------------------------------------------
//--------------------------------------> Define Maquina <--------------------------------------------------
#define SYNC 0x80
#define POLL 0x81
#define DIR 0x01
//-----------------------------------------------------------------------------------------------------------
#define U2RXD 16
#define U2TXD 17
#define U1TXD 32
#define U1RXD 33

uint8_t rxbuf[255];   // Buffer di ricezione
uint16_t rx_fifo_len; // Lunghezza dati
uint8_t UART2_data[1024];

char dat[1] = {SYNC};
char dat3[1] = {POLL};
char dat4[1] = {DIR};

int Conta_Poll = 0;
int Conta_Encuestas = 0;

extern char Archivo_CSV_Contadores[200];
extern char Archivo_CSV_Eventos[200];
extern char Archivo_LOG[200];
extern bool Archivos_Ready;
String SD_Cont;
String SD_EVEN;




int bandera = 0;
int contador = 0;
long numero_contador = 0;
int Contador_Encuestas = 0;
int numero_encuesta = 0;
int Max_Encuestas = 27;
bool Datos_OK=false;
bool Counter_Final=false;
bool Ultimo_Counter_=false;
bool Prueba=false;
String Encabezado_Contadores = "Hora,Total Cancel Credit,Coin In,Coin Out,Jackpot,Total Drop, Cancel Credit Hand Pay,Bill Amount, Casheable In, Casheable Restricted In, Casheable Non Restricted In, Casheable Out, Casheable Restricted Out,Casheable Nonrestricted Out,Games Played,Coin In Fisico,Coin Out Fisico,Total Coin Drop,Machine Paid Progresive Payout,Machine Paid External Bonus Payout,Attendant Paid Progresive Payout,Attendant Paid External Payout,Ticket In,Ticket Out,Current Credits,Contador 1C - Door Open Metter,Contador 18 - Games Since Last Power Up";
String Estructura_CSV[27] = {"n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a,", "n/a"};
//------------------------------------> Prototipo de Funciones <------------------------------------------------
void Transmite_Sincronizacion(void);
static void UART_ISR_ROUTINE(void *pvParameters);
bool Solicitud_Descarga_Cashless(void);
static void UART_ISR_ROUTINE_NUM1(void *pvParameters);
void Encuestas_Maquina_NUM1(void *pvParameters);
void Transmite_Download_AFT_Maq(void);
void Encuestas_Maquina(void *pvParameters);
//--------------Guarda Contadores SD
void Storage_Contadores_SD(char *ARCHIVO, String Encabezado1, bool Enable);
void Add_String(char *contador, String stringDT, bool Separador);
void Add_String_Hora(String Horario);
void Transmite_Load_AFT_Maq(void);

void Actualiza_Entradas_Cashless(void);
void Actualiza_Salidas_Cashless(void);
bool Actualiza_Cashless_Salidas(void);
bool Actualiza_Cashless_Entradas(void);

void Actualiza_Salidas(void);
//-------------------------------------
//--------------Guarda Eventos SD
void Store_Eventos_SD(char *ARCHIVO, bool Enable);
void Add_String_Hora_EVEN(String Horario);
void Add_String_EVEN(String EVENTO, bool Separador);
bool Solicitud_Carga_Cashless(void);
//---------------------------------------
void Encuestas_Maquinas_Poker_Ertech_Slot(void);
void Encuestas_Maquinas_Simple_No_Cancel(void);
void Encuestas_Maquinas_Genericas(void);
void Encuestas_Maquinas_Poker(void);
void Encuestas_Maquinas_IRT(void);
void Encuestas_Maquinas_EFT(void);
void Encuestas_Maquinas_IGT_Riel(void);
void Encuestas_Maquinas_IGT_Riel_Bill(void);
void Encuestas_Mecanicas(void);
void Encuestas_Maquinas_Simple(void);
void Encuestas_Maquinas_Ruleta_IRT(void);
void Encuesta_Billetes(void);
bool Inactiva_Maquina(void);
void Transmite_Inactiva_Maquina(void);
bool Activa_Maquina(void);
void Transmite_Activa_Maquina(void);
bool Verifica_Premio_1B(void);
void Encuesta_contador_1B(void);
void Selector_Modo_SD(void);
void Delete_Trama();
void Add_Contador(char *Contador_, int Filtro, bool Salto_Linea);
void Calcula_Cancel_Credit_IRT(void);
void Calcula_Bill_In_550(void);
void Escribe_Tarjeta_Mecanica(char *buf); /* 2 Contadores */
 void Escribe_Tarjeta_Mecanica_2(char buf[]); /* 4 contadores */
void Interroga_Info_Cashless(void);
void Transmite_Reset_Handpay(void);
bool Reset_HandPay(void);
void _Transmite_Encuesta_Creditos_D_Premio(void);
bool Encuesta_Creditos_Premio(void);
bool Transmite_Registro_AFT_Maq(void);
bool Transmite_AFT_Registro_Machine(void);
void Transmite_Cancela_Registro_MQ(void);
bool Transmite_Init_Registro(void);
void Encuestas_Aristocrat_Australiana(void);
void Transmite_Encuesta_Creditos(void);
void Transmite_Encuesta_Maquina_Juego(void);
void Transmite_Encuesta_ROM(void);
void Transmite_Carga_legacy_bonus_awards(void);
bool Filtro_Eventos_Mq(int Evento);
void Transmite_Consulta_Creditos_Cashless(void);
bool Consulta_Creditos_Cashless(void);
bool Carga_Bonus_Maquina(void);
bool Solicitud_Forzada_Carga_Cashless(void);
void Update_Status_Critical_Load(void);
bool Event_Transfer_Load_Pending(int Evento);
void Requerimiento_TITO(void);
void Remove_Ticket_In_Buffer(void);
void Requerimiento_Ticket_In(void);
void Set_Extended_Ticket_Data(void);
void Transmite_Carga_legacy_Bonus(void);
void Update_Status_Critical_Download(void);
bool Actualiza_Tito_Entradas(void);
bool Actualiza_Tito_Salidas(void);

void Actualiza_Salida_Tito(void);
void Actualiza_Entrada_Tito(void);

void Critial_Question(void);
void Tito_Expiration_Ticket(void);

void Init_Reg_Cashless(void);
void Genera_Registro_Maquina(void);
void Transmite_Load_EFT_Maq(void);
void Transmite_Dowmload_EFT_Maq(void);
void Transmite_Download_EFT_Maq(void);
unsigned char EFT_Maq_Cashable_Ack(void);
unsigned char EFT_Maq_Descarga_Ack(void);
void TIMEOUT_TRANSFER(bool Handle);

bool Flaggg=false;

extern unsigned long Bandera_RS232;
extern unsigned long Bandera_RS232_F;
//---------------------------------------------------------------------------------------------------------------
// MetodoCRC CRC_Maq;
// Contadores_SAS contadores;
// Eventos_SAS eventos;

bool ACK_Maq = false;
bool ACK_Premio = false;
bool Recibe_Mecanicas = false;
bool ACK_Mecanicas = false;

bool ACK_Maq_Cashless=false;
int Conta_Poll_Billetes = 0;

bool flag_ultimo_contador_Ok = false;
int conta_poll_comunicacion_maquina = 0;
bool flag_handle_maquina = false;

bool flag_handle_maquina_Cashless = false;


bool flag_handle_Forze_Load=false;
bool flag_handle_Forze_Download=false;


int Handle_Maquina = 0;
int Handle_Maquina_Cashless = 0;

bool Act_Coin_in_Poker = false;
bool Act_Coin_out_Poker = false;
bool Act_Bill_Poker = false;
bool Act_Current_Credits = false;
int Cuenta_Save_Mecanicas=0;


extern bool Flag_Change_Counters_Response;
extern bool Flag_Change_Counters_One;
bool Extended_Ticket_Command=false;



extern bool Flag_Handle_Inactiva;
extern bool Flag_Handle_Activa;

extern bool Flag_Recv_Inactiva;
extern bool Flag_Recv_Activa;




#define flag_bloquea_Maquina            1
#define flag_desbloquea_Maquina         2
#define flag_encuesta_premio            3
#define escribe_tarjeta_mecanica        4
#define Encuesta_Info_Cashless          5
#define Flag_Reset_Handpay              6
#define Flag_Creditos_Premio            7
#define Actualiza_Machine               8

#define Interroga_Est_Reg_AFT           9
#define Cancela_Registro_AFT_MQ         10
#define Init_Reg_AFT                    11 
#define Registra_Maquina_AFT            12
#define Creditos_machine                13
#define Flag_Encuesta_Maquina_Juego     14
#define Flag_Encuesta_ROM               15
#define Flag_Carga_Bonus                16
#define Flag_Carga_Cashless             17
#define Flag_Descarga_Cashless          18

#define Flag_Update_Out_Cashless        19
#define Flag_Update_In_Cashless         20
#define Flag_Consulta_Creditos_Cashless 21

// MetodoCRC CRC_Maq;
// Contadores_SAS contadores;
// Eventos_SAS eventos;
int Contador_Save_Data=0;

char Coin_In_Poker_Data[9]      =  {'0', '0', '0', '0', '0', '0', '0', '0'};
char Coin_Out_Poker_Data[9]     =  {'0', '0', '0', '0', '0', '0', '0', '0'};
char Total_Drop_Poker_Data[9]   =  {'0', '0', '0', '0', '0', '0', '0', '0'};
char CurrentCredit_Poker_Data[9]  = {'0', '0', '0', '0', '0', '0', '0', '0'};
//---------------------------Configuración de UART2 Data 8bits, baud 19200, 1 Bit de stop, Paridad Disable---------------

char buff[128];



extern bool Request_Inactiva;
extern bool Request_Activa;
extern bool Ack_Maq_Inactiva;
extern bool Ack_Maq_Activa;

void Init_UART1()
{
  uart_config_t Configurazione_UART1 = {
      .baud_rate = 19200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
     // .rx_flow_ctrl_thresh = 122, /* Add Flow*/
  };
  uart_param_config(UART_NUM_1, &Configurazione_UART1);
  uart_set_pin(UART_NUM_1, U1TXD, U1RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, BUF_SIZE1, BUF_SIZE1, 20, &uart1_queue, 1));

}

void Init_UART2()
{

  uart_config_t Configurazione_UART2 = {
      .baud_rate = 19200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
     // .rx_flow_ctrl_thresh = 122, /* Add Flow*/
  };

  uart_param_config(NUMERO_PORTA_SERIALE, &Configurazione_UART2);
 // ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &Configurazione_UART2));

   // Inicializa configuracion tipo de maquina
    // NVS.begin("Config_ESP32", false);
    // uint16_t Port_COM = NVS.getUInt("COM");
    // NVS.end();
    uint16_t Port_COM=Variables_globales.Get_Variable_Global_Uint16(Uart_Port_Select);
    if (Port_COM == 1)
    {
      uart_set_pin(NUMERO_PORTA_SERIALE, U2TXD, U2RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    }
    else
    {
      uart_set_pin(NUMERO_PORTA_SERIALE, U1TXD, U1RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    }

  //uart_set_pin(NUMERO_PORTA_SERIALE, U2TXD, U2RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  ESP_ERROR_CHECK(uart_driver_install(NUMERO_PORTA_SERIALE, BUF_SIZE, BUF_SIZE, 20, &uart2_queue, 0));
  
  //-----------------------------------------------Aquí Tareas Nucleo 0 Comunicación Maquina------------------------------
  xTaskCreatePinnedToCore(UART_ISR_ROUTINE, "UART_ISR_ROUTINE", 5048, NULL, configMAX_PRIORITIES, &RecepcionRS232, 1); // Máx Priority principal
 // xTaskCreatePinnedToCore(Encuestas_Maquina, "Encuestas", 2048, NULL, configMAX_PRIORITIES - 15, &Encuestas, 1);
  xTaskCreatePinnedToCore(Encuestas_Maquina, "Encuestas", 4048, NULL, configMAX_PRIORITIES - 15, &Encuestas, 1);
  //----------------------------------------------------------------------------------------------------------------------
}

void Init_RS232()
{
  /* */
  NVS.begin("Config_ESP32", false);
  uint16_t Port_COM = NVS.getUInt("COM");
  NVS.end();
 
  if (Port_COM == 1)
  {
    uart_config_t Configurazione_UART2 = {
        .baud_rate = 19200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        // .rx_flow_ctrl_thresh = 122, /* Add Flow*/
    };
    uart_param_config(NUMERO_PORTA_SERIALE, &Configurazione_UART2);
    uart_set_pin(NUMERO_PORTA_SERIALE, U2TXD, U2RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ESP_ERROR_CHECK(uart_driver_install(NUMERO_PORTA_SERIALE, BUF_SIZE, BUF_SIZE, 20, &uart2_queue, 0));
    //-----------------------------------------------Aquí Tareas Nucleo 0 Comunicación Maquina------------------------------
    xTaskCreatePinnedToCore(UART_ISR_ROUTINE, "UART_ISR_ROUTINE", 5048, NULL, configMAX_PRIORITIES, &RecepcionRS232, 1); // Máx Priority principal
    xTaskCreatePinnedToCore(Encuestas_Maquina, "Encuestas", 2048, NULL, configMAX_PRIORITIES - 15, &Encuestas, 1);
    //----------------------------------------------------------------------------------------------------------------------
  }else if(Port_COM == 2)
  {
    uart_config_t Configurazione_UART2 = {
        .baud_rate = 19200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        // .rx_flow_ctrl_thresh = 122, /* Add Flow*/
    };
    uart_param_config(NUMERO_PORTA_SERIALE, &Configurazione_UART2);
    uart_set_pin(NUMERO_PORTA_SERIALE, U1TXD, U1RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ESP_ERROR_CHECK(uart_driver_install(NUMERO_PORTA_SERIALE, BUF_SIZE, BUF_SIZE, 20, &uart2_queue, 0));
    //-----------------------------------------------Aquí Tareas Nucleo 0 Comunicación Maquina------------------------------
    xTaskCreatePinnedToCore(UART_ISR_ROUTINE, "UART_ISR_ROUTINE", 5048, NULL, configMAX_PRIORITIES, &RecepcionRS232, 1); // Máx Priority principal
    xTaskCreatePinnedToCore(Encuestas_Maquina, "Encuestas", 2048, NULL, configMAX_PRIORITIES - 15, &Encuestas, 1);
    //----------------------------------------------------------------------------------------------------------------------
  }
  else if (Port_COM == 3)
  {
    /* ------------------------> Inicializa UART 1 <---------------------------------------- */
    uart_config_t Configurazione_UART1 = {
        .baud_rate = 19200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        // .rx_flow_ctrl_thresh = 122, /* Add Flow*/
    };
    uart_param_config(UART_NUM_1, &Configurazione_UART1);
    uart_set_pin(UART_NUM_1, U1TXD, U1RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, BUF_SIZE1, BUF_SIZE1, 20, &uart1_queue, 1));
    /*-----------------------------------------------------------------------------------------*/


    /* --------------------------------------------------------> Inicializa UART 2 <--------------------------------------------*/
    uart_config_t Configurazione_UART2 = {
        .baud_rate = 19200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        // .rx_flow_ctrl_thresh = 122, /* Add Flow*/
    };
    uart_param_config(NUMERO_PORTA_SERIALE, &Configurazione_UART2);
    uart_set_pin(NUMERO_PORTA_SERIALE, U1TXD, U1RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ESP_ERROR_CHECK(uart_driver_install(NUMERO_PORTA_SERIALE, BUF_SIZE, BUF_SIZE, 20, &uart2_queue, 0));
    /*----------------------------------------------------------------------------------------------------------------------------*/

    //----------------------------------------------> Aquí Tareas Comunicación Maquina Puerto 1 <-----------------------------------
    xTaskCreatePinnedToCore(UART_ISR_ROUTINE, "UART_ISR_ROUTINE", 5048, NULL, configMAX_PRIORITIES, &RecepcionRS232, 1); // Máx Priority principal
    xTaskCreatePinnedToCore(Encuestas_Maquina, "Encuestas", 2048, NULL, configMAX_PRIORITIES - 15, &Encuestas, 1);
    /*------------------------------------------------> Aqui Tareas Comunicacion Maquina Puerto 2 <-------------------------------*/
    xTaskCreatePinnedToCore(UART_ISR_ROUTINE_NUM1, "UART_ISR_ROUTINE_NUM1", 5048, NULL, configMAX_PRIORITIES-5, &RecepcionRS232_NUM1, 1); // Second Max Priority
    xTaskCreatePinnedToCore(Encuestas_Maquina_NUM1, "Encuestas_NUM1", 2048, NULL, configMAX_PRIORITIES - 20, &Encuestas_NUM1, 1);
    //------------------------------------------------------------------------------------------------------------------------------
  }
}
//----------------------------------------------Envio de Datos UART2------------------------------------------------------
void sendDataa(const char *datos, unsigned int tamano) //  Envia Datos por UART2
{
  ESP_ERROR_CHECK(uart_wait_tx_done(UART_NUM_2, 10));    //  Espera 10ms  para envio de dato anterior
  uart_write_bytes(NUMERO_PORTA_SERIALE, datos, tamano); // Envia Datos Sin tener en cuenta Bit de paridad
  ESP_ERROR_CHECK(uart_wait_tx_done(UART_NUM_2, 10));    // Espera 10ms  para envio de dato actual
  
}
//---------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------- Función Para Selección De Paridad---------------------------------------------
// Return 1 Par & 0 impar
bool Conta_Parity_bits(int Bits)
{ // Función P
  int contador = 0;
  if (Bits == 0x00)
  {
    return true;
  }
  else
  {
    String Bits_Binario = String(Bits, BIN); // Convierte a  String Binario

    for (int i = 0; i < 8; i++)
    {
      if (Bits_Binario[i] == '1')
      {
        contador++;
      }
    }
    if (contador % 2 == 0)
    {
      return true; // par EVEN
    }
    else
    {
      return false; // impar ODD
    }
  }
}
//----------------------------------------------------------------------------------------------------------------------------
int Convert_Char_To_Int16(char buffer[]) {
    int resultado = 0;
    int factor = 1;
    int longitud = strlen(buffer);
    int primerDigito = 0;

    for (int i = 0; i < longitud; i++) {
        if (buffer[i] >= '0' && buffer[i] <= '9') {
            if (buffer[i] != '0' || primerDigito) {
                resultado = resultado * 10 + (buffer[i] - '0');
                primerDigito = 1;
            }
        }
    }

    return resultado;
}

void Procesa_Eventos(int Evento)
{

  switch (Evento)
  {
  case 0x01: /* ACK carga  bonus recibido */
    if (Variables_globales.Get_Variable_Global(Solicitud_Carga_Bonus))
    {
      Variables_globales.Set_Variable_Global(Flag_ACK_Carga_Bonus_Pendiente, true); /* Solicitud recibida por la maquina  */
    }
    break;

  case 0x7C: /* Bonus cargado en la maquina */
    if (Variables_globales.Get_Variable_Global(Solicitud_Carga_Bonus))
    {
      Variables_globales.Set_Variable_Global(Status_Load_Bonus, true); /* Bonus cargado a la maquina */
      Variables_globales.Set_Variable_Global(Solicitud_Carga_Bonus, false);
    }
    break;

  case 0x00: /* Maquina cancelo la solicitud */
    if (Variables_globales.Get_Variable_Global(Solicitud_Carga_Bonus) && !Variables_globales.Get_Variable_Global(Status_Load_Bonus));
    {
      Variables_globales.Set_Variable_Global(Status_Load_Bonus, false);
      Variables_globales.Set_Variable_Global(Solicitud_Carga_Bonus, false);
    }
    break;
  default:
    break;
  }
}
//------------------------------------------------------Transmite Comandos Poll-----------------------------------------------
void Transmite_Poll(unsigned char Com_SAS)
{

  char dat5[1] = {Com_SAS};
  if (Com_SAS != 0)
  {
    sendDataa(dat4, sizeof(dat4)); // Transmito Byte con 9bit = 1
  }
  else
  {

    sendDataa(dat3, sizeof(dat3)); // Transmito Byte con 9bit = 1
  }
  if (Com_SAS != 0)
  {
    if (Conta_Parity_bits(Com_SAS))
    {
      delay(3);
      uart_set_parity(UART_NUM_2, UART_PARITY_EVEN); // parity par
      sendDataa(dat5, sizeof(dat5));
      uart_set_parity(UART_NUM_2, UART_PARITY_DISABLE); // reset parity
      delay(100);                                       // 100
    }
    else
    {
      uart_set_parity(UART_NUM_2, UART_PARITY_ODD); // parity impar
      sendDataa(dat5, sizeof(dat5));
      delay(100);                                       // 100
      uart_set_parity(UART_NUM_2, UART_PARITY_DISABLE); // reset parity
    }
  }
  else
  {
    delay(100); // 100
  }
}
//----------------------------------------------------------------------------------------------------------------------------

//---------------------------------------Función Para  Transmición de Comandos Largos-----------------------------------------
void Transmite_Poll_Long(unsigned char Com_SAS)
{
  char Datos[1] = {Com_SAS};
  if (Conta_Parity_bits(Com_SAS))
  {
    delay(2);
    uart_set_parity(UART_NUM_2, UART_PARITY_EVEN); // parity par
    sendDataa(Datos, 1);
    // uart_set_parity(UART_NUM_2,UART_PARITY_DISABLE); //reset parity
  }
  else
  {
    delay(2);
    uart_set_parity(UART_NUM_2, UART_PARITY_ODD); // parity impar
    sendDataa(Datos, 1);
    //  uart_set_parity(UART_NUM_2,UART_PARITY_DISABLE); //reset parity
  }
  uart_set_parity(UART_NUM_2, UART_PARITY_DISABLE); // reset parity
}
//----------------------------------------------------------------------------------------------------------------------------
void Request_Ack(char Ack)
{

  if (Ack == 0x01)
  {
    if (Request_Inactiva)
    {
      Ack_Maq_Inactiva = true;
      Request_Inactiva = false;
    }

    if (Request_Activa)
    {
      Ack_Maq_Activa = true;
      Request_Activa = false;
    }
  }
}
//-------------------------Interrupción  Recepción de Datos-------------------------------------------------------------------
static void UART_ISR_ROUTINE(void *pvParameters)
{
  uart_event_t event;
  size_t buffered_size;
  bool exit_condition = false;
  char buffer[128];

  char Ack_AFT_Bonus[128];
  char buffer_contadores[128];
  int conta_bytes;

  // Infinite loop to run main bulk of task
  while (1)
  {
    // Loop will continually block (i.e. wait) on event messages from the event queue
    if (xQueueReceive(uart2_queue, (void *)&event, (portTickType)portMAX_DELAY))
    {

      // Handle received event
      if (event.type == UART_DATA)
      {
        conta_bytes = 0;
        int UART2_data_length = 0;
        ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_2, (size_t *)&UART2_data_length));
        UART2_data_length = uart_read_bytes(NUMERO_PORTA_SERIALE, UART2_data, UART2_data_length, 100);
        for (byte i = 0; i < UART2_data_length; i++)
        {
          if (UART2_data[i] != 0x00 && UART2_data[i] != 0x1F && UART2_data[i] != 0x80 && UART2_data[i] != 0x81)
          {
            //            Serial.print(UART2_data[i], HEX);
          }
          buffer[i] = UART2_data[i];
          conta_bytes++;
          
          Ack_AFT_Bonus[i]= UART2_data[i];
        }
        //        Serial.println();
      }
      
      


      if (conta_bytes == 1)
      {
        if (buffer[0] == 0x01 && flag_handle_maquina)
        {
          #ifdef Debug_ACK_MSG
          Serial.println("Mensaje de ACK recibido.................................................");
          #endif
          ACK_Maq = true;

          
          
        }

        Request_Ack(buffer[0]);
        

        // if(buffer[0] == 0x01 && flag_handle_maquina_Cashless)
        // {
        //   ACK_Maq_Cashless = true;
        // }

        // if(buffer[0]==0x01 && Variables_globales.Get_Variable_Global(Solicitud_Carga_Bonus)||buffer[0]==0x81 && Variables_globales.Get_Variable_Global(Solicitud_Carga_Bonus)||buffer[0]==0x7C && Variables_globales.Get_Variable_Global(Solicitud_Carga_Bonus))
        // {
        //   Cashless.Set_ACK_Bonus(buffer);
        //   Variables_globales.Set_Variable_Global(Solicitud_Carga_Bonus,false);
        // }
       
        if(buffer[0]==0x01 || buffer[0]==0x01&&buffer[1]==0x00)
        {
          if(Variables_globales.Get_Variable_Global(Solicitud_Carga_Bonus))
            Cashless.Set_ACK_Bonus(Ack_AFT_Bonus);
        }
      }
      if (buffer[3] == 0xF3 && buffer[4] == 0x07||buffer[3]==0xE1 &&buffer[4]==0x24)
      {
        Variables_globales.Set_Variable_Global_Char(Reset_Handay_OK,buffer[2]);
      }

      if (buffer[0] == 0x01 && conta_bytes > 1)
      {
        //   Serial.println("Es un contador");
        if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 9 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 15)
        {
          if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 9)
          {
            if (Recibe_Mecanicas)
            {
              #ifdef Debug_ACK_MSG
              Serial.println("Verifica recepcion de mensaje ACK Mecanicas");
              #endif
              Recibe_Mecanicas = false;
              if (Buffer.Verifica_buffer_Mecanicas(&buffer[0], conta_bytes))
              {
                #ifdef Debug_ACK_MSG
                Serial.println("Mensaje de ACK Mecanicas recibido.................................................");
                #endif
                ACK_Mecanicas = true;
              }
              else
              {
                #ifdef Debug_ACK_MSG
                Serial.println("No mensaje ACK Mecanicas.........................................................");
                #endif
              }
            }
            else
            {
              if (Buffer.Verifica_buffer_Mecanicas(&buffer[0], conta_bytes))
              {
               // Datos_OK=true;
               // Bandera_RS232_F=Bandera_RS232;
                Cuenta_Save_Mecanicas++;
                numero_contador++;
                int j = 1;
                for (size_t k = 0; k < 4; k++)
                {
                  char contador[7] = {};
                  bzero(contador, 7);
                  for (int i = 0; i < 8; i++)
                  {
                    contador[i] = buffer[j] + '0';
                    j++;
                  }
                  #ifdef Debug_Contadores
                    Serial.println(contador);
                  #endif
                  switch (k)
                  {
                  case 0:
                    Estructura_CSV[0] = RTC.getTime() + ","; // Add Hora Mecanicas
                    contadores.Set_Contadores(Total_Cancel_Credit, contador);
                    Add_Contador(contador, Total_Cancel_Credit, false);
                    contadores.Set_Contadores(Cancel_Credit_Hand_Pay, contador);
                    Add_Contador(contador, Cancel_Credit_Hand_Pay, false);

                    /* ----------------->Se agrego<--------------------------------- */
                    contadores.Change_Counters(contador);
                    /*---------------------------------------------------------------*/
                    break;
                  case 1:

                    contadores.Set_Contadores(Coin_In, contador);
                    Add_Contador(contador, Coin_In, false);
                    break;
                  case 2:
                    contadores.Set_Contadores(Coin_Out, contador);
                    Add_Contador(contador, Coin_Out, false);
                    break;
                  case 3:
                    contadores.Set_Contadores(Total_Drop, contador);
                    Add_Contador(contador, Total_Drop, false);
                    contadores.Set_Contadores(Bill_Amount, contador);
                    Add_Contador(contador, Bill_Amount, false);
                    contadores.Change_Counters_Bill_In(contador);
                    Selector_Modo_SD(); // Ftp o Storage

                    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 9)
                    {
                      for (int i = 0; i < Max_Encuestas; i++)
                      {
                        SD_Cont = SD_Cont + Estructura_CSV[i];
                      }
                      if (Variables_globales.Get_Variable_Global(Ftp_Mode) == false)
                      {
                        if (Cuenta_Save_Mecanicas >= 30)
                        {
                          Cuenta_Save_Mecanicas = 0;
                          Storage_Contadores_SD(Archivo_CSV_Contadores, Encabezado_Contadores, Variables_globales.Get_Variable_Global(Enable_Storage));
                        }
                      }
                      for (int i = 0; i < Max_Encuestas; i++)
                      {
                        if (i == Max_Encuestas - 1)
                        {
                          Estructura_CSV[i] = "n/a";
                        }
                        else
                        {
                          Estructura_CSV[i] = "n/a,";
                        }
                      }
                      Delete_Trama();
                    }

                    break;
                  }
                }
              }
              else
              {
                #ifdef Debug_ACK
                Serial.println("Error de CRC contadores Mecanicos...");
                #endif
              }
            }
          }

          if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 15)
          {
            if (Recibe_Mecanicas)
            {
              #ifdef Debug_ACK_MSG
              Serial.println("Verifica recepcion de mensaje ACK Mecanicas");
              #endif
              Recibe_Mecanicas = false;
              if (Buffer.Verifica_buffer_Mecanicas(&buffer[0], conta_bytes))
              {
                #ifdef Debug_ACK_MSG
                Serial.println("Mensaje de ACK Mecanicas recibido.................................................");
                #endif
                ACK_Mecanicas = true;
              }
              else
              {
                #ifdef Debug_ACK_MSG
                Serial.println("No mensaje ACK Mecanicas.........................................................");
                #endif
              }
            }
            else
            {
              if (Buffer.Verifica_buffer_Mecanicas(&buffer[0], conta_bytes))
              {
                Datos_OK=true;
                Bandera_RS232_F=Bandera_RS232;
                Cuenta_Save_Mecanicas++;
                numero_contador++;
                int j = 1;
                for (size_t k = 0; k < 4; k++)
                {
                  char contador[7] = {};
                  bzero(contador, 7);
                  for (int i = 0; i < 8; i++)
                  {
                    contador[i] = buffer[j] + '0';
                    j++;
                  }
                  #ifdef Debug_Contadores
                    Serial.println(contador);
                  #endif
                  switch (k)
                  {
                  case 0:
                    Estructura_CSV[0] = RTC.getTime() + ","; // Add Hora Mecanicas
                    contadores.Set_Contadores(Total_Cancel_Credit, contador);
                    Add_Contador(contador, Total_Cancel_Credit, false);
                    contadores.Set_Contadores(Cancel_Credit_Hand_Pay, contador);
                    Add_Contador(contador, Cancel_Credit_Hand_Pay, false);

                    contadores.Change_Counters(contador);
                    break;
                  case 1:

                    contadores.Set_Contadores(Coin_In, contador);
                    Add_Contador(contador, Coin_In, false);
                    break;
                  case 2:
                    contadores.Set_Contadores(Coin_Out, contador);
                    Add_Contador(contador, Coin_Out, false);
                    break;
                  case 3:
                    contadores.Set_Contadores(Total_Drop, contador);
                    Add_Contador(contador, Total_Drop, false);
                    contadores.Set_Contadores(Bill_Amount, contador);
                    Add_Contador(contador, Bill_Amount, false);
                    contadores.Change_Counters_Bill_In(contador);
                    break;

                  default:
                    break;
                  }
                }

                int Iterador=33;
                
                for(size_t k = 0; k < 4; k++)
                {
                  char Multiplicador[4] = {};
                  bzero(Multiplicador, 4);

                  char contador[7] = {};
                  bzero(contador, 7);

                  for (int i = 0; i < 4; i++)
                  {
                    Multiplicador[i] = buffer[Iterador] + '0';  
                    Iterador++;
                  }

                  contador[0] = '0';
                  contador[1] = '0';
                  contador[2] = '0';
                  contador[3] = '0';
                  contador[4] = Multiplicador[0];
                  contador[5] = Multiplicador[1];
                  contador[6] = Multiplicador[2];
                  contador[7] = Multiplicador[3];
                  
                  switch (k)
                  {

                  case 0:
                    contadores.Set_Contadores(Machine_Paid_Progresive_Payout, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                    Add_Contador(contador, Machine_Paid_Progresive_Payout, false);
                    break;
                  case 1:
                    contadores.Set_Contadores(Machine_Paid_External_Bonus_Payout, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                    Add_Contador(contador, Machine_Paid_External_Bonus_Payout, false);
                    break;
                  case 2:
                    contadores.Set_Contadores(Attendant_Paid_Progresive_Payout, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                    Add_Contador(contador, Attendant_Paid_Progresive_Payout, false);
                    break;
                  case 3:
                    contadores.Set_Contadores(Attendant_Paid_External_Bonus_Payout, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                    Add_Contador(contador, Attendant_Paid_External_Bonus_Payout, false);

                    Selector_Modo_SD(); // Ftp o Storage

                    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 15)
                    {

                      for (int i = 0; i < Max_Encuestas; i++)
                      {
                        SD_Cont = SD_Cont + Estructura_CSV[i];
                      }
                      if (Variables_globales.Get_Variable_Global(Ftp_Mode) == false)
                      {
                        if (Cuenta_Save_Mecanicas >= 30)
                        {
                          Cuenta_Save_Mecanicas = 0;
                          Storage_Contadores_SD(Archivo_CSV_Contadores, Encabezado_Contadores, Variables_globales.Get_Variable_Global(Enable_Storage));
                        }
                      }
                      for (int i = 0; i < Max_Encuestas; i++)
                      {
                        if (i == Max_Encuestas - 1)
                        {
                          Estructura_CSV[i] = "n/a";
                        }
                        else
                        {
                          Estructura_CSV[i] = "n/a,";
                        }
                      }
                      Delete_Trama();
                    }
                    break;

                  default:
                    break;
                  }
                }
                
              }
              else
              {
                #ifdef Debug_ACK
                Serial.println("Error de CRC contadores Mecanicos...");
                #endif
              }
            }
          }

          for (size_t i = 0; i < conta_bytes; i++)
          {
            #ifdef Debug_Contadores
            Serial.print(buffer[i], HEX);
            #endif
          }
          // Serial.println();
        }

        if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 15)
        {

          if(buffer[0]==0x01&& buffer[1]==0x69||buffer[0]==0x01&& buffer[1]==0x64)
          {
            Serial.println(" Ack Recibido Maquina");
            Buffer_Cashless.Set_Buffer_Transfer_EFT(buffer);
          }

          if (buffer[0] == 0x01 && buffer[1] == 0x73)
          {
#ifdef Debug_Contadores
            Serial.println("Genial.........>>>>>");
#endif
            Buffer_Cashless.Set_RX_AFT(Buffer_RX_AFT_, buffer);
            Buffer_Cashless.Set_Buffer_Reg_AFT(buffer);
          }

         

          if (buffer[0] == 0x01 && buffer[1] == 0x72)
          {

            Buffer_Cashless.Set_Buffer_Transfer_AFT(buffer);

            // Serial.println("Recibida Transferencia por la maquina");
            /* Descarga */
            Variables_globales.Set_Variable_Global(Machine_Receives_Load_Transfer, true);
            /* Carga*/
            if (Ack_Cashless_Transfer_Load)
            {
              Flag_Transfer_Ok=true;
              Variables_globales.Set_Variable_Global(Machine_Receives_Download_Transfer, true);
              
            }

            for(int i=0; i<129; i++)
            {
              buff[i]=buff[i];
            }

              if(buff[4]==0x40)
              {
                //Serial.println("Transfer OK");
              }
          }
          /* -----------------------------------> Buffer datos TITO <-------------------------------------------------- */
          if(buffer[1]==0x57 && buffer[2]==0x00||buffer[1]==0x57 && buffer[2]==0x01|| buffer[1]==0x57 && buffer[2]==0x80)
          {
            Buffer_Cashless.Set_Buffer_TITO(buffer);
          }

          if(buffer[1]==0x70||buffer[1]==0x71)
          {
            Buffer_Cashless.Set_Buffer_TITO_70(buffer);
          }

          if(buffer[0]==0x01 &&buffer[1]==0x58)
          {
            if(buffer[2]==0x00||buffer[2]==0x80||buffer[2]==0x81)
              Buffer_Cashless.Set_Buffer_TITO_58(buffer);
          }

          if(buffer[0]==0x01 && buffer[1]==0x4D)
          {
            Buffer_Cashless.Set_Buffer_TITO_4D(buffer);
          }

          if(buffer[0]==0x01 && buffer[1]==0x7C)
          {
           
            Buffer_Cashless.Set_Buffer_TITO_7C(buffer);
          }
          /*------------------------------------------------------------------------------------------------------------*/
          if(buffer[1]==0x74 && buffer[7] == 0xFF||buffer[1]==0x74 && buffer[7] == 0x40||buffer[1]==0x74 && buffer[7] == 0x00)
          {
            Variables_globales.Set_Variable_Global(Amount_Download_Ready,true);
            Buffer_Cashless.Set_RX_AFT(Buffer_RX_Cashless,buffer);
          }

          if(buffer[1]==0x7B)
          {
            // Serial.println("Recibido ack 7b");
            Buffer_Cashless.Init_Buffer_TITO_7B();
            Buffer_Cashless.Set_Buffer_TITO_7B(buffer);
            // Serial.println("----------------->Response<-------------------");
            // for(int i=0;i<20; i++)
            // {
            //   Serial.print(buffer[i],DEC);
            //   Serial.println();
            // }
            //  Serial.println("----------------->Response<-------------------");
          }

          

          if (Buffer.Verifica_buffer_Maq(buffer, conta_bytes))
          {
            Bandera_RS232_F=Bandera_RS232;
            numero_contador++;
            Datos_OK=true;


          
            for (int index = 0; index < conta_bytes; index++)
            {
              String buffer_contadores_string = String(buffer[index], HEX);
              buffer_contadores[index] = buffer_contadores_string.toInt();
            }

            if (buffer_contadores[0] == 0x01 && buffer[1] == 0x73 && buffer[3] == 0x01)
            {
#ifdef Debug_Contadores
              Serial.println("Genial.........>>>>>");
#endif

              //Buffer_Cashless.Set_RX_AFT(Buffer_RX_AFT_, buffer);
            }
            
            
            if (buffer_contadores[1] > 9 && buffer_contadores[1] < 16 || buffer_contadores[1] == 46 || buffer[1] == 0x1A)
            {
              char contador[7] = {};
              bzero(contador, 7);
              int j = 2;
              int dato = 0;
              for (int i = 0; i < 7; i++)
              {
                dato = (buffer_contadores[j] - (buffer_contadores[j] % 10)) / 10;
                contador[i] = dato + '0';
                i++;
                dato = buffer_contadores[j] % 10;
                contador[i] = dato + '0';
                j++;
              }
              #ifdef Debug_Contadores
              Serial.println(contador);
              #endif
              switch (buffer_contadores[1])
              {
              case 10:

                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 4)
                {
                  contadores.Set_Contadores(Copia_Cancel_Credit, contador);
                  
                }
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 4)
                {
                  contadores.Set_Contadores(Total_Cancel_Credit, contador);
                }

                //              if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 4)
                //                Calcula_Cancel_Credit_IRT();
                //? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 5 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 4||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 10||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 13 ||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 3 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 1||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2)
                {
                  Estructura_CSV[0] = RTC.getTime() + ","; // Add Hora MAQ Generica
                }
                Add_Contador(contador, Total_Cancel_Credit, false);

                if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 10||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 13)
                {
                  contadores.Set_Contadores(Cancel_Credit_Hand_Pay, contador);
                  Add_Contador(contador, Cancel_Credit_Hand_Pay, false);
                  /* ----------------->Se agrego<--------------------------------- */
                  contadores.Change_Counters(contador);
                  /*---------------------------------------------------------------*/
                }

                //Serial.println("Credit ");
                break;
              case 11:

                Coin_In_Poker_Data[0]=contador[0];
                Coin_In_Poker_Data[1]=contador[1];
                Coin_In_Poker_Data[2]=contador[2];
                Coin_In_Poker_Data[3]=contador[3];
                Coin_In_Poker_Data[4]=contador[4];
                Coin_In_Poker_Data[5]=contador[5];
                Coin_In_Poker_Data[6]=contador[6];
                Coin_In_Poker_Data[7]=contador[7];

               // Serial.println(Coin_In_Poker_Data);
                contadores.Set_Contadores(Coin_In, contador); //? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6 && Variables_globales.Get_Variable_Global(Flag_Hopper_Enable)||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 14 && Variables_globales.Get_Variable_Global(Flag_Hopper_Enable))
                {
                  Act_Coin_in_Poker = true;
                }
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 7 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 8)
                {
                  Estructura_CSV[0] = RTC.getTime() + ","; // Add Hora MAQ IGT Riel
                }
                Add_Contador(contador, Coin_In, false);
                contadores.Set_Contadores(Coin_In, contador); //? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 14)
                {
                  Estructura_CSV[0] = RTC.getTime() + ",";                                                       // Add Hora Poker
                  Add_Contador(contadores.Get_Contadores_Char(Total_Cancel_Credit), Total_Cancel_Credit, false); /*Poker*/
                }
                Add_Contador(contador, Coin_In, false);
                break;
              case 12:

                Coin_Out_Poker_Data[0]=contador[0];
                Coin_Out_Poker_Data[1]=contador[1];
                Coin_Out_Poker_Data[2]=contador[2];
                Coin_Out_Poker_Data[3]=contador[3];
                Coin_Out_Poker_Data[4]=contador[4];
                Coin_Out_Poker_Data[5]=contador[5];
                Coin_Out_Poker_Data[6]=contador[6];
                Coin_Out_Poker_Data[7]=contador[7];
              //  Serial.println(Coin_Out_Poker_Data);
                contadores.Set_Contadores(Coin_Out, contador); //? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6 && Variables_globales.Get_Variable_Global(Flag_Hopper_Enable)||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 14 && Variables_globales.Get_Variable_Global(Flag_Hopper_Enable))
                {
                  Act_Coin_out_Poker = true;
                }
                Add_Contador(contador, Coin_Out, false);
                break;
              case 13:
                Total_Drop_Poker_Data[0]=contador[0];
                Total_Drop_Poker_Data[1]=contador[1];
                Total_Drop_Poker_Data[2]=contador[2];
                Total_Drop_Poker_Data[3]=contador[3];
                Total_Drop_Poker_Data[4]=contador[4];
                Total_Drop_Poker_Data[5]=contador[5];
                Total_Drop_Poker_Data[6]=contador[6];
                Total_Drop_Poker_Data[7]=contador[7];
              //  Serial.println(Total_Drop_Poker_Data);

                contadores.Set_Contadores(Total_Drop, contador);
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6|| Configuracion.Get_Configuracion(Tipo_Maquina,0)==12||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 14)
                  contadores.Set_Contadores(Bill_Amount, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6 && Variables_globales.Get_Variable_Global(Flag_Hopper_Enable)||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 14 && Variables_globales.Get_Variable_Global(Flag_Hopper_Enable))
                {
                  Act_Bill_Poker = true;
                  if (Act_Coin_in_Poker && Act_Coin_out_Poker && Act_Bill_Poker && Act_Current_Credits)
                  {
                    Act_Coin_in_Poker = false;
                    Act_Coin_out_Poker = false;
                    Act_Bill_Poker = false;
                    Act_Current_Credits = false;
                    Variables_globales.Set_Variable_Global(Calc_Cancel_Credit, true);
                  }
                }
                Add_Contador(contador, Total_Drop, false);
                Add_Contador(contador, Bill_Amount, false);

                if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 10||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 13)
                {
                  contadores.Set_Contadores(Total_Drop, contador);
                  contadores.Set_Contadores(Bill_Amount, contador);
                  
                }

                break;
              case 14:
                contadores.Set_Contadores(Jackpot, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                Add_Contador(contador, Jackpot, false);
                break;
              case 15:
                contadores.Set_Contadores(Games_Played, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                Add_Contador(contador, Games_Played, false);
                break;
              case 46:

                if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 2)
                {
                  contadores.Set_Contadores(Bill_Amount, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                  Add_Contador(contador, Bill_Amount, false);
                }
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 5 && contadores.Get_Contadores_Int(Bill_Amount) == 0)
                {
                  char res[7] = {};
                  bzero(res, 7); // Pone el buffer en 0
                  memcpy(res, contadores.Get_Contadores_Char(Total_Drop), 7);
                  contadores.Set_Contadores(Bill_Amount, res);
                  Add_Contador(res, Bill_Amount, false);
                  
                }
                
                else if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2)
                {
                  contadores.Set_Contadores(Copia_Bill_Amount,contador);
                  Calcula_Bill_In_550();
                  Add_Contador(contadores.Get_Contadores_Char(Copia_Bill_Amount), Bill_Amount, false);
                }
                break;
              }
              if (buffer[1] == 0x1A)
              {
                CurrentCredit_Poker_Data[0]=contador[0];
                CurrentCredit_Poker_Data[1]=contador[1];
                CurrentCredit_Poker_Data[2]=contador[2];
                CurrentCredit_Poker_Data[3]=contador[3];
                CurrentCredit_Poker_Data[4]=contador[4];
                CurrentCredit_Poker_Data[5]=contador[5];
                CurrentCredit_Poker_Data[6]=contador[6];
                CurrentCredit_Poker_Data[7]=contador[7];
               // Serial.println(Total_Drop_Poker_Data);
                
                contadores.Set_Contadores(Current_Credits, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                Add_Contador(contador, Current_Credits, false);
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6 && Variables_globales.Get_Variable_Global(Flag_Hopper_Enable)||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 14 && Variables_globales.Get_Variable_Global(Flag_Hopper_Enable))
                {
                  Act_Current_Credits = true;
                }

                if(Credit_Handle)
                  Actualizacion_datos_Ok=true;
                 
              }
            }
            

            else if (buffer[1] == 0x2A || buffer[1] == 0x2B || (buffer[1] > 0x3B && buffer[1] < 0x44))
            {
              char contador[7] = {};
              bzero(contador, 7);
              int j = 2;
              int dato = 0;
              for (int i = 0; i < 7; i++)
              {
                dato = (buffer_contadores[j] - (buffer_contadores[j] % 10)) / 10;
                contador[i] = dato + '0';
                i++;
                dato = buffer_contadores[j] % 10;
                contador[i] = dato + '0';
                j++;
              }
              #ifdef Debug_Contadores
              Serial.println(contador);
              #endif

              switch (buffer[1])
              {
              case 0x2A:
                contadores.Set_Contadores(Physical_Coin_In, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                Add_Contador(contador, Physical_Coin_In, false);

                if(Configuracion.Get_Configuracion(Tipo_Maquina,0)==11)
                {
                  Selector_Modo_SD();
                  Contador_Save_Data++;
                  for (int i = 0; i < Max_Encuestas; i++)
                  {
                    SD_Cont = SD_Cont + Estructura_CSV[i];
                  }
                  if (Variables_globales.Get_Variable_Global(Ftp_Mode) == false)
                  {
                    if(Contador_Save_Data>=3)
                    {
                      Storage_Contadores_SD(Archivo_CSV_Contadores, Encabezado_Contadores, Variables_globales.Get_Variable_Global(Enable_Storage));
                      Contador_Save_Data=0;
                    }
                  }

                  for (int i = 0; i < Max_Encuestas; i++)
                  {
                    if (i == Max_Encuestas - 1)
                    {
                        Estructura_CSV[i] = "n/a";
                    }
                    else
                    {
                        Estructura_CSV[i] = "n/a,";
                    }
                  }
                  Delete_Trama();
                }
                break;
              case 0x2B:
                contadores.Set_Contadores(Physical_Coin_Out, contador);
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 4||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 16)
                {
                  Calcula_Cancel_Credit_IRT();
                }   
                Add_Contador(contador, Physical_Coin_Out, false);
                break;
              case 0x3C:
                contadores.Set_Contadores(Billetes_2k, contador);
                break;
              case 0x3F:
                contadores.Set_Contadores(Billetes_5k, contador);
                break;
              case 0x40:
                contadores.Set_Contadores(Billetes_10k, contador);
                break;
              case 0x41:
                contadores.Set_Contadores(Billetes_20k, contador);
                break;
              case 0x43:
                contadores.Set_Contadores(Billetes_50k, contador);
                break;
              default:
               // Serial.println("Default");
                break;
              }
            }

            else if (buffer[1] == 0x1C)
            {
              char contador[7] = {};
              bzero(contador, 7);
              int j = 26;
              int dato = 0;
              for (int i = 0; i < 7; i++)
              {
                dato = (buffer_contadores[j] - (buffer_contadores[j] % 10)) / 10;
                contador[i] = dato + '0';
                i++;
                dato = buffer_contadores[j] % 10;
                contador[i] = dato + '0';
                j++;
              }
              #ifdef Debug_Contadores
              Serial.println(contador);
              #endif

              contadores.Set_Contadores(Door_Open, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Add_Contador(contador, Door_Open, false);
              Selector_Modo_SD(); // Ftp o Storage
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 7
                ||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 8||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 10 ||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 13||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 14||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 16)
                {
                  Contador_Save_Data++;
                  for (int i = 0; i < Max_Encuestas; i++)
                  {
                    SD_Cont = SD_Cont + Estructura_CSV[i];
                  }
                  if (Variables_globales.Get_Variable_Global(Ftp_Mode) == false)
                  {

                    if(Configuracion.Get_Configuracion(Tipo_Maquina,0)==6&& Variables_globales.Get_Variable_Global(Flag_Hopper_Enable)==false)
                    {
                      if(Contador_Save_Data>=9)
                      {
                        Storage_Contadores_SD(Archivo_CSV_Contadores, Encabezado_Contadores, Variables_globales.Get_Variable_Global(Enable_Storage));
                        Contador_Save_Data=0;
                      }
                      
                    }
                    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 7||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 8 ||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 10 ||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 13)
                    {

                      if(Contador_Save_Data>=4)
                      {
                        Storage_Contadores_SD(Archivo_CSV_Contadores, Encabezado_Contadores, Variables_globales.Get_Variable_Global(Enable_Storage));  
                        Contador_Save_Data=0;
                      }
                    }
                  }
                  for (int i = 0; i < Max_Encuestas; i++)
                  {
                    if (i == Max_Encuestas - 1)
                    {
                      Estructura_CSV[i] = "n/a";
                    }
                    else
                    {
                      Estructura_CSV[i] = "n/a,";
                    }
                  }
                  Delete_Trama();
                }
              
              
            }

            else if (buffer[1] == 0x18)
            {
              int unidades, descenas, centenas, uni_mil, desc_mil, cent_mil, uni_millon, desc_millon = 0;
              char contador[8] = {};

              desc_millon = 0;
              contador[0] = desc_millon + '0';
              uni_millon = 0;
              contador[1] = uni_millon + '0';

              cent_mil = 0;
              contador[2] = cent_mil + '0';
              desc_mil = 0;
              contador[3] = desc_mil + '0';

              uni_mil = (buffer_contadores[2] - (buffer_contadores[2] % 10)) / 10;
              contador[4] = uni_mil + '0';
              centenas = buffer_contadores[2] % 10;
              contador[5] = centenas + '0';

              descenas = (buffer_contadores[3] - (buffer_contadores[3] % 10)) / 10;
              contador[6] = descenas + '0';
              unidades = buffer_contadores[3] % 10;
              contador[7] = unidades + '0';
              contadores.Set_Contadores(Games_Since_Last_Power_Up, contador); // ? Serial.println("Guardado con exito") : Serial.println("No se pudo guardar");
              Add_Contador(contador, Games_Since_Last_Power_Up, true);
            
              
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 5 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 4 ||
                Configuracion.Get_Configuracion(Tipo_Maquina, 0) ==2||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 3 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 1 ||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
                {

                  Contador_Save_Data++;
                  for (int i = 0; i < Max_Encuestas; i++)
                  {
                    SD_Cont = SD_Cont + Estructura_CSV[i];
                  }
                  if (Variables_globales.Get_Variable_Global(Ftp_Mode) == false)
                  {
                    if(Contador_Save_Data>=4&& Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 4)
                    {
                      Selector_Modo_SD();
                      Storage_Contadores_SD(Archivo_CSV_Contadores, Encabezado_Contadores, Variables_globales.Get_Variable_Global(Enable_Storage));
                      Contador_Save_Data=0;
                    }

                    if(Contador_Save_Data>1&& Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 5 ||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 3 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 1||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
                    {
                      Selector_Modo_SD();
                      Storage_Contadores_SD(Archivo_CSV_Contadores, Encabezado_Contadores, Variables_globales.Get_Variable_Global(Enable_Storage));
                      Contador_Save_Data=0;
                    }

                    if(Contador_Save_Data>1&& Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2)
                    {
                      Selector_Modo_SD();
                      Storage_Contadores_SD(Archivo_CSV_Contadores, Encabezado_Contadores, Variables_globales.Get_Variable_Global(Enable_Storage));
                      Contador_Save_Data=0;
                    }
                  }
                  for (int i = 0; i < Max_Encuestas; i++)
                  {
                    if (i == Max_Encuestas - 1)
                    {
                      Estructura_CSV[i] = "n/a";
                    }
                    else
                    {
                      Estructura_CSV[i] = "n/a,";
                    }
                  }
                  Delete_Trama();
                }
            }
            else if (buffer[1] == 0x1F)
            {
              char contador[20] = {};

              int j = 2;
              for (int i = 0; i < 20; i++)
              {
                contador[i] = buffer[j];
                j++;
              }
              #ifdef Debug_Contadores
              Serial.println(contador);
              #endif
              contadores.Set_Contadores(Informacion_Maquina, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Ultimo_Counter_=true;
            }

            else if (buffer[1] == 0x21)
            {
              char contador[2] = {};

              contador[0] = buffer[2];
              contador[1] = buffer[3];
              #ifdef Debug_Contadores
              Serial.print(contador[0], HEX);
              Serial.println(contador[1], HEX);
              #endif
              contadores.Set_Contadores(ROM_Signature, contador);
             
            }

            else if (buffer[1] == 0x2F)
            {
              char contador[10] = {};
              bzero(contador, 10);
              if (buffer[5] == 0x0D || buffer[5] == 0x0E)
              {
                int j = 6;
                int dato = 0;
                for (int i = 0; i < 9; i++)
                {
                  dato = (buffer_contadores[j] - (buffer_contadores[j] % 10)) / 10;
                  contador[i] = dato + '0';
                  i++;
                  dato = buffer_contadores[j] % 10;
                  contador[i] = dato + '0';
                  j++;
                }
              }
              else
              {
                int j = 6;
                int dato = 0;
                for (int i = 0; i < 7; i++)
                {
                  dato = (buffer_contadores[j] - (buffer_contadores[j] % 10)) / 10;
                  contador[i] = dato + '0';
                  i++;
                  dato = buffer_contadores[j] % 10;
                  contador[i] = dato + '0';
                  j++;
                }
              }
              #ifdef Debug_Contadores
              Serial.println(contador);
              #endif
              switch (buffer[5])
              {
              case 0x0D:
                contadores.Set_Contadores(Ticket_In, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                Add_Contador(contador, Ticket_In, false);

                if(Comando_Entradas_Tito_OK)
                {
                  Flag_Entradas_Tito_OK=true;
                  Comando_Entradas_Tito_OK=false;
                }
                break;
              case 0x0E:
                contadores.Set_Contadores(Ticket_Out, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                Add_Contador(contador, Ticket_Out, false);
                if(Comando_Salidas_Tito_OK)
                {
                  Flag_Salidas_Tito_OK=true;
                  Comando_Salidas_Tito_OK=false;
                }
                break;
              case 0x1D:
                contadores.Set_Contadores(Machine_Paid_Progresive_Payout, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                Add_Contador(contador, Machine_Paid_Progresive_Payout, false);
                break;
              case 0x1E:
                contadores.Set_Contadores(Machine_Paid_External_Bonus_Payout, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                Add_Contador(contador, Machine_Paid_External_Bonus_Payout, false);
                break;
              case 0x20:
                contadores.Set_Contadores(Attendant_Paid_Progresive_Payout, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                Add_Contador(contador, Attendant_Paid_Progresive_Payout, false);
                break;
              case 0x21:
                contadores.Set_Contadores(Attendant_Paid_External_Bonus_Payout, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                Add_Contador(contador, Attendant_Paid_External_Bonus_Payout, false);
                break;
              case 0x24:
                contadores.Set_Contadores(Total_Coin_Drop, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                Add_Contador(contador, Total_Coin_Drop, false);
                break;
              case 0x2E:
                contadores.Set_Contadores(Casheable_In, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                Add_Contador(contador, Casheable_In, false);
                break;
              case 0x2F:
                contadores.Set_Contadores(Casheable_Restricted_In, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                Add_Contador(contador, Casheable_Restricted_In, false);
                break;
              case 0x30:
                contadores.Set_Contadores(Casheable_NONrestricted_In, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                Add_Contador(contador, Casheable_NONrestricted_In, false);

                if(Comando_Entradas_OK)
                {
                  //Serial.println(" Actualizados");
                  Flag_Entradas_Cashless_OK=true;
                  Comando_Entradas_OK=false;
                }
                break;
              case 0x32:
                contadores.Set_Contadores(Casheable_Out, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                Add_Contador(contador, Casheable_Out, false);
                break;
              case 0x33:
                contadores.Set_Contadores(Casheable_Restricted_Out, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                Add_Contador(contador, Casheable_Restricted_Out, false);
                break;
              case 0x34:
                contadores.Set_Contadores(Casheable_NONrestricted_Out, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                
                Add_Contador(contador, Casheable_NONrestricted_Out, false);

                if(Comando_Salidas_OK)
                {
                  //Serial.println(" Actualizados");
                  Flag_Salidas_Cashless_OK=true;
                  Comando_Salidas_OK=false;
                }
                break;
              }
            }

            else if (buffer[1] == 0x2D)
            {
              char contador[7] = {};
              bzero(contador, 7);
              int j = 4;
              int dato = 0;
              for (int i = 0; i < 7; i++)
              {
                dato = (buffer_contadores[j] - (buffer_contadores[j] % 10)) / 10;
                contador[i] = dato + '0';
                i++;
                dato = buffer_contadores[j] % 10;
                contador[i] = dato + '0';
                j++;
              }
              #ifdef Debug_Contadores
              Serial.println(contador);
              #endif

              
              if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 12)
              {
                contadores.Set_Contadores(Cancel_Credit_Hand_Pay, contador);
                contadores.Set_Contadores(Total_Cancel_Credit, contador);
                Add_Contador(contador, Cancel_Credit_Hand_Pay, false);
                Add_Contador(contador, Total_Cancel_Credit, false);

                /* ----------------->Se agrego<--------------------------------- */
                contadores.Change_Counters(contador);
                /*---------------------------------------------------------------*/
              }else{
                contadores.Set_Contadores(Cancel_Credit_Hand_Pay, contador);
                Add_Contador(contador, Cancel_Credit_Hand_Pay, false);
                /* ----------------->Se agrego<--------------------------------- */
                contadores.Change_Counters(contador);
                /*---------------------------------------------------------------*/

                // Serial.println("Handpay ");
              }
             
             // contadores.Change_Counters(contador);
            }

            else if(buffer[1]==0x0F)
            {

              if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 16)
              {

                char contador[8] = {};
                bzero(contador, 8);
                int j = 2;
                int dato = 0;
                for (int i = 0; i < 8; i++)
                {
                  dato = (buffer_contadores[j] - (buffer_contadores[j] % 10)) / 10;
                  contador[i] = dato + '0';
                  i++;
                  dato = buffer_contadores[j] % 10;
                  contador[i] = dato + '0';
                  j++;
                }
#ifdef Debug_Contadores
                Serial.println(contador);
#endif
                contadores.Set_Contadores(Copia_Cancel_Credit, contador);


                /* Coin In */
                char contador_Coin_In[7] = {};
                bzero(contador_Coin_In, 7);
                int dato1 = 0;
                for (int i = 0; i < 7; i++)
                {
                  dato1 = (buffer_contadores[j] - (buffer_contadores[j] % 10)) / 10;
                  contador_Coin_In[i] = dato1 + '0';
                  i++;
                  dato1 = buffer_contadores[j] % 10;
                  contador_Coin_In[i] = dato1 + '0';
                  j++;
                }
                contadores.Set_Contadores(Coin_In, contador_Coin_In);


                /* Coin Out */
                char contador_Coin_Out[7] = {};
                bzero(contador_Coin_Out, 7);
                int dato2 = 0;
                for (int i = 0; i < 7; i++)
                {
                  dato2 = (buffer_contadores[j] - (buffer_contadores[j] % 10)) / 10;
                  contador_Coin_Out[i] = dato2 + '0';
                  i++;
                  dato2 = buffer_contadores[j] % 10;
                  contador_Coin_Out[i] = dato2 + '0';
                  j++;
                }
                contadores.Set_Contadores(Coin_Out, contador_Coin_Out);



                /* Total Drop */
                char contador_Total_Drop[7] = {};
                bzero(contador_Total_Drop, 7);
                int dato3 = 0;
                for (int i = 0; i < 7; i++)
                {
                  dato3 = (buffer_contadores[j] - (buffer_contadores[j] % 10)) / 10;
                  contador_Total_Drop[i] = dato3 + '0';
                  i++;
                  dato3 = buffer_contadores[j] % 10;
                  contador_Total_Drop[i] = dato3 + '0';
                  j++;
                }
                contadores.Set_Contadores(Total_Drop, contador_Total_Drop);

                
                /* Total Drop */
                char contador_Jackpot[7] = {};
                bzero(contador_Jackpot, 7);
                int dato4 = 0;
                for (int i = 0; i < 7; i++)
                {
                  dato4 = (buffer_contadores[j] - (buffer_contadores[j] % 10)) / 10;
                  contador_Jackpot[i] = dato4 + '0';
                  i++;
                  dato4 = buffer_contadores[j] % 10;
                  contador_Jackpot[i] = dato4 + '0';
                  j++;
                }
                contadores.Set_Contadores(Jackpot, contador_Jackpot);

                /* Games Played */
                char contador_Games_Played[7] = {};
                bzero(contador_Games_Played, 7);
                int dato5 = 0;
                for (int i = 0; i < 7; i++)
                {
                  dato5 = (buffer_contadores[j] - (buffer_contadores[j] % 10)) / 10;
                  contador_Games_Played[i] = dato5 + '0';
                  i++;
                  dato5 = buffer_contadores[j] % 10;
                  contador_Games_Played[i] = dato5 + '0';
                  j++;
                }
                contadores.Set_Contadores(Games_Played, contador_Games_Played);

                /* Almacenamiento memoria */
                Add_Contador(contador_Coin_In, Coin_In, false);
                Add_Contador(contador_Coin_Out, Coin_Out, false);
                Add_Contador(contador_Total_Drop, Total_Drop, false);
                Add_Contador(contador_Jackpot, Jackpot, false);
                Add_Contador(contador_Games_Played, Games_Played, false);
              }
            }

            else if(buffer[7]==0x00 |buffer[7]==0x40 |buffer[7]==0xFF)
            {
              if(Buffer_Cashless.Set_RX_AFT(Info_MQ_AFT,buffer))
              {
                Variables_globales.Set_Variable_Global(Consulta_Info_Cashless_OK,true);
              }

            }
            
            // else if (buffer[1] == 0x1B)
            // {
            //   char contador[10] = {};
            //   bzero(contador, 10);
            //   int j = 4;
            //   int dato = 0;
            //   for (int i = 0; i < 10; i++)
            //   {
            //     dato = (buffer_contadores[j] - (buffer_contadores[j] % 10)) / 10;
            //     contador[i] = dato + '0';
            //     i++;
            //     dato = buffer_contadores[j] % 10;
            //     contador[i] = dato + '0';
            //     j++;
            //   }
            //   #ifdef Debug_Contadores
            //   Serial.println(contador);
            //   #endif

            //   contadores.Set_Contadores(Premio_1B, contador);
            // }

            /* ----------------------------> Aqui contadores EFT <----------------------*/
            
            if (buffer[0] == 0x01 && buffer[1] == 0x1D && Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2||buffer[0] == 0x01 && buffer[1] == 0x1D && Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
            {

              unsigned char j=10,Contador;
              /* Total Drop */
              char contador_cashable_In[7] = {};
              bzero(contador_cashable_In, 7);
              int dato4 = 0;
              for (int i = 0; i < 7; i++)
              {
                dato4 = (buffer_contadores[j] - (buffer_contadores[j] % 10)) / 10;
                contador_cashable_In[i] = dato4 + '0';
                i++;
                dato4 = buffer_contadores[j] % 10;
                contador_cashable_In[i] = dato4 + '0';
                j++;
              }
              j=14;

              contadores.Set_Contadores(Casheable_Out,contador_cashable_In);

              char contador_cashleable_out[7] = {};
              bzero(contador_cashleable_out, 7);
              int dato5 = 0;
              for (int i = 0; i < 7; i++)
              {
                dato5 = (buffer_contadores[j] - (buffer_contadores[j] % 10)) / 10;
                contador_cashleable_out[i] = dato5 + '0';
                i++;
                dato5 = buffer_contadores[j] % 10;
                contador_cashleable_out[i] = dato5 + '0';
                j++;
              }
              contadores.Set_Contadores(Casheable_In, contador_cashleable_out);
            }

            if (buffer[0]==0x01 &&buffer[1] == 0x1B)
            {
              char contador[10] = {};
              bzero(contador, 10);
              int j = 4;
              int dato = 0;
              for (int i = 0; i < 10; i++)
              {
                dato = (buffer_contadores[j] - (buffer_contadores[j] % 10)) / 10;
                contador[i] = dato + '0';
                i++;
                dato = buffer_contadores[j] % 10;
                contador[i] = dato + '0';
                j++;
              }
              #ifdef Debug_Contadores
              Serial.println(contador);
              #endif

              contadores.Set_Contadores(Premio_1B, contador);
              /* Se agrego....*/

              if(Variables_globales.Get_Variable_Global(Handle_Premios_SAS))
                Accounting.Save_Handpay_Informations(buffer_contadores,contador);
            }
            // bzero(buffer, 128);
            conta_bytes = 0;

            //  Serial.print("Contador de encuestas con CRC valido --> ");
            //  Serial.println(numero_contador);
          }
          else
          {
            bzero(buffer, 128);
            #ifdef Debug_Contadores
            Serial.println("Error de CRC en contadores...");
            #endif
          }
        }
      }
      else if (buffer[0] != 0x00 && buffer[0] != 0x01 && buffer[0] != 0x1F)
      {

        if (eventos.Set_evento(buffer[0]))
        {
          #ifdef Debug_Eventos
          Serial.println("Es un evento");
          Serial.println("--------------------------------------------------");
          Serial.println();
          #endif
         // Bandera_RS232_F=Bandera_RS232;
          Selector_Modo_SD(); // Ftp o Storage
          // Guarda Evento En Memoria SD.
          int Evento = eventos.Get_evento();

          Info_Cashless.Bloquea_Descarga_EFT(Evento,Configuracion.Get_Configuracion(Tipo_Maquina, 0),contadores.Get_Client_ID_Transaccion_Int(),Variables_globales.Get_Variable_Global(Flag_Sesion_RFID));

          
          Info_Cashless.Estado_Juego_Maquina(Evento);
          // Backup.enviarInformacionMaquina(Backup.Eventos_Accounting(5,Evento));
          if(Capture_Evento_Cashless && Evento==0x69)
          {
            Capture_Evento_Cashless_OK=true;
            Capture_Evento_Cashless=false;
          }
          // if(Evento==0x52 && Variables_globales.Get_Variable_Global(Enable_Cashless) && Configuracion.Get_Configuracion(Tipo_Maquina, 0)<4 && Info_Cashless.Type_Sesion() == PLAYER_CASHLESS_SESION && !Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss))
          // {

          // }

         // Event_Transfer_Load_Pending(Evento);
         // Serial.println(Evento);
          //Requerimiento_TITO(Evento);
          // bool Attend=Buffer_Cashless.Attend_Tito_Request(Evento,Variables_globales.Get_Variable_Global(Comunicacion_Maq),false,true);
          // Variables_globales.Set_Variable_Global(Attend_Pending_Tito_Request,Attend);

          Info_Cashless.Requerimiento_AFT_6A(Evento, Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss));

          Tito.Requerimiento_TITO_Ticket_In(Evento, Variables_globales.Get_Variable_Global(Enable_Tito_Ticket));
          Tito.Requerimiento_TITO_Ticket_Out(Evento, Variables_globales.Get_Variable_Global(Enable_Tito_Ticket), Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss), Variables_globales.Get_Variable_Global(Descarga_Solo_Tito));

          //Cashless.Requerimiento_Bonusing(Evento,true);
          /* ------------------------> TITO <--------------------------*/
          // Tito.Handle_Event_Tito(Evento,true);

          /*----------------------------------------------------------*/
          // if(Evento==0x51 && Variables_globales.Get_Variable_Global(Enable_Cashless) && Configuracion.Get_Configuracion(Tipo_Maquina, 0)<4 && Info_Cashless.Type_Sesion() == PLAYER_CASHLESS_SESION && !Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss))
          // {
          //   Variables_globales.Set_Variable_Global(Excepcion_51,true);
          // }
          
          if(eventos.Ignore_Event(Evento) || Evento==0x51)
            Variables_globales.Set_Variable_Global(Dato_Evento_Valido, true);
            // Serial.println("Evento1");

            //if(eventos.Ignore_Event_Plus(Evento))
           
            // if(eventos.Ignore_Event(Evento))
            //   Variables_globales.Set_Variable_Global(Dato_Evento_Valido, true);
            // else
            //Serial.println("Evento Repetido en 1 Minuto");
          
          
          if (Evento != 0x00 && Datos_OK == true)
          {
            Bandera_RS232_F = Bandera_RS232;
          }

          if (Evento == 0x4F)
          {
            Variables_globales.Set_Variable_Global(Billete_Insert, true);
          }

          if (Evento == 0x51)
          {
            if (Variables_globales.Get_Variable_Global(Flag_Maquina_En_Juego) == false && Variables_globales.Get_Variable_Global(Flag_Sesion_RFID) == true && Convert_Char_To_Int16(contadores.Get_Contadores_Char(24)) > 10)
            {
              condicionCumplida = false;
            }
            Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO, false);
            //contadores.Close_ID_Operador(); /* Evento 51  Borra ID Para nueva lectura */
            Condicion_Cumpl = false;        /* Reset TimeOut*/
                                            // Variables_globales.Set_Variable_Global(MARCA_OPERADOR_VALIDO,true); /* Activa Bandera */
          }

          if (Evento != 0x6A && Evento != 0x8C && Evento != 0x00)
          {

            delay(10);

            if (eventos.Ignore_Event_Plus(Evento))
            {
              String Descrip = Tabla_Evento.Get_Descrip_Eventos(Evento);
              Add_String_Hora_EVEN(RTC.getTime());                                                           // Agrega Hora de Evento a String
              Add_String_EVEN(String(Evento), true);                                                         // Agrega Tipo de Evento a String
              Add_String_EVEN(Descrip, false);                                                               // Agrega Descripción  de Evento a String
              Store_Eventos_SD(Archivo_CSV_Eventos, Variables_globales.Get_Variable_Global(Enable_Storage)); // Envia String Completo.
            }  
          }
        }
      }
      bzero(buffer, 128); // Pone el buffer en 0 /*Descomentar*/
      
    }
    // If you want to break out of the loop due to certain conditions, set exit condition to true
    if (exit_condition)
    {
      break;
    }
  }
  vTaskDelay(10);
  free(UART2_data);
  vTaskDelete(NULL);
}
//----------------------------------------------------------------------------------------------------------------------------
unsigned long tiemp=0;
unsigned long temp2=0;
unsigned long inter=1000;
unsigned long tope=0;
//---------------------------- Tarea Para Consulta de Contadores y General Poll-----------------------------------------------
void Encuestas_Maquina(void *pvParameters)
{
  TickType_t xLastWakeTime;
  const TickType_t xDefaultFrequency = pdMS_TO_TICKS(100);

  TickType_t xFrequency;



  switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
  {
  case 4:
    xFrequency= pdMS_TO_TICKS(3000);
    break;

  case 14:
    xFrequency=pdMS_TO_TICKS(1000);
    break;

  case 16:
    xFrequency= pdMS_TO_TICKS(200);
    break;
  
  default:
    xFrequency = xDefaultFrequency;
    break;
  }

 
  for (;;)
  {
    // Verifica cada 10 segundos aproximadamente si hay comunicacion con la maquina
    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 0:
      //Serial.println("Defecto");
      break;
    case 1:
      //Serial.println("Cashless AFT");
      break;
    case 2:
      /*
      if (numero_encuesta > 18) // Numero de encuestas realizadas a la maquina
      {
        if (numero_contador < 10) // Numero de respuestas recibidas por la maquina
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, false);
        else
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
        numero_encuesta = 0;
        numero_contador = 0;
      }
      */
      break;
    case 3:
      //Serial.println("Cashless AFT Single");
      break;
    case 4:
      /*
      if (numero_encuesta > 10) // Numero de encuestas realizadas a la maquina
      {
        if (numero_contador < 8) // Numero de respuestas recibidas por la maquina
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, false);
        else
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
        numero_encuesta = 0;
        numero_contador = 0;
      }
      */
      break;
    case 5:
      /*
      if (numero_encuesta > 20) // Numero de encuestas realizadas a la maquina
      {
        if (numero_contador < 12) // Numero de respuestas recibidas por la maquina
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, false);
        else
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
        numero_encuesta = 0;
        numero_contador = 0;
      }
      */
      break;
    case 6:
      /*
      if (numero_encuesta > 6) // Numero de encuestas realizadas a la maquina
      {
        if (numero_contador < 5) // Numero de respuestas recibidas por la maquina
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, false);
        else
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
        numero_encuesta = 0;
        numero_contador = 0;
      }
      */
      break;
    case 7:
      /*
      if (numero_encuesta > 4) // Numero de encuestas realizadas a la maquina
      {
        if (numero_contador < 3) // Numero de respuestas recibidas por la maquina
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, false);
        else
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
        numero_encuesta = 0;
        numero_contador = 0;
      }
      */
      break;
    case 8:
    /*
      if (numero_encuesta > 4) // Numero de encuestas realizadas a la maquina
      {
        if (numero_contador < 3) // Numero de respuestas recibidas por la maquina
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, false);
        else
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
        numero_encuesta = 0;
        numero_contador = 0;
      }
      */
      break;
    case 9:
      
      if (numero_encuesta > 5) // Numero de encuestas realizadas a la maquina
      {
        if (numero_contador < 3) // Numero de respuestas recibidas por la maquina
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, false);
        else
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
        numero_encuesta = 0;
        numero_contador = 0;
      }
      
      break;

    case 10:
        /*
        Serial.println("Poker Solo SAS");
        
        */
      break;


     case 15:
      
      // if (numero_encuesta > 150) // Numero de encuestas realizadas a la maquina
      // {
      //   if (numero_contador < 3) // Numero de respuestas recibidas por la maquina
      //     Variables_globales.Set_Variable_Global(Comunicacion_Maq, false);
      //   else
      //     Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
      //   numero_encuesta = 0;
      //   numero_contador = 0;
      // }
      
      break;

    default:
      break;
    }

    // A Task shall never return or exit.
    // Get the actual execution tick
    xLastWakeTime = xTaskGetTickCount();
    // Switch the led
    int Vel_Poll=5;

    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 4||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 14||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 16)
    {
        Vel_Poll=15;
        if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 16)
        {
          Vel_Poll=8;
        }
    }

    if (bandera == 0 && Conta_Poll < Vel_Poll)
    {
      if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 15)
        sendDataa(dat, sizeof(dat)); // transmite sincronización
      bandera = 1;
      Conta_Poll++;
    }
    else if (bandera == 1 && Conta_Poll < Vel_Poll)
    {
      if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 15)
        Transmite_Poll(0x00); // Transmite Poll
      bandera = 0;
      Conta_Poll++;
    }
    else if (Conta_Poll >= Vel_Poll)
    {
      numero_encuesta++;
      Conta_Poll = 0;

      

      // if(Variables_globales.Get_Variable_Global(Handle_Controller_Transfer_Load))
      // {
      //   ACK_Maq_Cashless=true;
      //   Transmite_Load_AFT_Maq();
      //   Handle_Maquina_Cashless=0;
      //   flag_handle_maquina_Cashless=false;
      //   Variables_globales.Get_Variable_Global(Handle_Controller_Transfer_Load,false);
      // }

      // if(flag_handle_maquina_Cashless)
      // {
      //   switch (Handle_Maquina_Cashless)
      //   {
      //   case Flag_Carga_Cashless: /* Solicitud Carga */
      //     Variables_globales.Set_Variable_Global(Handle_Controller_Transfer_Load,true);
      //     ACK_Maq_Cashless=true;
      //     Transmite_Load_AFT_Maq();
      //     Handle_Maquina_Cashless=0;
      //     flag_handle_maquina_Cashless=false;
          
      //   break;

      //   case Flag_Descarga_Cashless:/* Solicitud Descarga */
      //     Variables_globales.Set_Variable_Global(Handle_Controller_Transfer_Download,true);
      //     ACK_Maq_Cashless=true;
      //     Transmite_Download_AFT_Maq();
      //     Handle_Maquina_Cashless=0;
      //     flag_handle_maquina_Cashless=false;
      //   break;

      //   default:
      //     Handle_Maquina_Cashless=0;
      //     flag_handle_maquina_Cashless=false;
      //     break;
      //   }
      // }
      if(Info_Cashless.Get_Controller_Transfer_Load())
      {
        ACK_Maq_Cashless=true;
        Transmite_Load_AFT_Maq();
        Handle_Maquina_Cashless=0;
        flag_handle_maquina_Cashless=false;
        Info_Cashless.Set_Controller_Transfer_Load(false);
      }

      if(Info_Cashless.Get_Controller_Transfer_Download()&& Info_Cashless.Get_Status_Transfer()==TRANSFER_IDLE)
      {
        Info_Cashless.Set_Controller_Transfer_Download(false);
        ACK_Maq_Cashless=true;
        Transmite_Download_AFT_Maq();
        Handle_Maquina_Cashless=0;
        flag_handle_maquina_Cashless=false;
      }

      if(Extended_Ticket_Command)
      {
        Tito_Expiration_Ticket();
        Extended_Ticket_Command=false;
      }

      if(Flag_Set_Ticket_Data)
      {
        Set_Extended_Ticket_Data();
        Flag_Set_Ticket_Data=false;
      }

      if(flag_handle_Forze_Load)
      {
        Transmite_Load_AFT_Maq();
        flag_handle_Forze_Load=false;
      }

      if(Update_In_Cashless)
      {
       Actualiza_Entradas_Cashless();
        Update_In_Cashless=false;
      }

      if(Update_In_Tito)
      {
        Actualiza_Entrada_Tito();
        Update_In_Tito=false;
      }

      if(Update_Out_Tito)
      {
        Actualiza_Salida_Tito();
        Update_Out_Tito=false;
      }

      if(Update_Out_Cashless)
      {
       Actualiza_Salidas();
        Update_Out_Cashless=false;
      }

      if(Variables_globales.Get_Variable_Global(Event_Load_Cashless_Pending))
      {
        Update_Status_Critical_Load();
        //Variables_globales.Set_Variable_Global(Trasnsmite_Transacion_to_Machine,false);
      }

      if(Variables_globales.Get_Variable_Global(Event_Dowmload_Cashless_Pending))
      {
        Update_Status_Critical_Download();
      }

      if(Variables_globales.Get_Variable_Global(Attend_Pending_Tito_Request) && !Tito.Get_Status_Process_Ticket())
      {
       // Serial.println("Atentido el requerimiento");
        Requerimiento_TITO();
        Variables_globales.Set_Variable_Global(Attend_Pending_Tito_Request,false);
      }
      
      if(Variables_globales.Get_Variable_Global(Attend_Pending_Tito_Request_In))
      {
       // Command_Reedemed_Ticket(); /* Transmite Transaccion Ticket */
        Requerimiento_Ticket_In();
        Variables_globales.Set_Variable_Global(Attend_Pending_Tito_Request_In,false);
      }
      
      if(Tito.Get_Confirma_Ticket())
      {
        Remove_Ticket_In_Buffer();
        Tito.Set_Confirma_Ticket(false);
      }


      
      if(App)
      {
        sendDataa(dat, sizeof(dat)); // Transmite SYN
        delay(200);

        sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        Transmite_Poll_Long(0x72);
        Transmite_Poll_Long(0x02);
        Transmite_Poll_Long(0xFF);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x0F);
        Transmite_Poll_Long(0x22);
        App=false;
      }

      if(Flaggg)
      {
        Init_Reg_Cashless();
        Flaggg=false;
      }

      if(Flag_Critial_Questions)
      {
        Critial_Question();
        Flag_Critial_Questions=false;

      }

      if(Flag_Handle_Inactiva)
      {
        Flag_Recv_Inactiva=true;
        //Serial.println("Inactiva Maquina");
        Transmite_Inactiva_Maquina();
        Flag_Handle_Inactiva=false;
      }

      if(Flag_Handle_Activa)
      {
        Flag_Recv_Activa=true;
        //Serial.println("Activa Maquina");
        Transmite_Activa_Maquina();
        Flag_Handle_Activa=false;
      }

      // if(Tito.Get_Status_3D() && !Tito.Get_Status_Process_Ticket())
      // {
      //   Serial.println("Confirma Ticket");
      //   Remove_Ticket_In_Buffer();
      //   Tito.Set_Status_3D(false);
      // }
      TIMEOUT_TRANSFER(Hadle_comunications);

      if (flag_handle_maquina)
      {
        switch (Handle_Maquina)
        {
        case 1:
          
          Transmite_Inactiva_Maquina();
          Handle_Maquina = 0;
          break;
        case 2:
          //    Serial.println("Activa Maquina");
          Transmite_Activa_Maquina();
          Handle_Maquina = 0;
          break;
        case 3:
          //    Serial.println("Encuesta Premio");
          Encuesta_contador_1B();
          Handle_Maquina = 0;
          break;
        case 4:
          //    Serial.println("Encuesta Premio");
          if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 9)
            Escribe_Tarjeta_Mecanica(Buffer.Get_buffer_tarjeta_mecanica());

          else if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 15)
            Escribe_Tarjeta_Mecanica_2(Buffer.Get_buffer_tarjeta_mecanica_2());
        
          Handle_Maquina = 0;
          break;
        case Encuesta_Info_Cashless:
          Interroga_Info_Cashless();
          Handle_Maquina = 0;
         // Serial.println("Encuesta info Cashless");
          break;
        case Flag_Reset_Handpay:
          Transmite_Reset_Handpay();
          Handle_Maquina = 0;
         // Serial.println("Transmite reset Handpay");
          break;

        case Flag_Creditos_Premio:
          _Transmite_Encuesta_Creditos_D_Premio();
          Handle_Maquina=0;
          //Serial.println("Encuesta creditos..");
          Variables_globales.Set_Variable_Global(Flag_Creditos_D_P,true);
          break;
        case Interroga_Est_Reg_AFT:
          Transmite_Registro_AFT_Maq();
          Handle_Maquina=0;
          break;
        case Cancela_Registro_AFT_MQ:
          Transmite_Cancela_Registro_MQ();
          Handle_Maquina=0;
        break;
        case Init_Reg_AFT:
          Transmite_Init_Registro();
        Handle_Maquina=0;
        break;

        case Registra_Maquina_AFT:
          Transmite_AFT_Registro_Machine();
          Handle_Maquina=0;
          break;

        case Creditos_machine:
          Transmite_Encuesta_Creditos();
          Handle_Maquina=0;
          break;

        case Flag_Encuesta_Maquina_Juego:
          Transmite_Encuesta_Maquina_Juego();
          Handle_Maquina=0;
          break;

        case Flag_Encuesta_ROM:
          if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)!=6 &&Configuracion.Get_Configuracion(Tipo_Maquina, 0)!=14 && Configuracion.Get_Configuracion(Tipo_Maquina, 0)!=15 && Configuracion.Get_Configuracion(Tipo_Maquina, 0)!=9)
          {
            Transmite_Encuesta_ROM();
          }
          Handle_Maquina=0;
          break;
        case Flag_Carga_Bonus:
          //Transmite_Carga_legacy_bonus_awards();
          Transmite_Carga_legacy_Bonus();
          Handle_Maquina=0;
          break;

        case Flag_Update_In_Cashless:
          //Actualiza_Cashless_Entradas();
          Handle_Maquina=0;
        break;

        case Flag_Update_Out_Cashless:
          //Actualiza_Cashless_Salidas();
          Handle_Maquina=0;
        break;

        case Flag_Consulta_Creditos_Cashless:
        Transmite_Consulta_Creditos_Cashless();
        Handle_Maquina=0;
        break;

        default:
          Handle_Maquina = 0;
          break;
        }
        flag_handle_maquina = false;
      }
      else
      {
        switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
        {
        case 0:
          Encuestas_Maquinas_Genericas();
          break;
        case 1:
          Encuestas_Maquinas_Genericas();
          break;
        case 2:
          Encuestas_Maquinas_EFT();
          break;
        case 3:
          Encuestas_Maquinas_Genericas();
          break;
        case 4:
          Encuestas_Maquinas_IRT();
          break;
        case 5:
          Encuestas_Maquinas_Genericas();
          break;
        case 6:
          Encuestas_Maquinas_Poker();
          break;
        case 7:
          Encuestas_Maquinas_IGT_Riel();
          break;
        case 8:
          Encuestas_Maquinas_IGT_Riel_Bill();
          break;
        case 9:
          Encuestas_Mecanicas();
          break;
        case 10:
          Encuestas_Maquinas_Simple();
          break;
        case 11:
          Encuestas_Aristocrat_Australiana();
          break;

        case 12:
          Encuestas_Maquinas_Simple_No_Cancel();
          break;
        case 13:
          Encuestas_Maquinas_Simple(); /* Rele*/
          break;

        case 14:
          Encuestas_Maquinas_Poker_Ertech_Slot();
          break;

        case 15:                       
          Encuestas_Mecanicas();  /* Mecanica 4 contadores */
          break;

        case 16:
          Encuestas_Maquinas_Ruleta_IRT();
          break;

        case 17:
          Encuestas_Maquinas_Genericas();
        break;

        default:
          break;
        }
      }
      //  Serial.print("Contador de encuestas realizadas --> ");
      //  Serial.println(numero_encuesta);
    }
    // Ejecuta   Taraea Encuestas_Maquina Cada 100ms

    // Ejecuta la tarea de encuestas cada xFrequency ms
    vTaskDelayUntil(&xLastWakeTime, xFrequency);

  }
  
  vTaskDelay(10);
}
//----------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------

void Storage_Contadores_SD(char *ARCHIVO, String Encabezado1, bool Enable)
{
  if (Enable == true)
  {
    Write_Data_File2(SD_Cont, ARCHIVO, true, Encabezado1);
    // Write_Data_File2("", ARCHIVO, true, Encabezado1);
    for (int i = 0; i <= SD_Cont.length(); i++)
    {
      SD_Cont.remove(i);
    }
  }
  else
  {
    #ifdef Debug_SD_CARD
    Serial.println("Guardado SD Deshabilitado..");
    #endif
    for (int i = 0; i <= SD_Cont.length(); i++)
    {
      SD_Cont.remove(i);
    }
  }
}

void Store_Eventos_SD(char *ARCHIVO, bool Enable)
{
  if (Enable == true)
  {
    Write_Data_File_Txt(SD_EVEN, ARCHIVO);
    for (int i = 0; i <= SD_EVEN.length(); i++)
    {
      SD_EVEN.remove(i);
    }
  }
  else
  {

    #ifdef Debug_SD_CARD
    Serial.println("Guardado SD Deshabilitado..");
    #endif
    for (int i = 0; i <= SD_EVEN.length(); i++)
    {
      SD_EVEN.remove(i);
    }
  }
}

void Add_String(char *contador, String stringDT, bool Separador)
{

  if (Separador == true)
  {
    SD_Cont += String(contador);
    SD_Cont += String(","); // Add Separador
  }
  else if (Separador == false)
  {
    SD_Cont += String(contador); // Delete Separador
  }
}

void Add_Contador(char *Contador_, int Filtro, bool Salto_Linea)
{
  if (Salto_Linea)
  {
    Estructura_CSV[Filtro] = String(Contador_);
  }
  else
  {
    Estructura_CSV[Filtro] = String(Contador_) + ",";
  }
}

void Add_String_EVEN(String EVENTO, bool Separador)
{
  if (Separador == true)
  {
    SD_EVEN += EVENTO;
    SD_EVEN += String(","); // Add Separador
  }
  else if (Separador == false)
  {
    SD_EVEN += String(EVENTO); // Delete Separador
  }
}

void Add_String_Hora(String Horario)
{
  SD_Cont += String(Horario);
  SD_Cont += String(","); // Add Separador
}

void Add_String_Hora_EVEN(String Horario)
{
  SD_EVEN += String(Horario);
  SD_EVEN += String(","); // Add Separador
}
//---------------------------- Maneja Encuestas Maquina - Tipo MAQ ----------------------------------------------
void Encuestas_Maquinas_Simple(void)
{
  Conta_Encuestas++;
  switch (Conta_Encuestas)
  {

  case 1:
    #ifdef Debug_Encuestas
    Serial.println("Total Cancel Credit"); // total cancel credit
    #endif
    Transmite_Poll(0x10);
    break;
  case 2:
    #ifdef Debug_Encuestas
    Serial.println("Coin In"); // Coin in
    #endif
    Transmite_Poll(0x11);
    break;
  case 3:
    #ifdef Debug_Encuestas
    Serial.println("Coin Out"); // Coin out
    #endif
    Transmite_Poll(0x12);
    break;
  case 4:
    #ifdef Debug_Encuestas
    Serial.println("Jackpot"); // Jackpot
    #endif
    Transmite_Poll(0x14);
    break;
  case 5:
    #ifdef Debug_Encuestas
    Serial.println("Total Drop"); // total drop
    #endif
    Transmite_Poll(0x13);
    break;
  case 6:
    #ifdef Debug_Encuestas
    Serial.println("Current Credits");
    #endif
    Transmite_Poll(0x1A);
    break;
  case 7:
    #ifdef Debug_Encuestas
    Serial.println("Games Played"); // Games played
    #endif
    Transmite_Poll(0x15);
    break;
  case 8:
    #ifdef Debug_Encuestas
    Serial.println("Contador 1C - Door Open Metter");
    #endif
    Transmite_Poll(0x1C);
    break;
  case 9:
    #ifdef Debug_Encuestas
    Serial.println("Contador 18 - Games Since Last Power Up");
    #endif
    Transmite_Poll(0x18);
    break;
  case 10:
    #ifdef Debug_Encuestas
    Serial.println("ID Machine");
    #endif
    Transmite_Poll(0x1F);
    Conta_Encuestas = 0;
    flag_ultimo_contador_Ok = true;
    break;
  }
}

void Encuestas_Maquinas_Simple_No_Cancel(void)
{
  Conta_Encuestas++;
  switch (Conta_Encuestas)
  {

  case 1:

    #ifdef Debug_Encuestas
    Serial.println("Cancel Credit Hand Paid"); // Cancel credit hand paid
    #endif
    sendDataa(dat4, sizeof(dat4));             // Transmite DIR
    Transmite_Poll_Long(0x2D);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0xFF);
    Transmite_Poll_Long(0xE0);
    break;
  case 2:
    #ifdef Debug_Encuestas
    Serial.println("Coin In"); // Coin in
    #endif
    Transmite_Poll(0x11);
    break;
  case 3:
    #ifdef Debug_Encuestas
    Serial.println("Coin Out"); // Coin out
    #endif
    Transmite_Poll(0x12);
    break;
  case 4:
    #ifdef Debug_Encuestas
    Serial.println("Jackpot"); // Jackpot
    #endif
    Transmite_Poll(0x14);
    break;
  case 5:
    #ifdef Debug_Encuestas
    Serial.println("Total Drop"); // total drop
    #endif
    Transmite_Poll(0x13);
    break;
  case 6:
    #ifdef Debug_Encuestas
    Serial.println("Current Credits");
    #endif
    Transmite_Poll(0x1A);
    break;
  case 7:
    #ifdef Debug_Encuestas
    Serial.println("Games Played"); // Games played
    #endif
    Transmite_Poll(0x15);
    break;
  case 8:
    #ifdef Debug_Encuestas
    Serial.println("Contador 1C - Door Open Metter");
    #endif
    Transmite_Poll(0x1C);
    break;
  case 9:
    #ifdef Debug_Encuestas
    Serial.println("Contador 18 - Games Since Last Power Up");
    #endif
    Transmite_Poll(0x18);
    break;
  case 10:
    #ifdef Debug_Encuestas
    Serial.println("ID Machine");
    #endif
    Transmite_Poll(0x1F);
    Conta_Encuestas = 0;
    flag_ultimo_contador_Ok = true;
    break;
  }
}
// Encuestas Maquinas Genericas
void Encuestas_Maquinas_Genericas(void)
{
  Conta_Encuestas++;
  switch (Conta_Encuestas)
  {
  case 1:
    #ifdef Debug_Encuestas
    Serial.println("Total Cancel Credit"); // total cancel credit
    #endif
    Transmite_Poll(0x10);
    break;
  case 2:
    #ifdef Debug_Encuestas
    Serial.println("Coin In"); // Coin in
    #endif
    Transmite_Poll(0x11);
    break;
  case 3:
    #ifdef Debug_Encuestas
    Serial.println("Coin Out"); // Coin out
    #endif
    Transmite_Poll(0x12);
    break;
  case 4:
    #ifdef Debug_Encuestas
    Serial.println("Jackpot"); // Jackpot
    #endif
    Transmite_Poll(0x14);
    break;
  case 5:
    #ifdef Debug_Encuestas
    Serial.println("Total Drop"); // total drop
    #endif
    Transmite_Poll(0x13);
    break;
  case 6:
    #ifdef Debug_Encuestas
    Serial.println("Cancel Credit Hand Paid"); // Cancel credit hand paid
    #endif
    sendDataa(dat4, sizeof(dat4));             // Transmite DIR
    Transmite_Poll_Long(0x2D);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0xFF);
    Transmite_Poll_Long(0xE0);
    break;
  case 7:
    #ifdef Debug_Encuestas
    Serial.println("Bill amount");
    #endif
    Transmite_Poll(0x46); // Bill amount
    break;
  case 8:

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
    {
      Transmite_Poll(0x1D);
    }
    else
    {
#ifdef Debug_Encuestas
      Serial.println("Casheable In"); // Casheable in
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x2E);
      Transmite_Poll_Long(0xF7);
      Transmite_Poll_Long(0xE3);
    }

    break;
  case 9:
    #ifdef Debug_Encuestas
    Serial.println("Casheable Restricted In"); // Casheable restricted in
    #endif
    sendDataa(dat4, sizeof(dat4));             // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x7E);
    Transmite_Poll_Long(0xF2);
    break;
  case 10:
    #ifdef Debug_Encuestas
    Serial.println("Casheable Nonrestricted In"); // Casheable Nonrestricted in
    #endif
    sendDataa(dat4, sizeof(dat4));                // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x30);
    Transmite_Poll_Long(0x08);
    Transmite_Poll_Long(0x1A);
    break;
  case 11:

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
    {
      Transmite_Poll(0x1D);
    }
    else
    {
#ifdef Debug_Encuestas
      Serial.println("Casheable Out"); // Casheable out
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x32);
      Transmite_Poll_Long(0x1A);
      Transmite_Poll_Long(0x39);
    }

    break;
  case 12:
    #ifdef Debug_Encuestas
    Serial.println("Casheable Restricted Out"); // Casheable restricted out
    #endif
    sendDataa(dat4, sizeof(dat4));              // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x33);
    Transmite_Poll_Long(0x93);
    Transmite_Poll_Long(0x28);
    break;
  case 13:
    #ifdef Debug_Encuestas
    Serial.println("Casheable Nonrestricted Out"); // Casheable nonrestricted out
    #endif
    sendDataa(dat4, sizeof(dat4));                 // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x34);
    Transmite_Poll_Long(0x2C);
    Transmite_Poll_Long(0x5C);
    break;
  case 14:
    #ifdef Debug_Encuestas
    Serial.println("Games Played"); // Games played
    #endif
    Transmite_Poll(0x15);
    break;
  case 15:
    #ifdef Debug_Encuestas
    Serial.println("Coin In Fisico"); // Physical coin in
    #endif
    Transmite_Poll(0x2A);
    break;
  case 16:
    #ifdef Debug_Encuestas
    Serial.println("Coin Out Fisico"); // Physical coin out
    #endif
    Transmite_Poll(0x2B);
    break;
  case 17:
    #ifdef Debug_Encuestas
    Serial.println("Total Coin Drop");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x24);
    Transmite_Poll_Long(0xAD);
    Transmite_Poll_Long(0x4C);
    break;
  case 18:
    #ifdef Debug_Encuestas
    Serial.println("Machine Paid Progresive Payout");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x1D);
    Transmite_Poll_Long(0xEF);
    Transmite_Poll_Long(0xE0);
    break;
  case 19:
    #ifdef Debug_Encuestas
    Serial.println("Machine Paid External Bonus Payout");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x1E);
    Transmite_Poll_Long(0x74);
    Transmite_Poll_Long(0xD2);
    break;
  case 20:
    #ifdef Debug_Encuestas
    Serial.println("Attendant Paid Progresive Payout");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x20);
    Transmite_Poll_Long(0x89);
    Transmite_Poll_Long(0x0A);
    break;
  case 21:
    #ifdef Debug_Encuestas
    Serial.println("Attendant Paid External Payout");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x21);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x1B);
    break;
  case 22:
    #ifdef Debug_Encuestas
    Serial.println("Ticket In");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x0D);
    Transmite_Poll_Long(0x6E);
    Transmite_Poll_Long(0xF0);
    break;
  case 23:
    #ifdef Debug_Encuestas
    Serial.println("Ticket Out");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x0E);
    Transmite_Poll_Long(0xF5);
    Transmite_Poll_Long(0xC2);
    break;
  case 24:
    #ifdef Debug_Encuestas
    Serial.println("Current Credits");
    #endif
    Transmite_Poll(0x1A);
    break;
  case 25:
    #ifdef Debug_Encuestas
    Serial.println("Contador 1C - Door Open Metter");
    #endif
    Transmite_Poll(0x1C);
    break;
  case 26:
    #ifdef Debug_Encuestas
    Serial.println("Contador 18 - Games Since Last Power Up");
    #endif
    Transmite_Poll(0x18);
    break;
  case 27:
    #ifdef Debug_Encuestas
    Serial.println("ID Machine");
    #endif
    Transmite_Poll(0x1F);
    break;
  case 28:
    #ifdef Debug_Encuestas
    Serial.println("ROM Signature");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x21);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x5C);
    Transmite_Poll_Long(0x45);
    break;
  case 29:
    Encuesta_Billetes();
    Conta_Encuestas = 0;
    flag_ultimo_contador_Ok = true;
  }
}

/* Australiana*/
void Encuestas_Aristocrat_Australiana(void)
{
  Conta_Encuestas++;
  switch (Conta_Encuestas)
  {
  case 1:
    #ifdef Debug_Encuestas
    Serial.println("Total Cancel Credit"); // total cancel credit
    #endif
    Transmite_Poll(0x10);
    break;
  case 3:
    #ifdef Debug_Encuestas
    Serial.println("Coin In"); // Coin in
    #endif
    Transmite_Poll(0x11);
    break;
  case 5:
    #ifdef Debug_Encuestas
    Serial.println("Coin Out"); // Coin out
    #endif
    Transmite_Poll(0x12);
    break;
  case 7:
    #ifdef Debug_Encuestas
    Serial.println("Coin In Fisico"); // Physical coin in
    #endif
    Transmite_Poll(0x2A);
    break;
  case 9:
    #ifdef Debug_Encuestas
    Serial.println("Coin Out Fisico"); // Physical coin out
    #endif
    Transmite_Poll(0x2B);
    break;
  case 13:
    #ifdef Debug_Encuestas
    Serial.println("Contador 1C - Door Open Metter");
    #endif
    Transmite_Poll(0x1C);
    break;
  case 15:
    #ifdef Debug_Encuestas
    Serial.println("Contador 18 - Games Since Last Power Up");
    #endif
    Transmite_Poll(0x18);
    break;
  case 17:
    #ifdef Debug_Encuestas
    Serial.println("ID Machine");
    #endif
    Transmite_Poll(0x1F);
    break;
  case 19:
    #ifdef Debug_Encuestas
    Serial.println("ROM Signature");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x21);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x5C);
    Transmite_Poll_Long(0x45);
    Conta_Encuestas = 0;
    flag_ultimo_contador_Ok = true;
    break;
  }
}

// Encuestas Maquinas Poker
void Encuestas_Maquinas_Poker(void)
{
  Conta_Encuestas++;
  switch (Conta_Encuestas)
  {
  case 1:
    #ifdef Debug_Encuestas
    Serial.println("Coin In"); // Coin in
    #endif
    Transmite_Poll(0x11);
    break;
  case 2:
    #ifdef Debug_Encuestas
    Serial.println("Coin Out"); // Coin out
    #endif
    Transmite_Poll(0x12);
    break;
  case 3:
    #ifdef Debug_Encuestas
    Serial.println("Jackpot"); // Jackpot
    #endif
    Transmite_Poll(0x14);
    break;
  case 4:
    #ifdef Debug_Encuestas
    Serial.println("Total Drop"); // total drop
    #endif
    Transmite_Poll(0x13);
    break;
  case 5:
    #ifdef Debug_Encuestas
    Serial.println("Games Played"); // Games played
    #endif
    Transmite_Poll(0x15);
    break;
  case 6:
    #ifdef Debug_Encuestas
    Serial.println("Current Credits");
    #endif
    Transmite_Poll(0x1A);
    break;
  case 7:
    #ifdef Debug_Encuestas
    Serial.println("Contador 1C - Door Open Metter");
    #endif
    Transmite_Poll(0x1C);
    break;
  case 8:
    #ifdef Debug_Encuestas
    Serial.println("ID Machine");
    #endif
    Transmite_Poll(0x1F);
    break;
  case 9:
#ifdef Debug_Encuestas
    Serial.println("ROM Signature");
#endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x21);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x5C);
    Transmite_Poll_Long(0x45);
    Conta_Encuestas = 0;
    flag_ultimo_contador_Ok = true;
    break;
  }
}

/* Encuestas Maquinas Poker Ertech Slot */
void Encuestas_Maquinas_Poker_Ertech_Slot(void)
{
  Conta_Encuestas++;
  switch (Conta_Encuestas)
  {
  case 1:
    #ifdef Debug_Encuestas
    Serial.println("Coin In"); // Coin in
    #endif
    Transmite_Poll(0x11);
    break;
  case 2:
    #ifdef Debug_Encuestas
    Serial.println("Coin Out"); // Coin out
    #endif
    Transmite_Poll(0x12);
    break;
  case 3:
    #ifdef Debug_Encuestas
    Serial.println("Jackpot"); // Jackpot
    #endif
    Transmite_Poll(0x14);
    break;
  case 4:
    #ifdef Debug_Encuestas
    Serial.println("Total Drop"); // total drop
    #endif
    Transmite_Poll(0x13);
    break;
  case 5:
    #ifdef Debug_Encuestas
    Serial.println("Games Played"); // Games played
    #endif
    Transmite_Poll(0x15);
    break;
  case 6:
    #ifdef Debug_Encuestas
    Serial.println("Current Credits");
    #endif
    Transmite_Poll(0x1A);
    break;
  case 7:
    #ifdef Debug_Encuestas
    Serial.println("Contador 1C - Door Open Metter");
    #endif
    Transmite_Poll(0x1C);
    break;
  case 8:
    #ifdef Debug_Encuestas
    Serial.println("ID Machine");
    #endif
    Transmite_Poll(0x1F);
    break;
  case 9:
#ifdef Debug_Encuestas
    Serial.println("ROM Signature");
#endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x21);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x5C);
    Transmite_Poll_Long(0x45);
    Conta_Encuestas = 0;
    flag_ultimo_contador_Ok = true;
    break;
  }
}

void Encuestas_Maquinas_Ruleta_IRT(void)
{
  Conta_Encuestas++;
  switch (Conta_Encuestas)
  {
  case 1:
    #ifdef Debug_Encuestas
    Serial.println("Total Cancel Credit"); // total cancel credit
    #endif
    Transmite_Poll(0x0F);
    break;
  case 5:
    #ifdef Debug_Encuestas
    Serial.println("Coin In"); // Coin in
    #endif
    Transmite_Poll(0x2B);
    break;
  case 10:
    #ifdef Debug_Encuestas
    Serial.println("Coin Out"); // Coin out
    #endif
    Transmite_Poll(0x46);
    break;
  case 15:
    #ifdef Debug_Encuestas
    Serial.println("Jackpot"); // Jackpot
    #endif
    Transmite_Poll(0x1A);
    break;
  case 20:
    #ifdef Debug_Encuestas
    Serial.println("Total Drop"); // total drop
    #endif
    Transmite_Poll(0x1C);
    break;
  case 25:
    #ifdef Debug_Encuestas
    Serial.println("ID Machine");
    #endif
    Transmite_Poll(0x1F);
    Conta_Encuestas = 0;
    flag_ultimo_contador_Ok = true;
    break;
  }
}


// Encuestas Maquinas IRT
void Encuestas_Maquinas_IRT(void)
{
  Conta_Encuestas++;
  switch (Conta_Encuestas)
  {
  case 1:
    #ifdef Debug_Encuestas
    Serial.println("Total Cancel Credit"); // total cancel credit
    #endif
    Transmite_Poll(0x10);
    break;
  case 3:
    #ifdef Debug_Encuestas
    Serial.println("Coin In"); // Coin in
    #endif
    Transmite_Poll(0x11);
    break;
  case 6:
    #ifdef Debug_Encuestas
    Serial.println("Coin Out"); // Coin out
    #endif
    Transmite_Poll(0x12);
    break;
  case 9:
    #ifdef Debug_Encuestas
    Serial.println("Jackpot"); // Jackpot
    #endif
    Transmite_Poll(0x14);
    break;
  case 12:
    #ifdef Debug_Encuestas
    Serial.println("Total Drop"); // total drop
    #endif
    Transmite_Poll(0x13);
    break;
  case 15:
    #ifdef Debug_Encuestas
    Serial.println("Games Played"); // Games played
    #endif
    Transmite_Poll(0x15);
    break;
  case 18:
    #ifdef Debug_Encuestas
    Serial.println("Coin In Fisico"); // Physical coin in
    #endif
    Transmite_Poll(0x2A);
    break;
  case 21:
    #ifdef Debug_Encuestas
    Serial.println("Coin Out Fisico"); // Physical coin out
    #endif
    Transmite_Poll(0x2B);
    break;
  case 24:
    #ifdef Debug_Encuestas
    Serial.println("Current Credits");
    #endif
    Transmite_Poll(0x1A);
    break;
  case 27:
    #ifdef Debug_Encuestas
    Serial.println("Contador 1C - Door Open Metter");
    #endif
    Transmite_Poll(0x1C);
    break;
  case 30:
    #ifdef Debug_Encuestas
    Serial.println("Contador 18 - Games Since Last Power Up");
    #endif
    Transmite_Poll(0x18);
    break;
  case 33:
    #ifdef Debug_Encuestas
    Serial.println("ID Machine");
    #endif
    Transmite_Poll(0x1F);
    break;

  case 35:
    Transmite_Poll(0x46);
  break;
  case 38:
    #ifdef Debug_Encuestas
    Serial.println("ROM Signature");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x21);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x5C);
    Transmite_Poll_Long(0x45);
    break;
  case 42:
    #ifdef Debug_Encuestas
    Serial.println("Encuesta Billetes");
    #endif
   // Encuesta_Billetes();
    Conta_Encuestas = 0;
    flag_ultimo_contador_Ok = true;
    break;
  }
}

// Encuestas Maquinas EFT - 550
void Encuestas_Maquinas_EFT(void)
{
  Conta_Encuestas++;
  switch (Conta_Encuestas)
  {
  case 1:
    #ifdef Debug_Encuestas
    Serial.println("Total Cancel Credit"); // total cancel credit
    #endif
    Transmite_Poll(0x10);
    break;
  case 2:
    #ifdef Debug_Encuestas
    Serial.println("Coin In"); // Coin in
    #endif
    Transmite_Poll(0x11);
    break;
  case 3:
    #ifdef Debug_Encuestas
    Serial.println("Coin Out"); // Coin out
    #endif
    Transmite_Poll(0x12);
    break;
  case 4:
    #ifdef Debug_Encuestas
    Serial.println("Jackpot"); // Jackpot
    #endif
    Transmite_Poll(0x14);
    break;
  case 5:
    #ifdef Debug_Encuestas
    Serial.println("Total Drop"); // total drop
    #endif
    Transmite_Poll(0x13);
    break;
  case 6:
    #ifdef Debug_Encuestas
    Serial.println("Cancel Credit Hand Paid"); // Cancel credit hand paid
    #endif
    sendDataa(dat4, sizeof(dat4));             // Transmite DIR
    Transmite_Poll_Long(0x2D);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0xFF);
    Transmite_Poll_Long(0xE0);
    break;
  case 7:
    #ifdef Debug_Encuestas
    Serial.println("Bill amount");
    #endif
    Transmite_Poll(0x46); // Bill amount
    break;
  case 8:
    #ifdef Debug_Encuestas
    Serial.println("Games Played"); // Games played
    #endif
    Transmite_Poll(0x15);
    break;
  case 9:
    #ifdef Debug_Encuestas
    Serial.println("Coin In Fisico"); // Physical coin in
    #endif
    Transmite_Poll(0x2A);
    break;
  case 10:
    #ifdef Debug_Encuestas
    Serial.println("Coin Out Fisico"); // Physical coin out
    #endif
    Transmite_Poll(0x2B);
    break;
  case 11:
    #ifdef Debug_Encuestas
    Serial.println("Total Coin Drop");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x24);
    Transmite_Poll_Long(0xAD);
    Transmite_Poll_Long(0x4C);
    break;
  case 12:
    #ifdef Debug_Encuestas
    Serial.println("Machine Paid Progresive Payout");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x1D);
    Transmite_Poll_Long(0xEF);
    Transmite_Poll_Long(0xE0);
    break;
  case 13:
    #ifdef Debug_Encuestas
    Serial.println("Machine Paid External Bonus Payout");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x1E);
    Transmite_Poll_Long(0x74);
    Transmite_Poll_Long(0xD2);
    break;
  case 14:
    #ifdef Debug_Encuestas
    Serial.println("Attendant Paid Progresive Payout");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x20);
    Transmite_Poll_Long(0x89);
    Transmite_Poll_Long(0x0A);
    break;
  case 15:
    #ifdef Debug_Encuestas
    Serial.println("Attendant Paid External Payout");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x21);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x1B);
    break;
  case 16:
    #ifdef Debug_Encuestas
    Serial.println("Ticket In");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x0D);
    Transmite_Poll_Long(0x6E);
    Transmite_Poll_Long(0xF0);
    break;
  case 17:
    #ifdef Debug_Encuestas
    Serial.println("Ticket Out");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x0E);
    Transmite_Poll_Long(0xF5);
    Transmite_Poll_Long(0xC2);
    break;
  case 18:
    #ifdef Debug_Encuestas
    Serial.println("Current Credits");
    #endif
    Transmite_Poll(0x1A);
    break;

  case 19:
  Transmite_Poll(0x1D);
  break;

  case 20:
    #ifdef Debug_Encuestas
    Serial.println("Contador 1C - Door Open Metter");
    #endif
    Transmite_Poll(0x1C);
    break;
  case 21:
    #ifdef Debug_Encuestas
    Serial.println("Contador 18 - Games Since Last Power Up");
    #endif
    Transmite_Poll(0x18);
    break;
  case 22:
    #ifdef Debug_Encuestas
    Serial.println("ID Machine");
    #endif
    Transmite_Poll(0x1F);
    break;
  case 23:
    #ifdef Debug_Encuestas
    Serial.println("ROM Signature");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x21);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x5C);
    Transmite_Poll_Long(0x45);
    break;
  case 24:
    Encuesta_Billetes();
    Conta_Encuestas = 0;
    flag_ultimo_contador_Ok = true;
  }
}

// Encuestas Maquinas IGT Riel
void Encuestas_Maquinas_IGT_Riel(void)
{
  Conta_Encuestas++;
  switch (Conta_Encuestas)
  {
  case 1:
    #ifdef Debug_Encuestas
    Serial.println("Coin In"); // Coin in
    #endif
    Transmite_Poll(0x11);
    break;
  case 2:
    #ifdef Debug_Encuestas
    Serial.println("Coin Out"); // Coin out
     #endif
    Transmite_Poll(0x12);
    break;
  case 3:
    #ifdef Debug_Encuestas
    Serial.println("Jackpot"); // Jackpot
    #endif
    Transmite_Poll(0x14);
    break;
  case 4:
    #ifdef Debug_Encuestas
    Serial.println("Games Played"); // Games played
    #endif
    Transmite_Poll(0x15);
    break;
  case 5:
    #ifdef Debug_Encuestas
    Serial.println("Contador 1C - Door Open Metter");
    #endif
    Transmite_Poll(0x1C);
    break;
  case 6:
    #ifdef Debug_Encuestas
    Serial.println("ID Machine");
    #endif
    Transmite_Poll(0x1F);
    Conta_Encuestas = 0;
    flag_ultimo_contador_Ok = true;
    break;
  }
}

// Encuestas Maquinas IGT_Riel_Bill
void Encuestas_Maquinas_IGT_Riel_Bill(void)
{
  Conta_Encuestas++;
  switch (Conta_Encuestas)
  {
  case 1:
    #ifdef Debug_Encuestas
    Serial.println("Coin In"); // Coin in
    #endif
    Transmite_Poll(0x11);
    break;
  case 2:
    #ifdef Debug_Encuestas
    Serial.println("Coin Out"); // Coin out
    #endif
    Transmite_Poll(0x12);
    break;
  case 3:
    #ifdef Debug_Encuestas
    Serial.println("Jackpot"); // Jackpot
    #endif
    Transmite_Poll(0x14);
    break;
  case 4:
    #ifdef Debug_Encuestas
    Serial.println("Total Drop"); // total drop
    #endif
    Transmite_Poll(0x13);
    break;
  case 5:
    #ifdef Debug_Encuestas
    Serial.println("Games Played"); // Games played
    #endif
    Transmite_Poll(0x15);
    break;
  case 6:
    #ifdef Debug_Encuestas
    Serial.println("Contador 1C - Door Open Metter");
    #endif
    Transmite_Poll(0x1C);
    break;
  case 7:
    #ifdef Debug_Encuestas
    Serial.println("ID Machine");
    #endif
    Transmite_Poll(0x1F);
    Conta_Encuestas = 0;
    flag_ultimo_contador_Ok = true;
    break;
  }
}

// Encuestas Mecanicas
void Encuestas_Mecanicas(void)
{

  if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 9)
  {
    // Serial.println("Transmite solicitud de contadores Mecanicas");
    const char dato1[] = {0x01};
    const char dato2[] = {0x00};
    const char dato3[] = {0x38};
    sendDataa(dato1, sizeof(dato1)); // 0
    sendDataa(dato2, sizeof(dato2)); // 1
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2)); // 10
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2)); // 20
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2)); // 30
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato3, sizeof(dato3)); // 34
    flag_ultimo_contador_Ok = true;
   // Counter_Final=true;
  }
  else if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 15)
  {
    // Serial.println("Transmite solicitud de contadores Mecanicas");
    const char dato1[] = {0x01};
    const char dato2[] = {0x00};
    const char dato3[] = {0x38};
    sendDataa(dato1, sizeof(dato1)); // 0
    sendDataa(dato2, sizeof(dato2)); // 1
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2)); // 10
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2)); // 20
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2)); // 30
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato2, sizeof(dato2));
    sendDataa(dato3, sizeof(dato3)); // 50
    flag_ultimo_contador_Ok = true;
    Counter_Final=true;
  }
}
//---------------------------- Funcion para encuestar Billetes  ----------------------------------------------

void Encuesta_Billetes(void)
{
  Conta_Poll_Billetes++;

  switch (Conta_Poll_Billetes)
  {
  case 1:
    //  Serial.println("Contador Billetes 2K");
    Transmite_Poll(0x3C);
    break;
  case 2:
    // Serial.println("Contador Billetes 5K");
    Transmite_Poll(0x3F);
    break;
  case 3:
    // Serial.println("Contador Billetes 10K");
    Transmite_Poll(0x40);
    break;
  case 4:
    // Serial.println("Contador Billetes 20K");
    Transmite_Poll(0x41);
    break;
  case 5:
    //  Serial.println("Contador Billetes 50K");
    Transmite_Poll(0x43);
    Conta_Poll_Billetes = 0;
    break;

  default:
    break;
  }
}

//---------------------------- Tarea Para Activar e inactivar maquina -----------------------------------------------

bool Inactiva_Maquina(void)
{
  flag_handle_maquina = true;
  Handle_Maquina = flag_bloquea_Maquina;
  delay(600);
  if (ACK_Maq)
  {
    ACK_Maq = false;
    return true;
  }
  else
  {
    return false;
  }
}

bool Actualiza_Maquina_En_Juego(void)
{
  flag_handle_maquina = true;
  Handle_Maquina = Flag_Encuesta_Maquina_Juego;
  delay(600);
  if (ACK_Maq)
  {
    ACK_Maq = false;
    return true;
  }
  else
  {
    return false;
  }
}

void Transmite_Inactiva_Maquina(void)
{
  sendDataa(dat4, sizeof(dat4)); // Transmite DIR
  Transmite_Poll_Long(0x01);
  Transmite_Poll_Long(0x51);
  Transmite_Poll_Long(0x08);
}

bool Activa_Maquina(void)
{
  flag_handle_maquina = true;
  Handle_Maquina = flag_desbloquea_Maquina;
  delay(600);
  if (ACK_Maq)
  {
    ACK_Maq = false;
    return true;
  }
  else
  {
    return false;
  }
}

bool Encuesta_ROM(void)
{
  flag_handle_maquina = true;
  Handle_Maquina = Flag_Encuesta_ROM;
  delay(600);

  if (ACK_Maq)
  {
    ACK_Maq = false;
    return true;
  }
  else
  {
    return false;
  }
}

void Transmite_Encuesta_ROM(void)
{
  #ifdef Debug_Encuestas
    Serial.println("ROM Signature");
  #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x21);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x5C);
    Transmite_Poll_Long(0x45);
    
}

bool  Encuesta_Creditos_Premio(void)
{
  flag_handle_maquina = true;
  Handle_Maquina = Flag_Creditos_Premio;
  delay(600);

  if (ACK_Maq)
  {
    ACK_Maq = false;
    return true;
  }
  else
  {
    return false;
  }
}

void _Transmite_Encuesta_Creditos_D_Premio(void)
{

  if(Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion==1)&& Configuracion.Get_Configuracion(Tipo_Maquina,0)==2)
  {
    #ifdef Debug_Encuestas
    Serial.println("Total Cancel Credit"); // total cancel credit
    #endif
    Transmite_Poll(0x10);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);

    #ifdef Debug_Encuestas
    Serial.println("Coin In"); // Coin in
    #endif
    Transmite_Poll(0x11);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);

    #ifdef Debug_Encuestas
    Serial.println("Coin Out"); // Coin out
    #endif
    Transmite_Poll(0x12);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    #ifdef Debug_Encuestas
    Serial.println("Jackpot"); // Jackpot
    #endif
    Transmite_Poll(0x14);
    
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    #ifdef Debug_Encuestas
    Serial.println("Total Drop"); // total drop
    #endif
    Transmite_Poll(0x13);
   
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    #ifdef Debug_Encuestas
    Serial.println("Cancel Credit Hand Paid"); // Cancel credit hand paid
    #endif
    sendDataa(dat4, sizeof(dat4));             // Transmite DIR
    Transmite_Poll_Long(0x2D);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0xFF);
    Transmite_Poll_Long(0xE0);
    
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
  
    #ifdef Debug_Encuestas
    Serial.println("Bill amount");
    #endif
    Transmite_Poll(0x46); // Bill amount
   
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);

    #ifdef Debug_Encuestas
    Serial.println("Games Played"); // Games played
    #endif
    Transmite_Poll(0x15);
    
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    #ifdef Debug_Encuestas
    Serial.println("Coin In Fisico"); // Physical coin in
    #endif
    Transmite_Poll(0x2A);
    #ifdef Debug_Encuestas
    Serial.println("Coin Out Fisico"); // Physical coin out
    #endif
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Transmite_Poll(0x2B);
   

    #ifdef Debug_Encuestas
    Serial.println("Total Coin Drop");
    #endif
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x24);
    Transmite_Poll_Long(0xAD);
    Transmite_Poll_Long(0x4C);
   
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    #ifdef Debug_Encuestas
    Serial.println("Machine Paid Progresive Payout");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x1D);
    Transmite_Poll_Long(0xEF);
    Transmite_Poll_Long(0xE0);
    
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    #ifdef Debug_Encuestas
    Serial.println("Machine Paid External Bonus Payout");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x1E);
    Transmite_Poll_Long(0x74);
    Transmite_Poll_Long(0xD2);
   
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    #ifdef Debug_Encuestas
    Serial.println("Attendant Paid Progresive Payout");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x20);
    Transmite_Poll_Long(0x89);
    Transmite_Poll_Long(0x0A);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
 
    #ifdef Debug_Encuestas
    Serial.println("Attendant Paid External Payout");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x21);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x1B);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
 
    #ifdef Debug_Encuestas
    Serial.println("Ticket In");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x0D);
    Transmite_Poll_Long(0x6E);
    Transmite_Poll_Long(0xF0);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    delay(200);
    #ifdef Debug_Encuestas
    Serial.println("Ticket Out");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x0E);
    Transmite_Poll_Long(0xF5);
    Transmite_Poll_Long(0xC2);
   
    #ifdef Debug_Encuestas
    Serial.println("Current Credits");
    #endif

    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Transmite_Poll(0x1A);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    #ifdef Debug_Encuestas
    Serial.println("Contador 1C - Door Open Metter");
    #endif
    Transmite_Poll(0x1C);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    #ifdef Debug_Encuestas
    Serial.println("Contador 18 - Games Since Last Power Up");
    #endif
    Transmite_Poll(0x18);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
  
    #ifdef Debug_Encuestas
    Serial.println("ID Machine");
    #endif
    Transmite_Poll(0x1F);
   delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    #ifdef Debug_Encuestas
    Serial.println("ROM Signature");
    #endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x21);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x5C);
    Transmite_Poll_Long(0x45);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Transmite_Poll(0x1D);
    Counter_Final=true;
  }

  if(Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) == 2&& Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6)
  {
    #ifdef Debug_Encuestas
      Serial.println("Current Credits");
    #endif
    
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);

      Transmite_Poll(0x1A);
      delay(100);
  }

  if(Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) == 2&& Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 14)
  {
    #ifdef Debug_Encuestas
      Serial.println("Current Credits");
    #endif
    
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);

      Transmite_Poll(0x1A);
      delay(100);
  }

  if (Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) == 1 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 14)
  {
    /* Actualiza Mq Poker*/

    #ifdef Debug_Encuestas     
    Serial.println("Encuesta Completa Poker.......");
    #endif
    Transmite_Poll(0x1A);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Transmite_Poll(0x11);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Transmite_Poll(0x12);
    delay(100);
     sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Transmite_Poll(0x14);
    delay(100);
    Transmite_Poll(0x13);
    delay(100);
     sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);

    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x24);
    Transmite_Poll_Long(0xAD);
    Transmite_Poll_Long(0x4C);
    delay(100);
    Transmite_Poll(0x1F);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Counter_Final=true;
  }

  if (Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) == 1 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6)
  {
    /* Actualiza Mq Poker*/

    #ifdef Debug_Encuestas     
    Serial.println("Encuesta Completa Poker.......");
    #endif
    Transmite_Poll(0x1A);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Transmite_Poll(0x11);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Transmite_Poll(0x12);
    delay(100);
     sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Transmite_Poll(0x14);
    delay(100);
    Transmite_Poll(0x13);
    delay(100);
     sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);

    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x24);
    Transmite_Poll_Long(0xAD);
    Transmite_Poll_Long(0x4C);
    delay(100);
    Transmite_Poll(0x1F);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Counter_Final=true;
  }

  if (Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) == 0 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6)
  {
    /* Actualiza Mq Poker*/
    #ifdef Debug_Encuestas 
    Serial.println("Encuesta Completa Poker.......");
    #endif
    Transmite_Poll(0x1A);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Transmite_Poll(0x11);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Transmite_Poll(0x12);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Transmite_Poll(0x14);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Transmite_Poll(0x13);
    delay(100);

    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x24);
    Transmite_Poll_Long(0xAD);
    Transmite_Poll_Long(0x4C);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Counter_Final=true;
  }

  if (Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) == 0 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 14)
  {
    /* Actualiza Mq Poker*/
    #ifdef Debug_Encuestas 
    Serial.println("Encuesta Completa Poker.......");
    #endif
    Transmite_Poll(0x1A);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Transmite_Poll(0x11);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Transmite_Poll(0x12);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Transmite_Poll(0x14);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Transmite_Poll(0x13);
    delay(100);

    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x24);
    Transmite_Poll_Long(0xAD);
    Transmite_Poll_Long(0x4C);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    Counter_Final=true;
  }

  if(Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) == 3 &&Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 5)
  {
    Transmite_Poll(0x12);
    delay(100);
  }

  if (Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) == 0)
  {
    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 5 ||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2|| Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 0||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 1||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 3||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
    {
      /*Actualiza Creditos Generica*/
      #ifdef Debug_Encuestas
      Serial.println("Encuesta Creditos Generica....... ");
      #endif

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      delay(200);
      Transmite_Poll(0x1A);
      delay(200);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
      Transmite_Poll(0x10);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
      sendDataa(dat4, sizeof(dat4));             // Transmite DIR
      Transmite_Poll_Long(0x2D);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0xFF);
      Transmite_Poll_Long(0xE0);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
      Transmite_Poll(0x11);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
      Transmite_Poll(0x12);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
      Transmite_Poll(0x13);

      delay(100);

      if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2)
      {
        sendDataa(dat, sizeof(dat)); // transmite sincronización
        delay(200);
        Transmite_Poll(0x1D);
      }
    }
  }

  if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 12)
  {
    if (Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) >= 0)
    {
#ifdef Debug_Encuestas
      Serial.println("Encuesta Simple no Cancel.......");
#endif

#ifdef Debug_Encuestas
      Serial.println("Cancel Credit Hand Paid"); // Cancel credit hand paid
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2D);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0xFF);
      Transmite_Poll_Long(0xE0);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x11);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Coin Out"); // Coin out
#endif
      Transmite_Poll(0x12);
      delay(100);
       sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Jackpot"); // Jackpot
#endif
      Transmite_Poll(0x14);
      delay(100);
#ifdef Debug_Encuestas
      Serial.println("Total Drop"); // total drop
#endif
      Transmite_Poll(0x13);
      delay(100);
#ifdef Debug_Encuestas
      Serial.println("Current Credits");
#endif
      Transmite_Poll(0x1A);
      delay(100);
#ifdef Debug_Encuestas
      Serial.println("Games Played"); // Games played
#endif
      Transmite_Poll(0x15);
      delay(100);

#ifdef Debug_Encuestas
      Serial.println("Contador 1C - Door Open Metter");
#endif
      Transmite_Poll(0x1C);
      delay(100);

#ifdef Debug_Encuestas
      Serial.println("Contador 18 - Games Since Last Power Up");
#endif
      Transmite_Poll(0x18);
      delay(100);

#ifdef Debug_Encuestas
      Serial.println("ID Machine");
#endif
      Transmite_Poll(0x1F);
      delay(100);
      Counter_Final = true;
    }
  }
  if (Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) == 1 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 5||Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) == 1 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 0||Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) == 1 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 1||Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) == 1 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 3|| Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) == 1 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
  {
    /*Actualiza  Completa Mq Generica*/
#ifdef Debug_Encuestas
    Serial.println("Encuesta Completa.......");
#endif
#ifdef Debug_Encuestas
      Serial.println("Total Cancel Credit"); // total cancel credit
#endif

      #ifdef Debug_Encuestas
      Serial.println("ROM Signature");
      #endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x21);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x5C);
      Transmite_Poll_Long(0x45);
      delay(100);

      Transmite_Poll(0x10);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

#ifdef Debug_Encuestas
      Serial.println("Coin In"); // Coin in
#endif
      Transmite_Poll(0x11);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

#ifdef Debug_Encuestas
      Serial.println("Coin Out"); // Coin out
#endif
      Transmite_Poll(0x12);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Jackpot"); // Jackpot
#endif
      Transmite_Poll(0x14);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Total Drop"); // total drop
#endif
      Transmite_Poll(0x13);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

#ifdef Debug_Encuestas
      Serial.println("Cancel Credit Hand Paid"); // Cancel credit hand paid
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2D);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0xFF);
      Transmite_Poll_Long(0xE0);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Bill amount");
#endif
      Transmite_Poll(0x46); // Bill amount
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Casheable In"); // Casheable in
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x2E);
      Transmite_Poll_Long(0xF7);
      Transmite_Poll_Long(0xE3);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Casheable Restricted In"); // Casheable restricted in
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x7E);
      Transmite_Poll_Long(0xF2);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Casheable Nonrestricted In"); // Casheable Nonrestricted in
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x30);
      Transmite_Poll_Long(0x08);
      Transmite_Poll_Long(0x1A);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Casheable Out"); // Casheable out
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x32);
      Transmite_Poll_Long(0x1A);
      Transmite_Poll_Long(0x39);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Casheable Restricted Out"); // Casheable restricted out
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x33);
      Transmite_Poll_Long(0x93);
      Transmite_Poll_Long(0x28);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Casheable Nonrestricted Out"); // Casheable nonrestricted out
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x34);
      Transmite_Poll_Long(0x2C);
      Transmite_Poll_Long(0x5C);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Games Played"); // Games played
#endif
      Transmite_Poll(0x15);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Coin In Fisico"); // Physical coin in
#endif
      Transmite_Poll(0x2A);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Coin Out Fisico"); // Physical coin out
#endif
      Transmite_Poll(0x2B);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Total Coin Drop");
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x24);
      Transmite_Poll_Long(0xAD);
      Transmite_Poll_Long(0x4C);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Machine Paid Progresive Payout");
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x1D);
      Transmite_Poll_Long(0xEF);
      Transmite_Poll_Long(0xE0);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Machine Paid External Bonus Payout");
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x1E);
      Transmite_Poll_Long(0x74);
      Transmite_Poll_Long(0xD2);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Attendant Paid Progresive Payout");
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x20);
      Transmite_Poll_Long(0x89);
      Transmite_Poll_Long(0x0A);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Attendant Paid External Payout");
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x21);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x1B);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Ticket In");
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x0D);
      Transmite_Poll_Long(0x6E);
      Transmite_Poll_Long(0xF0);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Ticket Out");
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x0E);
      Transmite_Poll_Long(0xF5);
      Transmite_Poll_Long(0xC2);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Current Credits");
#endif
      Transmite_Poll(0x1A);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Contador 1C - Door Open Metter");
#endif
      Transmite_Poll(0x1C);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Contador 18 - Games Since Last Power Up");
#endif
      Transmite_Poll(0x18);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("ID Machine");
#endif
      Transmite_Poll(0x1F);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("ROM Signature");
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x21);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x5C);
      Transmite_Poll_Long(0x45);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2)
      {
        sendDataa(dat, sizeof(dat)); // transmite sincronización
        delay(100);
        Transmite_Poll(0x1D);
      }
      Counter_Final=true;
  }

  if (Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) == 1 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2)
  {
    /*Actualiza  Completa Mq Generica*/
#ifdef Debug_Encuestas
    Serial.println("Encuesta Completa.......");
#endif
#ifdef Debug_Encuestas
      Serial.println("Total Cancel Credit"); // total cancel credit
#endif

      #ifdef Debug_Encuestas
      Serial.println("ROM Signature");
      #endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x21);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x5C);
      Transmite_Poll_Long(0x45);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x10);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

#ifdef Debug_Encuestas
      Serial.println("Coin In"); // Coin in
#endif
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x11);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

#ifdef Debug_Encuestas
      Serial.println("Coin Out"); // Coin out
#endif
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x12);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Jackpot"); // Jackpot
#endif
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x14);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Total Drop"); // total drop
#endif

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x13);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

#ifdef Debug_Encuestas
      Serial.println("Cancel Credit Hand Paid"); // Cancel credit hand paid
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2D);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0xFF);
      Transmite_Poll_Long(0xE0);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Bill amount");
#endif

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x46); // Bill amount
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Casheable In"); // Casheable in
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x2E);
      Transmite_Poll_Long(0xF7);
      Transmite_Poll_Long(0xE3);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Casheable Restricted In"); // Casheable restricted in
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x7E);
      Transmite_Poll_Long(0xF2);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Casheable Nonrestricted In"); // Casheable Nonrestricted in
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x30);
      Transmite_Poll_Long(0x08);
      Transmite_Poll_Long(0x1A);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Casheable Out"); // Casheable out
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x32);
      Transmite_Poll_Long(0x1A);
      Transmite_Poll_Long(0x39);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Casheable Restricted Out"); // Casheable restricted out
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x33);
      Transmite_Poll_Long(0x93);
      Transmite_Poll_Long(0x28);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Casheable Nonrestricted Out"); // Casheable nonrestricted out
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x34);
      Transmite_Poll_Long(0x2C);
      Transmite_Poll_Long(0x5C);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Games Played"); // Games played
#endif
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x15);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Coin In Fisico"); // Physical coin in
#endif
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x2A);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Coin Out Fisico"); // Physical coin out
#endif

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x2B);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Total Coin Drop");
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x24);
      Transmite_Poll_Long(0xAD);
      Transmite_Poll_Long(0x4C);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Machine Paid Progresive Payout");
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x1D);
      Transmite_Poll_Long(0xEF);
      Transmite_Poll_Long(0xE0);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Machine Paid External Bonus Payout");
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x1E);
      Transmite_Poll_Long(0x74);
      Transmite_Poll_Long(0xD2);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Attendant Paid Progresive Payout");
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x20);
      Transmite_Poll_Long(0x89);
      Transmite_Poll_Long(0x0A);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Attendant Paid External Payout");
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x21);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x1B);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Ticket In");
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x0D);
      Transmite_Poll_Long(0x6E);
      Transmite_Poll_Long(0xF0);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Ticket Out");
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x0E);
      Transmite_Poll_Long(0xF5);
      Transmite_Poll_Long(0xC2);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Current Credits");
#endif
      Transmite_Poll(0x1A);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Contador 1C - Door Open Metter");
#endif
      Transmite_Poll(0x1C);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("Contador 18 - Games Since Last Power Up");
#endif
      Transmite_Poll(0x18);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("ID Machine");
#endif
      Transmite_Poll(0x1F);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);
#ifdef Debug_Encuestas
      Serial.println("ROM Signature");
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x21);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x5C);
      Transmite_Poll_Long(0x45);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2)
      {
        sendDataa(dat, sizeof(dat)); // transmite sincronización
        delay(100);
        Transmite_Poll(0x1D);
      }
      Counter_Final=true;
  }


  if (Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) == 0 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 11)
  {
#ifdef Debug_Encuestas
      Serial.println("Total Cancel Credit"); // total cancel credit
#endif

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x10);
      delay(100);
#ifdef Debug_Encuestas
      Serial.println("Coin In"); // Coin in
#endif
      Transmite_Poll(0x11);
      delay(100);
#ifdef Debug_Encuestas
      Serial.println("Coin Out"); // Coin out
#endif

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x12);
      delay(100);

#ifdef Debug_Encuestas
      Serial.println("Coin In Fisico"); // Physical coin in
#endif

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x2A);
      delay(100);
#ifdef Debug_Encuestas
      Serial.println("Coin Out Fisico"); // Physical coin out
#endif

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x2B);
      delay(100);
#ifdef Debug_Encuestas
      Serial.println("Contador 1C - Door Open Metter");
#endif
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x1C);
      delay(100);
#ifdef Debug_Encuestas
      Serial.println("Contador 18 - Games Since Last Power Up");
#endif
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x18);
      delay(100);
#ifdef Debug_Encuestas
      Serial.println("ID Machine");
#endif
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);


      Transmite_Poll(0x1F);
      delay(100);
#ifdef Debug_Encuestas
      Serial.println("ROM Signature");
#endif
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x21);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x5C);
      Transmite_Poll_Long(0x45);
      Counter_Final = true;
  }
  if (Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) == 1 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 11)
  {

    
    #ifdef Debug_Encuestas
    Serial.println("Total Cancel Credit"); // total cancel credit
    #endif

    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);

    Transmite_Poll(0x10);
    delay(100);
    #ifdef Debug_Encuestas
    Serial.println("Coin In"); // Coin in
    #endif
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);

    Transmite_Poll(0x11);
    delay(100);
    #ifdef Debug_Encuestas
    Serial.println("Coin Out"); // Coin out
    #endif
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);

    Transmite_Poll(0x12);
    delay(100);

    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    #ifdef Debug_Encuestas
    Serial.println("Coin In Fisico"); // Physical coin in
    #endif
    Transmite_Poll(0x2A);
    delay(100);
    #ifdef Debug_Encuestas
    Serial.println("Coin Out Fisico"); // Physical coin out
    #endif

    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);

    Transmite_Poll(0x2B);
    delay(100);
    #ifdef Debug_Encuestas
    Serial.println("Contador 1C - Door Open Metter");
    #endif

    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);

    Transmite_Poll(0x1C);
    delay(100);
    #ifdef Debug_Encuestas
    Serial.println("Contador 18 - Games Since Last Power Up");
    #endif
    Transmite_Poll(0x18);
    delay(100);
    #ifdef Debug_Encuestas
    Serial.println("ID Machine");
    #endif

    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);

    Transmite_Poll(0x1F);
    delay(100);
    #ifdef Debug_Encuestas
    Serial.println("ROM Signature");
    #endif
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x21);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x5C);
    Transmite_Poll_Long(0x45);
    Counter_Final=true;
  }

  if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 10||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 13)
  {
      #ifdef Debug_Encuestas
      Serial.println("Encuesta Completa............");
      #endif
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x1A);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x10);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x11);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x12);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x14);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x13);
      Counter_Final=true;
  }
  if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 16)
  {
    Transmite_Poll(0x0F);
    delay(100);

    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);

    Transmite_Poll(0x46);
    delay(100);

    Transmite_Poll(0x2B);
    delay(100);

    Transmite_Poll(0x1A);
    delay(100);

    Transmite_Poll(0x1C);
    delay(100);

    Transmite_Poll(0x1F);
    delay(100);

    Counter_Final=true;
  }
  if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 4)
  {
      #ifdef Debug_Encuestas
      Serial.println("Encuesta Completa............");
      #endif

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x1A);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x2A);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x10);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x46);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x2A);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x11);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x12);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x14);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x13);
      delay(100);
      Transmite_Poll(0x2B);
      delay(100);

      Counter_Final=true;
  }
  if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 7)
  {
      #ifdef Debug_Encuestas
      Serial.println("Encuesta Completa............");
      #endif
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x11);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x12);
      delay(100);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x14);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x1F);
      Counter_Final=true;
  }
  if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 8)
  {
      #ifdef Debug_Encuestas
      Serial.println("Encuesta Completa............");
      #endif
      Transmite_Poll(0x11);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x12);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x14);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x13);
      delay(100);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x00);
      delay(50);

      Transmite_Poll(0x1F);
      Counter_Final=true;
  }

}

  //------------------------------> Transmite info cashless <---------------------------------------

void Interroga_Info_Cashless(void)
  {
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x74);
    Transmite_Poll_Long(0xFF);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x05);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0xC6);
    Transmite_Poll_Long(0x68);
  }

bool Consulta_Info_Cashless(void)
  {
    flag_handle_maquina = true;
    Handle_Maquina = Encuesta_Info_Cashless;
    delay(600);
    if (ACK_Maq)
    {
      ACK_Maq = false;
      return true;
    }
    else
    {
      return false;
    }
  }

void Transmite_Activa_Maquina(void)
  {
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x02);
    Transmite_Poll_Long(0xCA);
    Transmite_Poll_Long(0x3A);
  }

  //---------------------------- Tarea Para Encuestar premio y reset evento 52 -----------------------------------------------

bool Verifica_Premio_1B(void)
  {
    flag_handle_maquina = true;
    Handle_Maquina = flag_encuesta_premio;
    delay(600);
    if (ACK_Premio)
    {
      ACK_Premio = false;
      return true;
    }
    else
    {
      return false;
    }
  }

bool Reset_HandPay(void)
  {
    flag_handle_maquina = true;
    Handle_Maquina = Flag_Reset_Handpay;
    delay(600);
    if (ACK_Maq)
    {
      ACK_Maq = false;
      return true;
    }
    else
    {
      return false;
    }
  }
void Transmite_Reset_Handpay(void)
  {
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x94);
    Transmite_Poll_Long(0x75);
    Transmite_Poll_Long(0xCB);
  }

void Encuesta_contador_1B(void)
  {
    Transmite_Poll(0x1B);
  }

void Selector_Modo_SD(void)
  {
    if (Variables_globales.Get_Variable_Global(Ftp_Mode) == true || Variables_globales.Get_Variable_Global(Flag_Memoria_SD_Full) == true || Variables_globales.Get_Variable_Global(SD_INSERT) == false || Variables_globales.Get_Variable_Global(Falla_MicroSD) || Variables_globales.Get_Variable_Global(Updating_System) || Variables_globales.Get_Variable_Global(Flag_Log))
    {
      Variables_globales.Set_Variable_Global(Enable_Storage, false); // Deshabilita  Guardado SD.
    }
    else
    {
      if (Variables_globales.Get_Variable_Global(Sincronizacion_RTC) == true && Variables_globales.Get_Variable_Global(Ftp_Mode) == false && Variables_globales.Get_Variable_Global(Flag_Memoria_SD_Full) == false && Variables_globales.Get_Variable_Global(SD_INSERT) == true && Variables_globales.Get_Variable_Global(Flag_Archivos_OK) == true && !Variables_globales.Get_Variable_Global(Falla_MicroSD) && !Variables_globales.Get_Variable_Global(Updating_System) && !Variables_globales.Get_Variable_Global(Flag_Log))
      {
        Variables_globales.Set_Variable_Global(Enable_Storage, true);
      }
    }

    //Variables_globales.Set_Variable_Global(Enable_Storage, false);
  }

void Delete_Trama()
  {
    for (int i = 0; i <= SD_Cont.length(); i++)
    {
      SD_Cont.remove(i);
    }
  }
int Convert_Char_To_Int3(char buffer[])
  {
    int resultado = ((buffer[0] - 48) * 10000000) + ((buffer[1] - 48) * 1000000) +
                    ((buffer[2] - 48) * 100000) + ((buffer[3] - 48) * 10000) +
                    ((buffer[4] - 48) * 1000) + ((buffer[5] - 48) * 100) +
                    ((buffer[6] - 48) * 10) + ((buffer[7] - 48) * 1);
    return resultado;
  }

int Convert_Char_To_Int15(char buffer[]) {
    int resultado = 0;
    int factor = 1;
    int longitud = strlen(buffer);
    int primerDigito = 0;

    for (int i = 0; i < longitud; i++) {
        if (buffer[i] >= '0' && buffer[i] <= '9') {
            if (buffer[i] != '0' || primerDigito) {
                resultado = resultado * 10 + (buffer[i] - '0');
                primerDigito = 1;
            }
        }
    }

    return resultado;
  }
  
void Calcula_Cancel_Credit_IRT(void)
{
    int Total_Cancel_Credit_IRT, Cancel_Credit_IRT, Coin_Out_IRT, Residuo, Total_Cancel_Credit_IRT_2;
    int uni, dec, cen, unimil, decmil, centmil, unimill, decmill;
    char Contador_Cancel_Credit_IRT[9];
    bzero(Contador_Cancel_Credit_IRT, 9);

   // Cancel_Credit_IRT = contadores.Get_Contadores_Int(Copia_Cancel_Credit);
    Cancel_Credit_IRT = Convert_Char_To_Int15(contadores.Get_Contadores_Char(Copia_Cancel_Credit));
    // Cancel_Credit_IRT = contadores.Get_Contadores_Int(Copia_Cancel_Credit);
    Coin_Out_IRT = Convert_Char_To_Int15(contadores.Get_Contadores_Char(Physical_Coin_Out));
    //Coin_Out_IRT = contadores.Get_Contadores_Int(Physical_Coin_Out);
   // Serial.println(Cancel_Credit_IRT );
   // Serial.println(Coin_Out_IRT);
    Total_Cancel_Credit_IRT = Cancel_Credit_IRT + Coin_Out_IRT;

    
    
      Total_Cancel_Credit_IRT_2 = Total_Cancel_Credit_IRT;
     // Serial.print("contador cancel credit int IRT es: ");
     // Serial.println(Total_Cancel_Credit_IRT);

      char NL[9]={'F','F','F','F','F','F','F','F'};

      decmill = Total_Cancel_Credit_IRT / 10000000;
      Contador_Cancel_Credit_IRT[0] = decmill + 48;
      Residuo = Total_Cancel_Credit_IRT % 10000000;

      unimill = Residuo / 1000000;
      Contador_Cancel_Credit_IRT[1] = unimill + 48;
      Residuo = Total_Cancel_Credit_IRT % 1000000;

      centmil = Residuo / 100000;
      Contador_Cancel_Credit_IRT[2] = centmil + 48;
      Residuo = Total_Cancel_Credit_IRT % 100000;

      decmil = Residuo / 10000;
      Contador_Cancel_Credit_IRT[3] = decmil + 48;
      Residuo = Total_Cancel_Credit_IRT % 10000;

      unimil = Residuo / 1000;
      Contador_Cancel_Credit_IRT[4] = unimil + 48;
      Residuo = Total_Cancel_Credit_IRT % 1000;

      cen = Residuo / 100;
      Contador_Cancel_Credit_IRT[5] = cen + 48;
      Residuo = Total_Cancel_Credit_IRT % 100;

      dec = Residuo / 10;
      Contador_Cancel_Credit_IRT[6] = dec + 48;
      Residuo = Total_Cancel_Credit_IRT % 10;

      uni = Residuo;
      Contador_Cancel_Credit_IRT[7] = uni + 48;

     // Serial.print("contador cancel credit char IRT es: ");
     // Serial.println(Contador_Cancel_Credit_IRT);
      if (Convert_Char_To_Int15(Contador_Cancel_Credit_IRT) != Total_Cancel_Credit_IRT_2)
      {
      //  Serial.println("Error en calculo premio IRT");

      contadores.Set_Contadores(Total_Cancel_Credit, NL);
      contadores.Set_Contadores(Cancel_Credit_Hand_Pay, NL);
      }
      else
      {
        contadores.Set_Contadores(Total_Cancel_Credit, Contador_Cancel_Credit_IRT);
        contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_Cancel_Credit_IRT);

        /* ----------------->Se agrego<--------------------------------- */
        contadores.Change_Counters(Contador_Cancel_Credit_IRT);
        /*---------------------------------------------------------------*/
      }
    //  contadores.Set_Contadores(Total_Cancel_Credit, Contador_Cancel_Credit_IRT);
    //  contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_Cancel_Credit_IRT);
      Variables_globales.Set_Variable_Global(Firts_Cancel_IRT,true);
}

void Calcula_Bill_In_550(void)
  {
    int Bill_In_550, Residuo;
    int uni, dec, cen, unimil, decmil, centmil, unimill, decmill;
    char Contador_Bill_In_550[9];
    bzero(Contador_Bill_In_550, 9);

    Bill_In_550 = contadores.Get_Contadores_Int(Copia_Bill_Amount);

    Bill_In_550 *= 10;

    // Serial.print("contador bill_in int 550 es: ");
    // Serial.println(Bill_In_550);

    decmill = Bill_In_550 / 10000000;
    Contador_Bill_In_550[0] = decmill + 48;
    Residuo = Bill_In_550 % 10000000;

    unimill = Residuo / 1000000;
    Contador_Bill_In_550[1] = unimill + 48;
    Residuo = Bill_In_550 % 1000000;

    centmil = Residuo / 100000;
    Contador_Bill_In_550[2] = centmil + 48;
    Residuo = Bill_In_550 % 100000;

    decmil = Residuo / 10000;
    Contador_Bill_In_550[3] = decmil + 48;
    Residuo = Bill_In_550 % 10000;

    unimil = Residuo / 1000;
    Contador_Bill_In_550[4] = unimil + 48;
    Residuo = Bill_In_550 % 1000;

    cen = Residuo / 100;
    Contador_Bill_In_550[5] = cen + 48;
    Residuo = Bill_In_550 % 100;

    dec = Residuo / 10;
    Contador_Bill_In_550[6] = dec + 48;
    Residuo = Bill_In_550 % 10;

    uni = Residuo;
    Contador_Bill_In_550[7] = uni + 48;

    // Serial.print("contador bill_in char 550 es: ");
    // Serial.println(Contador_Bill_In_550);
    contadores.Set_Contadores(Bill_Amount, Contador_Bill_In_550);
  }

void Escribe_Tarjeta_Mecanica_2(char buf[])
  {
    
    for (int i = 0; i < 51; i++)
    {
      sendDataa(&buf[i], sizeof(buf[i]));
      //Serial.println(buf[i],DEC);
    }
    Recibe_Mecanicas = true;
  }

void Escribe_Tarjeta_Mecanica(char buf[])
  {
    for (int i = 0; i < 35; i++)
    {
      sendDataa(&buf[i], sizeof(buf[i]));
    }
    Recibe_Mecanicas = true;
  }

bool Verifica_Tarjeta_Mecanica(void)
  {
    flag_handle_maquina = true;

    Handle_Maquina = escribe_tarjeta_mecanica;
    delay(3000);
    if (ACK_Mecanicas)
    {
      ACK_Mecanicas = false;
      return true;
    }
    else
    {
      return false;
    }
  }

/* ----------------------------------------> SESION CASHLESS <---------------------------------*/
bool Interroga_Estado_registro_AFT(void)
{
    flag_handle_maquina = true;
    Handle_Maquina = Interroga_Est_Reg_AFT;
    delay(600);
    if (ACK_Maq)
    {
      ACK_Maq = false;
      return true;
    }
    else
    {
      return false;
    }
}

bool Inicializa_Registro_AFT(void)
{
    flag_handle_maquina = true;
    Handle_Maquina = Init_Reg_AFT;
    delay(600);
    if (ACK_Maq)
    {
      ACK_Maq = false;
      return true;
    }
    else
    {
      return false;
    }
}

bool Registra_MAQ_AFT(void)
{
    flag_handle_maquina = true;
    Handle_Maquina = Registra_Maquina_AFT;
    delay(600);
    if (ACK_Maq)
    {
      ACK_Maq = false;
      return true;
    }
    else
    {
      return false;
    }
}
bool Creditos_Machine(void)
{
  flag_handle_maquina = true;
    Handle_Maquina = Creditos_machine;
    delay(600);
    if (ACK_Maq)
    {
      ACK_Maq = false;
      return true;
    }
    else
    {
      return false;
    }
}

void Transmite_Encuesta_Creditos(void)
{

  for (int i = 0; i < 5; i++)
  {
    Transmite_Poll(0x1A);
    delay(100);
    Transmite_Poll(0x1A);
    delay(100);
  }
}

void Transmite_Encuesta_Maquina_Juego(void)
{


  Transmite_Poll(0x1A);
  delay(100);
  Transmite_Poll(0x11);
  delay(100);
  Transmite_Poll(0x12);
  delay(100);

  if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)==11)
  {
    Transmite_Poll(0x2A);
  }else{
   if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)!=7)
   {
      Transmite_Poll(0x13);
   }
    
  }
  delay(100);
}

bool Transmite_AFT_Registro_Machine(void)
{
  // Serial.println("REGISTRA MAQUINA....");
  Data_TX_AFT[0] = 0x01;
  Data_TX_AFT[1] = 0x73;
  Data_TX_AFT[2] = 0x1D;
  Data_TX_AFT[3] = 0x01;
  /* Asset*/
  Data_TX_AFT[4] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[0];
  Data_TX_AFT[5] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[1];
  Data_TX_AFT[6] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[2];
  Data_TX_AFT[7] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[3];

  /*Key */
  /*Asset*/
  Data_TX_AFT[8] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[0];
  Data_TX_AFT[9] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[1];
  Data_TX_AFT[10] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[2];
  Data_TX_AFT[11] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[3];
  /*POS ID*/
  Data_TX_AFT[12] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[4];
  Data_TX_AFT[13] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[5];
  Data_TX_AFT[14] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[6];
  Data_TX_AFT[15] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[7];
  /*MAC*/
  Data_TX_AFT[16] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[8];
  Data_TX_AFT[17] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[9];
  Data_TX_AFT[18] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[10];
  Data_TX_AFT[19] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[11];
  Data_TX_AFT[20] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[12];
  Data_TX_AFT[21] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[13];
  /*FECHA*/
  Data_TX_AFT[22] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[14];
  Data_TX_AFT[23] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[15];
  Data_TX_AFT[24] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[16];
  Data_TX_AFT[25] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[17];
  Data_TX_AFT[26] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[18];
  Data_TX_AFT[27] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[19];
  /*POS ID*/
  Data_TX_AFT[28] = 0x01;
  Data_TX_AFT[29] = 0x00;
  Data_TX_AFT[30] = 0x00;
  Data_TX_AFT[31] = 0x00;

  calcularCRC_Registro_AFT();

  sendDataa(dat4, sizeof(dat4)); // Transmite DIR
  Transmite_Poll_Long(Data_TX_AFT[1]);
  Transmite_Poll_Long(Data_TX_AFT[2]);
  Transmite_Poll_Long(Data_TX_AFT[3]);
  Transmite_Poll_Long(Data_TX_AFT[4]);
  Transmite_Poll_Long(Data_TX_AFT[5]);
  Transmite_Poll_Long(Data_TX_AFT[6]);
  Transmite_Poll_Long(Data_TX_AFT[7]);
  Transmite_Poll_Long(Data_TX_AFT[8]);
  Transmite_Poll_Long(Data_TX_AFT[9]);
  Transmite_Poll_Long(Data_TX_AFT[10]);
  Transmite_Poll_Long(Data_TX_AFT[11]);
  Transmite_Poll_Long(Data_TX_AFT[12]);
  Transmite_Poll_Long(Data_TX_AFT[13]);
  Transmite_Poll_Long(Data_TX_AFT[14]);
  Transmite_Poll_Long(Data_TX_AFT[15]);
  Transmite_Poll_Long(Data_TX_AFT[16]);
  Transmite_Poll_Long(Data_TX_AFT[17]);
  Transmite_Poll_Long(Data_TX_AFT[18]);
  Transmite_Poll_Long(Data_TX_AFT[19]);
  Transmite_Poll_Long(Data_TX_AFT[20]);
  Transmite_Poll_Long(Data_TX_AFT[21]);
  Transmite_Poll_Long(Data_TX_AFT[22]);
  Transmite_Poll_Long(Data_TX_AFT[23]);
  Transmite_Poll_Long(Data_TX_AFT[24]);
  Transmite_Poll_Long(Data_TX_AFT[25]);
  Transmite_Poll_Long(Data_TX_AFT[26]);
  Transmite_Poll_Long(Data_TX_AFT[27]);
  Transmite_Poll_Long(Data_TX_AFT[28]);
  Transmite_Poll_Long(Data_TX_AFT[29]);
  Transmite_Poll_Long(Data_TX_AFT[30]);
  Transmite_Poll_Long(Data_TX_AFT[31]);
  Transmite_Poll_Long(Data_TX_AFT[32]);
  Transmite_Poll_Long(Data_TX_AFT[33]);
  return true;
}

bool Cancela_Registro_AFT(void)
{
    flag_handle_maquina = true;
    Handle_Maquina = Cancela_Registro_AFT_MQ;
    delay(600);
    if (ACK_Maq)
    {
      ACK_Maq = false;
      return true;
    }
    else
    {
      return false;
    }
}

void Transmite_Cancela_Registro_MQ(void)
{
  Serial.println("Transmite Cancela Reg AFT");
  sendDataa(dat4, sizeof(dat4)); // Transmite DIR
  Transmite_Poll_Long(0x73);
  Transmite_Poll_Long(0x01);
  Transmite_Poll_Long(0x80);
  Transmite_Poll_Long(0xD7);
  Transmite_Poll_Long(0xEE);

}

bool Carga_Bonus_Maquina(void)
{
    flag_handle_maquina = true;
    Handle_Maquina = Flag_Carga_Bonus;
    delay(600);
    if (ACK_Maq)
    {
      ACK_Maq = false;
      return true;
    }
    else
    {
      return false;
    }
}

char Data_TX[8];

/* ***************** CASHLESS ********************/


void Transmite_Carga_legacy_Bonus(void)
{

  Cashless.Delete_ACK_Bonus();

  Data_TX[0]=0x01;
  Data_TX[1]=0x8A;    

  Data_TX[2]=contadores.Get_Amount_Legacy_Bonus_Awards()[0];         
  Data_TX[3]=contadores.Get_Amount_Legacy_Bonus_Awards()[1];
  Data_TX[4]=contadores.Get_Amount_Legacy_Bonus_Awards()[2];
  Data_TX[5]=contadores.Get_Amount_Legacy_Bonus_Awards()[3];
  Data_TX[6]=contadores.Get_Type_Legacy_Bonus_Awards();

  CalcularCRC_Datos();

  Variables_globales.Set_Variable_Global(Solicitud_Carga_Bonus,true);

  sendDataa(dat4, sizeof(dat4)); // Transmite DIR
  Transmite_Poll_Long(Data_TX[1]);
  Transmite_Poll_Long(Data_TX[2]);
  Transmite_Poll_Long(Data_TX[3]);
  Transmite_Poll_Long(Data_TX[4]);
  Transmite_Poll_Long(Data_TX[5]);
  Transmite_Poll_Long(Data_TX[6]);
  Transmite_Poll_Long(Data_TX[7]);
  Transmite_Poll_Long(Data_TX[8]);

  unsigned long Timout_Break_Response;
  int Stop_Transaccion_Amount_Response = 5000; // Tiempo de espera en milisegundos (2 Seg MAX)
  Timout_Break_Response = millis();
  esp_task_wdt_init(1000000, true);
  esp_task_wdt_add(NULL);


  bool Comando=false;
  while ((millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response))
  {
    esp_task_wdt_reset();


    if(Cashless.Get_ACK_Bonus()[0]==0x01)
    {
      Comando=true;
      break;
    }
      
    vTaskDelay(10);
    // Serial.println("Esperando por la transferencia.....!");
  }
  
  
  if(Comando)
  {

    Cashless.Set_Status_AFT_Bonus(COMMAND_RECEIVED);
    bool Status=false;

    Serial.println("Comando Recibido por la maquina ");

    if(Cashless.Get_ACK_Bonus()[1]==0x00 &&Cashless.Get_ACK_Bonus()[0]==0x01)
      Status=false;
    else
      Status=true;

    if (Status)
    {
      Cashless.Set_Status_AFT_Bonus(BONUS_PENDING);
      Serial.println(" Bonus Pending ");
    }
    else{
      Cashless.Set_Status_AFT_Bonus(REJECTED);
      Serial.println(" Bono Rechazado! ");
      Variables_globales.Set_Variable_Global(Solicitud_Carga_Bonus,false);
    }
  }
  else
  {
    /**/
    Cashless.Set_Status_AFT_Bonus(NOT_COMUNICATIONS_WITH_MACHINE_FOR_BONUS);
    Variables_globales.Set_Variable_Global(Solicitud_Carga_Bonus,false);
  }

  

  //delay(500); /* Espera  respuesta Maq */
   /* Aqui validacion  Buffer*/
  //Variables_globales.Set_Variable_Global(Status_Load_Bonus,true); 
}


void Transmite_Carga_legacy_bonus_awards(void)
{
  Cashless.Delete_ACK_Bonus();
  #define BONUS_OK           0x01
  #define EXCEPTION          0x81
  #define SIN_RESPUESTA      0XFF
  #define EVENTO_CARGA_BONUS 0x7C

  Data_TX[0]=0x01;
  Data_TX[1]=0x8A;    

  Data_TX[2]=contadores.Get_Amount_Legacy_Bonus_Awards()[0];         
  Data_TX[3]=contadores.Get_Amount_Legacy_Bonus_Awards()[1];
  Data_TX[4]=contadores.Get_Amount_Legacy_Bonus_Awards()[2];
  Data_TX[5]=contadores.Get_Amount_Legacy_Bonus_Awards()[3];
  Data_TX[6]=contadores.Get_Type_Legacy_Bonus_Awards();

  CalcularCRC_Datos();

  // /* Bloque MAQUINA */
  // sendDataa(dat4, sizeof(dat4)); // Transmite DIR
  // Transmite_Poll_Long(0x01);
  // Transmite_Poll_Long(0x51);
  // Transmite_Poll_Long(0x08);

  // delay(300);

  sendDataa(dat4, sizeof(dat4)); // Transmite DIR
  Transmite_Poll_Long(Data_TX[1]);
  Transmite_Poll_Long(Data_TX[2]);
  Transmite_Poll_Long(Data_TX[3]);
  Transmite_Poll_Long(Data_TX[4]);
  Transmite_Poll_Long(Data_TX[5]);
  Transmite_Poll_Long(Data_TX[6]);
  Transmite_Poll_Long(Data_TX[7]);
  Transmite_Poll_Long(Data_TX[8]);
  
  delay(500); /* Espera  respuesta Maq */
   /* Aqui validacion  Buffer*/
  Variables_globales.Set_Variable_Global(Status_Load_Bonus,true); 
}


bool Transmite_Init_Registro(void)
{
 // Serial.println("INICIALIZA REGISTRO....");
  

  Data_TX_AFT[0] = 0x01;
  Data_TX_AFT[1] = 0x73;
  Data_TX_AFT[2] = 0x1D;
  Data_TX_AFT[3] = 0x00;
  /* Asset*/
  Data_TX_AFT[4] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[0];
  Data_TX_AFT[5] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[1];
  Data_TX_AFT[6] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[2];
  Data_TX_AFT[7] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[3];

  /*Key */
  /*Asset*/
  Data_TX_AFT[8] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[0];
  Data_TX_AFT[9] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[1];
  Data_TX_AFT[10] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[2];
  Data_TX_AFT[11] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[3];
  /*POS ID*/
  Data_TX_AFT[12] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[4];
  Data_TX_AFT[13] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[5];
  Data_TX_AFT[14] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[6];
  Data_TX_AFT[15] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[7];
  /*MAC*/
  Data_TX_AFT[16] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[8];
  Data_TX_AFT[17] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[9];
  Data_TX_AFT[18] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[10];
  Data_TX_AFT[19] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[11];
  Data_TX_AFT[20] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[12];
  Data_TX_AFT[21] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[13];
  /*FECHA*/
  Data_TX_AFT[22] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[14];
  Data_TX_AFT[23] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[15];
  Data_TX_AFT[24] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[16];
  Data_TX_AFT[25] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[17];
  Data_TX_AFT[26] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[18];
  Data_TX_AFT[27] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[19];
  /*POS ID*/
  Data_TX_AFT[28] = 0x01;
  Data_TX_AFT[29] = 0x00;
  Data_TX_AFT[30] = 0x00;
  Data_TX_AFT[31] = 0x00;

  calcularCRC_Registro_AFT();

  sendDataa(dat4, sizeof(dat4)); // Transmite DIR
  Transmite_Poll_Long(Data_TX_AFT[1]);
  Transmite_Poll_Long(Data_TX_AFT[2]);
  Transmite_Poll_Long(Data_TX_AFT[3]);
  Transmite_Poll_Long(Data_TX_AFT[4]);
  Transmite_Poll_Long(Data_TX_AFT[5]);
  Transmite_Poll_Long(Data_TX_AFT[6]);
  Transmite_Poll_Long(Data_TX_AFT[7]);
  Transmite_Poll_Long(Data_TX_AFT[8]);
  Transmite_Poll_Long(Data_TX_AFT[9]);
  Transmite_Poll_Long(Data_TX_AFT[10]);
  Transmite_Poll_Long(Data_TX_AFT[11]);
  Transmite_Poll_Long(Data_TX_AFT[12]);
  Transmite_Poll_Long(Data_TX_AFT[13]);
  Transmite_Poll_Long(Data_TX_AFT[14]);
  Transmite_Poll_Long(Data_TX_AFT[15]);
  Transmite_Poll_Long(Data_TX_AFT[16]);
  Transmite_Poll_Long(Data_TX_AFT[17]);
  Transmite_Poll_Long(Data_TX_AFT[18]);
  Transmite_Poll_Long(Data_TX_AFT[19]);
  Transmite_Poll_Long(Data_TX_AFT[20]);
  Transmite_Poll_Long(Data_TX_AFT[21]);
  Transmite_Poll_Long(Data_TX_AFT[22]);
  Transmite_Poll_Long(Data_TX_AFT[23]);
  Transmite_Poll_Long(Data_TX_AFT[24]);
  Transmite_Poll_Long(Data_TX_AFT[25]);
  Transmite_Poll_Long(Data_TX_AFT[26]);
  Transmite_Poll_Long(Data_TX_AFT[27]);
  Transmite_Poll_Long(Data_TX_AFT[28]);
  Transmite_Poll_Long(Data_TX_AFT[29]);
  Transmite_Poll_Long(Data_TX_AFT[30]);
  Transmite_Poll_Long(Data_TX_AFT[31]);
  Transmite_Poll_Long(Data_TX_AFT[32]);
  Transmite_Poll_Long(Data_TX_AFT[33]);
  return true;
}
bool Transmite_Registro_AFT_Maq(void)
{

  //Serial.println("Consulta Registro AFT....");
  sendDataa(dat4, sizeof(dat4)); // Transmite DIR
  Transmite_Poll_Long(0x73);
  Transmite_Poll_Long(0x01);
  Transmite_Poll_Long(0xFF);
  Transmite_Poll_Long(0xA7);
  Transmite_Poll_Long(0x65);

  return true;
}
/*---------------------------------------------------------------------------------------------*/

bool Solicitud_Forzada_Carga_Cashless(void)
{
  flag_handle_Forze_Load=true;
  delay(600);
  return true;
}

bool Solicitud_Carga_Cashless(void)
{
  Info_Cashless.Set_Controller_Transfer_Load(true);
  flag_handle_maquina_Cashless = true;
  Handle_Maquina_Cashless = Flag_Carga_Cashless;
  delay(600);
  if (ACK_Maq_Cashless)
  {
    ACK_Maq_Cashless = false;
    return true;
  }
  else
  {
    return false;
  }
}

bool Solicitud_Descarga_Cashless(void)
{
  Info_Cashless.Set_Controller_Transfer_Download(true);
  flag_handle_maquina_Cashless = true;
  Handle_Maquina_Cashless = Flag_Descarga_Cashless;
  delay(600);
  if (ACK_Maq_Cashless)
  {
    ACK_Maq_Cashless = false;
    return true;
  }
  else
  {
    return false;
  }
}

bool Actualiza_Cashless_Entradas(void)
{
  Update_In_Cashless=true;

  if(Update_In_Cashless)
    return true;
  else
    return false;
}

bool Actualiza_Tito_Entradas(void)
{
  Update_In_Tito = true;

  if (Update_In_Tito)
    return true;
  else
    return false;
}

bool Actualiza_Tito_Salidas(void)
{
  Update_Out_Tito = true;

  if (Update_Out_Tito)
  {
    return true;
  }
  else
    return false;
}

bool Actualiza_Cashless_Salidas(void)
{
  Update_Out_Cashless=true;

  if(Update_Out_Cashless)
    return true;
  else 
    return false;
}

bool Transmite_Status_AFT_Maq(void)
{

  // TX>= 01 74 FF 00 05 00 A2 87
  sendDataa(dat4, sizeof(dat4)); // Transmite DIR
  Transmite_Poll_Long(0x74);
  Transmite_Poll_Long(0xFF);
  Transmite_Poll_Long(0x00);
  Transmite_Poll_Long(0x05);
  Transmite_Poll_Long(0x00);
  Transmite_Poll_Long(0xA2);
  Transmite_Poll_Long(0x87);

  delay(500); /* Espera tiempo de respuesta Maq */

  return true; /* Condicion OK carga  o descarga  */
}

bool Consulta_Creditos_Cashless(void)
{
  flag_handle_maquina_Cashless = true;
  Handle_Maquina_Cashless = Flag_Consulta_Creditos_Cashless;
  delay(600);
  if (ACK_Maq)
  {
    ACK_Maq = false;
    return true;
  }
  else
  {
    return false;
  }
}

void Transmite_Consulta_Creditos_Cashless(void)
{
  char Request_Credit[9];
  int Size_Command_Data = 7;

  Request_Credit[0] = 0x01; // DIRECCION MAQUINA
  Request_Credit[1] = 0x74; // COMANDO
  Request_Credit[2] = 0xFF; // Request Look
  Request_Credit[3] = 0x03; // Trasnfer Condition
  Request_Credit[4] = 0x05; // Duracion Bloqueo H
  Request_Credit[5] = 0x00; // Duracion Bloqueo L
  Request_Credit[6] = 0xC6; // CRC
  Request_Credit[7] = 0x68; // CRC

 

  for (int i = 0; i < 8; i++)
  {
    if (i == 0)
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    else
    {
      Transmite_Poll_Long(Request_Credit[i]);
    }
  }

  delay(500); /* Espera respuesta de maquina */
}

void Transmite_Download_AFT_Maq(void)
{

  Info_Cashless.Set_Status_Transfer(TRANSFER_IN_PROGRESS);
  /*EFT*/

  String logg="";

  if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
  {
    Transmite_Download_EFT_Maq();
  } /*AFT*/
  else
  {

    sendDataa(dat, sizeof(dat)); // transmite sincronización
    delay(250);

    /*----------> Consulta creditos actuales de la maquina AFT <----------------------------- */
    char Request_Credit[9];
    int Size_Command_Data = 7;

    Request_Credit[0] = 0x01; // DIRECCION MAQUINA
    Request_Credit[1] = 0x74; // COMANDO
    Request_Credit[2] = 0xFF; // Request Look
    Request_Credit[3] = 0x03; // Trasnfer Condition
    Request_Credit[4] = 0x05; // Duracion Bloqueo H
    Request_Credit[5] = 0x00; // Duracion Bloqueo L
    Request_Credit[6] = 0xC6; // CRC
    Request_Credit[7] = 0x68; // CRC

    esp_task_wdt_init(1000000, true);
    esp_task_wdt_add(NULL);
    unsigned long Timout_Break_Amount;
    int Stop_Transaccion_Amount = 15000;                                  // Tiempo de espera en milisegundos (5 Seg MAX)
    Variables_globales.Set_Variable_Global(Amount_Download_Ready, false); /* Inicial Valor */
    Timout_Break_Amount = millis();
    /* Consulta los creditos de la maquina a descargar Hasta recibir o que se termine el TimeOut */
    while (!Variables_globales.Get_Variable_Global(Amount_Download_Ready) && (millis() - Timout_Break_Amount < Stop_Transaccion_Amount))
    {
      esp_task_wdt_reset();

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      delay(250);

      for (int i = 0; i < 8; i++)
      {
        if (i == 0)
          sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        else
        {
          Transmite_Poll_Long(Request_Credit[i]);
        }
      }
      delay(250);
      vTaskDelay(300);
      if (Variables_globales.Get_Variable_Global(Amount_Download_Ready))
        break;
    }

    /*--------------------------------------------------------------------*/
    
    /* --------------> Pregunta  Por Ack Respuesta de la maquina <----------*/
    if (Variables_globales.Get_Variable_Global(Amount_Download_Ready)) /* Creditos OK */
    {

      // sendDataa(dat, sizeof(dat)); // transmite sincronización
      // delay(250);

      // sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      // Transmite_Poll_Long(0x72);
      // Transmite_Poll_Long(0x02);
      // Transmite_Poll_Long(0xFF);
      // Transmite_Poll_Long(0x00);
      // Transmite_Poll_Long(0x0F);
      // Transmite_Poll_Long(0x22);

      Variables_globales.Set_Variable_Global(Amount_Download_Ready, false);
      Variables_globales.Set_Variable_Global(Machine_Receives_Load_Transfer, false);
      
      char Transfer_Command_Down[63];
      int Size_Command = 60;
      Cashless.Increase_Transaction_Number_ID();

      Transfer_Command_Down[0] = 0x01;
      Transfer_Command_Down[1] = 0x72;
      Transfer_Command_Down[2] = 0x3A;
      Transfer_Command_Down[3] = 0x01;
      Transfer_Command_Down[4] = 0x00;
      Transfer_Command_Down[5] = 0x80;

      // CASHABLES
      Transfer_Command_Down[6] = 0x99;
      Transfer_Command_Down[7] = 0x99;
      Transfer_Command_Down[8] = 0x99;
      Transfer_Command_Down[9] = 0x99;
      Transfer_Command_Down[10] = 0x99;
      // RESTRICTED
      Transfer_Command_Down[11] = 0x99;
      Transfer_Command_Down[12] = 0x99;
      Transfer_Command_Down[13] = 0x99;
      Transfer_Command_Down[14] = 0x99;
      Transfer_Command_Down[15] = 0x99;
      // NO RESTRICTED
      Transfer_Command_Down[16] = 0x99;
      Transfer_Command_Down[17] = 0x99;
      Transfer_Command_Down[18] = 0x99;
      Transfer_Command_Down[19] = 0x99;
      Transfer_Command_Down[20] = 0x99;

      Transfer_Command_Down[21] = 0b00000001; // TRANSFER FLAGS
      

      // ASSET NUMEBER
      Transfer_Command_Down[22] = Buffer_Cashless.Get_Key_Register_AFT()[0];
      Transfer_Command_Down[23] = Buffer_Cashless.Get_Key_Register_AFT()[1];
      Transfer_Command_Down[24] = Buffer_Cashless.Get_Key_Register_AFT()[2];
      Transfer_Command_Down[25] = Buffer_Cashless.Get_Key_Register_AFT()[3];

      // REG KEY
      Transfer_Command_Down[26] = Buffer_Cashless.Get_Key_Register_AFT()[0];
      Transfer_Command_Down[27] = Buffer_Cashless.Get_Key_Register_AFT()[1];
      Transfer_Command_Down[28] = Buffer_Cashless.Get_Key_Register_AFT()[2];
      Transfer_Command_Down[29] = Buffer_Cashless.Get_Key_Register_AFT()[3];

      Transfer_Command_Down[30] = Buffer_Cashless.Get_Key_Register_AFT()[4];
      Transfer_Command_Down[31] = Buffer_Cashless.Get_Key_Register_AFT()[5];
      Transfer_Command_Down[32] = Buffer_Cashless.Get_Key_Register_AFT()[6];
      Transfer_Command_Down[33] = Buffer_Cashless.Get_Key_Register_AFT()[7];

      Transfer_Command_Down[34] = Buffer_Cashless.Get_Key_Register_AFT()[8];
      Transfer_Command_Down[35] = Buffer_Cashless.Get_Key_Register_AFT()[9];
      Transfer_Command_Down[36] = Buffer_Cashless.Get_Key_Register_AFT()[10];
      Transfer_Command_Down[37] = Buffer_Cashless.Get_Key_Register_AFT()[11];

      Transfer_Command_Down[38] = Buffer_Cashless.Get_Key_Register_AFT()[12];
      Transfer_Command_Down[39] = Buffer_Cashless.Get_Key_Register_AFT()[13];
      Transfer_Command_Down[40] = Buffer_Cashless.Get_Key_Register_AFT()[14];
      Transfer_Command_Down[41] = Buffer_Cashless.Get_Key_Register_AFT()[15];

      Transfer_Command_Down[42] = Buffer_Cashless.Get_Key_Register_AFT()[16];
      Transfer_Command_Down[43] = Buffer_Cashless.Get_Key_Register_AFT()[17];
      Transfer_Command_Down[44] = Buffer_Cashless.Get_Key_Register_AFT()[18];
      Transfer_Command_Down[45] = Buffer_Cashless.Get_Key_Register_AFT()[19];

      Transfer_Command_Down[46] = 0x07; // LENGT TRASN_ID

      // TRANS_ID
      Transfer_Command_Down[47] = Cashless.Get_Trans_ID()[0];
      Transfer_Command_Down[48] = Cashless.Get_Trans_ID()[1];
      Transfer_Command_Down[49] = Cashless.Get_Trans_ID()[2];
      Transfer_Command_Down[50] = Cashless.Get_Trans_ID()[3];
      Transfer_Command_Down[51] = Cashless.Get_Trans_ID()[4];
      Transfer_Command_Down[52] = Cashless.Get_Trans_ID()[5];
      Transfer_Command_Down[53] = Cashless.Get_Trans_ID()[6];

      // EXPIRATION
      Transfer_Command_Down[54] = 0x00;
      Transfer_Command_Down[55] = 0x00;
      Transfer_Command_Down[56] = 0x00;
      Transfer_Command_Down[57] = 0x00;

      // POLL ID
      Transfer_Command_Down[58] = 0x00;
      Transfer_Command_Down[59] = 0x00;

      // RECEIPT DATA LENGT
      Transfer_Command_Down[60] = 0x00;

      CalcularCRC_Transfer(Transfer_Command_Down, Size_Command);

      Buffer_Cashless.Init_Buffer_Transfer_AFT(true); /* Borra data AFT */

      // for (int i = 0; i <= 60; i++)
      // {
      //   if (i > 0)
      //     logg += "|";

      //   // Agrega cada byte como 0xHH
      //   if (Transfer_Command_Down[i] < 0x10)
      //   {
      //     logg += "0x0" + String(Transfer_Command_Down[i], HEX);
      //   }
      //   else
      //   {
      //     logg += "0x" + String(Transfer_Command_Down[i], HEX);
      //   }
      // }
      // logg.replace(" ", "");
      // Info_Cashless.Log(RTC, "TRANSACCION_DESCARGA_RECIBIDA_CONTROLADOR:", logg);

      /* ------------------------> Envia Transferencia de Desccarga <---------------------------*/
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      delay(200);

      for (int i = 0; i < 63; i++)
      {
        if (i == 0)
          sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        else
        {

          esp_task_wdt_reset();
          Transmite_Poll_Long(Transfer_Command_Down[i]);
        }
      }

      delay(200);

      int Contador=0;
      /*  Espera a que  la maquina reciba el comando de  transferencia */
      unsigned long Timout_Break_Response;
      int Stop_Transaccion_Amount_Response = 20000; // Tiempo de espera en milisegundos (5 Seg MAX)
      Timout_Break_Response = millis();
      esp_task_wdt_init(1000000, true);
      esp_task_wdt_add(NULL);
      /* Consulta los creditos de la maquina a descargar Hasta recibir */
      while (!Variables_globales.Get_Variable_Global(Machine_Receives_Load_Transfer) && (millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response))
      {
        esp_task_wdt_reset();
        Contador++;
        if (Contador == 1)
          sendDataa(dat, sizeof(dat)); // Transmite DIR
        if (Contador > 1)
        {
          Transmite_Poll(0x00);
          Contador = 0;
        }
        delay(200);
        // Serial.println("Esperando Ack.......Transfer");
        vTaskDelay(300);
      }
      /*---------------------------------------------------------------------------------*/
      bool Error = Variables_globales.Get_Variable_Global(Machine_Receives_Load_Transfer);

      if (!Error)
      {

        // for (int i = 0; i <= 60; i++)
        // {
        //   if (i > 0)
        //     logg += "|";

        //   // Agrega cada byte como 0xHH
        //   if (Buffer_Cashless.Get_RX_AFT(Buffer_RX_Cashless)[i] < 0x10)
        //   {
        //     logg += "0x0" + String(Buffer_Cashless.Get_RX_AFT(Buffer_RX_Cashless)[i], HEX);
        //   }
        //   else
        //   {
        //     logg += "0x" + String(Buffer_Cashless.Get_RX_AFT(Buffer_RX_Cashless)[i], HEX);
        //   }
        // }
        // logg.replace(" ", "");
        // Info_Cashless.Log(RTC, "NO_HAY_COMUNICACION_CON_LA_MET:", logg);
        // Variables_globales.Set_Variable_Global(Machine_Receives_Load_Transfer, false);
        int clienteID = contadores.Get_Client_ID_Transaccion_Int();
        Info_Cashless.Status_Transfer_Download(NOT_COMMUNICATION_WITH_THE_MACHINE, Buffer_Cashless.Get_RX_AFT(Buffer_RX_Cashless), RTC,clienteID);
        Status_Barra(ERROR_LECTURA);

        return;

      }
      else
      {
        Variables_globales.Set_Variable_Global(Machine_Receives_Load_Transfer, false);

        esp_task_wdt_init(1000000, true);
        esp_task_wdt_add(NULL);

        unsigned long Timout_Break;
        int Stop_Transaccion = 20000; // Tiempo de espera en milisegundos (10 Seg MAX)
        bool Comp = false;
        Timout_Break = millis();

        bool CurrentStatus = false;

        if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[1] == 0x72)
        {

          switch (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4])
          {
          case 0x40:
            Info_Cashless.Ack_Transfer_Pending(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4], Buffer_Cashless.Get_RX_AFT(Buffer_RX_Cashless), RTC, DOWNLOAD_TRANSACTION);
            CurrentStatus = true;
            break;

          case 0x00:
            CurrentStatus = true;
            break;

          case 0x01:
            CurrentStatus = true;
            break;

          default:
            int clienteID = contadores.Get_Client_ID_Transaccion_Int();
            Info_Cashless.Status_Transfer_Download(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4], Buffer_Cashless.Get_RX_AFT(Buffer_RX_Cashless), RTC,clienteID);
            Status_Barra(ERROR_LECTURA);
            for (int i = 0; i < 5; i++)
            {
              if (Info_Cashless.Reader_Lock(false))
              {
                Info_Cashless.Reader_Lock(false);
              }
              else
              {
                Info_Cashless.Reader_Lock(false);
              }
            }
            CurrentStatus = false;
            break;
          }

          delay(500);

          if (CurrentStatus)
          {
            /* Borra data AFT para consulta de estado de transaccion  */
            Buffer_Cashless.Init_Buffer_Transfer_AFT(true);

            /* Sincroniza Maquina */
            sendDataa(dat, sizeof(dat)); // Transmite SYN
            delay(200);
            /* AFT FUNDS TRANSFER STATUS */
            sendDataa(dat4, sizeof(dat4)); // Transmite DIR
            Transmite_Poll_Long(0x72);
            Transmite_Poll_Long(0x02);
            Transmite_Poll_Long(0xFF);
            Transmite_Poll_Long(0x00);
            Transmite_Poll_Long(0x0F);
            Transmite_Poll_Long(0x22);
            delay(500);

            /* Transmite Poll*/
            Transmite_Poll(0x00);

            while ((Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x40 || Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0xAA) && (millis() - Timout_Break < Stop_Transaccion))
            {
              esp_task_wdt_reset();
              // if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x40 || Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0xAA)
              // {
              /* AFT FUNDS TRANSFER STATUS */
              // delay(200);
              sendDataa(dat, sizeof(dat)); // transmite sincronización
              delay(250);

              sendDataa(dat4, sizeof(dat4)); // Transmite DIR
              Transmite_Poll_Long(0x72);
              Transmite_Poll_Long(0x02);
              Transmite_Poll_Long(0xFF);
              Transmite_Poll_Long(0x00);
              Transmite_Poll_Long(0x0F);
              Transmite_Poll_Long(0x22);

              delay(500); /* Espera respuesta de maquina */
              Transmite_Poll(0x00);
              delay(250);
              // }
              // Serial.println("Esperando por la transferencia.....!");
              vTaskDelay(300);
            }

            if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x00 || Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x01)
            {

              /* Reset Variable de juego terminado */
              Variables_globales.Set_Variable_Global(Status_Games_Machine, false);
              int clienteID = contadores.Get_Client_ID_Transaccion_Int();
              Info_Cashless.Status_Transfer_Download(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4], Buffer_Cashless.Get_RX_AFT(Buffer_RX_Cashless), RTC,clienteID);

              Info_Cashless.Close_Player_Tracking_Sesion(true);
              Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
              // Info_Cashless.Reader_Lock(false);
              contadores.Close_ID_Client_Transaccion();

              for (int i = 0; i < 5; i++)
              {
                if (Info_Cashless.Reader_Lock(false))
                {
                  Info_Cashless.Reader_Lock(false);
                }
                else
                {
                  Info_Cashless.Reader_Lock(false);
                }
              }

              // logg = "";
              // for (int i = 0; i <= 60; i++)
              // {
              //   if (i > 0)
              //     logg += "|";

              //   // Agrega cada byte como 0xHH
              //   if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i] < 0x10)
              //   {
              //     logg += "0x0" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
              //   }
              //   else
              //   {
              //     logg += "0x" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
              //   }
              // }
              // logg.replace(" ", "");
              // Info_Cashless.Log(RTC, "TRANSFERENCIA_DESCARGA_REALIZADA_CON_EXITO", logg);
              return;
            }
            else
            {

              if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x40)
              {
                // Serial.println("Transaccion pendiente por reportar ");
                /* Bloquea lector */
                Info_Cashless.Lock_Reader();
                Status_Barra(301); /* Marca lector ocupado */
                /* Bandera para captura de evento! OK */

                /* Indica que la transacción esta pendiente....*/

                /* Guarda Transaccion pendiente en memoria */
                NVS.begin("Config_ESP32", false);
                NVS.putBool("Cash_Pen_Dow", true);
                NVS.end();
                Variables_globales.Set_Variable_Global(Event_Dowmload_Cashless_Pending, true);

                // String logg = "";
                // for (int i = 0; i <= 60; i++)
                // {
                //   if (i > 0)
                //     logg += "|";

                //   // Agrega cada byte como 0xHH
                //   if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i] < 0x10)
                //   {
                //     logg += "0x0" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
                //   }
                //   else
                //   {
                //     logg += "0x" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
                //   }
                // }
                // logg.replace(" ", "");
                // Info_Cashless.Log(RTC, "TRANSFERENCIA__DESCARGA_CRITICAMENTE_PENDIENTE:", logg);
                return;
              }
              else if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x00 || Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x01)
              {
                int clienteID = contadores.Get_Client_ID_Transaccion_Int();
                Info_Cashless.Status_Transfer_Download(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4], Buffer_Cashless.Get_RX_AFT(Buffer_RX_Cashless), RTC,clienteID);

                Info_Cashless.Close_Player_Tracking_Sesion(true);
                Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
                // Info_Cashless.Reader_Lock(false);
                contadores.Close_ID_Client_Transaccion();

                for (int i = 0; i < 5; i++)
                {
                  if (Info_Cashless.Reader_Lock(false))
                  {
                    Info_Cashless.Reader_Lock(false);
                  }
                  else
                  {
                    Info_Cashless.Reader_Lock(false);
                  }
                }

                // logg = "";
                // for (int i = 0; i <= 60; i++)
                // {
                //   if (i > 0)
                //     logg += "|";

                //   // Agrega cada byte como 0xHH
                //   if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i] < 0x10)
                //   {
                //     logg += "0x0" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
                //   }
                //   else
                //   {
                //     logg += "0x" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
                //   }
                // }
                // logg.replace(" ", "");
                // Info_Cashless.Log(RTC, "TRANSFERENCIA_DESCARGA_REALIZADA_CON_EXITO", logg);
                return;
              }
              else
              {
                int clienteID = contadores.Get_Client_ID_Transaccion_Int();
                Info_Cashless.Status_Transfer_Download(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4], Buffer_Cashless.Get_RX_AFT(Buffer_RX_Cashless), RTC,clienteID);
                Status_Barra(ERROR_LECTURA);
                for (int i = 0; i < 5; i++)
                {
                  if (Info_Cashless.Reader_Lock(false))
                  {
                    Info_Cashless.Reader_Lock(false);
                  }
                  else
                  {
                    Info_Cashless.Reader_Lock(false);
                  }
                }

                // logg = "";
                // for (int i = 0; i <= 60; i++)
                // {
                //   if (i > 0)
                //     logg += "|";

                //   // Agrega cada byte como 0xHH
                //   if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i] < 0x10)
                //   {
                //     logg += "0x0" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
                //   }
                //   else
                //   {
                //     logg += "0x" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
                //   }
                // }
                // logg.replace(" ", "");
                // Info_Cashless.Log(RTC, "ERROR_DESCARGA_CODIGO: "+String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4],HEX), logg);
                return;
              }
            }
          }
         
        }
        else
        {
          int clienteID = contadores.Get_Client_ID_Transaccion_Int();
          Info_Cashless.Status_Transfer_Download(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4], Buffer_Cashless.Get_RX_AFT(Buffer_RX_Cashless), RTC,clienteID);
          Status_Barra(ERROR_LECTURA);
          for (int i = 0; i < 5; i++)
          {
            if (Info_Cashless.Reader_Lock(false))
            {
              Info_Cashless.Reader_Lock(false);
            }
            else
            {
              Info_Cashless.Reader_Lock(false);
            }
          }


          // logg = "";
          //       for (int i = 0; i <= 60; i++)
          //       {
          //         if (i > 0)
          //           logg += "|";

          //         // Agrega cada byte como 0xHH
          //         if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i] < 0x10)
          //         {
          //           logg += "0x0" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
          //         }
          //         else
          //         {
          //           logg += "0x" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
          //         }
          //       }
          //       logg.replace(" ", "");
          //       Info_Cashless.Log(RTC, "ERROR_DESCARGA_CODIGO: "+String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4],HEX), logg);
          return;
        }
      }
    }
    else
    {
      int clienteID = contadores.Get_Client_ID_Transaccion_Int();	
      Info_Cashless.Status_Transfer_Download(NOT_COMMUNICATION_WITH_THE_MACHINE, Buffer_Cashless.Get_RX_AFT(Buffer_RX_Cashless), RTC,clienteID);
      Status_Barra(ERROR_LECTURA);
      Variables_globales.Set_Variable_Global(Amount_Download_Ready, false); /* Inicial Valor */
      return;
    }
  }
}

void Actualiza_Entradas_Cashless(void)
{
  Flag_Entradas_Cashless_OK = false;
  Comando_Entradas_OK = true;


  /* Contadores  EFT */
  if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2)
  {
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    delay(200);

    for(int i=0; i<5; i++)
    {
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x1D);
      delay(250);
    }

  }/* Contadores  AFT */
  else
  {

    for (int i = 0; i < 4; i++)
    {
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      delay(200);

#ifdef Debug_Encuestas
      Serial.println("Casheable In"); // Casheable in
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x2E);
      Transmite_Poll_Long(0xF7);
      Transmite_Poll_Long(0xE3);

      delay(200);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      delay(200);

      // sendDataa(dat, sizeof(dat)); // transmite sincronización
      // Transmite_Poll(0x00);
      // delay(150);

#ifdef Debug_Encuestas
      Serial.println("Casheable Restricted In"); // Casheable restricted in
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x7E);
      Transmite_Poll_Long(0xF2);

      delay(200);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      delay(200);

#ifdef Debug_Encuestas
      Serial.println("Casheable Nonrestricted In"); // Casheable Nonrestricted in
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x30);
      Transmite_Poll_Long(0x08);
      Transmite_Poll_Long(0x1A);

      // sendDataa(dat, sizeof(dat)); // transmite sincronización
      // Transmite_Poll(0x00);
      // delay(150);
    }
  }
}

void Actualiza_Salidas(void)
{

  Flag_Salidas_Cashless_OK = false;
  Comando_Salidas_OK = true;
  /* Contadores EFT */
  if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2)
  {
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    delay(200);

    for (int i = 0; i < 5; i++)
    {
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x1D);
      delay(250);
    }
  }
  else
  { /* Contadores AFT */

    for (int i = 0; i < 5; i++)
    {
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      delay(200);

#ifdef Debug_Encuestas
      Serial.println("Casheable Out"); // Casheable out
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x32);
      Transmite_Poll_Long(0x1A);
      Transmite_Poll_Long(0x39);

      delay(200);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      delay(200);

      // sendDataa(dat, sizeof(dat)); // transmite sincronización
      // Transmite_Poll(0x00);
      // delay(150);
#ifdef Debug_Encuestas
      Serial.println("Casheable Restricted Out"); // Casheable restricted out
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x33);
      Transmite_Poll_Long(0x93);
      Transmite_Poll_Long(0x28);

      

      delay(200);

      sendDataa(dat, sizeof(dat)); // transmite sincronización
      delay(200);

#ifdef Debug_Encuestas
      Serial.println("Casheable Nonrestricted Out"); // Casheable nonrestricted out
#endif
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2F);
      Transmite_Poll_Long(0x03);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x34);
      Transmite_Poll_Long(0x2C);
      Transmite_Poll_Long(0x5C);
      // delay(150);
      // sendDataa(dat, sizeof(dat)); // transmite sincronización
      // Transmite_Poll(0x00);
      // delay(150);
    }
  }
}

void Actualiza_Entrada_Tito(void)
{
  Flag_Entradas_Tito_OK = false;

  Comando_Entradas_Tito_OK = true;

  for (int i = 0; i < 3; i++)
  {
#ifdef Debug_Encuestas
    Serial.println("Ticket In");
#endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x0D);
    Transmite_Poll_Long(0x6E);
    Transmite_Poll_Long(0xF0);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
  }
}

void Actualiza_Salida_Tito(void)
{

  Flag_Salidas_Tito_OK = false;

  Comando_Salidas_Tito_OK = true;

  for (int i = 0; i < 3; i++)
  {
#ifdef Debug_Encuestas
    Serial.println("Ticket Out");
#endif
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x0E);
    Transmite_Poll_Long(0xF5);
    Transmite_Poll_Long(0xC2);
    delay(100);
    sendDataa(dat, sizeof(dat)); // transmite sincronización
    Transmite_Poll(0x00);
    delay(50);
  }
}

void Transmite_Load_AFT_Maq(void)
{
  
  if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
  {
    Transmite_Load_EFT_Maq();
  }
  else
  {

   // String logg="";

    Ack_Cashless_Transfer_Load = true; /* Espera Ack Maquina */
    char Transfer_Command[63];
    int Size_Command = 60;
    Cashless.Increase_Transaction_Number_ID();

    Transfer_Command[0] = 0x01; // DIRECCION MAQUINA 01
    Transfer_Command[1] = 0x72; // COMANDO 72
    Transfer_Command[2] = 0x3A; // LONGITUD 3A
    Transfer_Command[3] = 0x00; // TRANSFER CODE 00
    Transfer_Command[4] = 0x00; // TRANSACTION INDEX 00
    Transfer_Command[5] = 0x00; // TRANSFER TYPE 00

    // CASHABLES
    Transfer_Command[6] = Cashless.Get_Credit_To_Load()[0]; // MILLONES
    Transfer_Command[7] = Cashless.Get_Credit_To_Load()[1]; // MIL
    Transfer_Command[8] = Cashless.Get_Credit_To_Load()[2]; // PESOS
    Transfer_Command[9] = Cashless.Get_Credit_To_Load()[3]; // 0x01
    Transfer_Command[10] = Cashless.Get_Credit_To_Load()[4];
    // RESTRICTED
    Transfer_Command[11] = Cashless.Get_Credit_To_Load()[5];
    Transfer_Command[12] = Cashless.Get_Credit_To_Load()[6];
    Transfer_Command[13] = Cashless.Get_Credit_To_Load()[7];
    Transfer_Command[14] = Cashless.Get_Credit_To_Load()[8];
    Transfer_Command[15] = Cashless.Get_Credit_To_Load()[9];

    // NO RESTRICTED
    Transfer_Command[16] = Cashless.Get_Credit_To_Load()[10];
    Transfer_Command[17] = Cashless.Get_Credit_To_Load()[11];
    Transfer_Command[18] = Cashless.Get_Credit_To_Load()[12];
    Transfer_Command[19] = Cashless.Get_Credit_To_Load()[13];
    Transfer_Command[20] = Cashless.Get_Credit_To_Load()[14];

    if (Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss))
      // TRANSFER FLAGS
      Transfer_Command[21] = 0b00000111; /* Cobro Solo Cashless */
    else
      Transfer_Command[21] = 0b00000011; /* Permite Cobro Cancel Credit */

    // ASSET NUMEBER
    Transfer_Command[22] = Buffer_Cashless.Get_Key_Register_AFT()[0];
    Transfer_Command[23] = Buffer_Cashless.Get_Key_Register_AFT()[1];
    Transfer_Command[24] = Buffer_Cashless.Get_Key_Register_AFT()[2];
    Transfer_Command[25] = Buffer_Cashless.Get_Key_Register_AFT()[3];

    // REG KEY
    Transfer_Command[26] = Buffer_Cashless.Get_Key_Register_AFT()[0];
    Transfer_Command[27] = Buffer_Cashless.Get_Key_Register_AFT()[1];
    Transfer_Command[28] = Buffer_Cashless.Get_Key_Register_AFT()[2];
    Transfer_Command[29] = Buffer_Cashless.Get_Key_Register_AFT()[3];

    Transfer_Command[30] = Buffer_Cashless.Get_Key_Register_AFT()[4];
    Transfer_Command[31] = Buffer_Cashless.Get_Key_Register_AFT()[5];
    Transfer_Command[32] = Buffer_Cashless.Get_Key_Register_AFT()[6];
    Transfer_Command[33] = Buffer_Cashless.Get_Key_Register_AFT()[7];

    Transfer_Command[34] = Buffer_Cashless.Get_Key_Register_AFT()[8];
    Transfer_Command[35] = Buffer_Cashless.Get_Key_Register_AFT()[9];
    Transfer_Command[36] = Buffer_Cashless.Get_Key_Register_AFT()[10];
    Transfer_Command[37] = Buffer_Cashless.Get_Key_Register_AFT()[11];

    Transfer_Command[38] = Buffer_Cashless.Get_Key_Register_AFT()[12];
    Transfer_Command[39] = Buffer_Cashless.Get_Key_Register_AFT()[13];
    Transfer_Command[40] = Buffer_Cashless.Get_Key_Register_AFT()[14];
    Transfer_Command[41] = Buffer_Cashless.Get_Key_Register_AFT()[15];

    Transfer_Command[42] = Buffer_Cashless.Get_Key_Register_AFT()[16];
    Transfer_Command[43] = Buffer_Cashless.Get_Key_Register_AFT()[17];
    Transfer_Command[44] = Buffer_Cashless.Get_Key_Register_AFT()[18];
    Transfer_Command[45] = Buffer_Cashless.Get_Key_Register_AFT()[19];

    Transfer_Command[46] = 0x07; // LENGT TRASN_ID

    // TRANS_ID
    Transfer_Command[47] = Cashless.Get_Trans_ID()[0];
    Transfer_Command[48] = Cashless.Get_Trans_ID()[1];
    Transfer_Command[49] = Cashless.Get_Trans_ID()[2];
    Transfer_Command[50] = Cashless.Get_Trans_ID()[3];
    Transfer_Command[51] = Cashless.Get_Trans_ID()[4];
    Transfer_Command[52] = Cashless.Get_Trans_ID()[5];
    Transfer_Command[53] = Cashless.Get_Trans_ID()[6];

    // EXPIRATION
    Transfer_Command[54] = 0x00;
    Transfer_Command[55] = 0x00;
    Transfer_Command[56] = 0x00;
    Transfer_Command[57] = 0x00;

    // POLL ID
    Transfer_Command[58] = 0x00;
    Transfer_Command[59] = 0x00;

    // RECEIPT DATA LENGT
    Transfer_Command[60] = 0x00;

    // CalcularCRC_Tmp(); // Calcula CRC
    CalcularCRC_Transfer(Transfer_Command, Size_Command);

    // for (int i = 0; i <= 60; i++)
    // {
    //   if (i > 0)
    //     logg += "|";

    //   // Agrega cada byte como 0xHH
    //   if (Transfer_Command[i] < 0x10)
    //   {
    //     logg += "0x0" + String(Transfer_Command[i], HEX);
    //   }
    //   else
    //   {
    //     logg += "0x" + String(Transfer_Command[i], HEX);
    //   }
    // }
    // logg.replace(" ", "");
    // Info_Cashless.Log(RTC,"TRANSACCION_RECIBIDA_CONTROLADOR:",logg);

    // /* ----------> Interroga AFT Maq <-------------- */

    // sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    // Transmite_Poll_Long(0x72);    //  Comando
    // Transmite_Poll_Long(0x02);
    // Transmite_Poll_Long(0xFF);
    // Transmite_Poll_Long(0x00);
    // Transmite_Poll_Long(0x0F);
    // Transmite_Poll_Long(0x22);
    // delay(250); /* Espera respuesta de maquina */
    // /*-----------------------------------------------*/
    
    Buffer_Cashless.Init_Buffer_Transfer_AFT(true); /* Borra data AFT */
    /* Limpia buffer AFT Para espera de respuesta Maquina */
    sendDataa(dat, sizeof(dat)); // Transmite SYN
    delay(200);


    /*Captura de evento 69 */
    
    Capture_Evento_Cashless=true;
  
    Flag_Transfer_Ok=false;
    Variables_globales.Set_Variable_Global(Machine_Receives_Download_Transfer, false);
    /*-----------> Transmite  AFT Maq <--------------*/
    for (int i = 0; i < 63; i++)
    {
      if (i == 0)
        sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      else
      {
        Transmite_Poll_Long(Transfer_Command[i]);
      }
    }

   

    delay(200);
    int Contador = 0;
    /*  Espera a que  la maquina reciba el comando de  transferencia */
    unsigned long Timout_Break_Response;
    int Stop_Transaccion_Amount_Response = 20000; // Tiempo de espera en milisegundos (5 Seg MAX)
    Timout_Break_Response = millis();
    esp_task_wdt_init(1000000, true);
    esp_task_wdt_add(NULL);
    /* Consulta los creditos de la maquina a descargar Hasta recibir */
    while (!Flag_Transfer_Ok && (millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response))
    {
      esp_task_wdt_reset();

      Contador++;
      if (Contador == 5)
        sendDataa(dat, sizeof(dat)); // Transmite DIR
      if (Contador > 10)
      {
        Transmite_Poll(0x00);
        Contador = 0;
      }
      delay(200);
      // Serial.println("Esperando Ack.......Transfer");
      vTaskDelay(300);
    }

    delay(200);
    bool Error = Flag_Transfer_Ok;
    delay(10);

    if (!Error)
    {

      

      if (Capture_Evento_Cashless_OK)
      {
        Hadle_comunications = true;
        Capture_Evento_Cashless_OK=false;

        //Info_Cashless.Log(RTC,"TRANSACCION_NO_RECIBIDA_POR_LA_MAQUINA_ESPERA_EVENTO");
      }
        
      else
      {
        Info_Cashless.Status_Transfer(NOT_COMMUNICATION_WITH_THE_MACHINE, Buffer_Cashless.Get_Bufffer_Transfer_AFT(), RTC);
        Status_Barra(ERROR_LECTURA);
        for (int i = 0; i < 5; i++)
        {
          if (Info_Cashless.Reader_Lock(false))
          {
            Info_Cashless.Reader_Lock(false);
          }
          else
          {
            Info_Cashless.Reader_Lock(false);
          }
        }
        Status_Barra(500);
        Ack_Cashless_Transfer_Load = false; /* Espera Ack Maquina */

        // logg="";

        // for (int i = 0; i <= 60; i++)
        // {
        //   if (i > 0)
        //     logg += "|";

        //   // Agrega cada byte como 0xHH
        //   if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i] < 0x10)
        //   {
        //     logg += "0x0" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
        //   }
        //   else
        //   {
        //     logg += "0x" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
        //   }
        // }

        // Info_Cashless.Log(RTC,"TRANSACCION_NO_RECIBIDA_POR_LA_MAQUINA",logg);
      }
     
    }
    else
    {

      if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[1] == 0x72)
      {

        bool CurrentStatus = false;
        switch (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4])
        {
        case 0x40:
          /* Transaccion pendiente */
          Info_Cashless.Ack_Transfer_Pending(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4], Buffer_Cashless.Get_Bufffer_Transfer_AFT(), RTC, LOAD_TRANSACTION);
          CurrentStatus = true;
          break;

        case 0x00:
          CurrentStatus = true; /* Maquinas que envian directo la transferencia */
          break;
        case 0x01:
          CurrentStatus = true; /* Maquinas que envian directo la transferencia */
          break;

        default:
          /* Reporta error de transaccion */
          Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
          // Info_Cashless.Init_Player_Tracking_Sesion(contadores.Get_Client_ID_Transaccion());
          Info_Cashless.Status_Transfer(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4], Buffer_Cashless.Get_Bufffer_Transfer_AFT(), RTC);
          Status_Barra(ERROR_LECTURA);
          Status_Barra(500);
          Ack_Cashless_Transfer_Load = false; /* Espera Ack Maquina */
          /* Error en transaccion */
          CurrentStatus = false;
          break;
        }
        delay(500);

        if (CurrentStatus)
        {

          /* Borra data AFT para consulta de estado de transaccion  */
          Buffer_Cashless.Init_Buffer_Transfer_AFT(true);

          /* Sincroniza Maquina */
          sendDataa(dat, sizeof(dat)); // Transmite SYN
          delay(200);
          /* AFT FUNDS TRANSFER STATUS */
          sendDataa(dat4, sizeof(dat4)); // Transmite DIR
          Transmite_Poll_Long(0x72);
          Transmite_Poll_Long(0x02);
          Transmite_Poll_Long(0xFF);
          Transmite_Poll_Long(0x00);
          Transmite_Poll_Long(0x0F);
          Transmite_Poll_Long(0x22);
          delay(500);

          /* Transmite Poll*/
          Transmite_Poll(0x00);

          esp_task_wdt_init(1000000, true);
          esp_task_wdt_add(NULL);
          unsigned long Timout_Break;
          int Stop_Transaccion = 20000; // Tiempo de espera en milisegundos (15 Seg MAX)
          bool Comp = false;
          Timout_Break = millis();

          /* SI esta pendiente o  no se recibe respuesta entra en el While de espera.*/
          while ((Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x40 || Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0xAA) && (millis() - Timout_Break < Stop_Transaccion))
          {
            esp_task_wdt_reset();
            // if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x40)
            // {
            sendDataa(dat, sizeof(dat)); // Transmite SYN
            delay(200);
            /* AFT FUNDS TRANSFER STATUS */
            sendDataa(dat4, sizeof(dat4)); // Transmite DIR
            Transmite_Poll_Long(0x72);
            Transmite_Poll_Long(0x02);
            Transmite_Poll_Long(0xFF);
            Transmite_Poll_Long(0x00);
            Transmite_Poll_Long(0x0F);
            Transmite_Poll_Long(0x22);

            delay(500);

            Transmite_Poll(0x00); /* Transmite Poll*/
            vTaskDelay(300);
            //}
            // Serial.println("Esperando por la transferencia.....!");
          }

          /* Reporta la transaccion OK */
          if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x00 || Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x01)
          {

            Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, true); /*INICIA SESION PLAYER CASHLESS*/
            Info_Cashless.Init_Player_Tracking_Sesion(contadores.Get_Client_ID_Transaccion());
            Info_Cashless.Status_Transfer(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4], Buffer_Cashless.Get_Bufffer_Transfer_AFT(), RTC);
            Ack_Cashless_Transfer_Load = false; /* Espera Ack Maquina */

            // logg = "";
            // for (int i = 0; i <= 60; i++)
            // {
            //   if (i > 0)
            //     logg += "|";

            //   // Agrega cada byte como 0xHH
            //   if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i] < 0x10)
            //   {
            //     logg += "0x0" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
            //   }
            //   else
            //   {
            //     logg += "0x" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
            //   }
            // }

            // Info_Cashless.Log(RTC, "TRANSFERENCIA_REALIZADA_CON_EXITO", logg);
          }
          else
          {
            /* Transaferencia Criticamente pendiente.*/
            if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x40)
            {

              // Serial.println("Transaccion pendiente por reportar ");
              /* Bloquea lector */
              Info_Cashless.Lock_Reader();
              Status_Barra(301); /* Marca lector ocupado */
              /* Bandera para captura de evento! OK */
              Variables_globales.Set_Variable_Global(Event_Load_Cashless_Pending, true);
              /* Indica que la transacción esta pendiente....*/

              /* Guarda Transaccion pendiente en memoria */
              NVS.begin("Config_ESP32", false);
              NVS.putBool("Cash_Pending", true);
              NVS.end();

              // logg = "";
              // for (int i = 0; i <= 60; i++)
              // {
              //   if (i > 0)
              //     logg += "|";

              //   // Agrega cada byte como 0xHH
              //   if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i] < 0x10)
              //   {
              //     logg += "0x0" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
              //   }
              //   else
              //   {
              //     logg += "0x" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
              //   }
              // }

              // Info_Cashless.Log(RTC, "TRANSFERENCIA_CRITICAMENTE_PENDIENTE", logg);
            }
            else
            { /* Reporta error de transaccion */
              Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
              // Info_Cashless.Init_Player_Tracking_Sesion(contadores.Get_Client_ID_Transaccion());
              Info_Cashless.Status_Transfer(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4], Buffer_Cashless.Get_Bufffer_Transfer_AFT(), RTC);
              Status_Barra(ERROR_LECTURA);
              Status_Barra(500);
              Ack_Cashless_Transfer_Load = false; /* Espera Ack Maquina */

              // logg = "";
              // for (int i = 0; i <= 60; i++)
              // {
              //   if (i > 0)
              //     logg += "|";

              //   // Agrega cada byte como 0xHH
              //   if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i] < 0x10)
              //   {
              //     logg += "0x0" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
              //   }
              //   else
              //   {
              //     logg += "0x" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
              //   }
              // }

              // Info_Cashless.Log(RTC, "ERROR_TRANSFERENCIA_CODIGO "+String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4],HEX), logg);
            }
          }
        }
      }else
      {

        /* Reporta error de transaccion */
        Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
        // Info_Cashless.Init_Player_Tracking_Sesion(contadores.Get_Client_ID_Transaccion());
        Info_Cashless.Status_Transfer(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4], Buffer_Cashless.Get_Bufffer_Transfer_AFT(), RTC);
        Status_Barra(ERROR_LECTURA);
        Status_Barra(500);
        Ack_Cashless_Transfer_Load = false; /* Espera Ack Maquina */

        // logg="";
        // for (int i = 0; i <= 60; i++)
        // {
        //   if (i > 0)
        //     logg += "|";

        //   // Agrega cada byte como 0xHH
        //   if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i] < 0x10)
        //   {
        //     logg += "0x0" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
        //   }
        //   else
        //   {
        //     logg += "0x" + String(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[i], HEX);
        //   }
        // }

        // Info_Cashless.Log(RTC,"COMANDO_NO_IDENTIFICADO_PARA_CARGA",logg);
      }
    }
  }
  
  Flag_Transfer_Ok=false;
  Variables_globales.Set_Variable_Global(Machine_Receives_Download_Transfer, false);
  Cashless.Set_Amount_To_Load(0, 0, 0); /* Reset Valores para una */
}

/* Se activa cuando la maquina Genera una transacción pendiente y su tiempo de reporte es mayor a 15sg*/
void Update_Status_Critical_Load(void)
{


  if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2)
  {

    unsigned long Status;

    // sendDataa(dat, sizeof(dat)); // Transmite SYN
    // delay(200);
    esp_task_wdt_reset();
    Status = EFT_Maq_Cashable_Ack();
    delay(500); /* Espera respuesta de maquina */
    // Transmite_Poll(0x00); /*  Transmite poll */
    // vTaskDelay(10);
    Serial.println("Trasmite Ack de transaccion de carga critica");

    if (Status != 0x40)
    {

      if (Status = 0x10)
      {
        Serial.print("Transaccion Realizada con exito: ");
        Serial.println(Buffer_Cashless.Get_Buffer_Transfer_EFT()[4]);

        Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, true); /*INICIA SESION PLAYER CASHLESS*/
        Info_Cashless.Init_Player_Tracking_Sesion(contadores.Get_Client_ID_Transaccion());
        Info_Cashless.Status_Transfer(0x00, Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC);
        Info_Cashless.Lock_Reader();
      }
      else
      {
        Serial.print("Transaccion Rechazada: ");
        Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
        Serial.println(Buffer_Cashless.Get_Buffer_Transfer_EFT()[4]);
        Info_Cashless.Status_Transfer(Buffer_Cashless.Get_Buffer_Transfer_EFT()[4], Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC);
        Status_Barra(ERROR_LECTURA);
        Info_Cashless.Lock_Reader();
        Status_Barra(500);
      }

      /* Termina Trasacción pendiente...*/
      /*-> Actualiza estado pendiente de transaccion <-*/
      Variables_globales.Set_Variable_Global(Event_Load_Cashless_Pending, false);
      NVS.begin("Config_ESP32", false);
      NVS.putBool("Cash_Pending", false);
      NVS.end();
    }
  }
  else
  {
    esp_task_wdt_reset();

    sendDataa(dat, sizeof(dat)); // Transmite SYN
    delay(250);

    esp_task_wdt_reset();
    /* AFT FUNDS TRANSFER STATUS */
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x72);
    Transmite_Poll_Long(0x02);
    Transmite_Poll_Long(0xFF);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x0F);
    Transmite_Poll_Long(0x22);

    delay(500); /* Espera respuesta de maquina */
    Transmite_Poll(0x00); /*  Transmite poll */
    vTaskDelay(10);
    // Serial.print("Esperando confirmación.");
    // Serial.print(".");
    
    esp_task_wdt_reset();
    /* Pregunta si la transaccion dejo de estar pendiente */
    if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] != 0x40)
    {
      /* Reporta estado final de la transaccion critica */
      if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x00 || Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x01)
      {
        Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, true); /*INICIA SESION PLAYER CASHLESS*/
        Info_Cashless.Init_Player_Tracking_Sesion(contadores.Get_Client_ID_Transaccion());
        Info_Cashless.Status_Transfer(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4], Buffer_Cashless.Get_Bufffer_Transfer_AFT(), RTC);
      }
      else
      {
        Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
        // Info_Cashless.Init_Player_Tracking_Sesion(contadores.Get_Client_ID_Transaccion());
        Info_Cashless.Status_Transfer(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4], Buffer_Cashless.Get_Bufffer_Transfer_AFT(), RTC);
        Status_Barra(ERROR_LECTURA);
      }
      /* Termina Trasacción pendiente...*/
      /*-> Actualiza estado pendiente de transaccion <-*/
      Variables_globales.Set_Variable_Global(Event_Load_Cashless_Pending, false);
      NVS.begin("Config_ESP32", false);
      NVS.putBool("Cash_Pending", false);
      NVS.end();
    }
  }

  // sendDataa(dat, sizeof(dat));
  // Transmite_Poll(0x00);
  // delay(500);
  /* Continua */
}





// void Consulta_Transfer()
// {
//   if(Variables_globales.Get_Variable_Global(Event_Dowmload_Cashless_Pending) && !Variables_globales.Get_Variable_Global(Event_Load_Cashless_Pending))
//   {

//   }else if(Variables_globales.Get_Variable_Global(Event_Load_Cashless_Pending) && )
// }

void Update_Status_Critical_Download(void)
{

  if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2)
  {

    // sendDataa(dat, sizeof(dat)); // Transmite SYN
    // delay(200);

    esp_task_wdt_reset();

    unsigned long Status;
    Status = EFT_Maq_Descarga_Ack();
    delay(500); /* Espera respuesta de maquina */
    // Transmite_Poll(0x00); /*  Transmite poll */
    // vTaskDelay(10);

    Serial.println("Trasmite  Ack de transaccion de descarga critica ");

    if (Status != 0x40)
    {
      if (Status = 0x10)
      {
        int clienteID = contadores.Get_Client_ID_Transaccion_Int();	
        Info_Cashless.Status_Transfer_Download(0x00, Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC,clienteID);
        Info_Cashless.Close_Player_Tracking_Sesion(true);
        Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
        // Info_Cashless.Reader_Lock(false);
        contadores.Close_ID_Client_Transaccion();
      }
      else
      {
        int clienteID = contadores.Get_Client_ID_Transaccion_Int();	
        Info_Cashless.Status_Transfer_Download(Buffer_Cashless.Get_Buffer_Transfer_EFT()[4], Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC,clienteID);
        Status_Barra(ERROR_LECTURA);
        for (int i = 0; i < 5; i++)
        {
          if (Info_Cashless.Reader_Lock(false))
          {
            Info_Cashless.Reader_Lock(false);
          }
          else
          {
            Info_Cashless.Reader_Lock(false);
          }
        }
      }

      Variables_globales.Set_Variable_Global(Event_Dowmload_Cashless_Pending, false);
      NVS.begin("Config_ESP32", false);
      NVS.putBool("Cash_Pen_Dow", false);
      NVS.end();
    }
  }
  else
  {

    sendDataa(dat, sizeof(dat)); // Transmite SYN
    delay(200);

    esp_task_wdt_reset();
    /* AFT FUNDS TRANSFER STATUS */
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x72);
    Transmite_Poll_Long(0x02);
    Transmite_Poll_Long(0xFF);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x0F);
    Transmite_Poll_Long(0x22);

    delay(500); /* Espera respuesta de maquina */
    vTaskDelay(10);
    Transmite_Poll(0x00);
    // Serial.print("Esperando confirmación.");
    // Serial.print(".");
    esp_task_wdt_reset();

    if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] != 0x40)
    {
      if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x00 || Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x01)
      {
        int clienteID = contadores.Get_Client_ID_Transaccion_Int();	
        Info_Cashless.Status_Transfer_Download(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4], Buffer_Cashless.Get_RX_AFT(Buffer_RX_Cashless), RTC,clienteID);

        Info_Cashless.Close_Player_Tracking_Sesion(true);
        Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
        // Info_Cashless.Reader_Lock(false);
        contadores.Close_ID_Client_Transaccion();

        for (int i = 0; i < 5; i++)
        {
          if (Info_Cashless.Reader_Lock(false))
          {
            Info_Cashless.Reader_Lock(false);
          }
          else
          {
            Info_Cashless.Reader_Lock(false);
          }
        }
      }
      else
      {
        int clienteID = contadores.Get_Client_ID_Transaccion_Int();	
        Info_Cashless.Status_Transfer_Download(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4], Buffer_Cashless.Get_RX_AFT(Buffer_RX_Cashless), RTC,clienteID);
        Status_Barra(ERROR_LECTURA);
      }

      Variables_globales.Set_Variable_Global(Event_Dowmload_Cashless_Pending, false);
      NVS.begin("Config_ESP32", false);
      NVS.putBool("Cash_Pen_Dow", false);
      NVS.end();
    }
  }

  // sendDataa(dat, sizeof(dat));
  // Transmite_Poll(0x00);
  // delay(500);
}

/* Lista Negra de eventos */
bool Filtro_Eventos_Mq(int Evento)
{
  /*Si el evento esta en lista negra  lo procesa pero no lo envia */

  // if (Variables_globales.Get_Variable_Global(Gmaster_API_Mode))
  // {
  //   switch (Evento)
  //   {
  //   case 0x4F:
  //     return true;
  //     break;

  //   case 0x52:
  //     return true;
  //     break;

  //   case 0x66:
  //     return true;
  //     break;

  //   case 0x8C:
  //     return true;
  //     break;

  //   case 0x2C:
  //     return true;
  //     break;

  //   default:
  //     return true;
  //     break;
  //   }
  //   return false;
  // }else{
  //   return false;
  // }

  return false;
}


/*-------------------------------------------->>>>>*/
unsigned char EFT_Maq_Carga_Aristocrat(void)
{
  char Transfer_Command[10];
  int Size_Command = 7;

  Transfer_Command[0] = 0x01;
  Transfer_Command[1] = 0x69;
  Transfer_Command[2] = Cashless.Get_Trans_ID_EFT();
  Transfer_Command[3] = 0x00;

  Transfer_Command[4] = Cashless.Get_Credit_To_Load()[0];
  Transfer_Command[5] = Cashless.Get_Credit_To_Load()[1];
  Transfer_Command[6] = Cashless.Get_Credit_To_Load()[2];
  Transfer_Command[7] = Cashless.Get_Credit_To_Load()[3];

  /* CRC Datos */
  CalcularCRC_Transfer(Transfer_Command, Size_Command);

  Buffer_Cashless.Init_Buffer_Transfer_EFT(true);

  sendDataa(dat, sizeof(dat)); // Transmite SYN
  delay(200);

  for (int i = 0; i < 10; i++)
  {

    if (i == 0)
    {
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    }
    else
    {
      Transmite_Poll_Long(Transfer_Command[i]);
    }
  }
  int Contador = 0;
  unsigned long Timout_Break_Response;
  int Stop_Transaccion_Amount_Response = 20000; // Tiempo de espera en milisegundos (2 Seg MAX)
  Timout_Break_Response = millis();
  esp_task_wdt_init(1000000, true);
  esp_task_wdt_add(NULL);

  while ((Buffer_Cashless.Get_Buffer_Transfer_EFT()[0] == 0xAA) && (millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response))
  {
    esp_task_wdt_reset();
    vTaskDelay(150);
  }

  int Command_Recv = Buffer_Cashless.Get_Buffer_Transfer_EFT()[1];

  if (Command_Recv==0x69)
  {

    if (Buffer_Cashless.Get_Buffer_Transfer_EFT()[3] == 0x00 && Buffer_Cashless.Get_Buffer_Transfer_EFT()[4] == 0x00)
    {
      Serial.print("Transaccion Realizada con exito: ");
      Serial.println(Buffer_Cashless.Get_Buffer_Transfer_EFT()[4]);

      Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, true); /*INICIA SESION PLAYER CASHLESS*/
      Info_Cashless.Init_Player_Tracking_Sesion(contadores.Get_Client_ID_Transaccion());
      Info_Cashless.Status_Transfer(0x00, Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC);
      Info_Cashless.Lock_Reader();
    }
    // else if (Buffer_Cashless.Get_Buffer_Transfer_EFT()[4] == 0x40)
    // {
    //   /* Transfer pendiente */
    //   Serial.print("Transaccion de Carga Péndiente: ");
    //   Serial.println(Buffer_Cashless.Get_Buffer_Transfer_EFT()[4]);
    //   Info_Cashless.Ack_Transfer_Pending(0x40, Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC, LOAD_TRANSACTION);
    //   // Serial.println("Transaccion pendiente por reportar ");
    //   /* Bloquea lector */
    //   Info_Cashless.Lock_Reader();
    //   Status_Barra(301); /* Marca lector ocupado */
    //   /* Bandera para captura de evento! OK */
    //   Variables_globales.Set_Variable_Global(Event_Load_Cashless_Pending, true);
    //   /* Indica que la transacción esta pendiente....*/

    //   /* Guarda Transaccion pendiente en memoria */
    //   NVS.begin("Config_ESP32", false);
    //   NVS.putBool("Cash_Pending", true);
    //   NVS.end();
    // }
    else
    {
      Serial.print("Transaccion Rechazada: ");
      Serial.println(Buffer_Cashless.Get_Buffer_Transfer_EFT()[4]);
      Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
      Info_Cashless.Status_Transfer(0x87, Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC);
      Status_Barra(ERROR_LECTURA);
      Info_Cashless.Lock_Reader();
      Status_Barra(500);
    }

    unsigned char Status = 0x11;
    char Transfer_Command[10];
    int Size_Command = 7;

    Transfer_Command[0] = 0x01;
    Transfer_Command[1] = 0x69;                        /* COMANDO */
    Transfer_Command[2] = Cashless.Get_Trans_ID_EFT(); /* NUM TRANS */
    Transfer_Command[3] = 0x01;                        /*FLACK ACK*/

    Transfer_Command[4] = Cashless.Get_Credit_To_Load()[0];
    Transfer_Command[5] = Cashless.Get_Credit_To_Load()[1];
    Transfer_Command[6] = Cashless.Get_Credit_To_Load()[2];
    Transfer_Command[7] = Cashless.Get_Credit_To_Load()[3];

    CalcularCRC_Transfer(Transfer_Command, Size_Command);

    Buffer_Cashless.Init_Buffer_Transfer_EFT(true);

    sendDataa(dat, sizeof(dat)); // Transmite SYN
    delay(200);

    for (int i = 0; i < 10; i++)
    {

      if (i == 0)
      {
        sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      }
      else
      {
        Transmite_Poll_Long(Transfer_Command[i]);
      }
    }
  }else
  {

    Serial.print("Primera transaccion no recibida: ");
    Serial.println(Buffer_Cashless.Get_Buffer_Transfer_EFT()[4]);

    Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
    Info_Cashless.Status_Transfer(NOT_COMMUNICATION_WITH_THE_MACHINE, Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC);
    Status_Barra(ERROR_LECTURA);
    Info_Cashless.Lock_Reader();
    Status_Barra(500);
  }
  return 0x00;
}


unsigned char EFT_Maq_Descarga_Aristocrat(void)
{
  unsigned char Status=0x11;
  char Transfer_Command[6];
  int Size_Command = 3;

  Transfer_Command[0] = 0x01;
  Transfer_Command[1] = 0x64; /* COMANDO */
  Transfer_Command[2] = Cashless.Get_Trans_ID_EFT();/* NUM TRANS */
  Transfer_Command[3] = 0x00; /*FLACK ACK*/

  Transfer_Command[4] = 0x00;
  Transfer_Command[5] = 0x00;
   

  CalcularCRC_Transfer(Transfer_Command, Size_Command);

  Buffer_Cashless.Init_Buffer_Transfer_EFT(true);

  sendDataa(dat, sizeof(dat)); // Transmite SYN
  delay(200);

  for (int i = 0; i < 6; i++)
  {

    if(i==0)
    {
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    }else{
      Transmite_Poll_Long(Transfer_Command[i]);
    }
    
  }

  int Contador=0;
  unsigned long Timout_Break_Response;
  int Stop_Transaccion_Amount_Response = 20000; // Tiempo de espera en milisegundos (2 Seg MAX)
  Timout_Break_Response = millis();
  esp_task_wdt_init(1000000, true);
  esp_task_wdt_add(NULL);


  while ((Buffer_Cashless.Get_Buffer_Transfer_EFT()[0] == 0xAA) && (millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response))
  {
    esp_task_wdt_reset();
    

    // Contador++;
    // if (Contador == 1)
    //   sendDataa(dat, sizeof(dat)); // Transmite DIR
    // if (Contador > 1)
    // {
    //   Transmite_Poll(0x00);
    //   Contador = 0;
    // }
    // delay(200);

    vTaskDelay(150);
    // Serial.println("Esperando por la transferencia.....!");
  }

  int Command_Recv=Buffer_Cashless.Get_Buffer_Transfer_EFT()[1];

  if(Command_Recv==0x64)
  {
    if(Buffer_Cashless.Get_Buffer_Transfer_EFT()[3]==0x00 && Buffer_Cashless.Get_Buffer_Transfer_EFT()[4]==0x00)
    {
      int clienteID = contadores.Get_Client_ID_Transaccion_Int();	
      Info_Cashless.Status_Transfer_Download(0x00, Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC,clienteID);
      Info_Cashless.Close_Player_Tracking_Sesion(true);
      Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
      // Info_Cashless.Reader_Lock(false);
      contadores.Close_ID_Client_Transaccion();
    // }else if(Buffer_Cashless.Get_Buffer_Transfer_EFT()[4]==0x40)
    // {
    //   Info_Cashless.Ack_Transfer_Pending(0x40, Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC, DOWNLOAD_TRANSACTION);
    //   // Serial.println("Transaccion pendiente por reportar ");
    //   /* Bloquea lector */
    //   Info_Cashless.Lock_Reader();
    //   Status_Barra(301); /* Marca lector ocupado */
    //   /* Bandera para captura de evento! OK */
    //   /* Indica que la transacción esta pendiente....*/
    //   /* Guarda Transaccion pendiente en memoria */
    //   NVS.begin("Config_ESP32", false);
    //   NVS.putBool("Cash_Pen_Dow", true);
    //   NVS.end();
    //   Variables_globales.Set_Variable_Global(Event_Dowmload_Cashless_Pending, true);
    }
    else{
      /* Puerta abierta, contadores, tilt */
      int clienteID = contadores.Get_Client_ID_Transaccion_Int();	
      Info_Cashless.Status_Transfer_Download(0x87,Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC,clienteID);
      Status_Barra(ERROR_LECTURA);
      for (int i = 0; i < 5; i++)
      {
        if (Info_Cashless.Reader_Lock(false))
        {
          Info_Cashless.Reader_Lock(false);
        }
        else
        {
          Info_Cashless.Reader_Lock(false);
        }
      }
    }

    unsigned char Status = 0x11;
    char Transfer_Command[6];
    int Size_Command = 3;

    Transfer_Command[0] = 0x01;
    Transfer_Command[1] = 0x64;                        /* COMANDO */
    Transfer_Command[2] = Cashless.Get_Trans_ID_EFT(); /* NUM TRANS */
    Transfer_Command[3] = 0x01;                        /*FLACK ACK*/

    Transfer_Command[4] = 0x00;
    Transfer_Command[5] = 0x00;

    CalcularCRC_Transfer(Transfer_Command, Size_Command);

    Buffer_Cashless.Init_Buffer_Transfer_EFT(true);

    // sendDataa(dat, sizeof(dat)); // Transmite SYN
    // delay(200);

    for (int i = 0; i < 6; i++)
    {

      if (i == 0)
      {
        sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      }
      else
      {
        Transmite_Poll_Long(Transfer_Command[i]);
      }
    }
  }
  else
  {
    int clienteID = contadores.Get_Client_ID_Transaccion_Int();	
    Info_Cashless.Status_Transfer_Download(NOT_COMMUNICATION_WITH_THE_MACHINE, Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC,clienteID);
    Status_Barra(ERROR_LECTURA);
    for (int i = 0; i < 5; i++)
    {
      if (Info_Cashless.Reader_Lock(false))
      {
        Info_Cashless.Reader_Lock(false);
      }
      else
      {
        Info_Cashless.Reader_Lock(false);
      }
    }
  }
  return 0x00;
}

/*--------------------------------------------->>>>>*/
unsigned char EFT_Maq_Carga(void)
{

  char Transfer_Command[10];
  int Size_Command = 7;

  Transfer_Command[0] = 0x01;
  Transfer_Command[1] = 0x69;
  Transfer_Command[2] = Cashless.Get_Trans_ID_EFT();
  Transfer_Command[3] = 0x00;

  Transfer_Command[4] = Cashless.Get_Credit_To_Load()[0];
  Transfer_Command[5] = Cashless.Get_Credit_To_Load()[1];
  Transfer_Command[6] = Cashless.Get_Credit_To_Load()[2];
  Transfer_Command[7] = Cashless.Get_Credit_To_Load()[3];

  /* CRC Datos */
  CalcularCRC_Transfer(Transfer_Command, Size_Command);

  Buffer_Cashless.Init_Buffer_Transfer_EFT(true);

  //sendDataa(dat, sizeof(dat)); // Transmite SYN
  //delay(200);

  if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
  {
    sendDataa(dat, sizeof(dat)); // Transmite SYN
    delay(200);
  }

  for (int i = 0; i < 10; i++)
  {

    if(i==0)
    {
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    }else{
      Transmite_Poll_Long(Transfer_Command[i]);
    }
    
  }
  int Contador=0;
  unsigned long Timout_Break_Response;
  int Stop_Transaccion_Amount_Response = 20000; // Tiempo de espera en milisegundos (2 Seg MAX)
  Timout_Break_Response = millis();
  esp_task_wdt_init(1000000, true);
  esp_task_wdt_add(NULL);


  while ((Buffer_Cashless.Get_Buffer_Transfer_EFT()[0] == 0xAA) && (millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response))
  {
    esp_task_wdt_reset();

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
    {
      Contador++;
      if (Contador == 3)
        sendDataa(dat, sizeof(dat)); // Transmite DIR
      if (Contador > 3)
      {
        Transmite_Poll(0x00);
        Contador = 0;
      }
      delay(200);
    }

    vTaskDelay(150);
    // Serial.println("Esperando por la transferencia.....!");
  }

  int Command_Recv=Buffer_Cashless.Get_Buffer_Transfer_EFT()[1];

  if (Command_Recv == 0x69)
  {
    if (Buffer_Cashless.Get_Buffer_Transfer_EFT()[3] == 0x00 && Buffer_Cashless.Get_Buffer_Transfer_EFT()[4] == 0x00)
      return 0x10;                                                 /* Primera transfer OK */
    else if (Buffer_Cashless.Get_Buffer_Transfer_EFT()[4] != 0x00) /* Primera transaccion no OK */
      return Buffer_Cashless.Get_Buffer_Transfer_EFT()[4];
    else
      return 0x11;
  }else{
    return 0x11;
  }
}

unsigned char EFT_Maq_Cashable_Ack(void)
{
  
  unsigned char Status = 0x11;
  char Transfer_Command[10];
  int Size_Command = 7;

  Transfer_Command[0] = 0x01;
  Transfer_Command[1] = 0x69;                        /* COMANDO */
  Transfer_Command[2] = Cashless.Get_Trans_ID_EFT(); /* NUM TRANS */
  Transfer_Command[3] = 0x01;                        /*FLACK ACK*/

  Transfer_Command[4] = Cashless.Get_Credit_To_Load()[0];
  Transfer_Command[5] = Cashless.Get_Credit_To_Load()[1];
  Transfer_Command[6] = Cashless.Get_Credit_To_Load()[2];
  Transfer_Command[7] = Cashless.Get_Credit_To_Load()[3];

  CalcularCRC_Transfer(Transfer_Command, Size_Command);

  Buffer_Cashless.Init_Buffer_Transfer_EFT(true);
  if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
  {
    sendDataa(dat, sizeof(dat)); // Transmite SYN
    delay(200);
  }
  // sendDataa(dat, sizeof(dat)); // Transmite SYN
  // delay(200);

  for (int i = 0; i < 10; i++)
  {

    if (i == 0)
    {
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    }
    else
    {
      Transmite_Poll_Long(Transfer_Command[i]);
    }
  }
  int Contador=0;
  unsigned long Timout_Break_Response;
  int Stop_Transaccion_Amount_Response = 20000; // Tiempo de espera en milisegundos (2 Seg MAX)

  if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
    Stop_Transaccion_Amount_Response=1000;

  Timout_Break_Response = millis();
  esp_task_wdt_init(1000000, true);
  esp_task_wdt_add(NULL);

  while ((Buffer_Cashless.Get_Buffer_Transfer_EFT()[0] == 0xAA) && (millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response))
  {
    esp_task_wdt_reset();
    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
    {
      Contador++;
      if (Contador == 3)
        sendDataa(dat, sizeof(dat)); // Transmite DIR
      if (Contador > 3)
      {
        Transmite_Poll(0x00);
        Contador = 0;
      }
      delay(200);
    }

    vTaskDelay(150);
    // Serial.println("Esperando por la transferencia.....!");
  }

  int Command_Recv = Buffer_Cashless.Get_Buffer_Transfer_EFT()[1];

  if (Command_Recv == 0x69)
  {
    if (Buffer_Cashless.Get_Buffer_Transfer_EFT()[3] == 0x01 && Buffer_Cashless.Get_Buffer_Transfer_EFT()[4] == 0x00)
      return 0x10;

    else if (Buffer_Cashless.Get_Buffer_Transfer_EFT()[4] != 0x00)
      return Buffer_Cashless.Get_Buffer_Transfer_EFT()[4]; /* Transfer no*/

    else{
      return 0xF0;
    }
  }
  else
  {
    return 0x11;
  }
}

unsigned char EFT_Maq_Descarga(void)
{

  unsigned char Status=0x11;
  char Transfer_Command[6];
  int Size_Command = 3;

  Transfer_Command[0] = 0x01;
  Transfer_Command[1] = 0x64; /* COMANDO */
  Transfer_Command[2] = Cashless.Get_Trans_ID_EFT();/* NUM TRANS */
  Transfer_Command[3] = 0x00; /*FLACK ACK*/

  Transfer_Command[4] = 0x00;
  Transfer_Command[5] = 0x00;
   

  CalcularCRC_Transfer(Transfer_Command, Size_Command);

  Buffer_Cashless.Init_Buffer_Transfer_EFT(true);

  // sendDataa(dat, sizeof(dat)); // Transmite SYN
  // delay(200);

  for (int i = 0; i < 6; i++)
  {

    if(i==0)
    {
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    }else{
      Transmite_Poll_Long(Transfer_Command[i]);
    }
    
  }

  int Contador=0;
  unsigned long Timout_Break_Response;
  int Stop_Transaccion_Amount_Response = 20000; // Tiempo de espera en milisegundos (2 Seg MAX)
  Timout_Break_Response = millis();
  esp_task_wdt_init(1000000, true);
  esp_task_wdt_add(NULL);


  while ((Buffer_Cashless.Get_Buffer_Transfer_EFT()[0] == 0xAA) && (millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response))
  {
    esp_task_wdt_reset();

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
    {
      Contador++;
      if (Contador == 1)
        sendDataa(dat, sizeof(dat)); // Transmite DIR
      if (Contador > 1)
      {
        Transmite_Poll(0x00);
        Contador = 0;
      }
      delay(200);
    }

    vTaskDelay(150);
    // Serial.println("Esperando por la transferencia.....!");
  }

  int Command_Recv=Buffer_Cashless.Get_Buffer_Transfer_EFT()[1];

  if(Command_Recv==0x64)
  {
    if(Buffer_Cashless.Get_Buffer_Transfer_EFT()[3]==0x00 && Buffer_Cashless.Get_Buffer_Transfer_EFT()[4]==0x00)
      return 0x10;

    else if(Buffer_Cashless.Get_Buffer_Transfer_EFT()[4]!=0x00)
      return Buffer_Cashless.Get_Buffer_Transfer_EFT()[4]; /* Transfer no*/

    else{
      return 0xF0;
    }

  }else{
    return 0x11;
  }
} 

unsigned char EFT_Maq_Descarga_Ack(void)
{

  unsigned char Status=0x11;
  char Transfer_Command[6];
  int Size_Command = 3;

  Transfer_Command[0] = 0x01;
  Transfer_Command[1] = 0x64; /* COMANDO */
  Transfer_Command[2] = Cashless.Get_Trans_ID_EFT();/* NUM TRANS */
  Transfer_Command[3] = 0x01; /*FLACK ACK*/

  Transfer_Command[4] = 0x00;
  Transfer_Command[5] = 0x00;
   

  CalcularCRC_Transfer(Transfer_Command, Size_Command);
  
  Buffer_Cashless.Init_Buffer_Transfer_EFT(true);

  // sendDataa(dat, sizeof(dat)); // Transmite SYN
  // delay(200);

  for (int i = 0; i < 6; i++)
  {

    if (i == 0)
    {
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    }
    else
    {
      Transmite_Poll_Long(Transfer_Command[i]);
    }
  }

  int Contador=0;

  unsigned long Timout_Break_Response;
  int Stop_Transaccion_Amount_Response = 20000; // Tiempo de espera en milisegundos (2 Seg MAX)
  if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
    Stop_Transaccion_Amount_Response=1000;

  Timout_Break_Response = millis();
  esp_task_wdt_init(1000000, true);
  esp_task_wdt_add(NULL);

  while ((Buffer_Cashless.Get_Buffer_Transfer_EFT()[0] == 0xAA) && (millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response))
  {
    esp_task_wdt_reset();

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
    {
      Contador++;
      if (Contador == 1)
        sendDataa(dat, sizeof(dat)); // Transmite DIR
      if (Contador > 1)
      {
        Transmite_Poll(0x00);
        Contador = 0;
      }
      delay(200);
    }
    
    vTaskDelay(150);
    // Serial.println("Esperando por la transferencia.....!");
  }

  int Command_Recv=Buffer_Cashless.Get_Buffer_Transfer_EFT()[1];

  if(Command_Recv==0x64)
  {
    if(Buffer_Cashless.Get_Buffer_Transfer_EFT()[3]==0x01 && Buffer_Cashless.Get_Buffer_Transfer_EFT()[4]==0x00)
      return 0x10;

    else if(Buffer_Cashless.Get_Buffer_Transfer_EFT()[4]!=0x00)
      return Buffer_Cashless.Get_Buffer_Transfer_EFT()[4]; /* Transfer no*/

    else{
      return 0xF0;
    }
    
  }else{
    return 0x11;
  }
}



void Transmite_Download_EFT_Maq(void)
{

  Cashless.Increase_Transaction_Number_ID_EFT();
  unsigned long Status;

  Status = EFT_Maq_Descarga();
  char Temp_Response_Machine[128];

  for (int i = 0; i < 128; i++)
  {
    Temp_Response_Machine[i] = Buffer_Cashless.Get_Buffer_Transfer_EFT()[i];
  }

  if (Status == 0x10)
  {

    Status = EFT_Maq_Descarga_Ack();

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
      Status = 0x10;

    if (Status == 0x10)
    {

      int clienteID = contadores.Get_Client_ID_Transaccion_Int();

      if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
        Info_Cashless.Status_Transfer_Download(0x00, Temp_Response_Machine, RTC, clienteID);
      else
        Info_Cashless.Status_Transfer_Download(0x00, Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC, clienteID);

      Info_Cashless.Close_Player_Tracking_Sesion(true);
      Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
      // Info_Cashless.Reader_Lock(false);
      contadores.Close_ID_Client_Transaccion();

      return;
    }
    else if (Status == 0x40)
    {
      if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
        Info_Cashless.Ack_Transfer_Pending(0x40, Temp_Response_Machine, RTC, DOWNLOAD_TRANSACTION);
      else
        Info_Cashless.Ack_Transfer_Pending(0x40, Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC, DOWNLOAD_TRANSACTION);
      // Serial.println("Transaccion pendiente por reportar ");
      /* Bloquea lector */
      Info_Cashless.Lock_Reader();
      Status_Barra(301); /* Marca lector ocupado */
      /* Bandera para captura de evento! OK */
      /* Indica que la transacción esta pendiente....*/
      /* Guarda Transaccion pendiente en memoria */
      NVS.begin("Config_ESP32", false);
      NVS.putBool("Cash_Pen_Dow", true);
      NVS.end();
      Variables_globales.Set_Variable_Global(Event_Dowmload_Cashless_Pending, true);
      return;
    }
    else
    {
      int clienteID = contadores.Get_Client_ID_Transaccion_Int();
      if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
        /* Puerta abierta, contadores, tilt */
        Info_Cashless.Status_Transfer_Download(0x87, Temp_Response_Machine, RTC, clienteID);
      else
        Info_Cashless.Status_Transfer_Download(0x87, Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC, clienteID);
      Status_Barra(ERROR_LECTURA);
      for (int i = 0; i < 5; i++)
      {
        if (Info_Cashless.Reader_Lock(false))
        {
          Info_Cashless.Reader_Lock(false);
        }
        else
        {
          Info_Cashless.Reader_Lock(false);
        }
      }

      return;
    }
  }
  else
  {
    int clienteID = contadores.Get_Client_ID_Transaccion_Int();

    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
      Info_Cashless.Status_Transfer_Download(NOT_COMMUNICATION_WITH_THE_MACHINE, Temp_Response_Machine, RTC, clienteID);
    else
      Info_Cashless.Status_Transfer_Download(NOT_COMMUNICATION_WITH_THE_MACHINE, Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC, clienteID);
    Status_Barra(ERROR_LECTURA);
    for (int i = 0; i < 5; i++)
    {
      if (Info_Cashless.Reader_Lock(false))
      {
        Info_Cashless.Reader_Lock(false);
      }
      else
      {
        Info_Cashless.Reader_Lock(false);
      }
    }
    return;
  }
}

void Transmite_Load_EFT_Maq(void)
{
  char Temp_Response_Machine[128];

  Cashless.Increase_Transaction_Number_ID_EFT();
  // Serial.println(" Trasmite Transaccion EFT ");
  unsigned long Status;

  

  Status = EFT_Maq_Carga();
  
  if (Status == 0x10)
  {

    for (int i = 0; i < 128; i++)
    {
      Temp_Response_Machine[i] = Buffer_Cashless.Get_Buffer_Transfer_EFT()[i];
    }
    // Serial.print("Primera transaccion recibida con exito: ");
    // Serial.println(Status);

    

    Status = EFT_Maq_Cashable_Ack();

    // Serial.println("Trasmite Ack de transaccion ");
    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
      Status=0x10;

    if (Status = 0x10)
    {

      if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
      {
        // Serial.print("Transaccion Realizada con exito: ");
        // Serial.println(Buffer_Cashless.Get_Buffer_Transfer_EFT()[4]);

        Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, true); /*INICIA SESION PLAYER CASHLESS*/
        Info_Cashless.Init_Player_Tracking_Sesion(contadores.Get_Client_ID_Transaccion());
        Info_Cashless.Status_Transfer(0x00, Temp_Response_Machine, RTC);
        Info_Cashless.Lock_Reader();
      }
      else
      {
        // Serial.print("Transaccion Realizada con exito: ");
        // Serial.println(Buffer_Cashless.Get_Buffer_Transfer_EFT()[4]);

        Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, true); /*INICIA SESION PLAYER CASHLESS*/
        Info_Cashless.Init_Player_Tracking_Sesion(contadores.Get_Client_ID_Transaccion());
        Info_Cashless.Status_Transfer(0x00, Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC);
        Info_Cashless.Lock_Reader();
      }
    }
    else if (Status == 0x40)
    {
      if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
      {
        Info_Cashless.Ack_Transfer_Pending(0x40,Temp_Response_Machine, RTC, LOAD_TRANSACTION);
      }else
        Info_Cashless.Ack_Transfer_Pending(0x40, Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC, LOAD_TRANSACTION);
      /* Transfer pendiente */
      // Serial.print("Transaccion de Carga Péndiente: ");
      // Serial.println(Buffer_Cashless.Get_Buffer_Transfer_EFT()[4]);
      //Info_Cashless.Ack_Transfer_Pending(0x40, Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC, LOAD_TRANSACTION);
      // Serial.println("Transaccion pendiente por reportar ");
      /* Bloquea lector */
      Info_Cashless.Lock_Reader();
      Status_Barra(301); /* Marca lector ocupado */
      /* Bandera para captura de evento! OK */
      Variables_globales.Set_Variable_Global(Event_Load_Cashless_Pending, true);
      /* Indica que la transacción esta pendiente....*/

      /* Guarda Transaccion pendiente en memoria */
      NVS.begin("Config_ESP32", false);
      NVS.putBool("Cash_Pending", true);
      NVS.end();
    }
    else
    {
      // Serial.print("Transaccion Rechazada: ");
      // Serial.println(Buffer_Cashless.Get_Buffer_Transfer_EFT()[4]);
      

      Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
      if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 17)
        Info_Cashless.Status_Transfer(0x87, Temp_Response_Machine , RTC);
      else
        Info_Cashless.Status_Transfer(0x87, Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC);
      Status_Barra(ERROR_LECTURA);
      Info_Cashless.Lock_Reader();
      Status_Barra(500);
    }
  }
  else
  {
    // Serial.print("Primera transaccion no recibida: ");
    // Serial.println(Status);

    Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
    Info_Cashless.Status_Transfer(NOT_COMMUNICATION_WITH_THE_MACHINE, Buffer_Cashless.Get_Buffer_Transfer_EFT(), RTC);
    Status_Barra(ERROR_LECTURA);
    Info_Cashless.Lock_Reader();
    Status_Barra(500);
  }
}
/* ------------------------------------> TITO <-----------------------------------------*/
unsigned long Validation=1;


/*

Buffer_Cashless.Init_Buffer_TITO_70();
  
  sendDataa(dat4, sizeof(dat4)); // Transmite DIR
  Transmite_Poll_Long(0x70);

  unsigned long Timout_Break_Response;
  int Stop_Transaccion_Amount_Response = 5000; // Tiempo de espera en milisegundos (2 Seg MAX)
  Timout_Break_Response = millis();
  esp_task_wdt_init(1000000, true);
  esp_task_wdt_add(NULL);

  delay(500);

  while ((Buffer_Cashless.Get_Buffer_TITO_70()[1] == 0xAA) && (millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response))
  {
    esp_task_wdt_reset();
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x70);
    if (Buffer_Cashless.Get_Buffer_TITO_70()[1] != 0xAA)
      break;
    vTaskDelay(10);
    // Serial.println("Esperando por la transferencia.....!");
  }

  

  char Amount[5];
  char Validation_Number[19];
  int Ack;
  int Ack2;
  // Comando = 70
  Ack=Buffer_Cashless.Get_Buffer_TITO_70()[1];
  // Ticke status 00
  Ack2=Buffer_Cashless.Get_Buffer_TITO_70()[3];

  // 00 00 00 00 00 00 
  Amount[0]=Buffer_Cashless.Get_Buffer_TITO_70()[4];
  Amount[1]=Buffer_Cashless.Get_Buffer_TITO_70()[5];
  Amount[2]=Buffer_Cashless.Get_Buffer_TITO_70()[6];
  Amount[3]=Buffer_Cashless.Get_Buffer_TITO_70()[7];
  Amount[4]=Buffer_Cashless.Get_Buffer_TITO_70()[8];

  //00
  char Paring_Code=Buffer_Cashless.Get_Buffer_TITO_70()[9];

  //00 22 24 28 67 51 54 15 68

  Validation_Number[0]=Buffer_Cashless.Get_Buffer_TITO_70()[10];
  Validation_Number[1]=Buffer_Cashless.Get_Buffer_TITO_70()[11];
  Validation_Number[2]=Buffer_Cashless.Get_Buffer_TITO_70()[12];
  Validation_Number[3]=Buffer_Cashless.Get_Buffer_TITO_70()[13];
  Validation_Number[4]=Buffer_Cashless.Get_Buffer_TITO_70()[14];
  Validation_Number[5]=Buffer_Cashless.Get_Buffer_TITO_70()[15];
  Validation_Number[6]=Buffer_Cashless.Get_Buffer_TITO_70()[16];
  Validation_Number[7]=Buffer_Cashless.Get_Buffer_TITO_70()[17];
  Validation_Number[8]=Buffer_Cashless.Get_Buffer_TITO_70()[18];
  Validation_Number[9]=Buffer_Cashless.Get_Buffer_TITO_70()[19];
  Buffer_Cashless.Init_Buffer_TITO_70();  Borra respuesta 0xAA 

  bool iSsuscess=false;
  if(Ack==0x70 && Ack2==0x00)
    iSsuscess=true;
  else
    iSsuscess=false;

  if(iSsuscess)
  {
    
  }


*/
void Remove_Ticket_In_Buffer(void)
{
  //Serial.println("Confirma Ticket "); 
  Buffer_Cashless.Init_Buffer_TITO_4D();
  delay(10);

  sendDataa(dat4, sizeof(dat4)); // Transmite DIR
  Transmite_Poll_Long(0x4D);
  Transmite_Poll_Long(0x00);
  Transmite_Poll_Long(0xC2);
  Transmite_Poll_Long(0xAC);


  unsigned long Timout_Break_Response;
  int Stop_Transaccion_Amount_Response = 15000; // Tiempo de espera en milisegundos (2 Seg MAX)
  Timout_Break_Response = millis();
  esp_task_wdt_init(1000000, true);
  esp_task_wdt_add(NULL);

  delay(800);

  while ((Buffer_Cashless.Get_Buffer_TITO_4D()[1] == 0xAA||Buffer_Cashless.Get_Buffer_TITO_4D()[1] == 0x00) && (millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response))
  {
    esp_task_wdt_reset();
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x4D);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0xC2);
    Transmite_Poll_Long(0xAC);
    delay(800);


    if(Buffer_Cashless.Get_Buffer_TITO_4D()[1]!=0xAA && Buffer_Cashless.Get_Buffer_TITO_4D()[1]!=0x00)
      break;

    vTaskDelay(10);
    //Serial.println("Esperando por la transferencia.....!");
  }


  Tito.Status_Ticket_Out(Buffer_Cashless.Get_Buffer_TITO_4D(),RTC);
  Tito.Status_Process_Ticket(false);
}

void copiarDatos(char *destino, int &pos, uint8_t tipo, const char *origen, int longitud)
{
  destino[pos] = tipo;
  pos++;
  destino[pos] = longitud;
  pos++;
  memcpy(&destino[pos], origen, longitud);
  pos += longitud;
}

void Set_Extended_Ticket_Data(void)
{

  String Location = Cashless.Get_Parameter_Print_Ticket(Location_Tick);
  String Adress_1 = Cashless.Get_Parameter_Print_Ticket(Adress_1_Tick);
  String Adress_2 = Cashless.Get_Parameter_Print_Ticket(Adress_2_Tick);
  String Restricted_Ticket_Title = Cashless.Get_Parameter_Print_Ticket(Restricted_Ticket_Title_Tick);
  String Debit_Ticket_Title = Cashless.Get_Parameter_Print_Ticket(Debit_Ticket_Title_Tick);

  // Location="CASINO DESARROLLO";
  // Adress_1="CALLE 1A#12-18";
  // Adress_2="PEREIRA";
  // Restricted_Ticket_Title="CASHOUT TICKET";
  // Debit_Ticket_Title="DEBIT TICKET";

  int Location_Len = Location.length();
  int Adress_1_Len = Adress_1.length();
  int Adress_2_Len = Adress_2.length();
  int Restricted_Ticket_Title_Len = Restricted_Ticket_Title.length();
  int Debit_Ticket_Title_Len = Debit_Ticket_Title.length();


  char COmmando[200];

  /* DIRECCION */
  COmmando[0]=0x01;
  /* COMANDO */
  COmmando[1]=0x7C;

  
  int pos=3;

  /*TAMAÑO COMANDO*/
  COmmando[pos]=0x00;
  pos++;
  COmmando[pos]=Location_Len;
  pos++;

  for(int i=0; i<Location_Len; i++)
  {

    COmmando[pos]=Location[i];
    pos++;
  }

  COmmando[pos]=0x01; /* Code */
  pos++;
  COmmando[pos]=Adress_1_Len; /* Code */
  pos++;
  for(int i=0; i<Adress_1_Len; i++)
  {
    COmmando[pos]=Adress_1[i];
    pos++;
  }

  COmmando[pos] = 0x02; /* Code */
  pos++;
  COmmando[pos] = Adress_2_Len; /* Code */
  pos++;
  for (int i = 0; i < Adress_2_Len; i++)
  {
    COmmando[pos] = Adress_2[i];
    pos++;
  }

  COmmando[pos] = 0x10; /* Code */
  pos++;
  COmmando[pos] = Restricted_Ticket_Title_Len; /* Code */
  pos++;
  for (int i = 0; i < Restricted_Ticket_Title_Len; i++)
  {
    COmmando[pos] = Restricted_Ticket_Title[i];
    pos++;
  }


  COmmando[pos] = 0x20; /* Code */
  pos++;
  COmmando[pos] = Debit_Ticket_Title_Len; /* Code */
  pos++;
  for (int i = 0; i < Debit_Ticket_Title_Len; i++)
  {
    COmmando[pos] = Debit_Ticket_Title[i];
    pos++;
  }


  /* adress-Comando-length-posicion pos++*/
  int New=(pos-3);
  COmmando[2]=New; /*  Comando completo*/
  

  int NewSize=pos+3;
  

  char BufferFinal[NewSize];


  for(int i=0;i<pos; i++)
  {
    BufferFinal[i]=COmmando[i];
  }

  // Serial.println(pos);

  // for(int i=0; i<pos; i++)
  // {
  //   Serial.println(COmmando[i],HEX);
  // }

  int Size_Not_CRC=pos-1;
  CalcularCRC_Transfer(BufferFinal,Size_Not_CRC);



  // for(int i=0; i<pos+2; i++)
  // {
  //   Serial.println(BufferFinal[i],HEX);
  // }

  int Len_Comando=pos+2;

  Buffer_Cashless.Init_Buffer_TITO_7C();
  delay(5);

  for(int i=0; i<Len_Comando;i++)
  {

    if (i == 0)
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    else
    {
      Transmite_Poll_Long(BufferFinal[i]);
    }
  }

  // esp_task_wdt_init(1000000, true);
  // esp_task_wdt_add(NULL);
  // unsigned long Timout_Break;
  // int Stop_Transaccion = 5000; // Tiempo de espera en milisegundos (15 Seg MAX)
  // Timout_Break = millis();

  // while ((Buffer_Cashless.Get_Buffer_TITO_7C()[1] == 0xAA && millis() - Timout_Break < Stop_Transaccion))
  // {
  //   esp_task_wdt_reset();
  //   Serial.println("Esperando respuesta.....");
  //   vTaskDelay(10);
  // }

  // int Comando=Buffer_Cashless.Get_Buffer_TITO_7C()[1];
  // int Ticket_Flag=Buffer_Cashless.Get_Buffer_TITO_7C()[2];
  // switch (Ticket_Flag)
  // {
  // case 0x00:
  //   Serial.println("No se Aplico la configuracion ");
  //   break;
  // case 0x01:
  //   Serial.println("Configuracion aplicada con exito!");
  // break;

  // default:
  //    Serial.println("No respuesta de la maquina ");
  //   break;
  // }
}

void Requerimiento_Ticket_In(void)
{

  //Serial.println("Contador");
  Buffer_Cashless.Init_Buffer_TITO_70();
  
  sendDataa(dat4, sizeof(dat4)); // Transmite DIR
  Transmite_Poll_Long(0x70);

  unsigned long Timout_Break_Response;
  int Stop_Transaccion_Amount_Response = 5000; // Tiempo de espera en milisegundos (2 Seg MAX)
  Timout_Break_Response = millis();
  esp_task_wdt_init(1000000, true);
  esp_task_wdt_add(NULL);

  delay(500);

  while ((Buffer_Cashless.Get_Buffer_TITO_70()[1] == 0xAA) && (millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response))
  {
    esp_task_wdt_reset();
    
    if((Buffer_Cashless.Get_Buffer_TITO_70()[1]==0xAA))
    {
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x70);
    delay(500);
    }

    if (Buffer_Cashless.Get_Buffer_TITO_70()[1] != 0xAA)
      break;
    vTaskDelay(10);
    // Serial.println("Esperando por la transferencia.....!");
  }

  int Ack;
  int Ack2;
  // Comando = 70
  Ack=Buffer_Cashless.Get_Buffer_TITO_70()[1];
  // Ticke status 00
  Ack2=Buffer_Cashless.Get_Buffer_TITO_70()[3];

  bool iSsuscess=false;
  if(Ack==0x70 && Ack2==0x00)
    iSsuscess=true;
  else
    iSsuscess=false;

  if(iSsuscess)
  {
    esp_task_wdt_init(1000000, true);
    esp_task_wdt_add(NULL);
    esp_task_wdt_reset();

    if (Tito.Ticket_Information_Capture(Buffer_Cashless.Get_Buffer_TITO_70(), RTC))
    {
      char command[21];
      int command_Size = 18; /* Without CRC */

      /*Address Machine */
      command[0] = 0x01;
      /* COMMAND */
      command[1] = 0x71;
      /*LENGTH*/
      command[2] = 0x10;
      /* TRANSFER CODE */

      int Transfer_Code=Objeto_Ticket_In_Response["Transfer_Code"];
      command[3] = Transfer_Code;
  
      uint32_t Amount = Objeto_Ticket_In_Response["Transfer_Amount"];

      Tito.Set_Amount_Ticket_Transfer(Amount);
      /* TRANSFER AMOUNT */
      command[4] = Tito.Get_Amount_Ticket_Transfer()[0];
      command[5] = Tito.Get_Amount_Ticket_Transfer()[1];
      command[6] = Tito.Get_Amount_Ticket_Transfer()[2];
      command[7] = Tito.Get_Amount_Ticket_Transfer()[3];
      command[8] = Tito.Get_Amount_Ticket_Transfer()[4];

      /* PARSING CODE */

      int Parsing_Code=Objeto_Ticket_In_Response["Parsing_Code"];
      command[9] = Parsing_Code;

      

      String Val_Number=Objeto_Ticket_In_Response["Numero_Validacion"];
      
      Tito.Set_Validations_Number_Out(Val_Number);
     // 00 22 24 28 67 51 54 15 68
      
      /* SYSTEM ID */
      int System_Id=Objeto_Ticket_In_Response["Validation_System_ID"];
      command[10] = System_Id;
      /*VALIDATION NUMBER*/
      command[11] = Tito.Get_Validatioins_Number_Out()[0];
      command[12] = Tito.Get_Validatioins_Number_Out()[1];
      command[13] = Tito.Get_Validatioins_Number_Out()[2];
      command[14] = Tito.Get_Validatioins_Number_Out()[3];
      command[15] = Tito.Get_Validatioins_Number_Out()[4];
      command[16] = Tito.Get_Validatioins_Number_Out()[5];
      command[17] = Tito.Get_Validatioins_Number_Out()[6];
      command[18] = Tito.Get_Validatioins_Number_Out()[7];

      // /*RESTRICTED EXPIRATION*/
      // command[19] = 0x00;
      // command[20] = 0x00;
      // command[21] = 0x00;
      // command[22] = 0x00;
      // /*POLL ID*/
      // command[23] = 0x00;
      // command[24] = 0x00;
      // /*CRC*/
      command[19] = 0x00;
      command[20] = 0x00;


      //Objeto_Ticket_In_Response.clear();
      Buffer_Cashless.Init_Buffer_TITO_70();
      CalcularCRC_Transfer(command, command_Size);

      for (int i = 0; i < 21; i++)
      {
        if (i == 0)
        {
          sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        }
          
        else
        {

          //Serial.println(command[i],DEC);
          Transmite_Poll_Long(command[i]);
        } 
         
      }

      // delay(500);
      unsigned long Timout_Break_Response;
      int Stop_Transaccion_Amount_Response = 5000; // Tiempo de espera en milisegundos (2 Seg MAX)
      Timout_Break_Response = millis();
      esp_task_wdt_init(1000000, true);
      esp_task_wdt_add(NULL);

      while ((Buffer_Cashless.Get_Buffer_TITO_70()[1] == 0xAA) && (millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response))
      {
        esp_task_wdt_reset();

        if (Buffer_Cashless.Get_Buffer_TITO_70()[1] != 0xAA)
          break;
        vTaskDelay(10);
         //Serial.println("Esperando por la transferencia.....!");
      }

      bool isSuccess = false;
      int Comando = Buffer_Cashless.Get_Buffer_TITO_70()[1];
      if (Comando == 0x71)
        isSuccess = true;

      if (isSuccess)
      {
        Serial.println(" ------------------------> Maquina recibio <-----------------------------");
        /* TICKET  STATUS */
        sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        Transmite_Poll_Long(0x71);
        Transmite_Poll_Long(0x01);
        Transmite_Poll_Long(0xFF);
        Transmite_Poll_Long(0x1F);
        Transmite_Poll_Long(0xD0);

        delay(500);
        esp_task_wdt_init(1000000, true);
        esp_task_wdt_add(NULL);
        unsigned long Timout_Break;
        int Stop_Transaccion = 10000; // Tiempo de espera en milisegundos (15 Seg MAX)
        bool Comp = false;
        Timout_Break = millis();

        if (Buffer_Cashless.Get_Buffer_TITO_70()[3] == 0x40)
        {
         
          Tito.Status_Ticket_In_Data(Buffer_Cashless.Get_Buffer_TITO_70(), RTC);
        }

        while ((Buffer_Cashless.Get_Buffer_TITO_70()[3] == 0x40 || Buffer_Cashless.Get_Buffer_TITO_70()[3] == 0xAA) && (millis() - Timout_Break < Stop_Transaccion))
        {
          esp_task_wdt_reset();
          if (Buffer_Cashless.Get_Buffer_TITO_70()[3] == 0x40)
          {
            /* TICKET  STATUS */
            sendDataa(dat4, sizeof(dat4)); // Transmite DIR
            Transmite_Poll_Long(0x71);
            Transmite_Poll_Long(0x01);
            Transmite_Poll_Long(0xFF);
            Transmite_Poll_Long(0x1F);
            Transmite_Poll_Long(0xD0);

            delay(500); /* Espera respuesta de maquina */
            vTaskDelay(10);
          }
          // Serial.println("Esperando por la transferencia.....!");
        }

        Tito.Status_Ticket_In_Data(Buffer_Cashless.Get_Buffer_TITO_70(), RTC);

      }else{
        Serial.println(" ------------------------> Maquina no recibio <-----------------------------");
      }

      //Tito.Set_Status_Ticket_In(false);
    }
    else
    {
      Serial.println("Rej");
      char command[21];
      int command_Size = 18; /* Without CRC */

      /*Address Machine */
      command[0] = 0x01;
      /* COMMAND */
      command[1] = 0x71;
      /*LENGTH*/
      command[2] = 0x10;
      /* TRANSFER CODE */

      command[3] = 0x82;

      /* TRANSFER AMOUNT */
      command[4] = 0x00;
      command[5] = 0x00;
      command[6] = 0x00;
      command[7] = 0x00;
      command[8] = 0x00;

      /* PARSING CODE */
      command[9] = 0x00;

     
      /* SYSTEM ID */
      command[10] = 0x00;
      /*VALIDATION NUMBER*/
      command[11] = 0x00;
      command[12] = 0x00;
      command[13] = 0x00;
      command[14] = 0x00;
      command[15] = 0x00;
      command[16] = 0x00;
      command[17] = 0x00;
      command[18] = 0x00;

      // /*RESTRICTED EXPIRATION*/
      // command[19] = 0x00;
      // command[20] = 0x00;
      // command[21] = 0x00;
      // command[22] = 0x00;
      // /*POLL ID*/
      // command[23] = 0x00;
      // command[24] = 0x00;
      // /*CRC*/
      command[19] = 0x00;
      command[20] = 0x00;

      Buffer_Cashless.Init_Buffer_TITO_70();
      CalcularCRC_Transfer(command, command_Size);
      for (int i = 0; i < 3; i++)
      {
        for (int i = 0; i < 21; i++)
        {
          if (i == 0)
          {
            sendDataa(dat4, sizeof(dat4)); // Transmite DIR
          }

          else
          {

            // Serial.println(command[i],DEC);
            Transmite_Poll_Long(command[i]);
          }
        }
      }
    }
  }
}

void Command_Reedemed_Ticket(void)
{
  char command[30];
  int command_Size = 10; /* Without CRC */

  /*DIRECCION MAQUINA */
  command[0] = 0x01;
  /* COMANDO */
  command[1] = 0x71;
  /*LENGTH*/
  command[2] = 0x00;
  /* TRANSFER CODE */
  command[3] = 0x00; 

  /* TRANSFER AMOUNT */
  command[4] = 0x00;
  command[5] = 0x00;
  command[6] = 0x00;
  command[7] = 0x00;
  command[8] = 0x00;
  /* PARSING CODE */
  command[9] = 0x00;

  /* SYSTEM ID */
  command[10] = 0x00;

  /*VALIDATION NUMBER*/
  command[11] = 0x00;
  command[12] = 0x00;
  command[13] = 0x00;
  command[14] = 0x00;
  command[15] = 0x00;
  command[16] = 0x00;
  command[17] = 0x00;
  command[18] = 0x00;
  
  /*RESTRICTED EXPIRATION*/
  command[15] = 0x00;
  command[16] = 0x00;
  command[17] = 0x00;
  command[18] = 0x00;
  /*POLL ID*/
  command[17] = 0x00;
  command[18] = 0x00;
  /*CRC*/
  command[17] = 0x00;
  command[18] = 0x00;

  Buffer_Cashless.Init_Buffer_TITO_70();
  CalcularCRC_Transfer(command, command_Size);

  for (int i = 0; i < 13; i++)
  {
    if (i == 0)
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    else
      Transmite_Poll_Long(command[i]);
  }

  // delay(500);
  unsigned long Timout_Break_Response;
  int Stop_Transaccion_Amount_Response = 5000; // Tiempo de espera en milisegundos (2 Seg MAX)
  Timout_Break_Response = millis();
  esp_task_wdt_init(1000000, true);
  esp_task_wdt_add(NULL);

  while ((Buffer_Cashless.Get_Buffer_TITO_70()[1] == 0xAA) && (millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response))
  {
    esp_task_wdt_reset();

    if (Buffer_Cashless.Get_Buffer_TITO_70()[1] != 0xAA)
      break;
    vTaskDelay(10);
    // Serial.println("Esperando por la transferencia.....!");
  }

  bool isSuccess=false;
  int Comando=Buffer_Cashless.Get_Buffer_TITO_70()[1];
  if(Comando==0x71)
    isSuccess=true;

  if(isSuccess)
  {

    Tito.Status_Ticket_In_Data(Buffer_Cashless.Get_Buffer_TITO_70(),RTC);

    /* TICKET  STATUS */
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x71);
    Transmite_Poll_Long(0x01);
    Transmite_Poll_Long(0xFF);
    Transmite_Poll_Long(0x1F);
    Transmite_Poll_Long(0xD0);


    delay(500);
    esp_task_wdt_init(1000000, true);
    esp_task_wdt_add(NULL);
    unsigned long Timout_Break;
    int Stop_Transaccion = 10000; // Tiempo de espera en milisegundos (15 Seg MAX)
    bool Comp = false;
    Timout_Break = millis();

    if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x40)
    {
      Info_Cashless.Ack_Transfer_Pending(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4], Buffer_Cashless.Get_Bufffer_Transfer_AFT(), RTC, LOAD_TRANSACTION);
    }

    while ((Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x40 || Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0xAA) && (millis() - Timout_Break < Stop_Transaccion))
    {
      esp_task_wdt_reset();
      if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x40)
      {
        /* AFT FUNDS TRANSFER STATUS */
        sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        Transmite_Poll_Long(0x72);
        Transmite_Poll_Long(0x02);
        Transmite_Poll_Long(0xFF);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x0F);
        Transmite_Poll_Long(0x22);

        delay(500); /* Espera respuesta de maquina */
        vTaskDelay(10);
      }
     // Serial.println("Esperando por la transferencia.....!");
    }
  }
    

  Tito.Set_Status_Ticket_In(false);
}

uint8_t decimalToBCD(int decimal)
{
  return ((decimal / 10) << 4) | (decimal % 10); // Conversión a BCD
}

void Critial_Question(void)
{

  if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 1 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 3 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 5 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 10)
  {

    esp_task_wdt_init(1000000, true);
    esp_task_wdt_add(NULL);

    for (int i = 0; i < 10; i++)
    {
      //Serial.println("Encuestas... Critica 10 veces ");
      esp_task_wdt_reset();
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x10);
      delay(200);
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x2D);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0x00);
      Transmite_Poll_Long(0xFF);
      Transmite_Poll_Long(0xE0);
      esp_task_wdt_reset();
      delay(200);
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      Transmite_Poll(0x1A);
      delay(200);
      
    }

    if(Flag_Change_Counters_Response)
      Flag_Change_Counters_One=true;
  }
}

void Requerimiento_TITO(void)
{

  /* Habilitar No lectura de evento  */
  /* Reset de proceso */
  Tito.Status_Process_Ticket(true);

  esp_task_wdt_init(1000000, true);
  esp_task_wdt_add(NULL);
  esp_task_wdt_reset();

  Serial.println("Evento 57 Atendido");

  if (Tito.Generate_Key_Ticket_Out())
  {
    /* Limpia buffer para la transaccion */
    Buffer_Cashless.Init_Buffer_TITO();

    /* ------------------------> Encuesta tipo R 57 <-------------------------------------------- */
    for (int i = 0; i < 1; i++)
    {
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x57);
      delay(500);
    }

    unsigned long Timout_Break_Response;
    int Stop_Transaccion_Amount_Response = 2000; // Tiempo de espera en milisegundos (2 Seg MAX)
    Timout_Break_Response = millis();
    esp_task_wdt_init(1000000, true);
    esp_task_wdt_add(NULL);

    while ((Buffer_Cashless.Get_Buffer_TITO()[1] == 0xAA) && (millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response))
    {
      esp_task_wdt_reset();
      delay(200);
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      Transmite_Poll_Long(0x57);

      if (Buffer_Cashless.Get_Buffer_TITO()[1] != 0xAA)
        break;
      vTaskDelay(10);
      // Serial.println("Esperando por la transferencia.....!");
    }
    /*--------------------------------------------------------------------------------------------------*/

    /* Comando */
    int Handle = Buffer_Cashless.Get_Buffer_TITO()[1];

    bool Requerimiento = false;
    if (Handle == 0x57)
      Requerimiento = true;

    /* ----------------------> Verifica si la maquina recibio la encuesta Tipo R 57 <-------------------*/
    if (Requerimiento)
    {
      Tito.Increase_Transaction_Number_ID_Tito();
      Serial.println("------------------------> Comando 57 Rebido por la maquina <----------------------------------");

      /* Type Cashout */
      int Type_Ticket = Buffer_Cashless.Get_Buffer_TITO()[2];
      char Amount[6];
      /* Amount */
      Amount[0] = Buffer_Cashless.Get_Buffer_TITO()[3];
      Amount[1] = Buffer_Cashless.Get_Buffer_TITO()[4];
      Amount[2] = Buffer_Cashless.Get_Buffer_TITO()[5];
      Amount[3] = Buffer_Cashless.Get_Buffer_TITO()[6];
      Amount[4] = Buffer_Cashless.Get_Buffer_TITO()[7];
      Buffer_Cashless.Init_Buffer_TITO();

      char Host_Command[13];
      int Host_Size = 10;

      /*DIRECCION MAQUINA */
      Host_Command[0] = 0x01;
      /* COMANDO */
      Host_Command[1] = 0x58;
      /*VALIDATION SYSTEM ID */
      
      Host_Command[2] = decimalToBCD(Tito.Get_Validacion_System_ID());
      /* AMOUNT*/
      Host_Command[3] = Tito.Get_Validacion_Number()[0];
      Host_Command[4] = Tito.Get_Validacion_Number()[1];
      Host_Command[5] = Tito.Get_Validacion_Number()[2];
      Host_Command[6] = Tito.Get_Validacion_Number()[3];
      Host_Command[7] = Tito.Get_Validacion_Number()[4];
      Host_Command[8] = Tito.Get_Validacion_Number()[5];
      Host_Command[9] = Tito.Get_Validacion_Number()[6];
      Host_Command[10]= Tito.Get_Validacion_Number()[7];

      /* CRC */
      Host_Command[11] = 0x00;
      Host_Command[12] = 0x00;

      // CalcularCRC_Tmp(); // Calcula CRC
      CalcularCRC_Transfer(Host_Command, Host_Size);
      Buffer_Cashless.Init_Buffer_TITO_58();
      Buffer_Cashless.Init_Buffer_TITO_3D_3E();
      delay(1);

      for (int i = 0; i < 13; i++)
      {
        if (i == 0)
          sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        else
          Transmite_Poll_Long(Host_Command[i]);
      }

      // delay(500);
      unsigned long Timout_Break_Response;
      int Stop_Transaccion_Amount_Response = 5000; // Tiempo de espera en milisegundos (2 Seg MAX)
      Timout_Break_Response = millis();
      esp_task_wdt_init(1000000, true);
      esp_task_wdt_add(NULL);

      while ((Buffer_Cashless.Get_Buffer_TITO_58()[1] == 0xAA) && (millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response))
      {
        esp_task_wdt_reset();

        if (Buffer_Cashless.Get_Buffer_TITO_58()[1] != 0xAA)
          break;
        vTaskDelay(10);
        // Serial.println("Esperando por la transferencia.....!");
      }

      bool Test = false;
      int Comando = Buffer_Cashless.Get_Buffer_TITO_58()[1];
      int Status = Buffer_Cashless.Get_Buffer_TITO_58()[2];
      Buffer_Cashless.Init_Buffer_TITO_58();
     

      if (Comando == 0x58)
      {
        /* Recibio la data */
        Serial.println("------------------------> Comando 58 Rebido por la maquina <----------------------------------");
        if (Status == 0x00)
        {
          //Serial.println(" Ticket OK ");
          Tito.Update_Ticket(Status, Type_Ticket);
          Tito.Set_Flag_Ticket_Out_Pending(true); 
        }
        else
        {
          /* 0x80 y 0x81*/
          Serial.println("Error Generando Ticket ");
          Tito.Update_Ticket(0xFF, 0xAA);
          Tito.Set_Flag_Ticket_Out_Pending(true);
          Tito.Status_Process_Ticket(false);
        }
      }
      else
      {
        Tito.Update_Ticket(0xFF, 0xAA);
        Tito.Set_Flag_Ticket_Out_Pending(true);
        Serial.println("Comando 58 no recibido ");
        Tito.Status_Process_Ticket(false);
      }
    }
    else
    {
      Serial.println("Requerimiento no atendido ");
    }
  }else
  {
    Tito.Status_Process_Ticket(false);
  }

  //Tito.Status_Process_Ticket(false); /* Reset de proceso */
}

void Tito_Expiration_Ticket(void)
{
  
  // 01 7B 08 00 FF 00 FF 00 10 00 10 6C F6
  //Serial.println("Extend ticket");
  char Host_Command[13];
  int Host_Size = 10;

  /*DIRECCION MAQUINA */
  Host_Command[0] = 0x01;
  /* COMANDO */
  Host_Command[1] = 0x7B;
  Host_Command[2] = 0x08;


  Host_Command[3] = 0x00;
  Host_Command[4] = 0x00;

  Host_Command[5] = 0x00;
  Host_Command[6] = 0x00;

  
  Host_Command[7] = Buffer_Cashless.Get_Buffer_TITO_Data()[0];
  Host_Command[8] = Buffer_Cashless.Get_Buffer_TITO_Data()[1];

  Host_Command[9] =  Buffer_Cashless.Get_Buffer_TITO_Data()[2];
  Host_Command[10] = Buffer_Cashless.Get_Buffer_TITO_Data()[3];

  Host_Command[11] = 0x00;
  Host_Command[12] = 0x00;

 
  /* Limpia buffer para la transaccion */
  Buffer_Cashless.Init_Buffer_TITO_7B();
  CalcularCRC_Transfer(Host_Command, Host_Size);
  delay(1);

  for (int i = 0; i < 13; i++)
  {
    if (i == 0)
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    else
    {
      Transmite_Poll_Long(Host_Command[i]);
      //Serial.println(Host_Command[i],DEC);
    }
      
  }
}

void Host_Control_Update()
{

  char Transfer_Command[63];
  int Size_Command = 60;
  Cashless.Increase_Transaction_Number_ID();

  Transfer_Command[0] = 0x01; // DIRECCION MAQUINA 01
  Transfer_Command[1] = 0x72; // COMANDO 72
  Transfer_Command[2] = 0x3A; // LONGITUD 3A
  Transfer_Command[3] = 0x00; // TRANSFER CODE 00
  Transfer_Command[4] = 0x00; // TRANSACTION INDEX 00
  Transfer_Command[5] = 0x00; // TRANSFER TYPE 00

  // CASHABLES
  Transfer_Command[6] = 0x00; // MILLONES
  Transfer_Command[7] = 0x00; // MIL
  Transfer_Command[8] = 0x00; // PESOS
  Transfer_Command[9] = 0x00; // 0x01
  Transfer_Command[10] = 0x00;
  // RESTRICTED
  Transfer_Command[11] = 0x00;
  Transfer_Command[12] = 0x00;
  Transfer_Command[13] = 0x00;
  Transfer_Command[14] = 0x00;
  Transfer_Command[15] = 0x00;

  // NO RESTRICTED
  Transfer_Command[16] = 0x00;
  Transfer_Command[17] = 0x00;
  Transfer_Command[18] = 0x00;
  Transfer_Command[19] = 0x00;
  Transfer_Command[20] = 0x00;

  // TRANSFER FLAGS

  if(Variables_globales.Get_Variable_Global(Descarga_Solo_Cashelss))
    Transfer_Command[21] = 0b00000111;
  else
    Transfer_Command[21] = 0b00000011;

  // ASSET NUMEBER
  Transfer_Command[22] = Buffer_Cashless.Get_Key_Register_AFT()[0];
  Transfer_Command[23] = Buffer_Cashless.Get_Key_Register_AFT()[1];
  Transfer_Command[24] = Buffer_Cashless.Get_Key_Register_AFT()[2];
  Transfer_Command[25] = Buffer_Cashless.Get_Key_Register_AFT()[3];

  // REG KEY
  Transfer_Command[26] = Buffer_Cashless.Get_Key_Register_AFT()[0];
  Transfer_Command[27] = Buffer_Cashless.Get_Key_Register_AFT()[1];
  Transfer_Command[28] = Buffer_Cashless.Get_Key_Register_AFT()[2];
  Transfer_Command[29] = Buffer_Cashless.Get_Key_Register_AFT()[3];

  Transfer_Command[30] = Buffer_Cashless.Get_Key_Register_AFT()[4];
  Transfer_Command[31] = Buffer_Cashless.Get_Key_Register_AFT()[5];
  Transfer_Command[32] = Buffer_Cashless.Get_Key_Register_AFT()[6];
  Transfer_Command[33] = Buffer_Cashless.Get_Key_Register_AFT()[7];

  Transfer_Command[34] = Buffer_Cashless.Get_Key_Register_AFT()[8];
  Transfer_Command[35] = Buffer_Cashless.Get_Key_Register_AFT()[9];
  Transfer_Command[36] = Buffer_Cashless.Get_Key_Register_AFT()[10];
  Transfer_Command[37] = Buffer_Cashless.Get_Key_Register_AFT()[11];

  Transfer_Command[38] = Buffer_Cashless.Get_Key_Register_AFT()[12];
  Transfer_Command[39] = Buffer_Cashless.Get_Key_Register_AFT()[13];
  Transfer_Command[40] = Buffer_Cashless.Get_Key_Register_AFT()[14];
  Transfer_Command[41] = Buffer_Cashless.Get_Key_Register_AFT()[15];

  Transfer_Command[42] = Buffer_Cashless.Get_Key_Register_AFT()[16];
  Transfer_Command[43] = Buffer_Cashless.Get_Key_Register_AFT()[17];
  Transfer_Command[44] = Buffer_Cashless.Get_Key_Register_AFT()[18];
  Transfer_Command[45] = Buffer_Cashless.Get_Key_Register_AFT()[19];

  Transfer_Command[46] = 0x07; // LENGT TRASN_ID

  // TRANS_ID
  Transfer_Command[47] = Cashless.Get_Trans_ID()[0];
  Transfer_Command[48] = Cashless.Get_Trans_ID()[1];
  Transfer_Command[49] = Cashless.Get_Trans_ID()[2];
  Transfer_Command[50] = Cashless.Get_Trans_ID()[3];
  Transfer_Command[51] = Cashless.Get_Trans_ID()[4];
  Transfer_Command[52] = Cashless.Get_Trans_ID()[5];
  Transfer_Command[53] = Cashless.Get_Trans_ID()[6];

  // EXPIRATION
  Transfer_Command[54] = 0x00;
  Transfer_Command[55] = 0x00;
  Transfer_Command[56] = 0x00;
  Transfer_Command[57] = 0x00;

  // POLL ID
  Transfer_Command[58] = 0x00;
  Transfer_Command[59] = 0x00;

  // RECEIPT DATA LENGT
  Transfer_Command[60] = 0x00;

  // CalcularCRC_Tmp(); // Calcula CRC
  CalcularCRC_Transfer(Transfer_Command, Size_Command);

  // /* ----------> Interroga AFT Maq <-------------- */

  // sendDataa(dat4, sizeof(dat4)); // Transmite DIR
  // Transmite_Poll_Long(0x72);    //  Comando
  // Transmite_Poll_Long(0x02);
  // Transmite_Poll_Long(0xFF);
  // Transmite_Poll_Long(0x00);
  // Transmite_Poll_Long(0x0F);
  // Transmite_Poll_Long(0x22);
  // delay(250); /* Espera respuesta de maquina */
  // /*-----------------------------------------------*/

  sendDataa(dat, sizeof(dat)); // Transmite SYN
  delay(200);

  Ack_Cashless_Transfer_Load = true;
  Flag_Transfer_Ok=false;
  Variables_globales.Set_Variable_Global(Machine_Receives_Download_Transfer, false);
  /*-----------> Transmite  AFT Maq <--------------*/
  for (int i = 0; i < 63; i++)
  {
    if (i == 0)
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    else
    {
      Transmite_Poll_Long(Transfer_Command[i]);
    }
  }

  delay(200);
  int Contador = 0;
  /*  Espera a que  la maquina reciba el comando de  transferencia */
  unsigned long Timout_Break_Response;
  int Stop_Transaccion_Amount_Response = 15000; // Tiempo de espera en milisegundos (5 Seg MAX)
  Timout_Break_Response = millis();
  esp_task_wdt_init(1000000, true);
  esp_task_wdt_add(NULL);
  /* Consulta los creditos de la maquina a descargar Hasta recibir */
  while (!Flag_Transfer_Ok && (millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response))
  {
    esp_task_wdt_reset();

    Contador++;
    if (Contador == 1)
      sendDataa(dat, sizeof(dat)); // Transmite DIR
    if (Contador > 1)
    {
      Transmite_Poll(0x00);
      Contador = 0;
    }
    delay(200);
    // Serial.println("Esperando Ack.......Transfer");
    vTaskDelay(300);
  }

  bool Error = Variables_globales.Get_Variable_Global(Machine_Receives_Download_Transfer);
  Variables_globales.Set_Variable_Global(Machine_Receives_Download_Transfer, false);

  if (!Error)
  {
    
  }
  else
  {
    sendDataa(dat, sizeof(dat)); // Transmite SYN
    delay(200);
    /* AFT FUNDS TRANSFER STATUS */
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x72);
    Transmite_Poll_Long(0x02);
    Transmite_Poll_Long(0xFF);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x0F);
    Transmite_Poll_Long(0x22);
    delay(500);

    Transmite_Poll(0x00); /* Transmite Poll*/

    esp_task_wdt_init(1000000, true);
    esp_task_wdt_add(NULL);
    unsigned long Timout_Break;
    int Stop_Transaccion = 15000; // Tiempo de espera en milisegundos (15 Seg MAX)
    bool Comp = false;
    Timout_Break = millis();

    if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x40)
    {
      Info_Cashless.Ack_Transfer_Pending(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4], Buffer_Cashless.Get_Bufffer_Transfer_AFT(), RTC, LOAD_TRANSACTION);
    }

    while ((Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x40 || Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0xAA) && (millis() - Timout_Break < Stop_Transaccion))
    {
      esp_task_wdt_reset();
      if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x40)
      {

        sendDataa(dat, sizeof(dat)); // Transmite SYN
        delay(200);
        /* AFT FUNDS TRANSFER STATUS */
        sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        Transmite_Poll_Long(0x72);
        Transmite_Poll_Long(0x02);
        Transmite_Poll_Long(0xFF);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x0F);
        Transmite_Poll_Long(0x22);

        delay(500);

        Transmite_Poll(0x00); /* Transmite Poll*/

        vTaskDelay(300);
      }
      // Serial.println("Esperando por la transferencia.....!");
    }

    if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x00 || Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x01)
    {
      
    }
    else
    {
     
    }
  }
}

void Init_Reg_Cashless(void)
{
  // sendDataa(dat4, sizeof(dat4)); // Transmite DIR
  // Transmite_Poll_Long(0x73);
  // Transmite_Poll_Long(0x01);
  // Transmite_Poll_Long(0xFF);
  // Transmite_Poll_Long(0xA7);
  // Transmite_Poll_Long(0x65);

  // delay(250);

  // Serial.println("Estado de registro: "+ String(Buffer_Cashless.Get_Buffer_Reg_AFT()[3],HEX));

  // Declaración de Buff_Tx

  sendDataa(dat, sizeof(dat)); // Transmite SYN
  delay(250);

  // sendDataa(dat4, sizeof(dat4)); // Transmite DIR
  // Transmite_Poll_Long(0x73);
  // Transmite_Poll_Long(0x01);
  // Transmite_Poll_Long(0xFF);
  // Transmite_Poll_Long(0xA7);
  // Transmite_Poll_Long(0x65);

  // delay(250);

  char Buff_Tx[34];
  int Host_Size = 31;

  // Inicialización de Buff_Tx
  Buffer_Cashless.Init_Buffer_Reg();
  Genera_Registro_Maquina();
  // Serial.println("INICIALIZA REGISTRO....");

  Buff_Tx[0] = 0x01;
  Buff_Tx[1] = 0x73;
  Buff_Tx[2] = 0x1D;
  Buff_Tx[3] = 0x00;
  /* Asset */
  Buff_Tx[4] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[0];
  Buff_Tx[5] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[1];
  Buff_Tx[6] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[2];
  Buff_Tx[7] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[3];

  /* Key */
  /* Asset */
  Buff_Tx[8] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[0];
  Buff_Tx[9] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[1];
  Buff_Tx[10] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[2];
  Buff_Tx[11] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[3];
  /* POS ID */
  Buff_Tx[12] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[4];
  Buff_Tx[13] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[5];
  Buff_Tx[14] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[6];
  Buff_Tx[15] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[7];
  /* MAC */
  Buff_Tx[16] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[8];
  Buff_Tx[17] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[9];
  Buff_Tx[18] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[10];
  Buff_Tx[19] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[11];
  Buff_Tx[20] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[12];
  Buff_Tx[21] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[13];
  /* FECHA */
  Buff_Tx[22] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[14];
  Buff_Tx[23] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[15];
  Buff_Tx[24] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[16];
  Buff_Tx[25] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[17];
  Buff_Tx[26] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[18];
  Buff_Tx[27] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[19];
  /* POS ID */
  Buff_Tx[28] = 0x01;
  Buff_Tx[29] = 0x00;
  Buff_Tx[30] = 0x00;
  Buff_Tx[31] = 0x00;

  CalcularCRC_Transfer(Buff_Tx, Host_Size);

  sendDataa(dat, sizeof(dat)); // Transmite SYN
  delay(250);

  for (int i = 0; i < 34; i++)
  {
    if (i == 0)
    {
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    }
    else
    {
      Transmite_Poll_Long(Buff_Tx[i]);
    }
  }
  delay(500);
  unsigned long Timout_Break_Response;
  int Stop_Transaccion_Amount_Response = 15000; // Tiempo de espera en milisegundos (2 Seg MAX)
  Timout_Break_Response = millis();
  esp_task_wdt_init(1000000, true);
  esp_task_wdt_add(NULL);

  int Contador = 0;

  while ((Buffer_Cashless.Get_Buffer_Reg_AFT()[3] == 0xAA) && (millis() - Timout_Break_Response < Stop_Transaccion_Amount_Response))
  {

    esp_task_wdt_reset();
    // Contador++;
    // if(Contador==1)
    //   sendDataa(dat, sizeof(dat)); // Transmite DIR
    // if(Contador>1)
    // {
    //   Transmite_Poll(0x00);
    //   Contador=0;
    // }
    // delay(200);
    vTaskDelay(10);
    // Serial.println("Esperando respuesta comando de inicialización.....");
    // sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    // Transmite_Poll(0x00);          // Transmite Poll
  }

  int comando = Buffer_Cashless.Get_Buffer_Reg_AFT()[3];
  Buffer_Cashless.Init_Buffer_Reg();

  // delay(800);

  /// Serial.println(" Estado Inicialización: " + String(comando));

  if (comando == 0x00)
  {
    Buff_Tx[0] = 0x01;
    Buff_Tx[1] = 0x73;
    Buff_Tx[2] = 0x1D;
    Buff_Tx[3] = 0x01;
    /* Asset */
    Buff_Tx[4] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[0];
    Buff_Tx[5] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[1];
    Buff_Tx[6] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[2];
    Buff_Tx[7] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[3];

    /* Key */
    /* Asset */
    Buff_Tx[8] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[0];
    Buff_Tx[9] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[1];
    Buff_Tx[10] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[2];
    Buff_Tx[11] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[3];
    /* POS ID */
    Buff_Tx[12] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[4];
    Buff_Tx[13] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[5];
    Buff_Tx[14] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[6];
    Buff_Tx[15] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[7];
    /* MAC */
    Buff_Tx[16] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[8];
    Buff_Tx[17] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[9];
    Buff_Tx[18] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[10];
    Buff_Tx[19] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[11];
    Buff_Tx[20] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[12];
    Buff_Tx[21] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[13];
    /* FECHA */
    Buff_Tx[22] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[14];
    Buff_Tx[23] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[15];
    Buff_Tx[24] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[16];
    Buff_Tx[25] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[17];
    Buff_Tx[26] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[18];
    Buff_Tx[27] = Buffer_Cashless.Get_RX_AFT(Buffer_registro_Mq_)[19];
    /* POS ID */
    Buff_Tx[28] = 0x01;
    Buff_Tx[29] = 0x00;
    Buff_Tx[30] = 0x00;
    Buff_Tx[31] = 0x00;

    // calcularCRC_Registro_AFT();
    CalcularCRC_Transfer(Buff_Tx, Host_Size);

    sendDataa(dat, sizeof(dat)); // Transmite SYN
    delay(250);

    for (int i = 0; i < 34; i++)
    {
      if (i == 0)
      {
        sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      }
      else
      {
        Transmite_Poll_Long(Buff_Tx[i]);
      }
    }
    int Contador = 0;
    unsigned long Timout_Break_Response1;
    int Stop_Transaccion_Amount_Response1 = 15000; // Tiempo de espera en milisegundos (2 Seg MAX)
    Timout_Break_Response1 = millis();
    esp_task_wdt_init(1000000, true);
    esp_task_wdt_add(NULL);

    while ((Buffer_Cashless.Get_Buffer_Reg_AFT()[3] == 0xAA) && (millis() - Timout_Break_Response1 < Stop_Transaccion_Amount_Response1))
    {
      esp_task_wdt_reset();

      // Contador++;
      // if(Contador==1)
      //   sendDataa(dat, sizeof(dat)); // Transmite DIR
      // if(Contador>1)
      // {
      //   Transmite_Poll(0x00);
      //   Contador=0;
      // }

      vTaskDelay(10);
      // Serial.println("Esperando respuesta comando de registro .....");
      // sendDataa(dat4, sizeof(dat4)); // Transmite DIR
      // Transmite_Poll(0x00);          // Transmite Poll
    }

    // Serial.println(" Estado de registro: " + String(comando));

    int comando = Buffer_Cashless.Get_Buffer_Reg_AFT()[3];
    Buffer_Cashless.Init_Buffer_Reg();
    Buffer_Cashless.Set_Status_Reg(0xAA);

    switch (comando)
    {
    case 0x00:
      // Serial.println("Gaming Machine Registration ready");
      Buffer_Cashless.Set_Status_Reg(0x00);
      break;

    case 0x01:
      // Serial.println("Gaming Machine Registered");
      Buffer_Cashless.Set_Status_Reg(0x01);
      break;
    case 0x40:
      // Serial.println("Gaming Machine Registration pending");
      Buffer_Cashless.Set_Status_Reg(0x40);
      break;

    case 0x80:
      // Serial.println("Gaming Machine not Registered");
      Buffer_Cashless.Set_Status_Reg(0x80);
      break;

    default:
      // Serial.println("No respuesta de la maquina");
      Buffer_Cashless.Set_Status_Reg(0xAA);
      break;
    }
  }
  else
  {
    // Serial.println("Registro no inicializado");
    Buffer_Cashless.Set_Status_Reg(0xAA);
  }
  /* Inicializa y registra maquina Cashless */
}

void AFT_Lock_Status_Timeout(void)
{

  char Buff_Tx[10];
  int Host_Size = 7;

  Buff_Tx[0] = 0x01;
  Buff_Tx[1] = 0x74;

  /* LOCK CODE */
  Buff_Tx[2] = 0x00;

  /* TRANSFER COONDITION */
  Buff_Tx[3] = 0x0F;

  /* TIMEOUT 2BCD */
  Buff_Tx[4] = 0x00;
  Buff_Tx[5] = 0x00;

  /* CRC */
  Buff_Tx[6] = 0x00;
  Buff_Tx[7] = 0x00;

  CalcularCRC_Transfer(Buff_Tx, Host_Size);

  Buffer_Cashless.Init_Buffer_Lock_Status_74();

  sendDataa(dat, sizeof(dat)); // Transmite SYN
  delay(250);

  for (int i = 0; i < 9; i++)
  {
    if (i == 0)
      sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    else
      Transmite_Poll_Long(Buff_Tx[i]);
  }

  int Contador = 0;
  unsigned long Timout_Break_Response1;
  int Stop_Transaccion_Amount_Response1 = 10000; // Tiempo de espera en milisegundos (2 Seg MAX)
  Timout_Break_Response1 = millis();
  esp_task_wdt_init(1000000, true);
  esp_task_wdt_add(NULL);

  while ((Buffer_Cashless.Get_Buffer_Lock_Status_74()[1] == 0xAA) && (millis() - Timout_Break_Response1 < Stop_Transaccion_Amount_Response1))
  {
    esp_task_wdt_reset();

    // Contador++;
    // if(Contador==1)
    //   sendDataa(dat, sizeof(dat)); // Transmite DIR
    // if(Contador>1)
    // { 
    //   Transmite_Poll(0x00);
    //   Contador=0;
    // }

    vTaskDelay(10);
    // Serial.println("Esperando respuesta comando de registro .....");
    // sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    // Transmite_Poll(0x00);          // Transmite Poll
  }

  int Command = Buffer_Cashless.Get_Buffer_Lock_Status_74()[1];

  int Av_Trasfer = Buffer_Cashless.Get_Buffer_Lock_Status_74()[7];

  if (Command == 0x74)
  {
  }
}

void TIMEOUT_TRANSFER(bool Handle)
{
  if (Handle)
  {
    /* Borra data AFT para consulta de estado de transaccion  */
    Buffer_Cashless.Init_Buffer_Transfer_AFT(true);

    /* Sincroniza Maquina */
    sendDataa(dat, sizeof(dat)); // Transmite SYN
    delay(200);
    /* AFT FUNDS TRANSFER STATUS */
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x72);
    Transmite_Poll_Long(0x02);
    Transmite_Poll_Long(0xFF);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x0F);
    Transmite_Poll_Long(0x22);
    delay(500);

    /* Reporta la transaccion OK */
    if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x00 || Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x01)
    {
      Hadle_comunications=false;
      Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, true); /*INICIA SESION PLAYER CASHLESS*/
      Info_Cashless.Init_Player_Tracking_Sesion(contadores.Get_Client_ID_Transaccion());
      Info_Cashless.Status_Transfer(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4], Buffer_Cashless.Get_Bufffer_Transfer_AFT(), RTC);
      Ack_Cashless_Transfer_Load = false; /* Espera Ack Maquina */

      /* Reset  Bandera */
    }
    else if (Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4] == 0x40)
    {
      /* Pendiente mantiene bandera */
    }
    else
    {
      Hadle_comunications=false;
      /* Reporta error de transaccion */
      Info_Cashless.Type_Sesion(PLAYER_CASHLESS_SESION, false);
      // Info_Cashless.Init_Player_Tracking_Sesion(contadores.Get_Client_ID_Transaccion());
      Info_Cashless.Status_Transfer(Buffer_Cashless.Get_Bufffer_Transfer_AFT()[4], Buffer_Cashless.Get_Bufffer_Transfer_AFT(), RTC);
      Status_Barra(ERROR_LECTURA);
      Status_Barra(500);
      Ack_Cashless_Transfer_Load = false; /* Espera Ack Maquina */
    }
  }
}