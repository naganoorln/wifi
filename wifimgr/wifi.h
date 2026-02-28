#ifndef WIFI_H
#define WIFI_H

#include <linux/netlink.h>
#include <linux/genetlink.h>
#include <linux/nl80211.h>

#define BUF_SIZE 1024
#define MAX_MSG_SIZE 8192
#define MAX_NETWORKS 100

#ifndef NLA_ALIGNTO
#define NLA_ALIGNTO 4
#endif
#ifndef NLA_ALIGN
#define NLA_ALIGN(len) (((len) + NLA_ALIGNTO - 1) & ~(NLA_ALIGNTO - 1))
#endif
#ifndef NLA_HDRLEN
#define NLA_HDRLEN ((int) NLA_ALIGN(sizeof(struct nlattr)))
#endif
#ifndef NLA_DATA
#define NLA_DATA(nla) ((void *)((char *)(nla) + NLA_HDRLEN))
#endif
#ifndef NLA_NEXT
#define NLA_NEXT(nla, attrlen) \
 ((attrlen) -= NLA_ALIGN((nla)->nla_len), \
  (struct nlattr*)(((char *)(nla)) + NLA_ALIGN((nla)->nla_len)))
#endif
#ifndef NLA_OK
#define NLA_OK(nla, len) \
 ((len) >= (int)sizeof(struct nlattr) && \
  (nla)->nla_len >= sizeof(struct nlattr) && \
  (nla)->nla_len <= (len))
#endif

typedef struct {
    unsigned char bssid[6];
    char ssid[33];
    int ssid_len;
    int freq;
    int channel;
    float signal;
    char security[16];
    int is_open;
} wifi_bss;

extern int nl_sock;
extern int nl80211_id;
extern int ifindex;
extern int network_count;
extern wifi_bss networks[MAX_NETWORKS];

/* netlink */
int send_nlmsg(void *buffer, int len);
int resolve_nl80211(void);

/* scan */
void trigger_scan(void);
void parse_bss(struct nlattr *bss_attr);
void get_scan_results(void);
void scan_wifi(void);

/* connect */
int disconnect_wifi(void);
void connect_wifi(int choice);
void get_password(char *password, size_t size);

/* display */
void header(void);
void row(int index, wifi_bss *b);
void help(void);

#endif
