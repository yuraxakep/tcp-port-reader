/*****************************************************************************/
/**
*  Brief: 	Reads data from TCP ports 4001 ... 4003 and prints it to STDOUT every 20 ms,
            changes the behavior of port 4001 based on data from port 4003.
*
*  Created: 01.10.2024
*  Author: 	Yurii Shenbor
*
******************************************************************************/

/***************************** Include Files ********************************/

#include "../lib/client_lib.h"

/************************** Constant Definitions *****************************/

#define TIMEOUT_MS      20UL

// #define PRINT_TO_FILE   1   // Uncomment to print to file instead of STDOUT

/**************************************************************************/
/**
*
* @brief    Main function for client 2.
*
* @param	None
*
* @return	None
*
* @note		None
*
**************************************************************************/
int main(int argc, char *argv[]) {
    struct port_t ports[MAX_PORTS];
    pthread_t thread_id[MAX_PORTS];
    struct sockaddr_in tcp_server_addr, udp_server_addr;
    struct timeval time;
    unsigned long int target_time_msec = 0, current_time_msec;
    int port, udp_sokfd;
    FILE *out = stdout;

#ifdef PRINT_TO_FILE
    out = fopen("logs/client2.log", "w");
    if (!out) {
        error_exit("Unable to open file");
    }
#endif

    for (port = 0; port < MAX_PORTS; port++) {
        struct sockaddr_in tcp_server_addr;
        if (findOpenPort(port + TCP_PORT, &tcp_server_addr) < 0) {
            error_exit("An open port could not be found");
        }
        
        // printf("Connecting to port %d...\n", port + TCP_PORT);
        ports[port].sockfd = connectToPort(&tcp_server_addr, 10000);
    }

    // Start UDP server
    udp_sokfd = startServer(&tcp_server_addr.sin_addr, UDP_PORT, &udp_server_addr);

    while (1) {
        gettimeofday(&time, NULL);
        current_time_msec = ((unsigned long int)time.tv_sec * 1000) + ((unsigned long int)time.tv_usec / 1000);

        if (current_time_msec >= target_time_msec) {
            target_time_msec = current_time_msec + TIMEOUT_MS;

            // Spawn threads to read from multiple ports simultaneously
            for (port = MAX_PORTS; port >= 0; --port) {
                pthread_create(&thread_id[port], NULL, readFromPortInThread, (void *)&ports[port]);
            }

            pthread_join(thread_id[0], NULL);
            pthread_join(thread_id[1], NULL);
            pthread_join(thread_id[2], NULL);
            
            changeBehavior(ports[2].data_ptr, udp_sokfd, &udp_server_addr);
            fprintf(out, "{\"timestamp\": %lu, \"out1\": \"%s\", \"out2\": \"%s\", \"out3\": \"%s\"}\n",
                    current_time_msec, ports[0].data_ptr, ports[1].data_ptr, ports[2].data_ptr);
        }
    }

    for (port = 0; port < MAX_PORTS; port++) {
        close(ports[port].sockfd);
    }
    close(udp_sokfd);

#ifdef PRINT_TO_FILE
    fclose(out);
#endif

    return 0;
}
