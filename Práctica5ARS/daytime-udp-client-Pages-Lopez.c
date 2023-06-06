// Practica tema 5, Pages Lopez Juan Antonio

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

	int socketError;
        int bindError;
        int sendtoError;
	int inet_atonError;
	int recvfromError;
	
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

		// Obtengo el numero de puerto asociado al servicio DAYTIME UDP
		service = getservbyname("daytime", "udp");
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
	
	
	// 3. Creacion del socket UDP
	socketError = socket(AF_INET, SOCK_DGRAM, 0);

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
	// 5. Envia datos a una ip especifica a traves de un socket UDP
	char *msg_cliente = malloc(sizeof(msg_cliente));
	sendtoError = sendto(socketError, msg_cliente, sizeof(msg_cliente), 0, (struct sockaddr*)&dest_addr, (socklen_t)sizeof(dest_addr));

	if(sendtoError<0){
		perror("sendto()");
		exit(EXIT_FAILURE);
	}
#ifdef DEBUG
	printf("Mensaje enviado al servidor correctamente...\n");
#endif
	// 6. Recibo datos del servidor y los imprimo por pantalla
	socklen_t tamVar;
	char msg_servidor[100];
	recvfromError = recvfrom(socketError, msg_servidor, sizeof(msg_servidor), 0, (struct sockaddr*)&dest_addr, &tamVar);

	if(recvfromError<0){
		perror("recvfrom()");
		exit(EXIT_FAILURE);
	}
	printf("%s\n", msg_servidor);

	// 7. Cierro el socket UDP
	close(socketError);
	free(msg_cliente);
	

	return 0;

}
