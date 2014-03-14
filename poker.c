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

// ********************************************************************* //

// Project 3 of Computer Networks. Students: Giancarlo Klemm Camilo, Renan Greca
// UFPR 2012/2

// This program uses a library udp_socket.h (Made by us) that was implemented using socket.h
// This program tries to simulate a poker game with 4 players.

// The basic rules for the program are:
// 1. Each player sould start the program with a unique ID (integer).
// 2. The player '0' starts.
// 3. The normal rules of texas hold'em apply

// Trabalho 3 de redes, Alunos: Giancarlo Klemm Camilo, Renan Greca.
// UFPR 2012/2

// Este programa utiliza a biblioteca udp_socket.h (feita por nós) que por sua vez implementa a biblioteca socket.h
// Este programa visa simular um jogo de poker entre 4 jogadores. Sendo que o jogo tem as seguintes regras:
//
//  1. Cada jogador deve se identificar com um numero unico como argumento do programa
//	2. O jogador '0' sempre começa
//  3. As regras normal do poker Texas Hold'em se aplicam

// message: |Source|Destination|'x'|Option|----[4]"hand"|----------[10]"desk's cards"|----------------[16]"player's pot"|----[4]"pot"


#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include"udp_socket.h"
#include <time.h>

#define copas 0
#define paus 1
#define ouros 2
#define espadas 3

#define straight_flush 8
#define four_of_kind 7
#define full_house 6
#define flush 5
#define straight 4
#define three_of_kind 3
#define two_pair 2
#define one_pair 1
#define high_card 0

#define WAIT 0
#define CHECK 1
#define BET 2
#define PAY 3
#define FOLD 4

#define INITIAL_POT 1000

// struct that defines a card //
typedef struct Carta{
	int valor;
	int naipe;
	int usada;
	int visivel;
}Carta;

typedef struct Combinacao_Carta{
	Carta carta[7];
	int tipo;
	int valor;
}Combinacao_Carta;

// struct that defines a player's hand //
typedef struct Mao{
	Carta carta[2];
}Mao;

// struct that defines the desk of bets //
typedef struct Mesa{
	Carta carta[5];
}Mesa;

// struct that defines the deck of cards //
typedef struct Baralho{
	Carta carta[52];
}Baralho;

// struct that defines a player //

typedef struct Jogo{
	int fim_jogo;
	int player_turn;
	int fim_turno;
	int fim_rodada;
	int big_blind, small_blind;
	int dealer;
	int pot_mesa;
	int pot_mesa_ant;
	Mesa mesa;
	Baralho baralho;
}Jogo;

typedef struct Player{
	Mao mao;
	int estado_atual;
	int aposta_atual;
	int aposta_anterior;
	int pot;
}Player;

// return 1 if the program has enough arguments to begin //
int testa_argumentos(int n_arg, char **args, int *p_number, char *server_ip){
	if(n_arg == 3){
		*p_number = atoi(args[1]);
		printf("Player number: %d\n",*p_number);
		strcpy(server_ip, args[2]);
		return 1;
	}else{
		printf("The program requires 2 arguments. ./poker [player number] [ip to connect]\n");
		return 0;
	}
}

// select the port's numbers to be used //
void seleciona_portas(int *port_server, int *port_client, int p_number){
	*port_server = 8888 + p_number;
	if(p_number != 3)
		*port_client = 8888 + p_number + 1;
	else
		*port_client = 8888;
}

// send a message to all the player in the correct order //
void envia_todos(int *player_number,char *message,struct sockaddr_in *si_other,int s,int port_client,int *recv_len,char *buf,int *slen, char *server_ip){
	int i = *player_number - 1;
	while(*player_number != i){ // manda uma mensagem para cada dos outros jogaores
		if((i >= 0) && (i <= 3)){
			do{
				message[0] = *player_number + '0';
				message[1] = i + '0';
			}while((envia_mesg(si_other, s, message, port_client, recv_len, buf, slen, server_ip) == 0) && (i < 4));
//			printf("mensagem enviada e recebida com sucesso para %d!\n",i);
//			getchar();
		}
		if(i == -1)
			i = 4;
		else
			i--;
	}
}

// shows the deck of cards //
void mostra_baralho(struct Baralho *baralho){
	int i;
	printf("Deck -  Value  Kind\n");
	for(i=0;i<52;i++)
		printf("carta %d: %d %d\n",i+1,baralho->carta[i].valor,baralho->carta[i].naipe);
}

// show the player's hand //
void mostra_mao(struct Mao mao){
	printf("Hand - Value Kind\n");
	printf("card 1: %d %d\ncard 2: %d %d\n",mao.carta[0].valor,mao.carta[0].naipe,mao.carta[1].valor,mao.carta[1].naipe);
}

