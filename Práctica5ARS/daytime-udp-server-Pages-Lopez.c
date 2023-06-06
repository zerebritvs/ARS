// Practica tema 5, Pages Lopez Juan Antonio

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h> // Para definir sockaddr_in
#include <arpa/inet.h>
#include <netdb.h>	// Para usar getservbyname()
#include <errno.h>	// Para usar errno y perror()
#include <stdio.h>	// Para usar perror()
#include <stdlib.h>	// Para usar EXIT_FAILURE
#include <sys/unistd.h> // Para usar close() y gethostname()
#include <string.h>	// Para usar strcat()
#undef DEBUG


int main(int argc, char *argv[]){

	int socketServer;
	int bindServer;
	int recvfromServer;
	int sendtoServer;
	
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;

	struct sockaddr_in client_addr;
	socklen_t socketLen = sizeof(client_addr);

	struct servent *service;
	
	char hostname[100];
	char bufferServer[512];
	
	FILE *fich;
	socklen_t tamVar;
	
	// 1. Obtengo el nombre del host del servidor
	if(gethostname(hostname, sizeof(hostname))<0){
		perror("gethostname()");
		exit(EXIT_FAILURE);
	}
	
	// 2.1 Si no se especifica el puerto
	if(argc == 1){
		
		service = getservbyname("daytime", "udp");
		server_addr.sin_port = service->s_port;

		if(!service){
			printf("No se ha encontrado una entrada para el servicio daytime. \n");
			exit(EXIT_FAILURE);
		}

	// 2.2 Si el puerto si que se especifica
	}else{
		int puertoServidor;
		sscanf(argv[2], "%d", &puertoServidor);
		server_addr.sin_port = htons(puertoServidor);
	}
	
	// 3. Creacion del socket UDP
	socketServer = socket(AF_INET, SOCK_DGRAM, 0);

	if(socketServer<0){
		perror("socket()");
		exit(EXIT_FAILURE);
	}
#ifdef DEBUG
	printf("Socket creado correctamente...\n");
#endif
	// 4. Enlace del socket anterior con direccion local
	bindServer = bind(socketServer, (struct sockaddr*)&server_addr, sizeof(server_addr));
	
	if(bindServer<0){
		perror("bind()");
		exit(EXIT_FAILURE);
	}
#ifdef DEBUG
	printf("Bind hecho correctamente...\n");
#endif
	
	// 5. El servidor se queda escuchando para recibir peticiones
	while(1){	

		printf("Esperando mensajes...\n\n");

		// 6. Recibo datos del cliente
		recvfromServer = recvfrom(socketServer, bufferServer, sizeof(bufferServer), 0, (struct sockaddr*)&client_addr, &tamVar);
		if(recvfromServer<0){
			perror("recvfrom()");
			exit(EXIT_FAILURE);
		}
#ifdef DEBUG
		printf("Mensaje recibido correctamente...\n");
#endif
		// 7. Obtengo fecha y hora del sistema
		char date[30] = "";
		system("date > /tmp/tt.txt");
		fich = fopen("/tmp//tt.txt", "r");

		if(fgets(date, sizeof(date), fich) == NULL){
			printf("Error en system(), en fopen(), o en fgets()\n");
			exit(EXIT_FAILURE);
		}
#ifdef DEBUG
		printf("La fecha y hora son: %s\n", date);
#endif	
		char bufferRespuesta[100] = "";

		strcat(bufferRespuesta, hostname);
		strcat(bufferRespuesta, ": ");
		strcat(bufferRespuesta, date);
		
#ifdef DEBUG
		printf("hostname: %s\n", hostname);
		printf("date: %s\n", date);
		printf("Mensaje: %s\n", bufferRespuesta);
#endif	

		// 8. Envio la cadena obtenida al cliente
		sendtoServer = sendto(socketServer, bufferRespuesta, sizeof(bufferRespuesta), 0, (struct sockaddr*) &client_addr, socketLen);
		if(sendtoServer<0){
			perror("sendto()");
			exit(EXIT_FAILURE);
		}
#ifdef DEBUG
		printf("Mensaje enviado al cliente correctamente...\n");
#endif
		
	}

	// 9. Cierro el socket UDP
	close(socketServer);
	

	return 0;
}
