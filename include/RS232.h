#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"

#if CONFIG_IDF_TARGET_ESP32
    #include "esp32/rom/uart.h"
#elif CONFIG_IDF_TARGET_ESP32S2
    #include "esp32s2/rom/uart.h"
#endif

static const char *TAG = "uart_events";

//PINES ---------> UART2 <----------------------

#define TXD_PIN (GPIO_NUM_17)  
#define RXD_PIN (GPIO_NUM_16)
#define ECHO_TEST_RTS  (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS  (UART_PIN_NO_CHANGE)
#define EX_UART_NUM UART_NUM_2 

#define PATTERN_CHR_NUM    (3)         
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
//-----------------------------------------------

#define                 SYNC                  0x0180
#define                 POLL                  0x0181
#define                 DIR                   0x0101

static QueueHandle_t uart2_queue;
uart_event_t event;
size_t buffered_size;
uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);

//-----------------------------------> Interrupción UART2 Data<-----------------------------------------------

static void uart_event_task(void *pvParameters)
{
    for(;;) {
        //Waiting for UART event.
        if(xQueueReceive(uart2_queue, (void * )&event, (portTickType)portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);
            ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);
            switch(event.type) {
                case UART_DATA:
                    ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    ESP_LOGI(TAG, "[DATA EVT]:");
                   // uart_write_bytes(EX_UART_NUM, (const char*) dtmp, event.size);
                    break;
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

//----------------------------------> Envia Datos HEXA <-----------------------------------------
void Write2USART(uint8_t datos) 
{

  char buf[1] = {datos};
  const int txBytes = uart_write_bytes(UART_NUM_2, buf, sizeof(buf));
  ESP_ERROR_CHECK(uart_wait_tx_done(EX_UART_NUM,100)); // wait timeout is 100 RTOS ticks (TickType_t)
  ESP_LOGI("TX_TASK", "Wrote %d bytes", txBytes);
  //return txBytes;
}

void Delete_Buffer_RX(){
  uart_flush(EX_UART_NUM);
}


void Transmite_Sincronizacion(void)
{
  Write2USART(SYNC);
}

void Transmite_Poll( unsigned char Com_SAS ){

  if ( Com_SAS != 0 )
    {
        Write2USART(DIR);
    }
  else{
        Write2USART(POLL);
  }

  //Transmito Byte de comando
  if ( Com_SAS != 0 )
    {
        Write2USART( Com_SAS );
        delay(100);	
    }
  else
    {
        delay(100);
    }
}

//-------------------------------------------> CONFIGURACIÓN DE UART2 <---------------------------------------------
void Init_UART2(){

  esp_log_level_set(TAG, ESP_LOG_INFO);
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 115200, 
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(EX_UART_NUM, &uart_config);
    //Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);
    //Set UART pins (using UART2 default pins ie no changes.)
    uart_set_pin(EX_UART_NUM,  TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    //Install UART driver, and get the queue.
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart2_queue, 0);
    //Set uart pattern detect function.
    uart_enable_pattern_det_intr(EX_UART_NUM, '+', PATTERN_CHR_NUM, 10000, 10, 10);
    //Reset the pattern queue length to record at most 20 pattern positions.
    uart_pattern_queue_reset(EX_UART_NUM, 20);
    //Create a task to handler UART event from ISR
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);
}



