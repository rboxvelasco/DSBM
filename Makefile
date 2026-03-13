all:
	gcc -lwiringPi -o code code.c

clean:
	rm code
