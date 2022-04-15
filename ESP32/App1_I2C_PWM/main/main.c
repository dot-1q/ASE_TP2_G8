#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/timer.h"
#include "esp_log.h"
#include "driver/mcpwm.h"

#define SERVO_MIN_PULSEWIDTH_US (1000) // Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH_US (2000) // Maximum pulse width in microsecond
#define SERVO_MAX_DEGREE        (90)   // Maximum angle in degree upto which servo can rotate

#define SERVO_PULSE_GPIO        (18)   // GPIO connects to the PWM signal line

static inline uint32_t servo_angle_to_duty_us(int angle);

int count = 0;
void TIMER_INIT2(int timer_idx);

static inline uint32_t servo_angle_to_duty_us(int angle)
{
    return (angle + SERVO_MAX_DEGREE) * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) / (2 * SERVO_MAX_DEGREE) + SERVO_MIN_PULSEWIDTH_US;
}

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

void servo_pwm_init(){
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, SERVO_PULSE_GPIO); // To drive a RC servo, one MCPWM generator is enough

    mcpwm_config_t pwm_config = {
        .frequency = 50, // frequency = 50Hz, i.e. for every servo motor time period should be 20ms
        .cmpr_a = 0,     // duty cycle of PWMxA = 0
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0,
    };
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
}

void app_main(void)
{
    servo_pwm_init();
    while (1) {
        for (int duty_cycle = 300; duty_cycle < 1400; duty_cycle+=10) {
            //ESP_LOGI(TAG, "Angle of rotation: %d", angle);
            ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty_cycle));
            TickType_t tick = pdMS_TO_TICKS(500);
            ets_printf("%d\n", duty_cycle);
            vTaskDelay(tick); //Add delay, since it takes time for servo to rotate, generally 100ms/60degree rotation under 5V power supply
            
        }
    }
}