// show the desk of cards //
void mostra_mesa(struct Mesa mesa){
	int i;
	printf("Table - Value Kind\n");
	for(i=0;i<5;i++){
		if(mesa.carta[i].visivel == 1)
			printf("Card %d: %d %d\n",i+1,mesa.carta[i].valor,mesa.carta[i].naipe);
		else
			printf("Card %d: X X\n",i+1);
	}
}

// iniciates the deck of cards //
void inicia_baralho(struct Baralho *baralho){
	int i,j,aux = 0;
	memset(baralho, 0, sizeof(baralho));
	for(i=2;i<15;i++)
		for(j=0;j<4;j++){
			baralho->carta[aux].valor = i;
			baralho->carta[aux].naipe = j;
			baralho->carta[aux].usada = 0;
			baralho->carta[aux].visivel = 1;
			aux++;
		}
}

// generates a random deck of cards //
struct Mesa seleciona_mesa(struct Baralho *baralho){
	Mesa mesa;
	int i, rand_aux;

	srand(time(NULL));
	for(i=0;i<5;i){
		rand_aux = rand() % 52;
		if(baralho->carta[rand_aux].usada == 0){
			mesa.carta[i] = baralho->carta[rand_aux];
			mesa.carta[i].visivel = 0;
			baralho->carta[rand_aux].usada = 1;
			i++;
		}
	}
	return mesa;
}

// generates a random player's hand //
struct Mao seleciona_mao(struct Baralho *baralho){
	Mao mao;
	int i, rand_aux;

	srand (time(NULL));
	for(i=0;i<2;i){
		rand_aux = rand() % 52;
		if(baralho->carta[rand_aux].usada == 0){
			mao.carta[i] = baralho->carta[rand_aux];
			baralho->carta[rand_aux].usada = 1;
			i++;
		}
	}
	return mao;
}

// iniciates the player's pot //
iniciate_pot(struct Player *jogador){
	int i;
	for(i=0;i<4;i++){
		jogador[i].pot = INITIAL_POT + 2;
	}
}

// transcribe the deck of cards to a string mode //
void escreve_mesa(struct Mesa mesa, char *mao_string){
	int i, aux = 8;
	for(i=0;i<5;i++){
		if(mesa.carta[i].valor > 9){
			if(mesa.carta[i].valor == 10){
				mao_string[aux] = 'D';
			}
			else if(mesa.carta[i].valor == 11){
				mao_string[aux] = 'Q';
			}
			else if(mesa.carta[i].valor == 12){
				mao_string[aux] = 'J';
			}
			else if(mesa.carta[i].valor == 13){
				mao_string[aux] = 'K';
			}
			else if(mesa.carta[i].valor == 14){
				mao_string[aux] = 'A';
			}
			mao_string[aux+1] = (char) mesa.carta[i].naipe + '0';
		}else{
			mao_string[aux] = (char) mesa.carta[i].valor + '0';
			mao_string[aux+1] = (char) mesa.carta[i].naipe + '0';
		}
		aux+=2;
	}
	mao_string[18] = '\0';
}

// transcribe the player's hand to a string mode //
void escreve_mao(struct Mao outra_mao, char *mao_string){
	int i, aux = 4;
	for(i=0;i<2;i++){
		if(outra_mao.carta[i].valor > 9){
			if(outra_mao.carta[i].valor == 10){
				mao_string[aux] = 'D';
			}
			else if(outra_mao.carta[i].valor == 11){
				mao_string[aux] = 'Q';
			}
			else if(outra_mao.carta[i].valor == 12){
				mao_string[aux] = 'J';
			}
			else if(outra_mao.carta[i].valor == 13){
				mao_string[aux] = 'K';
			}
			else if(outra_mao.carta[i].valor == 14){
				mao_string[aux] = 'A';
			}
			mao_string[aux+1] = (char) outra_mao.carta[i].naipe + '0';
		}else{
			mao_string[aux] = (char) outra_mao.carta[i].valor + '0';
			mao_string[aux+1] = (char) outra_mao.carta[i].naipe + '0';
		}
		aux+=2;
	}
	mao_string[8] = '\0';
}

void escreve_aposta_atual(char *mao_string, struct Player *jogador){
	int pot_aux[4], i, j;

	for(j=0;j<4;j++)
		pot_aux[j] = jogador[j].aposta_atual;
	for(i=0;i<4;i++){
		for(j=0;j<4;j++){
			mao_string[(57-i*4)-j] = (char) ((pot_aux[3-i] % 10) + '0');
			pot_aux[3-i] = pot_aux[3-i] / 10;
		}
	}
	mao_string[58] = '\0';
}

