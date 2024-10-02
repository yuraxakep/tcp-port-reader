/*****************************************************************************/
/**
*  Brief: 	Contains definition for the functions needed by client apps.
*
*  Created: 02.10.2024
*  Author: 	Yurii Shenbor
*
******************************************************************************/

/***************************** Include Files ********************************/

#include "client_lib.h"

/************************** Constant Definitions *****************************/


/**************************** Type Definitions *******************************/


/************************** Function Prototypes ******************************/


/************************** Variable Definitions *****************************/

static int last_value = 3;

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
void error_exit(const char *msg) {
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
void changeBehavior(char *data, int udp_sokfd, struct sockaddr_in *udp_server_addr) {
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
int findOpenPort(unsigned int port_number, struct sockaddr_in *tcp_server_addr) {
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
int connectToPort(struct sockaddr_in *tcp_server_addr, int timeout_ms) {
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
    struct timeval timeout = {0, timeout_ms};
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
* @return	None
*
* @note		None
*
**************************************************************************/
void readFromPort(struct port_t *port) {
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
}

/**************************************************************************/
/**
*
* @brief    Reads data from the given port using separate thread
*
* @param	args - contains the file descriptor and buffer for the given port
*
* @return	None
*
* @note		None
*
**************************************************************************/
void* readFromPortInThread(void *args) {
    readFromPort((struct port_t *)args);
    pthread_exit(NULL);
}