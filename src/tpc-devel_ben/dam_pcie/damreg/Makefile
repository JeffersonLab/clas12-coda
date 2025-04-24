CFLAGS=-I../common -L../libdam -ldam

%.o: %.c 
		$(CC) -c -o $@ $< $(CFLAGS)

damreg: damreg.o 
		$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o damreg 
