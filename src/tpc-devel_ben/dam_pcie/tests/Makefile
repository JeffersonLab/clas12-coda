CFLAGS=-I../common -Wall -std=c99

.PHONY: all
##all: ioctl dma_mmap test1    # mmap dammem
all: test1 ioctl

%.o: %.c
		$(CC) -c -o $@ $< $(CFLAGS)

test1: test1.o sampaLib.o
		$(CC) -o $@ $^ $(CFLAGS) -L../libdam -ldam

ioctl: ioctl.o
		$(CC) -o $@ $^ $(CFLAGS)

#dma_mmap: dma_mmap.o
#		$(CC) -o $@ $^ $(CFLAGS) -L../libdam -ldam

#mmap: mmap.o
#		$(CC) -o $@ $^ $(CFLAGS) -L../libdam -ldam

#dammem: dammem.o
#		$(CC) -o $@ $^ $(CFLAGS) -L../libdam -ldam

clean:
	rm -f *.o test1 dma_mmap ioctl