void recebe_aposta_atual(char *buf, struct Player *jogador){
	int i=0, j, aux = 26;

	for(i=0;i<4;i++){
		jogador[i].aposta_atual = char_to_int(buf,42+i*4,(42+(i+1)*4)-1);
//		printf("aposta_atual[i] = %d\n",aposta_atual[i]);
	}
}

// transcribe the player's state to a string mode //
void escreve_estado(char *mao_string, struct Player *jogador){
	mao_string[18] = (char) jogador[0].estado_atual + '0';
	mao_string[19] = (char) jogador[1].estado_atual + '0';
	mao_string[20] = (char) jogador[2].estado_atual + '0';
	mao_string[21] = (char) jogador[3].estado_atual + '0';
	mao_string[22] = '\0';
}

int char_to_int(char *string, int inicio, int fim){
	char buff[MESG_SIZE], i = 0, j;

	for(j=inicio;j<=fim;j++){
		buff[i] = string[j];
		i++;
	}
	buff[i] = '\0';
	return atoi(buff);
}

// transcribe the main pot and the player's pot to a string mode //
void escreve_pot(char *mao_string, int pot, struct Player *jogador){
	int pot_aux[4];
	int i=0, j, aux = 26;

	for(j=0;j<4;j++){
		pot_aux[j] = jogador[j].pot;
//		printf("pot do jogador %d: %d\n",j,jogador[j].pot);
	}
	while(i < 4){
		mao_string[25-i] = (char) ((pot % 10) + '0');
		pot = pot/10;
		i++;
	}
	for(i=0;i<4;i++){
		for(j=0;j<4;j++){
			mao_string[(41-i*4)-j] = (char) ((pot_aux[3-i] % 10) + '0');
			pot_aux[3-i] = pot_aux[3-i] / 10;
		}
	}
}

void recebe_pot(char *buf, int *pot_mesa, struct Player *jogador){
	int i=0, j, aux = 26;

	*pot_mesa = char_to_int(buf,22,25);
	for(i=0;i<4;i++){
		jogador[i].pot = char_to_int(buf,26+i*4,(26+(i+1)*4)-1);
	}
}

// transcribe a string to a player's state mode //
void recebe_estado(char *buf, struct Player *jogador){
	jogador[0].estado_atual = (int) buf[18] - '0';
	jogador[1].estado_atual = (int) buf[19] - '0';
	jogador[2].estado_atual = (int) buf[20] - '0';
	jogador[3].estado_atual = (int) buf[21] - '0';
}

// transcribe a string to a desk of cards mode //
void recebe_mesa(char *buf, struct Mesa *mesa){
	int i, aux = 8;
	for(i=0;i<5;i++){
		if(buf[aux] > '9'){
			if(buf[aux] == 'D'){
				mesa->carta[i].valor = 10;
			}
			else if(buf[aux] == 'Q'){
				mesa->carta[i].valor = 11;
			}
			else if(buf[aux] == 'J'){
				mesa->carta[i].valor = 12;
			}
			else if(buf[aux] == 'K'){
				mesa->carta[i].valor = 13;
			}
			else if(buf[aux] == 'A'){
				mesa->carta[i].valor = 14;
			}
			mesa->carta[i].naipe = (int) buf[aux+1] - '0';
		}else{
			mesa->carta[i].valor = (int) buf[aux] - '0';
			mesa->carta[i].naipe = (int) buf[aux+1] - '0';
		}
		aux+=2;
	}
}

// transcribe a string to a player's hand mode //
void recebe_mao(char *buf, struct Player *jogador, int player_number){
	int i, aux = 4;
	for(i=0;i<2;i++){
		if(buf[aux] > '9'){
			if(buf[aux] == 'D'){
				jogador[player_number].mao.carta[i].valor = 10;
			}
			else if(buf[aux] == 'Q'){
				jogador[player_number].mao.carta[i].valor = 11;
			}
			else if(buf[aux] == 'J'){
				jogador[player_number].mao.carta[i].valor = 12;
			}
			else if(buf[aux] == 'K'){
				jogador[player_number].mao.carta[i].valor = 13;
			}
			else if(buf[aux] == 'A'){
				jogador[player_number].mao.carta[i].valor = 14;
			}
			jogador[player_number].mao.carta[i].naipe = (int) buf[aux+1] - '0';
		}else{
			jogador[player_number].mao.carta[i].valor = (int) buf[aux] - '0';
			jogador[player_number].mao.carta[i].naipe = (int) buf[aux+1] - '0';
		}
		aux+=2;
	}
}

