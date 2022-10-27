#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/binary_info.h"
#include "pico/cyw43_arch.h"

#include <stdio.h>
#include <memory.h>
#include <time.h>

extern int adc_main(void);
extern int wifi_scan(void);
extern int tcp_server_main(void);
extern int ds18b20_main(void);

#define NO_WIFI

#ifdef NO_WIFI
	#define LED_PIN 25
	#define SET_LED(x) gpio_put(LED_PIN, x);
#else
	#define SET_LED(x) cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, x)
#endif
//====================================================================================
// My test program main.
//====================================================================================

int main() {

    bi_decl(bi_program_description("My do multiple things test program"));

    stdio_init_all();

#ifdef NO_WIFI
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
#else
    if (cyw43_arch_init()) {
        printf("failed to initialise cyw43\n");
        //return 1;
    }
#endif

    for (int n=3;n>=0;n--){
        sleep_ms(500);
        SET_LED(1)
        sleep_ms(500);
        SET_LED(0)
        printf("Starting in %d abs:%d\n",n, get_absolute_time());
    }

    while (1){
        puts("\nCommands:");
        puts("a\t: Adc mode");
        puts("w\t: Wifi scan");
        puts("t\t: Tcp server");
		puts("1\t: 18b20 test");

        char c = getchar();
        printf("%c (%d)", c,c);
        if (c == 'a') adc_main();
        if (c == 'w') wifi_scan();
        if (c == 't') tcp_server_main();
		if (c == '1') ds18b20_main();
    }


    int n=0;
    while (1) {
        printf("After iteration %d\n",n++);
        sleep_ms(900);
        SET_LED(1)
        sleep_ms(100);
        SET_LED(0)
    }
}
