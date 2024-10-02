/*****************************************************************************/
/**
*  Brief: 	Reads data from the given TCP port and stores log into a file.
*
*  Created: 29.09.2024
*  Author: 	Yurii Shenbor
*
******************************************************************************/

/***************************** Include Files ********************************/

#include "../lib/client_lib.h"

/**************************************************************************/
/**
*
* @brief    Main function for tcp logger.
*
* @param	None
*
* @return	None
*
* @note		None
*
**************************************************************************/
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Port> <Duration_sec>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    struct timeval time;
    unsigned long int current_time_msec, duration_msec;
    int bytes_received = 0;

    int port_number = atoi(argv[1]);
    int duration_sec = atoi(argv[2]);

    struct sockaddr_in server_addr;
    if (findOpenPort(port_number, &server_addr) < 0) {
        error_exit("An open port could not be found");
    }
    
    printf("Connecting to port %d...\n", port_number);
    int sockfd = connectToPort(&server_addr, 1);

    char file_name[32];
    strcpy(file_name, argv[1]);
    strcat(file_name, ".log");
    FILE *fp = fopen(file_name, "w");
    if (!fp) {
        error_exit("Unable to open file");
    }

    gettimeofday(&time, NULL);
    duration_msec = ((unsigned long int)time.tv_sec * 1000) + ((unsigned long int)time.tv_usec / 1000) + 
                    ((unsigned long int)duration_sec * 1000);

    while (1) {
        bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received - 1] = '\0';
            
            gettimeofday(&time, NULL);
            current_time_msec = ((unsigned long int)time.tv_sec * 1000) + ((unsigned long int)time.tv_usec / 1000);
            fprintf(fp, "{\"timestamp\": %lu, \"data\": \"%s\"}\n", current_time_msec, buffer);

            if (current_time_msec >= duration_msec){
                break;
            }
        }
    }
    
    printf("Log saved into the file: %s\n", file_name);
    fclose(fp);
    close(sockfd);
    return 0;
}
