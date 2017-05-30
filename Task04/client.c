#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

//	Кулаков Владислав КБ-401

// Функция печатает ошибку и выходит
void error(char *message) {
    fprintf(stderr, "%s.\n", message);
    exit(1);
}

int main(int argc, char *argv[]) {

    struct sockaddr_in server;
    struct hostent *host;

    int clisock = socket(AF_INET, SOCK_STREAM, 0);
    if (clisock < 0)
    {
        fprintf(stderr, "Error: Can't create socket\n");
        exit(9);
    }
    host = gethostbyname("localhost");
    if (!host)
    {
        fprintf(stderr, "Error: Can't get host\n");
        close(clisock);
        exit(9);
    }
    server.sin_family = AF_INET;
    bcopy((char *)host->h_addr, (char *) &server.sin_addr.s_addr, host->h_length);
    server.sin_port = htons(65535);

    if (connect(clisock, (struct sockaddr*) &server, sizeof(server)) < 0)
    {
        fprintf(stderr, "Error: Can't connect\n");
        close(clisock);
        exit(11);
    }

    char sym;
    while (read(clisock, &sym, 1) > 0) printf("%c", sym);
    close(clisock);
    return 0;
}
