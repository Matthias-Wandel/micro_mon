extern int adc_main(void);
extern int wifi_scan(void);
extern int tcp_server_setup(void);
extern void tcp_server_refresh_addr(void);
extern void tcp_finished_sending(void * arg);
extern int QueueRequest(void * arg, char * Url);
void TCP_EnqueueForSending(void * arg, void * Data, int NumBytes, bool Final);
void core1_entry();
extern void GetAnemometerFrequency(void * arg);

#ifdef __cplusplus
extern "C" void ds18b20_read_sesnors(void * arg);
extern "C" void tcp_sleep_ms(int ms);
extern "C" void SendResponse(void * arg, char * ResponseStr, int n);
extern "C" void my_sleep_ms(int ms);
#else
extern void ds18b20_read_sesnors(void * arg);
extern void tcp_sleep_ms(int ms);
extern void SendResponse(void * arg, char * ResponseStr, int n);
extern void my_sleep_ms(int ms);
#endif

int PzemInit(void);
typedef struct {
    float Voltage;
    float Current;
    float Power;
    unsigned long Energy;
    float Freq;
    float PowerFactor;
}PzemFields_t;
PzemFields_t ReadAll(int addr);
void PzemReport(void * arg);
