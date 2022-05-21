
#include "driver/uart.h"
#include <string.h>
#include <iostream>
using namespace std;
#include <stdio.h>
#include "Clase_Contadores.h"
//#include "Memoria_SD.h"

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

void Transmite_Sincronizacion(void);
static void UART_ISR_ROUTINE(void *pvParameters);
void Encuestas_Maquina(void *pvParameters);

Contadores_SAS contadores;

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
  xTaskCreatePinnedToCore(UART_ISR_ROUTINE, "UART_ISR_ROUTINE", 2048, NULL, 12, NULL, 1); // Máx Priority principal
  xTaskCreatePinnedToCore(Encuestas_Maquina, "Encuestas", 2048, NULL, configMAX_PRIORITIES - 5, NULL, 1);
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
  uint8_t buffer[128];
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

        for (int index = 0; index < conta_bytes; index++)
        {
          String buffer_contadores_string = String(buffer[index], HEX);
          buffer_contadores[index] = buffer_contadores_string.toInt();
        }

        if (buffer_contadores[1] > 0x09 && buffer_contadores[1] < 0x16)
        {
          int unidades, descenas, centenas, uni_mil, desc_mil, cent_mil, uni_millon, desc_millon, resultado = 0;

          desc_millon = (buffer_contadores[2] - (buffer_contadores[2] % 10)) / 10;
          Serial.println(desc_millon);
          uni_millon = buffer_contadores[2] % 10;
          Serial.println(uni_millon);

          cent_mil = (buffer_contadores[3] - (buffer_contadores[3] % 10)) / 10;
          Serial.println(cent_mil);
          desc_mil = buffer_contadores[3] % 10;
          Serial.println(desc_mil);

          uni_mil = (buffer_contadores[4] - (buffer_contadores[4] % 10)) / 10;
          Serial.println(uni_mil);
          centenas = buffer_contadores[4] % 10;
          Serial.println(centenas);

          descenas = (buffer_contadores[5] - (buffer_contadores[5] % 10)) / 10;
          Serial.println(descenas);
          unidades = buffer_contadores[5] % 10;
          Serial.println(unidades);

          resultado = (desc_millon * 10000000) + (uni_millon * 1000000) +
                      (cent_mil * 100000) + (desc_mil * 10000) +
                      (uni_mil * 1000) + (centenas * 100) +
                      (descenas * 10) + (unidades * 1);

          Serial.println(resultado);

          switch (buffer_contadores[1])
          {
          case 10:
            if (contadores.Set_Contadores(Total_Cancel_Credit, resultado))
            {
              Serial.println("Guardado con exito");
            }
            else
            {
              Serial.println("No se pudo guardar");
            }
            break;

          default:
            Serial.println("Default");
            break;
          }
        }

        buffer[0] = 0x00;
        conta_bytes = 0;
      }

      // Handle frame error event
      else if (event.type == UART_FRAME_ERR)
      {
        // TODO...
      }
      else
      {
        // TODO...
      }
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
    if (bandera == 0 && Conta_Poll < 10)
    {
      sendDataa(dat, sizeof(dat)); // transmite sincronización
      bandera = 1;
      Conta_Poll++;
    }
    else if (bandera == 1 && Conta_Poll < 10)
    {
      Transmite_Poll(0x00); // Transmite Poll
      bandera = 0;
      Conta_Poll++;
    }
    else if (Conta_Poll == 10)
    {
      Conta_Poll = 0;
      Conta_Encuestas++;
      switch (Conta_Encuestas)
      {
      case 1:
        Serial.println("1");
        Transmite_Poll(0x10);
        break;

      case 2:
        Serial.println("2");
        Transmite_Poll(0x11);
        break;

      case 3:
        Serial.println("3");
        Transmite_Poll(0x12);
        break;

      case 4:
        Serial.println("4");
        Transmite_Poll(0x14);
        break;

      case 5:
        Serial.println("5");
        Transmite_Poll(0x13);
        break;

      case 6:
        sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        Serial.println("6");
        Transmite_Poll_Long(0x2D);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0xFF);
        Transmite_Poll_Long(0xE0);
        break;

      case 7:
        Serial.println("7");
        Transmite_Poll(0x46);
        break;
      case 8:
        sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        Serial.println("8");
        Transmite_Poll_Long(0x2F);
        Transmite_Poll_Long(0x03);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x2E);
        Transmite_Poll_Long(0xF7);
        Transmite_Poll_Long(0xE3);
        break;

      case 9:
        Serial.println("9");
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
        sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        Serial.println("10");
        Transmite_Poll_Long(0x2F);
        Transmite_Poll_Long(0x03);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x30);
        Transmite_Poll_Long(0x08);
        Transmite_Poll_Long(0x1A);
        break;

      case 11:

        sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        Serial.println("11");
        Transmite_Poll_Long(0x2F);
        Transmite_Poll_Long(0x03);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x32);
        Transmite_Poll_Long(0x1A);
        Transmite_Poll_Long(0x39);
        break;

      case 12:
        sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        Serial.println("12");
        Transmite_Poll_Long(0x2F);
        Transmite_Poll_Long(0x03);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x33);
        Transmite_Poll_Long(0x93);
        Transmite_Poll_Long(0x28);
        break;
      case 13:
        sendDataa(dat4, sizeof(dat4)); // Transmite DIR
        Serial.println("13");
        Transmite_Poll_Long(0x2F);
        Transmite_Poll_Long(0x03);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x00);
        Transmite_Poll_Long(0x34);
        Transmite_Poll_Long(0x2C);
        Transmite_Poll_Long(0x5C);
        break;

      case 14:
        Serial.println("14");
        Transmite_Poll(0x15);
        Conta_Encuestas = 0;
        break;
      }
    }
    // Ejecuta   Taraea Encuestas_Maquina Cada 100ms
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
  vTaskDelete(NULL);
}

//----------------------------------------------------------------------------------------------------------------------------
