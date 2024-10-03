/*****************************************************************************/
/**
*  Brief: 	Reads data from TCP ports 4001 ... 4003 and prints it to standard output.
*
*  Created: 28.09.2024
*  Author: 	Yurii Shenbor
*
******************************************************************************/

/***************************** Include Files ********************************/

#include "../lib/client_lib.h"

/************************** Constant Definitions *****************************/

#define TIMEOUT_MS      100UL

// #define PRINT_TO_FILE   1   // Uncomment to print to file instead of STDOUT

/**************************************************************************/
/**
*
* @brief    Main function for client 1.
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
    struct timeval time;
    unsigned long int target_time_msec = 0, current_time_msec;
    int port;
    FILE *out = stdout;

#ifdef PRINT_TO_FILE
    out = fopen("logs/client1.log", "w");
    if (!out) {
        error_exit("Unable to open file");
    }
#endif

    for (port = 0; port < MAX_PORTS; port++) {
        struct sockaddr_in server_addr;
        if (findOpenPort(port + TCP_PORT, &server_addr) < 0) {
            error_exit("An open port could not be found");
        }
        
        // printf("Connecting to port %d...\n", port + TCP_PORT);
        ports[port].sockfd = connectToPort(&server_addr, 25000);
    }

    while (1) {
        gettimeofday(&time, NULL);
        current_time_msec = ((unsigned long int)time.tv_sec * 1000) + ((unsigned long int)time.tv_usec / 1000);

        if (current_time_msec >= target_time_msec) {
            target_time_msec = current_time_msec + TIMEOUT_MS;

            // Read all ports, starting with the slowest one
            for (port = MAX_PORTS; port >= 0; --port) {
                readFromPort(&ports[port]);
            }

            fprintf(out, "{\"timestamp\": %lu, \"out1\": \"%s\", \"out2\": \"%s\", \"out3\": \"%s\"}\n",
                    current_time_msec, ports[0].data_ptr, ports[1].data_ptr, ports[2].data_ptr);
        }
    }

    for (port = 0; port < MAX_PORTS; port++) {
        close(ports[port].sockfd);
    }

#ifdef PRINT_TO_FILE
    fclose(out);
#endif

    return 0;
}
