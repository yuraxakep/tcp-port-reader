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
#define TIMEOUT_MS      100UL

// #define PRINT_TO_FILE   1   // Uncomment to print to file instead of STDOUT

/**************************** Type Definitions *******************************/

struct port_t{
    int sockfd;
    char buffer[BUFFER_SIZE];
};

/************************** Function Prototypes ******************************/

static void error_exit(const char *msg);

static int findOpenPort(unsigned int port_number, struct sockaddr_in *server_addr);
static int connectToPort(struct sockaddr_in *server_addr);
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
    FILE *out = stdout;

#ifdef PRINT_TO_FILE
    out = fopen("client1.log", "w");
    if (!out) {
        error_exit("Unable to open file");
    }
#endif

    for (port = 0; port < MAX_PORTS; port++) {
        struct sockaddr_in server_addr;
        if (findOpenPort(port + FIRST_PORT, &server_addr) < 0) {
            error_exit("An open port could not be found");
        }
        
        printf("Connecting to port %d...\n", port + FIRST_PORT);
        ports[port].sockfd = connectToPort(&server_addr);
    }

    while (1) {
        gettimeofday(&time, NULL);
        current_time_msec = ((unsigned long int)time.tv_sec * 1000) + ((unsigned long int)time.tv_usec / 1000);

        if (current_time_msec >= target_time_msec) {
            target_time_msec = current_time_msec + TIMEOUT_MS;

            // Read all ports, starting with the slowest one
            for (port = MAX_PORTS; port >= 0; --port) {
                data[port] = readFromPort(&ports[port]);
            }

            fprintf(out, "{\"timestamp\": %lu, \"out1\": \"%s\", \"out2\": \"%s\", \"out3\": \"%s\"}\n",
                    current_time_msec, data[0], data[1], data[2]);
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
    struct timeval timeout = {0, 25000};
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
        //printf("Received: %d\n", bytes_received);
    } else {
        // Set value as '--' if no data received
        port->buffer[0] = port->buffer[1] = '-';
        port->buffer[2] = '\0';
        data_p = &port->buffer[0];
    }

    return data_p;
}