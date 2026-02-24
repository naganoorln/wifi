#ifndef WIFI_CTRL_H
#define WIFI_CTRL_H

#include "types.h"

int send_command(const char *cmd, char *resp, int resp_size);
int wait_for_connection();
int scan_networks(wifi_network *networks);
void connect_wifi(const char *ssid, const char *pass);
void disconnect_wifi();

#endif
