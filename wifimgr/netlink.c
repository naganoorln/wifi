#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "wifi.h"

int send_nlmsg(void *buffer, int len)
{
    struct sockaddr_nl addr = {0};
    addr.nl_family = AF_NETLINK;
    return sendto(nl_sock, buffer, len, 0,
                  (struct sockaddr *)&addr, sizeof(addr));
}

int resolve_nl80211()
{
    char buffer[MAX_MSG_SIZE] = {0};

    struct nlmsghdr *nlh = (struct nlmsghdr *)buffer;
    struct genlmsghdr *genlh =
        (struct genlmsghdr *)(buffer + sizeof(*nlh));

    nlh->nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    nlh->nlmsg_type = GENL_ID_CTRL;
    nlh->nlmsg_flags = NLM_F_REQUEST;
    nlh->nlmsg_seq = 1;
    nlh->nlmsg_pid = getpid();

    genlh->cmd = CTRL_CMD_GETFAMILY;
    genlh->version = 1;

    struct nlattr *attr =
        (struct nlattr *)(buffer + nlh->nlmsg_len);

    attr->nla_type = CTRL_ATTR_FAMILY_NAME;
    attr->nla_len = NLA_HDRLEN + strlen("nl80211") + 1;
    strcpy((char*)attr + NLA_HDRLEN, "nl80211");

    nlh->nlmsg_len += attr->nla_len;

    send_nlmsg(buffer, nlh->nlmsg_len);

    int len = recv(nl_sock, buffer, sizeof(buffer), 0);
    if (len <= 0) return -1;

    for (nlh = (struct nlmsghdr*)buffer;
         NLMSG_OK(nlh, len);
         nlh = NLMSG_NEXT(nlh, len)) {

        genlh = NLMSG_DATA(nlh);
        attr = (struct nlattr *)
               ((char*)genlh + GENL_HDRLEN);
        int attrlen =
            nlh->nlmsg_len - NLMSG_LENGTH(GENL_HDRLEN);

        for (; NLA_OK(attr, attrlen);
             attr = NLA_NEXT(attr, attrlen)) {

            if (attr->nla_type == CTRL_ATTR_FAMILY_ID)
                return *(int*)NLA_DATA(attr);
        }
    }

    return -1;
}

