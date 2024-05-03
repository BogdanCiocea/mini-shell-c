CC := gcc
CFLAGS := -Wall -Wextra -g

SRC := main.c
TARGET := mini_shell

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
