#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/binary_info.h"

#include <stdio.h>
#include <memory.h>
#include <math.h>
#include "micro_mon.h"


static void printhelp() {
    puts("\nCommands:");
    puts("c0, ...\t: Select ADC channel n");
    puts("s\t: Sample once");
    puts("S\t: Sample many");
    puts("m\t: Many samples individually");
    puts("t\t: Read temperature");
    puts("w\t: Wiggle pins");
    puts("x\t: Exit");
}

//void __not_in_flash_func(adc_capture)(uint16_t *buf, size_t count) {
static void adc_capture(uint16_t *buf, size_t count) {

//https://github.com/raspberrypi/pico-examples/blob/master/adc/dma_capture/dma_capture.c
// Line 69]
// set adc_set_clkdiv -- 48 mhz clock, want 20 samples per 60 hz cycles
// so should set divisor to 40,000?  Or just time it via the CPU.


    adc_fifo_setup(true, false, 0, false, false);
    adc_run(true);
    for (int i = 0; i < count; i = i + 1)
        buf[i] = adc_fifo_get_blocking();
    adc_run(false);
    adc_fifo_drain();
}


//====================================================================================
// ADC testing code main
//====================================================================================
int adc_main(void)
{
    adc_init();
    adc_set_temp_sensor_enabled(true);

    const float conversion_factor = 3.3f / (1 << 12);

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
            case '0':
            case '1':
            case '2':
            case '3':
                adc_select_input(c - '0');
                printf("Switched to channel %c\n", c);
                break;

            case 's': {
                uint32_t result = adc_read();
                                printf("\n%d -> %f V\n", result, result * conversion_factor);
                break;
            }
            case 'S':
            case 'm': {
                #define N_SAMPLES 1000
                uint16_t sample_buf[N_SAMPLES];
                printf("\nStarting capture %d samples\n",N_SAMPLES);
                absolute_time_t start, end;

                start = get_absolute_time();
                if (c == 'S'){
                    adc_capture(sample_buf, N_SAMPLES); // Gets about 500k samples per seconds
                }else{
                    // 'm' was pressed.
                    for (int a=0;a<1000;a++){
                        sample_buf[a] = adc_read();
                        sleep_ms(1);
                    }
                }
                end = get_absolute_time();

                printf("Time for 1000 samples: %6.3f ms\n",(end-start)/1000.0);

                short Histogram[1<<12];
                int sum = 0;
                memset(Histogram, 0, sizeof(Histogram));
                for (int i = 0; i < N_SAMPLES; i = i + 1){
                    sum += adc_read();
                    printf("%5d", sample_buf[i]);
                    if (i % 10 == 9) printf("\n");
                    Histogram[sample_buf[i]] += 1;
                }

                float result = sum/1000.0;
                const float conversion_factor = 3.3f / (1 << 12);
                printf("\n1000 readings average: %fx -> %f V\n", result, result * conversion_factor);


                int last = 0;
                for (int i=0;i<1<<12;i++){
                    // show histogram.  it turns out values ending with 3 lsbs set are 1/2 top 1/4 as likely
                    // Each increment represents 0.8 millivolts.  stddev with something hooked up is about 1.5 increments.
                    if (Histogram[i]){
                        if (last && (last != i-1)){
                            printf(".....\n");
                        }
                        last=i;
                        printf("%4d: %4d ",i,Histogram[i]);
                        for (int a=0;a<Histogram[i];a+=5) putchar('#');
                        putchar('\n');
                    }
                }
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

                int adci = adc_read();
                float adc = (float)adci * conversion_factor;
                float tempC = 27.0f - (adc - 0.706f) / 0.001721f;
                printf("\nTemperature = %f (adc=%fV or %d)\n",tempC, adc, adci);

                break;
            }
            case 'x':
                return 0;

            case '\n':
            case '\r':
                break;
            default:
                printf("\nUnrecognised command: %c\n", c);
            case 'h':
                printhelp();
                printhelp();
                break;
        }
    }
}

//====================================================================================
// Do running current measurements
//====================================================================================
static int running_average = (2048*4) << 16; // Start running average at mid-point
static unsigned variance_running = 0;
float CurrentMeasure(void)
{
    adc_init();
    adc_select_input(1);
	int a;
	for (a=0;a<400;a++){
		int adc = 0;
		for (int n=0;n<4;n++){
			// Average 4 samples.  this provides some amount of low-pass filtering.
			adc = adc + adc_read();
			sleep_ms(1);
		}
		
		printf("%04d ",adc); // adc is in 4x actual ADC values.  Don't divide back down for extra integer precision.
		
		running_average = running_average + (adc << 8) - (running_average >> 8); // Averaging time constant is 256 readings
		
		int deviation = adc-(running_average>>16);  // This can get to 8000
		int variance = (deviation*deviation)>>4;    // This number can get to 4 million regularly
		if (variance > 4000000) variance = 4000000; // Avoid overflows.
		
		variance_running = variance_running + (variance << 1) - (variance_running >> 7); // Averaging time constant is 128 readings.
	    //  variance_running is 256 times actual A/D values variance
		
		if ((a & 7) == 7){
			printf(" a=%4d vr=%d dv=%5.1f\n",running_average>>18, variance_running>>10, sqrt(variance_running)/16);
		}
	}
	return sqrt(variance_running)/16;
}