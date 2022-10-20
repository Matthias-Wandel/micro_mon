#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/binary_info.h"

#include <stdio.h>
#include <memory.h>
#include <time.h>

int state = 0;
const uint LED_PIN = 25;

extern int adc_main(void);

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

    bi_decl(bi_program_description("This is a test binary."));
    bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));

    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    for (int n=7;n>=0;n--){
        sleep_ms(500);
        gpio_put(LED_PIN, 1);
        sleep_ms(500);
        gpio_put(LED_PIN, 0);
        //printf("adc = %d",adc_read())
        printf("Starting in %d abs:%d\n",n, get_absolute_time());
    }

    //benchmark_main();
    adc_main();
    printf("After abs time: %lld\n", get_absolute_time());

    int n=0;
    while (1) {
        gpio_put(LED_PIN, 0);
        sleep_ms(150);
        gpio_put(LED_PIN, 1);
        sleep_ms(150);
        gpio_put(LED_PIN, 0);
        sleep_ms(150);
        gpio_put(LED_PIN, 1);
        printf("After iteration %d\n",n++);

        sleep_ms(500);
    }
}

//  Try wifi
//  Try RS485 stuff?
//