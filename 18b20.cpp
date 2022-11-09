#include <stdio.h>
#include <memory.h>
#include "one_wire.h"
#include "sensor_remote.h"

//====================================================================================
// My code to use one_wire.cpp to get a set of readings.
//====================================================================================
extern "C" int ds18b20_read_sesnors(void * arg)
{
    #define MAX_SENSORS 20  // If more than that many 18b20s, ignore some of them.

    #define ONE_WIRE_BUS 18 // GPIO 18 is pin 24 on the board.

    One_wire oneWire(ONE_WIRE_BUS);
    oneWire.init();

    int num_s = oneWire.find_and_count_devices_on_bus();
	my_sleep_ms(0);
    printf("\n1-wire Devices found: %d\n",num_s);

	if (num_s > MAX_SENSORS) num_s = MAX_SENSORS;

    for (int a=0;a<num_s;a++){
        rom_address_t addr = oneWire.get_address(a);
        unsigned char * addr_sane = (unsigned char *)&addr;
        for (int i=6;i>0;i--){
            printf("%02x",addr_sane[i]);
        }
        printf(" ");
    }
	my_sleep_ms(0);
    printf("\nTemperature readings:\n");
    for (int a=0;a<num_s;a++){
        rom_address_t addr = oneWire.get_address(a);
        unsigned char * addr_sane = (unsigned char *)&addr;
        printf("   %02x%02x", addr_sane[2],addr_sane[1]);
		my_sleep_ms(0);
    }
    printf("\n");

	float temp_sums[MAX_SENSORS];
	memset(temp_sums, 0, sizeof(temp_sums));
	

    // Take several readings each for better accuracy
	const int NUM_AVERAGE = 4;
    for (int nr=0;nr<NUM_AVERAGE;nr++){
        for (int a=0;a<num_s;a++){
            rom_address_t addr = oneWire.get_address(a);
            int wait_ms = oneWire.convert_temperature(addr,0,0); // How long conversion *should* take
			my_sleep_ms(0);
        }

        my_sleep_ms(900); // 500 ms fast enough for brand name devices.
        // Cheap chinese clones need 800 miliseconds to have a conversion ready.

        for (int a=0;a<num_s;a++){
            rom_address_t addr = oneWire.get_address(a);
            unsigned char * addr_sane = (unsigned char *)&addr;
			float temp = oneWire.temperature(addr, 0);
            printf(" %6.2f", addr_sane[1], temp);
			temp_sums[a] += temp;
        }
        printf("\n");
    }

	const int LINE_LEN = 21;
	char ResponseStr[MAX_SENSORS*LINE_LEN+10];
	strcpy(ResponseStr, "abcdefg                                                                                          ");
	
    for (int line=0;line<num_s;line++){
        rom_address_t addr = oneWire.get_address(line);
        unsigned char * addr_sane = (unsigned char *)&addr;
		int str_index = LINE_LEN*line;
        for (int i=6;i>0;i--){
            sprintf(ResponseStr+str_index,"%02x",addr_sane[i]);
			str_index += 2;
        }
        sprintf(ResponseStr+str_index, ",t=%5.2f\n  ",temp_sums[line]/NUM_AVERAGE);
    }
	//printf("res=>>%s<<\n",ResponseStr);


	if (arg){
		// Request came from a TCP connection.
		SendResponse(arg, ResponseStr, LINE_LEN*num_s);
	}
}