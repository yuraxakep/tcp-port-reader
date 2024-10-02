/*****************************************************************************/
/**
*  Brief: 	Contains definition for the functions needed by client apps.
*
*  Created: 02.10.2024
*  Author: 	Yurii Shenbor
*
******************************************************************************/

#ifndef __CLIENT_LIB_H__
#define __CLIENT_LIB_H__

/***************************** Include Files ********************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdint.h>

/************************** Constant Definitions *****************************/

#define BUFFER_SIZE     1024
#define MAX_PORTS       3
#define TCP_PORT        4001
#define UDP_PORT        4000
#define MSG_MAX_SIZE    4

/**************************** Type Definitions *******************************/

struct port_t {
    int sockfd;
    char *data_ptr;
    char buffer[BUFFER_SIZE];
};

enum operation_e {
    READ = 1,
    WRITE = 2
};

enum object_e {
    CHANNEL_1 = 1,
    CHANNEL_2 = 2,
    CHANNEL_3 = 3
};

enum property_e {
    ENABLED = 14,
    MIN_DURATION = 42,  // For channel 3 only
    MAX_DURATION = 43,  // For channel 3 only
    AMPLITUDE = 170,
    FREQUENCY = 255,
    GLITCH_CHANCE = 300
};

/************************** Function Prototypes ******************************/

void error_exit(const char *msg);

int findOpenPort(unsigned int port_number, struct sockaddr_in *tcp_server_addr);
int connectToPort(struct sockaddr_in *tcp_server_addr, int timeout_ms);
void readFromPort(struct port_t *port);
void* readFromPortInThread(void *args);

int makeMessage(enum operation_e op, enum object_e obj, enum property_e prop, uint16_t val, uint16_t *msg);
int startServer(struct in_addr *sin_addr, int port, struct sockaddr_in *server_addr);
void sendMessage(int sockfd, struct sockaddr_in *server_addr, uint16_t *msg, size_t size);
void changeBehavior(char *data, int udp_sokfd, struct sockaddr_in *udp_server_addr);

/************************** Variable Definitions *****************************/



#endif /* __CLIENT_LIB_H__ */