// copies one vector to another //
void copia_vetor(struct Player *jogador){
	int i;
	for(i=0;i<4;i++)
		jogador[i].aposta_anterior = jogador[i].aposta_atual;
}

// increments the counter that can goes from 0 to 3 //
void count_to_four(int *a){
	(*a)++;
	if(*a == 4)
		*a = 0;
}

// shows 'n' cards of the desk //
void show_desk(struct Mesa *mesa, int n){
	int i;

	for(i=0;i<n;i++)
		mesa->carta[i].visivel = 1;
}

// iniciates the turn
int iniciate_turn(int *rodada, struct Jogo *jogo, int *first_turn){
	int i;

	*rodada = 0;
	jogo->fim_turno = 0;
	*first_turn = 0;
	for(i=0;i<5;i++)
		jogo->mesa.carta[i].visivel = 0;
	jogo->pot_mesa = 0;
}

// check to see if it's the end of a turn //
int check_end_turn(struct Player *jogador){
	int i = 0,cont_a = 0,cont_b = 0, cont_c = 0, ant_bet = 0, aux_i;

	while(i<4){
		if(ant_bet == jogador[i].aposta_atual)
			cont_c++;
		ant_bet = jogador[i].aposta_atual;
		if(jogador[i].estado_atual == PAY)
			cont_a++;
		else if(jogador[i].estado_atual == BET){
			cont_b++;
		}
		i++;
	}
	if(((cont_a == 3) && (cont_b == 1)) && (cont_c == 3))
		return 1;
		
	i = 0;
	cont_a = 0;
	while(i<4){
		if(jogador[i].estado_atual == FOLD)
			cont_a++;
		i++;
	}
	if(cont_a == 3)
		return 1;
		
	
	i = 0;
	while(i<4){
		if((jogador[i].estado_atual == WAIT) || (jogador[i].estado_atual == PAY) || (jogador[i].estado_atual == BET))
			return 0;
		i++;
	}
	return 1;
}

// set 0 at the current bets
void zera_aposta_atual(struct Player *jogador){
	int i;

	for(i=0;i<4;i++)
		jogador[i].aposta_atual = 0;
}

// set 0 at the current states, exept if the state is FOLD
void zera_estado_atual(struct Player *jogador){
	int i;
	
	for(i=0;i<4;i++)
		if(jogador[i].estado_atual != FOLD)
			jogador[i].estado_atual = WAIT;
}

void ordena_cartas(struct Combinacao_Carta *combinacao, int n){
	int i,j;

	for(j=0;j<7;j++){
		for(i=0;i<7;i++){
			if(i != 6)
				if(combinacao[n].carta[i].valor > combinacao[n].carta[i+1].valor){
					Carta aux;
					aux = combinacao[n].carta[i];
					combinacao[n].carta[i] = combinacao[n].carta[i+1];
					combinacao[n].carta[i+1] = aux;
				}
		}
	}
}

