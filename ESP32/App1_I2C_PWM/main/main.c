#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "tc74.h"

#include "driver/gpio.h"


#define RED_LED_GPIO        (18) 
#define GREEN_LED_GPIO      (19) 
#define BLUE_LED_GPIO       (23) 

#define LEDC_TIMER              LEDC_TIMER_0 
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (5) // Define the output GPIO
// #define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY_MAX               (8189) // Set duty to 100%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz

#define GPIO_INPUT_IO_0     4
#define GPIO_INPUT_IO_1     5
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
#define ESP_INTR_FLAG_DEFAULT 0

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    printf("gpio: \n");
}

void led_pwm_init(){

    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel[3] = {
        {
            .channel    = LEDC_CHANNEL_0,
            .duty       = 0,
            .gpio_num   = RED_LED_GPIO,
            .speed_mode = LEDC_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER,
            .flags.output_invert = 1
        },
        {
            .channel    = LEDC_CHANNEL_1,
            .duty       = 0,
            .gpio_num   = GREEN_LED_GPIO,
            .speed_mode = LEDC_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER,
            .flags.output_invert = 1
        },
        {
            .channel    = LEDC_CHANNEL_2,
            .duty       = 0,
            .gpio_num   = BLUE_LED_GPIO,
            .speed_mode = LEDC_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER,
            .flags.output_invert = 1
        }};
    for (int ch = 0; ch < 3; ch++) {
        // ledc_channel_config(&ledc_channel[ch]);
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel[ch]));
    }
}

float rgb[3] = {0, 0, 0};

void HSVtoRGB(float H, float S,float V){
    if(H>360 || H<0 || S>100 || S<0 || V>100 || V<0){
        printf("The givem HSV values are not in valid range");
        rgb[0] = 0;
        rgb[1] = 0;
        rgb[2] = 0;
    }
    float s = S/100;
    float v = V/100;
    float C = s*v;
    float X = C*(1.0f-fabs(fmod(H/60.0f, 2.0f)-1.0f));
    float m = v-C;
    float r,g,b;
    if(H >= 0 && H < 60){
        r = C;g = X;b = 0;
    }
    else if(H >= 60 && H < 120){
        r = X;g = C;b = 0;
    }
    else if(H >= 120 && H < 180){
        r = 0;g = C;b = X;
    }
    else if(H >= 180 && H < 240){
        r = 0;g = X;b = C;
    }
    else if(H >= 240 && H < 300){
        r = X;g = 0;b = C;
    }
    else{
        r = C;g = 0;b = X;
    }

    rgb[0] = (r+m)*255;
    rgb[1] = (g+m)*255;
    rgb[2] = (b+m)*255;
}

void set_led_rgb(float r, float g, float b){

    float red_dc = r/255;
    float green_dc = g/255;
    float blue_dc = b/255;

    // printf("%f %f %f \n", red_dc, green_dc, blue_dc);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, (int)(red_dc*LEDC_DUTY_MAX));
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_1, (int)(green_dc*LEDC_DUTY_MAX));
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_1);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_2, (int)(blue_dc*LEDC_DUTY_MAX));
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_2);
}

#define MIN_TEMP 17
#define MAX_TEMP 70

#define MIN_HUE 0
#define MAX_HUE 200

float hue_from_temp(float temp){

    if (temp > MAX_TEMP){
        return MIN_HUE;
    }else if(temp < MIN_TEMP){
        return MAX_TEMP;
    }

    float ratio = (temp-MIN_TEMP)/(MAX_TEMP-MIN_TEMP);

    return MAX_HUE - (MAX_HUE*ratio);

}

int temperature = 0;

static void read_temperature(void* arg)
{
    while(1){ 
        vTaskDelay(50); 

        i2c_master_read_temp(I2C_MASTER_NUM,&temperature);

    }
}

void app_main(void)
{
    uint8_t operation_mode;
    i2c_master_init();
    i2c_master_read_tc74_config(I2C_MASTER_NUM,&operation_mode);
    // ESP_LOGI(TAG,"Operation mode is : %d",operation_mode);
    // set normal mode for testing (200uA consuption)
    i2c_master_set_tc74_mode(I2C_MASTER_NUM, SET_NORM_OP_VALUE);
    led_pwm_init();

    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    //change gpio intrrupt type for one pin
    // gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);

    //install gpio isr service
    // gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // //hook isr handler for specific gpio pin
    // gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
    // //hook isr handler for specific gpio pin
    // gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);

    // gpio_isr_register(gpio_isr_handler, (void*) GPIO_INPUT_IO_1, ESP_INTR_FLAG_DEFAULT, NULL);

    xTaskCreate(read_temperature, "read_temperature", 2048, NULL, 10, NULL);

    float h = 0;
    int gpio_0 = 0;
    int gpio_1 = 0;
    float v = 100;
    while (1) {
        
        vTaskDelay(20); 
        gpio_0 = gpio_get_level(GPIO_INPUT_IO_0);
        gpio_1 = gpio_get_level(GPIO_INPUT_IO_1);

        if (v > 0 && gpio_0){
            v -= 10;
        }
        if (v < 100 && gpio_1){
            v += 10;
        }

        h = hue_from_temp(temperature);

        HSVtoRGB(h, 100, v);

        set_led_rgb(rgb[0], rgb[1], rgb[2]);

        printf("Temp: %d\n", temperature);  

        printf("v: %f\n", v);
    }
}