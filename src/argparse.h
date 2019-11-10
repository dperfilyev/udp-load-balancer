unsigned int is_help_required_or_flags_set(int argc, char **argv);

unsigned int get_udp_load_balancer_port(int argc, char **argv);

char *get_udp_load_balancer_host(int argc, char **argv);

char **get_servers_list(int argc, char **argv);

char **get_servers_hosts(char **servers_list, int servers_amount);

void free_servers_hosts(char **servers_hosts, int servers_amount);

int *get_servers_ports(char **servers_list, int servers_amount);

void free_servers_ports(int *servers_ports, int servers_amount);
