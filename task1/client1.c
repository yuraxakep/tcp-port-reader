/*****************************************************************************/
/**
*  Brief: 	Reads data from TCP ports 4001 ... 4003 and prints it to standard output.
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
#define MAX_PORTS       3
#define FIRST_PORT      4001
#define IP_ADDR         "0.0.0.0"
#define TIMEOUT_MS      100UL

/**************************** Type Definitions *******************************/

struct port_t{
    int sockfd;
    char buffer[BUFFER_SIZE];
};

/************************** Function Prototypes ******************************/

static void error_exit(const char *msg);

static int connectToPort(int port);
static char *readFromPort(struct port_t *port) ;

/************************** Variable Definitions *****************************/



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
    char *data[MAX_PORTS];
    struct timeval time;
    unsigned long int target_time_msec, current_time_msec;
    int port;

    for (port = 0; port < MAX_PORTS; port++) {
        ports[port].sockfd = connectToPort(port + FIRST_PORT);
    }

    gettimeofday(&time, NULL);
    target_time_msec = ((unsigned long int)time.tv_sec * 1000) + ((unsigned long int)time.tv_usec / 1000) + TIMEOUT_MS;

    while (1) {
        for (port = 0; port < MAX_PORTS; port++) {
            data[port] = readFromPort(&ports[port]);
        }

        gettimeofday(&time, NULL);
        current_time_msec = ((unsigned long int)time.tv_sec * 1000) + ((unsigned long int)time.tv_usec / 1000);

        if (current_time_msec >= target_time_msec) {
            target_time_msec = current_time_msec + TIMEOUT_MS;

            printf("{\"timestamp\": %lu, \"out1\": \"%s\", \"out2\": \"%s\", \"out3\": \"%s\"}\n",
                    current_time_msec, data[0], data[1], data[2]);
        }
    }

    for (port = 0; port < MAX_PORTS; port++) {
        close(ports[port].sockfd);
    }

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
*
* @return	file descriptor
*
* @note		None
*
**************************************************************************/
static int connectToPort(int port_number) {
    struct sockaddr_in server_addr;
    int sockfd;

    printf("Connecting to port %d...\n", port_number);

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error_exit("Socket creation failed");
    }

    // Set up server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    if (inet_pton(AF_INET, IP_ADDR, &server_addr.sin_addr) <= 0) {
        error_exit("Invalid address or address not supported");
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
    }

    // Set read timeout to 1ms
    struct timeval timeout = {0, 1000};
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Invalid socket options");
    }

    return sockfd;
}

/**************************************************************************/
/**
*
* @brief    Reads data from the given port
*
* @param	port - contains the file descriptor and buffer for the given port
*
* @return	a pointer to the current value
*
* @note		None
*
**************************************************************************/
static char *readFromPort(struct port_t *port) {
    char *data_p = NULL;

    int bytes_received = recv(port->sockfd, port->buffer, sizeof(port->buffer) - 1, 0);
    if (bytes_received > 0) {
        // Replace '\n' with '\0'
        port->buffer[bytes_received - 1] = '\0';
        // Handle negative and positive values accordingly
        if (bytes_received >= 5 && port->buffer[bytes_received - 5] == '-'){
            data_p = &port->buffer[bytes_received - 5];
        } else {
            data_p = &port->buffer[bytes_received - 4];
        }
    } else {
        // Set value as '--' if no data received
        port->buffer[0] = port->buffer[1] = '-';
        port->buffer[2] = '\0';
        data_p = &port->buffer[0];
    }

    return data_p;
}