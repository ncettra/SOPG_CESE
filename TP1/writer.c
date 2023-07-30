#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>

#define FIFO_NAME "myfifo"
#define BUFFER_SIZE 300

const char* DATA_HEADER = "DATA:";

uint32_t bytesWrote;
int32_t returnCode, fd;

void signalHandler(int signum){
    if(signum==SIGUSR1){
        printf(" - SINGNAL 1 DETECTADO!\n");
        const char* dataToSend = "SIGN:1\n";
		if ((bytesWrote = write(fd, dataToSend, strlen(dataToSend)-1)) == -1) {
			perror("write");
        }
        else {
			printf(" - writer: se enviaron %d bytes\n", bytesWrote);
        }
    }
    else if (signum==SIGUSR2){
        printf(" - SINGNAL 2 DETECTADO!\n");
        const char* dataToSend = "SIGN:2\n";
		if ((bytesWrote = write(fd, dataToSend, strlen(dataToSend)-1)) == -1) {
			perror("write");
        }
        else {
			printf(" - writer: se enviaron %d bytes\n", bytesWrote);
        }
    }
}


int main(void)
{
    pid_t pid = getpid();
    printf("-Writer inicializado el pid es %d\n\n",pid);

    char outputBuffer[BUFFER_SIZE];
    signal(SIGUSR1, signalHandler);
    signal(SIGUSR2, signalHandler);

    if ( (returnCode = mknod(FIFO_NAME, S_IFIFO | 0666, 0) ) < -1 )    {
        printf("(x) Error al crear FIFO: %d\n", returnCode);
        exit(1);
    }

	printf("-Esperando escuchador...\n");
	if ( (fd = open(FIFO_NAME, O_WRONLY) ) < 0 )    {
        printf("(x) Error al abrir FIFO: %d\n", returnCode);
        exit(1);
    }
    
    
	
    printf("-Escuchador detectado!\n");
    printf("__________Instrucciones___________\n");
    printf(" Señal 1 cmd: kill -s SIGUSR1 %d\n",pid);
    printf(" Señal 2 cmd: kill -s SIGUSR2 %d\n",pid);
    printf(" Para enviar informacion solo escribe y presiona enter\n");
    printf("__________________________________\n");


	while (1)	{
		fgets(outputBuffer, BUFFER_SIZE, stdin);

        char dataToSend[BUFFER_SIZE+strlen(DATA_HEADER)] ;
        sprintf(dataToSend,"%s%s",DATA_HEADER,outputBuffer);

		if ((bytesWrote = write(fd, dataToSend, strlen(dataToSend)-1)) == -1) {
			perror("write");
        }
        else {
			printf(" - writer: se enviaron %d bytes\n", bytesWrote);
        }
	}
	return 0;
}