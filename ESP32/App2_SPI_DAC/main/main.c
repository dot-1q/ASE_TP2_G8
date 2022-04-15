/*  Wave Generator Example

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    DAC output channel, waveform, wave frequency can be customized in menuconfig.
    If any questions about this example or more information is needed, please read README.md before your start.
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/dac.h"
#include "driver/timer.h"
#include "esp_log.h"

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

#define DAC_CHAN               CONFIG_EXAMPLE_DAC_CHANNEL           // DAC_CHANNEL_1 (GPIO25) by default
// #define FREQ                   CONFIG_EXAMPLE_WAVE_FREQUENCY        // 3kHz by default

// _Static_assert(OUTPUT_POINT_NUM <= POINT_ARR_LEN, "The CONFIG_EXAMPLE_WAVE_FREQUENCY is too low and using too long buffer.");

static int raw_val[POINT_ARR_LEN];                      // Used to store raw values
static int volt_val[POINT_ARR_LEN];                    // Used to store voltage values(in mV)
static const char *TAG = "wave_gen";

static int g_index = 0;

static int AMP_DAC = 255;

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
        // #ifdef CONFIG_EXAMPLE_WAVEFORM_SINE
        raw_val[i] = (int)((sin( i * CONST_PERIOD_2_PI / pnt_num) + 1) * (double)(AMP_DAC)/2 + 0.5);
        // #elif CONFIG_EXAMPLE_WAVEFORM_TRIANGLE
        //     raw_val[i] = (i > (pnt_num/2)) ? (2 * AMP_DAC * (pnt_num - i) / pnt_num) : (2 * AMP_DAC * i / pnt_num);
        // #elif CONFIG_EXAMPLE_WAVEFORM_SAWTOOTH
        //     raw_val[i] = (i == pnt_num) ? 0 : (i * AMP_DAC / pnt_num);
        // #elif CONFIG_EXAMPLE_WAVEFORM_SQUARE
        //     raw_val[i] = (i < (pnt_num/2)) ? AMP_DAC : 0;
        // #endif
        volt_val[i] = (int)(VDD * raw_val[i] / (float)AMP_DAC);
    }
    g_index = 0;
    timer_start(TIMER_GROUP_0, TIMER_0);
}

static void log_info(void)
{
    ESP_LOGI(TAG, "DAC output channel: %d", DAC_CHAN);
    if (DAC_CHAN == DAC_CHANNEL_1) {
        ESP_LOGI(TAG, "GPIO:%d", GPIO_NUM_25);
    } else {
        ESP_LOGI(TAG, "GPIO:%d", GPIO_NUM_26);
    }
    #ifdef CONFIG_EXAMPLE_WAVEFORM_SINE
        ESP_LOGI(TAG, "Waveform: SINE");
    #elif CONFIG_EXAMPLE_WAVEFORM_TRIANGLE
        ESP_LOGI(TAG, "Waveform: TRIANGLE");
    #elif CONFIG_EXAMPLE_WAVEFORM_SAWTOOTH
        ESP_LOGI(TAG, "Waveform: SAWTOOTH");
    #elif CONFIG_EXAMPLE_WAVEFORM_SQUARE
        ESP_LOGI(TAG, "Waveform: SQUARE");
    #endif

    // ESP_LOGI(TAG, "Frequency(Hz): %d", FREQ);
    // ESP_LOGI(TAG, "Output points num: %d\n", OUTPUT_POINT_NUM);
}

static int freqs[] = {100, 1000, 200, 1000, 300} ;
static int freqs_len = 5;



#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST 0

// change this to make the song slower or faster
int tempo = 144;

// change this to whichever pin you want to use
int buzzer = 11;

// notes of the moledy followed by the duration.
// a 4 means a quarter note, 8 an eighteenth , 16 sixteenth, so on
// !!negative numbers are used to represent dotted notes,
// so -4 means a dotted quarter note, that is, a quarter plus an eighteenth!!
int melody[] = {
  NOTE_E5, 8, NOTE_D5, 8, NOTE_FS4, 4, NOTE_GS4, 4, 
  NOTE_CS5, 8, NOTE_B4, 8, NOTE_D4, 4, NOTE_E4, 4, 
  NOTE_B4, 8, NOTE_A4, 8, NOTE_CS4, 4, NOTE_E4, 4,
  NOTE_A4, 2
};

int notes = sizeof(melody) / sizeof(melody[0]) / 2;

void app_main(void)
{
    esp_err_t ret;
    example_timer_init(TIMER_0, WITH_RELOAD);

    ret = dac_output_enable(DAC_CHAN);
    ESP_ERROR_CHECK(ret);

    log_info();
    g_index = 0;

    // int freq = 3000;

    // prepare_data(freq);

    // int freq_idx = 0;

    // this calculates the duration of a whole note in ms (60s/tempo)*4 beats
    int wholenote = (60000 * 4) / tempo;

    int divider = 0, noteDuration = 100;

    int thisNote = 0;
    while(1){
        thisNote = thisNote + 2;
        if (thisNote > notes * 2){
            thisNote = 0;
        }
        
        vTaskDelay((int)(noteDuration/10));
        divider = melody[thisNote + 1];
        if (divider > 0) {
             // regular note, just proceed
            noteDuration = (wholenote) / divider;
        } else if (divider < 0) {
            // dotted notes are represented with negative durations!!
            noteDuration = (wholenote) / abs(divider);
            noteDuration *= 1.5; // increases the duration in half for dotted notes
        }
        if(2*melody[thisNote] > 100){
            prepare_data(2*melody[thisNote]);
        }else{
            prepare_data(100);
        }
        

        printf("note: %d\ndura: %d\n\n", melody[thisNote], noteDuration);
        
        
    }
}
