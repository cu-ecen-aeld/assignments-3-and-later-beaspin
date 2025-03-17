CC := gcc
CFLAGS := -Wall -Wextra -pthread
TARGET := aesdsocket
SRC := aesdsocket.c
OBJ := $(SRC:.c=.o)
CROSS_COMPILE ?=
OUTPUT_DIR := server

all: $(TARGET)

$(TARGET): $(SRC)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o $(TARGET) $^

clean:
	rm -f $(TARGET)
	rm -f $(OBJ)

.PHONY: all clean
