main: https_server.c hash_functions.c send_status.c makefile
	gcc -o https_server https_server.c hash_functions.c send_status.c -lcrypto -lssl