# Consistent hashing library
# pyke@dailymotion.com - 05/2009

test: libchash_test libchash.o libchash.so.1.0.0
	./libchash_test

libchash.a: libchash.c libchash.h
	gcc -Wall -O3 -c libchash.c -o libchash.o
	ar r libchash.a libchash.o

libchash.so.1.0.0: libchash.c libchash.h
	gcc -Wall -O3 -fPIC -c libchash.c -o libchash.o
	gcc -shared -Wl,-soname,libchash.so.1 -o libchash.so.1.0.0 libchash.o

libchash_test: libchash_test.c libchash.a
	gcc -Wall -O3 -o libchash_test libchash_test.c libchash.a -lm

clean:

distclean: debclean
	rm -f *.o *.a *.so.* libchash_test

deb:
	debuild -i -us -uc -b

debclean:
	debuild clean
