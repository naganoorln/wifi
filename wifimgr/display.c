#include <stdio.h>
#include "wifi.h"

void header()
{
    printf("------------------------------------------------------------------------------------------------------------\n");
    printf("| %-3s | %-30s | %-17s | %-8s | %-8s | %-8s | %-12s |\n",
           "No", "SSID", "BSSID", "Signal", "Channel", "Freq", "Security");
    printf("------------------------------------------------------------------------------------------------------------\n");
}

void row(int index, wifi_bss *b)
{
    printf("| %-3d | %-30s | %02x:%02x:%02x:%02x:%02x:%02x | %-8.1f | %-8d | %-8d | %-12s |\n",
           index,
           b->ssid_len ? b->ssid : "<hidden>",
           b->bssid[0], b->bssid[1], b->bssid[2],
           b->bssid[3], b->bssid[4], b->bssid[5],
           b->signal,
           b->channel,
           b->freq,
           b->security);
}

void help()
{
    printf("\nAvailable commands:\n");
    printf(" scan              - Scan nearby WiFi networks\n");
    printf(" connect <num>     - Connect to network number\n");
    printf(" disconnect        - Disconnect current network\n");
    printf(" clear             - Clear screen\n");
    printf(" help              - Show this help\n");
    printf(" exit              - Exit program\n");
}
