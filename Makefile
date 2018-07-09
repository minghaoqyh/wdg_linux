test:watch_dog.o uart.o
	gcc -m32 -static watch_dog.o uart.o -o test

watch_dog.o:watch_dog.c
	gcc -m32 -static -c watch_dog.c

uart.o:uart.c uart.h
	gcc -m32 -static -c uart.c

.PHONY:clean
clean:
	-rm test *.o
