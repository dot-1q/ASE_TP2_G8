#include <driver/gpio.h>
#include <driver/uart.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "soc/adc_channel.h"

//ADC Channels
#define ADC1_EXAMPLE_CHAN0          ADC1_CHANNEL_7 // GPIO 35
static const char *TAG_CH = "ADC1_CH7";

//ADC Attenuation
#define ADC_EXAMPLE_ATTEN           ADC_ATTEN_DB_11

//ADC Calibration
#define ADC_EXAMPLE_CALI_SCHEME     ESP_ADC_CAL_VAL_EFUSE_VREF


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

int app_main() {

    esp_err_t ret = ESP_OK;
    uint32_t voltage = 0;
    bool cali_enable = adc_calibration_init();

    //ADC1 config
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_EXAMPLE_CHAN0, ADC_EXAMPLE_ATTEN));

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_driver_install(UART_NUM_1, 2048, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, 10, 9, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // Main loop
    char* teste_var="A";

    while(true) {

        adc_raw = adc1_get_raw(ADC1_EXAMPLE_CHAN0);
        ESP_LOGI(TAG_CH, "raw  data: %d", adc_raw);
        if (cali_enable) {
            voltage = esp_adc_cal_raw_to_voltage(adc_raw, &adc1_chars);
            ESP_LOGI(TAG_CH, "cali data: %d mV", voltage);
        }

        // Aqui iremos ler da dac e imprimir
        int ret = uart_write_bytes(UART_NUM_1, (void*)teste_var, 1);
        printf("%d\n", ret);
        printf("gpio: %d\n", ADC1_GPIO35_CHANNEL);
        vTaskDelay(100);
    }
}