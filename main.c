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

//====================================================================================
// Benchmark code
//====================================================================================
int benchmark_main(void)
{
    absolute_time_t start, end;
    start = get_absolute_time(); // Appears to return microseconds
    int b=0;
    for (int a=0;a<100000000;a++){
        b += a;
    }
    end = get_absolute_time();

    printf("Solutions:%lld\n",b);
    printf("Time: %lld --> %lld\n",start,end);
    printf("Elapsed: %lld\n",end-start);
    printf("Elapsed ms: %d\n",to_ms_since_boot(end-start));

    return 0;
}

//====================================================================================
// My main
//====================================================================================

int main() {

    bi_decl(bi_program_description("My do multiple things test program"));

    stdio_init_all();
    
//    if (cyw43_arch_init()) {
//        printf("failed to initialise cyw43\n");
//        return 1;
//    }

    for (int n=5;n>=0;n--){
        sleep_ms(500);
//        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(500);
//        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        //printf("adc = %d",adc_read())
        printf("Starting in %d abs:%d\n",n, get_absolute_time());
    }

    while (1){
        puts("\nCommands:");
        puts("a\t: Adc mode");
        puts("w\t: Wifi scan");
        puts("t\t: Tcp server");

        char c = getchar();
        printf("%c (%d)", c,c);
        if (c == 'a') adc_main();
        if (c == 'w') wifi_scan();
        if (c == 't') tcp_server_main();
    }


    int n=0;
    while (1) {
        printf("After iteration %d\n",n++);
        sleep_ms(900);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(100);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    }
}

//  Try RS485 stuff?
//