CC = gcc -std=gnu99 -O -Wall

.PHONY : test

daGLiclock: *.c
	$(CC) main.c daGLiclock.c -o daGLiclock -lGL `sdl-config --cflags --libs`

test:
	$(CC) daGLiclock.c test.c -o test -lcunit -lm
	./test | spc --config=spcrc
