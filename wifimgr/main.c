#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <net/if.h>
#include <sys/socket.h>
#include "wifi.h"

int nl_sock;
int nl80211_id;
int ifindex;
int network_count = 0;
wifi_bss networks[MAX_NETWORKS];

int main()
{
    const char *iface = "wlo1";

    ifindex = if_nametoindex(iface);
    if (!ifindex) {
        printf("Invalid interface\n");
        return -1;
    }

    nl_sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
    if (nl_sock < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_nl local = {0};
    local.nl_family = AF_NETLINK;
    local.nl_pid = getpid();
    bind(nl_sock, (struct sockaddr *)&local, sizeof(local));

    nl80211_id = resolve_nl80211();
    if (nl80211_id < 0) {
        printf("Failed to resolve nl80211\n");
        return -1;
    }

    char command[128];

    while (1) {
        printf("\nWiFi> ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0;

        if (strcmp(command, "scan") == 0)
            scan_wifi();
        else if (strncmp(command, "connect", 7) == 0)
            connect_wifi(atoi(command + 8));
        else if (strcmp(command, "disconnect") == 0)
            disconnect_wifi();
        else if (strcmp(command, "clear") == 0)
            system("clear");
        else if (strcmp(command, "help") == 0)
            help();
        else if (strcmp(command, "exit") == 0)
            break;
        else
            printf("Unknown command. Type 'help'\n");
    }

    close(nl_sock);
    return 0;
}
