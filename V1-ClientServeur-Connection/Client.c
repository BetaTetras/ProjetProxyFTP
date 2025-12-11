#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    // Valeur qui vas contenir les descripteur de socket
    int sockfd;
    // Structure qui contient l’adresse du serveur
    struct sockaddr_in server_addr;
    // Buffer pour le message recu
    char buffer[1024];
    // Variable contenant le nombre d’octets reçus
    int recv_len;

    // Création d'un socket (IPv4, TCP ...)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erreur création socket");
        return 1;
    }

   // On mes tout les valeur de hints pour eviter les artefacte
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;    // IPv4
    server_addr.sin_port = htons(40002); // Port 40002

    // Convertit l’adresse IP "88.179.6.57" en format binaire et Stocke le résultat dans server_addr.sin_addr.
    if (inet_pton(AF_INET, "88.179.6.57", &server_addr.sin_addr) <= 0) {
        perror("Erreur inet_pton");
        close(sockfd);
        return 1;
    }

    /* connect -> Envois une requete TCP a une adresse
     * sockfd -> Socket de la connection
     * (struct sockaddr*)&server_addr -> Emplacement memoire ou se trouve les info du serveur
     * sizeof(server_addr) -> Longueur de la structure server_addr.
    */
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erreur connexion");
        close(sockfd);
        return 1;
    }
    printf("Connecté au serveur !\n");

    /* recv -> lit des données depuis une socket connectée (ici sockfd côté client).
     * sockfd -> Toujour le socket
     * buffer -> L'adresse memoire ou stockée les donnée recu
     * sizeof(buffer)-1 -> Nombre maximum d’octets à lire.
     * 0 -> Flags (Aucune option en gros)
    */
    recv_len = recv(sockfd, buffer, sizeof(buffer)-1, 0);
    if (recv_len > 0) {
        buffer[recv_len] = '\0';
        printf("Message reçu : %s\n", buffer);
    } else {
        printf("Aucune donnée reçue ou erreur.\n");
    }

    close(sockfd);
    return 0;
}
