main: https_server.c makefile
	gcc -o https_server https_server.c -lcrypto -lssl