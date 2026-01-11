#include <stdio.h>      // Entrées/sorties standard
#include <stdlib.h>     // Gestion mémoire (Calloc, malloc ...)
#include <string.h>     // Manipulation des chaînes et blocs mémoire
#include <sys/socket.h> // Base de l’API socket (POSIX)
#include <netdb.h>      // Fonctions pour la résolution de noms et informations réseau
#include <unistd.h>     // Fonctions système UNIX
#include <arpa/inet.h>  // Conversions IP & fonctions réseau IPv4
#include <stdarg.h>     // Arguments variables (...) -> Pour printf_RGB (Grégoire) 

#define MAXLENGHT 4096  // Définition d'une valeur MAX

/* Structure : PASVInfo
 * Strcuture de donnée conternant mes 4 octet d'une adresse id en format string (IP1, IP2, IP3, IP4)
 * ainsi que les deux instance d'un port (PORT1 et PORT2)
 * En plus de l'IP et du PORT en entier et contruit.
 * Sachant que PASV retourne "227 Entering Passive Mode (192,168,52,83,205,179)" et que les information
 * sont divisée en partie, cela me permet de remplire au fur et a mesure la strcuture avec les valeur 
 * PUIS effectée les calcule concernant le PORT
*/
typedef struct { 
    char* IP1; 
    char* IP2; 
    char* IP3; 
    char* IP4; 
    char* FullIP; 
    char* PORT1; 
    char* PORT2; 
    char* FullPORT; 
} PASVInfo;

void printf_RGB(int r, int g, int b, const char* format, ...);
PASVInfo getInfo(const char* Info);
char* intToString(int NbINT);
char* strbcpy(char* Input, int x, int y);
int findNext(char* s, char c, int start);
int readLine(int sock, char* buffer, int maxlen);
int creerSocket(const char* ip, const char* port);
int connecterFTP(const char* hostname);

