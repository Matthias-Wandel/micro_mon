// My tcp server, based on pico example
#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#define DEBUG_printf printf
#define BUF_SIZE 1024
#define POLL_TIME 1 // Number of half seconds "polling"


#include "sensor_remote.h"

static const char Wifi_ssid[] = "82 starwood";
static const char Wifi_password[] = "6132266151";
#define TCP_PORT 80

#define STATIC_IP 1

#ifdef STATIC_IP
static const uint8_t addr_use[] = {192,168,0,31};
#endif

typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb; // A whole lot of internal state variables of lwip in there!
} TCP_SERVER_T;

static TCP_SERVER_T state; // Only one instance of tcp server, but multiple instances of connection.


typedef struct { // Tcp connection instance.
    struct tcp_pcb *client_pcb; // A whole lot of internal state variables of lwip in there!

    uint8_t SendBuffer[BUF_SIZE];
    int n_to_send;
    int n_sent;
    bool finished_send_queuing; // Everything that needs sending is queued.

    uint8_t RecvBuffer[BUF_SIZE];
    int n_received;
    bool got_request;    // Wheter we processed the HTTP request already.
} TCP_CONNECTION_T;



//====================================================================================
// Close and deallocate the TCP connection.
//====================================================================================
static err_t tcp_connection_close(TCP_CONNECTION_T * Conn) {
    printf("tcp_connection_close()\n");
    err_t err = ERR_OK;

    tcp_arg(Conn->client_pcb, NULL);
    tcp_poll(Conn->client_pcb, NULL, 0);
    tcp_sent(Conn->client_pcb, NULL);
    tcp_recv(Conn->client_pcb, NULL);
    tcp_err(Conn->client_pcb, NULL);
    err = tcp_close(Conn->client_pcb);
    if (err != ERR_OK) {
        DEBUG_printf("close failed %d\n", err);
        tcp_abort(Conn->client_pcb);
        err = ERR_ABRT;
    }

    tcp_close(Conn->client_pcb);
    Conn->client_pcb = NULL;
    free(Conn);

    return err;
}
//====================================================================================
// Callback -- data has been sent.
//====================================================================================
static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    TCP_CONNECTION_T *Conn = (TCP_CONNECTION_T *)arg;

    DEBUG_printf("tcp_server_sent(arg=%x) %u, total %d of %d\n",(int)arg, len, Conn->n_sent, Conn->n_to_send);
    Conn->n_sent += len;

    if (Conn->n_sent >= Conn->n_to_send) {
        //printf("Everything of %d sent\n", Conn->n_to_send);
        if (Conn->finished_send_queuing){
            printf("sending all done, close\n");
            return  tcp_connection_close(Conn);
        }
    }else{
        // More to send.
        printf("More stuff to send\n");
        return tcp_write(Conn->client_pcb, Conn->SendBuffer+Conn->n_sent, Conn->n_to_send-Conn->n_sent, TCP_WRITE_FLAG_COPY);
    }
    return ERR_OK;
}
//====================================================================================
// Called to send data, must be called from a callback.
//====================================================================================
static int tcp_server_send_data(void * arg, const uint8_t * Response, int len)
{
    TCP_CONNECTION_T *Conn = (TCP_CONNECTION_T *)arg;
    printf("tcp_server_send_data(len=%d)\n",len);
    cyw43_arch_lwip_check();


    // Send http header (that part we already know)
    static const char HttpStart[] = "HTTP/1.0 200 OK\r\n\r\n\r\n";
    err_t err = tcp_write(Conn->client_pcb, HttpStart, sizeof(HttpStart)-1, TCP_WRITE_FLAG_MORE);
    if (err != ERR_OK) DEBUG_printf("Write error %d\n", err);

    err = tcp_write(Conn->client_pcb, Response, len, TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE);
    if (err != ERR_OK) {
        DEBUG_printf("Write error %d\n", err);
    }
    return err;
}
//====================================================================================
// Callback
//====================================================================================
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    TCP_CONNECTION_T *Conn = (TCP_CONNECTION_T*)arg;
    printf("tcp_server_recv( arg=%x)\n",(int)arg);
    if (!p) {
        printf("Remote closed connection\n");
        return ERR_OK;
    }
    cyw43_arch_lwip_check();

    int got_was = Conn->n_received;
    if (p->tot_len > 0) {
        printf("tcp_server_recv %d/%d err %d\n", p->tot_len, Conn->n_received, err);

        // Receive the buffer
        const uint16_t buffer_left = BUF_SIZE - Conn->n_received;
        Conn->n_received += pbuf_copy_partial(p, Conn->RecvBuffer + Conn->n_received,
                                             p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);

    //printf("recv_len = %d\n",Conn->n_received); // Check for request.
    //printf("Got: %s\n",Conn->RecvBuffer);

    if (!Conn->got_request){
        for (int a=0;a<Conn->n_received;a++){
            if (Conn->RecvBuffer[a] == '\n'){
                printf("Request line: %.*s\n", a, Conn->RecvBuffer);
                if (memcmp(Conn->RecvBuffer, "GET ", 4) == 0){
                    for (int b=4;b<Conn->n_received;b++){
                        if (Conn->RecvBuffer[b] == ' '){ // Get rid of bits trailing the URL.
                            Conn->RecvBuffer[b] = '\0';
                            break;
                        }
                    }
                    QueueRequest(arg, Conn->RecvBuffer+4);
                }
                Conn->got_request = 1;

                break;
            }
        }
    }

    return ERR_OK;
}
//====================================================================================
// Callback
//====================================================================================
static void tcp_server_err(void * arg, err_t err)
{
    printf("tcp_server_err() arg=%x, err=%d\n",(int)arg, err);
    if (err == ERR_RST){
        printf("Remote closed connection\n");
        // But which connection?  Pointer to tcp server or connection?
        err_t err = tcp_close(arg);
        if (err != ERR_OK) DEBUG_printf("Close error %d\n", err);
    }else if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err_fn %d\n", err);
    }
}

