#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/multicore.h"

#include <stdio.h>
#include <memory.h>
#include <time.h>

#include "sensor_remote.h"

#define TRANS_COUNT_SIZE 32 // Must be a power of two.

#define ANEMOMETER_PIN 18

static int TransCounts[TRANS_COUNT_SIZE]; //Transition counts, half second bins.
static int TransWriteIndex = 0;

//====================================================================================
// Work out anemometer frequency.
//====================================================================================
void GetAnemometerFrequency(void * arg)
{
    int index = TransWriteIndex;
    int tot = 0;
	const int num_average = 20; // 10 seconds worth.

    for (int a=0;a<num_average;a++){ //Total counts for number of bins.
        int ri = (index-1-a) & (TRANS_COUNT_SIZE-1);
        #if REPORT_STR
            printf("t(%d)=%d ",ri,TransCounts[ri]);
        #endif
        tot += TransCounts[ri];
    }
    //printf("Transitons = %d\n",tot);
    char ReportStr[50];
    sprintf(ReportStr,"Anm_freq=%5.1f\n",tot/(num_average));
    printf("Report: %s",ReportStr);
    if (arg) SendResponse(arg, ReportStr, strlen(ReportStr));
}


//====================================================================================
// Second core code -- runs tight timing stuff.
//====================================================================================
volatile int core2count = 0;
void core1_entry()
{
    
    gpio_set_dir(ANEMOMETER_PIN, GPIO_IN);

    static int NextSecond;
    NextSecond = get_absolute_time();

    int Transitions = 0;
    int PrevState = 0;

    #if REPORT_STR
        char report[200];
        int NumReport = 0;
    #endif

    printf("Core 2 in");
    for(;;){
        sleep_ms(1);
        core2count += 1;
        int state = gpio_get(ANEMOMETER_PIN);
        if (state != PrevState){
            PrevState = state;
            Transitions += 1;
        }
        #if REPORT_STR
            if (NumReport < sizeof(report)-1) report[NumReport++] = state ? '1' : ' ';
        #endif

        //printf("C=%4d A=%d\n",core2count,gpio_get(ANEMOMETER_PIN));
        int now = get_absolute_time();
        if (now-NextSecond > 0){
            #if REPORT_STR
                report[NumReport] = 0;
                report[70] = 0;
                printf("%s %d\n", report,Transitions);
                NumReport = 0
            #endif
            TransCounts[TransWriteIndex] = Transitions;
            TransWriteIndex = (TransWriteIndex+1) & (TRANS_COUNT_SIZE-1);
            //printf("WriteIndex = %d\n",TransWriteIndex);
            Transitions = 0;
            NextSecond = now + 500000; // Half seconds, actually.
        }

        //if ((core2count & 4095) == 0) GetAnemometerFrequency(NULL); // Test it.
    }
}
