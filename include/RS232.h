#include "Arduino.h"

#include "driver/uart.h"

#define NUMERO_PORTA_SERIALE UART_NUM_2
#define BUF_SIZE (1024 * 2)
#define RD_BUF_SIZE (1024)
static QueueHandle_t uart2_queue;

#define     SYNC                  0x080
#define     POLL                  0x081
#define     DIR                   0x001

static const char * TAG = "";
TaskHandle_t Task1;
TaskHandle_t Task2;
#define U2RXD 17
#define U2TXD 16

uint8_t rxbuf[256];     //Buffer di ricezione
uint16_t rx_fifo_len;        //Lunghezza dati


unsigned  long tiempo_actual = 0;
unsigned long tiempo_previo = 0;
unsigned  long tiempo_actual2 = 0;
unsigned long tiempo_previo2 = 0;
int bandera = 0;
char dat[1] = {SYNC};
char dat2[1] = {0x081};

static void UART_ISR_ROUTINE(void *pvParameters);

void Init_UART2(){

  Serial.begin(19200);
  pinMode(2, OUTPUT);
  pinMode(4,INPUT);

  //Configuro la porta Serial2 (tutti i parametri hanno anche un get per effettuare controlli)
  uart_config_t Configurazione_UART2 = {
    .baud_rate = 19200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
  };
  uart_param_config(NUMERO_PORTA_SERIALE, &Configurazione_UART2);



  //Firma: void esp_log_level_set(const char *tag, esp_log_level_tlevel)
  esp_log_level_set(TAG, ESP_LOG_INFO);



  //Firma: esp_err_tuart_set_pin(uart_port_tuart_num, int tx_io_num, int rx_io_num, int rts_io_num, int cts_io_num)
  uart_set_pin(NUMERO_PORTA_SERIALE, U2TXD, U2RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);


  //Firma: uart_driver_install(UART_NUM_2, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));
  //       uart_driver_install(Numero_porta, RXD_BUFFER, TXD_Buffer, event queue handle and size, flags to allocate an interrupt)
  uart_driver_install(NUMERO_PORTA_SERIALE, BUF_SIZE, BUF_SIZE, 20, &uart2_queue, 0);


  //Create a task to handler UART event from ISR
  xTaskCreatePinnedToCore(UART_ISR_ROUTINE, "UART_ISR_ROUTINE", 10000, NULL, 12, NULL,1);
  
}





int sendDataa(const char* datos, unsigned int tamano)
{


  // ESP_ERROR_CHECK(uart_wait_tx_done(UART_NUM_2, 1));
  const int txBytes = uart_write_bytes(UART_NUM_2, datos, tamano);
  // ESP_ERROR_CHECK(uart_wait_tx_done(UART_NUM_2, 11));
  return txBytes;
}


void Transmite_Poll( unsigned char Com_SAS )
{
  char dat3[1] = {POLL};
  char dat4[1] = {DIR};
  char dat5[1] = {Com_SAS};

  /**********************************************************************************/
  //Transmite direccion con bit 9 en 1
  if ( Com_SAS != 0 )
  {
    sendDataa( dat4, sizeof(dat4) ); //Transmito Byte con 9bit = 1
  }
  else
  {
    sendDataa( dat3, sizeof(dat3)); //Transmito Byte con 9bit = 1
  }
  /***********************************************************************************/
  //Retardo de 3ms para inter_byte_delay
  delay(3);
  /***********************************************************************************/
  //Transmito Byte de comando
  if ( Com_SAS != 0 )
  {
    sendDataa(dat5, sizeof(dat5));
      delay( 100 );  //100
  }
  else
  {
     delay( 100 ); // 100
  }
}

static void UART_ISR_ROUTINE(void *pvParameters)
{
  uart_event_t event;
  size_t buffered_size;
  bool exit_condition = false;

  //Infinite loop to run main bulk of task
  while (1) {

    

    //Loop will continually block (i.e. wait) on event messages from the event queue
    if (xQueueReceive(uart2_queue, (void * )&event, (portTickType)portMAX_DELAY)) {

      //Handle received event
      if (event.type == UART_DATA) {
        

        //  uart_get_wakeup_threshold()
        uint8_t UART2_data[255];
        int UART2_data_length = 0;
        ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_2, (size_t*)&UART2_data_length));
        UART2_data_length = uart_read_bytes(UART_NUM_2, UART2_data, UART2_data_length, 10);

        // Serial.println("LEN= ");Serial.println(UART2_data_length);

        //  Serial.print("DATA= ");
        for (byte i = 0; i < UART2_data_length; i++) {


          if(UART2_data[i]!=0x00 && UART2_data[i]!=0x1F){
            Serial.println(UART2_data[i], HEX);
          }
          
        }
       
        /*
          if (UART2_data[0]==0x1F){
          digitalWrite(2,HIGH);
          }
          else if(UART2_data[0]==0x2F){
          digitalWrite(2,LOW);
          }
        */
      }

      //Handle frame error event
      else if (event.type == UART_FRAME_ERR) {
        //TODO...
      }

      //Keep adding else if statements for each UART event you want to support
      //else if (event.type == OTHER EVENT) {
      //TODO...
      //}


      //Final else statement to act as a default case
      else {
        //TODO...
      }
    }

    //If you want to break out of the loop due to certain conditions, set exit condition to true
    if (exit_condition) {
      break;
    }
  }
  uart_flush(UART_NUM_2);
  // uart_set_wakeup_threshold(UART_NUM_2,  1);

  //Out side of loop now. Task needs to clean up and self terminate before returning
  vTaskDelete(NULL);
}