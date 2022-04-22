/*  Wave Generator Example

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    DAC output channel, waveform, wave frequency can be customized in menuconfig.
    If any questions about this example or more information is needed, please read README.md before your start.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/dac.h"
#include "driver/timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/uart.h"


#include "store_songs.c"

/*  The timer ISR has an execution time of 5.5 micro-seconds(us).
    Therefore, a timer period less than 5.5 us will cause trigger the interrupt watchdog.
    7 us is a safe interval that will not trigger the watchdog. No need to customize it.
*/

#define WITH_RELOAD            1
#define TIMER_INTR_US          7                                    // Execution time of each ISR interval in micro-seconds
#define TIMER_DIVIDER          16
#define POINT_ARR_LEN          10000                                  // Length of points array
//#define AMP_DAC                255                                  // Amplitude of DAC voltage. If it's more than 256 will causes dac_output_voltage() output 0.
#define VDD                    3300                                 // VDD is 3.3V, 3300mV
#define CONST_PERIOD_2_PI      6.2832
#define SEC_TO_MICRO_SEC(x)    ((x) / 1000 / 1000)                  // Convert second to micro-second
#define UNUSED_PARAM           __attribute__((unused))              // A const period parameter which equals 2 * pai, used to calculate raw dac output value.
#define TIMER_TICKS            (TIMER_BASE_CLK / TIMER_DIVIDER)     // TIMER_BASE_CLK = APB_CLK = 80MHz
#define ALARM_VAL_US           SEC_TO_MICRO_SEC(TIMER_INTR_US * TIMER_TICKS)     // Alarm value in micro-seconds
// #define OUTPUT_POINT_NUM       (int)(1000000 / (TIMER_INTR_US * FREQ) + 0.5)     // The number of output wave points.

#define DAC_CHAN               DAC_CHANNEL_1           // DAC_CHANNEL_1 (GPIO25) by default
// #define FREQ                   CONFIG_EXAMPLE_WAVE_FREQUENCY        // 3kHz by default

// _Static_assert(OUTPUT_POINT_NUM <= POINT_ARR_LEN, "The CONFIG_EXAMPLE_WAVE_FREQUENCY is too low and using too long buffer.");

#define GPIO_INPUT_IO_0     4
#define GPIO_INPUT_IO_1     5
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
#define ESP_INTR_FLAG_DEFAULT 0

static int raw_val[POINT_ARR_LEN];                      // Used to store raw values
static int volt_val[POINT_ARR_LEN];                    // Used to store voltage values(in mV)
// static const char *TAG = "wave_gen";

static int g_index = 0;

static int AMP_DAC = 250;

// static int freq = 3000;

static int OUTPUT_POINT_NUM = 0;

/* Timer interrupt service routine */
static void IRAM_ATTR timer0_ISR(void *ptr)
{
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);

    int *head = (int*)ptr;

    /* DAC output ISR has an execution time of 4.4 us*/
    if (g_index >= OUTPUT_POINT_NUM) g_index = 0;
    dac_output_voltage(DAC_CHAN, *(head + g_index));
    g_index++;
}

/* Timer group0 TIMER_0 initialization */
static void example_timer_init(int timer_idx, bool auto_reload)
{
    esp_err_t ret;
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .intr_type = TIMER_INTR_LEVEL,
        .auto_reload = auto_reload,
    };

    ret = timer_init(TIMER_GROUP_0, timer_idx, &config);
    ESP_ERROR_CHECK(ret);
    ret = timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0x00000000ULL);
    ESP_ERROR_CHECK(ret);
    ret = timer_set_alarm_value(TIMER_GROUP_0, timer_idx, ALARM_VAL_US);
    ESP_ERROR_CHECK(ret);
    ret = timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    ESP_ERROR_CHECK(ret);
    /* Register an ISR handler */
    timer_isr_register(TIMER_GROUP_0, timer_idx, timer0_ISR, (void *)raw_val, 0, NULL);
}

 static void prepare_data(int freq)
{
    int pnt_num = (int)(1000000 / (TIMER_INTR_US * freq) + 0.5);
    OUTPUT_POINT_NUM = pnt_num;
    timer_pause(TIMER_GROUP_0, TIMER_0);
    for (int i = 0; i < pnt_num; i ++) {
        raw_val[i] = (int)((sin( i * CONST_PERIOD_2_PI / pnt_num) + 1) * (double)(AMP_DAC)/2 + 0.5);
        
        volt_val[i] = (int)(VDD * raw_val[i] / (float)AMP_DAC);
    }
    g_index = 0;
    timer_start(TIMER_GROUP_0, TIMER_0);

    ESP_LOGI("WAVE_GEN", "Frequency(Hz): %d", freq);
}

static void log_info(void)
{
    ESP_LOGI("WAVE_GEN", "DAC output channel: %d", DAC_CHAN);
    if (DAC_CHAN == DAC_CHANNEL_1) {
        ESP_LOGI("WAVE_GEN", "GPIO:%d", GPIO_NUM_25);
    } else {
        ESP_LOGI("WAVE_GEN", "GPIO:%d", GPIO_NUM_26);
    }
    
    // ESP_LOGI(TAG, "Output points num: %d\n", OUTPUT_POINT_NUM);
}

