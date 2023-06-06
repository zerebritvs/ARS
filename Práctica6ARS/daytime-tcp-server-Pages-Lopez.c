// Practica tema 6, Pages Lopez Juan Antonio

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
#include <signal.h>	// Para usar signal()
#undef DEBUG

int socketServer; // Variable de error del socket que tambien se usa en la funcion signal_handler y main

void *signal_handler(int sig);

int main(int argc, char *argv[]){
	// Variables de error de las funciones
	int bindServer;
	int listenServer;
	int acceptServer;
	int sendServer;
	int recvServer;
	int shutdownServer;
	// Variables para la estructura del servidor
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	// Variables para la estructura del cliente
	struct sockaddr_in client_addr;
	socklen_t socketLen = sizeof(client_addr);

	struct servent *service;
	char hostname[100];
	FILE *fich;
	int child;

	signal(SIGINT, (void *)&signal_handler); //Funcion para capturar el fin del programa <ctrl>C
	
	// 1. Obtengo el nombre del host del servidor
	if(gethostname(hostname, sizeof(hostname))<0){
		perror("gethostname()");
		exit(EXIT_FAILURE);
	}
	
	// 2.1 Si no se especifica el puerto
	if(argc == 1){
		service = getservbyname("daytime", "tcp");
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
	
	// 3. Creacion del socket TCP
	socketServer = socket(AF_INET, SOCK_STREAM, 0);

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
	// 5. Marcamos el socket con apertura pasiva, preparado para recibir hasta 5 peticiones en la cola
	listenServer = listen(socketServer, 5);
	
	if(listenServer<0){
		perror("listen()");
		exit(EXIT_FAILURE);
	}
#ifdef DEBUG
	printf("listen hecho correctamente...\n");
#endif
	
	// 6. El servidor se queda escuchando para recibir peticiones
	while(1){	

		printf("Esperando mensajes...\n\n");

		//7. Espero la conexion de un cliente (llamada bloqueante) y obtengo otro socket "hijo" en acceptServer
		acceptServer = accept(socketServer, (struct sockaddr *)&client_addr, &socketLen);

		if(acceptServer<0){
			perror("accept()");
			exit(EXIT_FAILURE);
		}
#ifdef DEBUG
		printf("Accept hecho correctamente...\n");
#endif
		//8. Creo un hijo para procesar la peticion
		child = fork();
		if(child==0){

			// 9. Obtengo fecha y hora del sistema
			char date[30] = "";
			system("date > /tmp/tt.txt");
			fich = fopen("/tmp/tt.txt", "r");

			if(fgets(date, sizeof(date), fich) == NULL){
				printf("Error en system(), en fopen(), o en fgets()\n");
				exit(EXIT_FAILURE);
			}
#ifdef DEBUG
			printf("La fecha y hora son: %s\n", date);
#endif	
			char bufferRespuesta[100] = "";
			char bufferServer[100] = "";

			strcat(bufferRespuesta, hostname);
			strcat(bufferRespuesta, ": ");
			strcat(bufferRespuesta, date);
		
#ifdef DEBUG
			printf("Hostname: %s\n", hostname);
			printf("Date: %s\n", date);
			printf("Mensaje: %s\n", bufferRespuesta);
#endif	
			// 10. Envio la cadena obtenida al cliente
			sendServer = send(acceptServer, bufferRespuesta, sizeof(bufferRespuesta), 0);
			if(sendServer<0){
				perror("send()");
				exit(EXIT_FAILURE);
			}
#ifdef DEBUG
			printf("Mensaje enviado al cliente correctamente...\n");
#endif			
			// 11. Vacio el bufferServer con recv
			recvServer = recv(acceptServer, bufferServer, sizeof(bufferServer), 0);
			if(recvServer<0){
				perror("recv()");
				exit(EXIT_FAILURE);
			}
#ifdef DEBUG
			printf("Recv hecho correctamente...\n");
#endif
			// 12. Notifico al cliente que quiero cerrar la conexion
			shutdownServer = shutdown(acceptServer, SHUT_RDWR);
			if(shutdownServer<0){
				perror("shutdown() fork");
				exit(EXIT_FAILURE);
			}

			//13. Cierro el socket del hijo del Servidor
			close(socketServer);
			
			//14. Salgo del hijo del fork para procesar una solicitud nueva
			exit(EXIT_SUCCESS);
		}
	}
}

// Funcion que captura la interrupcion <ctrl>C para cerrar los sockets y puertos abiertos
void *signal_handler(int sig){
	if(sig == SIGINT){
		if(shutdown(socketServer, SHUT_RDWR)<0){
			perror("shutdown() socket");
			exit(EXIT_FAILURE);
		}
		close(socketServer);
		exit(EXIT_SUCCESS);
	}
	return 0;
}
