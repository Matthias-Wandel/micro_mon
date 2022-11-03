#include <stdio.h>
#include <memory.h>
#include "one_wire.h"
#include "sensor_remote.h"

//====================================================================================
// My code to use one_wire.cpp to get a set of readings.
//====================================================================================
extern "C" int ds18b20_main()
{
    printf("\nds18b20 main\n");

    #define MAX_SENSORS 20  // If more than that many 18b20s, ignore some of them.

    #define ONE_WIRE_BUS 18 // GPIO 18 is pin 24 on the board.

    One_wire oneWire(ONE_WIRE_BUS);
    oneWire.init();

    int n = oneWire.find_and_count_devices_on_bus();
    printf("\n1-wire Devices found: %d\n",n);

	if (n > MAX_SENSORS) n = MAX_SENSORS;

    for (int a=0;a<n;a++){
        rom_address_t addr = oneWire.get_address(a);
        unsigned char * addr_sane = (unsigned char *)&addr;
        printf("  (%d) ",a);
        for (int i=6;i>0;i--){
            printf("%02x",addr_sane[i]);
        }
        printf("\n");
    }
    printf("\nTemperature readings:\n");
    for (int a=0;a<n;a++){
        rom_address_t addr = oneWire.get_address(a);
        unsigned char * addr_sane = (unsigned char *)&addr;
        printf("   %02x%02x", addr_sane[2],addr_sane[1]);
    }
    printf("\n");

	float temp_sums[MAX_SENSORS];
	memset(temp_sums, 0, sizeof(temp_sums));
	const int NUM_AVERAGE = 4;

    // Take 4 readings each for consistency
    for (int nr=0;nr<NUM_AVERAGE;nr++){
        for (int a=0;a<n;a++){
            rom_address_t addr = oneWire.get_address(a);
            int wait_ms = oneWire.convert_temperature(addr,0,0); // How long conversion *should* take
        }

        tcp_sleep_ms(900); // 500 ms fast enough for brand name devices.
        // Cheap chinese clones need 800 miliseconds to have a conversion ready.

        for (int a=0;a<n;a++){
            rom_address_t addr = oneWire.get_address(a);
            unsigned char * addr_sane = (unsigned char *)&addr;
			float temp = oneWire.temperature(addr, 0);
            printf(" %6.2f", addr_sane[1], temp);
			temp_sums[a] += temp;
        }
        printf("\n");
    }
    for (int a=0;a<n;a++){
        rom_address_t addr = oneWire.get_address(a);
        unsigned char * addr_sane = (unsigned char *)&addr;
        printf("  (%d) ",a);
        for (int i=6;i>0;i--){
            printf("%02x",addr_sane[i]);
        }
        printf(" t=%5.2f\n",temp_sums[a]/NUM_AVERAGE);
    }
}