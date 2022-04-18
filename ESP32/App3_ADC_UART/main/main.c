#include<stdio.h>
#include<driver/adc.h> 
#include"driver/uart.h"
#include<esp_adc_cal.h> 
#include<string.h>
 
#define DEFAULT_VREF    1100
#define NO_OF_SAMPLES 25
  
//To configure the UART 
void config_UART (uart_port_t uart_num){ 
  
    uart_config_t uart_config = { 
        .baud_rate = 115200, 
        .data_bits = UART_DATA_8_BITS, 
        .parity = UART_PARITY_DISABLE, 
        .stop_bits = UART_STOP_BITS_1, 
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,      //​ enable hardware flow control 
        .rx_flow_ctrl_thresh = 122, 
    }; 

    //​ Configure UART parameters 
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config)); 

    //​ Set UART pins(TX: IO4, RX: IO5, RTS: IO18, CTS: IO19) 
    ESP_ERROR_CHECK(uart_set_pin(uart_num, 4, 5, 18, 19)); 

    //​ Setup UART buffered IO with event queue 
    const int uart_buffer_size = (1024 * 2); 
    QueueHandle_t uart_queue; 
    //uart_driver_install(uart_num, rx_buffer_size, tx_buffer_size, queue_size, uart_queue, intr_alloc_flags) 
    ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size, uart_buffer_size, 10, &uart_queue,0)); 
} 

//​ To configure the ADC 
void config_ADC(adc_channel_t channel, esp_adc_cal_characteristics_t *adc_chars){ 
   //​ ADC capture width is 12Bit. 
   adc_bits_width_t width = ADC_WIDTH_BIT_12; 

   //​ ADC attenuation is 0 dB 
   adc_atten_t atten = ADC_ATTEN_DB_11; 

   //​ Configure precision 
   adc1_config_width(width); 

   //​ To calculate the voltage based on the ADC raw results, this formula can be used: 
   //​ Vout = Dout * Vmax / Dmax  

   //​ ADC unit - SAR ADC 1 
   adc_unit_t unit = ADC_UNIT_1; 

   //​ Configure attenuation 
   //​ +----------+-------------+-----------------+ 
   //​ |          | attenuation | suggested range | 
   //​ |    SoC   |     (dB)    |      (mV)       | 
   //​ +==========+=============+=================+ 
   //​ |          |       0     |    100 ~  950   | 
   //​ |          +-------------+-----------------+ 
   //​ |          |       2.5   |    100 ~ 1250   | 
   //​ |   ESP32  +-------------+-----------------+ 
   //​ |          |       6     |    150 ~ 1750   | 
   //​ |          +-------------+-----------------+ 
   //​ |          |      11     |    150 ~ 2450   | 
   //​ +----------+-------------+-----------------+ 
   adc1_config_channel_atten(channel, atten); 

   //​Characterize ADC at particular atten 
   esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars); 
} 

void app_main(void){ 
   //​ ADC1 channel 0 is GPIO36 
   gpio_set_direction(GPIO_NUM_36, GPIO_MODE_INPUT); 

   config_UART(UART_NUM_0); 
   
   adc_channel_t channel = ADC1_CHANNEL_0; 
   esp_adc_cal_characteristics_t *adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t)); 

   config_ADC(channel, adc_chars);     
   
   //​ To read ADC conversion result 
   while(1){ 
        uint32_t raw = 0; 
         
        for(int i = 0; i < NO_OF_SAMPLES; i++){ 
            //​ Get raw value 
            raw += adc1_get_raw;
        }
   }
}
