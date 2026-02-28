CC=gcc
CFLAGS=-Wall
TARGET=wifimgr

OBJ=main.o netlink.o scan.o connect.o display.o

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $(OBJ)

%.o: %.c wifi.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o $(TARGET)
