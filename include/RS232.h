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
int Envio=0;
char Prueba_AFT[128];
char Data_TX_AFT[33];
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
//----------------------------------------------------------------------------------------------------------
//--------------------------------------> TaskHandle_t <----------------------------------------------------

TaskHandle_t RecepcionRS232, Encuestas;

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
void Encuestas_Maquina(void *pvParameters);
//--------------Guarda Contadores SD
void Storage_Contadores_SD(char *ARCHIVO, String Encabezado1, bool Enable);
void Add_String(char *contador, String stringDT, bool Separador);
void Add_String_Hora(String Horario);
//-------------------------------------
//--------------Guarda Eventos SD
void Store_Eventos_SD(char *ARCHIVO, bool Enable);
void Add_String_Hora_EVEN(String Horario);
void Add_String_EVEN(String EVENTO, bool Separador);
//---------------------------------------
void Encuestas_Maquinas_Simple_No_Cancel(void);
void Encuestas_Maquinas_Genericas(void);
void Encuestas_Maquinas_Poker(void);
void Encuestas_Maquinas_IRT(void);
void Encuestas_Maquinas_EFT(void);
void Encuestas_Maquinas_IGT_Riel(void);
void Encuestas_Maquinas_IGT_Riel_Bill(void);
void Encuestas_Mecanicas(void);
void Encuestas_Maquinas_Simple(void);
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
void Escribe_Tarjeta_Mecanica(char *buf);
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
int Conta_Poll_Billetes = 0;

bool flag_ultimo_contador_Ok = false;
int conta_poll_comunicacion_maquina = 0;
bool flag_handle_maquina = false;
int Handle_Maquina = 0;

bool Act_Coin_in_Poker = false;
bool Act_Coin_out_Poker = false;
bool Act_Bill_Poker = false;
bool Act_Current_Credits = false;
int Cuenta_Save_Mecanicas=0;

#define flag_bloquea_Maquina     1
#define flag_desbloquea_Maquina  2
#define flag_encuesta_premio     3
#define escribe_tarjeta_mecanica 4
#define Encuesta_Info_Cashless   5
#define Flag_Reset_Handpay       6
#define Flag_Creditos_Premio     7
#define Actualiza_Machine        8

#define Interroga_Est_Reg_AFT    9
#define Cancela_Registro_AFT_MQ  10
#define Init_Reg_AFT             11 
#define Registra_Maquina_AFT     12
#define Creditos_machine         13
#define Flag_Encuesta_Maquina_Juego 14
#define Flag_Encuesta_ROM        15

// MetodoCRC CRC_Maq;
// Contadores_SAS contadores;
// Eventos_SAS eventos;
int Contador_Save_Data=0;
//---------------------------Configuración de UART2 Data 8bits, baud 19200, 1 Bit de stop, Paridad Disable---------------
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
    NVS.begin("Config_ESP32", false);
    uint16_t Port_COM = NVS.getUInt("COM");
    NVS.end();

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
  xTaskCreatePinnedToCore(Encuestas_Maquina, "Encuestas", 2048, NULL, configMAX_PRIORITIES - 15, &Encuestas, 1);
  //----------------------------------------------------------------------------------------------------------------------
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

