#include "driver/uart.h"
#include <string.h>
#include <iostream>
#include <stdio.h>
#include "Memory_SD.h"
using namespace std;
#include <stdio.h>
#include <stdlib.h>

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
extern SdVolume volume;
extern Sd2Card card;
extern SdFile root;
uint8_t rxbuf[255];   // Buffer di ricezione
uint16_t rx_fifo_len; // Lunghezza dati
uint8_t UART2_data[1024];
char dat[1] = {SYNC};
char dat3[1] = {POLL};
char dat4[1] = {DIR};

int Conta_Poll = 0;
int Conta_Encuestas = 0;

extern char Archivo_CSV_Contadores[100];
extern char Archivo_CSV_Eventos[100];
extern char Archivo_LOG[100];

String SD_Cont;
String SD_EVEN;

int bandera = 0;
int contador = 0;
long numero_contador = 0;
int Contador_Encuestas=0;
int numero_encuesta=0; 
int Max_Encuestas=27;
String Encabezado_Contadores="Hora,Total Cancel Credit,Coin In,Coin Out,Jackpot,Total Drop, Cancel Credit Hand Pay,Bill Amount, Casheable In, Casheable Restricted In, Casheable Non Restricted In, Casheable Out, Casheable Restricted Out,Casheable Nonrestricted Out,Games Played,Coin In Fisico,Coin Out Fisico,Total Coin Drop,Machine Paid Progresive Payout,Machine Paid External Bonus Payout,Attendant Paid Progresive Payout,Attendant Paid External Payout,Ticket In,Ticket Out,Current Credits,Contador 1C - Door Open Metter,Contador 18 - Games Since Last Power Up";
String Estructura_CSV[27]={"n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a,","n/a"};
//------------------------------------> Prototipo de Funciones <------------------------------------------------
void Transmite_Sincronizacion(void);
static void UART_ISR_ROUTINE(void *pvParameters);
void Encuestas_Maquina(void *pvParameters);
//--------------Guarda Contadores SD
void Storage_Contadores_SD(char* ARCHIVO,String Encabezado1,bool Enable);
void Add_String(char* contador, String stringDT, bool Separador);
void Add_String_Hora(String Horario);
//-------------------------------------
//--------------Guarda Eventos SD
void Store_Eventos_SD(char* ARCHIVO,bool Enable);
void Add_String_Hora_EVEN(String Horario);
void Add_String_EVEN(String EVENTO,bool Separador);
//---------------------------------------

void Encuestas_Maquinas_Genericas(void);
void Encuestas_Maquinas_Poker(void);
void Encuestas_Maquinas_IRT(void);

void Encuesta_Billetes(void);
bool Inactiva_Maquina(void);
void Transmite_Inactiva_Maquina(void);
bool Activa_Maquina(void);
void Transmite_Activa_Maquina(void);
bool Verifica_Premio_1B(void);
void Encuesta_contador_1B(void);
void Selector_Modo_SD(void);
void Delete_Trama();
void Add_Contador(char* Contador_,int Filtro, bool Salto_Linea);
void Calcula_Cancel_Credit_IRT(void);
//---------------------------------------------------------------------------------------------------------------
// MetodoCRC CRC_Maq;
// Contadores_SAS contadores;
// Eventos_SAS eventos;

bool ACK_Maq = false;
bool ACK_Premio = false;
int Conta_Poll_Billetes = 0;

bool flag_ultimo_contador_Ok = false;
int conta_poll_comunicacion_maquina = 0;
bool flag_handle_maquina = false;
int Handle_Maquina = 0;

#define flag_bloquea_Maquina 1
#define flag_desbloquea_Maquina 2
#define flag_encuesta_premio 3

// MetodoCRC CRC_Maq;
// Contadores_SAS contadores;
// Eventos_SAS eventos;

