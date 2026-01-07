#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdarg.h>

#define MAXLENGHT 256
#define TEST "Wake the fuck up samurai, [...]\0"

int connecterSocket(const char* ip,int port,const char* libellé);
int commandeProxy(int sockCTRL, int sockDATA);
void printf_RGB(int r, int g, int b, const char* format, ...);

int main() {
    ////////////////////////////////////////////////////////////////////////////////////////
    printf_RGB(0,255,0,"[INFO] Initialisation client...\n");

    int sockCTRL = connecterSocket("192.168.1.120", 40010, "CONTROLE");
    int sockDATA = connecterSocket("192.168.1.120", 40011, "DATA");

    ////////////////////////////////////////////////////////////////////////////////////////
    char* buffer = (char*)calloc(MAXLENGHT,sizeof(char));
    char* Commande = (char*)calloc(MAXLENGHT,sizeof(char));
    int n = read(sockCTRL,buffer,MAXLENGHT);
    if(n<=0){
        printf_RGB(0,255,0,"[KO] Erreur lecture\n");
        return 1;
    }
    // Message debienvenue sur le serveur
    printf_RGB(0,255,0,"%s\n\n",buffer);

    while(1){
        int sortie = commandeProxy(sockCTRL,sockDATA);
        if(sortie == 1){
            break;
        }
    }

    close(sockCTRL);
    close(sockDATA);
    free(Commande);
    return 0;
}
/////////////////////////////////////////////////////////////////////////////////////
int connecterSocket(const char* ip,int port,const char* libellé){
    int sock;
    struct sockaddr_in server;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    printf_RGB(0,255,0,"[OK] Socket créée {%s}\n", libellé);

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    inet_pton(AF_INET, ip, &server.sin_addr);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("connect");
        exit(1);
    }

    printf_RGB(0,255,0,"[OK] Connecté à {%s}\n", libellé);
    return sock;
}

int commandeProxy(int sockCTRL, int sockDATA){
    char* Commande = (char*)calloc(MAXLENGHT,sizeof(char));
    char* Response = (char*)calloc(MAXLENGHT,sizeof(char));
    
    printf("ftpProxy> ");
    fgets(Commande, MAXLENGHT, stdin);
    
    if(strcmp(Commande,"bye\n") == 0){
        free(Commande);
        free(Response);
        return 1;
    }
    
    // Détecter si c'est une commande LIST
    int isListCommand = (strncmp(Commande, "LIST", 4) == 0);
    
    // Envoyer la commande
    Commande[strcspn(Commande, "\n")] = '\0';
    write(sockCTRL, Commande, strlen(Commande));
    write(sockCTRL, "\r\n", 2);
    
    // TOUJOURS lire la première réponse sur CTRL (ex: "150 Opening...")
    int n = read(sockCTRL, Response, MAXLENGHT-1);
    if(n > 0){
        Response[n] = '\0';
        printf("%s", Response);
    }
    
    // Si c'est LIST, lire les données du canal DATA
    if(isListCommand){
        char dataBuffer[MAXLENGHT];
        printf_RGB(0,255,255,"========== Listing ==========\n");
        
        // Maintenant on lit les données
        while((n = read(sockDATA, dataBuffer, MAXLENGHT-1)) > 0){
            dataBuffer[n] = '\0';
            printf("%s", dataBuffer);
        }
        printf_RGB(0,255,255,"=============================\n");
        
        // Lire la réponse finale "226 Transfer complete"
        n = read(sockCTRL, Response, MAXLENGHT-1);
        if(n > 0){
            Response[n] = '\0';
            printf("%s", Response);
        }
    }
    
    free(Commande);
    free(Response);
    return 0;
}
/////////////////////////////////////////////////////////////////////////////////////
void printf_RGB(int r, int g, int b, const char* format, ...) {
    va_list args;
    va_start(args, format);

    printf("\033[38;2;%d;%d;%dm", r, g, b);
    vprintf(format, args);
    printf("\033[0m");

    va_end(args);
}
