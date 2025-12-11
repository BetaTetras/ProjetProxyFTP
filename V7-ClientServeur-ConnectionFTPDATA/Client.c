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
int commandeProxy();
void printf_RGB(int r, int g, int b, const char* format, ...);

int main() {
    ////////////////////////////////////////////////////////////////////////////////////////
    printf_RGB(0,255,0,"[INFO] Initialisation client...\n");

    int sockCTRL = connecterSocket("192.168.1.120", 40010, "CONTROLE");
    int sockDATA = connecterSocket("192.168.1.120", 40011, "DATA");

    ////////////////////////////////////////////////////////////////////////////////////////
    char* buffer = (char*)calloc(MAXLENGHT,sizeof(char));
    char* Commande = (char*)calloc(MAXLENGHT,sizeof(char));
    int n = read(sockDATA,buffer,MAXLENGHT);
    if(n<=0){
        printf_RGB(0,255,0,"[KO] Erreur lecture\n");
        return 1;
    }
    // Message debienvenue sur le serveur
    printf_RGB(0,255,0,"%s\n\n",buffer);

    while(1){
        int sortie = commandeProxy(sockCTRL);
        if(sortie == 1){
            break;
        }
        // Lire toutes les données disponibles du socket DATA
        while((n = read(sockDATA, buffer, MAXLENGHT-1)) > 0){
            buffer[n] = '\0'; // Terminer la chaîne
            printf("%s", buffer);

            // Si moins que MAXLENGHT lu, on suppose que le serveur a fini
            if(n < MAXLENGHT-1) break;
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

int commandeProxy(int sock){
    char* Commande = (char*)calloc(MAXLENGHT,sizeof(char));
    printf("ftpProxy> ");
    fgets(Commande,MAXLENGHT,stdin);
    if(strcmp(Commande,"bye\n") == 0){
        free(Commande);
        return 1;
    }
    Commande[strcspn(Commande, "\n")] = '\0';
    write(sock,Commande,strlen(Commande));
    free(Commande);
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
