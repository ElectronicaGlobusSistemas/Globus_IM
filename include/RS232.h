#include "driver/uart.h"
#include <string.h>
#include <iostream>
#include <stdio.h>
#include "Memory_SD.h"
#include "RTC.h"
using namespace std;

#define NUMERO_PORTA_SERIALE UART_NUM_2
#define BUF_SIZE (1024 * 2)
#define RD_BUF_SIZE (1024 * 2)
static QueueHandle_t uart2_queue;

#define SYNC 0x80
#define POLL 0x81
#define DIR 0x01

#define U2RXD 17
#define U2TXD 16

uint8_t rxbuf[255];   // Buffer di ricezione
uint16_t rx_fifo_len; // Lunghezza dati
uint8_t UART2_data[1024];
char dat[1] = {SYNC};
char dat3[1] = {POLL};
char dat4[1] = {DIR};

int bandera = 0;
int contador = 0;
long numero_contador = 0;
int Contador_Encuestas=0;
int Max_Encuestas=23;

void Transmite_Sincronizacion(void);
static void UART_ISR_ROUTINE(void *pvParameters);
void Encuestas_Maquina(void *pvParameters);

// MetodoCRC CRC_Maq;
// Contadores_SAS contadores;
// Eventos_SAS eventos;
String Encabezado_Contadores="Hora,Total Cancel Credit,Coin In,Coin Out,Jackpot,Total Drop, Cancel Credit Hand Pay,Bill Amount, Casheable In, Casheable Restricted In, Casheable Non Restricted In, Casheable Out, Casheable Restricted Out,Casheable Nonrestricted Out, Games Played";
char *Archivo_Format;
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

  //-----------------------------------------------Aquí Tareas Nucleo 0 Comunicación Maquina------------------------------
  //  xTaskCreatePinnedToCore(UART_ISR_ROUTINE, "UART_ISR_ROUTINE", 2048, NULL, 12, NULL, 1); // Máx Priority principal
  //  xTaskCreatePinnedToCore(Encuestas_Maquina, "Encuestas", 2048, NULL, configMAX_PRIORITIES - 5, NULL, 1);
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
    delay(3);
    uart_set_parity(UART_NUM_2, UART_PARITY_EVEN); // parity par
    sendDataa(Datos, 1);
    // uart_set_parity(UART_NUM_2,UART_PARITY_DISABLE); //reset parity
  }
  else
  {
    delay(3);
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
        //  uart_get_wakeup_threshold()
        conta_bytes = 0;
        int UART2_data_length = 0;
        ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_2, (size_t *)&UART2_data_length));
        UART2_data_length = uart_read_bytes(NUMERO_PORTA_SERIALE, UART2_data, UART2_data_length, 100);

        for (byte i = 0; i < UART2_data_length; i++)
        {
          if (UART2_data[i] != 0x00 && UART2_data[i] != 0x1F && UART2_data[i] != 0x80 && UART2_data[i] != 0x81)
          {
            Serial.print(UART2_data[i], HEX);
          }
          buffer[i] = UART2_data[i];
          conta_bytes++;
        }
        Serial.println();
      }

      if (buffer[0] == 0x01)
      {
        Serial.println("Es un contador");

        if (Buffer.Verifica_buffer_Maq(buffer, conta_bytes))
        {

          numero_contador++;
          Contador_Encuestas++;
          

          for (int index = 0; index < conta_bytes; index++)
          {
            String buffer_contadores_string = String(buffer[index], HEX);
            buffer_contadores[index] = buffer_contadores_string.toInt();
          }

          if (buffer_contadores[1] > 9 && buffer_contadores[1] < 16 || buffer_contadores[1] == 46 || buffer[1] == 0x1A)
          {
            int unidades, descenas, centenas, uni_mil, desc_mil, cent_mil, uni_millon, desc_millon = 0;
            char contador[8] = {};

            desc_millon = (buffer_contadores[2] - (buffer_contadores[2] % 10)) / 10;
            contador[0] = desc_millon + '0';
            uni_millon = buffer_contadores[2] % 10;
            contador[1] = uni_millon + '0';

            cent_mil = (buffer_contadores[3] - (buffer_contadores[3] % 10)) / 10;
            contador[2] = cent_mil + '0';
            desc_mil = buffer_contadores[3] % 10;
            contador[3] = desc_mil + '0';

            uni_mil = (buffer_contadores[4] - (buffer_contadores[4] % 10)) / 10;
            contador[4] = uni_mil + '0';
            centenas = buffer_contadores[4] % 10;
            contador[5] = centenas + '0';

            descenas = (buffer_contadores[5] - (buffer_contadores[5] % 10)) / 10;
            contador[6] = descenas + '0';
            unidades = buffer_contadores[5] % 10;
            contador[7] = unidades + '0';

            Serial.println(contador);

            switch (buffer_contadores[1])
            {
            case 10:
              contadores.Set_Contadores(Total_Cancel_Credit, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Write_Data_File2(String(contador),Archivo_Format,false,Encabezado_Contadores);   
              break;
            case 11:
              contadores.Set_Contadores(Coin_In, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Write_Data_File2(String(contador),Archivo_Format,false,Encabezado_Contadores); 
              break;
            case 12:
              contadores.Set_Contadores(Coin_Out, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Write_Data_File2(String(contador),Archivo_Format,false,Encabezado_Contadores); 
              break;
            case 13:
              contadores.Set_Contadores(Total_Drop, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Write_Data_File2(String(contador),Archivo_Format,false,Encabezado_Contadores); 
              break;
            case 14:
              contadores.Set_Contadores(Jackpot, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Write_Data_File2(String(contador),Archivo_Format,false,Encabezado_Contadores); 
              break;
            case 15:
              contadores.Set_Contadores(Games_Played, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Write_Data_File2(String(contador),Archivo_Format,false,Encabezado_Contadores); 
              break;
            case 46:
              contadores.Set_Contadores(Bill_Amount, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Write_Data_File2(String(contador),Archivo_Format,false,Encabezado_Contadores); 
              break;
            default:
              Serial.println("Default");
              break;
            }
            if (buffer[1] == 0x1A)
            {
              contadores.Set_Contadores(Current_Credits, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
            }
          }

          else if (buffer[1] == 0x2A || buffer[1] == 0x2B)
          {
            int unidades, descenas, centenas, uni_mil, desc_mil, cent_mil, uni_millon, desc_millon = 0;
            char contador[8] = {};

            desc_millon = (buffer_contadores[2] - (buffer_contadores[2] % 10)) / 10;
            contador[0] = desc_millon + '0';
            uni_millon = buffer_contadores[2] % 10;
            contador[1] = uni_millon + '0';

            cent_mil = (buffer_contadores[3] - (buffer_contadores[3] % 10)) / 10;
            contador[2] = cent_mil + '0';
            desc_mil = buffer_contadores[3] % 10;
            contador[3] = desc_mil + '0';

            uni_mil = (buffer_contadores[4] - (buffer_contadores[4] % 10)) / 10;
            contador[4] = uni_mil + '0';
            centenas = buffer_contadores[4] % 10;
            contador[5] = centenas + '0';

            descenas = (buffer_contadores[5] - (buffer_contadores[5] % 10)) / 10;
            contador[6] = descenas + '0';
            unidades = buffer_contadores[5] % 10;
            contador[7] = unidades + '0';

            Serial.println(contador);

            switch (buffer[1])
            {
            case 0x2A:
              contadores.Set_Contadores(Physical_Coin_In, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              break;
            case 0x2B:
              contadores.Set_Contadores(Physical_Coin_Out, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              break;
            default:
              Serial.println("Default");
              break;
            }
          }

          else if (buffer[1] == 0x1C)
          {
            int unidades, descenas, centenas, uni_mil, desc_mil, cent_mil, uni_millon, desc_millon = 0;
            char contador[8] = {};

            desc_millon = (buffer_contadores[26] - (buffer_contadores[26] % 10)) / 10;
            contador[0] = desc_millon + '0';
            uni_millon = buffer_contadores[26] % 10;
            contador[1] = uni_millon + '0';

            cent_mil = (buffer_contadores[27] - (buffer_contadores[27] % 10)) / 10;
            contador[2] = cent_mil + '0';
            desc_mil = buffer_contadores[27] % 10;
            contador[3] = desc_mil + '0';

            uni_mil = (buffer_contadores[28] - (buffer_contadores[28] % 10)) / 10;
            contador[4] = uni_mil + '0';
            centenas = buffer_contadores[28] % 10;
            contador[5] = centenas + '0';

            descenas = (buffer_contadores[29] - (buffer_contadores[29] % 10)) / 10;
            contador[6] = descenas + '0';
            unidades = buffer_contadores[29] % 10;
            contador[7] = unidades + '0';

            Serial.println(contador);

            contadores.Set_Contadores(Door_Open, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
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

            Serial.println(contador);

            contadores.Set_Contadores(Games_Since_Last_Power_Up, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
          }

          else if (buffer[1] == 0x2F)
          {
            int unidades, descenas, centenas, uni_mil, desc_mil, cent_mil, uni_millon, desc_millon, cent_millon, uni_mil_millon = 0;
            char contador[10] = {};

            if (buffer[5] == 0x0D || buffer[5] == 0x0E)
            {
              uni_mil_millon = (buffer_contadores[6] - (buffer_contadores[6] % 10)) / 10;
              contador[0] = uni_mil_millon + '0';
              cent_millon = buffer_contadores[6] % 10;
              contador[1] = cent_millon + '0';

              desc_millon = (buffer_contadores[7] - (buffer_contadores[7] % 10)) / 10;
              contador[2] = desc_millon + '0';
              uni_millon = buffer_contadores[7] % 10;
              contador[3] = uni_millon + '0';

              cent_mil = (buffer_contadores[8] - (buffer_contadores[8] % 10)) / 10;
              contador[4] = cent_mil + '0';
              desc_mil = buffer_contadores[8] % 10;
              contador[5] = desc_mil + '0';

              uni_mil = (buffer_contadores[9] - (buffer_contadores[9] % 10)) / 10;
              contador[6] = uni_mil + '0';
              centenas = buffer_contadores[9] % 10;
              contador[7] = centenas + '0';

              descenas = (buffer_contadores[10] - (buffer_contadores[10] % 10)) / 10;
              contador[8] = descenas + '0';
              unidades = buffer_contadores[10] % 10;
              contador[9] = unidades + '0';
            }
            else
            {
              desc_millon = (buffer_contadores[6] - (buffer_contadores[6] % 10)) / 10;
              contador[0] = desc_millon + '0';
              uni_millon = buffer_contadores[6] % 10;
              contador[1] = uni_millon + '0';

              cent_mil = (buffer_contadores[7] - (buffer_contadores[7] % 10)) / 10;
              contador[2] = cent_mil + '0';
              desc_mil = buffer_contadores[7] % 10;
              contador[3] = desc_mil + '0';

              uni_mil = (buffer_contadores[8] - (buffer_contadores[8] % 10)) / 10;
              contador[4] = uni_mil + '0';
              centenas = buffer_contadores[8] % 10;
              contador[5] = centenas + '0';

              descenas = (buffer_contadores[9] - (buffer_contadores[9] % 10)) / 10;
              contador[6] = descenas + '0';
              unidades = buffer_contadores[9] % 10;
              contador[7] = unidades + '0';
            }

            Serial.println(contador);

            switch (buffer[5])
            {
            case 0x0D:
              contadores.Set_Contadores(Ticket_In, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              break;
            case 0x0E:
              contadores.Set_Contadores(Ticket_Out, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              break;
            case 0x1D:
              contadores.Set_Contadores(Machine_Paid_Progresive_Payout, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              break;
            case 0x1E:
              contadores.Set_Contadores(Machine_Paid_External_Bonus_Payout, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              break;
            case 0x20:
              contadores.Set_Contadores(Attendant_Paid_Progresive_Payout, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              break;
            case 0x21:
              contadores.Set_Contadores(Attendant_Paid_External_Bonus_Payout, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              break;
            case 0x24:
              contadores.Set_Contadores(Total_Coin_Drop, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              break;
            case 0x2E:
              contadores.Set_Contadores(Casheable_In, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Write_Data_File2(String(contador),Archivo_Format,false,Encabezado_Contadores); 
              break;
            case 0x2F:
              contadores.Set_Contadores(Casheable_Restricted_In, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Write_Data_File2(String(contador),Archivo_Format,false,Encabezado_Contadores); 
              break;
            case 0x30:
              contadores.Set_Contadores(Casheable_NONrestricted_In, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Write_Data_File2(String(contador),Archivo_Format,false,Encabezado_Contadores); 
              break;
            case 0x32:
              contadores.Set_Contadores(Casheable_Out, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Write_Data_File2(String(contador),Archivo_Format,false,Encabezado_Contadores); 
              break;
            case 0x33:
              contadores.Set_Contadores(Casheable_Restricted_Out, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Write_Data_File2(String(contador),Archivo_Format,false,Encabezado_Contadores); 
              break;
            case 0x34:
              contadores.Set_Contadores(Casheable_NONrestricted_Out, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
              Write_Data_File2(String(contador),Archivo_Format,false,Encabezado_Contadores); 
              break;
            default:
              Serial.println("Default");
              break;
            }
          }

          else if (buffer[1] == 0x2D)
          {
            int unidades, descenas, centenas, uni_mil, desc_mil, cent_mil, uni_millon, desc_millon = 0;
            char contador[8] = {};

            desc_millon = (buffer_contadores[4] - (buffer_contadores[4] % 10)) / 10;
            contador[0] = desc_millon + '0';
            uni_millon = buffer_contadores[4] % 10;
            contador[1] = uni_millon + '0';

            cent_mil = (buffer_contadores[5] - (buffer_contadores[5] % 10)) / 10;
            contador[2] = cent_mil + '0';
            desc_mil = buffer_contadores[5] % 10;
            contador[3] = desc_mil + '0';

            uni_mil = (buffer_contadores[6] - (buffer_contadores[6] % 10)) / 10;
            contador[4] = uni_mil + '0';
            centenas = buffer_contadores[6] % 10;
            contador[5] = centenas + '0';

            descenas = (buffer_contadores[7] - (buffer_contadores[7] % 10)) / 10;
            contador[6] = descenas + '0';
            unidades = buffer_contadores[7] % 10;
            contador[7] = unidades + '0';

            Serial.println(contador);

            contadores.Set_Contadores(Cancel_Credit_Hand_Pay, contador) ? Serial.println("Guardado con exito") : Serial.println("So se pudo guardar");
            Write_Data_File2(String(contador),Archivo_Format,true,Encabezado_Contadores); 
          }

          // bzero(buffer, 128);
          conta_bytes = 0;

          Serial.print("Contador de encuestas con CRC valido --> ");
          Serial.println(numero_contador);
          if(Contador_Encuestas==Max_Encuestas){
            Contador_Encuestas=0; //Reset Contador
          }
        }
        else
        {
          // bzero(buffer, 128);
          Serial.println("Error de CRC en contadores...");
          numero_contador = 0;
          Contador_Encuestas++;
          if(Contador_Encuestas==Max_Encuestas)
          {
            //Guarda ERROR CRC en Ultima posición  
            Write_Data_File2("Error CRC",Archivo_Format,true,Encabezado_Contadores); 
            // Era la ultima..
            Contador_Encuestas=0;
          }else
          {
            // Guarda ERROR CRC en ultima posición
            // No Era la Ultima. Guarda dato sin salto de linea.
            Write_Data_File2("Error CRC",Archivo_Format,false,Encabezado_Contadores); 
              
          }
          
        }
      }
      else if (buffer[0] != 0x00 && buffer[0] != 0x01 && buffer[0] != 0x1F)
      {
        if (eventos.set_evento(buffer[0]))
        {
          Serial.println("Es un evento");
          Serial.println("--------------------------------------------------");
          Serial.println();
          // bzero(buffer, 128);
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
  free(UART2_data);
  vTaskDelete(NULL);
}

//----------------------------------------------------------------------------------------------------------------------------

//---------------------------- Tarea Para Consulta de Contadores y General Poll-----------------------------------------------

void Encuestas_Maquina(void *pvParameters)
{
  int Conta_Poll = 0;
  int Conta_Encuestas = 0;
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS(100);
  for (;;)
  { // A Task shall never return or exit.
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
      Conta_Poll = 0;
      Conta_Encuestas++;
      switch (Conta_Encuestas)
      {
      case 1:
        Serial.println("Total Cancel Credito"); // total cancel credit
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
        Serial.println("Casheable In"); // Casheable in
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
        Serial.println("Casheable Restricted In"); // Casheable restricted in
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
        Serial.println("Casheable Nonrestricted In"); // Casheable Nonrestricted in
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
        Serial.println("Casheable Out"); // Casheable out
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
        Serial.println("Casheable Restricted Out"); // Casheable restricted out
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
        Serial.println("Casheable Nonrestricted Out"); // Casheable nonrestricted out
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
        Serial.println("Games Played"); // Games played
        Transmite_Poll(0x15);
        break;
      case 15:
        Serial.println("Coin In Fisico"); // Physical coin in
        Transmite_Poll(0x2A);
        break;
      case 16:
        Serial.println("Coin Out Fisico"); // Physical coin out
        Transmite_Poll(0x2B);
        break;
      case 17:
        Serial.println("Total Coin Drop"); // Casheable nonrestricted out
        sendDataa(dat4, sizeof(dat4));     // Transmite DIR
        Transmite_Poll_Long(0x2F);
        Transmite_Poll_Long(0x03);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x24);
        Transmite_Poll_Long(0xAD);
        Transmite_Poll_Long(0x4C);
        break;
      case 18:
        Serial.println("Machine Paid Progresive Payout"); // Casheable nonrestricted out
        sendDataa(dat4, sizeof(dat4));                    // Transmite DIR
        Transmite_Poll_Long(0x2F);
        Transmite_Poll_Long(0x03);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x1D);
        Transmite_Poll_Long(0xEF);
        Transmite_Poll_Long(0xE0);
        ;
        break;
      case 19:
        Serial.println("Machine Paid External Bonus Payout"); // Casheable nonrestricted out
        sendDataa(dat4, sizeof(dat4));                        // Transmite DIR
        Transmite_Poll_Long(0x2F);
        Transmite_Poll_Long(0x03);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x1E);
        Transmite_Poll_Long(0x74);
        Transmite_Poll_Long(0xD2);
        break;
      case 20:
        Serial.println("Attendant Paid Progresive Payout"); // Casheable nonrestricted out
        sendDataa(dat4, sizeof(dat4));                      // Transmite DIR
        Transmite_Poll_Long(0x2F);
        Transmite_Poll_Long(0x03);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x20);
        Transmite_Poll_Long(0x89);
        Transmite_Poll_Long(0x0A);
        break;
      case 21:
        Serial.println("Attendant Paid External Payout"); // Casheable nonrestricted out
        sendDataa(dat4, sizeof(dat4));                    // Transmite DIR
        Transmite_Poll_Long(0x2F);
        Transmite_Poll_Long(0x03);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x21);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x1B);
        break;
      case 22:
        Serial.println("Ticket In");   // Casheable nonrestricted out
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
        Serial.println("Ticket Out");  // Casheable nonrestricted out
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
        Serial.println("Current Credits"); // Physical coin out
        Transmite_Poll(0x1A);
        break;
      case 25:
        Serial.println("Contador 1C - Door Open Metter"); // Physical coin out
        Transmite_Poll(0x1C);
        break;
      case 26:
        Serial.println("Contador 18 - Games Since Last Power Up"); // Physical coin out
        Transmite_Poll(0x18);
        Conta_Encuestas = 0;
        break;
      }
    }
    // Ejecuta   Taraea Encuestas_Maquina Cada 100ms
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
  vTaskDelete(NULL);
}
