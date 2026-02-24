#include "wifi_ctrl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

int send_command(const char *cmd, char *resp, int resp_size)
{
    int sock;
    struct sockaddr_un local, remote;
    int len;

    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return -1; }

    memset(&local, 0, sizeof(local));
    local.sun_family = AF_UNIX;
    snprintf(local.sun_path, sizeof(local.sun_path), "/tmp/wpa_ctrl_%d", getpid());
    unlink(local.sun_path);

    if (bind(sock, (struct sockaddr *)&local, sizeof(local)) < 0) {
        perror("bind"); close(sock); return -1;
    }

    memset(&remote, 0, sizeof(remote));
    remote.sun_family = AF_UNIX;
    strncpy(remote.sun_path, CTRL_PATH, sizeof(remote.sun_path) - 1);

    if (sendto(sock, cmd, strlen(cmd), 0, (struct sockaddr *)&remote, sizeof(remote)) < 0) {
        perror("sendto"); close(sock); unlink(local.sun_path); return -1;
    }

    len = recv(sock, resp, resp_size - 1, 0);
    if (len > 0) resp[len] = '\0'; else resp[0] = '\0';

    close(sock);
    unlink(local.sun_path);
    return len;
}

int wait_for_connection()
{
    char buf[BUF_SIZE];
    for (int i = 0; i < 15; i++) {
        send_command("STATUS", buf, sizeof(buf));
        if (strstr(buf, "wpa_state=COMPLETED")) return 1;
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
    while (line) {
        if (!strstr(line, "bssid")) {
            char *saveptr2;
            char *bssid = strtok_r(line, "\t", &saveptr2);
            strtok_r(NULL, "\t", &saveptr2); // freq
            char *signal = strtok_r(NULL, "\t", &saveptr2);
            char *flags  = strtok_r(NULL, "\t", &saveptr2);
            char *ssid   = strtok_r(NULL, "\t", &saveptr2);

            if (ssid && strlen(ssid) > 0 && count < MAX_NETWORKS) {
                strncpy(networks[count].ssid, ssid, sizeof(networks[count].ssid) - 1);
                strncpy(networks[count].bssid, bssid, sizeof(networks[count].bssid) - 1);
                strncpy(networks[count].signal, signal, sizeof(networks[count].signal) - 1);
                strncpy(networks[count].flags, flags, sizeof(networks[count].flags) - 1);
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

    snprintf(cmd, sizeof(cmd), "SET_NETWORK %d ssid \"%s\"", net_id, ssid);
    send_command(cmd, buf, sizeof(buf));
    snprintf(cmd, sizeof(cmd), "SET_NETWORK %d psk \"%s\"", net_id, pass);
    send_command(cmd, buf, sizeof(buf));
    snprintf(cmd, sizeof(cmd), "ENABLE_NETWORK %d", net_id);
    send_command(cmd, buf, sizeof(buf));
    snprintf(cmd, sizeof(cmd), "SELECT_NETWORK %d", net_id);
    send_command(cmd, buf, sizeof(buf));

    printf("Authenticating...\n");
    if (!wait_for_connection()) { printf("Connection failed.\n"); return; }

    printf("Connected to WiFi. Obtaining IP...\n");
    int ret;

    ret = system("dhclient -r wlo1");
    if (ret != 0) {
    	fprintf(stderr, "Warning: dhclient -r failed\n");
    }
    ret = system("dhclient wlo1");
    if (ret != 0) {
    	fprintf(stderr, "Warning: dhclient failed\n");
    }
    ret = system("resolvectl dns wlo1 8.8.8.8 1.1.1.1");
    if (ret != 0) {
    	fprintf(stderr, "Warning: setting DNS failed\n");
    }
    ret = system("resolvectl domain wlo1 ~.");
    if (ret != 0) {
    	fprintf(stderr, "Warning: setting domain failed\n");
    }
    printf("Internet ready.\n");
}

void disconnect_wifi()
{
    char buf[BUF_SIZE];
    int ret;

    send_command("DISCONNECT", buf, sizeof(buf));
    ret = system("dhclient -r wlo1");
    if (ret != 0) {
        fprintf(stderr, "Warning: dhclient failed\n");
    }
    printf("Disconnected.\n");
}
