#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "udp_load_balancer.h"
#include "argparse.h"
#include "socket.h"
#include "hostname_resolver.h"

#include <pthread.h> 
#include <unistd.h>
#include <netdb.h>

unsigned int servers_amount_resolved = 0;

unsigned int get_servers_amount(char **servers_list) {
    unsigned int i = 0;

    while(1) {
        if(servers_list[i] == NULL) {
            return i;
        }
        i++;
    }
}

void* run_resolver( void* call_arg )
{
    char **servers_list = call_arg; 
    char **servers_list_resolved_tmp;
    char **servers_list_resolved_free;
    int *servers_ports_resolved_tmp;
    int *servers_ports_resolved_free;
    servers_list_resolved = NULL;
    servers_ports_resolved = NULL;
    int first_run = 1;
    unsigned int servers_amount_resolved_prev;
    unsigned int servers_amount = get_servers_amount(servers_list);
    char **servers_hosts = get_servers_hosts(servers_list, servers_amount);
    int *servers_ports = get_servers_ports(servers_list, servers_amount);

    while(1) {
        servers_amount_resolved_prev = servers_amount_resolved;
        servers_list_resolved_tmp = calloc(sizeof (char*), servers_amount); 
        servers_ports_resolved_tmp = calloc(sizeof (int), servers_amount); 
        if ( verb_flag ) printf("\n\nBalancing to servers:\n");
        unsigned int i;
        unsigned int j = 0;
        for (i = 0; i < servers_amount; ++i) {
            if ( verb_flag ) printf("  %s\n", servers_list[i]);
            if (is_ip(servers_hosts[i]) == 0) {

                struct hostent *he = gethostbyname(servers_hosts[i]);

                if (he == NULL) {
                    printf("Host \"%s\" not found\n", servers_hosts[i]);
                    continue;
                }

                struct in_addr **addr_list = (struct in_addr **)he->h_addr_list;

                while ( *addr_list ) {
                    if ( j >= servers_amount ) {
                       servers_list_resolved_tmp = realloc(servers_list_resolved_tmp, sizeof (char*) * (j+1));
                       servers_ports_resolved_tmp = realloc(servers_ports_resolved_tmp, sizeof (int) * (j+1));
                    }
                    servers_list_resolved_tmp[j] = strdup(inet_ntoa(**addr_list));
                    servers_ports_resolved_tmp[j] = servers_ports[i];
                    if ( verb_flag ) printf("    %s %d\n",servers_list_resolved_tmp[j], servers_ports_resolved_tmp[j]);
                    j++;
                    addr_list += 1;
                }
            } else {
              if ( j >= servers_amount ) {
                 servers_list_resolved_tmp = realloc(servers_list_resolved_tmp, sizeof (char*) * (j+1));
                 servers_ports_resolved_tmp = realloc(servers_ports_resolved_tmp, sizeof (int) * (j+1));
              }
              servers_list_resolved_tmp[j] = strdup(servers_hosts[i]);
              servers_ports_resolved_tmp[j] = servers_ports[i];
              j++;
            }
            servers_amount_resolved = j;
        }
        if ( j < 1 ) {
             printf( "Can't resolve any of servers, waiting for 5sec\n" );
             sleep(5);
             continue ;
        }
        if ( first_run == 0 ) {
           pthread_mutex_lock(&resolv_mutex);
        }
        first_run = 0;
        servers_list_resolved_free = servers_list_resolved;
        servers_list_resolved = servers_list_resolved_tmp;
        servers_ports_resolved_free = servers_ports_resolved;
        servers_ports_resolved = servers_ports_resolved_tmp;
        pthread_mutex_unlock(&resolv_mutex);
        if ( debug_flag ) {
          printf("servers_list_resolved array:\n");
          for ( j = 0 ; j < servers_amount_resolved ; j++ ) {
               printf(" (%u %s:%d)", j, servers_list_resolved_tmp[j], servers_ports_resolved_tmp[j]);
               printf("\n");
          }
        }
        free_servers_hosts(servers_list_resolved_free,servers_amount_resolved_prev);
        free(servers_ports_resolved_free);
    
        usleep(5000000); // resolve every 5sec
    }
    free_servers_hosts(servers_hosts,servers_amount);
    free_servers_ports(servers_ports,servers_amount);
}

