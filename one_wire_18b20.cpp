#include <stdio.h>
#include <memory.h>
#include "one_wire.h"

//====================================================================================
// Code to try out one_wire.cpp
//====================================================================================
extern "C" int ds18b20_main()
{
	printf("ds18b20 main\n");
	
	#define ONE_WIRE_BUS 28
	One_wire oneWire(ONE_WIRE_BUS);
	
	oneWire.init();
	
	int n = oneWire.find_and_count_devices_on_bus();
	printf("Devices found: %d\n",n);

	for (int a=0;a<n;a++){
		rom_address_t addr = oneWire.get_address(a);
		unsigned char * addr_sane = (unsigned char *)&addr;
		printf("  (%d) ",a);
		for (int i=0;i<10;i++){
			printf("%02x ",addr_sane[i]);
		}
		int wait_ms = oneWire.convert_temperature(addr,0,0); // How long conversion *should* take
		printf("  ms=%d\n",wait_ms);
	}
	
	sleep_ms(1000); // Need some dealy before new temperature reading is available.
	
	for (int a=0;a<n;a++){
		rom_address_t addr = oneWire.get_address(a);
		printf("  Temp(%d)=%5.2f\n", a, oneWire.temperature(addr, 0));
	}
}