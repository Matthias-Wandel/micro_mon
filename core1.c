//====================================================================================
// Code to run on the second core.
// Rather than figuring out how to have the A/D converter convert into memory
// in the background, I just dedicate the second core to reading the A/D converter
// and keeping an up to date current level that can be polled via GetCurrent()
//====================================================================================

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/multicore.h"

#include <stdio.h>
#include <math.h>
#include <time.h>
#include "micro_mon.h"

//====================================================================================
// Second core code -- Keeps a tally of current
//====================================================================================
static int running_average = (2048*4) << 16; // Start running average at mid-point
static unsigned variance_running = 0;
void core1_entry()
{
    adc_init();
    adc_select_input(1);
    printf("adc check: %d\n",adc_read());
    int a = 0;
    for (;;a++){
        int adc = 0;
        for (int n=0;n<4;n++){
            // Average 4 samples.  this provides some amount of low-pass filtering.
            adc = adc + adc_read();
            sleep_ms(1);
        }

        //printf("%04d ",adc); // adc is in 4x actual ADC values.  Don't divide back down for extra integer precision.

        running_average = running_average + (adc << 8) - (running_average >> 8); // Averaging time constant is 256 readings

        int deviation = adc-(running_average>>16);  // This can get to 8000
        int variance = (deviation*deviation)>>4;    // This number can get to 4 million regularly
        if (variance > 4000000) variance = 4000000; // Avoid overflows.

        variance_running = variance_running + (variance << 2) - (variance_running >> 6); // Averaging time constant is 64 readings.
        //  variance_running is 256 times actual A/D values variance

        if ((a & 1023) == 0){
            printf("adc_avg=%4d stdev=%5.1f\n",running_average>>18, sqrt(variance_running)/16);
        }
    }
}

//====================================================================================
// Return current current tally
//====================================================================================
float GetCurrent(void)
{
    return sqrt(variance_running)/16;
}




