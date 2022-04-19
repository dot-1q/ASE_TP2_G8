
#include <driver/gpio.h>
#include <driver/uart.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "soc/adc_channel.h"

#include "driver/gpio.h"

#include "stdlib.h"

//ADC Channels
#define ADC1_EXAMPLE_CHAN0          ADC1_CHANNEL_7 // GPIO 35
static const char *TAG_CH = "ADC1_CH7";

//ADC Attenuation
#define ADC_EXAMPLE_ATTEN           ADC_ATTEN_DB_11

//ADC Calibration
#define ADC_EXAMPLE_CALI_SCHEME     ESP_ADC_CAL_VAL_EFUSE_VREF

#define LED_OUTPUT_0    18
#define LED_OUTPUT_1    19
#define LED_OUTPUT_2    23


static int adc_raw;

static const char *TAG = "ADC SINGLE";

static esp_adc_cal_characteristics_t adc1_chars;

static bool adc_calibration_init(void)
{
    esp_err_t ret;
    bool cali_enable = false;

    ret = esp_adc_cal_check_efuse(ADC_EXAMPLE_CALI_SCHEME);
    if (ret == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGW(TAG, "Calibration scheme not supported, skip software calibration");
    } else if (ret == ESP_ERR_INVALID_VERSION) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else if (ret == ESP_OK) {
        cali_enable = true;
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_EXAMPLE_ATTEN, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_chars);
    } else {
        ESP_LOGE(TAG, "Invalid arg");
    }

    return cali_enable;
}
 
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

void app_main(void){ 
   //​ ADC1 channel 0 is GPIO36 
   gpio_set_direction(GPIO_NUM_35, GPIO_MODE_INPUT); 

   config_UART(UART_NUM_0);

   uint32_t voltage = 0;
   bool cali_enable = adc_calibration_init();

    //ADC1 config
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_EXAMPLE_CHAN0, ADC_EXAMPLE_ATTEN));   
   
    gpio_set_direction(LED_OUTPUT_0, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_OUTPUT_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_OUTPUT_2, GPIO_MODE_OUTPUT);


    // gpio_set_level(LED_OUTPUT_0, 1);
    // gpio_set_level(LED_OUTPUT_1, 1);
    // gpio_set_level(LED_OUTPUT_2, 1);

   char send_buf[5];

   while(1){ 

        vTaskDelay(100);
        adc_raw = adc1_get_raw(ADC1_EXAMPLE_CHAN0);
        ESP_LOGI(TAG_CH, "raw  data: %d", adc_raw);
        if (cali_enable) {
            voltage = esp_adc_cal_raw_to_voltage(adc_raw, &adc1_chars);
            ESP_LOGI(TAG_CH, "cali data: %d mV", voltage);
        }
        itoa(voltage, send_buf, 10);
        // Aqui iremos ler da dac e imprimir
        int ret = uart_write_bytes(UART_NUM_0, (void*)send_buf, 5);

        
        gpio_set_level(LED_OUTPUT_0, (voltage < 1500));
        gpio_set_level(LED_OUTPUT_1, (voltage < 1500));
        gpio_set_level(LED_OUTPUT_2, (voltage < 1500));

   }
}

