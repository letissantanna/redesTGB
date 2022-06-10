#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include <netinet/sctp.h> // Novo include

#define ECHOMAX 1024

int main(int argc, char *argv[]) {

	/* Checagem de parametros. Aborta se número de parametros está errado ou se porta é um string invalido. */
	if (argc != 3 || atoi(argv[2]) == 0) {
		printf("Parametros:<remote_host> <remote_port> \n");
		exit(1);
	}

	/* Variáveis Locais */
	int rem_sockfd;
	char linha[ECHOMAX];
    char aux_exit[ECHOMAX];
    
    /* Variaveis pro pipe */
    FILE *fp; 
    char path[1035];

	/* Construcao da estrutura do endereco local */
	struct sockaddr_in rem_addr = {
		.sin_family = AF_INET, /* familia do protocolo*/
		.sin_addr.s_addr = inet_addr(argv[1]), /* endereco IP local */
		.sin_port = htons(atoi(argv[2])), /* porta local  */
		//sin_zero = 0, /* por algum motivo pode se botar isso em 0 usando memset() */
	};
   	/* Cria o socket para enviar e receber fluxos */
	rem_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (rem_sockfd < 0) {
		perror("Criando stream socket");
		exit(1);
	}
	printf("> Conectando no servidor '%s:%s'\n", argv[1], argv[2]);

   	/* Estabelece uma conexao remota */
	if (connect(rem_sockfd, (struct sockaddr *) &rem_addr, sizeof(rem_addr)) < 0) {
		perror("Conectando stream socket");
		exit(1);
	}

    printf("Você entrou na rede :D\n");

	do  {
        char menu[3];
        printf("\nAguarde a central...\n");

        sctp_recvmsg(rem_sockfd, menu, sizeof(linha), NULL, 0, 0, 0);

        switch(menu[0]){   //'C' manda comandos,   'S' serve
            case('C'):
                printf("\n--------- COMANDANDO --------- \n");
                printf("Digite o comando que deseja executar: "); 
                fgets (linha, ECHOMAX, stdin);
                linha[strcspn(linha, "\n")] = 0;
                sctp_sendmsg(rem_sockfd, &linha, sizeof(linha), NULL, 0, 0, 0, 0, 0, 0);
                strcpy(aux_exit, linha);
                
                /* faz o que ele mesmo mandou */
                system(linha);
                
                /* Le as saída do outro */
                do{           
                    sctp_recvmsg(rem_sockfd, &linha, sizeof(linha), NULL, 0, 0, 0);
                    printf("%s", linha);
                }while(strcmp(linha, "---")); 
                
                memset(linha, '\0', ECHOMAX);
                break; 
            

        /* servindo */
            case('S'):
                printf("\n--------- SERVINDO ---------\n");
                sctp_recvmsg(rem_sockfd, &linha, sizeof(linha), NULL, 0, 0, 0); //recebe comando peer
                printf("Recebido: %s\n", linha);

                /* Abre o comando para leitura */
                fp = popen(linha, "r");
                
                /* Lê o output linha a linha e envia. */
                while (fgets(path, sizeof(path), fp) != NULL) {
                    printf("%s", path);
                    sctp_sendmsg(rem_sockfd, path, sizeof(linha), NULL, 0, 0, 0, 0, 0, 0); //envia resposta
                }
                sctp_sendmsg(rem_sockfd, "---", sizeof(linha), NULL, 0, 0, 0, 0, 0, 0);
                    
                /* close */
                pclose(fp);
                
                break;
            default:
                printf("Comando estranho... %s\n Tentando novamente!\n\n", menu);
        }
        memset(menu, '\0', 3);

	}while(strcmp(aux_exit,"exit"));
	
	close(rem_sockfd);
}
