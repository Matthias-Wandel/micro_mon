#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"

#include <stdio.h>
#include <memory.h>
#include <time.h>

#include "sensor_remote.h"
#include "RP2040-Zero_led.h"

//#define LED_PIN 25 // For plain vanila pi picoo (not W)
#define LED_PIN 16 // for Pico RP2040-Zero
#define SET_LED(x) gpio_put(LED_PIN, x);




//====================================================================================
// Process character from stdin (via usb serial)
//====================================================================================
void process_stdin_char(int c)
{
    printf("Key %c, uptime: %d min\n", c, (int)(get_absolute_time()/(1000000*60)));

	if (c == 'b'){
		printf("Built: "__DATE__" "__TIME__"\n");
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

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);


    while (1) {
		printf("green\n");
		int time1 = (int)get_absolute_time();
        sendRGB(0x060000); // Example: Send full red, no green, no blue
		int time2 = (int)get_absolute_time();
		printf("send took %d us\n",time2-time1);
        sleep_ms(500); // Just delay for a while

		time1 = (int)get_absolute_time();
		time2 = (int)get_absolute_time();
		printf("nothing took %d us\n",time2-time1);

		
		printf("red ");
        sendRGB(0x006000); // Example: Send full red, no green, no blue
        sleep_ms(500); // Just delay for a while

		printf("blue\n");
        sendRGB(0x000060); // Example: Send full red, no green, no blue
        sleep_ms(2000); // Just delay for a while
    }

    multicore_launch_core1(core1_entry);

    for (int n=1;n>=0;n--){
        sleep_ms(500);
        SET_LED(1);
        sleep_ms(500);
        SET_LED(0);
        printf("Starting in %d abs:%d\n",n, get_absolute_time());
    }
    printf("====================================================\n");


    printf("starting main loop\n");

    while (1){
        my_sleep_ms(20);
        int c =  getchar_timeout_us(0);
        if (c > 0) process_stdin_char(c);

    }
}


//============================================================================================
/*
#define LED_PORT   PORTB
#define LED_DDR    DDRB

static void sendBit(bool bitVal) {
    if (bitVal) { // 1 bit
		SET_LED(1);
        _delay_us(0.8);
		SET_LED(0);
        _delay_us(0.45);
    } else { // 0 bit
        SET_LED(1);
        _delay_us(0.4);
		SET_LED(0);
        _delay_us(0.85);
    }
}

static void sendByte(unsigned char byte) {
    for (unsigned char bit = 0; bit < 8; bit++) {
        sendBit((byte & (1 << (7 - bit))));
    }
}

void cycle_loop(int cycles)
{
}
*/

int xmain(void) {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (1) {
		printf("red");
        sendRGB(0xff0000); // Example: Send full red, no green, no blue
        sleep_ms(500); // Just delay for a while
		
		printf("green");
        sendRGB(0x00ff00); // Example: Send full red, no green, no blue
        sleep_ms(500); // Just delay for a while

		printf("blue");
        sendRGB(0x0000ff); // Example: Send full red, no green, no blue
        sleep_ms(500); // Just delay for a while
    }
}
