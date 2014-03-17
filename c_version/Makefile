clean:
	rm -f parser sanitizer_util.o out*.gpx
sanitizer_util.o:
	gcc -Wall -I/usr/include/libxml2/ -c sanitizer_util.c
all: clean sanitizer_util.o
	gcc -Wall -lm -lxml2 -I/usr/include/libxml2/ -o parser sanitizer_util.o parser.c
run: all
	./parser sample.gpx

install: all
	cp parser ../../sanitize

valgrind: all
	valgrind ./parser sample.gpx