//---------------------------Configuración de UART2 Data 8bits, baud 19200, 1 Bit de stop, Paridad Disable---------------
void Init_UART2()
{

  uart_config_t Configurazione_UART2 = {
      .baud_rate = 19200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
  };

  uart_param_config(NUMERO_PORTA_SERIALE, &Configurazione_UART2);
  ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &Configurazione_UART2));
  uart_set_pin(NUMERO_PORTA_SERIALE, U2TXD, U2RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  ESP_ERROR_CHECK(uart_driver_install(NUMERO_PORTA_SERIALE, BUF_SIZE, BUF_SIZE, 20, &uart2_queue, 0));
  // uart_set_rx_timeout(uart_port_t uart_num, const uint8_t tout_thresh);
  //-----------------------------------------------Aquí Tareas Nucleo 0 Comunicación Maquina------------------------------
  xTaskCreatePinnedToCore(UART_ISR_ROUTINE, "UART_ISR_ROUTINE", 2048, NULL, configMAX_PRIORITIES, &RecepcionRS232, 1); // Máx Priority principal
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
            //   Serial.print(UART2_data[i], HEX);
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
          Serial.println("Mensaje de ACK recibido.................................................");
          ACK_Maq = true;
        }
      }

      if (buffer[0] == 0x01 && conta_bytes > 1)
      {
        //   Serial.println("Es un contador");

        if (Buffer.Verifica_buffer_Maq(buffer, conta_bytes))
        {

          numero_contador++;

          for (int index = 0; index < conta_bytes; index++)
          {
            String buffer_contadores_string = String(buffer[index], HEX);
            buffer_contadores[index] = buffer_contadores_string.toInt();
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

            Serial.println(contador);

            switch (buffer_contadores[1])
            {
            case 10:
              contadores.Set_Contadores(Total_Cancel_Credit, contador);
              if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 4)
                Calcula_Cancel_Credit_IRT();
               //? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Estructura_CSV[0]=RTC.getTime()+",";
              Add_Contador(contador,Total_Cancel_Credit,false);
              break;
            case 11:
              contadores.Set_Contadores(Coin_In, contador); //? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Add_Contador(contador,Coin_In,false);
              break;
            case 12:
              contadores.Set_Contadores(Coin_Out, contador); //? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
             Add_Contador(contador,Coin_Out,false);
              break;
            case 13:
              contadores.Set_Contadores(Total_Drop, contador);
              if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 6)
                contadores.Set_Contadores(Bill_Amount, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Add_Contador(contador,Total_Drop,false);
              break;
            case 14:
              contadores.Set_Contadores(Jackpot, contador);// ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
             Add_Contador(contador,Jackpot,false);
              break;
            case 15:
              contadores.Set_Contadores(Games_Played, contador);// ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
             Add_Contador(contador,Games_Played,false);
              break;
            case 46:
              contadores.Set_Contadores(Bill_Amount, contador);// ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
             Add_Contador(contador,Bill_Amount,false);
              break;
            }
            if (buffer[1] == 0x1A)
            {
             contadores.Set_Contadores(Current_Credits, contador);// ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
            Add_Contador(contador,Current_Credits,false);
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

            Serial.println(contador);

            switch (buffer[1])
            {
            case 0x2A:
              contadores.Set_Contadores(Physical_Coin_In, contador);// ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Add_Contador(contador,Physical_Coin_In,false);
              break;
            case 0x2B:
              contadores.Set_Contadores(Physical_Coin_Out, contador);
              if (Configuracion.Get_Configuracion(Tipo_Maquina, 0) == 4)
                Calcula_Cancel_Credit_IRT(); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Add_Contador(contador,Physical_Coin_Out,false);
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

            Serial.println(contador);

            contadores.Set_Contadores(Door_Open, contador);// ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
            Add_Contador(contador,Door_Open,false);
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
            contadores.Set_Contadores(Games_Since_Last_Power_Up, contador);// ? Serial.println("Guardado con exito") : Serial.println("No se pudo guardar");
            Add_Contador(contador,Games_Since_Last_Power_Up,true);
            Selector_Modo_SD(); // Ftp o Storage

            for(int i=0;i<Max_Encuestas;i++)
            {
              SD_Cont=SD_Cont+Estructura_CSV[i];
            }
            if(Variables_globales.Get_Variable_Global(Ftp_Mode)==false)
            {
              Storage_Contadores_SD(Archivo_CSV_Contadores, Encabezado_Contadores,Variables_globales.Get_Variable_Global(Enable_Storage));
            }
            for(int i=0;i<Max_Encuestas;i++)
            {
              if(i==Max_Encuestas-1)
              {
                Estructura_CSV[i]="n/a";
              }
              else
              {
                Estructura_CSV[i]="n/a,";
              }
            }
            Delete_Trama();

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

            Serial.println(contador);
            contadores.Set_Contadores(Informacion_Maquina, contador); // ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
          }

          else if (buffer[1] == 0x21)
          {
            char contador[2] = {};

            contador[0] = buffer[2];
            contador[1] = buffer[3];

            Serial.print(contador[0], HEX);
            Serial.println(contador[1], HEX);

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

            Serial.println(contador);
            switch (buffer[5])
            {
            case 0x0D:
              contadores.Set_Contadores(Ticket_In, contador);// ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Add_Contador(contador,Ticket_In,false);
              break;
            case 0x0E:
              contadores.Set_Contadores(Ticket_Out, contador);// ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Add_Contador(contador,Ticket_Out,false);
              break;
            case 0x1D:
              contadores.Set_Contadores(Machine_Paid_Progresive_Payout, contador);// ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Add_Contador(contador,Machine_Paid_Progresive_Payout,false);
              break;
            case 0x1E:
              contadores.Set_Contadores(Machine_Paid_External_Bonus_Payout, contador);// ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Add_Contador(contador,Machine_Paid_External_Bonus_Payout,false);
              break;
            case 0x20:
              contadores.Set_Contadores(Attendant_Paid_Progresive_Payout, contador);// ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Add_Contador(contador,Attendant_Paid_Progresive_Payout,false);
              break;
            case 0x21:
              contadores.Set_Contadores(Attendant_Paid_External_Bonus_Payout, contador);// ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Add_Contador(contador,Attendant_Paid_External_Bonus_Payout,false);
              break;
            case 0x24:
              contadores.Set_Contadores(Total_Coin_Drop, contador);// ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Add_Contador(contador,Total_Coin_Drop,false);
              break;
            case 0x2E:
              contadores.Set_Contadores(Casheable_In, contador);// ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Add_Contador(contador,Casheable_In,false);
              break;
            case 0x2F:
              contadores.Set_Contadores(Casheable_Restricted_In, contador);// ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Add_Contador(contador,Casheable_Restricted_In,false);
              break;
            case 0x30:
              contadores.Set_Contadores(Casheable_NONrestricted_In, contador);// ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Add_Contador(contador,Casheable_NONrestricted_In,false);
              break;
            case 0x32:
              contadores.Set_Contadores(Casheable_Out, contador);// ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Add_Contador(contador,Casheable_Out,false);
              break;
            case 0x33:
              contadores.Set_Contadores(Casheable_Restricted_Out, contador);// ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Add_Contador(contador,Casheable_Restricted_Out,false);
              break;
            case 0x34:
              contadores.Set_Contadores(Casheable_NONrestricted_Out, contador);// ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Add_Contador(contador,Casheable_NONrestricted_Out,false);
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

            Serial.println(contador);

            contadores.Set_Contadores(Cancel_Credit_Hand_Pay, contador);
            Add_Contador(contador,Cancel_Credit_Hand_Pay,false);
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

            Serial.println(contador);

            contadores.Set_Contadores(Premio_1B, contador);
          }
          // bzero(buffer, 128);
          conta_bytes = 0;

          //  Serial.print("Contador de encuestas con CRC valido --> ");
          //  Serial.println(numero_contador);
        }
        else
        {
          // bzero(buffer, 128);
          Serial.println("Error de CRC en contadores...");
        }
      }
      else if (buffer[0] != 0x00 && buffer[0] != 0x01 && buffer[0] != 0x1F)
      {
        
        if (eventos.Set_evento(buffer[0]))
        {
          Serial.println("Es un evento");
          Serial.println("--------------------------------------------------");
          Serial.println();

          try
          {
            // Guarda Evento En Memoria SD.
            int Evento = buffer[0];
            Add_String_Hora_EVEN(RTC.getTime());                                                           // Agrega Hora a String
            Add_String_EVEN(String(Evento), true);                                                         // Agrega Evento a String
            Add_String_EVEN("evento", false);                                                              // Agrega Descripción a String
            Store_Eventos_SD(Archivo_CSV_Eventos, Variables_globales.Get_Variable_Global(Enable_Storage)); // Envia String Completo.
            Variables_globales.Set_Variable_Global(Dato_Evento_Valido, true);
          }
          catch(const std::exception& e)
          {
            Serial.println(e.what());
           // std::cerr << e.what() << '\n';
          }
          
        }
      }
      bzero(buffer, 128); // Pone el buffer en 0
    }

    else
    {
    }

    // If you want to break out of the loop due to certain conditions, set exit condition to true
    if (exit_condition)
    {
      break;
    }
  }
  vTaskDelay(10);
  free(UART2_data);
}
//---------------------------- Tarea Para Consulta de Contadores y General Poll-----------------------------------------------

