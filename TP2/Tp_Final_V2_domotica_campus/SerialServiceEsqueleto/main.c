#include "main.h"

//Señales
struct sigaction sa1, sa2;
static bool running = true; //me permite las detenciones con las señales
static uint8_t fd; //file descriptor tcp
// Buffer del puerto serial
static char rxSerialBuffer [SERIAL_BUFFER_LENGTH_];

//internal functions
static void* thread_serial_handler(); // Tarea de puerto serial
	//Señales
static void signal_handler(int signal); 
static void signal_init();
	//Bloqueo/Desbloqueo de señales
static void block_signals();
static void unblock_signals();

int main(void)
{
	signal_init();

	// Creamos la task de comunicacion serial
	pthread_t threadSerial = pthread_create(&threadSerial, NULL, thread_serial_handler, NULL);	
	
	// TCP IP socket server implementation
	socklen_t addr_len;
	struct sockaddr_in clientaddr;
	struct sockaddr_in serveraddr;

	int tcp_rx_bytes; //cant de bytes recibidos (TCP)
	bool isConnected = false; //esta conectado (TCP)

	// Creamos el socket  (1)
	int socket_tcp = socket(AF_INET,SOCK_STREAM, 0); // 0 = TCP ; 1 = UDP 

	// setsocketop	(2)
	if (setsockopt(socket_tcp, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
		perror(" (x) Set socket error!"); 
	}
	bzero((char*) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_port = htons(port);
	serveraddr.sin_family = AF_INET;
	
	
	if(inet_pton(serveraddr.sin_family, local_host, &(serveraddr.sin_addr))< 1)
	{
		fprintf(stderr," (x) Direccion IP invalida!\r\n");
		return 1;
	}

	// bind	(3)
	if (bind(socket_tcp, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0) //-1 = error
	{
		close(socket_tcp);
		perror(" (x) Bind error!\r\n");
		return 1;
	}
	
	// listen  ----->  connect(Server)(4)
	if (listen (socket_tcp, 128) < 0)
  	{
    	perror(" (x) Listen error!\r\n");
    	exit(1);
  	}

	while(running)
	{
		// accept	(5)
		addr_len = sizeof(struct sockaddr_in);
		printf("Esperando conexion...\r\n");
	 	if ( (fd = accept(socket_tcp, (struct sockaddr *)&clientaddr,&addr_len)) < 0 )
		{
			perror(" (x) Accept error!\r\n");
			exit(1);
		}	
		char ipClient[32];
		inet_ntop(serveraddr.sin_family , &(clientaddr.sin_addr), ipClient, sizeof(ipClient));
		printf  ("Server ---> Conexion con %s\n",ipClient);
		isConnected = true;	
		// send/recv   ------> Server(send/recv)	(6)
		while(isConnected)
		{	
			if( (tcp_rx_bytes = read(fd, rxSerialBuffer, SERIAL_BUFFER_LENGTH_)) < 0 )	// -1 = error
			{
				if(running)
				{
					perror("(x) Read error!\r\n");
					exit(1);
				}
				break;
			}
			else if(tcp_rx_bytes == 0)
			{
				isConnected = false;
			}
			else
			{
				rxSerialBuffer[tcp_rx_bytes]=0x00;
				printf("Recibido: %s\n",rxSerialBuffer);
				serial_send(rxSerialBuffer, SERIAL_BUFFER_LENGTH_);
			}			
		}
		if(running)
		{
			printf(" (x) Conexion perdida!\r\n");
		}
		close(fd);
	}
	
	close(fd);
	exit(EXIT_SUCCESS);
	return 0;
}
/* Esta tarea se comunica por puerto serial con el emulador para el intercambio de datos con el dispositvo*/
static void* thread_serial_handler()
{
	if(0 != serial_open(1, 115200)) //Abiendo el puerto serial
	{
		perror(" (x) Error al abrir el puerto, puede estar en uso...\r\n");
		exit(1);
	}

	int16_t bytes_rx;

	while(running) //no me conviene el while true sino no voy a poder cerrar el puerto....
	{
		if( bytes_rx = serial_receive(rxSerialBuffer, SERIAL_BUFFER_LENGTH_) > 0)
		{
			write(fd, rxSerialBuffer, sizeof(rxSerialBuffer));
		}	
		usleep(1000); //Esto es para que no quede la cpu al 100%
	}
	
	printf("Cerrando puerto serial\r\n");
	serial_close();
}
static void signal_init()
{
    sa1.sa_handler = signal_handler;
    sa1.sa_flags = 0;
    sigemptyset(&sa1.sa_mask);
    
    if(sigaction(SIGINT, &sa1, NULL) < 0){
        perror(" (x) Error al crear señal");
        exit(1);
    }

	sa2.sa_handler = signal_handler;
    sa2.sa_flags = 0;
    sigemptyset(&sa2.sa_mask);

    if(sigaction(SIGTERM, &sa2, NULL) < 0){
        perror(" (x) Error al crear señal");
        exit(1);
    }
}
static void signal_handler(int signal)
{
	running = false;
	printf(" Señal detectada: %i\r\n", signal);
}