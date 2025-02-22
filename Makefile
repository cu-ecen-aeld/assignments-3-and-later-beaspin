CROSS_COMPILE ?=
CC = $(CROSS_COMPILE)gcc

CFLAGS = -Wall -Wextra -pedantic -std=c99

TARGET = writer
SRC = finder-app/writer.c
OBJ = finder-app/writer.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
    $(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

finder-app/%.o: finder-app/%.c
    $(CC) $(CFLAGS) -c $< -o $@

clean:
    rm -f $(TARGET) $(OBJ)