void Encuestas_Maquina(void *pvParameters)
{
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS(100);
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
      Serial.println("Cashless EFT");
      break;
    case 3:
      Serial.println("Cashless AFT Single");
      break;
    case 4:
      if (numero_encuesta > 20) // Numero de encuestas realizadas a la maquina
      {
        if (numero_contador < 15) // Numero de respuestas recibidas por la maquina
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, false);
        else
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
        numero_encuesta = 0;
        numero_contador = 0;
      }
      break;
    case 5:
      if (numero_encuesta > 20) // Numero de encuestas realizadas a la maquina
      {
        if (numero_contador < 15) // Numero de respuestas recibidas por la maquina
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, false);
        else
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
        numero_encuesta = 0;
        numero_contador = 0;
      }
      break;
    case 6:
      if (numero_encuesta > 6) // Numero de encuestas realizadas a la maquina
      {
        if (numero_contador < 5) // Numero de respuestas recibidas por la maquina
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, false);
        else
          Variables_globales.Set_Variable_Global(Comunicacion_Maq, true);
        numero_encuesta = 0;
        numero_contador = 0;
      }
      break;
    default:
      break;
    }

    // A Task shall never return or exit.
    // Get the actual execution tick
    xLastWakeTime = xTaskGetTickCount();
    // Switch the led
    if (bandera == 0 && Conta_Poll < 5)
    {
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      bandera = 1;
      Conta_Poll++;
    }
    else if (bandera == 1 && Conta_Poll < 5)
    {
      Transmite_Poll(0x00); // Transmite Poll
      bandera = 0;
      Conta_Poll++;
    }
    else if (Conta_Poll >= 5)
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
          Serial.println("Cashless EFT");
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
        default:
          break;
        }

        // Conta_Encuestas++;
        // switch (Conta_Encuestas)
        // {
        // case 1:
        //   //    Serial.println("Total Cancel Credit"); // total cancel credit
        //   Transmite_Poll(0x10);
        //   break;
        // case 2:
        //   //    Serial.println("Coin In"); // Coin in
        //   Transmite_Poll(0x11);
        //   break;
        // case 3:
        //   //    Serial.println("Coin Out"); // Coin out
        //   Transmite_Poll(0x12);
        //   break;
        // case 4:
        //   //    Serial.println("Jackpot"); // Jackpot
        //   Transmite_Poll(0x14);
        //   break;
        // case 5:
        //   //    Serial.println("Total Drop"); // total drop
        //   Transmite_Poll(0x13);
        //   break;
        // case 6:
        //   //    Serial.println("Cancel Credit Hand Paid"); // Cancel credit hand paid
        //   sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        //   Transmite_Poll_Long(0x2D);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0xFF);
        //   Transmite_Poll_Long(0xE0);
        //   break;
        // case 7:
        //   //    Serial.println("Bill amount");
        //   Transmite_Poll(0x46); // Bill amount
        //   break;
        // case 8:
        //   //    Serial.println("Casheable In"); // Casheable in
        //   sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        //   Transmite_Poll_Long(0x2F);
        //   Transmite_Poll_Long(0x03);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x2E);
        //   Transmite_Poll_Long(0xF7);
        //   Transmite_Poll_Long(0xE3);
        //   break;
        // case 9:
        //   //    Serial.println("Casheable Restricted In"); // Casheable restricted in
        //   sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        //   Transmite_Poll_Long(0x2F);
        //   Transmite_Poll_Long(0x03);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x2F);
        //   Transmite_Poll_Long(0x7E);
        //   Transmite_Poll_Long(0xF2);
        //   break;
        // case 10:
        //   //    Serial.println("Casheable Nonrestricted In"); // Casheable Nonrestricted in
        //   sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        //   Transmite_Poll_Long(0x2F);
        //   Transmite_Poll_Long(0x03);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x30);
        //   Transmite_Poll_Long(0x08);
        //   Transmite_Poll_Long(0x1A);
        //   break;
        // case 11:
        //   //    Serial.println("Casheable Out"); // Casheable out
        //   sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        //   Transmite_Poll_Long(0x2F);
        //   Transmite_Poll_Long(0x03);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x32);
        //   Transmite_Poll_Long(0x1A);
        //   Transmite_Poll_Long(0x39);
        //   break;
        // case 12:
        //   //    Serial.println("Casheable Restricted Out"); // Casheable restricted out
        //   sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        //   Transmite_Poll_Long(0x2F);
        //   Transmite_Poll_Long(0x03);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x33);
        //   Transmite_Poll_Long(0x93);
        //   Transmite_Poll_Long(0x28);
        //   break;
        // case 13:
        //   //    Serial.println("Casheable Nonrestricted Out"); // Casheable nonrestricted out
        //   sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        //   Transmite_Poll_Long(0x2F);
        //   Transmite_Poll_Long(0x03);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x34);
        //   Transmite_Poll_Long(0x2C);
        //   Transmite_Poll_Long(0x5C);
        //   break;
        // case 14:
        //   //    Serial.println("Games Played"); // Games played
        //   Transmite_Poll(0x15);
        //   break;
        // case 15:
        //   //    Serial.println("Coin In Fisico"); // Physical coin in
        //   Transmite_Poll(0x2A);
        //   break;
        // case 16:
        //   //    Serial.println("Coin Out Fisico"); // Physical coin out
        //   Transmite_Poll(0x2B);
        //   break;
        // case 17:
        //   //    Serial.println("Total Coin Drop");
        //   sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        //   Transmite_Poll_Long(0x2F);
        //   Transmite_Poll_Long(0x03);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x24);
        //   Transmite_Poll_Long(0xAD);
        //   Transmite_Poll_Long(0x4C);
        //   break;
        // case 18:
        //   //    Serial.println("Machine Paid Progresive Payout");
        //   sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        //   Transmite_Poll_Long(0x2F);
        //   Transmite_Poll_Long(0x03);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x1D);
        //   Transmite_Poll_Long(0xEF);
        //   Transmite_Poll_Long(0xE0);
        //   break;
        // case 19:
        //   //    Serial.println("Machine Paid External Bonus Payout");
        //   sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        //   Transmite_Poll_Long(0x2F);
        //   Transmite_Poll_Long(0x03);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x1E);
        //   Transmite_Poll_Long(0x74);
        //   Transmite_Poll_Long(0xD2);
        //   break;
        // case 20:
        //   //    Serial.println("Attendant Paid Progresive Payout");
        //   sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        //   Transmite_Poll_Long(0x2F);
        //   Transmite_Poll_Long(0x03);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x20);
        //   Transmite_Poll_Long(0x89);
        //   Transmite_Poll_Long(0x0A);
        //   break;
        // case 21:
        //   //    Serial.println("Attendant Paid External Payout");
        //   sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        //   Transmite_Poll_Long(0x2F);
        //   Transmite_Poll_Long(0x03);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x21);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x1B);
        //   break;
        // case 22:
        //   //    Serial.println("Ticket In");
        //   sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        //   Transmite_Poll_Long(0x2F);
        //   Transmite_Poll_Long(0x03);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x0D);
        //   Transmite_Poll_Long(0x6E);
        //   Transmite_Poll_Long(0xF0);
        //   break;
        // case 23:
        //   //    Serial.println("Ticket Out");
        //   sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        //   Transmite_Poll_Long(0x2F);
        //   Transmite_Poll_Long(0x03);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x0E);
        //   Transmite_Poll_Long(0xF5);
        //   Transmite_Poll_Long(0xC2);
        //   break;
        // case 24:
        //   //    Serial.println("Current Credits");
        //   Transmite_Poll(0x1A);
        //   break;
        // case 25:
        //   //    Serial.println("Contador 1C - Door Open Metter");
        //   Transmite_Poll(0x1C);
        //   break;
        // case 26:
        //   //    Serial.println("Contador 18 - Games Since Last Power Up");
        //   Transmite_Poll(0x18);
        //   break;
        // case 27:
        //   //    Serial.println("ID Machine");
        //   Transmite_Poll(0x1F);
        //   break;
        // case 28:
        //   //    Serial.println("ROM Signature");
        //   sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        //   Transmite_Poll_Long(0x21);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x00);
        //   Transmite_Poll_Long(0x5C);
        //   Transmite_Poll_Long(0x45);
        //   break;
        // case 29:
        //   Encuesta_Billetes();
        //   Conta_Encuestas = 0;
        //   flag_ultimo_contador_Ok = true;
        // }
      }
      //  Serial.print("Contador de encuestas realizadas --> ");
      //  Serial.println(numero_encuesta);
    }
    // Ejecuta   Taraea Encuestas_Maquina Cada 100ms
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
  vTaskDelay(10);
}
void Storage_Contadores_SD(char* ARCHIVO,String Encabezado1,bool Enable)
{
  if (Enable == true)
  {
      Write_Data_File2(SD_Cont, ARCHIVO, true, Encabezado1);
      //Write_Data_File2("", ARCHIVO, true, Encabezado1);
    for (int i = 0; i <= SD_Cont.length(); i++)
    {
      SD_Cont.remove(i);
    }
  }
  else
  {
    Serial.println("Guardado SD Deshabilitado..");
    for (int i = 0; i <= SD_Cont.length(); i++)
    {
      SD_Cont.remove(i);
    }
  }
}

