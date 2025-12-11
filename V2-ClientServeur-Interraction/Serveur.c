#include <stdio.h>      // pour printf(), perror()
#include <stdlib.h>     // pour exit(), malloc(), free()
#include <sys/socket.h> // pour socket(), bind(), listen(), accept()
#include <netdb.h>      // pour getaddrinfo(), struct addrinfo
#include <string.h>     // pour memset(), strlen()
#include <unistd.h>     // pour close(), read(), write()
#include <stdarg.h>
#include <arpa/inet.h>

#define TEST "Hello World\0"
#define MAXLENGTH 256

void printf_RGB(int r, int g, int b, const char* format, ...);

int main(void){
    /////////////////////////////////////////////////////////////////////////////////////////
    // Vlauer qui vas contenir les descripteur de socket
    int sockfd;
    // hints = Structure qui contiendra les critère pour choisir une adresse (TCP,IPv4 ...)
    // *res = TABLEAU que getaddrinfo
    struct addrinfo hints, *res;
    // Stock les valeur de retour -> En cas d'erreur = -1
    int ecode;
    /////////////////////////////////////////////////////////////////////////////////////////
    // On mes tout les valeur de hints pour eviter les artefacte
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    //AI_PASSIVE signifie : "Je veux une adresse pour mon serveur (pour bind())".
    hints.ai_flags = AI_PASSIVE;      // option pour bind plus tard
    /////////////////////////////////////////////////////////////////////////////////////////
    // Affcte a la structure res tout les adresse prete a étre bind
    ecode = getaddrinfo("192.168.1.120","40002",&hints,&res);
    if(ecode < 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
        exit(1);
    }
    printf_RGB(0,255,0,"[OK] Liste d'adresse crée avec succès\n");

    // On crée un socket avec tout les paramétre souhaitée 
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockfd < 0){
        printf_RGB(255,0,0,"[KO] Erreur création socket\n");
        exit(1);
    }
    printf_RGB(0,255,0,"[OK] Socket TCP créée avec succès\n");

    // Permet de relancer le serveur sans attendre que le port soit libéré
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        printf_RGB(255,0,0,"[KO] Erreur setsockopt(SO_REUSEADDR)\n");
        exit(1);
    }
    /////////////////////////////////////////////////////////////////////////////////////////
    // Liaison du socket à l'adresse
    if(bind(sockfd,res->ai_addr, res->ai_addrlen) < 0){
        printf_RGB(255,0,0,"[KO] Erreur bind\n");
        exit(1);
    }
    printf_RGB(0,255,0,"[OK] Socket liée au port avec succès \n");

    // En attente d'un client ...
    int backlog = 5; // nombre max de clients en attente
    if (listen(sockfd, backlog) < 0) {
        printf_RGB(255,0,0,"[KO] Erreur liste");
        exit(1);
    }
    printf_RGB(0,0,255,"[INFO] Serveur en écoute...\n");
    /////////////////////////////////////////////////////////////////////////////////////////
    int clientfd;
    char* ip_client = (char*)calloc(INET_ADDRSTRLEN,sizeof(char));
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    /* accept -> Bloque le programme jusqu'a communiquer avec un client
     * sockfd -> Le socket qu'on a bindé 
     * (struct sockaddr*)&client_addr -> c'est ici que le programme vas écrire l'adresse du client
     * &addr_len -> La taille max ou on peut écrir l'adresse du client
     */
    clientfd = accept(sockfd, (struct sockaddr*) &client_addr, &addr_len);
    inet_ntop(AF_INET, &client_addr.sin_addr, ip_client, INET_ADDRSTRLEN);
    if(clientfd < 0){
        printf_RGB(255,0,0,"[KO] Erreur accept");
        exit(1);
    }
    printf_RGB(0,255,0,"[OK] Client connecté (%s)\n",ip_client);

    int n = write(clientfd, TEST, strlen(TEST));
    if(n < 0){
        perror("Erreur write");
        exit(1);
    }

    char* buffer = (char*)calloc(MAXLENGTH,sizeof(char));
    int recv_len;

    // Lire la TOTALITEE du message
    int total = 0;
    while(1){
        recv_len = recv(clientfd, &buffer[total], 1, 0); // lire 1 octet
        if(buffer[total] == '\n'){ // détection fin de message
            buffer[total] = '\0';   // termine la chaîne
            break;
        }
        total += recv_len;
    }

    printf("Message Client = %s\n",buffer);

    /////////////////////////////////////////////////////////////////////////////////////////

    freeaddrinfo(res);
    close(sockfd); // fermeture propre
    return 0;
}



void printf_RGB(int r, int g, int b, const char* format, ...) {
    va_list args;
    va_start(args, format);

    printf("\033[38;2;%d;%d;%dm", r, g, b);

    vprintf(format, args);

    printf("\033[0m");

    va_end(args);
}