void verifica_combinacao(struct Combinacao_Carta *combinacao, int n){
	int i, contador = 1, maior = 0, j, full_house_n = 0;

	ordena_cartas(combinacao, n);

	// check to se if the hand is a straight flush
	contador = 1; maior = 0;
	for(i=6;i>0;i--){
		if((combinacao[n].carta[i].naipe == combinacao[n].carta[i-1].naipe) && (combinacao[n].carta[i].valor == combinacao[n].carta[i-1].valor - 1)){
			if(combinacao[n].carta[i].valor > maior)
				maior = combinacao[n].carta[i].valor;
			contador++;
			if(contador == 5){
				printf("It's a STRAIGHT FLUSH\n");
				combinacao[n].tipo = straight_flush;
				combinacao[n].valor = maior;
				return;
			}
		}
		else{
			contador = 1;
			maior = 0;
		}
	}

	// check to see if the hand is a four of kind
	contador = 1; maior = 0;
	for(i=6;i>0;i--){
		if(combinacao[n].carta[i].valor == combinacao[n].carta[i-1].valor){
			contador++;
			if(contador == 4){
				printf("It's a FOUR OF KIND\n");
				combinacao[n].tipo = four_of_kind;
				combinacao[n].valor = combinacao[n].carta[i+2].valor;
				return;
			}
		}
		else
			contador = 1;
	}

	// check to see if the hand is a full house CHECAR MAIOR
	int valor_usado, tem_three_kind = 0;
	contador = 1; maior = 0;
		for(i=6;i>0;i--){
			if(combinacao[n].carta[i].valor == combinacao[n].carta[i-1].valor){
				contador++;
				if(contador == 3){
					valor_usado = combinacao[n].carta[i].valor;
					tem_three_kind = 1;
				}
			}
			else
				contador = 1;
		}
		contador = 1;
		for(i=6;i>0;i--){
			if((combinacao[n].carta[i].valor == combinacao[n].carta[i-1].valor) && (combinacao[n].carta[i].valor != valor_usado)){
				contador++;
				if(contador == 2){
					full_house_n = 1;
					if((full_house_n == 1) && (tem_three_kind == 1)){
						printf("It's a FULL HOUSE\n");
						combinacao[n].tipo = full_house;
						combinacao[n].valor = valor_usado;
						return;
					}
				}
			}
			else
				contador = 1;
		}

	// check to se if the hand is a flush
	contador = 1; maior = 0;
	for(i=6;i>0;i--){
		if(combinacao[n].carta[i].naipe == combinacao[n].carta[i-1].naipe){
			contador++;
			if(contador == 5){
				printf("It's a FLUSH\n");
				combinacao[n].tipo = flush;
				combinacao[n].valor = maior;
				return;
			}
		}
		else{
			contador = 1;
			maior = 0;
		}
	}

	// check to see if the hand is a straight
	contador = 1; maior = 0;
	for(i=6;i>0;i--){
		if(combinacao[n].carta[i].valor == combinacao[n].carta[i-1].valor + 1){
			contador++;
			if(contador == 5){
				printf("It's a STRAIGHT\n");
				combinacao[n].tipo = straight;
				combinacao[n].valor = combinacao[n].carta[i+3].valor;
				return;
			}
		}
		else
			contador = 1;
	}

	// check to see if the hand is a three of kind
	contador = 1; maior = 0;
	for(i=6;i>0;i--){
		if(combinacao[n].carta[i].valor == combinacao[n].carta[i-1].valor){
			contador++;
			if(contador == 3){
				printf("It's a THREE OF KIND\n");
				combinacao[n].tipo = three_of_kind;
				combinacao[n].valor = combinacao[n].carta[i+1].valor;
				return;
			}
		}
		else
			contador = 1;
	}

	// check to see if the hand is a two pair CHECAR MAIOR
	contador = 1; maior = 0;
	int n_pair = 0;
	for(i=6;i>0;i--){
		if(combinacao[n].carta[i].valor == combinacao[n].carta[i-1].valor){
			contador++;
			if(contador == 2){
				if(maior < combinacao[n].carta[i].valor)
					maior = combinacao[n].carta[i].valor;
				n_pair++;
				if(n_pair == 2){
					printf("It's a TWO PAIR\n");
					combinacao[n].tipo = two_pair;
					combinacao[n].valor = maior;
					return;
				}
			}
		}
		else{
			contador = 1;
		}
	}

	// check to see if the hand is a one pair
	contador = 1; maior = 0;
	for(i=6;i>0;i--){
		if(combinacao[n].carta[i].valor == combinacao[n].carta[i-1].valor){
			contador++;
			if(contador == 2){
				printf("It's a ONE PAIR\n");
				combinacao[n].tipo = one_pair;
				combinacao[n].valor = combinacao[n].carta[i].valor;
				return;
			}
		}
		else
			contador = 1;
	}
	
	// retorna a maior carta
	maior = 0;
	for(i=6;i>-1;i--){
		if(combinacao[n].carta[i].valor > maior)
				maior = combinacao[n].carta[i].valor;
	}
	printf("It's a HIGHER CARD\n");
	combinacao[n].tipo = 0;
	combinacao[n].valor = maior;
}

int check_end_game(struct Player *jogador){
	int i, cont = 0, buff;
	
	for(i=0;i<4;i++)
		if(jogador[i].pot == 0)
			cont++;
		else
			buff = i;
	if(cont == 3)
		return buff;
	return 0;
}

// check to see who is the winner //
int check_winner(struct Mesa mesa, struct Player *jogador, int pot_mesa){
	int i, j;
	Combinacao_Carta combinacao[4];

	for(i=0;i<4;i++){
		for(j=0;j<2;j++){
			combinacao[i].carta[j] = jogador[i].mao.carta[j];
		}
		for(j=0;j<5;j++){
			combinacao[i].carta[j+2] = mesa.carta[j];
		}
		verifica_combinacao(combinacao, i);
		printf("Combination %d: ",i);
		printf("Kind: %d   Value: %d\n",combinacao[i].tipo, combinacao[i].valor);
//		for(j=0;j<7;j++)
//			printf("Carta %d: %d %d\n",j+1,combinacao[i].carta[j].valor,combinacao[i].carta[j].naipe);
//		printf("\n");
		getchar();
	}
	Combinacao_Carta maior;
	int winner;
	maior.tipo = 0;
	maior.valor = 0;
	for(i=3;i>-1;i--){
		if((combinacao[i].tipo > maior.tipo) && (jogador[i].estado_atual != 4)){
			maior = combinacao[i];
			winner = i;
		}
		else if((combinacao[i].tipo == maior.tipo) && (jogador[i].estado_atual != 4)){
			if(combinacao[i].valor > maior.valor){
				maior = combinacao[i];
				winner = i;
			}
		}
	}
	return winner;
}

