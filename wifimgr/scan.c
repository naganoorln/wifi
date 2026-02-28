#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>        
#include <sys/socket.h>  

#include "wifi.h"

void trigger_scan()
{
    char buffer[MAX_MSG_SIZE] = {0};

    struct nlmsghdr *nlh = (struct nlmsghdr *)buffer;
    struct genlmsghdr *genlh =
        (struct genlmsghdr *)(buffer + sizeof(*nlh));

    nlh->nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    nlh->nlmsg_type = nl80211_id;
    nlh->nlmsg_flags = NLM_F_REQUEST;
    nlh->nlmsg_seq = 2;
    nlh->nlmsg_pid = getpid();

    genlh->cmd = NL80211_CMD_TRIGGER_SCAN;
    genlh->version = 0;

    struct nlattr *attr =
        (struct nlattr *)(buffer + nlh->nlmsg_len);

    attr->nla_type = NL80211_ATTR_IFINDEX;
    attr->nla_len = NLA_HDRLEN + sizeof(int);
    memcpy((char*)attr + NLA_HDRLEN,
           &ifindex, sizeof(int));
    nlh->nlmsg_len += attr->nla_len;

    struct nlattr *ssid =
        (struct nlattr *)(buffer + nlh->nlmsg_len);

    ssid->nla_type = NL80211_ATTR_SCAN_SSIDS;
    ssid->nla_len = NLA_HDRLEN;  
    nlh->nlmsg_len += ssid->nla_len;

    send_nlmsg(buffer, nlh->nlmsg_len);
}

void parse_bss(struct nlattr *bss_attr)
{
    wifi_bss b;
    memset(&b, 0, sizeof(b));

    struct nlattr *attr;
    int rem = bss_attr->nla_len - NLA_HDRLEN;

    int wpa = 0, rsn = 0;

    for (attr = (struct nlattr*)((char*)bss_attr + NLA_HDRLEN);
         NLA_OK(attr, rem);
         attr = NLA_NEXT(attr, rem)) {

        switch (attr->nla_type) {

        case NL80211_BSS_BSSID:
            memcpy(b.bssid, NLA_DATA(attr), 6);
            break;

        case NL80211_BSS_FREQUENCY:
            b.freq = *(int*)NLA_DATA(attr);
            if (b.freq >= 2412 && b.freq <= 2472)
                b.channel = (b.freq - 2407) / 5;
            else if (b.freq >= 5000)
                b.channel = (b.freq - 5000) / 5;
            break;

        case NL80211_BSS_SIGNAL_MBM:
            b.signal = (*(int32_t*)NLA_DATA(attr)) / 100.0;
            break;

        case NL80211_BSS_INFORMATION_ELEMENTS: {
            unsigned char *ie = NLA_DATA(attr);
            int len = attr->nla_len - NLA_HDRLEN;
            int i = 0;

            while (i + 1 < len) {
                int id = ie[i];
                int elen = ie[i+1];

                if (id == 0 && elen <= 32) {
                    memcpy(b.ssid, &ie[i+2], elen);
                    b.ssid[elen] = '\0';
                    b.ssid_len = elen;
                }

                if (id == 48) rsn = 1;

                if (id == 221 && elen >= 4) {
                    if (ie[i+2]==0x00 && ie[i+3]==0x50 &&
                        ie[i+4]==0xf2 && ie[i+5]==1)
                        wpa = 1;
                }

                i += 2 + elen;
            }
            break;
        }
        }
    }

    if (rsn)
        strcpy(b.security, "WPA2/WPA3");
    else if (wpa)
        strcpy(b.security, "WPA");
    else
        strcpy(b.security, "OPEN");
    b.is_open = (rsn || wpa) ? 0 : 1;
    if (network_count < MAX_NETWORKS) {
   	networks[network_count] = b;
    	network_count++;
    	row(network_count, &b);
    }
}

void get_scan_results()
{
    char buffer[MAX_MSG_SIZE] = {0};

    struct nlmsghdr *nlh;
    struct genlmsghdr *genlh;

    nlh = (struct nlmsghdr *)buffer;
    genlh = (struct genlmsghdr *)(buffer + sizeof(*nlh));

    nlh->nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    nlh->nlmsg_type = nl80211_id;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nlh->nlmsg_seq = 3;
    nlh->nlmsg_pid = getpid();

    genlh->cmd = NL80211_CMD_GET_SCAN;
    genlh->version = 0;

    struct nlattr *attr =
        (struct nlattr *)(buffer + nlh->nlmsg_len);

    attr->nla_type = NL80211_ATTR_IFINDEX;
    attr->nla_len = NLA_HDRLEN + sizeof(int);
    memcpy((char*)attr + NLA_HDRLEN,
           &ifindex, sizeof(int));

    nlh->nlmsg_len += attr->nla_len;

    send_nlmsg(buffer, nlh->nlmsg_len);

    while (1) {
        int len = recv(nl_sock, buffer, sizeof(buffer), 0);
        if (len <= 0) break;

        for (nlh = (struct nlmsghdr*)buffer;
             NLMSG_OK(nlh, len);
             nlh = NLMSG_NEXT(nlh, len)) {

            if (nlh->nlmsg_type == NLMSG_DONE)
                return;

            genlh = NLMSG_DATA(nlh);
            attr = (struct nlattr*)
                   ((char*)genlh + GENL_HDRLEN);
            int attrlen =
                nlh->nlmsg_len - NLMSG_LENGTH(GENL_HDRLEN);

            for (; NLA_OK(attr, attrlen);
                 attr = NLA_NEXT(attr, attrlen)) {

                if (attr->nla_type == NL80211_ATTR_BSS)
                    parse_bss(attr);
            }
        }
    }
}

void scan_wifi()
{
    network_count = 0;
    header();
    trigger_scan();
    sleep(5);
    get_scan_results();
    printf("------------------------------------------------------------------------------------------------------------\n");
}

