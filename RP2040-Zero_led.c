#include "RP2040-Zero_led.h"
#include "hardware/gpio.h"

#define LED_PIN 16 // for Pico RP2040-Zero
#define SET_LED(x) gpio_put(LED_PIN, x);

//====================================================================================
// Dealy loop for sending to RGB LED
//====================================================================================
static void delay_loop(volatile uint32_t count) {
    asm volatile(
        "1: sub %0, %0, #1\n"
        "bne 1b\n"
        : "+r" (count)  // Output list, '+' means read-write operand
        :               // Input list
        : "cc"          // Clobbers list, 'cc' for condition code register
    );
}

//====================================================================================
// Send to the RGB LED on pi pico..  This function should take a total of 30 microseconds.
//====================================================================================
void sendRGB(unsigned int RGBValue)
{
	for (int bit=0;bit<24;bit++){
		if (RGBValue & 0x800000){
			SET_LED(1);
			//_delay_us(0.8);
			delay_loop(30);
			
			SET_LED(0);
			//_delay_us(0.45);
			delay_loop(9);
		
		} else { // 0 bit
			SET_LED(1);
			//_delay_us(0.4);
			delay_loop(10);
			SET_LED(0);
			//_delay_us(0.85);
			delay_loop(29);
		}
		RGBValue <<= 1;
	}
	//delay_loop(3000); // 50 miroseconds would actually be enough delay.
}
