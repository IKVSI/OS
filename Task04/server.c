#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

//	Кулаков Владислав КБ-401

const char *tempfilename = "/tmp/gameoflife.tmp";
int X;
int Y;
int XOFF[8] = {-1,  0,  1, -1, 1, -1, 0, 1};
int YOFF[8] = {-1, -1, -1,  0, 0,  1, 1, 1};
int ** FIELD;

void printfield(int x, int y, int ** field)
{
    for (int i=0; i<y; ++i)
    {
        for (int j=0; j<x; ++j)
        {
            printf("%i", field[i][j]);
        }
        printf("\n");
    }
}

void serv()
{
    int servsock = socket(AF_INET, SOCK_STREAM, 0);
    if (servsock < 0)
    {
        fprintf(stderr, "Error: Can't create socket\n");
        exit(9);
    }
    struct sockaddr_in server, client;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(65535);
    if (bind(servsock, (struct sockaddr *) &server, sizeof(server)) < 0)
    {
        fprintf(stderr, "Error: Can't bind 65535\n");
        exit(10);
    }
    listen(servsock, 10);
    unsigned int clientsize=sizeof(client);
    while (1)
    {
        int clisock = accept(servsock, (struct sockaddr *) &client, &clientsize);
        if (clisock < 0)
        {
            fprintf(stderr, "Error: Can't connect\n");
            close(servsock);
            exit(11);
        }
        FILE *handle = fopen(tempfilename, "r");
        if (!handle)
        {
            fprintf(stderr, "Error: Can't open file %s\n", tempfilename);
            close(clisock);
            close(servsock);
            exit(2);
        }
        while(1)
        {
            char sym=fgetc(handle);
            if(sym == EOF) break;
            int n = write(clisock, (void*)&sym, 1);
            if (n < 0)
            {
                fprintf(stderr, "Error: Can't write to socket\n");
                fclose(handle);
                close(clisock);
                close(servsock);
                exit(12);
            }
        }
        fclose(handle);
        close(clisock);
    }
    close(servsock);
}

// Проверка на живую
int isalive(int x, int y)
{
    if ((x<0) || (x>=X) || (y<0) || (y>=Y)) return 0;
    else return FIELD[y][x];
}

// Шаг
void move()
{
    int ** field = (int **) malloc(Y * sizeof(int *));
    for(int i=0; i<Y; ++i) field[i]=(int *) malloc(X * sizeof(int));
    time_t start = time(NULL);
    for(int i=0; i<Y; ++i)
    {
        for(int j=0; j<X; ++j)
        {
            int alive=0;
            for(int k=0; k<8; ++k) alive+=isalive(j+XOFF[k], i+YOFF[k]);
            if ((FIELD[i][j] == 0) && (alive == 3)) field[i][j]=1;
            else if ((FIELD[i][j] == 1) && ((alive < 2) || (alive > 3))) field[i][j]=0;
            else field[i][j]=FIELD[i][j];
        }
    }
    double seconds = difftime(time(NULL), start);
    if(seconds > 1.0)
    {
        fprintf(stderr, "Fail: Time=%lf > 1.0s\n", seconds);
    }
    else
    {
        int **temp=FIELD;
        FIELD=field;
        field=temp;
    }
    for(int i=0; i<Y; ++i) free(field[i]);
    free(field);
}

void work(int sgn)
{
    // Каждую секунду повторять
    alarm(1);
    // Запись в временный файл
    FILE *handle = fopen(tempfilename, "w");
    if (!handle)
    {
        fprintf(stderr, "Error: Can't open temp file %s\n", tempfilename);
        exit(2);
    }
    for (int i=0; i<Y; ++i)
    {
        for (int j=0; j<X; ++j)
        {
            if ( fprintf(handle, "%i", FIELD[i][j]) < 0)
            {
                fprintf(stderr, "Error: Can't write to temp file %s\n", tempfilename);
            }
        }
        if ( fprintf(handle, "\n") < 0)
        {
            fprintf(stderr, "Error: Can't write to temp file %s\n", tempfilename);
        }

    }
    fclose(handle);
    move();
    return;
}

int main(int argc, char *argv[])
{
    // Проверка Аргументов
    if (argc != 2)
    {
        fprintf(stderr, "Use: %s filename\n", argv[0]);
        return 1;
    }
    // Узнаём размер карты (из 0 и 1, прямоугольная, перенос=\n)
    FILE *handle = fopen(argv[1], "r");
    if (!handle)
    {
        fprintf(stderr, "Error: Can't open file %s\n", argv[1]);
        return 2;
    }
    int x=-1;
    int y=0;
    int temp=0;
    while (1)
    {
        char sym=fgetc(handle);
        if (sym == EOF) break;
        if (sym == '\n')
        {
            if (x==-1) x=temp;
            else if(x != temp)
            {
                fprintf(stderr, "Error: Different rows\n");
                return 3;
            }
            temp=0;
            ++y;
        }
        else if ((sym == '0') || (sym == '1')) ++temp;
        else
        {
            fprintf(stderr, "Error: Only 0 or 1 (or \\n) in input\n");
            return 4;
        }
    }
    // Даже 3x3 мало, хотя можно и меньше (необязательное условие) :-)
    if ((x < 3) && (y < 3))
    {
        fprintf(stderr, "Error: Bad map\n");
        return 5;
    }
    fclose(handle);
    // Считываем карту
    handle = fopen(argv[1], "r");
    if (!handle)
    {
        fprintf(stderr, "Error: Can't open file %s\n", argv[1]);
        return 2;
    }
    int ** field = (int **) malloc(y * sizeof(int *));
    for(int i=0; i<y; ++i)
    {
        field[i]=(int *) malloc(x * sizeof(int));
        for(int j=0; j<x; ++j)
        {
            char sym=fgetc(handle);
            if (sym == EOF)
            {
                fprintf(stderr, "Error: Something goes wrong!\n");
                return 6;
            }
            if(sym == '\n')
            {
                sym=fgetc(handle);
                if (sym == EOF)
                {
                    fprintf(stderr, "Error: Something goes wrong!\n");
                    return 6;
                }
            }
            field[i][j]=sym-'0';
        }
    }
    fclose(handle);
    X=x;
    Y=y;
    FIELD=field;

    pid_t pid;
    switch (pid=fork())
    {
        case -1:
            fprintf(stderr, "Error: Can't fork\n");
            return 7;
        case 0:
            serv();
            break;
        default:
            signal(SIGALRM, work);
            alarm(1);
            wait(NULL);
            break;
    }
}