void Store_Eventos_SD(char* ARCHIVO,bool Enable)
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
    Serial.println("Guardado SD Deshabilitado..");
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

void Add_Contador(char* Contador_,int Filtro, bool Salto_Linea)
{
  if(Salto_Linea)
  {
    Estructura_CSV[Filtro]=String(Contador_);
  }
  else
  {
    Estructura_CSV[Filtro]=String(Contador_)+",";
  }
}

void Add_String_EVEN(String EVENTO,bool Separador)
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

// Encuestas Maquinas Genericas
void Encuestas_Maquinas_Genericas(void)
{
  Conta_Encuestas++;
  switch (Conta_Encuestas)
  {
  case 1:
    //    Serial.println("Total Cancel Credit"); // total cancel credit
    Transmite_Poll(0x10);
    break;
  case 2:
    //    Serial.println("Coin In"); // Coin in
    Transmite_Poll(0x11);
    break;
  case 3:
    //    Serial.println("Coin Out"); // Coin out
    Transmite_Poll(0x12);
    break;
  case 4:
    //    Serial.println("Jackpot"); // Jackpot
    Transmite_Poll(0x14);
    break;
  case 5:
    //    Serial.println("Total Drop"); // total drop
    Transmite_Poll(0x13);
    break;
  case 6:
    //    Serial.println("Cancel Credit Hand Paid"); // Cancel credit hand paid
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2D);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0xFF);
    Transmite_Poll_Long(0xE0);
    break;
  case 7:
    //    Serial.println("Bill amount");
    Transmite_Poll(0x46); // Bill amount
    break;
  case 8:
    //    Serial.println("Casheable In"); // Casheable in
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x2E);
    Transmite_Poll_Long(0xF7);
    Transmite_Poll_Long(0xE3);
    break;
  case 9:
    //    Serial.println("Casheable Restricted In"); // Casheable restricted in
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x7E);
    Transmite_Poll_Long(0xF2);
    break;
  case 10:
    //    Serial.println("Casheable Nonrestricted In"); // Casheable Nonrestricted in
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x30);
    Transmite_Poll_Long(0x08);
    Transmite_Poll_Long(0x1A);
    break;
  case 11:
    //    Serial.println("Casheable Out"); // Casheable out
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x32);
    Transmite_Poll_Long(0x1A);
    Transmite_Poll_Long(0x39);
    break;
  case 12:
    //    Serial.println("Casheable Restricted Out"); // Casheable restricted out
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x33);
    Transmite_Poll_Long(0x93);
    Transmite_Poll_Long(0x28);
    break;
  case 13:
    //    Serial.println("Casheable Nonrestricted Out"); // Casheable nonrestricted out
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x2F);
    Transmite_Poll_Long(0x03);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x34);
    Transmite_Poll_Long(0x2C);
    Transmite_Poll_Long(0x5C);
    break;
  case 14:
    //    Serial.println("Games Played"); // Games played
    Transmite_Poll(0x15);
    break;
  case 15:
    //    Serial.println("Coin In Fisico"); // Physical coin in
    Transmite_Poll(0x2A);
    break;
  case 16:
    //    Serial.println("Coin Out Fisico"); // Physical coin out
    Transmite_Poll(0x2B);
    break;
  case 17:
    //    Serial.println("Total Coin Drop");
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
    //    Serial.println("Machine Paid Progresive Payout");
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
    //    Serial.println("Machine Paid External Bonus Payout");
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
    //    Serial.println("Attendant Paid Progresive Payout");
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
    //    Serial.println("Attendant Paid External Payout");
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
    //    Serial.println("Ticket In");
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
    //    Serial.println("Ticket Out");
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
    //    Serial.println("Current Credits");
    Transmite_Poll(0x1A);
    break;
  case 25:
    //    Serial.println("Contador 1C - Door Open Metter");
    Transmite_Poll(0x1C);
    break;
  case 26:
    //    Serial.println("Contador 18 - Games Since Last Power Up");
    Transmite_Poll(0x18);
    break;
  case 27:
    //    Serial.println("ID Machine");
    Transmite_Poll(0x1F);
    break;
  case 28:
    //    Serial.println("ROM Signature");
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

