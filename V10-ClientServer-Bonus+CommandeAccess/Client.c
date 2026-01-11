#include <stdio.h>      // Entrées / sorties standard
#include <stdlib.h>     // Gestion mémoire (Calloc, malloc ...)
#include <string.h>     // Manipulation des chaînes et blocs mémoire
#include <sys/socket.h> // Base de l’API socket (POSIX)
#include <arpa/inet.h>  // Conversions IP & fonctions réseau IPv4
#include <unistd.h>     // Fonctions système UNIX
#include <stdarg.h>     // Arguments variables (...) -> Pour printf_RGB (Grégoire) 

#define MAXLENGHT 4096  // Définition d'une valeur MAX

int connecterSocket(const char* ip, int port, const char* libelle, int display);
int commandeProxy(int* sockCTRL, int* sockDATA, const char* ip);
void printf_RGB(int r, int g, int b, const char* format, ...);

int main() {
    // Information Terminale sur l'initialisation du client
    printf_RGB(0,255,0,"[INFO] Initialisation client...\n");

    /* Création d'un socket "Controle" vers l'ip 192.168.1.120 et le port 40010 (LOCAL)
     * Utilisation de la focntion connecterSocket (Voire explication ..)
    */
    int sockCTRL = connecterSocket("192.168.1.120", 40010, "CONTROLE",1);
    int sockDATA = connecterSocket("192.168.1.120", 40011, "DATA",1);

    // Création d'un buffre de 4096 char le longeur 
    char buffer[MAXLENGHT];

    /* On LIT ce qui a été envoyée dans le socket Controle en mettant tout les information
     * dans le buffer précédament initialisée avec une quantité max de 4096 - 1 char
     */
    int n = read(sockCTRL, buffer, MAXLENGHT-1);
    if(n <= 0){
        // Si la lecture a echouée (n étant le nombre de byte lue est qu'il est obligatoirement > 0)
        // alors on préscise que la lecture a échouée et on donne une valeur de retour au main (1 = erreur)
        printf_RGB(255,0,0,"[KO] Erreur lecture\n");
        return 1;
    }
    buffer[n] = '\0'; // Ajout d'un caractére de fin pour le printf
    printf_RGB(0,255,0,"%s\n", buffer); // Affichage de l'information

    /* DEBUT DU SEGMENT DE COMMUNICATION
     * C'est dans cette boucle while que la communication entre le Client et le Proxy
     * vas se faire. Elle se fini selon plusieur condition :
     * - L'utilisateur appuis sur ctrl + c (SIGKILL)
     * - L'utilisateur entre "bye" (comme communication standare ftp)
     */
    while(1){
        /* Utilisation de la fonction commandeProxy
         * Posséde une valeur de retour qui est égale a l'utilisation de "bye"
         * si la commande détectée dans la fonction est "bye" alors sont retour
         * serra 1 donc on sort de la boucle de commande
         */
        int sortie = commandeProxy(&sockCTRL, &sockDATA, "192.168.1.120");
        if(sortie == 1){
            break;
        }
    }

    // Fermeture de la connection 
    close(sockCTRL);
    close(sockDATA);
    // Retour de la fonction -> On finir le programme 
    return 0;
}

