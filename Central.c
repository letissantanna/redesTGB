#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/sctp.h> // Novos includes
#include <sys/types.h>
#include <errno.h>

#define ECHOMAX 1024

int main(int argc, char *argv[]) {
	/* Variáveis para a lógica dos peers */
    int qnts_peers = 2;
    int loc_newsockfd[qnts_peers];
    int i_peer; //indice peer

	/* Variaveis Locais */
	int loc_sockfd, loc_newsockfd2, tamanho;
	char linha[ECHOMAX];
    char aux_linha[ECHOMAX];
    char aux_exit[ECHOMAX];

    /* Variaveis pro pipe */
    FILE *fp; //pipe
    char path[1035]; 

	/* Checagem de parametros. */
	if (argc != 2) {
		printf("Parametros: <local_port> \n");
		exit(1);
	}

	/* Construcao da estrutura do endereco local */
	struct sockaddr_in loc_addr = {
		.sin_family = AF_INET, /* familia do protocolo */
		.sin_addr.s_addr = INADDR_ANY, /* endereco IP local */
		.sin_port = htons(atoi(argv[1])), /* porta local */
		//.sin_zero = 0, /* por algum motivo pode se botar isso em 0 usando memset() */
	};
	/* Novas Estrutura: sctp_initmsg, comtém informações para a inicialização de associação*/
	struct sctp_initmsg initmsg = {
		.sinit_num_ostreams = 5, /* Número de streams que se deseja mandar. */
  		.sinit_max_instreams = 5, /* Número máximo de streams se deseja receber. */
  		.sinit_max_attempts = 4, /* Número de tentativas até remandar INIT. */
  		/*.sinit_max_init_timeo = 60000, Tempo máximo em milissegundos para mandar INIT antes de abortar. Default 60 segundos.*/
	};

	/* Cria o socket para enviar e receber datagramas */
	loc_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP); // Mudança protocolo '0' => 'IPPROTO_SCTP'
	
	if (loc_sockfd < 0) {
		perror("Criando stream socket");
		exit(1);
	}
   	/* Bind para o endereco local*/
	if (bind(loc_sockfd, (struct sockaddr *) &loc_addr, sizeof(struct sockaddr)) < 0) {
		perror("Ligando stream socket");
		exit(1);
	}
	
	/* SCTP necessita usar setsockopt, */
	if (setsockopt (loc_sockfd, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof (initmsg)) < 0){
		perror("setsockopt(initmsg)");
		exit(1);
  	}

	/* parametros(descritor socket,	numeros de conexoes em espera sem serem aceites pelo accept)*/
	listen(loc_sockfd, initmsg.sinit_max_instreams); // Mudado para initmsg.sinit_max_instreams.
	printf("> esperando %i peers se entrarem...\n", qnts_peers);

    tamanho = sizeof(struct sockaddr_in);

    /* Faz a conexão com quantos socketes foram determinados*/
	for(int i = 0; i < qnts_peers; i++) 
		loc_newsockfd[i] = accept(loc_sockfd, (struct sockaddr *)&loc_addr, &tamanho);
	
    printf("Todos os Peers foram inscritos!\n");

	do  {  /* Inicio do sistema */
        /* Recebe qual peer que vai falar ou se quer add novo peer */
		do{   
            printf("\n\nDigite qual Peer vai mandar o comado (1 a %i) ou 0 para add novo peer: ", qnts_peers);
            scanf("%d", &i_peer); //recebe quem está comandando/falando no sistema

            if(i_peer == 0){  //se quer add
                loc_newsockfd[qnts_peers++] = accept(loc_sockfd, (struct sockaddr *)&loc_addr, &tamanho);
                printf("Peer adicionado \nTotal de %i peers na rede\n", qnts_peers);
            }
            
		}while(i_peer<1 || i_peer>qnts_peers); 

        /* arruma opção digitada para o índice referente */  
        i_peer--; 
        
        /* Determina quem ouve e quem fala */
        for(int i = 0; i < qnts_peers; i++){  
            if(i==i_peer)   //se for o peer escolhido, manda que fale
                sctp_sendmsg(loc_newsockfd[i], "C", sizeof(linha), NULL, 0, 0, 0, 0, 0, 0);
            else //senão, serve (ouve)
                sctp_sendmsg(loc_newsockfd[i], "S", sizeof(linha), NULL, 0, 0, 0, 0, 0, 0);

        }
        printf("-------- RECEBENDO COMANDOS -------\n");    
            

        /* recebe o comando do que comanda */
        sctp_recvmsg(loc_newsockfd[i_peer], &linha, sizeof(linha), NULL, 0, 0, 0);
        strcpy(aux_exit, linha);    //copia para auxiliar de saída
        printf("Recebido: %s\n", linha);

        /* envia comando pros ouvintes */ 
        for(int i = 0; i < qnts_peers;){
            if( i =! i_peer ){   
                sctp_sendmsg(loc_newsockfd[i], &linha, sizeof(linha), NULL, 0, 0, 0, 0, 0, 0); //envia o comando
                do{            
                    sctp_recvmsg(loc_newsockfd[i], &linha, sizeof(linha), NULL, 0, 0, 0); //recebe a resposta
                    if(strcmp(linha, "---") != 0){
                        sprintf (aux_linha, "%d | ", i);
                        strcat (aux_linha, linha);
                        printf("%s", aux_linha);
                        
						sctp_sendmsg(loc_newsockfd[i_peer], &aux_linha, sizeof(linha), NULL, 0, 0, 0, 0, 0, 0); //envia resposta
                    }
                }while(strcmp(linha, "---"));
                sctp_sendmsg(loc_newsockfd[i_peer], "---", sizeof(linha), NULL, 0, 0, 0, 0, 0, 0); //envia saida
            }
            i++;
        }
        
        memset(linha, '\0', ECHOMAX); //limpa memoria
	}while(strcmp(aux_exit,"exit"));
	/* fechamento do socket local */ 
	close(loc_sockfd);
	for(int i = 0; i < qnts_peers; i++)
        close(loc_newsockfd[i]);
}


