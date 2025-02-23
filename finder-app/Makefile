CROSS_COMPILE ?=
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c99

TARGET = finder-app/writer
ROOT_TARGET = writer
SRC = finder-app/writer.c
OBJ = writer.o

.PHONY: all clean

all: $(TARGET) $(ROOT_TARGET)

$(TARGET): $(OBJ)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

$(OBJ): writer.c
	$(CC) $(CFLAGS) -c $< -o $@

$(ROOT_TARGET): $(TARGET)
	cp $(TARGET) $(ROOT_TARGET)

clean:
	rm -f $(TARGET) $(OBJ) $(ROOT_TARGET) finder-app/writer || true
