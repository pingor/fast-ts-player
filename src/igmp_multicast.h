// IGMP multicast configuration and context structures

#ifndef IGMP_MULTICAST_H
#define IGMP_MULTICAST_H

// IGMP configuration structure
typedef struct {
    int version;
    int type;
    char groupAddress[16]; // IPv4 multicast address
} IgmpConfig;

// IGMP context structure
typedef struct {
    IgmpConfig config;
    int socket;
    // Other context-specific variables
} IgmpContext;

#endif // IGMP_MULTICAST_H
