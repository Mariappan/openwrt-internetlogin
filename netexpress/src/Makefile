# build netexpress executable when user executes "make"
netexpress: netexpress.o
	$(CC) $(LDFLAGS) netexpress.o -o netexpress

netexpress.o: netexpress.c
	$(CC) $(CFLAGS) -c netexpress.c

# remove object files and executable when user executes "make clean"
clean:
	rm *.o netexpress
