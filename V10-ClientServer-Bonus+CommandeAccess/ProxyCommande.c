// Bibliothèques standard pour les entrées/sorties, gestion mémoire et chaînes
#include <stdio.h>      // printf, fprintf, perror
#include <stdlib.h>     // malloc, calloc, free, exit
#include <string.h>     // memset, strlen, strcat, strncmp, strdup
#include <sys/socket.h> // socket, bind, listen, accept, connect, setsockopt
#include <netdb.h>      // getaddrinfo, gai_strerror, freeaddrinfo
#include <unistd.h>     // read, write, close, fork
#include <arpa/inet.h>  // inet_ntop, inet_pton, htons, ntohs
#include <stdarg.h>     // va_list, va_start, va_end (pour printf_RGB)

// Taille maximale des buffers de communication
#define MAXLENGHT 4096

// Structure pour stocker les informations PASV parsées
// Le serveur FTP retourne : "227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)"
// où h1.h2.h3.h4 = IP et port = p1*256 + p2
typedef struct {
    char* IP1;       // Premier octet de l'IP (ex: "192")
    char* IP2;       // Deuxième octet (ex: "168")
    char* IP3;       // Troisième octet (ex: "1")
    char* IP4;       // Quatrième octet (ex: "120")
    char* FullIP;    // IP complète (ex: "192.168.1.120")
    char* PORT1;     // Première partie du port (p1)
    char* PORT2;     // Deuxième partie du port (p2)
    char* FullPORT;  // Port calculé en string (p1*256 + p2)
} PASVInfo;

// Déclarations des fonctions
void printf_RGB(int r, int g, int b, const char* format, ...);
PASVInfo getInfo(const char* Info);
char* intToString(int NbINT);
char* strbcpy(char* Input, int x, int y);
int findNext(char* s, char c, int start);
int readLine(int sock, char* buffer, int maxlen);
int creerSocket(const char* ip, const char* port);
int connecterFTP(const char* hostname);
int creerSocketEphemere();
int getPortFromSocket(int sock);
int connecterSocketData(const char* ip, int port);
void gererSessionFTP(int cliCTRL, const char* clientIP);
void relayerData(int cliDATA, int sockDataFTP);

int main(void){
    int srvCTRL = creerSocket("0.0.0.0", "40010");
    printf_RGB(0,0,255,"[INFO] Serveur FTP Proxy démarré sur le port 40010\n");

    while(1){
        struct sockaddr_in addrCONTROLE;
        socklen_t lenCONTROLE = sizeof(addrCONTROLE);
        int cliCTRL = accept(srvCTRL, (struct sockaddr*)&addrCONTROLE, &lenCONTROLE);

        char ipC[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addrCONTROLE.sin_addr, ipC, sizeof(ipC));
        printf_RGB(0,255,0,"[OK] Client connecté (%s)\n", ipC);

        pid_t pid = fork();
        
        if(pid == 0){
            close(srvCTRL);
            gererSessionFTP(cliCTRL, ipC);
            exit(0);
        } else {
            close(cliCTRL);
        }
    }

    return 0;
}