//-------------------------Interrupción  Recepción de Datos-------------------------------------------------------------------
static void UART_ISR_ROUTINE(void *pvParameters)
{
  uart_event_t event;
  size_t buffered_size;
  bool exit_condition = false;
  char buffer[128];
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
        
      }
      if (buffer[3] == 0xF3 && buffer[4] == 0x07||buffer[3]==0xE1 &&buffer[4]==0x24)
      {
        Variables_globales.Set_Variable_Global_Char(Reset_Handay_OK,buffer[2]);
      }

      if (buffer[0] == 0x01 && conta_bytes > 1)
      {
        //   Serial.println("Es un contador");
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

                  Selector_Modo_SD(); // Ftp o Storage
                  
                    if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 9)
                    {

                      for (int i = 0; i < Max_Encuestas; i++)
                      {
                        SD_Cont = SD_Cont + Estructura_CSV[i];
                      }
                      if (Variables_globales.Get_Variable_Global(Ftp_Mode) == false)
                      {
                        if(Cuenta_Save_Mecanicas>=30)
                        {
                          Cuenta_Save_Mecanicas=0;
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

          for (size_t i = 0; i < conta_bytes; i++)
          {
            #ifdef Debug_Contadores
            Serial.print(buffer[i], HEX);
            #endif
          }
          Serial.println();
        }

        if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9)
        {
          
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

              Buffer_Cashless.Set_RX_AFT(Buffer_RX_AFT_, buffer);
            }
              if (buffer[0] == 0x01 && buffer[1] == 0x73)
              {
                #ifdef Debug_Contadores
                Serial.println("Genial.........>>>>>");
                #endif
                Buffer_Cashless.Set_RX_AFT(Buffer_RX_AFT_, buffer);
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
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 5 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 4||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 10)
                {
                  Estructura_CSV[0] = RTC.getTime() + ","; // Add Hora MAQ Generica
                }
                Add_Contador(contador, Total_Cancel_Credit, false);

                if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 10)
                {
                  contadores.Set_Contadores(Cancel_Credit_Hand_Pay, contador);
                  Add_Contador(contador, Cancel_Credit_Hand_Pay, false);
                }
                break;
              case 11:
                contadores.Set_Contadores(Coin_In, contador); //? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6 && Variables_globales.Get_Variable_Global(Flag_Hopper_Enable))
                {
                  Act_Coin_in_Poker = true;
                }
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 7 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 8)
                {
                  Estructura_CSV[0] = RTC.getTime() + ","; // Add Hora MAQ IGT Riel
                }
                Add_Contador(contador, Coin_In, false);
                contadores.Set_Contadores(Coin_In, contador); //? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6)
                {
                  Estructura_CSV[0] = RTC.getTime() + ",";                                                       // Add Hora Poker
                  Add_Contador(contadores.Get_Contadores_Char(Total_Cancel_Credit), Total_Cancel_Credit, false); /*Poker*/
                }
                Add_Contador(contador, Coin_In, false);
                break;
              case 12:
                contadores.Set_Contadores(Coin_Out, contador); //? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6 && Variables_globales.Get_Variable_Global(Flag_Hopper_Enable))
                {
                  Act_Coin_out_Poker = true;
                }
                Add_Contador(contador, Coin_Out, false);
                break;
              case 13:
                contadores.Set_Contadores(Total_Drop, contador);
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 4 || Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6|| Configuracion.Get_Configuracion(Tipo_Maquina,0)==12)
                  contadores.Set_Contadores(Bill_Amount, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6 && Variables_globales.Get_Variable_Global(Flag_Hopper_Enable))
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

                if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 10)
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
                contadores.Set_Contadores(Bill_Amount, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 5 && contadores.Get_Contadores_Int(Bill_Amount) == 0)
                {
                  char res[7] = {};
                  bzero(res, 7); // Pone el buffer en 0
                  memcpy(res, contadores.Get_Contadores_Char(Total_Drop), 7);
                  contadores.Set_Contadores(Bill_Amount, res);
                }
                else if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2)
                {
                   Calcula_Bill_In_550();
                }
                Add_Contador(contador, Bill_Amount, false);
                break;
              }
              if (buffer[1] == 0x1A)
              {
                contadores.Set_Contadores(Current_Credits, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                Add_Contador(contador, Current_Credits, false);
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6 && Variables_globales.Get_Variable_Global(Flag_Hopper_Enable))
                {
                  Act_Current_Credits = true;
                }
                 
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
                if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 4)
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
                Serial.println("Default");
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
                ||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 8||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 10 )
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
                    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 7||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 8 ||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 10 )
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
                Configuracion.Get_Configuracion(Tipo_Maquina, 0) ==2)
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

                    if(Contador_Save_Data>1&& Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 5)
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
                break;
              case 0x0E:
                contadores.Set_Contadores(Ticket_Out, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
                Add_Contador(contador, Ticket_Out, false);
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
              }else{
                contadores.Set_Contadores(Cancel_Credit_Hand_Pay, contador);
              }
              Add_Contador(contador, Cancel_Credit_Hand_Pay, false);
            }

            else if(buffer[7]==0x00 |buffer[7]==0x40 |buffer[7]==0xFF)
            {
              if(Buffer_Cashless.Set_RX_AFT(Info_MQ_AFT,buffer))
              {
                Variables_globales.Set_Variable_Global(Consulta_Info_Cashless_OK,true);
              }

            }
            

            else if (buffer[1] == 0x1B)
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
          Variables_globales.Set_Variable_Global(Dato_Evento_Valido, true);
              
              // Guarda Evento En Memoria SD.
              int Evento = eventos.Get_evento();

              if(Evento!=0x00&& Datos_OK==true)
              {
                Bandera_RS232_F=Bandera_RS232;
              }

              if(Evento==0x4F)
              {
                Variables_globales.Set_Variable_Global(Billete_Insert,true);
              }

              if (Evento != 0x6A && Evento!=0x8C&& Evento!=0x00)
              {
                delay(10);
                String Descrip = Tabla_Evento.Get_Descrip_Eventos(Evento);

                Add_String_Hora_EVEN(RTC.getTime());                                                           // Agrega Hora de Evento a String
                Add_String_EVEN(String(Evento), true);                                                         // Agrega Tipo de Evento a String
                Add_String_EVEN(Descrip, false);                                                               // Agrega Descripción  de Evento a String
                Store_Eventos_SD(Archivo_CSV_Eventos, Variables_globales.Get_Variable_Global(Enable_Storage)); // Envia String Completo.
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
  const TickType_t xFrequency = pdMS_TO_TICKS(100);

  if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 4)
  {
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(3000);
  }else{
    TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS(100);
  }

  for (;;)
  {
    // Verifica cada 10 segundos aproximadamente si hay comunicacion con la maquina
    switch (Configuracion.Get_Configuracion(Tipo_Maquina, 0))
    {
    case 0:
      Serial.println("Defecto");
      break;
    case 1:
      Serial.println("Cashless AFT");
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
      Serial.println("Cashless AFT Single");
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
    default:
      break;
    }

    // A Task shall never return or exit.
    // Get the actual execution tick
    xLastWakeTime = xTaskGetTickCount();
    // Switch the led
    int Vel_Poll=5;

    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 4)
    {
        Vel_Poll=15;
    }
    if (bandera == 0 && Conta_Poll < Vel_Poll)
    {
      if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9)
        sendDataa(dat, sizeof(dat)); // transmite sincronización
      bandera = 1;
      Conta_Poll++;
    }
    else if (bandera == 1 && Conta_Poll < Vel_Poll)
    {
      if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) != 9)
        Transmite_Poll(0x00); // Transmite Poll
      bandera = 0;
      Conta_Poll++;
    }
    else if (Conta_Poll >= Vel_Poll)
    {
      numero_encuesta++;
      Conta_Poll = 0;
      if (flag_handle_maquina)
      {
        switch (Handle_Maquina)
        {
        case 1:
          //    Serial.println("Inactiva Maquina");
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
          Escribe_Tarjeta_Mecanica(Buffer.Get_buffer_tarjeta_mecanica());
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

          Transmite_Encuesta_ROM();
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
          Serial.println("Defecto");
          break;
        case 1:
          Serial.println("Cashless AFT");
          break;
        case 2:
          Encuestas_Maquinas_EFT();
          break;
        case 3:
          Serial.println("Cashless AFT Single");
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
        default:
          break;
        }
      }
      //  Serial.print("Contador de encuestas realizadas --> ");
      //  Serial.println(numero_encuesta);
    }
    // Ejecuta   Taraea Encuestas_Maquina Cada 100ms

    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) ==4)
    {
      vTaskDelay(250); // Pausa Tarea 10000ms
    }else{
      vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
    
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
    #ifdef Debug_Encuestas
    Serial.println("Casheable In"); // Casheable in
    #endif
    sendDataa(dat4, sizeof(dat4));  // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x2E);
    Transmite_Poll_Long(0xF7);
    Transmite_Poll_Long(0xE3);
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
    #ifdef Debug_Encuestas
    Serial.println("Casheable Out"); // Casheable out
    #endif
    sendDataa(dat4, sizeof(dat4));   // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x32);
    Transmite_Poll_Long(0x1A);
    Transmite_Poll_Long(0x39);
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
  case 36:
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
  case 39:
    #ifdef Debug_Encuestas
    Serial.println("Encuesta Billetes");
    #endif
   // Encuesta_Billetes();
    Conta_Encuestas = 0;
    flag_ultimo_contador_Ok = true;
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
    #ifdef Debug_Encuestas
    Serial.println("Contador 1C - Door Open Metter");
    #endif
    Transmite_Poll(0x1C);
    break;
  case 20:
    #ifdef Debug_Encuestas
    Serial.println("Contador 18 - Games Since Last Power Up");
    #endif
    Transmite_Poll(0x18);
    break;
  case 21:
    #ifdef Debug_Encuestas
    Serial.println("ID Machine");
    #endif
    Transmite_Poll(0x1F);
    break;
  case 22:
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
  case 23:
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
  Serial.println("Transmite solicitud de contadores Mecanicas");
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

  if(Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) == 3 &&Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 5)
  {
    Transmite_Poll(0x12);
    delay(100);
  }

  if (Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) == 0)
  {
    if(Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 5 ||Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 2)
    {
      /*Actualiza Creditos Generica*/
      #ifdef Debug_Encuestas
      Serial.println("Encuesta Creditos Generica....... ");
      #endif
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
  if (Variables_globales.Get_Variable_Global_Int(Flag_Type_excepcion) == 1 && Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 5)
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

  if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 10)
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

      Transmite_Poll(0x2B);
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
    if (Variables_globales.Get_Variable_Global(Ftp_Mode) == true || Variables_globales.Get_Variable_Global(Flag_Memoria_SD_Full) == true || Variables_globales.Get_Variable_Global(SD_INSERT) == false||Variables_globales.Get_Variable_Global(Falla_MicroSD))
    {
      Variables_globales.Set_Variable_Global(Enable_Storage, false); // Deshabilita  Guardado SD.
    }
    else
    {
      if (Variables_globales.Get_Variable_Global(Sincronizacion_RTC) == true && Variables_globales.Get_Variable_Global(Ftp_Mode) == false && Variables_globales.Get_Variable_Global(Flag_Memoria_SD_Full) == false && Variables_globales.Get_Variable_Global(SD_INSERT) == true && Variables_globales.Get_Variable_Global(Flag_Archivos_OK)==true&& !Variables_globales.Get_Variable_Global(Falla_MicroSD))
      {
        Variables_globales.Set_Variable_Global(Enable_Storage, true);
      }
    }
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
    Coin_Out_IRT =Convert_Char_To_Int15(contadores.Get_Contadores_Char(Physical_Coin_Out));
    // Coin_Out_IRT = contadores.Get_Contadores_Int(Physical_Coin_Out);



    Total_Cancel_Credit_IRT = Cancel_Credit_IRT + Coin_Out_IRT;

    
    
      Total_Cancel_Credit_IRT_2 = Total_Cancel_Credit_IRT;
      Serial.print("contador cancel credit int IRT es: ");
      Serial.println(Total_Cancel_Credit_IRT);

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

      Serial.print("contador cancel credit char IRT es: ");
      Serial.println(Contador_Cancel_Credit_IRT);
      if (Convert_Char_To_Int15(Contador_Cancel_Credit_IRT) != Total_Cancel_Credit_IRT_2)
      {
        Serial.println("Error en calculo premio IRT");
      }
      else
      {
        contadores.Set_Contadores(Total_Cancel_Credit, Contador_Cancel_Credit_IRT);
        // contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_Cancel_Credit_IRT);
      }
      contadores.Set_Contadores(Total_Cancel_Credit, Contador_Cancel_Credit_IRT);
}

  void Calcula_Bill_In_550(void)
  {
    int Bill_In_550, Residuo;
    int uni, dec, cen, unimil, decmil, centmil, unimill, decmill;
    char Contador_Bill_In_550[9];
    bzero(Contador_Bill_In_550, 9);

    Bill_In_550 = contadores.Get_Contadores_Int(Bill_Amount);

    Bill_In_550 *= 10;

    Serial.print("contador bill_in int 550 es: ");
    Serial.println(Bill_In_550);

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

    Serial.print("contador bill_in char 550 es: ");
    Serial.println(Contador_Bill_In_550);
    contadores.Set_Contadores(Bill_Amount, Contador_Bill_In_550);
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
  Transmite_Poll(0x1A);
  delay(100);
  Transmite_Poll(0x1A);
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
    Transmite_Poll(0x13);
  }
  delay(100);
}

bool Transmite_AFT_Registro_Machine(void)
{
    Serial.println("REGISTRA MAQUINA....");
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


bool Transmite_Init_Registro(void)
{
  Serial.println("INICIALIZA REGISTRO....");
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

  Serial.println("Consulta Registro AFT....");
  sendDataa(dat4, sizeof(dat4)); // Transmite DIR
  Transmite_Poll_Long(0x73);
  Transmite_Poll_Long(0x01);
  Transmite_Poll_Long(0xFF);
  Transmite_Poll_Long(0xA7);
  Transmite_Poll_Long(0x65);

  return true;
}
/*---------------------------------------------------------------------------------------------*/
