# Define variables
CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Werror
LDFLAGS ?= -pthread
TARGET ?= aesdsocket
SRC := aesdsocket.c
OBJ := $(SRC:.c=.o)

# Cross-compilation support
ifeq ($(CROSS_COMPILE),)
    CROSS_COMPILE := 
endif

#CC

# Default target to build the aesdsocket application
all: $(TARGET)

# Rule to build the aesdsocket application
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(MY_LIBS) $(OBJ) 

# Rule to build object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target to remove the aesdsocket application and .o files
clean:
	rm -f $(TARGET) $(OBJ)

# Phony targets
.PHONY: all clean

