
CC = gcc
CFLAGS = -O2 -g
LDFLAGS =

.PHONY: all clean

all: conv_spr

clean:
	rm -f *~ *.o conv_spr spr/*.h

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

CONV_SPR_OBJS = conv_spr.o bitmap.o gz_open.o

conv_spr: $(CONV_SPR_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(CONV_SPR_OBJS)

