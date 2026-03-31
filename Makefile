CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -std=c99
LDFLAGS = -lpthread -lm

TARGET = iptv-player
SOURCES = src/main.c src/igmp_multicast.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)
    @echo "✅ Build successful"

$(TARGET): $(OBJECTS)
    $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c src/igmp_multicast.h config.h
    $(CC) $(CFLAGS) -c $< -o $@

clean:
    rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
    sudo ./$(TARGET) -a 239.1.1.1 -p 1234 -i eth0

.PHONY: all clean run
