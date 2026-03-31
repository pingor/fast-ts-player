#include "igmp_multicast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <time.h>

static volatile int should_exit = 0;

static void signal_handler(int sig) {
    printf("\n[INFO] Signal received, shutting down...\n");
    should_exit = 1;
}

static void print_usage(const char *prog) {
    printf("\n╔══════════════════════════════════════════════════════╗\n");
    printf("║  🎬 IGMP Multicast TS Player                         ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n\n");
    printf("Usage: %s -a <addr> -p <port> -i <interface> [options]\n\n", prog);
    printf("Required:\n");
    printf("  -a ADDRESS        Multicast address (239.1.1.1)\n");
    printf("  -p PORT           Multicast port (1234)\n");
    printf("  -i INTERFACE      Interface name (eth0)\n\n");
    printf("Options:\n");
    printf("  -d SECS           Duration in seconds\n");
    printf("  -h                Show help\n\n");
}

int main(int argc, char *argv[]) {
    igmp_config_t config = {0};
    igmp_context_t ctx = {0};
    uint8_t buffer[65536];
    int duration = 0;
    
    config.multicast_port = 1234;
    config.buffer_size = 65536;
    config.recv_timeout_ms = 1000;

    int opt;
    while ((opt = getopt(argc, argv, "a:p:i:d:h")) != -1) {
        switch (opt) {
            case 'a': config.multicast_addr = optarg; break;
            case 'p': config.multicast_port = atoi(optarg); break;
            case 'i': config.interface_name = optarg; break;
            case 'd': duration = atoi(optarg); break;
            case 'h': print_usage(argv[0]); return 0;
            default: print_usage(argv[0]); return 1;
        }
    }

    if (!config.multicast_addr || !config.interface_name) {
        fprintf(stderr, "Error: Missing required parameters\n");
        print_usage(argv[0]);
        return 1;
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("\n╔══════════════════════════════════════════════════════╗\n");
    printf("║  Configuration                                      ║\n");
    printf("╠══════════════════════════════════════════════════════╣\n");
    printf("║ Multicast: %s:%d on %s\n", config.multicast_addr, config.multicast_port, config.interface_name);
    printf("╚══════════════════════════════════════════════════════╝\n\n");

    if (igmp_init(&config, &ctx) < 0) {
        fprintf(stderr, "Failed to initialize\n");
        return 1;
    }

    printf("📊 Receiving packets... (Ctrl+C to stop)\n\n");
    
    int packets = 0;
    time_t start = time(NULL);
    uint64_t bytes = 0;

    while (!should_exit) {
        int ret = igmp_recv(&ctx, buffer, sizeof(buffer));
        if (ret > 0) {
            packets++;
            bytes += ret;
            if (packets % 1000 == 0)
                printf("[%lds] %d packets, %.2f MB\n", time(NULL) - start, packets, bytes / (1024.0 * 1024.0));
        }
        if (duration > 0 && (time(NULL) - start) >= duration) break;
    }

    igmp_cleanup(&ctx);
    printf("\n✅ Total: %d packets, %.2f MB\n\n", packets, bytes / (1024.0 * 1024.0));
    return 0;
}
