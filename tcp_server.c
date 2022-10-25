// My tcp server, based on pico example
#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#define TCP_PORT 80
#define DEBUG_printf printf
#define BUF_SIZE 2048
#define TEST_ITERATIONS 10
#define POLL_TIME_S 5

typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb; // Just has the4 IP address
    struct tcp_pcb *client_pcb; // Just has the4 IP address
    uint8_t buffer_sent[BUF_SIZE];
    uint8_t buffer_recv[BUF_SIZE];
    int sent_len;     // How much sent so far
    int to_send_len;  // How much to actually send.
    int recv_len;
} TCP_SERVER_T;

/*
typedef struct {
	TCP_SERVER_T * Server
    uint8_t SentBytes[BUF_SIZE];
	int to_send_bytes;
	int num_bytes_sent;
	
    uint8_t RecvBytes[BUF_SIZE];
    int num_bytes_received;	
} TCP_Connection;

TCP_SERVER_T state; // Only one instance of tcp server, but multiple instances of connection.
*/

//====================================================================================
static TCP_SERVER_T* tcp_server_init(void) {
    TCP_SERVER_T *state = calloc(1, sizeof(TCP_SERVER_T));
	printf("tcp_server_init()\n");
    if (!state) {
        DEBUG_printf("failed to allocate state\n");
        return NULL;
    }
    return state;
}
//====================================================================================
static err_t tcp_server_close(void *arg) {
    printf("tcp_server_close()\n");
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    err_t err = ERR_OK;
    if (state->client_pcb != NULL) {
        tcp_arg(state->client_pcb, NULL);
        tcp_poll(state->client_pcb, NULL, 0);
        tcp_sent(state->client_pcb, NULL);
        tcp_recv(state->client_pcb, NULL);
        tcp_err(state->client_pcb, NULL);
        err = tcp_close(state->client_pcb);
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(state->client_pcb);
            err = ERR_ABRT;
        }
        state->client_pcb = NULL;
    }
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
    return err;
}
//====================================================================================
static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    state->sent_len += len;
    DEBUG_printf("tcp_server_sent() %u, total %d of %d\n", len, state->sent_len, state->to_send_len);

    if (state->sent_len >= state->to_send_len) {
        printf("Finished sending, close tcp\n");
        return tcp_close(arg);
		state->recv_len = 0;
    }

    return ERR_OK;
}
//====================================================================================
err_t tcp_server_send_data(void *arg, struct tcp_pcb *tpcb)
{
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
	printf("tcp_server_send_data()\n");
    for(int i=0; i< BUF_SIZE; i++) {
        state->buffer_sent[i] = 'x';
    }
    strcpy(state->buffer_sent, "HTTP/1.0 200 OK\r\nContent-Length: 31\r\n\r\n<html><b>Hello world</b></html>1234567890");
    int n_send = strlen(state->buffer_sent);
    state->sent_len = 0;
    state->to_send_len = n_send;
    printf("Writing %ld bytes to client\n", n_send);
    
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    err_t err = tcp_write(tpcb, state->buffer_sent, n_send, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        DEBUG_printf("Write error %d\n", err);
    }
    return err;
}
//====================================================================================
err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
	printf("tcp_server_recv()\n");
    if (!p) {
        return -1;
    }
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    int got_was = state->recv_len;
    if (p->tot_len > 0) {
        printf("tcp_server_recv %d/%d err %d\n", p->tot_len, state->recv_len, err);

        // Receive the buffer
        const uint16_t buffer_left = BUF_SIZE - state->recv_len;
        state->recv_len += pbuf_copy_partial(p, state->buffer_recv + state->recv_len,
                                             p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);
    
    printf("recv_len = %d\n",state->recv_len); // Check for request.
    printf("Got: %s\n",state->buffer_recv);
    if (got_was == 0){
        if (memcmp("GET /", state->buffer_recv, 5) == 0){
            return tcp_server_send_data(arg, state->client_pcb);
        }
    }

    return ERR_OK;
}
//====================================================================================
static err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb) {
    DEBUG_printf("tcp_server_poll_fn\n");
    return -1; // no response is an error?
}
//====================================================================================
static void tcp_server_err(void * arg, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (err == ERR_RST){
        printf("Remote closed connection\n");
		err_t err = tcp_close(arg);
		state->recv_len = 0;
        if (err != ERR_OK) DEBUG_printf("Close error %d\n", err);
    }else if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err_fn %d\n", err);
    }
}
//====================================================================================
static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) 
{
	printf("tcp_server_accept()\n");
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("Failure in accept\n");
        return ERR_VAL;
    }
    DEBUG_printf("Client connected\n");

    state->client_pcb = client_pcb;
    tcp_arg(client_pcb, state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2);
    tcp_err(client_pcb, tcp_server_err);

    return 0;
}
//====================================================================================
static bool tcp_server_open(TCP_SERVER_T *state)
{
    DEBUG_printf("tcp_server_open() at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        DEBUG_printf("failed to create pcb\n");
        return false;
    } 

    err_t err = tcp_bind(pcb, NULL, TCP_PORT);
    if (err) {
        DEBUG_printf("failed to bind to port %d\n");
        return false;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb) {
        DEBUG_printf("failed to listen\n");
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);

    return true;
}
//====================================================================================
int tcp_server_main()
{
    cyw43_arch_enable_sta_mode();

    printf("TCP Server main() Connecting to WiFi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms("82 starwood", "6132266151", CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("wifi connect fail\n");
        return 1;
    } else {
        printf("Connected.\n");
    }
    
    //netif_set_ipaddr(struct netif *netif, const ip4_addr_t *ipaddr) // Set static IP address, figure out how.

    TCP_SERVER_T *state = tcp_server_init();
    
    if (!state) return -1;
    
    if (!tcp_server_open(state)) {
        return -1;
    }
    
    while (1){
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer) to check for WiFi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        sleep_ms(1);
    }
    free(state);

    return 0;
}

/* Some error values codes
ERR_OK         = 0,
ERR_MEM        = -1,
ERR_BUF        = -2,
ERR_TIMEOUT    = -3,
ERR_RTE        = -4,
ERR_INPROGRESS = -5,
ERR_VAL        = -6,
ERR_WOULDBLOCK = -7,
ERR_USE        = -8,
ERR_ALREADY    = -9,
ERR_ISCONN     = -10,
ERR_CONN       = -11,
ERR_IF         = -12,
ERR_ABRT       = -13,
ERR_RST        = -14,
ERR_CLSD       = -15,
ERR_ARG        = -16
*/