static void volume_control(void* arg)
{
    int gpio_0, gpio_1;
    while(1){ 
        vTaskDelay(20);
        gpio_0 = gpio_get_level(GPIO_INPUT_IO_0);
        gpio_1 = gpio_get_level(GPIO_INPUT_IO_1);
        if (AMP_DAC > 0 && gpio_0){
            AMP_DAC -= 10;
            printf("Volume: %d\n", AMP_DAC);
        }
        if (AMP_DAC < 250 && gpio_1){
            AMP_DAC += 10;
            printf("Volume: %d\n", AMP_DAC);
        }
    }
}

void config_gpio(){
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //interrupt of rising edge
    // io_conf.intr_type = GPIO_INTR_DISABLE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
}

static int16_t* melody ;

 EEPROM_t dev;

static int get_song(uint16_t addr){

    uint8_t lenght[1];
    uint8_t len;
    do{
        //read first byte to get size
        len =  eeprom_Read(&dev, addr, lenght, 1);
        if (len != 1) {
            ESP_LOGE(TAG, "Read Fail");
            // while(1) { vTaskDelay(1); }
        }
        ESP_LOGI(TAG, "Read Data: len=%d", len);

        printf("Lenght: %d\n", lenght[0]);
    }while(lenght[0]==255);

    // Read Data
	// uint16_t rdata[lenght[0]];
    uint8_t rbuf[lenght[0]*2];

    addr++;

    bool check;

    do{
        check = true;
        len =  eeprom_Read(&dev, addr, rbuf , lenght[0]*2);
        if (len != lenght[0]*2) {
            ESP_LOGE(TAG, "Read Fail");
            // while(1) { vTaskDelay(1); }
        }
        ESP_LOGI(TAG, "Read Data: len=%d", len);
    
        uint8_t c1, c2;
        melody = malloc(255 * 2);
        for(int i = 0; i < lenght[0]; i++){  
            c1 = rbuf[i*2];
            c2 = rbuf[i*2+1];
            printf("Bytes: %d %d\n", c1, c2);

            melody[i] = (c2 << 8) | c1;
            printf("Read %d\n", melody[i]);
            // if (c1 == 255 && c2 == 255){
            //     check = false;
            //     break;
            // }
        }
    }while (!check);

    // melody = rdata;

    printf("Melody %d %d %d %d %d\n", melody[0], melody[1],melody[2],melody[3],melody[4]);

    return lenght[1];
	
}

#define STORE_SONGS 0

#define USE_EEPROM 0

void app_main(void)
{
    if (USE_EEPROM){
	    spi_master_init(&dev);
    }
    vTaskDelay(100);
    if (STORE_SONGS && USE_EEPROM){

        printf("Harry: %d\n", sizeof(harry)/2);
        store_song(dev, harry, 200,  sizeof(harry)/2);

        printf("Nokia: %d\n", sizeof(nokia)/2);
        store_song(dev, nokia, 0,  sizeof(nokia)/2);
        // return;
        vTaskDelay(100);
    }

    esp_err_t ret;
    example_timer_init(TIMER_0, WITH_RELOAD);

    ret = dac_output_enable(DAC_CHAN);
    ESP_ERROR_CHECK(ret);

    log_info();
    g_index = 0;


    config_gpio();

    xTaskCreate(volume_control, "volume_control", 2048, NULL, 10, NULL);

    // esp_sleep_enable_ext0_wakeup(GPIO_INPUT_IO_1, 1);

    gpio_wakeup_enable(GPIO_INPUT_IO_1, GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_gpio_wakeup();

    

    // change this to make the song slower or faster
    int tempo = 144;
    int notes;
    if (USE_EEPROM){
	    notes = get_song(200) / 2; //nokia 0 / harry 200
    }else{
        // uncomment the foolowing lines to ignore reading from spi
        melody = nokia;
        notes = sizeof(nokia)/4;
    }

    

    vTaskDelay(100);

     // this calculates the duration of a whole note in ms (60s/tempo)*4 beats
    int wholenote = (60000 * 4) / tempo;

    int divider = 0, noteDuration = 100;

    int thisNote = 0;

    

    while(1){
        if (thisNote > notes * 2){
            // while ( gpio_get_level(GPIO_INPUT_IO_1) == 0);
            uart_wait_tx_idle_polling(CONFIG_ESP_CONSOLE_UART_NUM);
            esp_light_sleep_start();
            thisNote = 0;
        }
        
        divider = melody[thisNote + 1];

        if (divider > 0) {
             // regular note, just proceed
            noteDuration = (wholenote) / divider;
        } else if (divider < 0) {
            // dotted notes are represented with negative durations!!
            noteDuration = (wholenote) / abs(divider);
            noteDuration *= 1.5; // increases the duration in half for dotted notes
        }
         
        if (noteDuration != 0){
            
            if(2*melody[thisNote] > 100){
            prepare_data(2*melody[thisNote]);
            }else{
                prepare_data(100);
            }
            printf("thisNote: %d\nnote: %d\ndura: %d\n\n", thisNote, melody[thisNote], noteDuration);

            vTaskDelay((int)(noteDuration/10));
        } 
        thisNote = thisNote + 2;
    }
}