void gererSessionFTP(int cliCTRL, const char* clientIP){
    int sockControleFTP = connecterFTP("ftp.fr.debian.org");
    
    char bufferClient[MAXLENGHT];
    char bufferFTP[MAXLENGHT];
    int nFTP, nClient;
    
    int cliDATA = -1;
    int sockDataFTP = -1;

    // Message de bienvenue
    nFTP = read(sockControleFTP, bufferFTP, MAXLENGHT-1);
    if(nFTP > 0){
        write(cliCTRL, bufferFTP, nFTP);
    }

    while(1){
        memset(bufferClient, 0, MAXLENGHT);
        memset(bufferFTP, 0, MAXLENGHT);

        nClient = readLine(cliCTRL, bufferClient, MAXLENGHT-1);
        if(nClient <= 0) break;

        // ========== QUIT ==========
        if(strncmp(bufferClient, "QUIT", 4) == 0){
            printf_RGB(255,255,0,"[INFO] Commande QUIT reçue\n");
            write(sockControleFTP, bufferClient, nClient);
            nFTP = readLine(sockControleFTP, bufferFTP, MAXLENGHT-1);
            if(nFTP > 0){
                bufferFTP[nFTP] = '\0';
                write(cliCTRL, bufferFTP, nFTP);
            }
            break;
        }
        
        // ========== PASV ==========
        else if(strncmp(bufferClient, "PASV", 4) == 0){
            printf_RGB(255,255,0,"[INFO] Commande PASV détectée\n");
            
            write(sockControleFTP, bufferClient, nClient);
            
            nFTP = readLine(sockControleFTP, bufferFTP, MAXLENGHT-1);
            bufferFTP[nFTP] = '\0';
            printf_RGB(0,255,255,"[FTP] %s", bufferFTP);
            
            if(strncmp(bufferFTP, "227", 3) == 0){
                PASVInfo infoFTP = getInfo(bufferFTP);
                printf_RGB(0,255,0,"[OK] Serveur FTP PASV: %s:%s\n", infoFTP.FullIP, infoFTP.FullPORT);
                
                // Créer socket éphémère pour DATA
                int srvDATA = creerSocketEphemere();
                int portProxy = getPortFromSocket(srvDATA);
                printf_RGB(0,255,0,"[OK] Socket DATA proxy créé sur port %d\n", portProxy);
                
                // Construire réponse PASV modifiée
                char responsePASV[256];
                int p1 = portProxy / 256;
                int p2 = portProxy % 256;
                
                snprintf(responsePASV, sizeof(responsePASV),
                    "227 Entering Passive Mode (127,0,0,1,%d,%d)\r\n",
                    p1, p2);
                
                write(cliCTRL, responsePASV, strlen(responsePASV));
                printf_RGB(0,255,255,"[PROXY] %s", responsePASV);
                
                // Accepter connexion DATA du client
                struct sockaddr_in addrDATA;
                socklen_t lenDATA = sizeof(addrDATA);
                cliDATA = accept(srvDATA, (struct sockaddr*)&addrDATA, &lenDATA);
                close(srvDATA);
                printf_RGB(0,255,0,"[OK] Client DATA connecté\n");
                
                // Se connecter au serveur FTP DATA
                sockDataFTP = connecterSocketData(infoFTP.FullIP, atoi(infoFTP.FullPORT));
                printf_RGB(0,255,0,"[OK] Connecté au serveur FTP DATA\n");
                
                free(infoFTP.IP1);
                free(infoFTP.IP2);
                free(infoFTP.IP3);
                free(infoFTP.IP4);
                free(infoFTP.PORT1);
                free(infoFTP.PORT2);
                free(infoFTP.FullIP);
                free(infoFTP.FullPORT);
            }
        }
        
        // ========== LIST / RETR ==========
        else if(strncmp(bufferClient, "LIST", 4) == 0 || strncmp(bufferClient, "RETR", 4) == 0){
            printf_RGB(255,255,0,"[INFO] Commande %s détectée\n", 
                strncmp(bufferClient, "LIST", 4) == 0 ? "LIST" : "RETR");
            
            write(sockControleFTP, bufferClient, nClient);
            
            // Lire réponse 150
            nFTP = readLine(sockControleFTP, bufferFTP, MAXLENGHT-1);
            bufferFTP[nFTP] = '\0';
            write(cliCTRL, bufferFTP, nFTP);
            printf_RGB(0,255,255,"[FTP] %s", bufferFTP);
            
            if(strncmp(bufferFTP, "150", 3) == 0){
                // Relayer les données
                relayerData(cliDATA, sockDataFTP);
                
                close(cliDATA);
                close(sockDataFTP);
                cliDATA = -1;
                sockDataFTP = -1;
                
                // Lire message 226
                nFTP = readLine(sockControleFTP, bufferFTP, MAXLENGHT-1);
                bufferFTP[nFTP] = '\0';
                write(cliCTRL, bufferFTP, nFTP);
                printf_RGB(0,255,255,"[FTP] %s", bufferFTP);
            }
        }
        
        // ========== AUTRES COMMANDES ==========
        else {
            write(sockControleFTP, bufferClient, nClient);
            
            nFTP = readLine(sockControleFTP, bufferFTP, MAXLENGHT-1);
            if(nFTP > 0){
                bufferFTP[nFTP] = '\0';
                write(cliCTRL, bufferFTP, nFTP);
            }
        }
    }

    if(cliDATA >= 0) close(cliDATA);
    if(sockDataFTP >= 0) close(sockDataFTP);
    close(sockControleFTP);
    close(cliCTRL);
    printf_RGB(0,0,255,"[INFO] Session terminée\n");
}

void relayerData(int cliDATA, int sockDataFTP){
    char buffer[MAXLENGHT];
    int n, total = 0;
    
    while((n = read(sockDataFTP, buffer, MAXLENGHT)) > 0){
        write(cliDATA, buffer, n);
        total += n;
    }
    
    printf_RGB(255,255,0,"[DATA] %d bytes transférés\n", total);
}

int creerSocketEphemere(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0;
    
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(sock, 1);
    
    return sock;
}

int getPortFromSocket(int sock){
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    getsockname(sock, (struct sockaddr*)&addr, &len);
    return ntohs(addr.sin_port);
}

int connecterSocketData(const char* ip, int port){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    
    if(connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        perror("connect DATA");
        return -1;
    }
    
    return sock;
}

int creerSocket(const char* ip, const char* port){
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
    hints.ai_family = AF_INET;
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
        if (i < 3) strcat(out.FullIP, ".");
    }

    free(cpy);
    free(Valeurs);
    return out;
}

int readLine(int sock, char* buffer, int maxlen) {
    int i = 0;
    char c;

    while (i < maxlen - 1) {
        int n = read(sock, &c, 1);
        if (n <= 0) return n;
        buffer[i++] = c;
        if (c == '\n') break;
    }
    buffer[i] = '\0';
    return i;
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