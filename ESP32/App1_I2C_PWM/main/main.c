#include <stdio.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/timer.h"
#include "esp_log.h"
// #include "driver/mcpwm.h"

#include "driver/ledc.h"


#define RED_LED_GPIO        (18) 
#define GREEN_LED_GPIO      (19) 
#define BLUE_LED_GPIO       (21) 

#define LED_PWM_FREQ        (50)

#define LEDC_TIMER              LEDC_TIMER_0 
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (5) // Define the output GPIO
// #define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY_MAX               (8189) // Set duty to 100%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz

// static inline uint32_t servo_angle_to_duty_us(int angle);

int count = 0;
void TIMER_INIT2(int timer_idx);

// static inline uint32_t servo_angle_to_duty_us(int angle)
// {
//     return (angle + SERVO_MAX_DEGREE) * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) / (2 * SERVO_MAX_DEGREE) + SERVO_MIN_PULSEWIDTH_US;
// }

static void IRAM_ATTR timer_callback(void* args) {
    count++;
    //ets_printf("Count: %d\n", count);
    ets_printf("Time: %02d:%02d\n", count/60, count%60);
}

void TIMER_INIT2(int timer_idx) {
    timer_config_t config = 
    {
  		  .alarm_en = true,
  		  .counter_en = true,
  		  .intr_type = TIMER_INTR_LEVEL,
  		  .counter_dir = TIMER_COUNT_UP,
  		  .auto_reload = true,
  		  .divider = 80 // new clock = 1MHz
            
    };

    timer_init(TIMER_GROUP_0, timer_idx, &config);
    timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0);
    timer_set_alarm_value(TIMER_GROUP_0, timer_idx, 1000000); //generate alarm each 1 second
    timer_enable_intr(TIMER_GROUP_0, timer_idx);
    timer_isr_callback_add(TIMER_GROUP_0, timer_idx, timer_callback, NULL, 0);
    timer_start(TIMER_GROUP_0, timer_idx);
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

    printf("%f %f %f \n", red_dc, green_dc, blue_dc);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, (int)(red_dc*LEDC_DUTY_MAX));
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_1, (int)(green_dc*LEDC_DUTY_MAX));
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_1);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_2, (int)(blue_dc*LEDC_DUTY_MAX));
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_2);
}

void app_main(void)
{
    led_pwm_init();
    while (1) {
        for (int h = 0; h < 360; h+=1) {
            vTaskDelay(1); 
          
            HSVtoRGB(h, 100, 100);

            set_led_rgb(100, rgb[1], rgb[2]);

        }
    }
}