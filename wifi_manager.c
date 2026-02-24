#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define CTRL_PATH "/run/wpa_supplicant/wlo1"
#define BUF_SIZE 8192
#define MAX_NETWORKS 50

typedef struct {
    char ssid[256];
    char bssid[32];
    char signal[16];
    char flags[128];
} wifi_network;

int send_command(const char *cmd, char *resp, int resp_size)
{
    int sock;
    struct sockaddr_un local, remote;
    int len;

    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    memset(&local, 0, sizeof(local));
    local.sun_family = AF_UNIX;
    snprintf(local.sun_path, sizeof(local.sun_path),
             "/tmp/wpa_ctrl_%d", getpid());
    unlink(local.sun_path);

    if (bind(sock, (struct sockaddr *)&local, sizeof(local)) < 0) {
        perror("bind");
        close(sock);
        return -1;
    }

    memset(&remote, 0, sizeof(remote));
    remote.sun_family = AF_UNIX;
    strncpy(remote.sun_path, CTRL_PATH,
            sizeof(remote.sun_path) - 1);

    if (sendto(sock, cmd, strlen(cmd), 0,
               (struct sockaddr *)&remote,
               sizeof(remote)) < 0) {
        perror("sendto");
        close(sock);
        unlink(local.sun_path);
        return -1;
    }

    len = recv(sock, resp, resp_size - 1, 0);
    if (len > 0)
        resp[len] = '\0';
    else
        resp[0] = '\0';

    close(sock);
    unlink(local.sun_path);
    return len;
}

int wait_for_connection()
{
    char buf[BUF_SIZE];

    for (int i = 0; i < 15; i++)
    {
        send_command("STATUS", buf, sizeof(buf));
        if (strstr(buf, "wpa_state=COMPLETED"))
	{
            return 1;
        }
        sleep(1);
    }
    return 0;
}

int scan_networks(wifi_network *networks)
{
    char buf[BUF_SIZE];
    int count = 0;

    send_command("SCAN", buf, sizeof(buf));
    sleep(2);

    send_command("SCAN_RESULTS", buf, sizeof(buf));

    char *saveptr1;
    char *line = strtok_r(buf, "\n", &saveptr1);

    while (line)
    {
        if (strstr(line, "bssid") == NULL)
	{

            char *saveptr2;
            char *bssid = strtok_r(line, "\t", &saveptr2);
            strtok_r(NULL, "\t", &saveptr2); // freq
            char *signal = strtok_r(NULL, "\t", &saveptr2);
            char *flags  = strtok_r(NULL, "\t", &saveptr2);
            char *ssid   = strtok_r(NULL, "\t", &saveptr2);

            if (ssid && strlen(ssid) > 0 && count < MAX_NETWORKS)
	    {
                strncpy(networks[count].ssid, ssid,
                        sizeof(networks[count].ssid) - 1);
                strncpy(networks[count].bssid, bssid,
                        sizeof(networks[count].bssid) - 1);
                strncpy(networks[count].signal, signal,
                        sizeof(networks[count].signal) - 1);
                strncpy(networks[count].flags, flags,
                        sizeof(networks[count].flags) - 1);
                count++;
            }
        }
        line = strtok_r(NULL, "\n", &saveptr1);
    }

    return count;
}

void connect_wifi(const char *ssid, const char *pass)
{
    char buf[BUF_SIZE], cmd[512];

    send_command("REMOVE_NETWORK all", buf, sizeof(buf));

    send_command("ADD_NETWORK", buf, sizeof(buf));
    int net_id = atoi(buf);

    snprintf(cmd, sizeof(cmd),
             "SET_NETWORK %d ssid \"%s\"", net_id, ssid);
    send_command(cmd, buf, sizeof(buf));

    snprintf(cmd, sizeof(cmd),
             "SET_NETWORK %d psk \"%s\"", net_id, pass);
    send_command(cmd, buf, sizeof(buf));

    snprintf(cmd, sizeof(cmd),
             "ENABLE_NETWORK %d", net_id);
    send_command(cmd, buf, sizeof(buf));

    snprintf(cmd, sizeof(cmd),
             "SELECT_NETWORK %d", net_id);
    send_command(cmd, buf, sizeof(buf));

    printf("Authenticating...\n");

    if (!wait_for_connection())
    {
        printf("Connection failed.\n");
        return;
    }

    printf("Connected to WiFi. Obtaining IP...\n");

    system("dhclient -r wlo1");
    if (system("dhclient wlo1") != 0)
    {
        printf("DHCP failed.\n");
        return;
    }

    system("resolvectl dns wlo1 8.8.8.8 1.1.1.1");
    system("resolvectl domain wlo1 ~.");

    printf("Internet ready.\n");
}

