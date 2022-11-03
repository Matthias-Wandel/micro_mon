#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/binary_info.h"
#include "pico/cyw43_arch.h"

#include <stdio.h>
#include <memory.h>
#include <time.h>

#include "sensor_remote.h"

#define NO_WIFI

#define SET_LED(x) cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, x)


typedef struct {
	void * arg;
	char * Url;
}Req_t;

static Req_t Request;

int ProcessRequest(void * arg, char * Url)
{
	Request.arg = arg;
	Request.Url = Url;
}


//====================================================================================
// My test program main.
//====================================================================================
int main() {

    bi_decl(bi_program_description("Sensor remote"));
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("failed to initialise cyw43\n");
        //return 1;
    }

    for (int n=3;n>=0;n--){
        sleep_ms(500);
        SET_LED(1);
        sleep_ms(500);
        SET_LED(0);
        printf("Starting in %d abs:%d\n",n, get_absolute_time());
    }

	ds18b20_main();

	tcp_server_setup();

    while (1){
        cyw43_arch_poll();
        sleep_ms(10);
    }

}
