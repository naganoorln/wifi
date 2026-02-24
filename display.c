#include "display.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void print_help()
{
    printf("\nCommands:\n");
    printf("  scan        - Scan WiFi networks\n");
    printf("  connect     - Connect to network\n");
    printf("  disconnect  - Disconnect WiFi\n");
    printf("  help        - Show commands\n");
    printf("  exit        - Quit\n");
}

void displayNetworks(wifi_network networks[], int n)
{
    int ssid_width = 4;
    int flags_width = 5;

    for (int i = 0; i < n; i++) {
        if (strlen(networks[i].ssid) > ssid_width) ssid_width = strlen(networks[i].ssid);
        if (strlen(networks[i].flags) > flags_width) flags_width = strlen(networks[i].flags);
    }

    int signal_width = 10;
    int table_width = 3 + 3 + ssid_width + 3 + signal_width + 3 + flags_width + 3;

    for (int i = 0; i < table_width; i++) printf("=");
    printf("\n");

    printf("| %-3s | %-*s | %-*s | %-*s |\n",
           "No", ssid_width, "SSID", signal_width, "Signal", flags_width, "Flags");

    for (int i = 0; i < table_width; i++) printf("-");
    printf("\n");

    int min_sig = 1000, max_sig = -1000;
    for (int i = 0; i < n; i++) {
        int sig = atoi(networks[i].signal);
        if (sig > max_sig) max_sig = sig;
        if (sig < min_sig) min_sig = sig;
    }

    for (int i = 0; i < n; i++) {
        int sig = atoi(networks[i].signal);
        int bars = 1 + ((sig - min_sig) * 5) / (max_sig - min_sig);
        if (bars < 1) bars = 1;
        if (bars > 5) bars = 5;

        char signal_bar[6] = "     ";
        for (int j = 0; j < bars; j++) signal_bar[j] = '#';

        char *color;
        if (sig >= -60) color = GREEN;
        else if (sig >= -75) color = YELLOW;
        else color = RED;

        char flags_trunc[flags_width + 1];
        if ((int)strlen(networks[i].flags) > flags_width) {
            strncpy(flags_trunc, networks[i].flags, flags_width - 3);
            flags_trunc[flags_width - 3] = '\0';
            strcat(flags_trunc, "...");
        } else strcpy(flags_trunc, networks[i].flags);

        printf("| %-3d | %-*s | %-3s %s%s%s | %-*s |\n",
               i + 1, ssid_width, networks[i].ssid,
               networks[i].signal, color, signal_bar, RESET,
               flags_width, flags_trunc);
    }

    for (int i = 0; i < table_width; i++) printf("=");
    printf("\n");
}
