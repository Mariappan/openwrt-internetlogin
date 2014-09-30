# build mwget executable when user executes "make"
mwget: mwget.o
	$(CC) $(LDFLAGS) mwget.o -o mwget

mwget.o: mwget.c
	$(CC) $(CFLAGS) -c mwget.c

# remove object files and executable when user executes "make clean"
clean:
	rm *.o mwget
