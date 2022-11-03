extern int adc_main(void);
extern int wifi_scan(void);
extern int tcp_server_setup(void);

#ifdef __cplusplus
extern "C" int ds18b20_main(void);
extern "C" int ProcessRequest(void * arg, char * Url);
extern "C" void tcp_sleep_ms(int ms);
#else
extern int ds18b20_main(void);
extern int ProcessRequest(void * arg, char * Url);
extern void tcp_sleep_ms(int ms);
#endif