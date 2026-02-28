#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#include "wifi.h"

int disconnect_wifi()
{
    char buf[BUF_SIZE];
    struct nlmsghdr *nlh = (struct nlmsghdr *)buf;
    struct genlmsghdr *gh;
    struct nlattr *attr;
    int seq = 0;

    nlh->nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    nlh->nlmsg_type = nl80211_id;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    nlh->nlmsg_seq = seq++;
    nlh->nlmsg_pid = getpid();

    gh = (struct genlmsghdr *)NLMSG_DATA(nlh);
    gh->cmd = NL80211_CMD_DISCONNECT;
    gh->version = 1;
    gh->reserved = 0;

    attr = (struct nlattr *)((char *)gh + GENL_HDRLEN);
    attr->nla_type = NL80211_ATTR_IFINDEX;
    attr->nla_len = NLA_HDRLEN + sizeof(int);
    memcpy((char *)attr + NLA_HDRLEN, &ifindex, sizeof(int));
    nlh->nlmsg_len += NLA_ALIGN(attr->nla_len);

    struct nlattr *reason = (struct nlattr *)((char *)attr + NLA_ALIGN(attr->nla_len));
    int reason_code = 3; /* DEAUTH_LEAVING */
    reason->nla_type = NL80211_ATTR_REASON_CODE;
    reason->nla_len = NLA_HDRLEN + sizeof(int);
    memcpy((char *)reason + NLA_HDRLEN, &reason_code, sizeof(int));
    nlh->nlmsg_len += NLA_ALIGN(reason->nla_len);

    struct sockaddr_nl kernel = {0};
    kernel.nl_family = AF_NETLINK;

    if (sendto(nl_sock, nlh, nlh->nlmsg_len, 0,
               (struct sockaddr *)&kernel, sizeof(kernel)) < 0) {
        perror("sendto");
        return -1;
    }

    char ack_buf[BUF_SIZE];
    if (recv(nl_sock, ack_buf, sizeof(ack_buf), 0) < 0) {
        perror("recv");
        return -1;
    }

    system("killall wpa_supplicant 2>/dev/null");
    system("ip addr flush dev wlo1"); /* clear IP */
    printf("WiFi disconnected\n");
    return 0;
}

void get_password(char *password, size_t size) {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    if (fgets(password, size, stdin) != NULL) {
        size_t len = strlen(password);
        if (len > 0 && password[len - 1] == '\n')
            password[len - 1] = '\0';
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

void connect_wifi(int choice) {
    if (choice < 1 || choice > network_count) {
        printf("Invalid selection\n");
        return;
    }

    wifi_bss *net = &networks[choice - 1];

    if (net->is_open) {
        printf("Connecting to open network '%s' via Netlink...\n", net->ssid);

        char buf[BUF_SIZE];
        struct nlmsghdr *nlh = (struct nlmsghdr *)buf;
        struct genlmsghdr *gh;
        struct nlattr *attr;
        int seq = 0;

        nlh->nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
        nlh->nlmsg_type = nl80211_id;
        nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
        nlh->nlmsg_seq = seq++;
        nlh->nlmsg_pid = getpid();

        gh = (struct genlmsghdr *)NLMSG_DATA(nlh);
        gh->cmd = NL80211_CMD_CONNECT;
        gh->version = 1;

        attr = (struct nlattr *)((char *)gh + GENL_HDRLEN);
        attr->nla_type = NL80211_ATTR_IFINDEX;
        attr->nla_len = NLA_HDRLEN + sizeof(int);
        memcpy((char *)attr + NLA_HDRLEN, &ifindex, sizeof(int));
        nlh->nlmsg_len += NLA_ALIGN(attr->nla_len);

        struct nlattr *ssid_attr = (struct nlattr *)((char *)attr + NLA_ALIGN(attr->nla_len));
        int ssid_len = strlen(net->ssid);
        ssid_attr->nla_type = NL80211_ATTR_SSID;
        ssid_attr->nla_len = NLA_HDRLEN + ssid_len;
        memcpy((char *)ssid_attr + NLA_HDRLEN, net->ssid, ssid_len);
        nlh->nlmsg_len += NLA_ALIGN(ssid_attr->nla_len);

        struct sockaddr_nl kernel = {0};
        kernel.nl_family = AF_NETLINK;
        if (sendto(nl_sock, nlh, nlh->nlmsg_len, 0, (struct sockaddr *)&kernel, sizeof(kernel)) < 0) {
            perror("sendto");
            return;
        }

        char ack_buf[BUF_SIZE];
        if (recv(nl_sock, ack_buf, sizeof(ack_buf), 0) < 0) {
            perror("recv");
            return;
        }

        printf("Connected successfully to open network '%s'\n", net->ssid);

    } else {
        char password[128];
        printf("Enter password for '%s': ", net->ssid);
        fflush(stdout);           
        get_password(password, sizeof(password));
        printf("\n");            

        FILE *fp = fopen("/tmp/wifi.conf", "w");
        if (!fp) { perror("fopen"); return; }

        fprintf(fp,
            "network={\n"
            "    ssid=\"%s\"\n"
            "    psk=\"%s\"\n"
            "}\n",
            net->ssid,
            password
        );
        fclose(fp);

        disconnect_wifi();
        system("ip link set wlo1 down");
        system("ip link set wlo1 up");

        system("wpa_supplicant -B -i wlo1 -c /tmp/wifi.conf -C /var/run/wpa_supplicant");
        sleep(5);
        
	system("dhclient wlo1");
        system("resolvectl dns wlo1 8.8.8.8 1.1.1.1");
        system("resolvectl domain wlo1 ~.");

        printf("Connected successfully to '%s'\n", net->ssid);
    }
}

