# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2

# Source and object files
SRCS = main.c wifi_ctrl.c display.c
OBJS = $(SRCS:.c=.o)

# Executable name
TARGET = wifi-manager

# Default rule
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
