#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "udp_load_balancer.h"


unsigned int is_help_required_or_flags_set(int argc, char **argv) {
    unsigned int i;
    for (i = 0; i < argc; ++i)
    {
        if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            return 1;
        } else if ( strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verb_flag = 1;
        } else if ( strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            verb_flag = 1;
            debug_flag = 1;
        }
    }

    return 0;
}

unsigned int get_udp_load_balancer_port(int argc, char **argv) {
    char *port_as_string;
    unsigned int port;

    unsigned int i;
    for (i = 0; i < argc; ++i)
    {
        if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            port_as_string = argv[i+1];
            port = (unsigned int) strtol(port_as_string, (char **)NULL, 10);

            if (port < 1 || port > 65535) {
                return 0;
            }

            return port;
        }
    }

    return 0;
}

char *get_udp_load_balancer_host(int argc, char **argv) {
    char *host;

    unsigned int i;
    for (i = 0; i < argc; ++i)
    {
        if(strcmp(argv[i], "--host") == 0) {
            host = argv[i+1];

            return host;
        }
    }

    return "localhost";
}

char **get_servers_list(int argc, char **argv) {
    char **servers_list = NULL;
    char *servers_list_raw = NULL;
    char *token;
    unsigned int i;
    for (i = 0; i < argc; ++i)
    {
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--servers") == 0) {
            servers_list_raw = argv[i+1];
            i++;
        }
    }

    if (servers_list_raw == NULL) {
        return NULL;
    }

    i = 0;
    token = strtok(servers_list_raw, " , ");
    while (token) {
        servers_list = realloc (servers_list, sizeof (char*) * ++i);
        servers_list[i-1] = token;
        token = strtok (NULL, " , ");
    }

    /* realloc one extra element for the last NULL */
    servers_list = realloc(servers_list, sizeof (char*) * (i+1));
    servers_list[i] = 0;

    return servers_list;
}

char **get_servers_hosts(char **servers_list, int servers_amount) {
    char *server_raw, *server_host, serve;
    char **servers_hosts = malloc(MAX_SERVERS * sizeof(char*));

    unsigned int i;
    for(i = 0; i < servers_amount; i++) {
        server_raw = strdup(servers_list[i]);
        server_host = strsep(&server_raw, ":");
        servers_hosts[i] = server_host;
    }

    return servers_hosts;
}

void free_servers_hosts(char **servers_hosts, int servers_amount) {
    unsigned int i;
    for(i = 0; i < servers_amount; i++) {
        free(servers_hosts[i]);
    }
    free(servers_hosts);
}

int *get_servers_ports(char **servers_list, int servers_amount) {
    char *server_raw, *free_pointer, *port_string;
    int server_port;
    int *servers_ports = malloc(MAX_SERVERS * sizeof(int*));

    unsigned int i, j;
    for(i = 0; i < servers_amount; i++) {
        free_pointer = strdup(servers_list[i]);
        server_raw = free_pointer;
        strsep(&server_raw, ":");
        port_string = strsep(&server_raw, ":");

        server_port = 0;
        for(j=0; j < strlen(port_string); j++){
		    server_port = server_port * 10 + ( port_string[j] - '0' );
	    }

        servers_ports[i] = server_port;
        free(free_pointer);
    }

    return servers_ports;
}

void free_servers_ports(int *servers_ports, int servers_amount) {
    free(servers_ports);
}
