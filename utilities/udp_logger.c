/*****************************************************************************/
/**
*  Brief: 	Sends a read request to UDP port 4000 with all possible combinations
*           of object and property fields to retrieve their values.
*
*  Created: 30.09.2024
*  Author: 	Yurii Shenbor
*
******************************************************************************/

/***************************** Include Files ********************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdint.h>

/************************** Constant Definitions *****************************/

#define MAX_PORTS       3
#define TCP_PORT        4001
#define UDP_PORT        4000

/**************************** Type Definitions *******************************/



/************************** Function Prototypes ******************************/

static void error_exit(const char *msg);

static int findOpenPort(unsigned int port_number, struct sockaddr_in *server_addr);
int startServer(struct in_addr *sin_addr, int port, struct sockaddr_in *server_addr);
void sendMessage(int sockfd, struct sockaddr_in *server_addr, uint16_t *msg, size_t size);

/************************** Variable Definitions *****************************/



/**************************************************************************/
/**
*
* @brief    Main function for UDP logger.
*
* @param	None
*
* @return	None
*
* @note		Look for the output log in the STDOUT of docker container.
*
**************************************************************************/
int main(int argc, char *argv[]) {
    struct sockaddr_in udp_server_addr;
    struct sockaddr_in tcp_server_addr;
    int sockfd;

    // Check for open ports and obtain IP address
    if (findOpenPort(TCP_PORT, &tcp_server_addr) < 0) {
        error_exit("An open port could not be found");
    }

    // Start UDP server
    sockfd = startServer(&tcp_server_addr.sin_addr, UDP_PORT, &udp_server_addr);

    // Read all objects to get ist property
    uint16_t msg[] = {1, 0, 0};
    for (int j = 0; j < MAX_PORTS; ++j) {
        msg[1]++;
        for (int i = 0; i < 65536; ++i) {
            usleep(1);
            msg[2] = i;
            sendMessage(sockfd, &udp_server_addr, msg, sizeof(msg));
        }
    }

    close(sockfd);
    return 0;
}

/**************************************************************************/
/**
*
* @brief    Helper function
*
* @param	msg - error message to print before exit
*
* @return	None
*
* @note		None
*
**************************************************************************/
static void error_exit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

/**************************************************************************/
/**
*
* @brief    Creates UDP socket
*
* @param	sin_addr - IP address
* @param	port_number - port number
* @param	[out] server_addr - the structure describing an Internet socket address
*
* @return	0 if given port is found, otherwise -1
*
* @note		None
*
**************************************************************************/
int startServer(struct in_addr *sin_addr, int port, struct sockaddr_in *server_addr) {
    int sockfd;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        error_exit("UDP Socket creation failed");
    }

    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);
    server_addr->sin_addr.s_addr = sin_addr->s_addr;
    
    return sockfd;
}

/**************************************************************************/
/**
*
* @brief    Sends message to the given port
*
* @param	sockfd - socket file descriptor
* @param	server_addr - the structure describing an Internet socket address
* @param	msg - a pointer to the message
* @param	size - size of the message
*
* @return	None
*
* @note		None
*
**************************************************************************/
void sendMessage(int sockfd, struct sockaddr_in *server_addr, uint16_t *msg, size_t size) {
    uint16_t *buf = malloc(size);

    // Swap byte order (Little-endian to Big-endian)
    for (int i = 0; i < size / sizeof(uint16_t); ++i) {
        buf[i] = ((msg[i] & 0x00FF) << 8) | ((msg[i] & 0xFF00) >> 8);
    }

    int status = sendto(sockfd, (void *)buf, size, 0, (struct sockaddr *)server_addr, sizeof(struct sockaddr_in));

    free(buf);
    if (status < 0) {
        error_exit("Sendto failed");
    }
}

/**************************************************************************/
/**
*
* @brief    Connects to the given port
*
* @param	port_number - port number
* @param	[out] server_addr - the structure describing an Internet socket address
*
* @return	0 if given port is found, otherwise -1
*
* @note		None
*
**************************************************************************/
static int findOpenPort(unsigned int port_number, struct sockaddr_in *server_addr) {
    FILE *fp = fopen("/proc/net/tcp", "r");
    int status = -1;

    if (!fp) {
        error_exit("Unable to open /proc/net/tcp");
    }

    char line[256];
    // Skip the first line (header)
    fgets(line, sizeof(line), fp); 

    while (fgets(line, sizeof(line), fp)) {
        unsigned int local_ip_hex, remote_ip_hex;
        unsigned int local_port, remote_port;
        unsigned int state;

        // Extract local and remote addresses, ports, and the connection state
        sscanf(line, "%*d: %8X:%4X %8X:%4X %2X", &local_ip_hex, &local_port, &remote_ip_hex, &remote_port, &state);

        // If port found and it is opened
        if (local_port == port_number && state == 0x0a) {
            // Set up server address structure
            server_addr->sin_family = AF_INET;
            server_addr->sin_port = htons(local_port);
            server_addr->sin_addr.s_addr = htonl(local_ip_hex);

            status = 0;
            break;
        }
    }

    fclose(fp);
    return status;
}
