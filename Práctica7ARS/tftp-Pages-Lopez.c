// Practica tema 7, Pages Lopez Juan Antonio
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h> // Para definir sockaddr_in
#include <netinet/in.h>
#include <netdb.h> // Para usar inet_aton()
#include <arpa/inet.h>
#include <unistd.h> // Para usar close()
#include <errno.h> // Para usar errno() y perror()
#include <stdio.h>
#include <stdlib.h> // Para usar EXIT_FAILURE
#include <string.h> // Para usar strcmp()
#include <stdbool.h> // Para poder usar booleanos

void codigoError(int errcode); // Prototipo de la funcion codigoError

int main(int argc, char *argv[]){ // Funcion main
	
	// Declaracion de variables
	int mode = 0; // Selecciona entre modo lectura(1) o escritura(2)
	bool modeV = false; // Controla si esta activo el modo detallado(-v)
	int sock;
	struct servent *num_port; // Estructura para almacenar el numero de puerto
	num_port = getservbyname("tftp", "udp"); // Obtiene el puerto del protocolo TFTP en UDP
	int server_port;
	char bloque[516];
	FILE *file;
	int opcode; // Almacena el opcode del paquete correspondiente
	int numBloque = 0; // Almacena el numero de bloque
	int numBloqueEsperado = 1; // Para controlar los ACKs
	int bytes = 512;

	struct sockaddr_in client_addr; // Estructura para almacenar los datos del cliente
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = 0;
	client_addr.sin_addr.s_addr = INADDR_ANY;

	struct sockaddr_in server_addr; // Estructura para almacenar los datos del servidor
	server_addr.sin_family = AF_INET;
	server_port = num_port -> s_port;
	server_addr.sin_port = server_port; // Almacena el puerto del servidor TFTP para UDP
	socklen_t serverLen = sizeof(server_addr); // Tamanh√o de sockaddr_in del servidor
	

	// 1. Gestion de los argumentos del programa
	if((argc!=5) && (argc!=4)){ // Si no hay argumentos adecuados
		exit(EXIT_FAILURE);
	}

	if(argc == 4 || argc == 5){
		if(strcmp(argv[2], "-r") == 0){ // Selecciona modo lectura(0)
			mode = 1;
		}else if(strcmp(argv[2], "-w") == 0){ // Selecciona modo escritura(1)
			mode = 2;
		}else{ // Si no hay argumentos adecuados
			exit(EXIT_FAILURE); 
		}
	}

	if(argc == 5){
		if(strcmp(argv[4], "-v") == 0){ // Selecciona modo detallado(-v)
			modeV = true;
		}else{ // Si no hay argumentos adecuados
			exit(EXIT_FAILURE);
		}
	}

	// 2. Almacena la ip del servidor en la estructura del servidor
	inet_aton(argv[1], (struct in_addr *)&server_addr.sin_addr.s_addr);
	
	// 3. Creacion del socket UDP
	if((sock = socket(AF_INET, SOCK_DGRAM, 0))<0){
		perror("socket()");
		exit(EXIT_FAILURE);
	}

	// 4. Enlaza el socket con los datos del cliente
	if(bind(sock, (struct sockaddr *)&client_addr, sizeof(client_addr))<0){
		perror("bind()");
		exit(EXIT_FAILURE);
	}	

	if(mode == 1){ // 5. Entra en modo lectura
		// Asignacion del opcode 01 (RRQ)
		bloque[0]=0x0;
		bloque[1]=0x1;
		strcpy(&bloque[2], argv[3]); // Asignacion del nombre del archivo en el bloque
		strcpy(&bloque[2+(strlen(argv[3]))+1], "octet"); // Asignacion del modo en el bloque
		// 5.1. Envio del datagrama RRQ
		if((sendto(sock, bloque, (2+strlen(argv[3])+1+5+1), 0, (struct sockaddr *)&server_addr, serverLen))<0){
			perror("sendto()");
			exit(EXIT_FAILURE);
		}

		if(modeV){ // Imprime informacion del programa si es modo detallado(-v)
			printf("Enviada solicitud de lectura de %s a servidor tftp en %s.\n", argv[3], argv[1]);
		}
		
		file = fopen(argv[3], "a"); // Abrir el fichero deseado y concatena contenido, si no existe lo creamos
		
		// Bucle de recepcion de datos y ACKs
		while(1){
			// 5.2. Recibo el datagrama DATA del servidor
			if((bytes = recvfrom(sock, bloque, sizeof(bloque), 0, (struct sockaddr *)&server_addr, &serverLen))<0){
				perror("recvfrom()");
				fclose(file);
				exit(EXIT_FAILURE);
			}
			
			opcode = (unsigned char)bloque[0]*256+(unsigned char)bloque[1]; // Obtiene el opcode
			numBloque = (unsigned char)bloque[2]*256+(unsigned char)bloque[3]; // Obtiene el errcode y numero de bloque
			if(opcode == 5){ // Administracion de errores con la funcion codigoError 
				codigoError(numBloque);
				remove(argv[3]);
				exit(EXIT_FAILURE);
			}

			if(modeV){ // Imprime informacion del programa si es modo detallado(-v)
				if(numBloque == 1){
					printf("Recibido bloque del servidor tftp.\nEs el primer bloque (numero de bloque 1).\n");
				}else{
					printf("Recibido bloque del servidor tftp.\nEs el bloque con codigo %d.\n", numBloque);
				}
			}

			if(numBloqueEsperado == numBloque){ // Si el bloque recibido es el esperado
				fwrite(&bloque[4], 1, bytes-4, file); // Escribe los bytes en el fichero
				numBloqueEsperado = numBloqueEsperado + 1; // Incrementa el numero de bloque si tiene exito
			}else{ // Si no es el bloque esperado
				numBloque = numBloqueEsperado + 1;
				printf("No es el bloque esperado enviamos ACK de bloque %d.\n", numBloque);
			}
			// Asignacion del opcode 04 (ACK)
			bloque[0] = 0x0;
			bloque[1] = 0x4;
			// Asigna el numero de bloque
			bloque[2] = ((unsigned int)numBloque)/256;
			bloque[3] = ((unsigned int)numBloque)%256;
			// 5.3 Envio del datagrama ACK al servidor
			if((sendto(sock, bloque, 4, 0, (struct sockaddr *)&server_addr, serverLen))<0){
				perror("sendto()");
				fclose(file);
				exit(EXIT_FAILURE);
			}

			if(modeV){ // Imprime informacion del programa si es modo detallado(-v)
				printf("Enviamos el ACK del bloque %d.\n", numBloque);
			}

			if(bytes < 512){ // Comprueba si ha acabado la transmision
				if(modeV){ // Imprime informacion del programa si es modo detallado(-v)
					printf("El bloque %d era el ultimo: cerramos el fichero.\n", numBloque);
				}
				fclose(file);
				exit(0);
			}
		}

	}else if(mode == 2){ // 6. Entra en modo escritura
		if((file = fopen(argv[3], "r")) == NULL){ // Comprueba que el fichero existe
			printf("Error en el fichero %s.\n", argv[3]);
			exit(EXIT_FAILURE);
		}

		// Asignacion del opcode 02 (WRQ)
		bloque[0]=0x0;
		bloque[1]=0x2;
		strcpy(&bloque[2], argv[3]); // Asignacion del nombre del archivo en el bloque
		strcpy(&bloque[2+(strlen(argv[3]))+1], "octet"); // Asignacion del modo en el bloque
		// 6.1. Envia del datagrama WRQ
		if((sendto(sock, bloque, (2+strlen(argv[3])+1+5+1), 0, (struct sockaddr *)&server_addr, serverLen))<0){
			perror("sendto()");
			exit(EXIT_FAILURE);
		}
		
		numBloqueEsperado = numBloqueEsperado - 1; // El primer numero de bloque de la escritura sera el 0

		if(modeV){ // Imprime informacion del programa si es modo detallado(-v)
			printf("Enviada solicitud de escritura de %s a servidor tftp en %s.\n", argv[3], argv[1]);
		}
		

		// Bucle de recepcion de datos y ACKs
		while(1){
			// 6.2. Recibe el datagrama ACK del servidor
			if((recvfrom(sock, bloque, sizeof(bloque), 0, (struct sockaddr *)&server_addr, &serverLen))<0){
				perror("recvfrom()");
				fclose(file);
				exit(EXIT_FAILURE);
			}
			
			opcode = (unsigned char)bloque[0]*256+(unsigned char)bloque[1]; // Obtiene el opcode
			numBloque = (unsigned char)bloque[2]*256+(unsigned char)bloque[3]; // Obtiene el errcode y numero de bloque
			if(opcode == 5){ // Administracion de errores con la funcion codigoError 
				codigoError(numBloque);
				remove(argv[3]); // Elimina el fichero en caso de fallo
				exit(EXIT_FAILURE);
			}

			if(modeV){ // Imprime informacion del programa si es modo detallado(-v)
				printf("Recibido el ACK del bloque %d.\n", numBloque);
			}

			if(bytes < 512){ // Comprueba si ha acabado la transmision
				if(modeV){ // Imprime informacion del programa si es modo detallado(-v)
					printf("El bloque %d era el ultimo: cerramos el fichero\n", numBloque);
				}
				fclose(file);
				exit(0);
			}

			if(numBloqueEsperado == numBloque){ // Si el bloque recibido es el esperado
				bytes = fread(&bloque[4], 1, 512, file); // Lee los bytes del fichero pendientes a enviar
				numBloqueEsperado = numBloqueEsperado + 1; // Incremento del numero de bloque si tiene exito
			}else{ // Si no es el bloque esperado
				numBloque = numBloqueEsperado - 1;
				printf("No es el bloque esperado cancelamos el intento de transmision.\n");
				exit(EXIT_FAILURE);
			}
			// Asignacion del opcode 03 (DATA)
			bloque[0] = 0x0;
			bloque[1] = 0x3;
			// Asignacion del numero de bloque
			bloque[2] = ((unsigned int)numBloqueEsperado)/256;
			bloque[3] = ((unsigned int)numBloqueEsperado)%256;
			// 6.3. Envio del datagrama DATA al servidor
			if((sendto(sock, bloque, bytes+4, 0, (struct sockaddr *)&server_addr, serverLen))<0){
				perror("sendto()");
				exit(EXIT_FAILURE);
			}

			if(modeV){ // Imprime informacion del programa si es modo detallado(-v)
				printf("Enviado bloque al servidor tftp\nEs el bloque con codigo %d.\n", numBloqueEsperado);
			}
		}
	}
	return 0;
}

// Funcion para gestinar el codigo de error en el servidor
void codigoError(int errcode){
	switch(errcode){
		case 0:
			printf("No definido. Comprobar errstring\n");
			break;	
		case 1:
			printf("Fichero no encontrado\n");
			break;
		case 2:
			printf("Violacion de acceso\n");
			break;
		case 3:
			printf("Espacio de almacenamiento lleno\n");
			break;
		case 4:
			printf("Operacion TFTP ilegal\n");
			break;
		case 5:
			printf("Identificador de transferencia desconocido\n");
			break;
		case 6:
			printf("El fichero ya existe\n");
			break;
		case 7:
			printf("Usuario desconocido\n");
			break;
		default:
			printf("Error desconocido\n");
	}
}








