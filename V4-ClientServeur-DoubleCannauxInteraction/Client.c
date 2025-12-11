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
void printf_RGB(int r, int g, int b, const char* format, ...);

int main() {
    ////////////////////////////////////////////////////////////////////////////////////////
    printf_RGB(0,255,0,"[INFO] Initialisation client...\n");

    int sockCTRL = connecterSocket("192.168.1.120", 40002, "CONTROLE");
    int sockDATA = connecterSocket("192.168.1.120", 40003, "DATA");
    ////////////////////////////////////////////////////////////////////////////////////////
    char* buffer = (char*)calloc(MAXLENGHT,sizeof(char));
    char* Mode = (char*)calloc(MAXLENGHT,sizeof(char));
    printf_RGB(0,0,255,"Quelle methode [PORT/PASV] : ");
    fgets(Mode,MAXLENGHT,stdin);
    Mode[strcspn(Mode, "\n")] = '\0';
    write(sockCTRL,Mode,strlen(Mode));

    int n = read(sockDATA,buffer,MAXLENGHT-1);
    printf("Réponse : %s\n",buffer);

    close(sockCTRL);
    close(sockDATA);
    free(Mode);
    return 0;
}

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

void printf_RGB(int r, int g, int b, const char* format, ...) {
    va_list args;
    va_start(args, format);

    printf("\033[38;2;%d;%d;%dm", r, g, b);
    vprintf(format, args);
    printf("\033[0m");

    va_end(args);
}
