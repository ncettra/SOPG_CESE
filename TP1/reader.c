#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#define FIFO_NAME "myfifo"
#define BUFFER_SIZE 300

const char* DATA_HEADER = "DATA:";
const char* SIGNAL_HEADER = "SIGN:";

int main(void)
{

    FILE *fileData;
    FILE *fileSignals;
    
    fileData = fopen("log.txt","a+"); //a+ = si no existe creo, si existe abro y escribo al final (append)
    fileSignals = fopen("signals.txt","a+");

    if(fileData==NULL){
        printf("(x) No se pudo abrir el archivo log.txt, se creara nuevo.\n");
        return 1;
    }
    else{
        printf("-Archivo log.txt abierto\n");
    }

    if(fileSignals==NULL){
        printf("(x) No se pudo abrir el archivo signals.txt, se creara nuevo.\n");
        return 1;
    }
    else{
        printf("-Archivo signals.txt abierto\n");
    }

	uint8_t inputBuffer[BUFFER_SIZE];
	int32_t bytesRead, returnCode, fd;
    
    if ( (returnCode = mknod(FIFO_NAME, S_IFIFO | 0666, 0) ) < -1  )    {
        printf("(x) Error creando fifo: %d\n", returnCode);
        exit(1);
    }
    
	printf("- Esperando al writer...\n");
	if ( (fd = open(FIFO_NAME, O_RDONLY) ) < 0 )    {
        printf("(X) Error abriendo fifo: %d\n", fd);
        exit(1);
    }
        
	printf("-Esperando mensajes...\n");
	do
	{
		if ((bytesRead = read(fd, inputBuffer, BUFFER_SIZE)) == -1)        {
			perror("read");
        }
        else {

			inputBuffer[bytesRead] = '\0';
			printf("reader: read %d bytes: \"%s\"\n", bytesRead, inputBuffer);

            if(strncmp(inputBuffer,DATA_HEADER,strlen(DATA_HEADER)) == 0){
                fprintf(fileData,"%s\n", inputBuffer);    
                printf(" - Guardado en log\n");
                fflush(fileData); //fflush me permite guardar el archivo sin necesidad de cerrarlo, esto es por si se cierra inesperadamente el reader
                
            } 
            else if (strncmp(inputBuffer,SIGNAL_HEADER,strlen(SIGNAL_HEADER)) == 0) {
                fprintf(fileSignals,"%s\n", inputBuffer);    
                printf(" - Guardado en signals\n");
                fflush(fileSignals);
            }            
            else{
                printf(" (x) Descartado por no respetar formato\n");
            }

		}
	}

	while (bytesRead > 0);

    fclose(fileData);
    fclose(fileSignals);
    
	return 0;
}