// Encuestas Maquinas Poker
void Encuestas_Maquinas_Poker(void)
{
  Conta_Encuestas++;
  switch (Conta_Encuestas)
  {
  case 1:
    Serial.println("Coin In"); // Coin in
    Transmite_Poll(0x11);
    break;
  case 2:
    Serial.println("Coin Out"); // Coin out
    Transmite_Poll(0x12);
    break;
  case 3:
    Serial.println("Jackpot"); // Jackpot
    Transmite_Poll(0x14);
    break;
  case 4:
    Serial.println("Total Drop"); // total drop
    Transmite_Poll(0x13);
    break;
  case 5:
    Serial.println("Games Played"); // Games played
    Transmite_Poll(0x15);
    break;
  case 6:
    Serial.println("Current Credits");
    Transmite_Poll(0x1A);
    break;
  case 7:
    Serial.println("Contador 1C - Door Open Metter");
    Transmite_Poll(0x1C);
    break;
  case 8:
    Serial.println("ID Machine");
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
    Serial.println("Total Cancel Credit"); // total cancel credit
    Transmite_Poll(0x10);
    break;
  case 2:
    Serial.println("Coin In"); // Coin in
    Transmite_Poll(0x11);
    break;
  case 3:
    Serial.println("Coin Out"); // Coin out
    Transmite_Poll(0x12);
    break;
  case 4:
    Serial.println("Jackpot"); // Jackpot
    Transmite_Poll(0x14);
    break;
  case 5:
    Serial.println("Total Drop"); // total drop
    Transmite_Poll(0x13);
    break;
  case 6:
    Serial.println("Cancel Credit Hand Paid"); // Cancel credit hand paid
    sendDataa(dat4, sizeof(dat4));             // Transmite DIR
    Transmite_Poll_Long(0x2D);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0xFF);
    Transmite_Poll_Long(0xE0);
    break;
  case 7:
    Serial.println("Bill amount");
    Transmite_Poll(0x46); // Bill amount
    break;
  case 8:
    Serial.println("Games Played"); // Games played
    Transmite_Poll(0x15);
    break;
  case 9:
    Serial.println("Coin In Fisico"); // Physical coin in
    Transmite_Poll(0x2A);
    break;
  case 10:
    Serial.println("Coin Out Fisico"); // Physical coin out
    Transmite_Poll(0x2B);
    break;
  case 11:
    Serial.println("Total Coin Drop");
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
    Serial.println("Machine Paid Progresive Payout");
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
    Serial.println("Machine Paid External Bonus Payout");
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
    Serial.println("Attendant Paid Progresive Payout");
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
    Serial.println("Attendant Paid External Payout");
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
    Serial.println("Ticket In");
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
    Serial.println("Ticket Out");
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
    Serial.println("Current Credits");
    Transmite_Poll(0x1A);
    break;
  case 19:
    Serial.println("Contador 1C - Door Open Metter");
    Transmite_Poll(0x1C);
    break;
  case 20:
    Serial.println("Contador 18 - Games Since Last Power Up");
    Transmite_Poll(0x18);
    break;
  case 21:
    Serial.println("ID Machine");
    Transmite_Poll(0x1F);
    break;
  case 22:
    Serial.println("ROM Signature");
    sendDataa(dat4, sizeof(dat4)); // Transmite DIR
    Transmite_Poll_Long(0x21);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x00);
    Transmite_Poll_Long(0x5C);
    Transmite_Poll_Long(0x45);
    break;
  case 23:
    Serial.println("Encuesta Billetes");
    Encuesta_Billetes();
    Conta_Encuestas = 0;
    flag_ultimo_contador_Ok = true;
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

