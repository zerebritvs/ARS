// Practica tema 6, Pages Lopez Juan Antonio

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h> // Para definir sockaddr_in
#include <arpa/inet.h>
#include <netdb.h>	// Para usar inet_aton()
#include <errno.h>	// Para usar errno y perror()
#include <stdio.h>	// Para usar perror()
#include <stdlib.h>	// Para usar EXIT_FAILURE
#include <sys/unistd.h> // Para usar close()
#undef DEBUG

int main(int argc, char *argv[]){
	// Variables para el control de errores
	int socketError;
        int bindError;
        int connectError;
	int inet_atonError;
	int recvError;
	int shutdownError;
	
	struct servent *service;
	
	struct sockaddr_in myaddr;
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = INADDR_ANY;
	myaddr.sin_port = 0;

	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;	
	struct in_addr ip32;

	
	// 1. Convierte ip en cadena a numero de 32 bits
	inet_atonError = inet_aton(argv[1], &ip32);
	dest_addr.sin_addr = ip32;

	if(inet_atonError==0){
		printf("Error al convertir la direccion ip parametro a numero de 32 bits");
		exit(EXIT_FAILURE);
	}
	
	// 2.1 Si no se especifica el puerto
	if(argc==2){

		// Obtengo el numero de puerto asociado al servicio DAYTIME TCP
		service = getservbyname("daytime", "tcp");
		dest_addr.sin_port = service->s_port;

		if(!service){
			printf("No se ha encontrado una entrada para el servicio daytime.\n");
			exit(EXIT_FAILURE);
		}
		
	// 2.2 Si el puerto si que se especifica	
	}else{
		int puertoDest;
		sscanf(argv[3], "%d", &puertoDest);
		dest_addr.sin_port = htons(puertoDest);		
	}
	
	
	// 3. Creacion del socket TCP
	socketError = socket(AF_INET, SOCK_STREAM, 0);

	if(socketError<0){
		perror("socket()");
		exit(EXIT_FAILURE);
	}
#ifdef DEBUG
	printf("Socket creado correctamente...\n");
#endif
	// 4. Enlace del socket anterior con direccion ip local
	bindError = bind(socketError, (struct sockaddr*)&myaddr, sizeof(myaddr));

	if(bindError<0){
		perror("bind()");
		exit(EXIT_FAILURE);
	}
#ifdef DEBUG
	printf("Bind hecho correctamente...\n");
#endif	
	// 5. Conecto un descriptor socket TCP con el servidor
	connectError = connect(socketError, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
	if(connectError<0){
		perror("connect()");
		exit(EXIT_FAILURE);
	}
#ifdef DEBUG
	printf("Connect hecho correctamente...\n");
#endif
	// 6. Recibo datos del servidor y los imprimo por pantalla
	char msg_servidor[100];
	recvError = recv(socketError, msg_servidor, sizeof(msg_servidor), 0);

	if(recvError<0){
		perror("recv()");
		exit(EXIT_FAILURE);
	}

	printf("%s", msg_servidor);

#ifdef DEBUG
	printf("Recv 1 hecho correctamente...\n");
#endif
	// 7. Notifico al servidor que quiero cerrar la conexion
	shutdownError = shutdown(socketError, SHUT_RDWR);

	if(shutdownError<0){
		perror("shutdown()");
		exit(EXIT_FAILURE);
	}
#ifdef DEBUG
	printf("Shutdown hecho correctamente...\n");
#endif
	
	// 8. Confirmo que el servidor ha dado por finalizada la conexion
	recvError = recv(socketError, msg_servidor, sizeof(msg_servidor), 0);

	if(recvError<0){
		perror("recv()");
		exit(EXIT_FAILURE);
	}
#ifdef DEBUG
	printf("Recv 2 hecho correctamente...\n");
#endif
	// 9. Cierro el socket TCP
	close(socketError);
	

	return 0;

}
