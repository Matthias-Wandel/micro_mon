#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/binary_info.h"

#include <stdio.h>


//====================================================================================
// Code from ADC console
//====================================================================================
#define N_SAMPLES 1000
uint16_t sample_buf[N_SAMPLES];

static void printhelp() {
    puts("\nCommands:");
    puts("c0, ...\t: Select ADC channel n");
    puts("s\t: Sample once");
    puts("S\t: Sample many");
    puts("m\t: Many samples individually");
    puts("t\t: Read temperature");
    puts("w\t: Wiggle pins");
}

//void __not_in_flash_func(adc_capture)(uint16_t *buf, size_t count) {
static void adc_capture(uint16_t *buf, size_t count) {
    adc_fifo_setup(true, false, 0, false, false);
    adc_run(true);
    for (int i = 0; i < count; i = i + 1)
        buf[i] = adc_fifo_get_blocking();
    adc_run(false);
    adc_fifo_drain();
}


int adc_main(void) {
    adc_init();
    adc_set_temp_sensor_enabled(true);

    // Set all pins to input (as far as SIO is concerned)
    gpio_set_dir_all_bits(0);
    for (int i = 2; i < 30; ++i) {
        gpio_set_function(i, GPIO_FUNC_SIO);
        if (i >= 26) {
            gpio_disable_pulls(i);
            gpio_set_input_enabled(i, false);
        }
    }

    printf("\n===========================\n");
    printf("RP2040 ADC and Test Console\n");
    printf("===========================\n");
    printhelp();

    while (1) {
        char c = getchar();
        printf("%c", c);
        switch (c) {
            case 'c':
                c = getchar();
                printf("%c\n", c);
                if (c < '0' || c > '7') {
                    printf("Unknown input channel\n");
                    printhelp();
                } else {
                    adc_select_input(c - '0');
                    printf("Switched to channel %c\n", c);
                }
                break;
            case 's': {
                uint32_t result = adc_read();
                const float conversion_factor = 3.3f / (1 << 12);
                printf("\n%d -> %f V\n", result, result * conversion_factor);
                break;
            }
            case 'S': {
                printf("\nStarting capture %d samples\n",N_SAMPLES);
                absolute_time_t start, end;

                start = get_absolute_time();
                adc_capture(sample_buf, N_SAMPLES); // Gets about 500k samples per seconds
                end = get_absolute_time();

                printf("Time: %6.3f ms\n",(end-start)/1000.0);

                for (int i = 0; i < N_SAMPLES; i = i + 1){
                    printf("%5d", sample_buf[i]);
                    if (i % 10 == 9) printf("\n");
                }
                break;
            }
            case 'm': {
                int sum = 0;
                absolute_time_t start, end;
                start = get_absolute_time();
                for (int a=0;a<1000;a++){
                    sum += adc_read();
                }
                end = get_absolute_time();
                const float conversion_factor = 3.3f / (1 << 12);
                float result = sum/1000.0;
                printf("\n1000 readings average: %fx -> %f V\n", result, result * conversion_factor);
                printf("Time: %6.3f ms\n",(end-start)/1000.0);
                break;
            }
            case 'w':
                printf("\nPress any key to stop wiggling\n");
                int i = 1;
                gpio_set_dir_all_bits(-1);
                while (getchar_timeout_us(0) == PICO_ERROR_TIMEOUT) {
                    // Pattern: Flash all pins for a cycle,
                    // Then scan along pins for one cycle each
                    i = i ? i << 1 : 1;
                    gpio_put_all(i ? i : ~0);
                }
                gpio_set_dir_all_bits(0);
                printf("Wiggling halted.\n");
                break;

            case 't':  {// My read temperature
                adc_set_temp_sensor_enabled(true);
                adc_select_input(4);

                const float conversionFactor = 3.3f / (1 << 12); // 12-bit conversion, assume max value == ADC_VREF == 3.3 V
                int adci = adc_read();
                float adc = (float)adci * conversionFactor;
                float tempC = 27.0f - (adc - 0.706f) / 0.001721f;
                printf("\nTemperature = %f (adc=%fV or %d)\n",tempC, adc, adci);

                break;
            }

            case '\n':
            case '\r':
                break;
            case 'h':
                printhelp();
                break;
            default:
                printf("\nUnrecognised command: %c\n", c);
                printhelp();
                break;
        }
    }
}

