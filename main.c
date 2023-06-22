#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"

#include <stdio.h>
#include <memory.h>
#include <time.h>

#include "micro_mon.h"
#include "RP2040-Zero_led.h"

float CurrentMeasure(void);
//====================================================================================
// Process character from stdin (via usb serial)
//====================================================================================
void process_stdin_char(int c)
{
    printf("Key %c, uptime: %d min\n", c, (int)(get_absolute_time()/(1000000*60)));

	switch(c){
		case 'b':
			printf("Built: "__DATE__" "__TIME__"\n");
			break;
		case 'a':
			adc_main();
			break;
		case 'c':
			CurrentMeasure();
			break;
	}
}

//====================================================================================
// Main loop
//====================================================================================
int main()
{
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

    printf("starting main loop\n");

    int green_val = 0;
    while (1){
        sleep_ms(20);
        int c =  getchar_timeout_us(0);
        if (c > 0) process_stdin_char(c);

        // Show a sign of life.
        green_val -= 1;
		if (green_val < 0) green_val = 64;
        RGB_set((green_val << 15) & 0xff0000);
    }
}