/* CONNECTERSOCKET
 * Paramétre d'entrée :
 * - const char* ip : Chaine de caractére qui contien l'ip du proxy
 * - const char* ip: Chaine de caractére qui contien le port du proxy
 * - const char* libelle : Nom pour affichage, utile pour le debug
 * 
 * Cette commande crée un socket vers une IP et un PORT passée en paramétre,
 * utile dans notre cas car nous l'utilisons plusieur fois => Moins de ligne effectives  
*/
int connecterSocket(const char* ip, int port, const char* libelle,int display){
    int sock; // Définition du descripteur de socket
    // Structure pour stocker les informations du serveur (adresse IP et port)
    struct sockaddr_in server;

    /* On crée le socket en mettant son descripteur dans l'int sock
     * Paramétre :
     * - AF_INET (IPv4)
     * - SOCK_STREAM (TCP)
     * - 0 : protocole par défaut (TCP pour SOCK_STREAM)
    */
    sock = socket(AF_INET, SOCK_STREAM, 0);

    // On affiche que le socket est crée uniquement si l'affichage et activée
    if(display){
        printf_RGB(0,255,0,"[OK] Socket créée {%s}\n", libelle);
    }
    

    // Initialisation de la structure sockaddr_in à zéro pour éviter les valeurs indéfinies
    memset(&server, 0, sizeof(server));
    
    /* Définition des paramétre du serveur 
     *
    */
    server.sin_family = AF_INET;    // IPv4
    server.sin_port = htons(port);  // Conversion du port en format réseau

    // Convertion de l'ip depuis le format text en format IP
    inet_pton(AF_INET, ip, &server.sin_addr);

     // Tentative de connexion au serveur
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("connect"); // Affiche l'erreur si la connexion échoue
        exit(1);           // Quitter le programme
    }

    // Préscicée qu'on a réussi a se connectée 
    if(display){
        printf_RGB(0,255,0,"[OK] Connecté à {%s}\n", libelle);
    }
     
    return sock; // Retourne le descripteur de socket connecté
}