// shows the screen of the game to the players who's not in turn //
void game_screen_play(struct Player *jogador, struct Jogo *jogo, int player_number){
	int i;

	system("clear");
	printf("I'm the player: %d\n",player_number);
	printf("Main pot: %d\n",jogo->pot_mesa);
	for(i=0;i<4;i++)
		printf("Player %d  State %d  Pot %d  Current bet %d\n",i,jogador[i].estado_atual,jogador[i].pot,jogador[i].aposta_atual);
	printf("\n");
	mostra_mao(jogador[player_number].mao);
	mostra_mesa(jogo->mesa);
	printf("\nPlayer's turn: %d\n",jogo->player_turn);
	printf("\nDo your move.\n");
}
// shows the screen of the game to the player who's in turn //
void game_screen_wait(struct Player *jogador, struct Jogo *jogo, int player_number){
	int i;

	system("clear");
	printf("I'm the player: %d\n",player_number);
	printf("Main pot: %d\n",jogo->pot_mesa);
	for(i=0;i<4;i++)
	printf("Player %d  State %d  Pot %d  Current bet %d\n",i,jogador[i].estado_atual,jogador[i].pot,jogador[i].aposta_atual);
	printf("\n");
	mostra_mao(jogador[player_number].mao);
	mostra_mesa(jogo->mesa);
	printf("\nPlayer's turn: %d\n",jogo->player_turn);
	printf("\nWaiting for the player %d...\n\n",jogo->player_turn);
}

// execute the player's turn //
int jogada(char *message, int player_number, struct Jogo *jogo, struct Player *jogador){
	int opcao, aposta = 0, i = player_number, cobrir = 0, jogada_valida = 0;
	char op[512];

	if(jogador[player_number].estado_atual != 4){
		do{
			i = player_number;
			do{
				printf("1-CHECK  2-BET  3-PAY  4- FOLD\n");
				printf("option: ");
				scanf("%s",op);
			} while (!(op[0]>'0' && op[0]<'5' && strlen(op)<3));   
			opcao = atoi(op);
			if(opcao == 1){
				do{
					if(i == 0)
						i = 3;
					else
						i--;
				}while((jogador[i].estado_atual != 2) && (i != player_number));
				if(i == player_number)
					jogada_valida = 1;
				else{
					printf("To continue you have to cover the bet or give up.\n");
					getchar();
				}
			}
			if(opcao == 2){
				printf("bet(1-1000): ");
				scanf("%d",&aposta);
				do{
					if(i == 0)
						i = 3;
					else
						i--;
				}while((jogador[i].estado_atual != 2) && (i != player_number));
				if(i != player_number){
					if(aposta + jogador[i].aposta_atual <= jogador[player_number].pot){
						jogo->pot_mesa = jogo->pot_mesa + (jogador[i].aposta_atual + aposta);
						jogador[player_number].pot = jogador[player_number].pot - (aposta + jogador[i].aposta_atual - jogador[player_number].aposta_atual);
						jogador[player_number].aposta_atual = jogador[i].aposta_atual + aposta;
						jogada_valida = 1;
					} else {
						printf("You don't have money enough to bet.\n");
						getchar();
					}
				}else{
					if(aposta <= jogador[player_number].pot){
						jogo->pot_mesa = jogo->pot_mesa + aposta;
						jogador[player_number].pot = jogador[player_number].pot - aposta;
						jogador[player_number].aposta_atual = aposta;
						jogada_valida = 1;
					} else {
						printf("You don't have money enough to bet.\n");
						getchar();
					}
				}
			}
			if(opcao == 3){
				do{
					if(i == 0)
						i = 3;
					else
						i--;
				}while((jogador[i].estado_atual != 2) && (i != player_number));
				if(i != player_number){
					jogo->pot_mesa = jogo->pot_mesa + (jogador[i].aposta_atual - jogador[player_number].aposta_atual);
					jogador[player_number].pot = jogador[player_number].pot - (jogador[i].aposta_atual - jogador[player_number].aposta_atual);
					jogador[player_number].aposta_atual = jogador[player_number].aposta_atual + (jogador[i].aposta_atual - jogador[player_number].aposta_atual);
					jogada_valida = 1;
				}else{
					printf("You can only pay if someone makes a bet.\n");
					getchar();
				}
			}
			if(opcao == 4)
				jogada_valida = 1;
		}while(jogada_valida == 0);
		jogador[player_number].estado_atual = opcao;
		for(i=0;i<4;i++)
			message[18+i] = (char) jogador[i].estado_atual + '0';
		escreve_pot(message,jogo->pot_mesa,jogador);
		escreve_aposta_atual(message, jogador);
	}else{
		for(i=0;i<4;i++)
			message[18+i] = (char) jogador[i].estado_atual + '0';
		escreve_pot(message,jogo->pot_mesa,jogador);
		escreve_aposta_atual(message, jogador);
		printf("You're in FOLD this round.\n");
	}
}