void disconnect_wifi()
{
    char buf[BUF_SIZE];
    send_command("DISCONNECT", buf, sizeof(buf));
    system("dhclient -r wlo1");
    printf("Disconnected.\n");
}

void print_help()
{
    printf("\nCommands:\n");
    printf("  scan        - Scan WiFi networks\n");
    printf("  connect     - Connect to network\n");
    printf("  disconnect  - Disconnect WiFi\n");
    printf("  help        - Show commands\n");
    printf("  exit        - Quit\n");
}

/* ANSI color codes */
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define GREEN   "\033[32m"

void displayNetworks(wifi_network networks[], int n)
{
    int ssid_width = 4;
    int flags_width = 5;

    for (int i = 0; i < n; i++)
    {
        int len_ssid = strlen(networks[i].ssid);
        int len_flags = strlen(networks[i].flags);
        if (len_ssid > ssid_width) ssid_width = len_ssid;
        if (len_flags > flags_width) flags_width = len_flags;
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
    for (int i = 0; i < n; i++)
    {
        int sig = atoi(networks[i].signal);
        if (sig > max_sig) max_sig = sig;
        if (sig < min_sig) min_sig = sig;
    }

    for (int i = 0; i < n; i++)
    {
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
        if ((int)strlen(networks[i].flags) > flags_width)
	{
            strncpy(flags_trunc, networks[i].flags, flags_width - 3);
            flags_trunc[flags_width - 3] = '\0';
            strcat(flags_trunc, "...");
        } else {
            strcpy(flags_trunc, networks[i].flags);
        }

        printf("| %-3d | %-*s | %-3s %s%s%s | %-*s |\n",
               i + 1, ssid_width, networks[i].ssid,
               networks[i].signal, color, signal_bar, RESET,
               flags_width, flags_trunc);
    }

    for (int i = 0; i < table_width; i++) printf("=");
    printf("\n");
}

int main()
{
    wifi_network networks[MAX_NETWORKS];
    char input[256];

    printf("=== WiFi CLI ===\nType 'help' for commands\n");

    while (1)
    {
        printf("\nWiFi> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin))
            break;

        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "help") == 0)
	{

            print_help();

        } else if (strcmp(input, "scan") == 0)
	{

            int n = scan_networks(networks);
            if (n == 0)
	    {
                printf("No networks found.\n");
                continue;
            }

	    displayNetworks(networks, n);

        } else if (strcmp(input, "connect") == 0)
	{

            int n = scan_networks(networks);
            if (n == 0)
	    {
                printf("No networks found.\n");
                continue;
            }

            printf("Select network number: ");
            int choice;
            if (scanf("%d", &choice) != 1 ||
                choice < 1 || choice > n)
	    {

                printf("Invalid selection.\n");
                while (getchar() != '\n');
                continue;
            }
            while (getchar() != '\n');

            char pass[128];
            printf("Enter password for '%s': ",
                   networks[choice - 1].ssid);

            if (!fgets(pass, sizeof(pass), stdin))
                continue;

            pass[strcspn(pass, "\n")] = 0;

            connect_wifi(networks[choice - 1].ssid, pass);

            memset(pass, 0, sizeof(pass));

        } else if (strcmp(input, "disconnect") == 0)
	{

            disconnect_wifi();

        } else if (strcmp(input, "exit") == 0)
	{

            break;

        } else
	{

		printf("Unknown command.\n");
        }
    }

    return 0;
}