/* COMMANDEPROXY
 * Paramétre :
 * - int* sockCTRL : descripteur de socket cotée CTRL
 * - int* sockDATA : descripteur de socket cotée DATA
 * - const char* ip : ip du proxy (Au cas de l'utilisation de LIST)
 *
 * Cette commande se trouve dans la boucle d'utilisation du proxy et permet de traitée les commande
 * entrée dans le stdin de l'utilisateur.
*/
int commandeProxy(int* sockCTRL, int* sockDATA, const char* ip){
    char Commande[MAXLENGHT];   // Buffer commande
    char Response[MAXLENGHT];   // Buffer réponse   
    int isListCommand = 0;
    int isRETRCommand = 0;

    // On affiche un invitée de commande
    printf("ftpProxy> ");
    // On récupére l'entrée utlisateur depuis stdin sur une longeur de 4096
    fgets(Commande, MAXLENGHT, stdin);
    
    // ========== GESTION QUIT/BYE ==========
    if(strncmp(Commande, "QUIT", 4) == 0 || strncmp(Commande, "quit", 4) == 0){
        // Retirer le \n et ajouter \r\n
        Commande[strcspn(Commande, "\n")] = '\0';
        write(*sockCTRL, Commande, strlen(Commande));
        write(*sockCTRL, "\r\n", 2);
        
        // Lire la réponse (221 Goodbye)
        int n = read(*sockCTRL, Response, MAXLENGHT-1);
        if(n > 0){
            Response[n] = '\0';
        }
        
        printf_RGB(255,0,0,"[INFO] See you next time!\n");
        return 1; // Indique qu'on doit sortir
    }
    
    // Détecter si c'est une commande LIST ou RETR
    isListCommand = (strncmp(Commande, "LIST", 4) == 0);
    isRETRCommand = (strncmp(Commande, "RETR", 4) == 0);
    
    // Envoyer la commande
    Commande[strcspn(Commande, "\n")] = '\0';       // Ajoute un char de terminaison
    write(*sockCTRL, Commande, strlen(Commande));   // ECRIT dans le socket controle la commande entrée sur tout sa longeur 
    write(*sockCTRL, "\r\n", 2);                    // Ajout CRLF -> Standardisation pour FTP
    
    // Lire la première réponse sur CTRL
    int n = read(*sockCTRL, Response, MAXLENGHT-1);
if(n > 0){
    Response[n] = '\0';
    // Afficher selon le code de réponse
    if(Response[0] == '4' || Response[0] == '5'){
        // Erreurs 4xx et 5xx - Rouge
        printf_RGB(255,0,0,"%s", Response);
    }else if(Response[0] == '1') {
        // Informations 1xx - Vert foncé
        printf_RGB(7,156,25,"%s", Response);
    }else if(Response[0] == '2') {
        // Succès 2xx - Vert clair
        printf_RGB(9,200,32,"%s", Response);
    }else if(Response[0] == '3') {
        // Besoin d'info 3xx - Orange/Jaune
        printf_RGB(255,195,0,"%s", Response);
    }else{
        // Autre - Blanc par défaut
        printf("%s",Response);
    }
}
    
    // ==================== TRAITEMENT LIST ====================
    if(isListCommand){
        char dataBuffer[MAXLENGHT]; // Buffer data
        printf_RGB(0,146,255,"========== Listing ==========\n");        // Délimitateur du LIST
        
        // Lire les données jusqu'à ce que le proxy ferme la connexion
        while((n = read(*sockDATA, dataBuffer, MAXLENGHT-1)) > 0){
            dataBuffer[n] = '\0';
            printf_RGB(0,92,163,"%s", dataBuffer); // Ecriture LIGNE PAR LIGNE
        }
        
        printf_RGB(0,146,255,"=============================\n");        // Délimitateur du LIST
        
        // IMPORTANT: Reconnecter AVANT de lire le message 226
        close(*sockDATA);
        *sockDATA = connecterSocket(ip, 40011, "DATA",0); // Reconnection au canal DATA entre le client et le proxy
        
        // Maintenant lire la réponse finale "226 Transfer complete"
        n = read(*sockCTRL, Response, MAXLENGHT-1);
        if(n > 0){
            Response[n] = '\0';
            printf_RGB(9,200,32,"%s", Response);
        }
    }
    
    // ==================== TRAITEMENT RETR ====================
    if(isRETRCommand){
        char dataBuffer[MAXLENGHT]; // Buffer data
        char nomFichier[256];       // Nom du fichier
        FILE* fichier = NULL;       // Pointeur vers le fichier (null pour l'instant)
        int totalBytes = 0;         // Compteur de byte
        
        // Extraire le nom du fichier de la commande "RETR nom_fichier.txt"
        sscanf(Commande, "RETR %s", nomFichier);
        
        // Ouvrir le fichier en mode écriture binaire
        fichier = fopen(nomFichier, "wb");
        if(fichier == NULL){
            printf_RGB(255,0,0,"[KO] Impossible de créer le fichier %s\n", nomFichier);
            perror("fopen");
            // Vider le canal DATA quand même pour ne pas bloquer
            while((n = read(*sockDATA, dataBuffer, MAXLENGHT-1)) > 0);
            close(*sockDATA);
            *sockDATA = connecterSocket(ip, 40011, "DATA",0);
            return 0;
        }
        
        printf_RGB(170,0,255,"===== Téléchargement: %s =====\n", nomFichier);
        
        // Lire les données du canal DATA et écrire dans le fichier
        while((n = read(*sockDATA, dataBuffer, MAXLENGHT-1)) > 0){
            fwrite(dataBuffer, 1, n, fichier);
            totalBytes += n;
        }
        
        // Fermer le fichier
        fclose(fichier);
        printf_RGB(115,0,179,"Fichier '%s' sauvegardé (%d bytes)\n", nomFichier, totalBytes);
        printf_RGB(170,0,255,"==================================\n");
        
        // IMPORTANT: Reconnecter AVANT de lire le message 226
        close(*sockDATA);
        *sockDATA = connecterSocket(ip, 40011, "DATA",0); // Reconnection au canal DATA
        
        // Maintenant lire la réponse finale "226 Transfer complete"
        n = read(*sockCTRL, Response, MAXLENGHT-1);
        if(n > 0){
            Response[n] = '\0';
            printf_RGB(9,200,32,"%s", Response);
        }
    }
    
    return 0;
}


/* PRINTF_RGB
 * Paramétre :
 * - int r : valeur en rouge
 * - int g : valeur en vert
 * - int b : valeur en bleu
 * - const char* format : format de string (comme printf ensuite)
 *
 * Permet un affichage terminal avec des couleur RGB -> Plus facile a comprendre
*/
void printf_RGB(int r, int g, int b, const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("\033[38;2;%d;%d;%dm", r, g, b);
    vprintf(format, args);
    printf("\033[0m");
    va_end(args);
}