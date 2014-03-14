// Copyright 2012 Giancarlo Klemm Camilo, Renan Greca

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// ************************************************************* //


// Trabalho 3 de redes, Alunos: Giancarlo Klemm Camilo, Renan Greca.
// UFPR 2012/2

// Para informações sobre este arquivo ver: udp_socket.h

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include"udp_socket.h"


// if any error accours //
void die(char *s)
{
	perror(s);
	exit(1);
}

// iniciates and prepares the sockets //
int setup_socket(int *s, struct sockaddr_in *si_me, struct sockaddr_in *si_other, int port_server, int port_client, char *server_ip){

	//create a UDP socket
	if ((*s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
		printf("Erro ao criar socket!\n");
		return 0;
	}
	// zero out the structure //
	memset((char *) si_me, 0, sizeof(*si_me));
	memset((char *) &si_other, 0, sizeof(si_other));

	printf("Sending data at port: %d\n",port_server);
	printf("Recieving data at port: %d\n",port_client);
	
	si_me->sin_family = AF_INET;
	si_me->sin_port = htons(port_server);
	si_me->sin_addr.s_addr = htonl(INADDR_ANY);
//	si_me->sin_addr.s_addr = inet_addr(server_ip);

//	if (inet_aton(server_ip , &si_other->sin_addr) == 0){
//		printf("Erro na função inet_aton()!\n");
//		return 0;
//	}
	// bind socket to port //
	if( bind(*s , (struct sockaddr*)si_me, sizeof(*si_me) ) == -1){
		printf("Erro ao fazer BIND no socket!\n");
		return 0;
	}
	return 1;
}

int send_data(struct sockaddr_in *si_other, int s, char *message, int slen, int port_client, char *server_ip){
//	printf("Connecting to port: %d\n",port_client);
	memset((char *) si_other, 0, sizeof(*si_other));
	si_other->sin_family = AF_INET;
	si_other->sin_port = htons(port_client);
	if (inet_aton(server_ip , &si_other->sin_addr) == 0){
		printf("Erro na função inet_aton()!\n");
		return 0;
	}
//	printf("Enviando mensagem...\n");
	//send the message
	if (sendto(s, message, strlen(message) , 0 , (struct sockaddr *) si_other, slen)==-1){
		die("sendto()");
	}
}

int recieve_sendout_data(int *recv_len, int s, char *buf, struct sockaddr_in *si_other, int *slen){
//	printf("Waiting for data...");
	fflush(stdout);
	//try to receive some data, this is a blocking call

	struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 100000;
  if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
      printf("Error");
  }
	if((*recv_len = recvfrom(s, buf, MESG_SIZE, 0, (struct sockaddr *) si_other, slen)) < 0){
//		printf("tempo limite atingido\n");
//		getchar();
		return 0;
	}
	return 1;
	//print details of the client/peer and the data received
//	printf("Received packet from %s:%d\n", inet_ntoa(si_other->sin_addr), ntohs(si_other->sin_port));
}

int recieve_data(int *recv_len, int s, char *buf, struct sockaddr_in *si_other, int *slen){
//	printf("Waiting for data...");
	fflush(stdout);
	//try to receive some data, this is a blocking call

//	int buffsize = 50000;
//  if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &buffsize, sizeof(buffsize))){
//      printf("Error");
//  }

	if ((*recv_len = recvfrom(s, buf, MESG_SIZE, 0, (struct sockaddr *) si_other, slen)) == -1){
		printf("recvfrom()");
	}
	//print details of the client/peer and the data received
//	printf("Received packet from %s:%d\n", inet_ntoa(si_other->sin_addr), ntohs(si_other->sin_port));
	return 1;
}

int envia_mesg(struct sockaddr_in *si_other, int s, char *message, int port_client, int *recv_len, char *buf, int *slen, char *server_ip){
//	printf("enviando mensagem: %s\n",message);
	do{
		send_data(si_other, s, message, *slen, port_client, server_ip);
	}while(recieve_sendout_data(recv_len, s, buf, si_other, slen) == 0);
//	printf("mensagem recebida: %s\n",buf);
	if(buf[3] == '1')
		return 1;
	else
		return 0;
}

int receba_mesg(struct sockaddr_in *si_other, int s, char *message, int port_client, int *recv_len, char *buf, int *slen, int p_number, char *server_ip){
	do{
	}while(recieve_sendout_data(recv_len, s, buf, si_other, slen) == 0);
	if(atoi(&buf[1]) == p_number){
//		printf("mensagem recebida de %c: %s\n",buf[0],buf);
//		getchar();
		strcpy(message, "00a1");
		message[0] = (char) p_number + '0';
		message[1] = buf[0];
		send_data(si_other, s, message, *slen, port_client,server_ip);
		return 1;
	}else{
//		printf("mensagem para outro destinatário.\n");
		strcpy(message, buf);
		send_data(si_other, s, message, *slen, port_client,server_ip);
		return 0;
	}
}
