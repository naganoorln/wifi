WiFi CLI Manager (C + wpa_supplicant):

A lightweight WiFi command-line manager written in C that communicates directly with
wpa_supplicant via UNIX domain sockets.

This project manually handles:

WiFi scanning
WPA/WPA2 authentication
DHCP IP assignment
DNS configuration using
systemd-resolved

It works without
NetworkManager.

Architecture Overview:

WiFi CLI (C Program)
        ↓
wpa_supplicant (Authentication)
        ↓
dhclient (IP address)
        ↓
systemd-resolved (DNS)
        ↓
Internet Access


Requirements:

Linux (Tested on Ubuntu)
gcc
wpa_supplicant
dhclient
systemd-resolved

Install dependencies:
sudo apt install build-essential wpasupplicant isc-dhcp-client

Setup & Execution Sequence

Stop NetworkManager (Important)
We manually manage WiFi, so stop:
sudo systemctl stop NetworkManager

(Optional disable auto-start)
sudo systemctl disable NetworkManager

Bring Interface Up
Replace wlo1 if your interface name is different.
sudo ip link set wlo1 up

Check:
ip link show wlo1

Start wpa_supplicant
sudo wpa_supplicant -B -i wlo1 -c /dev/null -C /run/wpa_supplicant

Verify control socket:

ls /run/wpa_supplicant/

You should see:

wlo1

Compile Program:
gcc wifi_manager.c -o wifi_manager

Run Program:
sudo ./wifi_manager

Available Commands

Inside CLI:

scan        → Scan and list networks
connect     → Select and connect to network
disconnect  → Disconnect WiFi
help        → Show commands
exit        → Quit program

How Internet Is Enabled:

When connecting:

Adds network via wpa_supplicant
Waits until authentication is complete

Runs DHCP:
dhclient wlo1


Configures DNS:
resolvectl dns wlo1 8.8.8.8 1.1.1.1
resolvectl domain wlo1 ~.

Without DNS configuration, browser will not work.

Testing Connectivity

After connection:

ping 8.8.8.8       # Test IP connectivity
ping google.com    # Test DNS resolution


If both work → browser will work.

Restore Normal System

To restore default behavior:
sudo systemctl start NetworkManager

Features:

Direct communication with wpa_supplicant via UNIX socket
Dynamic network ID handling
Waits for authentication completion
DHCP IP configuration
DNS setup with systemd-resolved
CLI interface


Package Creation:

Build Package
./create_package.sh

It will create:
wifi-manager_1.0-1.deb

Install
sudo dpkg -i wifi-manager_1.0-1.deb
Run Program
wifi-manager

Uninstall
Remove package:
sudo dpkg -r wifi-manager

Purge completely:
sudo dpkg -P wifi-manager