int main(void){
    // Crée un socket CONTROLE local (0.0.0.0) avec un port = 40010
    int srvCTRL = creerSocket("0.0.0.0", "40010");
    // Crée un socket CONTROLE local (0.0.0.0) avec un port = 40011
    int srvDATA = creerSocket("0.0.0.0", "40011");
    printf_RGB(0,0,255,"[INFO] Initialisation serveurs...\n");  // Message d'initialisation du serveur 

    while(1){
        /////////////////////////////////////// CONTROLE ///////////////////////////////////////
        struct sockaddr_in addrCONTROLE;              // Structure qui va stocker les informations du client qui se connecte (IP, port).
        socklen_t lenCONTROLE = sizeof(addrCONTROLE); // Indique la taille de la structure pour accept()
        /* accept -> Bloque le serveur jusqu'a qu'un client tente de se connecter sur srvCTRL 
         * srvCTRL étant le socket crée pour acceuir la connection, addrCONTROLE la structure qui vas acceuir les information
         * client et lenCONTROLE la longeur de la structure a affectée 
        */
        int cliCTRL = accept(srvCTRL, (struct sockaddr*)&addrCONTROLE, &lenCONTROLE);

        char ipC[INET_ADDRSTRLEN]; // Buffer pour stocker l’adresse IPv4 en format texte ("192.168.1.5").
        // Convertit l’adresse IP depuis le format binaire réseau (stocké dans sin_addr) vers une chaîne lisible.
        inet_ntop(AF_INET, &addrCONTROLE.sin_addr, ipC, sizeof(ipC));
        // Affichage de la connection du client avec son IP afficher 
        printf_RGB(0,255,0,"[OK] Client CONTROLE connecté (%s)\n", ipC);
        ///////////////////////////////////////// DATA /////////////////////////////////////////
        // Exactement comme CONTROLE
        struct sockaddr_in addrDATA;
        socklen_t lenDATA = sizeof(addrDATA);
        int cliDATA = accept(srvDATA, (struct sockaddr*)&addrDATA, &lenDATA);

        char ipD[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addrDATA.sin_addr, ipD, sizeof(ipD));
        printf_RGB(0,255,0,"[OK] Client DATA connecté (%s)\n", ipD);

        //////////////////////////////////////////////////////////////////////////////////////
        // On crée un enfant au programme -> Multi session -> Le serveur parent attend une nouvelle connection
        pid_t pid = fork();

        // Si l'enfant a bien étais crée alors ...
        if(pid == 0){
            // On utilise la fonction connecterFTP avec en entrée l'URL du serveur a se connecter
            int sockControleFTP = connecterFTP("ftp.fr.debian.org");
            int sockDataFTP; // Initialisation du descripteur du socket data cotée FTP

            // Définition des buffeur de donnée 
            char bufferClient[MAXLENGHT];
            char bufferFTP[MAXLENGHT];

            // Création de deux valeur int pour les retour du nombre de byte traiter
            int nFTP, nClient;

            // Message de bienvenue du serveur FTP (220 Welcome to french Debian FTP server)
            nFTP = read(sockControleFTP, bufferFTP, MAXLENGHT-1);
            if(nFTP <= 0){
                printf_RGB(255,0,0,"[KO] Erreur: lecture FTP\n");
                exit(1);
            }
            // Envoyée le message de bievenue au client 
            write(cliCTRL, bufferFTP, nFTP);

            while(1){
                memset(bufferClient, 0, MAXLENGHT); // Réinitialisation de bufferClient (Eviter les artefact)
                memset(bufferFTP, 0, MAXLENGHT);    // Réinitialisation de bufferFTP (Eviter les artefact)

                // Lire commande client
                nClient = readLine(cliCTRL, bufferClient, MAXLENGHT-1);
                if(nClient <= 0){
                    printf_RGB(255,0,0,"[KO] Erreur: Lecture commande client\n");
                    break;
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
                    
                    // 3. Parser la réponse PASV
                    if(strncmp(bufferFTP, "227", 3) == 0){
                        PASVInfo info = getInfo(bufferFTP);
                        printf_RGB(0,255,0,"[OK] Mode PASV: %s:%s\n", info.FullIP, info.FullPORT);
                        
                        // 4. Connecter au serveur DATA du FTP
                        int portFTP = atoi(info.FullPORT); // atoi = String to int 
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
                        write(sockControleFTP, bufferClient, nClient);
                        
                        // 6. Lire la réponse "150 Opening..."
                        nFTP = readLine(sockControleFTP, bufferFTP, MAXLENGHT-1);
                        bufferFTP[nFTP] = '\0';
                        write(cliCTRL, bufferFTP, nFTP);
                        printf_RGB(0,255,255,"[FTP] %s", bufferFTP);
                        
                        // 7. Transférer les données du serveur FTP vers le client
                        char dataBuffer[MAXLENGHT];
                        int nDataFTP;
                        int totalBytes = 0;
                        
                        while((nDataFTP = read(sockDataFTP, dataBuffer, MAXLENGHT)) > 0){
                            write(cliDATA, dataBuffer, nDataFTP);
                            totalBytes += nDataFTP;
                        }
                        
                        printf_RGB(255,255,0,"[DATA] %d bytes transférés\n", totalBytes);
                        
                        close(sockDataFTP);
                        
                        // Fermer la connexion DATA côté client
                        close(cliDATA);
                        printf_RGB(0,255,0,"[OK] Transfert DATA terminé\n");
                        
                        // IMPORTANT : ACCEPTER LA NOUVELLE CONNEXION MAINTENANT
                        // (avant de lire le message 226)
                        printf_RGB(255,255,0,"[INFO] En attente de nouvelle connexion DATA...\n");
                        cliDATA = accept(srvDATA, (struct sockaddr*)&addrDATA, &lenDATA);
                        inet_ntop(AF_INET, &addrDATA.sin_addr, ipD, sizeof(ipD));
                        printf_RGB(0,255,0,"[OK] Client DATA reconnecté (%s)\n", ipD);
                        
                        // 8. Maintenant lire la réponse finale "226 Transfer complete"
                        nFTP = readLine(sockControleFTP, bufferFTP, MAXLENGHT-1);
                        bufferFTP[nFTP] = '\0';
                        write(cliCTRL, bufferFTP, nFTP);
                        printf_RGB(0,255,255,"[FTP] %s", bufferFTP);
                        
                        // Libérer la mémoire
                        free(info.IP1);
                        free(info.IP2);
                        free(info.IP3);
                        free(info.IP4);
                        free(info.PORT1);
                        free(info.PORT2);
                        free(info.FullIP);
                        free(info.FullPORT);
                        
                    } else {
                        printf_RGB(255,0,0,"[KO] Réponse PASV invalide\n");
                    }
                    
                } else {
                    // Commande normale (USER, PASS, PWD, etc.)
                    write(sockControleFTP, bufferClient, nClient);
                    
                    nFTP = readLine(sockControleFTP, bufferFTP, MAXLENGHT-1);
                    if(nFTP <= 0){
                        printf_RGB(255,0,0,"[KO] Erreur: Lecture réponse FTP\n");
                        break;
                    }
                    
                    bufferFTP[nFTP] = '\0';
                    write(cliCTRL, bufferFTP, nFTP);
                }
            }

            printf_RGB(0,0,255,"[INFO] Fin de connexion\n");
            close(sockControleFTP);
            close(cliCTRL);
            close(cliDATA);
            exit(0);
            
        } else {
            close(cliCTRL);
            close(cliDATA);
        }
    }

    return 0;
}

