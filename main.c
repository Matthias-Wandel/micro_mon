#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"

#include <stdio.h>
#include <memory.h>
#include <time.h>

#include "sensor_remote.h"

//#define HAVE_WIFI

#ifdef HAVE_WIFI
	#define SET_LED(x) cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, x)
#else
	#define LED_PIN 25
	#define SET_LED(x) gpio_put(LED_PIN, x);
#endif


typedef struct {
    void * arg;
    char * Url;
}Req_t;

static Req_t Request; // Could make this a queue, but no need for that so far.

static bool HavePzem = false;

//====================================================================================
// Handle request received thru tcp_server.c module
//====================================================================================
int QueueRequest(void * arg, char * Url)
{
    printf("Queue request: %s\n", Url);
    Request.arg = arg;
    Request.Url = Url;
}

//====================================================================================
// Send response back to tcp_server.c module
//====================================================================================
void SendResponse(void * arg, char * ResponseStr, int len)
{
    if (arg){
        printf("SEND:%s (l=%d)",ResponseStr,len);
        if (len < 0) len = strlen(ResponseStr);
        TCP_EnqueueForSending(arg, ResponseStr, len, 0);
        printf("queued done\n");
    }
}


//====================================================================================
// My stuff that needs to run periodically
//====================================================================================
static void my_periodic(void)
{
    static int LastTime;
    int NewTime = get_absolute_time();
    int Delay = NewTime-LastTime;
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
        cyw43_arch_poll();
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

    bi_decl(bi_program_description("Sensor remote"));
    stdio_init_all();

#ifdef HAVE_WIFI
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    if (cyw43_arch_init()) {
        printf("failed to initialise cyw43\n");
        //return 1;
    }
#else
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
#endif

    multicore_launch_core1(core1_entry);

    for (int n=1;n>=0;n--){
        sleep_ms(500);
        SET_LED(1);
        sleep_ms(500);
        SET_LED(0);
        printf("Starting in %d abs:%d\n",n, get_absolute_time());
    }
    printf("====================================================\n");

#ifdef HAVE_WIFI
    tcp_server_setup();
#endif

    HavePzem = PzemInit();

    printf("starting main loop\n");

    while (1){
        my_sleep_ms(20);

        if (Request.arg){
            printf("process request\n");
            SendResponse(Request.arg,"Built="__DATE__", Up=",-1);
            char UpStr[32];
            sprintf(UpStr, "%dm\n",get_absolute_time()/(1000000*60));
            SendResponse(Request.arg,UpStr,-1);
            GetAnemometerFrequency(Request.arg);
            ds18b20_read_sesnors(Request.arg);
            TCP_EnqueueForSending(Request.arg, "end\n",4, 1); // Indicate end of stuff to send.
            Request.arg = NULL;
        }
    }
}
