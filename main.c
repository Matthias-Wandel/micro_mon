#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"

#include <stdio.h>
#include <memory.h>
#include <time.h>

#include "sensor_remote.h"
#include "RP2040-Zero_led.h"

//====================================================================================
// Process character from stdin (via usb serial)
//====================================================================================
void process_stdin_char(int c)
{
    printf("Key %c, uptime: %d min\n", c, (int)(get_absolute_time()/(1000000*60)));

    if (c == 'b'){
        printf("Built: "__DATE__" "__TIME__"\n");
    }
    if (c == 'a'){
        adc_main();
    }
}


//====================================================================================
// My stuff that needs to run periodically
//====================================================================================
static void my_periodic(void)
{
    static int LastTime;
    absolute_time_t NewTime = get_absolute_time();
    int Delay = ((int)NewTime)-LastTime;
    if (Delay > 50*1000){
        printf("Main loop dealy %5.2fms\n",Delay/1000.0);
    }
    LastTime = NewTime;
    
}

//====================================================================================
// Sleep while polling the TCP interface.
//====================================================================================
void my_sleep_ms(int ms)
{
    while (1){
        my_periodic();
        if (ms <= 0) break;
        int ms_do = ms > 10 ? 10 : ms;
        sleep_ms(ms_do);
        ms -= ms_do;
    }
}

//====================================================================================
// Remote sensor read program main
//====================================================================================
int main() {

    bi_decl(bi_program_description("RP2040-Zero"));
    stdio_init_all();

    RGB_init();


    printf("red\n");
    //int time1 = (int)get_absolute_time();
    RGB_set(0x006000); // Send red
    //int time2 = (int)get_absolute_time();
    //printf("send took %d us\n",time2-time1);
    sleep_ms(500); // Just delay for a while
    printf("green\n");
    RGB_set(0x060000); // Send green
    sleep_ms(500); // Just delay for a while
    printf("blue\n");
    RGB_set(0x000060); // Send blue
    sleep_ms(500); // Just delay for a while
    printf("green\n");
    RGB_set(0x060000); // Send green

    //multicore_launch_core1(core1_entry);

    printf("starting main loop\n");

    int green_val = 0;
    while (1){
        my_sleep_ms(20);
        int c =  getchar_timeout_us(0);
        if (c > 0) process_stdin_char(c);

        // Show sign of life.
        green_val += 1;
        RGB_set(green_val << 16);
        if (green_val > 32) green_val = 0;
    }
}

