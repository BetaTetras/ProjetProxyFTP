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

int main(void){
    ////////////////////////////////////////////////////////////////////////////////////////
    printf_RGB(0,0,255,"[INFO] Initialisation serveurs...\n");
    while(1){
        int srvCTRL = créeSocket("192.168.1.120", "40002");
        int srvDATA = créeSocket("192.168.1.120", "40003");
        ////////////////////////////////////////////////////////////////////////////////////////
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
        /////////////////////////////////////////////////////////////////////////////////////
        char* Reponse = (char*)calloc(MAXLENGHT,sizeof(char));
        char* buffer = (char*)calloc(MAXLENGHT,sizeof(char));
        while(1){

            int n = read(cliCTRL,buffer,MAXLENGHT-1);
            if(n <= 0) break;
            buffer[n] = '\0';

            if(strcmp(buffer,"PORT\0") == 0){
                strcpy(Reponse,"PORT - OK\n");
                printf_RGB(255,255,0,"[INFO] Client %s Type : PORT\n",ipC);
                write(cliDATA,Reponse,strlen(Reponse));
            }else if(strcmp(buffer,"PASV\0") == 0){
                strcpy(Reponse,"PASV - OK\n");
                printf_RGB(255,128,0,"[INFO] Client %s Type : PORT\n",ipC);
                write(cliDATA,Reponse,strlen(Reponse));
            }else {
                strcpy(Reponse,"Erreur\n");
                printf_RGB(255,0,0,"[INFO] Client %s Type : ERREUR\n",ipC);
                write(cliDATA,Reponse,strlen(Reponse));
            }
        }
        printf_RGB(0,0,255,"[INFO] End connection serveurs...\n");

        close(cliCTRL);
        close(cliDATA);
        close(srvCTRL);
        close(srvDATA);


        free(Reponse);
        free(buffer);
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

void printf_RGB(int r, int g, int b, const char* format, ...) {
    va_list args;
    va_start(args, format);

    printf("\033[38;2;%d;%d;%dm", r, g, b);
    vprintf(format, args);
    printf("\033[0m");

    va_end(args);
}
