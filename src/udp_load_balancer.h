#ifndef _DPLB_UDP_LOAD_BALANCER_
#define _DPLB_UDP_LOAD_BALANCER_

#define udp_load_balancer_version      1000000
#define UDP_LOAD_BALANCER_VERSION      "1.0.0"

#define MAX_PAYLOAD_LENGTH 65507  // 65535 bytes − 8 byte UDP header − 20 byte IP header
#define MAX_SERVERS 1024

pthread_mutex_t resolv_mutex;

int verb_flag;
int version_flag;
int debug_flag;

char **servers_list_resolved;
int *servers_ports_resolved;


typedef struct {
    char **servers_list;
    int socket_;
} dispatcher_arg_struct_t;

#endif /* _DPLB_UDP_LOAD_BALANCER_ */
