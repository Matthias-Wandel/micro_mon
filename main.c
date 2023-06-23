//====================================================================================
// Microwave monitoring main module
// Monitors current going into microwave oven with a current transformers,
// alerts if food is possiblyu left inside the microwave after cooking.
//
// This runs on a Wavershare RPI2040-Zero
// could run on a regular pi PICO just fine, but the colour LED indication
// would not be there because the color LED is only on th RPI2040-Zero
//====================================================================================

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"

#include <stdio.h>
#include <memory.h>
#include <time.h>

#include "micro_mon.h"
#include "RP2040-Zero_led.h"

//====================================================================================
// Alert beep
//====================================================================================
void DoAlertBeep(void)
{
    printf("Alert beep!\n");
    #define BEEP_PIN 14
    #define BEEP_PIN2 15
    #define BEEP_DELAY 600
    gpio_init(BEEP_PIN);
    gpio_init(BEEP_PIN2);
    gpio_set_dir(BEEP_PIN, GPIO_OUT);
    gpio_set_dir(BEEP_PIN2, GPIO_OUT);

    for (int a=0;a<256*3;a++){
        int Delay = a & 255;
        if (Delay >= 128) Delay = 256-Delay;
        Delay = 600-Delay*2;

        gpio_put(BEEP_PIN, true);
        gpio_put(BEEP_PIN2, false);
        sleep_us(Delay);
        gpio_put(BEEP_PIN, false);
        gpio_put(BEEP_PIN2, true);
        sleep_us(Delay);
    }
}

// Microwave states, based on power consumption
#define ST_DONE           0
#define ST_DOOR_OPEN      1
#define ST_MICROWAVING    2

//====================================================================================
// Monitor microwave and determine if take out food reminder is due
//====================================================================================
void MicrowaveMonitor(void)
{
    int CurrentState = ST_DONE;
    int LastState = ST_DONE;
    int food_in_microwave = false;
    int in_state_count = 0;
    int skip_delay = false;

    while (1){
        float current = GetCurrent();

        if (current > 30){
            // Microwaving (or at least fan running)
            CurrentState = ST_MICROWAVING;
            RGB_set(0x006000); // Show red.
        }else if (current > 5.0){
            // Microwave door is open
            CurrentState = ST_DOOR_OPEN;
            RGB_set(0x204000); // Show yellow.
        }else{
            // Microwave idle (door closed, not microwaving)
            CurrentState = ST_DONE;
            RGB_set(0x000020); // Show blue.
        }

        printf("c=%5.1f st=%d %s\n",current, CurrentState, food_in_microwave?"food!":"");

        if (CurrentState == LastState){
            in_state_count += 1;
            if (in_state_count == 7){
                if (CurrentState == ST_MICROWAVING) food_in_microwave = true;
                if (CurrentState == ST_DOOR_OPEN && food_in_microwave){
                    printf("Food was taken out\n");
                    food_in_microwave = false;
                }
            }
            if (CurrentState == ST_DONE && food_in_microwave == true){
                if ((in_state_count & 3) == 0){
                    // blink bright red -- food in microwave!
                    printf("blink red\n");
                     RGB_set(0x00B000);
                }
                if (in_state_count % 60 == 0){ 
					// roughly every 30 seconds, aert that food is left in the microwave.
                    printf("Take out the food\n");
                    DoAlertBeep();
                    skip_delay = true;
                }
            }
        }else{
            printf("State changed to %d\n",CurrentState);
            LastState = CurrentState;
            in_state_count = 0;
        }

        int c =  getchar_timeout_us(0);
        if (c == 'x'){
			// if x is pressed on sria, go to diagnostics console
            printf("Back to test menu\n");
            return;
        }
		
        if (skip_delay){
            skip_delay = false;
            continue;
        }
        sleep_ms(250);
    }
}


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
        case 'l':
            //Beep
            DoAlertBeep();
            break;
        case 'm':
            MicrowaveMonitor();
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
    multicore_launch_core1(core1_entry);

    sleep_ms(500);
    printf("Starting\n");
    RGB_set(0x006000); // Send red
    sleep_ms(500);
    printf("red\n");
    RGB_set(0x006000); // Send red
    sleep_ms(500);
    printf("green\n");
    RGB_set(0x060000); // Send green
    sleep_ms(500);
    printf("blue\n");
    RGB_set(0x000060); // Send blue
    sleep_ms(500);
    RGB_set(0x000000); // black
    sleep_ms(500);
	
    MicrowaveMonitor();

    printf("Entering test menu\n");

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
