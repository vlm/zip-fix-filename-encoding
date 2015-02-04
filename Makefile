
CFLAGS = -Wall -I/usr/local/include -I./libzip-0.7.1/lib

runzip: runzip.c UTF8String.c libzip.a
	$(CC) $(CFLAGS) runzip.c UTF8String.c -o runzip libzip.a -liconv -lz

libzip.a:
	cd libzip-0.7.1 && ./configure && make
	cp libzip-0.7.1/lib/.libs/libzip.a .

clean:
	rm -f runzip *.o *.a
	cd libzip-0.7.1 && make distclean
