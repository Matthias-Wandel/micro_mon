//====================================================================================
// Code to program the LED on the Waveshare RP2040-Zero (a Raspberry pi pico variant)
//====================================================================================

#include "RP2040-Zero_led.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"

#define LED_PIN 16 // for Pico RP2040-Zero on board fancy RGB led
//====================================================================================
// Initialize RGB led
//====================================================================================
void RGB_init(void)
{
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
}

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
// Send to the RGB LED on waveshare RP2040-Zero.  This function should take a total of 80 microseconds.
//====================================================================================
void RGB_set(unsigned int RGBValue)
{
    unsigned int interrupt_was = save_and_disable_interrupts();
    
    for (int bit=0;bit<24;bit++){
        if (RGBValue & 0x800000){
            gpio_put(LED_PIN,1);
            //_delay_us(0.8);
            delay_loop(32);
            gpio_put(LED_PIN,0);
            //_delay_us(0.45);
            delay_loop(20-3);
        } else { // 0 bit
            gpio_put(LED_PIN,1);
            //_delay_us(0.4);
            delay_loop(16);
            gpio_put(LED_PIN,0);
            //_delay_us(0.85);
            delay_loop(37-3);
        }
        RGBValue <<= 1;
    }
    restore_interrupts(interrupt_was);
	
	// Need 50 microseconds after sending stuff to LED, otherwise
	// LED assumes it is to pass on the next bits to the next LED in the chain.
	// This delay is unnecessary if we can assume this code won't be called again immediately.
    delay_loop(2000);
}
