#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdarg.h>

#define MAXLENGHT 4096
#define TEST "[...] we have a city to burn.\0"

typedef struct{ char* IP1; 
    char* IP2; 
    char* IP3; 
    char* IP4; 
    char* FullIP; 
    char* PORT1; 
    char* PORT2; 
    char* FullPORT; 
}PASVInfo;

void printf_RGB(int r, int g, int b, const char* format, ...);

PASVInfo getInfo(const char* Info);
char* intToString(int NbINT) ;
char* strbcpy(char* Input, int x, int y) ;
int findNext(char* s, char c, int start) ;

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
            int sockControleFTP = connecterFTP("ftp.fr.debian.org");
            int sockDataFTP;

            char* bufferClient = (char*)calloc(MAXLENGHT,sizeof(char));
            char* bufferFTP = (char*)calloc(MAXLENGHT,sizeof(char));

            int nFTP;
            int nClient;
            int nData;

            int BoolFTPData = 0;

            // Lire le message de bienvenue du serveur FTP et le renvoyer au client DATA
            nFTP = read(sockControleFTP, bufferFTP, MAXLENGHT);
            if(nFTP > 0){
                // L'envoyée au client
                write(cliDATA, bufferFTP, nFTP);
            }

            while(1){
                memset(bufferClient, 0, MAXLENGHT);
                memset(bufferFTP, 0, MAXLENGHT);

                nClient = read(cliCTRL, bufferClient, MAXLENGHT);
                if(nClient <= 0){
                    printf_RGB(255,0,0,"[KO] Erreur lecture CLIENT\n");
                    free(bufferClient);
                    free(bufferFTP);
                    break;
                }
                // Ajouter CRLF à la commande (OBLIGATOIRE)
                if(bufferClient[nClient-1] != '\n') {
                    bufferClient[nClient] = '\r';
                    bufferClient[nClient+1] = '\n';
                    nClient += 2;
                }

                if(strncmp(bufferClient,"PASV",4) == 0){
                    BoolFTPData = 1;
                    // Ecrire "PASV" vers le serveur FTP
                    write(sockControleFTP,bufferClient,nClient);
                    // Lire la réponse du serveur FTP
                    nFTP = read(sockControleFTP,bufferFTP,MAXLENGHT);
                    // Je recup les info du serveur
                    PASVInfo dataInfo = getInfo(bufferFTP);
                    printf_RGB(0,0,255,"[INFO] Client %s ->> Serveur FTP IP: %s - PORT : %s\n",ipD,dataInfo.FullIP,dataInfo.FullPORT);
                    int dataPort = atoi(dataInfo.FullPORT);
                    // Crée le socket
                    sockDataFTP = socket(AF_INET,SOCK_STREAM,0);

                    struct sockaddr_in dataAddr;
                    dataAddr.sin_family = AF_INET;
                    dataAddr.sin_port = htons(dataPort);
                    inet_pton(AF_INET, dataInfo.FullIP, &dataAddr.sin_addr);

                    if(connect(sockDataFTP, (struct sockaddr*)&dataAddr, sizeof(dataAddr)) < 0){
                        perror("[KO] Connect DATA");
                    } else {
                        printf_RGB(0,255,0,"[OK] Connecté au canal DATA\n");
                        BoolFTPData = 1;
                    }
                    write(cliCTRL, bufferFTP, nFTP);
                }

                if(BoolFTPData){
                    if(strncmp(bufferClient, "LIST", 4) == 0 && BoolFTPData) {
                        // Envoyer LIST au serveur FTP
                        write(sockControleFTP, bufferClient, nClient);

                        // Lire la réponse initiale du canal contrôle (150 Opening data connection)
                        nFTP = read(sockControleFTP, bufferFTP, MAXLENGHT);
                        if(nFTP > 0){
                            write(cliCTRL, bufferFTP, nFTP);
                        }

                        // Lire le canal DATA
                        char dataBuf[MAXLENGHT];
                        while((nData = read(sockDataFTP, dataBuf, MAXLENGHT)) > 0){
                            write(cliDATA, dataBuf, nData);
                        }

                        // Fermer le socket DATA après le transfert
                        close(sockDataFTP);
                        sockDataFTP = -1;

                        // Lire le message final du serveur sur le canal contrôle (226 Transfer complete)
                        nFTP = read(sockControleFTP, bufferFTP, MAXLENGHT);
                        if(nFTP > 0){
                            write(cliCTRL, bufferFTP, nFTP);
                        }

                        // Remettre BoolFTPData à 0 pour ne pas réutiliser le socket DATA
                        BoolFTPData = 0;
                    }
                }else{
                }

                write(sockControleFTP,bufferClient,nClient);

                int nFTP = read(sockControleFTP,bufferFTP,MAXLENGHT);
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

PASVInfo getInfo(const char* Info) {
    PASVInfo out;
    char* cpy = strdup(Info);

    char*** Valeurs = (char***)calloc(6, sizeof(char**));
    Valeurs[0] = &out.IP1;
    Valeurs[1] = &out.IP2;
    Valeurs[2] = &out.IP3;
    Valeurs[3] = &out.IP4;
    Valeurs[4] = &out.PORT1;
    Valeurs[5] = &out.PORT2;

    int pos = findNext(cpy, '(', 0) + 1;

    for (int i = 0; i < 6; i++) {
        int sep;
        if (i < 5) {
            sep = findNext(cpy, ',', pos);
        } else {
            sep = findNext(cpy, ')', pos);
        }
        *Valeurs[i] = strbcpy(cpy, pos, sep);
        pos = sep + 1;
    }

    out.FullPORT = intToString(atoi(*Valeurs[4]) * 256 + atoi(*Valeurs[5]));

    out.FullIP = (char*)calloc(16, sizeof(char));
    for (int i = 0; i < 4; i++) {
        strcat(out.FullIP, *Valeurs[i]);
        if (i < 3) {
            strcat(out.FullIP, ".");
        }
    }

    free(cpy);
    free(Valeurs);

    return out;
}

int findNext(char* s, char c, int start) {
    for (int i = start; s[i]; i++) {
        if (s[i] == c) return i;
    }
    return -1;
}

char* strbcpy(char* Input, int x, int y) {
    char* res = (char*)calloc(y - x + 1, sizeof(char));
    for (int i = 0, j = x; j < y; i++, j++) {
        res[i] = Input[j];
    }
    return res;
}

char* intToString(int NbINT) {
    int NbCar = 1;
    int BufferINT = NbINT;

    while (BufferINT / 10 != 0) {
        BufferINT = BufferINT / 10;
        NbCar++;
    }

    BufferINT = NbINT;
    char* NbSTR = (char*)calloc(NbCar + 1, sizeof(char));
    NbSTR[NbCar] = '\0';

    for (int i = NbCar - 1; i >= 0; i--) {
        NbSTR[i] = (BufferINT % 10) + '0';
        BufferINT /= 10;
    }

    return NbSTR;
}



void printf_RGB(int r, int g, int b, const char* format, ...) {
    va_list args;
    va_start(args, format);

    printf("\033[38;2;%d;%d;%dm", r, g, b);
    vprintf(format, args);
    printf("\033[0m");

    va_end(args);
}
