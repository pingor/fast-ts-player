#ifndef IGMP_MULTICAST_H
#define IGMP_MULTICAST_H

#include <stdint.h>

/**
 * IGMP 多播接收器配置结构体
 */
typedef struct {
    const char *multicast_addr;    // 多播 IP 地址 (e.g., "239.1.1.1")
    uint16_t    multicast_port;    // 多播端口号 (e.g., 1234)
    const char *interface_name;    // 网卡名称 (e.g., "eth0", "wlan0")
    uint32_t    buffer_size;       // UDP 接收缓冲大小 (bytes)
    uint32_t    recv_timeout_ms;   // 接收超时时间 (milliseconds)
} igmp_config_t;

/**
 * IGMP 多播接收器运行时上下文
 */
typedef struct {
    int socket_fd;                 // UDP socket 文件描述符
    uint8_t *recv_buffer;          // 接收数据缓冲区
    uint32_t buffer_size;          // 缓冲区大小
} igmp_context_t;

/**
 * 初始化 IGMP 多播接收器
 */
int igmp_init(const igmp_config_t *config, igmp_context_t *ctx);

/**
 * 从多播组接收一个数据包
 */
int igmp_recv(igmp_context_t *ctx, uint8_t *buf, uint32_t buf_len);

/**
 * 清理资源（离开多播组，关闭 socket）
 */
int igmp_cleanup(igmp_context_t *ctx);

#endif // IGMP_MULTICAST_H
