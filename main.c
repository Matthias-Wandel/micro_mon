#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"

#include <stdio.h>
#include <memory.h>
#include <time.h>

#include "sensor_remote.h"

#define HAVE_WIFI

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

static Req_t Request;         // Current request
static Req_t Request_waiting; // Plus potentially one queued.

static bool HavePzem = false;

static const int IpRefreshInterval = (6*3600*1000000); // Six hours.
static int NextIpRefresh = IpRefreshInterval;

//====================================================================================
// Handle request received thru tcp_server.c module
//====================================================================================
int QueueRequest(void * arg, char * Url)
{
    printf("Queue request: %s\n", Url);

    // Send build and uptime right away (no need to wait on that)
    char UpStr[40];
    sprintf(UpStr, "Built="__DATE__", Up=%dm\n",(int)(get_absolute_time()/(1000000*60)));
    SendResponse(arg,UpStr,-1);

    if (Request_waiting.arg == NULL){
        Request_waiting.arg = arg;
        Request_waiting.Url = Url;
    }else{
        // Busy plus one queued, don't handle it.
        TCP_EnqueueForSending(arg, "busy\n",5, 1);
    }

}

//====================================================================================
// Send response back to tcp_server.c module
//====================================================================================
void SendResponse(void * arg, char * ResponseStr, int len)
{
    if (arg){
        printf("SEND:%s\n",ResponseStr);
        if (len < 0) len = strlen(ResponseStr);
        TCP_EnqueueForSending(arg, ResponseStr, len, 0);
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
   
    int64_t ToIpRefresh = NextIpRefresh-NewTime;
    //printf("ToIpRefresh = %d sec",(int)(ToIpRefresh>>20));
    if (ToIpRefresh < 0){
        printf("Refreshing IP addr========\n");
        tcp_server_refresh_addr();
        NextIpRefresh = NewTime+IpRefreshInterval;
    }
        
    
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
// Process character from stdin (via usb serial)
//====================================================================================
void process_stdin_char(int c)
{
    printf("Key %d, uptime: %d min\n", c, (int)(get_absolute_time()/(1000000*60)));
}

//====================================================================================
// Remote sensor read program main
//====================================================================================
int main() {

    bi_decl(bi_program_description("Sensor remote"));
    stdio_init_all();

#ifdef HAVE_WIFI
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
        int c =  getchar_timeout_us(0);
        if (c > 0) process_stdin_char(c);

        if (Request.arg == NULL && Request_waiting.arg){
            // Dequeue to handle.
            Request = Request_waiting;
            Request_waiting.arg = NULL;
        }

        if (Request.arg){
            printf("process request\n");
            GetAnemometerFrequency(Request.arg);
            if (HavePzem) PzemReport(Request.arg);
            ds18b20_read_sesnors(Request.arg);
            TCP_EnqueueForSending(Request.arg, "end\n",4, 1); // Indicate end of stuff to send.
            Request.arg = NULL;
        }
    }
}