void Encuesta_contador_1B(void)
{
  Transmite_Poll(0x1B);
}

void Selector_Modo_SD(void)
{

  if (Variables_globales.Get_Variable_Global(Ftp_Mode)) // FTP activado
  {
    Variables_globales.Set_Variable_Global(Enable_Storage, false); // Deshabilita  Guardado SD.
  }
  else
  {
    if (Variables_globales.Get_Variable_Global(Archivo_CSV_OK))
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

void Calcula_Cancel_Credit_IRT(void)
{
  int Total_Cancel_Credit_IRT, Cancel_Credit_IRT, Coin_Out_IRT, Residuo;
  int uni, dec, cen, unimil, decmil, centmil, unimill, decmill;
  char Contador_Cancel_Credit_IRT[9];
  bzero(Contador_Cancel_Credit_IRT, 9);

  Cancel_Credit_IRT = contadores.Get_Contadores_Int(Total_Cancel_Credit);
  Coin_Out_IRT = contadores.Get_Contadores_Int(Physical_Coin_Out);

  Total_Cancel_Credit_IRT = Cancel_Credit_IRT + Coin_Out_IRT;

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
  contadores.Set_Contadores(Total_Cancel_Credit, Contador_Cancel_Credit_IRT);
  contadores.Set_Contadores(Cancel_Credit_Hand_Pay, Contador_Cancel_Credit_IRT);
}