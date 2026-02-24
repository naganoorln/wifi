#ifndef TYPES_H
#define TYPES_H

#define CTRL_PATH "/run/wpa_supplicant/wlo1"
#define BUF_SIZE 8192
#define MAX_NETWORKS 50

/* ANSI color codes */
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define GREEN   "\033[32m"

typedef struct {
    char ssid[256];
    char bssid[32];
    char signal[16];
    char flags[128];
} wifi_network;

#endif
