extern int adc_main(void);
extern int wifi_scan(void);
extern int tcp_server_setup(void);
extern void tcp_finished_sending(void * arg);
extern int QueueRequest(void * arg, char * Url);
void TCP_EnqueueForSending(void * arg, void * Data, int NumBytes, bool Final);

#ifdef __cplusplus
extern "C" int ds18b20_read_sesnors(void * arg);
extern "C" void tcp_sleep_ms(int ms);
extern "C" void SendResponse(void * arg, char * ResponseStr, int n);
#else
extern int ds18b20_read_sesnors(void * arg);
extern void tcp_sleep_ms(int ms);
extern void SendResponse(void * arg, char * ResponseStr, int n);
#endif