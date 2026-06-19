all:
	cc -O2 -Wall aecat.c -o aecat
install: all
	cp aecat /usr/local/bin
uninstall:
	rm /usr/local/bin/aecat
clean:
	rm aecat
