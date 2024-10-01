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
#define TIMEOUT_MS      20UL
#define MSG_MAX_SIZE    4

// #define PRINT_TO_FILE   1   // Uncomment to print to file instead of STDOUT

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

static void error_exit(const char *msg);

static int findOpenPort(unsigned int port_number, struct sockaddr_in *tcp_server_addr);
static int connectToPort(struct sockaddr_in *tcp_server_addr);
static void* readFromPort(void *args);

int makeMessage(enum operation_e op, enum object_e obj, enum property_e prop, uint16_t val, uint16_t *msg);
int startServer(struct in_addr *sin_addr, int port, struct sockaddr_in *server_addr);
void sendMessage(int sockfd, struct sockaddr_in *server_addr, uint16_t *msg, size_t size);
static inline void changeBehavior(char *data, int udp_sokfd, struct sockaddr_in *udp_server_addr);

/************************** Variable Definitions *****************************/



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
    unsigned long int target_time_msec, current_time_msec;
    int port, udp_sokfd;
    FILE *out = stdout;

#ifdef PRINT_TO_FILE
    out = fopen("client2.log", "w");
    if (!out) {
        error_exit("Unable to open file");
    }
#endif

    for (port = 0; port < MAX_PORTS; port++) {
        struct sockaddr_in tcp_server_addr;
        if (findOpenPort(port + TCP_PORT, &tcp_server_addr) < 0) {
            error_exit("An open port could not be found");
        }
        
        printf("Connecting to port %d...\n", port + TCP_PORT);
        ports[port].sockfd = connectToPort(&tcp_server_addr);
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
                pthread_create(&thread_id[port], NULL, readFromPort, (void *)&ports[port]);
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
* @brief    Changes behavior of output 1 based on value from output 3
*
* @param	data - a pointer to the value from output 3
* @param	sockfd - socket file descriptor
* @param	udp_server_addr - the structure describing an Internet socket address
*
* @return	None
*
* @note		None
*
**************************************************************************/
static inline void changeBehavior(char *data, int udp_sokfd, struct sockaddr_in *udp_server_addr) {
    static int last_value = 3;

    if (data[1] != '-') {
        int current_value = atoi(data);
        
        if (last_value != current_value) {
            uint16_t msg[MSG_MAX_SIZE];
            if (current_value >= 3) {
                // Set the frequency of server output 1 to 1Hz and amplitude to 8000
                int size = makeMessage(WRITE, CHANNEL_1, FREQUENCY, 1000, msg);
                sendMessage(udp_sokfd, udp_server_addr, msg, size);
                size = makeMessage(WRITE, CHANNEL_1, AMPLITUDE, 8000, msg);
                sendMessage(udp_sokfd, udp_server_addr, msg, size);
            } else {
                // Set the frequency of server output 1 to 2Hz and amplitude to 4000
                int size = makeMessage(WRITE, CHANNEL_1, FREQUENCY, 2000, msg);
                sendMessage(udp_sokfd, udp_server_addr, msg, size);
                size = makeMessage(WRITE, CHANNEL_1, AMPLITUDE, 4000, msg);
                sendMessage(udp_sokfd, udp_server_addr, msg, size);
            }
            last_value = current_value;
        }
    }
}

/**************************************************************************/
/**
*
* @brief    Creates UDP message
*
* @param	op - operation
* @param	obj - object
* @param	prop - property
* @param	val - value for write opperations
* @param	[out] msg - the array into which the message will be placed
*
* @return	size of the message in bytes
*
* @note		None
*
**************************************************************************/
int makeMessage(enum operation_e op, enum object_e obj, enum property_e prop, uint16_t val, uint16_t *msg) {
    int size = 0;

    if (msg == NULL) {
        return 0;
    }

    msg[size++] = (uint16_t)op;
    msg[size++] = (uint16_t)obj;
    msg[size++] = (uint16_t)prop;

    if (op == WRITE) {
        msg[size++] = (uint16_t)val;
    }

    return size * sizeof(uint16_t);
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
* @param	[out] tcp_server_addr - the structure describing an Internet socket address
*
* @return	0 if given port is found, otherwise -1
*
* @note		None
*
**************************************************************************/
static int findOpenPort(unsigned int port_number, struct sockaddr_in *tcp_server_addr) {
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
            tcp_server_addr->sin_family = AF_INET;
            tcp_server_addr->sin_port = htons(local_port);
            tcp_server_addr->sin_addr.s_addr = htonl(local_ip_hex);

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
* @param	tcp_server_addr - a pointer to the structure describing an Internet socket address
*
* @return	file descriptor
*
* @note		None
*
**************************************************************************/
static int connectToPort(struct sockaddr_in *tcp_server_addr) {
    int sockfd;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error_exit("Socket creation failed");
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)tcp_server_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("Connection failed");
        close(sockfd);
    }

    // TODO: Adjust the timeout to get a balance between correct output and precise time interval
    // Set read timeout to 10ms
    struct timeval timeout = {0, 10000};
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
static void* readFromPort(void *args) {
    struct port_t *port = (struct port_t *)args;

    int bytes_received = recv(port->sockfd, port->buffer, sizeof(port->buffer) - 1, 0);
    if (bytes_received > 0) {
        // Replace '\n' with '\0'
        port->buffer[bytes_received - 1] = '\0';
        // Handle negative and positive values accordingly
        if (bytes_received >= 5 && port->buffer[bytes_received - 5] == '-'){
            port->data_ptr = &port->buffer[bytes_received - 5];
        } else {
            port->data_ptr = &port->buffer[bytes_received - 4];
        }
    } else {
        // Set value as '--' if no data received
        port->buffer[0] = port->buffer[1] = '-';
        port->buffer[2] = '\0';
        port->data_ptr = &port->buffer[0];
    }

    pthread_exit(NULL);
}