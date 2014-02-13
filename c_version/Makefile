clean:
	rm -f parser linkedlist.o out*.gpx
linkedlist.o:
	gcc -Wall -c linkedlist.c
all: clean linkedlist.o
	gcc -ggdb -Wall -lm -lxml2 -I/usr/include/libxml2/ -o parser linkedlist.o parser.c
run: all
	./parser sample.gpx

install: all
	cp parser ../../sanitize