static err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb);
//====================================================================================
// Callback
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

    TCP_CONNECTION_T *Conn = calloc(1, sizeof(TCP_CONNECTION_T));
    if (!Conn){
        printf("Failed to allocate for connection\n");
        tcp_close(arg);
    }
    printf("Allocated TCP connection Conn = %x\n",(int)Conn);

    Conn->client_pcb = client_pcb;
    tcp_arg(client_pcb, Conn);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME);
    tcp_err(client_pcb, tcp_server_err);

    return 0;
}
//====================================================================================
static bool tcp_server_open()
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

    state.server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state.server_pcb) {
        DEBUG_printf("failed to listen\n");
        tcp_close(pcb);
        return false;
    }

    tcp_arg(state.server_pcb, NULL);
    tcp_accept(state.server_pcb, tcp_server_accept);

    return true;
}

//====================================================================================
// Callback -- poll for more data to send from application.
//====================================================================================
static err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb)
{
    TCP_CONNECTION_T *Conn = (TCP_CONNECTION_T *)arg;
    //DEBUG_printf("tcp_server_poll_fn\n");

    int n_unsent = Conn->n_to_send - Conn->n_sent;
    if (n_unsent > 0){
        u8_t flags = 0;
        if (!Conn->finished_send_queuing) flags |= TCP_WRITE_FLAG_MORE;
        tcp_write(tpcb, Conn->SendBuffer+Conn->n_sent, n_unsent, flags);
    }

    return 0;
}

//====================================================================================
// Put it in the output queue to then put in TCP stack when polled by
// tcp_server_poll which is invoked by a callback.
//====================================================================================
void TCP_EnqueueForSending(void * arg, void * Data, int NumBytes, bool Final)
{
    TCP_CONNECTION_T *Conn = (TCP_CONNECTION_T *)arg;

    if (Conn->n_to_send+NumBytes > BUF_SIZE){
        printf("Too many bytes to send");
        return;
    }

    memcpy(Conn->SendBuffer+Conn->n_to_send, Data, NumBytes);
    Conn->n_to_send += NumBytes;
    Conn->finished_send_queuing = Final;
}


//====================================================================================
// Set up TCP server
//====================================================================================
int tcp_server_setup()
{
    cyw43_arch_enable_sta_mode();

    printf("TCP Server main() Connecting to WiFi...\n");

    if (cyw43_arch_wifi_connect_timeout_ms(Wifi_ssid, Wifi_password, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("wifi connect fail\n");
        return 1;
    }

#ifdef STATIC_IP
    // Ignore address we got from DHCP and use static one instead.
    netif_set_ipaddr(netif_list, (const ip4_addr_t *) addr_use);
    netif_set_hostname(netif_list,"mypico");
#endif

    if (!tcp_server_open(&state)) {
        return -1;
    }

    return 0;
}

//====================================================================================
// It seems my router forgets that we are there after 36 hour.
// Just calling netif_set_ipaddr sends something out to clear that.
// Doesn't disturb TCP connections that may be open
//====================================================================================
void tcp_server_refresh_addr(void)
{
    printf("refresh ip addr\n");
    netif_set_ipaddr(netif_list, (const ip4_addr_t *) addr_use);
}


