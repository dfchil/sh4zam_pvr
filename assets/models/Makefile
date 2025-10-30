# Makefile for compiling tea_brewer.c

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu23

# Target and source files
TARGET = tea_brewer
SRCS = tea_brewer.c

# Build target
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

# Clean up build files
clean:
	rm -f $(TARGET)

.PHONY: all clean