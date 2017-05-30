#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>

//	Кулаков Владислав КБ-401

char * CONFPATH;
const char CONFNAME[11]="track.conf";

struct row
{
    char str[300];
    int tp;
    int k;
    pid_t pid;
};

int gettype(char * temp)
{
    int i=0;
    switch (temp[strlen(temp)-2])
    {
        case 'W':
            i=0;
            break;
        case 'R':
            i=1;
            break;
        default:
            i=-1;
            break;
    }
    temp[strlen(temp)-2]=0;
    return i;
}

int work()
{
    // Считаем кол-во строк в конф. файле=кол-во процессов
    FILE *handle = fopen(CONFPATH, "r");
    if (!handle)
    {
        fprintf(stderr, "Error: Can't open configuration file\n");
        exit(2);
    }
    int K=0;
    while(1)
    {
        char sym=fgetc(handle);
        if (sym == EOF) break;
        if (sym == '\n') K++;
    }
    struct row cmds[K];
    fclose(handle);
    // Читаем и парсим данные о процессах
    handle = fopen(CONFPATH, "r");
    if (!handle)
    {
        fprintf(stderr, "Error: Can't open configuration file\n");
        exit(2);
    }
    int i=0;
    while (1)
    {
        char * temp = fgets(cmds[i].str, 300, handle);
        if(!temp) break;
        cmds[i].tp=gettype(temp);
        cmds[i].k=50;
        if (cmds[i].tp < 0)
        {
            fprintf(stderr, "Error: Bad configuration file\n");
            exit(3);
        }
        ++i;
    }
    fclose(handle);
    char *rtemp;
    char temp[310];
    char *rargs[310];
    char trackfilename[25];
    for(int i=0; i<K; ++i)
    {
        switch (cmds[i].pid=fork())
        {
            case -1:
                fprintf(stderr, "Error: Can't start fork\n");
                exit(1);
            // Запускаем каждый процесс execvp
            case 0:
                strcpy(temp, cmds[i].str);
                rtemp = strtok(temp, " ");
                for(int i=0; i<300; i++)
                {
                    if (rtemp != NULL)
                    {
                        rargs[i] = (char *)malloc(strlen(temp));
                        strcpy(rargs[i], rtemp);
                        rtemp = strtok(NULL, " ");
                    }
                    else rargs[i] = NULL;
                }
                if (execvp(rargs[0], rargs) < 0)
                {
                    fprintf(stderr, "Error: exec\n");
                    exit(4);
                }
                exit(0);
                break;
             // Создаём и записываем идентификатор в файл
        default:
            sprintf(trackfilename, "/tmp/track.%d.pid", cmds[i].pid);
            FILE *handle = fopen(trackfilename, "w");
            if (!handle) syslog(LOG_WARNING, "Can't create /tmp/track.%d.pid", cmds[i].pid);
            else syslog(LOG_WARNING, "Created /tmp/track.%d.pid for (%s)", cmds[i].pid, cmds[i].str);
            if (fprintf(handle, "%i", cmds[i].pid) < 0) syslog(LOG_WARNING, "Can't write in /tmp/track.%d.pid", cmds[i].pid);
            fclose(handle);
            break;
        }
    }
    // Обработчик HUP сбрасываем настройку и запускаем work снова
    void hup(int sig)
    {
        for (int i=0; i<K; i++)
        {
            if (cmds[i].pid > 0)
            {
                kill(cmds[i].pid, SIGKILL);
                sprintf(trackfilename, "/tmp/track.%d.pid", cmds[i].pid);
                if (remove(trackfilename) < 0) syslog(LOG_WARNING, "Can't remove /tmp/track.%d.pid", cmds[i].pid);
            }
        }
        work();
    }
    signal(SIGHUP, hup);
    // Обработка завершённых
    while (1)
    {
        int cur = 0;
        for (int i=0; i<K; i++) if (cmds[i].pid != 0) cur++;
        if (cur == 0) exit(0);
        int status;
        pid_t pid = wait(&status);
        for (int i=0; i<K; ++i)
        {
            if(pid == cmds[i].pid)
            {
                sprintf(trackfilename, "/tmp/track.%d.pid", cmds[i].pid);
                if (remove(trackfilename) < 0) syslog(LOG_WARNING, "Can't remove /tmp/track.%d.pid", cmds[i].pid);
                if (status != 0) cmds[i].k -= 1;
                if ((status == 0) && (cmds[i].tp == 0)) cmds[i].pid=0;
                else if (cmds[i].tp == 1)
                {
                    // флаг для сна
                    int sleepfl=0;
                    if (cmds[i].k < 0)
                    {
                        cmds[i].k = 50;
                        sleepfl=1;
                        syslog(LOG_NOTICE, "(%s) exec failed, wait for 1 hour", cmds[i].str);
                    }
                    // перезапуск соответсвующик R процессов
                    switch(cmds[i].pid=fork())
                    {
                        case -1:
                            fprintf(stderr, "Error: Can't start fork\n");
                            exit(1);
                            break;
                        case 0:
                            strcpy(temp, cmds[i].str);
                            rtemp = strtok(temp, " ");
                            for(int i=0; i<300; ++i)
                            {
                                if (rtemp)
                                {
                                    rargs[i] = (char *)malloc(strlen(temp));
                                    strcpy(rargs[i], rtemp);
                                    rtemp = strtok(NULL, " ");
                                }
                                else rargs[i] = NULL;
                            }
                            if (sleepfl) sleep(3600);
                            if (execvp(rargs[0], rargs) < 0)
                            {
                                fprintf(stderr, "Error: exec\n");
                                exit(4);
                            }
                            exit(0);
                            break;
                        default:
                            sprintf(trackfilename, "/tmp/track.%d.pid", cmds[i].pid);
                            FILE *handle = fopen(trackfilename, "w");
                            if (!handle) syslog(LOG_WARNING, "Can't create /tmp/track.%d.pid", cmds[i].pid);
                            //else syslog(LOG_WARNING, "Created /tmp/track.%d.pid for (%s)", cmds[i].pid, cmds[i].str);
                            if (fprintf(handle, "%i", cmds[i].pid) < 0) syslog(LOG_WARNING, "Can't write in /tmp/track.%d.pid", cmds[i].pid);
                            fclose(handle);
                            break;
                    }
                }
            }
        }
    }
    return 0;
}

void getconfpath(char * progpath)
{
    progpath = realpath(progpath, NULL);
    int i=strlen(progpath)-1;
    for(; i>=0; --i)
    {
        if(progpath[i] == '/') break;
    }
    if(i == -1)
    {
        fprintf(stderr, "Error: Can't find configuration\n");
        exit(1);
    }
    else
    {
        char *confpath = (char *) malloc(i+12*sizeof(char *));
        for(int j=0; j<=i; ++j) confpath[j]=progpath[j];
        for(int j=0; j<11; ++j) confpath[j+i+1]=CONFNAME[j];
        CONFPATH=confpath;
    }
}

int main(int argc, char *argv[])
{
    // Путь до файла конфигурации
    getconfpath(argv[0]);
    int pid;
    switch (pid=fork())
    {
        case -1:
            fprintf(stderr, "Error: Can't start fork\n");
            return 1;
        case 0:
            umask(0);
            setsid();
            chdir("/");
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
            work();
            break;
        default:
            break;
    }
    return 0;
}
