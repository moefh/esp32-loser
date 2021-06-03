
CC = gcc
CFLAGS = -Wall -O2 -g
LDFLAGS =

.PHONY: all clean

all: conv_spr conv_bmp

clean:
	rm -f *~ *.o conv_spr conv_bmp spr/*.h

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

CONV_SPR_OBJS = conv_spr.o bitmap.o gz_open.o

conv_spr: $(CONV_SPR_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(CONV_SPR_OBJS)

CONV_BMP_OBJS = conv_bmp.o bmp.o

conv_bmp: $(CONV_BMP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(CONV_BMP_OBJS)

