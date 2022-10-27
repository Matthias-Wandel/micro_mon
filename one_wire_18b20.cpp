#include <stdio.h>
#include <memory.h>
#include "one_wire.h"

//====================================================================================
// Code to try out one_wire.cpp
//====================================================================================
extern "C" int ds18b20_main()
{
    printf("\nds18b20 main\n");

    #define ONE_WIRE_BUS 28
    One_wire oneWire(ONE_WIRE_BUS);

    oneWire.init();

    int n = oneWire.find_and_count_devices_on_bus();
    printf("\n1-wire Devices found: %d\n",n);

    for (int a=0;a<n;a++){
        rom_address_t addr = oneWire.get_address(a);
        unsigned char * addr_sane = (unsigned char *)&addr;
        printf("  (%d) ",a);
        for (int i=6;i>0;i--){
            printf("%02x",addr_sane[i]);
        }
        int wait_ms = oneWire.convert_temperature(addr,0,0); // How long conversion *should* take, but return is useless.
        printf("\n");
    }

    printf("\nTemperature readings:\n");

    for (int a=0;a<n;a++){
        rom_address_t addr = oneWire.get_address(a);
        unsigned char * addr_sane = (unsigned char *)&addr;
        printf("   %02x%02x", addr_sane[2],addr_sane[1]);
    }
    printf("\n");

    // Take 10 readings each, so we can see consistency
    for (int nr=0;nr<10;nr++){
        for (int a=0;a<n;a++){
            rom_address_t addr = oneWire.get_address(a);
            int wait_ms = oneWire.convert_temperature(addr,0,0); // How long conversion *should* take
        }

        sleep_ms(800); // 500 ms fast enough for brand name devices.
        // Cheap chinese clones need 800 miliseconds to have a conversion ready.

        for (int a=0;a<n;a++){
            rom_address_t addr = oneWire.get_address(a);
            unsigned char * addr_sane = (unsigned char *)&addr;
            printf(" %6.2f", addr_sane[1], oneWire.temperature(addr, 0));
        }
        printf("\n");
    }

}