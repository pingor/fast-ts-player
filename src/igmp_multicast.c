#include "igmp_multicast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <errno.h>

static int get_interface_ip(const char *if_name, struct in_addr *ip_addr) {
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        if (ifa->ifa_addr->sa_family == AF_INET && strcmp(ifa->ifa_name, if_name) == 0) {
            struct sockaddr_in *sin = (struct sockaddr_in *)ifa->ifa_addr;
            memcpy(ip_addr, &sin->sin_addr, sizeof(struct in_addr));
            freeifaddrs(ifaddr);
            return 0;
        }
    }
    freeifaddrs(ifaddr);
    return -1;
}

int igmp_init(const igmp_config_t *config, igmp_context_t *ctx) {
    struct sockaddr_in addr;
    struct ip_mreq mreq;
    struct ifreq ifr;

    if (!config || !ctx) {
        fprintf(stderr, "[IGMP] Invalid parameters\n");
        return -1;
    }

    ctx->socket_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (ctx->socket_fd < 0) {
        perror("[IGMP] socket() failed");
        return -1;
    }

    fprintf(stderr, "[IGMP] UDP socket created: fd=%d\n", ctx->socket_fd);

    int reuse = 1;
    setsockopt(ctx->socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    if (config->interface_name && config->interface_name[0]) {
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, config->interface_name, IFNAMSIZ - 1);
        if (setsockopt(ctx->socket_fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
            perror("[IGMP] setsockopt(SO_BINDTODEVICE) failed");
            close(ctx->socket_fd);
            return -1;
        }
        fprintf(stderr, "[IGMP] Bound to interface: %s\n", config->interface_name);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(config->multicast_port);

    if (bind(ctx->socket_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[IGMP] bind() failed");
        close(ctx->socket_fd);
        return -1;
    }

    fprintf(stderr, "[IGMP] Bound to port: %d\n", config->multicast_port);

    memset(&mreq, 0, sizeof(mreq));
    if (inet_aton(config->multicast_addr, &mreq.imr_multiaddr) == 0) {
        fprintf(stderr, "[IGMP] Invalid multicast address: %s\n", config->multicast_addr);
        close(ctx->socket_fd);
        return -1;
    }

    if (config->interface_name && config->interface_name[0]) {
        if (get_interface_ip(config->interface_name, &mreq.imr_interface) < 0) {
            fprintf(stderr, "[IGMP] Warning: Failed to get IP for interface\n");
            mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        }
    } else {
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    }

    if (setsockopt(ctx->socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("[IGMP] setsockopt(IP_ADD_MEMBERSHIP) failed");
        close(ctx->socket_fd);
        return -1;
    }

    fprintf(stderr, "[IGMP] ✓ Joined multicast group: %s\n", config->multicast_addr);

    int ttl = 32;
    setsockopt(ctx->socket_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    int loop = 0;
    setsockopt(ctx->socket_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

    int rmem = config->buffer_size * 4;
    setsockopt(ctx->socket_fd, SOL_SOCKET, SO_RCVBUF, &rmem, sizeof(rmem));

    ctx->buffer_size = config->buffer_size;
    ctx->recv_buffer = malloc(config->buffer_size);
    if (!ctx->recv_buffer) {
        fprintf(stderr, "[IGMP] Failed to allocate buffer\n");
        close(ctx->socket_fd);
        return -1;
    }

    fprintf(stderr, "[IGMP] ✓ Initialization successful\n");
    return 0;
}

int igmp_recv(igmp_context_t *ctx, uint8_t *buf, uint32_t buf_len) {
    if (!ctx || !buf || buf_len == 0 || ctx->socket_fd < 0) return -1;
    ssize_t bytes = recvfrom(ctx->socket_fd, buf, buf_len, 0, NULL, NULL);
    if (bytes < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("[IGMP] recvfrom() failed");
        }
        return -1;
    }
    return (int)bytes;
}

int igmp_cleanup(igmp_context_t *ctx) {
    if (!ctx || ctx->socket_fd < 0) return -1;
    close(ctx->socket_fd);
    ctx->socket_fd = -1;
    if (ctx->recv_buffer) {
        free(ctx->recv_buffer);
        ctx->recv_buffer = NULL;
    }
    fprintf(stderr, "[IGMP] ✓ Cleanup successful\n");
    return 0;
}
