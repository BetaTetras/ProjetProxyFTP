#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdarg.h>

#define MAXLENGHT 256
#define TEST "[...] we have a city to burn.\0"

void printf_RGB(int r, int g, int b, const char* format, ...);

int créeSocket(const char* ip,const char* port);
int connecterFTP(const char* ip);

int main(void){
    int srvCTRL = créeSocket("0.0.0.0", "40010");
    int srvDATA = créeSocket("0.0.0.0", "40011");
    printf_RGB(0,0,255,"[INFO] Initialisation serveurs...\n");
    ////////////////////////////////////////////////////////////////////////////////////////

    while(1){
        struct sockaddr_in addrCONTROLE;
        socklen_t lenCONTROLE = sizeof(addrCONTROLE);
        int cliCTRL = accept(srvCTRL, (struct sockaddr*)&addrCONTROLE, &lenCONTROLE);

        char ipC[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addrCONTROLE.sin_addr, ipC, sizeof(ipC));
        printf_RGB(0,255,0,"[OK] Client CONTROLE connecté (%s)\n", ipC);
        ////////////////////////////////////////////////////////////////////////////////////////
        struct sockaddr_in addrDATA;
        socklen_t lenDATA = sizeof(addrDATA);
        int cliDATA = accept(srvDATA, (struct sockaddr*)&addrDATA, &lenDATA);

        char ipD[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addrDATA.sin_addr, ipD, sizeof(ipD));
        printf_RGB(0,255,0,"[OK] Client DATA connecté (%s)\n", ipD);

        pid_t pid = fork();

        if(pid == 0){
            int sockFTP = connecterFTP("ftp.fr.debian.org");

            char* bufferClient = (char*)calloc(MAXLENGHT,sizeof(char));
            char* bufferFTP = (char*)calloc(MAXLENGHT,sizeof(char));

            // Lire le message de bienvenue du serveur FTP et le renvoyer au client DATA
            int nFTP = read(sockFTP, bufferFTP, MAXLENGHT);
            if(nFTP > 0){
                write(cliDATA, bufferFTP, nFTP);
            }

            while(1){
                memset(bufferClient, 0, MAXLENGHT);
                memset(bufferFTP, 0, MAXLENGHT);

                int nClient = read(cliCTRL, bufferClient, MAXLENGHT);
                if(nClient <= 0){
                    printf_RGB(255,0,0,"[KO] Erreur lecture CLIENT\n");
                    free(bufferClient);
                    free(bufferFTP);
                    break;
                }

                // Ajouter CRLF à la commande si ce n'est pas déjà le cas
                if(bufferClient[nClient-1] != '\n') {
                    bufferClient[nClient] = '\r';
                    bufferClient[nClient+1] = '\n';
                    nClient += 2;
                }

                write(sockFTP,bufferClient,nClient);

                int nFTP = read(sockFTP,bufferFTP,MAXLENGHT);
                if(nFTP<=0){
                    printf_RGB(0,255,0,"[KO] Erreur lecture FTP\n");
                    break;
                }
                write(cliDATA,bufferFTP,nFTP);
            }

            printf_RGB(0,0,255,"[INFO] End connection serveurs...\n");

            free(bufferClient);
            free(bufferFTP);
            close(cliCTRL);
            close(cliDATA);
            exit(0);
        }else{
            close(cliCTRL);
            close(cliDATA);
        }
    }

    return 0;
}
    

int créeSocket(const char* ip,const char* port){
    int ecode;
    int sockfd;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    ecode = getaddrinfo(ip, port, &hints, &res);
    if(ecode != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
        exit(1);
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if(bind(sockfd, res->ai_addr, res->ai_addrlen) < 0){
        perror("bind");
        exit(1);
    }

    listen(sockfd, 5);
    freeaddrinfo(res);
    return sockfd;
}

int connecterFTP(const char* hostname) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_STREAM;

    int ecode = getaddrinfo(hostname, "21", &hints, &res);
    if (ecode != 0) {
        printf_RGB(255,0,0,"[KO] getaddrinfo: %s\n", gai_strerror(ecode));
        return -1;
    }

    int sockFTP = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (connect(sockFTP, res->ai_addr, res->ai_addrlen) < 0){
        perror("[KO] connect");
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return sockFTP;
}


void printf_RGB(int r, int g, int b, const char* format, ...) {
    va_list args;
    va_start(args, format);

    printf("\033[38;2;%d;%d;%dm", r, g, b);
    vprintf(format, args);
    printf("\033[0m");

    va_end(args);
}
