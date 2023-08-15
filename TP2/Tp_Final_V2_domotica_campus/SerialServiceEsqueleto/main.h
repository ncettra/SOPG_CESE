#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include "SerialManager.h"	// funciones uart 

#define SERIAL_BUFFER_LENGTH_		128

// Configuracion del socket
const uint16_t port = 10000;
const char* local_host = "127.0.0.1";