// deal the cards to the other players
void deal_cards(struct Jogo *jogo, struct Player *jogador, int player_number , int first_turn, char *mao_string, char *message, struct sockaddr_in *si_other, int s, int port_client, int *recv_len, char *buf, int *slen, char *server_ip){
	printf("Dealing the cards...Press any key\n");
	getchar();
	inicia_baralho(&(jogo->baralho));
	jogador[0].estado_atual = 0;
	jogador[1].estado_atual = 0;
	jogador[2].estado_atual = 0;
	jogador[3].estado_atual = 0;
	jogador[player_number].mao = seleciona_mao(&(jogo->baralho));
	jogo->mesa = seleciona_mesa(&(jogo->baralho));
	int i = player_number - 1;
	while(player_number != i){ // manda uma mensagem para cada dos outros jogaores
		if((i >= 0) && (i <= 3)){
			do{
				jogador[i].mao = seleciona_mao(&(jogo->baralho));
				escreve_mao(jogador[i].mao, mao_string);
				escreve_mesa(jogo->mesa, mao_string);
				escreve_estado(mao_string, jogador);
				if(first_turn)
					iniciate_pot(jogador);
				escreve_pot(mao_string, jogo->pot_mesa, jogador);
				jogador[0].aposta_atual = 0;
				jogador[1].aposta_atual = 0;
				jogador[2].aposta_atual = 0;
				jogador[3].aposta_atual = 0;
				escreve_aposta_atual(mao_string, jogador);
				strcpy(message, mao_string);
				message[2] = 'a';
				message[3] = '2';
				message[0] = player_number + '0';
				message[1] = i + '0';
				printf("Message to be sent: %s\n\n",message);
//				iniciate_pot(jogador);
				copia_vetor(jogador);
				jogo->pot_mesa_ant = jogo->pot_mesa;
			}while((envia_mesg(si_other, s, message, port_client, recv_len, buf, slen,server_ip) == 0) && (i < 4));
		}
		if(i == -1)
			i = 4;
		else
			i--;
	}
}

// recieve the cards from the dealer
recieve_cards(struct sockaddr_in *si_other, int s, char *message, int port_client, int *recv_len, char *buf, int *slen, int player_number, struct Jogo *jogo, struct Player *jogador, char *server_ip){
	printf("Waiting the dealer to finish...\n");
	int recebida = 0;
	char destino_ant = 'p';
	while(recebida < 3){
		if(receba_mesg(si_other, s, message, port_client, recv_len, buf, slen, player_number,server_ip)){
			if(buf[3] == '2'){
//				printf("mensagem recebida: %s\n",buf);
//				getchar();
				recebe_mao(buf, jogador, player_number);
				recebe_mesa(buf, &(jogo->mesa));
				recebe_estado(buf, jogador);
				recebe_pot(buf,&(jogo->pot_mesa),jogador);
				recebe_aposta_atual(buf, jogador);
				if(buf[1] != destino_ant)
					recebida++;
			}
		}
		else if((buf[1] != destino_ant) || (buf[3] == '1')){
				recebida++;
//				printf("buf[1] = %c e destino_ant = %c\n",buf[1],destino_ant);
//				getchar();
			}
		destino_ant = buf[1];
	}
}

//------// Main //------//