void* run_dispatcher( void* call_arg )
{
    dispatcher_arg_struct_t *arg = call_arg; 
    char message[MAX_PAYLOAD_LENGTH];
    unsigned int message_length;
    char srv[16];
    int port;

    unsigned int i;

    i = 0;
    while(1) {
        if ( debug_flag ) printf("while waiting for a resolv_mutex\n");
        listen_udp_packet(message, &message_length, arg->socket_);
        if ( debug_flag ) printf("message:%s ", message );
        pthread_mutex_lock(&resolv_mutex);
        /* If number of servers' IPs has been decreased by resolver thread - begin from first IP */
        if ( i >= servers_amount_resolved ) {
             i = 0;
        }
        /* use srv & port temp variables for call send_udp_packet() outside of pthread_mutex_unlock() */
        strcpy(srv,servers_list_resolved[i]);
        port = servers_ports_resolved[i];
        pthread_mutex_unlock(&resolv_mutex);
        if ( debug_flag ) printf("%s\n", srv );
        send_udp_packet(srv, port, message, message_length, arg->socket_);
        i++;
    }
}

void show_version() {
    printf("udp_load_balancer %s\n",UDP_LOAD_BALANCER_VERSION);
}

void show_help() {
    printf("\n"
        "Usage: udp_load_balancer [-hvd] [--port PORT] [--servers SERVERS]\n"
        "\n"
        "The options are as follows:\n"
        "      -h, --help            show this help message and exit\n"
        "      -p, --port PORT\n"
        "                            Port to listen UDP messages (min 1, max 65535)\n"
        "      --host HOST or IP\n"
        "                            Bind the following Host or IP address to listen UDP messages (default: localhost)\n"
        "      -s, --servers SERVERS\n"
        "                            Servers list to balance the UDP messages\n"
        "                            Example: \"127.0.0.1:8123, localhost:8124, example.com:8123\"\n"
        "      -v, --verbose         Be verbose\n"
        "      -V, --version         Show version\n"
        "      -d, --debug           Debug output enabled\n"
    );
}

int main(int argc, char **argv)
{
    verb_flag = 0;
    version_flag = 0;
    debug_flag = 0;
    if (is_help_required_or_flags_set(argc, argv)) {
        show_help();
        exit(0);
    }

    if ( version_flag ) {
        show_version();
        exit(0);
    }

    unsigned int udp_load_balancer_port = get_udp_load_balancer_port(argc, argv);
    char **servers_list  = get_servers_list(argc, argv);

    if (udp_load_balancer_port == 0 || servers_list == NULL) {
        show_help();
        exit(2);
    }

    char *udp_load_balancer_ip = get_udp_load_balancer_host(argc, argv);

    if ( verb_flag )printf("Listening on %s:%d\n", udp_load_balancer_ip, udp_load_balancer_port);
    if(is_ip(udp_load_balancer_ip) == 0) {
        udp_load_balancer_ip = get_one_ip_from_hostname(udp_load_balancer_ip);
        if ( verb_flag ) printf(" (%s:%d)\n", udp_load_balancer_ip, udp_load_balancer_port);
    }

    pthread_mutex_init (&resolv_mutex , NULL);
    pthread_mutex_lock(&resolv_mutex); // initially lock resolv_mutex - unlock when resolv will be done

    int socket_ = create_socket(udp_load_balancer_ip, udp_load_balancer_port);

    pthread_t dispatcher_thread_id; 
    pthread_t resolver_thread_id; 
    dispatcher_arg_struct_t dispatcher_arg_struct = { servers_list , socket_ };
    pthread_create(&resolver_thread_id, NULL, run_resolver, (void *) servers_list); 
    pthread_create(&dispatcher_thread_id, NULL, run_dispatcher, (void *) &dispatcher_arg_struct); 
    pthread_join(dispatcher_thread_id, NULL); 
    if ( verb_flag ) printf("main exit\n");

    return 0;
}
