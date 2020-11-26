all: sca_start sca_stop sca_reset sca_read sca_app

sca_start: sca_start.c
	gcc -o sca_start sca_start.c

sca_stop: sca_stop.c
	gcc -o sca_stop sca_stop.c

sca_reset: sca_reset.c
	gcc -o sca_reset sca_reset.c

sca_read: sca_read.c
	gcc -o sca_read sca_read.c

sca_app: sca_app.c
	gcc -o sca_app sca_app.c -lncurses

clean:
	rm -f sca_start sca_stop sca_reset sca_read sca_app
