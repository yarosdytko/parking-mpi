
CC = mpicc

CFLAGS  = -g -Wall
TARGET = parking coche camion

all: $(TARGET)

$(TARGET): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	$(RM) $(TARGET)