int creerSocket(const char* ip, const char* port){
    int ecode;    // Code de retour des fonctions réseau
    int sockfd;   // Descripteur de la socket serveur
    struct addrinfo hints, *res; // Structures pour la résolution d'adresse

    // Initialisation de la structure hints à zéro
    // (évite les valeurs indéfinies)
    memset(&hints, 0, sizeof(hints));

    // Famille d'adresses : IPv4
    hints.ai_family = AF_INET;

    // Type de socket : TCP (orienté connexion)
    hints.ai_socktype = SOCK_STREAM;

    // Indique que la socket sera utilisée pour un serveur (bind)
    // et que l'adresse IP locale peut être automatiquement choisie
    hints.ai_flags = AI_PASSIVE;

    // Résolution de l'adresse IP et du port en structure utilisable par bind()
    ecode = getaddrinfo(ip, port, &hints, &res);
    if(ecode != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
        exit(1);
    }

    // Création de la socket à partir des informations retournées par getaddrinfo()
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    // Option permettant de réutiliser rapidement le port après un redémarrage
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Association de la socket à l'adresse IP et au port
    if(bind(sockfd, res->ai_addr, res->ai_addrlen) < 0){
        perror("bind");
        exit(1);
    }

    // Mise en écoute de la socket
    // 5 = taille maximale de la file d'attente des connexions
    listen(sockfd, 5);

    // Libération de la mémoire allouée par getaddrinfo()
    freeaddrinfo(res);

    // Retourne le descripteur de la socket serveur prête à accepter des clients
    return sockfd;
}

int connecterFTP(const char* hostname) {
    struct addrinfo hints, *res; // Structures pour la résolution DNS
                                // et les informations de connexion
                                
    // Initialisation de la structure hints à zéro
    memset(&hints, 0, sizeof(hints));

    // Famille d'adresses : IPv4
    hints.ai_family = AF_INET;

    // Type de socket : TCP (FTP fonctionne en TCP)
    hints.ai_socktype = SOCK_STREAM;

    // Résolution du nom d'hôte vers une adresse IP sur le port FTP (21)
    int ecode = getaddrinfo(hostname, "21", &hints, &res);
    if (ecode != 0) {
        printf_RGB(255,0,0,"[KO] getaddrinfo: %s\n", gai_strerror(ecode));
        return -1;
    }

    // Création de la socket FTP à partir des informations retournées
    int sockFTP = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    // Tentative de connexion au serveur FTP
    if (connect(sockFTP, res->ai_addr, res->ai_addrlen) < 0){
        perror("[KO] connect");
        freeaddrinfo(res); // Libération mémoire avant de quitter
        return -1;
    }

    // Libération de la mémoire allouée par getaddrinfo()
    freeaddrinfo(res);

    // Retourne le descripteur de la socket FTP connectée
    return sockFTP;
}


