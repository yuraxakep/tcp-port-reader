/*****************************************************************************/
/**
*  Brief: 	Reads data from the given TCP port and stores log into a file.
*
*  Created: 28.09.2024
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

/************************** Constant Definitions *****************************/

#define BUFFER_SIZE     1024

/**************************** Type Definitions *******************************/


/************************** Function Prototypes ******************************/

static void error_exit(const char *msg);

static int findOpenPort(unsigned int port_number, struct sockaddr_in *server_addr);
static int connectToPort(struct sockaddr_in *server_addr);

/************************** Variable Definitions *****************************/



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
        fprintf(stderr, "Usage: %s <Port> <Samples>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    struct timeval time;
    unsigned long int current_time_msec;
    int bytes_received = 0;
    int sample = 0;

    int port_number = atoi(argv[1]);
    int samples = atoi(argv[2]);

    struct sockaddr_in server_addr;
    if (findOpenPort(port_number, &server_addr) < 0) {
        error_exit("An open port could not be found");
    }
    
    printf("Connecting to port %d...\n", port_number);
    int sockfd = connectToPort(&server_addr);

    char file_name[32];
    strcpy(file_name, argv[1]);
    strcat(file_name, ".log");
    FILE *fp = fopen(file_name, "w");
    if (!fp) {
        error_exit("Unable to open file");
    }

    while (1) {
        bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received - 1] = '\0';
            
            gettimeofday(&time, NULL);
            current_time_msec = ((unsigned long int)time.tv_sec * 1000) + ((unsigned long int)time.tv_usec / 1000);
            fprintf(fp, "{\"timestamp\": %lu, \"data\": \"%s\"}\n", current_time_msec, buffer);

            if (sample++ >= samples){
                break;
            }
        }
    }
    
    printf("Log saved into the file: %s\n", file_name);
    fclose(fp);
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

/**************************************************************************/
/**
*
* @brief    Connects to the given port
*
* @param	server_addr - a pointer to the structure describing an Internet socket address
*
* @return	file descriptor
*
* @note		None
*
**************************************************************************/
static int connectToPort(struct sockaddr_in *server_addr) {
    int sockfd;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error_exit("Socket creation failed");
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)server_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("Connection failed");
        close(sockfd);
    }

    // TODO: Adjust the timeout to get a balance between correct output and precise time interval
    // Set read timeout to 25ms
    struct timeval timeout = {0, 1};
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Invalid socket options");
    }

    return sockfd;
}
