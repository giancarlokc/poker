
// Trabalho 3 de redes, Alunos: Giancarlo Klemm Camilo, Renan Greca.
// UFPR 2012/2

// Essa biblioteca garante uma comunicação segura usando protocolo UDP.
// Isso é obtido através das funções envia_mesg() e receba_mesg().
// Para funcionamento da biblioteca é necessario a declaração correta das variáveis
// e das estruturas, a seguir segue um exemplo:
//
// 			declarações para sokcets
//			struct sockaddr_in si_me, si_other;
//			int s, slen = sizeof(si_other) , recv_len, port_server, port_client;
//			char buf[MESG_SIZE], message[MESG_SIZE];
//
// Deve ser usada a função setup_socket() para iniciar e configurar os sockets 


#define SERVER "127.0.0.1"
#define MESG_SIZE 1000	//Max length of buffer

void die(char *);
int setup_socket(int *, struct sockaddr_in *, struct sockaddr_in *, int, int, char *);
int send_data(struct sockaddr_in *, int, char *, int, int, char *);
int recieve_data(int *, int, char *, struct sockaddr_in *, int *);
int envia_mesg(struct sockaddr_in *, int, char *, int, int *, char *, int *,char *);
int receba_mesg(struct sockaddr_in *, int, char *, int, int *, char *, int *, int, char *);