PASVInfo getInfo(const char* Info) {
    PASVInfo out; // Structure de sortie contenant IP et port

    // Duplication de la chaîne d'entrée car elle sera modifiée
    char* cpy = strdup(Info);

    /* Tableau de pointeurs vers les champs de la structure out
     * Permet d'assigner dynamiquement IP1..IP4 et PORT1..PORT2
     */
    char*** Valeurs = (char***)calloc(6, sizeof(char**));
    Valeurs[0] = &out.IP1;
    Valeurs[1] = &out.IP2;
    Valeurs[2] = &out.IP3;
    Valeurs[3] = &out.IP4;
    Valeurs[4] = &out.PORT1;
    Valeurs[5] = &out.PORT2;

    // Position du premier caractère après '('
    int pos = findNext(cpy, '(', 0) + 1;

    // Extraction des 6 valeurs séparées par des virgules
    for (int i = 0; i < 6; i++) {
        int sep;

        // Pour les 5 premières valeurs, on cherche une virgule
        // Pour la dernière, on cherche la parenthèse fermante
        if (i < 5) {
            sep = findNext(cpy, ',', pos);
        } else {
            sep = findNext(cpy, ')', pos);
        }

        // Copie de la sous-chaîne entre pos et sep
        *Valeurs[i] = strbcpy(cpy, pos, sep);

        // Mise à jour de la position de lecture
        pos = sep + 1;
    }

    /* Calcul du port FTP DATA :
     * port = PORT1 * 256 + PORT2
     */
    out.FullPORT = intToString(
        atoi(*Valeurs[4]) * 256 + atoi(*Valeurs[5])
    );

    // Construction de l'adresse IP complète (xxx.xxx.xxx.xxx)
    out.FullIP = (char*)calloc(16, sizeof(char));
    for (int i = 0; i < 4; i++) {
        strcat(out.FullIP, *Valeurs[i]);
        if (i < 3) {
            strcat(out.FullIP, ".");
        }
    }

    // Libération des ressources temporaires
    free(cpy);
    free(Valeurs);

    // Retour de la structure PASVInfo remplie
    return out;
}


int readLine(int sock, char* buffer, int maxlen) {
    int i = 0;   // Index dans le buffer
    char c;      // Caractère lu

    // Lecture tant qu'on a de la place dans le buffer
    while (i < maxlen - 1) {
        int n = read(sock, &c, 1); // Lecture d'un seul octet depuis la socket
        if (n <= 0){ // n <= 0 : erreur ou connexion fermée
            return n;
        }

        buffer[i++] = c;
        if (c == '\n'){
            break;
        } 
    }
    // Ajout du caractère de fin de chaîne
    buffer[i] = '\0';
    // Retourne le nombre de caractères lus
    return i;
}


int findNext(char* s, char c, int start) {
    // Parcours de la chaîne à partir de start
    for (int i = start; s[i]; i++) {
        if (s[i] == c)
            return i; // Caractère trouvé
    }
    return -1; // Caractère non trouvé
}

char* strbcpy(char* Input, int x, int y) {
    // Allocation du buffer résultat (+1 pour '\0')
    char* res = (char*)calloc(y - x + 1, sizeof(char));
    // Copie caractère par caractère
    for (int i = 0, j = x; j < y; i++, j++) {
        res[i] = Input[j];
    }
    // '\0' déjà mis par calloc
    return res;
}

// Convertir un entier positif en chaîne de caractères sans sprintf.
char* intToString(int NbINT) {
    int NbCar = 1;        // Nombre de chiffres
    int BufferINT = NbINT;

    // Comptage du nombre de chiffres
    while (BufferINT / 10 != 0) {
        BufferINT = BufferINT / 10;
        NbCar++;
    }

    BufferINT = NbINT;

    // Allocation de la chaîne résultat
    char* NbSTR = (char*)calloc(NbCar + 1, sizeof(char));
    NbSTR[NbCar] = '\0';

    // Remplissage de la chaîne depuis la fin
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