main: makefile https_server.c hash_functions.c send_status.c connection_helpers.c
	gcc -o https_server https_server.c hash_functions.c send_status.c connection_helpers.c -lcrypto -lssl