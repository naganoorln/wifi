#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "wifi_ctrl.h"
#include "display.h"

int main()
{
    wifi_network networks[MAX_NETWORKS];
    char input[256];

    printf("=== WiFi CLI ===\nType 'help' for commands\n");

    while (1)
    {
        printf("\nWiFi> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "help") == 0) print_help();
	else if (strcmp(input, "clear") == 0 || strcmp(input, "clr") == 0 || input[0] == 0x0c) {
            printf("\033[2J\033[H");
        }
        else if (strcmp(input, "scan") == 0) {
            int n = scan_networks(networks);
            if (n == 0) { printf("No networks found.\n"); continue; }
            displayNetworks(networks, n);
        }
        else if (strcmp(input, "connect") == 0) {
            int n = scan_networks(networks);
            if (n == 0) { printf("No networks found.\n"); continue; }

            printf("Select network number: ");
            int choice;
            if (scanf("%d", &choice) != 1 || choice < 1 || choice > n) {
                printf("Invalid selection.\n");
                while (getchar() != '\n');
                continue;
            }
            while (getchar() != '\n');

            char pass[128];
            printf("Enter password for '%s': ", networks[choice - 1].ssid);
            if (!fgets(pass, sizeof(pass), stdin)) continue;
            pass[strcspn(pass, "\n")] = 0;

            connect_wifi(networks[choice - 1].ssid, pass);
            memset(pass, 0, sizeof(pass));
        }
        else if (strcmp(input, "disconnect") == 0) disconnect_wifi();
        else if (strcmp(input, "exit") == 0) break;
        else printf("Unknown command.\n");
    }

    return 0;
}
