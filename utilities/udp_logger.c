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

#include "../lib/client_lib.h"

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
