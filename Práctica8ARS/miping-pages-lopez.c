// Practica tema 8, Pages Lopez Juan Antonio

#include "ip-icmp-ping.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void codigoError(int num); // Prototipado de la funcion codigoError

int main(int argc, char *argv[]){ // Programa principal

	int sock; // Variable de error para socket
	int modeV = 0; // Modo detallado activado si es 1
	char ip_addr [30]; // Almacena la direccion ip
	int numShorts; // Para dar tamanho a half words
	unsigned int acumulador = 0; // Contador para checksum
	unsigned short int *puntero; // Puntero para recorrer estructuras
	int i; // Iterador para el bucle for del checksum

	ECHORequest echoReq; // Estructara para almacenar la peticion ICMP
	ECHOResponse echoRes; // Estructura para almacenar la respuesta ICMP

	struct sockaddr_in origen_addr;	// Estructura para almacenar los datos de la maquina origen
	origen_addr.sin_family = AF_INET;
	origen_addr.sin_port = 0;
	origen_addr.sin_addr.s_addr = INADDR_ANY;

	struct sockaddr_in destino_addr; // Estructura para almacenar los datos de la maquina destino
	destino_addr.sin_family = AF_INET;
	socklen_t destinoLen = sizeof(destino_addr); // Tama�o de sockaddr_in de la maquina destino

	// Gestion de los parametros del programa
	if((argc!=3) && (argc!=2)){
		exit(EXIT_FAILURE);
	}
	if(argc == 3){
		if(strcmp(argv[2], "-v") == 0){
			modeV = 1;
		}
	}

	strcpy(ip_addr, argv[1]); // Almacenamos la cadena de la ip
	inet_aton(ip_addr, (struct in_addr *) &destino_addr.sin_addr.s_addr); // Asignamos la ip de la maquina destino
	// 1. Creacion de socket ICMP
	if((sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0){
		perror("socket()");
		exit(EXIT_FAILURE);
	}
	
	// 2. Enlazamiento de socket con la direccion origen
	if((bind(sock, (struct sockaddr *) &origen_addr, sizeof(origen_addr))) < 0){
		perror("bind()");
		exit(EXIT_FAILURE);
	}

	if(modeV){ // Modo detallado (-v)
		printf("-> Generando cabecera ICMP.\n");
	}

	echoReq.icmpHeader.Type = 8; // Asignamiento del tipo de ICMP
	if(modeV){ // Modo detallado (-v)
		printf("-> Type: %d\n", echoReq.icmpHeader.Type);
	}

	echoReq.icmpHeader.Code = 0; // Asignamiento del codigo de ICMP
	if(modeV){ // Modo detallado (-v)
		printf("-> Code: %d\n", echoReq.icmpHeader.Code);
	}

	echoReq.ID = getpid(); // Obtiene el PID
	if(modeV){ // Modo detallado (-v)
		printf("-> Identifier (pid): %d.\n", echoReq.ID);
	}

	echoReq.SeqNumber = 0; // Obtiene el numero de secuencia
	if(modeV){ // Modo detallado (-v)
		printf("-> Seq. number: %d\n", echoReq.SeqNumber);
	}
	
	echoReq.icmpHeader.Checksum = 0; // Inicializamos el checksum a 0 para calcularlo posteriormente
	
	memset(echoReq.payload, 0, 64); // Asignacion de todos los bytes de payload a 0

	strcpy(echoReq.payload, "Aqui esta el Payload."); // Asignacion del payload del mensaje
	if(modeV){ // Modo detallado (-v)
		printf("-> Cadena a enviar: %s\n", echoReq.payload);
	}

	// 3. Calculo del checksum
	numShorts = sizeof(echoReq)/2;
	puntero = (unsigned short *) &echoReq;
	for(i = 0; i < numShorts; i++){
		acumulador = acumulador + (unsigned int) *puntero;
		puntero++;
	}

	acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);
	acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);
	acumulador = ~acumulador;

	echoReq.icmpHeader.Checksum = (unsigned short) acumulador;
	

	if(modeV){ // Modo detallado (-v)
		printf("-> Checksum: 0x%x.\n", echoReq.icmpHeader.Checksum);
		printf("-> Tamaño total del paquete ICMP: %d.\n", (int)sizeof(echoReq));
	}
	
	// 4. Envio de solicitud a la maquina destino
	if((sendto(sock, &echoReq, sizeof(echoReq), 0, (struct sockaddr *) &destino_addr, destinoLen)) < 0){
		perror("sendto()");
		exit(EXIT_FAILURE);
	}
	printf("Paquete ICMP enviado a %s\n", argv[1]);
	
	// 5. Recepcion de la respuesta de la maquina destino
	if((recvfrom(sock, &echoRes, sizeof(echoRes), 0, (struct sockaddr *)&destino_addr, &destinoLen)) < 0){
		perror("recvfrom()");
		exit(EXIT_FAILURE);
	}
	printf("Respuesta recibida desde %s\n", argv[1]);

	if(modeV){ // Modo detallado (-v)
		printf("-> Tamaño de la respuesta: %d\n", (int)sizeof(echoRes));
		printf("-> Cadena recibida: %s\n", echoRes.payload);
		printf("-> Identifier (pid): %d.\n", echoRes.ID);
		printf("-> TTL: %d.\n", echoRes.ipHeader.TTL);
	}

	if(echoRes.icmpHeader.Type == 0){ // Si es correcta
		printf("Descripcion de la respuesta: respuesta correcta (type %d, code %d).\n", echoRes.icmpHeader.Type, echoRes.icmpHeader.Code);
	}else if(echoRes.icmpHeader.Type == 3){ // Si es incorrecta
		printf("Descripcion de la respuesta: respuesta incorrecta (type %d, code %d).\n", echoRes.icmpHeader.Type, echoRes.icmpHeader.Code);
		codigoError((int)echoRes.icmpHeader.Code); // Funcion para gestinar el codigo de error
	}

	return 0;
}

// Funcion para gestionar el codigo de error de la respuesta ICMP
void codigoError(int error){
	switch(error){
		case 0:
			printf("Net Unreachable\n");break;
		case 1:
			printf("Host Unreacheable\n");break;
		case 2:
			printf("Protocol Unreachable\n");break;
		case 3:
			printf("Port Unreachable\n");break;
		case 4:
			printf("Fragmentation Needed and Don't Fragment was Set\n");break;
		case 5:
			printf("Source Route Failed\n");break;
		case 6:
			printf("Destination Port Unknown\n");break;
		case 7:
			printf("Destination Host Unknown\n");break;
		case 8:
			printf("Source Host Isolated\n");break;
		case 9:
			printf("Communication with Destination Network is Administratively Prohibited\n");break;
		case 10:
			printf("Communication with Destinantion Host is Administratively Prohibited\n");break;
		case 11:
			printf("Destination Network Unreachable for Type of Service\n");break;
		case 12:
			printf("Destination Host Unreachable for Type of Service\n");break;
		case 13:
			printf("Communication Administratively Prohibited\n");break;
		case 14:
			printf("Host Precedence Violation\n");break;
		case 15:
			printf("Precedence cutoff un effect\n");break;
		default:
			printf("Unknown error\n");
	}
}




