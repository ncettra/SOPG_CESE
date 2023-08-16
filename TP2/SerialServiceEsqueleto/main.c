/*TODO:
	
	Correcciones TP2:

	1) controlar error en : pthread_create, socket
		-listo, linea 46 verifico el retorno de pthread_create 

	2) antes de lanzar el thread, tenes que bloquear las signals, para asegurarte de que las signals lleguen siempre al hilo ppal. (fijate que se vio en la clase 6)
		-listo, linea 44 y 54, se colocaron 2 funciones, una es para bloquear señal y otra para desbloquear

	3) en el thread, cuando recibis por el puerto serie y antes de hacer el envio por el socket, tenes que comprobar que haya una conexion establecida sino puede ser que hagas un write con un "fd" invalido. Usa el flag isConnected para hacer este chequeo
		-listo, para ello isConnected se hizo global (linea 23) y posteriormente fue anidada en la condicion de lectura en thread_serial_handler(), linea 173
	
	4) como este flag isConnected lo vas a leer desde un thread y escribir desde otro, podes usar un mutex para proteger el acceso a esta variable.
		-listo, el mutex fue agregado en cada escritura de isConnected, pero considero que no era necesario, dado que en thread era solo para lectura.
	
	5) la deteccion de error en el read o en el accept (que pasa cuando recibis la signal), deberian generar que se salga del loop ppal (no hacer un exit ahi nomas) y que salgas del proceso retornando por el main, alli, antes de salir, podes hacer el cierre de todos los recursos que tenes abiertos antes de terminar, incluyendo la llamada  a pthread_cancel y el join para esperar a que termine y el cierre de los sockets abiertos
		-listo, se aplico cancel, join y close del fd, lineas 114 y 135

	6) no se puede usar printf en el handler
		-listo, esta aplicado inecesariamente (si se quisiera imprimir se entiende que es con el write)

	7) la variable running tenia que ser del tipo volatile sig_atomic_t (ver este tema en clase 3)
		-listo, linea 31

	8) en serial_send no envies todo el buffer, envia solo la cantidad que recibiste, lo mismo cuando envias por el socket.
		-listo, se utilizo la funcion strlen para contar los caracteres en ambos casos 
*/


#include "main.h"

//Señales
struct sigaction sa1, sa2;
volatile sig_atomic_t running = true; //me permite las detenciones con las señales
bool isConnected = false; //esta conectado (TCP)
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

	//creo el mutex
	pthread_mutex_t mutexSem; //Igual tengo la duda de proteger isConnected, porque si bien es un recurso compartido, el thread lo usa solo para lectura

	//bloqueo las signals:
	block_signals();

	// Creamos la task de comunicacion serial
	pthread_t threadSerial;
	if( pthread_create(&threadSerial, NULL, thread_serial_handler, NULL)!=0)	
	{
		perror(" (x) Thread error!"); 
	}
	
	//desbloqueo la signals
	unblock_signals();

	// TCP IP socket server implementation
	socklen_t addr_len;
	struct sockaddr_in clientaddr;
	struct sockaddr_in serveraddr;

	int tcp_rx_bytes; //cant de bytes recibidos (TCP)
	

	// Creamos el socket  (1)
	int socket_tcp = socket(AF_INET,SOCK_STREAM, 0); // 0 = TCP ; 1 = UDP 

	// setsocketop	(2)
	if (setsockopt(socket_tcp, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
		perror(" (x) Set socket error!"); 
		return 1;
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
			//exit(1);
			pthread_cancel(threadSerial);
			pthread_join(threadSerial, NULL);
			close(fd);
		}	
		char ipClient[32];
		inet_ntop(serveraddr.sin_family , &(clientaddr.sin_addr), ipClient, sizeof(ipClient));
		printf  ("Server ---> Conexion con %s\n",ipClient);

		pthread_mutex_lock(&mutexSem);
		isConnected = true;
		pthread_mutex_unlock(&mutexSem);

		// send/recv   ------> Server(send/recv)	(6)
		while(isConnected)
		{	
			if( (tcp_rx_bytes = read(fd, rxSerialBuffer, SERIAL_BUFFER_LENGTH_)) < 0 )	// -1 = error
			{
				if(running)
				{
					perror("(x) Read error!\r\n");
					//exit(1);

					pthread_cancel(threadSerial);
					pthread_join(threadSerial, NULL);
					close(fd);
				}
				break;
			}
			else if(tcp_rx_bytes == 0)
			{
				pthread_mutex_lock(&mutexSem);
				isConnected = false;
				pthread_mutex_unlock(&mutexSem);
			}
			else
			{
				rxSerialBuffer[tcp_rx_bytes]=0x00;
				printf("Recibido: %s\n",rxSerialBuffer);
				serial_send(rxSerialBuffer, strlen(rxSerialBuffer));
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
		if( bytes_rx = serial_receive(rxSerialBuffer, SERIAL_BUFFER_LENGTH_) > 0 && isConnected)
		{
			write(fd, rxSerialBuffer, strlen(rxSerialBuffer));
		}	
		usleep(1000); //Esto es para que no quede la cpu al 100%
	}

	serial_close();
}
/* Señales */
static void signal_init()
{
    sa1.sa_handler = signal_handler;
    sa1.sa_flags = 0;
    sigemptyset(&sa1.sa_mask);
    
    if(sigaction(SIGINT, &sa1, NULL) < 0)
	{
        perror(" (x) Error al crear señal");
        exit(1);
    }

	sa2.sa_handler = signal_handler;
    sa2.sa_flags = 0;
    sigemptyset(&sa2.sa_mask);

    if(sigaction(SIGTERM, &sa2, NULL) < 0)
	{
        perror(" (x) Error al crear señal");
        exit(1);
    }
}
static void block_signals()
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
}
static void unblock_signals()
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}
static void signal_handler(int signal)
{
	running = false;
	printf(" Señal detectada: %i\r\n", signal);
}