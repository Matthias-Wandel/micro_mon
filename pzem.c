//==================================================================
// Code for reading PZEM-004T module.
//==================================================================

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#include "sensor_remote.h"

#define UART_ID uart1
#define BAUD_RATE 9600

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 8
#define UART_RX_PIN 9

#define MAX_RBYTES 32


// Pre-computed CRC table
static const unsigned short crcTable[] = {
    0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
    0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
    0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
    0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
    0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
    0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
    0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
    0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
    0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
    0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
    0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
    0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
    0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
    0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
    0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
    0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
    0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
    0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
    0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
    0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
    0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
    0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
    0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
    0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
    0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
    0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
    0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
    0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
    0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
    0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
    0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
    0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040
};

//==================================================================
// Caclulate CRC for modbus messages
//==================================================================
static unsigned short CalcCrc(unsigned char * data, int length)
{
    int crc = 0xFFFF;
    for (int a=0;a<length;a++){
        int nTemp = (data[a] ^ crc) & 0xff;
        crc >>= 8;
        crc ^= crcTable[nTemp];
    }
    return crc;
}

//==================================================================
// Send command, wait for response
//==================================================================
static int SendAndGetResponse(unsigned char * message, int length, unsigned char * rbytes)
{
    int n_rbytes;

    if (uart_is_readable(UART_ID)){
        printf("unread uart bytes\n");
        while (uart_is_readable(UART_ID)){
            static volatile int x;
            x = uart_get_hw(UART_ID)->dr;
        }
    }

    uart_write_blocking(UART_ID, message,length);
    unsigned short crc = CalcCrc(message, length);
    unsigned char crcArr[2];
    crcArr[0] = crc & 0xff;
    crcArr[1] = crc >> 8;
    uart_write_blocking(UART_ID, crcArr, 2);

    // Wait for a reply from the pzem-004T
    my_sleep_ms(65); // 65 ms is just enough wait for reading the


    // reply from read all.  I know how many bytes are coming,
    // so no need to be dynamic about the wait.

    int timeout = get_absolute_time()+20000; // Allow longer for first byte

    for (n_rbytes = 0;;n_rbytes<MAX_RBYTES){
        if (uart_is_readable(UART_ID)){
            //printf("g\n");
            rbytes[n_rbytes++] = uart_get_hw(UART_ID)->dr;
            timeout = get_absolute_time()+5000; // 5 ms without bytes means end of transmission.
        }else{
            int now = get_absolute_time();
            if ((now-timeout) > 0){
                break;
            }
            my_sleep_ms(1);
        }
    }

    if (n_rbytes < 5) return -1;

    crc = CalcCrc(rbytes, n_rbytes-2);
    unsigned short GotCrc = rbytes[n_rbytes-2] | (rbytes[n_rbytes-1] << 8);
    if (crc != GotCrc){
        printf("Wrong crc.  calc:%04x got: %04x\n", crc, GotCrc);
        return 0;
    }

    return n_rbytes;
}

//==================================================================
// Read all the readings (volt,amp,watt,watthours,freq,pf)
// This only updates about every 1.2 seconds.
//==================================================================
PzemFields_t ReadAll(int addr)
{
    // send command to read registers 0-10, default address 0xf8
    unsigned char SendBytes[] = {addr,4,0,0,0,10};
    unsigned char rbytes[MAX_RBYTES];
    PzemFields_t data = {};

    int n_rbytes = SendAndGetResponse(SendBytes, sizeof(SendBytes), rbytes);

    if (n_rbytes < 6 || (n_rbytes != rbytes[2]+5)){
        // Should receive 2 bytes header, 1 byt length, data + 2 bytes CRC.
        printf("Pzem Serial len err. Got %d\n",n_rbytes);
        return data;
    }

    // Now decode the data.
    data.Voltage = ((rbytes[3]<<8) + rbytes[4]) * 0.1;
    data.Current = ((rbytes[5]<<8) + rbytes[6] + (rbytes[7]<<16) + (rbytes[8] << 24)) * 0.001;
    data.Power = ((rbytes[9]<<8) + rbytes[10] + (rbytes[11]<<16) + (rbytes[12] << 12)) * 0.1;
    data.Energy = (rbytes[13]<<8) + rbytes[14] + (rbytes[15]<<16) + (rbytes[16] << 12);
    data.Freq = ((rbytes[17]<<8) + rbytes[18])*0.1;
    data.PowerFactor = ((rbytes[19]<<8) + rbytes[20]) * 0.01;

    printf("Volts=%5.1f Pow=%6.1f\n",data.Voltage, data.Power);
    //printf("Current =%6.3f A\n",data.Current);
    //printf("Energy  = %d Wh\n",data.Energy);
    //printf("Freq    =%5.1f Hz\n",data.Freq);
    //printf("Pf      = %4.2f\n",data.PowerFactor);

    return data;
}


//==================================================================
// Set address of single default device attached
//==================================================================
void SetAddr(int new_addr)
{
    unsigned char SendBytes[] = {new_addr,0x42, 0x00, 0x00};
    unsigned char rbytes[MAX_RBYTES];

    int n_rbytes = SendAndGetResponse(SendBytes, sizeof(SendBytes),rbytes);
}


//==================================================================
// Reset watt hour counter
//==================================================================
void ResetEnergy(int addr)
{
    unsigned char SendBytes[] = {addr,0x42, 0x00, 0x00};
    unsigned char rbytes[MAX_RBYTES];

    int n_rbytes = SendAndGetResponse(SendBytes, sizeof(SendBytes), rbytes);
}

#ifdef SERIAL_RX_INTERRUPTS
static volatile int ch;
static volatile int num_rx;

// RX interrupt handler -- tried that one out.
static void on_uart_rx()
{
    while (uart_is_readable(UART_ID)) {
        uint8_t ch = uart_get_hw(UART_ID)->dr;
        num_rx += 1;
    }
}
#endif

// More info on uart communication, see:
//pico/pico-sdk/src/rp2_common/hardware_uart/include/hardware/uart.h

static int OpenPzemSerial()
{
    // Set up our UART with the required speed.
    uart_init(UART_ID, BAUD_RATE);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);


    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;
#ifdef SERIAL_RX_INTERRUPTS
    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);
#endif
}

//==================================================================
// Initialize module
//==================================================================
int PzemInit(void)
{
    OpenPzemSerial();
    PzemFields_t data;
    data = ReadAll(0xf8);

    if (data.Voltage){
        printf("Pzem Volt=%6.1f\n",data.Voltage);
        return true;
    }else{
        printf("No pzem found\n");
        return false;
    }
}

//==================================================================
// Add pzem report
//==================================================================
void PzemReport(void * arg)
{
    PzemFields_t data = ReadAll(0xf8);
    char ReportStr[60];
    sprintf(ReportStr,"V=%5.1f,W=%6.1f, %4.2f%%, wh=%d\n", data.Voltage, data.Power, data.PowerFactor, data.Energy);
    SendResponse(arg, ReportStr, strlen(ReportStr));
}
