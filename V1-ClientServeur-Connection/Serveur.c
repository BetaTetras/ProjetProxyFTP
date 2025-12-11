#include <stdio.h>      // pour printf(), perror()
#include <stdlib.h>     // pour exit(), malloc(), free()
#include <sys/socket.h> // pour socket(), bind(), listen(), accept()
#include <netdb.h>      // pour getaddrinfo(), struct addrinfo
#include <string.h>     // pour memset(), strlen()
#include <unistd.h>     // pour close(), read(), write()

int main(void){
    // Vlauer qui vas contenir les descripteur de socket
    int sockfd;
    // hints = Structure qui contiendra les critère pour choisir une adresse (TCP,IPv4 ...)
    // *res = TABLEAU que getaddrinfo
    struct addrinfo hints, *res;
    // Stock les valeur de retour -> En cas d'erreur = -1
    int ecode;

    // On mes tout les valeur de hints pour eviter les artefacte
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    //AI_PASSIVE signifie : "Je veux une adresse pour mon serveur (pour bind())".
    hints.ai_flags = AI_PASSIVE;      // option pour bind plus tard

    // Affcte a la structure res tout les adresse prete a étre bind
    ecode = getaddrinfo("192.168.1.120","40002",&hints,&res);
    if(ecode < 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
        exit(1);
    }

    // On crée un socket avec tout les paramétre souhaitée 
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockfd < 0){
        perror("Erreur création socket");
        exit(1);
    }
    printf("Socket TCP créée avec succès ! Descripteur = %d\n", sockfd);

    // Liaison du socket à l'adresse
    if(bind(sockfd,res->ai_addr, res->ai_addrlen) < 0){
        perror("Erreur bind");
        exit(1);
    }
    printf("Socket liée au port avec succès !\n");

    // En attente d'un client ...
    int backlog = 5; // nombre max de clients en attente
    if (listen(sockfd, backlog) < 0) {
        perror("Erreur listen");
        exit(1);
    }
    printf("Serveur en écoute...\n");

    int clientfd;
    struct sockaddr_storage client_addr;
    socklen_t addr_len = sizeof(client_addr);

    /* accept -> Bloque le programme jusqu'a communiquer avec un client
     * sockfd -> Le socket qu'on a bindé 
     * (struct sockaddr*)&client_addr -> c'est ici que le programme vas écrire l'adresse du client
     * &addr_len -> La taille max ou on peut écrir l'adresse du client
     */
    clientfd = accept(sockfd, (struct sockaddr*) &client_addr, &addr_len);
    if(clientfd < 0){
        perror("Erreur accept");
        exit(1);
    }
    printf("Client connecté !\n");

    char buffer[] = "Hello World\n";
    int n = write(clientfd, buffer, strlen(buffer));
    if(n < 0){
        perror("Erreur write");
        exit(1);
    }
    printf("Message envoyé au client.\n");

    freeaddrinfo(res);
    close(sockfd); // fermeture propre
    return 0;
}