int main(int argc, char **argv){
	// declarations to the poker game
	Player jogador[4];
	Jogo jogo;
	int player_number, i;
	char mao_string[MESG_SIZE] = "00000000";
	memset(jogador,0,sizeof(jogador));

	// declarations to the sockets
	struct sockaddr_in si_me, si_other;
	int s, slen = sizeof(si_other) , recv_len, port_server, port_client;
	char buf[MESG_SIZE], message[MESG_SIZE], server_ip[20];

	if(!testa_argumentos(argc, argv, &player_number, server_ip))
		return 0;

	seleciona_portas(&port_server, &port_client, player_number);

	if(!setup_socket(&s, &si_me, &si_other, port_server, port_client, server_ip))
		return 0;

	// iniciates the game
	printf("Iniciating Poker!\n");
	int first_turn = 1;
	jogo.fim_jogo = 0;
	jogo.fim_turno = 0;
	jogo.fim_rodada = 0;
	jogo.dealer = 0;
	while(!jogo.fim_jogo){

		jogo.player_turn = 0;

		if(player_number == jogo.dealer){	// se o jogador for o dealer ele prepara as cartas
			deal_cards(&jogo, jogador, player_number , first_turn++, mao_string, message, &si_other, s, port_client, &recv_len, buf, &slen, server_ip);
		}else{ // caso o jogador nao seja o dealer ele aguarda as cartas serem dadas
			recieve_cards(&si_other, s, message, port_client, &recv_len, buf, &slen, player_number, &jogo, jogador, server_ip);
		}
		int rodada;
		iniciate_turn(&rodada, &jogo, &first_turn); // iniciates the turn
		while(!jogo.fim_turno){	// beggining of the round
			jogo.fim_rodada = 0;
			if(rodada == 1) // se for a primeira rodada
				show_desk(&(jogo.mesa), 3);
			else if(rodada == 2) // se for a segunda rodada
				show_desk(&(jogo.mesa), 4);
			else if(rodada == 3) // se for a terceira rodada
				show_desk(&(jogo.mesa), 5);
			else if(rodada == 4){ // se for a quarta rodada
				if(player_number == jogo.dealer){
					message[3] = (int) check_winner(jogo.mesa, jogador, jogo.pot_mesa) + '0';
					envia_todos(&player_number,message,&si_other,s,port_client,&recv_len,buf,&slen,server_ip);
					printf("CHECK WHO WON\n");
					printf("The winner is: %c\n",message[3]);
					jogador[(int) message[3] - '0'].pot = jogador[(int) message[3] - '0'].pot + jogo.pot_mesa;
//					getchar();
				}else{
					int recebi = 0, winner;
					char destino_ant = 'p';
					printf("Waiting for the dealer to tell who won.\n");
					while(recebi < 3){
						if(receba_mesg(&si_other, s, message, port_client, &recv_len, buf, &slen, player_number,server_ip)){
							winner = (int) buf[3] - '0';
							if(buf[1] != destino_ant)
								recebi++;
						}
						else if((buf[1] != destino_ant) || (buf[3] == '1'))
							recebi++;
						destino_ant = buf[1];
					}
					printf("The winner is: %d\n",winner);
					jogador[winner].pot = jogador[winner].pot + jogo.pot_mesa;
//					getchar();
				}
				jogo.fim_turno = 1;
				jogo.fim_rodada = 1;
			}

			zera_estado_atual(jogador);
			zera_aposta_atual(jogador);
			while(!jogo.fim_rodada){
				if(jogo.player_turn == player_number){ // caso a vez seja deste jogador
					while(jogo.player_turn == player_number){
						game_screen_play(jogador, &jogo, player_number);
						strcpy(message,"xxx0xxxxxxxxxxxxxxxxxxx");
						jogada(message, player_number, &jogo, jogador);
						getchar();
						envia_todos(&player_number,message,&si_other,s,port_client,&recv_len,buf,&slen,server_ip);
						count_to_four(&(jogo.player_turn));
					}
					if(check_end_turn(jogador) == 1){
						rodada++;
						jogo.fim_rodada = 1;
					}
				}else{ // caso a vez seja de outro jogador
					int recebi = 0;
					char destino_ant = 'p';
					while(recebi < 3){
						game_screen_wait(jogador, &jogo, player_number);
						// se for para o proprio jogador ele reage a tal mensagem
						if(receba_mesg(&si_other, s, message, port_client, &recv_len, buf, &slen, player_number,server_ip)){
							if(buf[3] == '0'){
								recebe_estado(buf, jogador);
								copia_vetor(jogador);
								jogo.pot_mesa_ant = jogo.pot_mesa;
								recebe_pot(buf, &(jogo.pot_mesa), jogador);
								recebe_aposta_atual(buf, jogador);
								count_to_four(&(jogo.player_turn));
								if(buf[1] != destino_ant)
									recebi++;
							}						
						}
						else if((buf[1] != destino_ant) || (buf[3] == '1'))
							recebi++;
						destino_ant = buf[1];
					}
					if(check_end_turn(jogador) == 1){
						rodada++;
						jogo.fim_rodada = 1;
					}
				}
			}
			// fim da rodada
		} 
		// fim do turno
		count_to_four(&(jogo.dealer));
		jogo.fim_jogo = check_end_game(jogador);
	}
	printf("End of game. The winner is: %d\n",jogo.fim_jogo);
	// fim do jogo
}
