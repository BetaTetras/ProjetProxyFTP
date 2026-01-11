#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdarg.h>

// Taille maximum
#define MAXLENGHT 4096

/* STRUCTURE PASVInfo
 * Contien tout les calcule nécessaire pour déterminée le port et l'adresse du serveur FTP.
 * Utile pour avoir en mémoire tout les info dans une seul structure de donnée
*/
typedef struct{ 
    char* IP1;      // Premier Octet
    char* IP2;      // Deuxiéme Octet
    char* IP3;      // Troisème Octet
    char* IP4;      // Quatrième Octet
    char* FullIP;   // L'adresse entière
    char* PORT1;    // Premier partie du port
    char* PORT2;    // Deuxième partie du port
    char* FullPORT; // Port apres les calcules
}PASVInfo;

PASVInfo getInfo(const char* Info);
char* intToString(int NbINT) ;
char* strbcpy(char* Input, int x, int y) ;
int findNext(char* s, char c, int start) ;

int readLine(int sock, char* buffer, int maxlen);
int créeSocket(const char* ip,const char* port);
int connecterFTP(const char* ip);

void printf_RGB(int r, int g, int b, const char* format, ...);

int main(void){
    // Crée un descripteur de socket locale CONTROLE
    int srvCTRL = créeSocket("0.0.0.0", "40010");
    // Crée un descripteur de socket locale DATA
    int srvDATA = créeSocket("0.0.0.0", "40011");
    printf_RGB(0,0,255,"[INFO] Initialisation serveurs...\n");
    ////////////////////////////////////////////////////////////////////////////////////////

    while(1){
        // Création de la structure qui contien l'addresse cotée client CONTROLE
        struct sockaddr_in addrCONTROLE;
        // Définit la longeur de la structure sockaddr_in
        socklen_t lenCONTROLE = sizeof(addrCONTROLE);
        // Crée le socket cotée client CONTROLE
        int cliCTRL = accept(srvCTRL, (struct sockaddr*)&addrCONTROLE, &lenCONTROLE);

        // On crée un tableau de char avec une longeure de INET_ADDRSTRLEN
        char ipC[INET_ADDRSTRLEN];
        // Converti une addresse binaire en addresse lisible et on mes le resultat dans ipC
        inet_ntop(AF_INET, &addrCONTROLE.sin_addr, ipC, sizeof(ipC));
        printf_RGB(0,255,0,"[OK] Client CONTROLE connecté (%s)\n", ipC);
        ////////////////////////////////////////////////////////////////////////////////////////
        // Création de la structure qui contien l'addresse cotée client DATA
        struct sockaddr_in addrDATA;
        // Définit la longeur de la structure sockaddr_in
        socklen_t lenDATA = sizeof(addrDATA);
        // Crée le socket cotée client DATA
        int cliDATA = accept(srvDATA, (struct sockaddr*)&addrDATA, &lenDATA);

        // On crée un tableau de char avec une longeure de INET_ADDRSTRLEN
        char ipD[INET_ADDRSTRLEN];
        // Converti une addresse binaire en addresse lisible et on mes le resultat dans ipD
        inet_ntop(AF_INET, &addrDATA.sin_addr, ipD, sizeof(ipD));
        printf_RGB(0,255,0,"[OK] Client DATA connecté (%s)\n", ipD);

        // On crée un enfant 
        pid_t pid = fork();

        // A l'interrieur de l'enfant ...
        if(pid == 0){
            int sockControleFTP = connecterFTP("ftp.fr.debian.org");
            int sockDataFTP;

            char* bufferClient = (char*)calloc(MAXLENGHT,sizeof(char));
            char* bufferFTP = (char*)calloc(MAXLENGHT,sizeof(char));

            int nFTP;
            int nClient;
            int nData;

            int BoolFTPData = 0;
            nFTP = read(sockControleFTP, bufferFTP, MAXLENGHT);
            if(nFTP <= 0){
                printf_RGB(255,0,0,"# Erreur: lecture FTP");
                exit(1);
            }
            nClient = write(cliCTRL, bufferFTP, nFTP);

            while(1){
                memset(bufferClient, 0, MAXLENGHT);
                memset(bufferFTP, 0, MAXLENGHT);

                // Lire commande client
                nClient = readLine(cliCTRL, bufferClient, MAXLENGHT-1);
                if(nClient <= 0){
                    printf_RGB(255,0,0,"# Erreur: Lecture commande client\n");
                    exit(1);
                }

                // Vérifier si c'est une commande LIST
                if(strncmp(bufferClient, "LIST", 4) == 0){
                    printf_RGB(255,255,0,"[INFO] Commande LIST détectée, passage en mode PASV\n");
                    
                    // 1. Envoyer PASV au serveur FTP
                    write(sockControleFTP, "PASV\r\n", 6);
                    
                    // 2. Lire la réponse PASV
                    nFTP = readLine(sockControleFTP, bufferFTP, MAXLENGHT-1);
                    bufferFTP[nFTP] = '\0';
                    printf_RGB(0,255,255,"[FTP] %s", bufferFTP);
                    
                    // 3. Parser la réponse avec ta fonction getInfo()
                    if(strncmp(bufferFTP, "227", 3) == 0){
                        PASVInfo info = getInfo(bufferFTP);
                        printf_RGB(0,255,0,"[OK] Mode PASV: %s:%s\n", info.FullIP, info.FullPORT);
                        
                        // 4. Connecter au serveur DATA du FTP
                        int portFTP = atoi(info.FullPORT);
                        struct sockaddr_in addrFTPData;
                        sockDataFTP = socket(AF_INET, SOCK_STREAM, 0);
                        
                        memset(&addrFTPData, 0, sizeof(addrFTPData));
                        addrFTPData.sin_family = AF_INET;
                        addrFTPData.sin_port = htons(portFTP);
                        inet_pton(AF_INET, info.FullIP, &addrFTPData.sin_addr);
                        
                        if(connect(sockDataFTP, (struct sockaddr*)&addrFTPData, sizeof(addrFTPData)) < 0){
                            printf_RGB(255,0,0,"[KO] Connexion DATA FTP échouée\n");
                            perror("connect");
                            exit(1);
                        }
                        printf_RGB(0,255,0,"[OK] Connecté au serveur DATA FTP\n");
                        
                        // 5. Envoyer la commande LIST au serveur FTP
                        if(bufferClient[nClient-1] != '\n') {
                            bufferClient[nClient] = '\r';
                            bufferClient[nClient+1] = '\n';
                            nClient += 2;
                        }
                        write(sockControleFTP, bufferClient, nClient);
                        
                        // 6. Lire la réponse "150 Opening..."
                        nFTP = readLine(sockControleFTP, bufferFTP, MAXLENGHT-1);
                        bufferFTP[nFTP] = '\0';
                        write(cliCTRL, bufferFTP, nFTP);
                        printf_RGB(0,255,255,"[FTP] %s", bufferFTP);
                        
                        // 7. Transférer les données du serveur FTP vers le client
                        char dataBuffer[MAXLENGHT];
                        int nDataFTP;
                        while((nDataFTP = read(sockDataFTP, dataBuffer, MAXLENGHT-1)) > 0){
                            write(cliDATA, dataBuffer, nDataFTP);
                            printf_RGB(255,255,0,"[DATA] %d bytes transférés\n", nDataFTP);
                        }
                        
                        close(sockDataFTP);
                        shutdown(cliDATA, SHUT_WR);  // ← AJOUTE CETTE LIGNE !
                        printf_RGB(0,255,0,"[OK] Transfert DATA terminé\n");
                        
                        // 8. Lire la réponse finale "226 Transfer complete"
                        nFTP = readLine(sockControleFTP, bufferFTP, MAXLENGHT-1);
                        bufferFTP[nFTP] = '\0';
                        write(cliCTRL, bufferFTP, nFTP);
                        printf_RGB(0,255,255,"[FTP] %s", bufferFTP);
                        
                        // Libérer la mémoire allouée par getInfo()
                        free(info.IP1);
                        free(info.IP2);
                        free(info.IP3);
                        free(info.IP4);
                        free(info.PORT1);
                        free(info.PORT2);
                        free(info.FullIP);
                        free(info.FullPORT);
                        
                    }else{
                        printf_RGB(255,0,0,"[KO] Réponse PASV invalide\n");
                    }
                    
                }else{
                    // Commande normale (USER, PASS, PWD, etc.)
                    if(bufferClient[nClient-1] != '\n') {
                        bufferClient[nClient] = '\r';
                        bufferClient[nClient+1] = '\n';
                        nClient += 2;
                    }
                    
                    write(sockControleFTP, bufferClient, nClient);
                    
                    nFTP = readLine(sockControleFTP, bufferFTP, MAXLENGHT-1);
                    if(nFTP <= 0){
                        printf_RGB(255,0,0,"# Erreur: Lecture réponse FTP\n");
                        exit(1);
                    }
                    
                    bufferFTP[nFTP] = '\0';
                    write(cliCTRL, bufferFTP, nFTP);
                }
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
    

/* CREESOCKET
 * Fonction qui passe en entrée l'ip et le port de connection voulu et renvois l'identificateur du socket
 * pour une communication dans le code main.
 * Je l'ai crée pour eviter de réecrire 10 000 fois la partie de code pour crée un socket.
*/
int créeSocket(const char* ip,const char* port){
    // Buffer de code de retour
    int ecode;
    // buffer d'identificateur du socket
    int sockfd;
    /* HINTS et RES
     * HINTS : hints est une structure qui est a passée en lecture a la fonction 
     * getaddrinfo et qui permet de mettre en place unse liste de paramétre voulue.
     * RES : res est une structure contenant tout les information retorunée par la
     * structure getaddrinfo 
     *
     * Nous avons *res (Qui est donc une liste d'élement res) car getaddrinfo ne
     * retourne pas que une seul instance de addrinfo mais plusieur.
    */
    struct addrinfo hints, *res;

    // On purge la mémoire de hints pour évitée les artefactes
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // Addresse bindable

    // On récupère la liste des addresse a laquelle on peut se connecter
    ecode = getaddrinfo(ip, port, &hints, &res);
    if(ecode != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
        exit(1);
    }

    /* On crée le socket en recupérant les donnée du premier élément de res
     * - resPtr->ai_family = AF_INET (IPv4) ou AF_INET6 (IPv6).
     * - resPtr->ai_socktype = SOCK_STREAM (TCP), SOCK_DGRAM (UDP)
     * - resPtr->ai_protocol = souvent 0 , ou IPPROTO_TCP / IPPROTO_UDP.
     */
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    /* Configurer une option du socket avant de l'utiliser
     * sockfd : Descripteur du socket a configurer
     * SOL_SOCKET : Niveau d'application du socket
     * SO_REUSEADDR : autorise la réutilisation de l’adresse/port (Evite le "Address already in use")
     * &opt : Valeur de l'option (Binaire) /!\ ON DOIS PASSER UNE ADDR MEMOIRE PAS UNE VAL
     * sizeof(opt) : Taille de la valeur (Savoire combien d'octet a lire)
    */
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Attache le socket a l'adresse du serveur
    if(bind(sockfd, res->ai_addr, res->ai_addrlen) < 0){
        perror("bind");
        exit(1);
    }

    // Mes le socket en mode serveur avec une liste maximal de 5 utilisateur
    listen(sockfd, 5);
    // Liberation de la liste d'addresse de res
    freeaddrinfo(res);
    // On renvois de descripteur de socket
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

int readLine(int sock, char* buffer, int maxlen) {
    int i = 0;
    char c;
    while(i < maxlen - 1) {
        int n = read(sock, &c, 1);
        if(n <= 0) return n; // erreur ou fin de connexion
        buffer[i++] = c;
        if(c == '\n') break; // fin de ligne
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
