# Compiler and flags
CC = gcc
CFLAGS = -Wall -I./lib

# Paths
LIB_DIR = lib
TASK1_DIR = task1
TASK2_DIR = task2
UTILITIES_DIR = utilities

# Libraries
LIB_CLIENT = $(LIB_DIR)/client_lib.o

# All target to build everything
all: client1 client2 tcp_logger udp_logger

# Build client_lib.o
$(LIB_CLIENT): $(LIB_DIR)/client_lib.c $(LIB_DIR)/client_lib.h
	$(CC) $(CFLAGS) -c $< -o $@

# Separate targets
client1: $(TASK1_DIR)/client1
client2: $(TASK2_DIR)/client2
tcp_logger: $(UTILITIES_DIR)/tcp_logger
udp_logger: $(UTILITIES_DIR)/udp_logger

# Build client1
$(TASK1_DIR)/client1: $(TASK1_DIR)/client1.c $(LIB_CLIENT)
	$(CC) $(CFLAGS) $^ -o $@

# Build client2
$(TASK2_DIR)/client2: $(TASK2_DIR)/client2.c $(LIB_CLIENT)
	$(CC) $(CFLAGS) $^ -o $@

# Build tcp_logger
$(UTILITIES_DIR)/tcp_logger: $(UTILITIES_DIR)/tcp_logger.c $(LIB_CLIENT)
	$(CC) $(CFLAGS) $^ -o $@

# Build udp_logger
$(UTILITIES_DIR)/udp_logger: $(UTILITIES_DIR)/udp_logger.c $(LIB_CLIENT)
	$(CC) $(CFLAGS) $^ -o $@

# Clean the build
clean:
	rm -f $(LIB_CLIENT) $(TASK1_DIR)/client1 $(TASK2_DIR)/client2 $(UTILITIES_DIR)/tcp_logger $(UTILITIES_DIR)/udp_logger

# Phony targets
.PHONY: all clean client1 client2 tcp_logger udp_logger