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

//====================================================================================
// Handle request received thru tcp_server.c module
//====================================================================================
int QueueRequest(void * arg, char * Url)
{
	printf("Process request: %s\n", Url);

// Somehow sending the response after the receive function returns doesn't work, so don't queue it for now.	
ds18b20_read_sesnors(arg);	
return;
	
	Request.arg = arg;
	Request.Url = Url;
}

void SendResponse(void * arg, char * ResponseStr, int len)
{
	if (arg){
		printf("send response to TCP:\n%s\n",ResponseStr);		
		tcp_server_send_data(arg, (uint8_t *) ResponseStr, len);
	}
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

    for (int n=1;n>=0;n--){
        sleep_ms(500);
        SET_LED(1);
        sleep_ms(500);
        SET_LED(0);
        printf("Starting in %d abs:%d\n",n, get_absolute_time());
    }
	printf("====================================================\n");

	//ds18b20_read_sesnors(NULL);
	tcp_server_setup();

	printf("startin main loop\n");
    while (1){
        cyw43_arch_poll();
        sleep_ms(10);
		
		if (Request.arg){
			printf("process request\n");
			ds18b20_read_sesnors(Request.arg);
			//tcp_finished_sending(Request.arg);
			Request.arg = NULL;
		}
	}

}
