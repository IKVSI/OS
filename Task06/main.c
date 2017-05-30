#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

//	Кулаков Владислав КБ-401

int main(int argc, char *argv[])
{
    // Проверка аргументов
    if (argc != 3)
    {
        fprintf(stderr, "Use: %s [file] [msg]\n", argv[0]);
        return 1;
    }

    char *lockpath;
    strcpy(lockpath, argv[1]);
    strcat(lockpath, ".lck");
    printf("Start wait...\n");
    while(1)
    {
        int handle = open(lockpath, O_WRONLY|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
        if (handle >= 0)
        {
            pid_t pid = getpid();
            if (dprintf(handle, "W%d\n", pid) < 0)
            {
                fprintf(stderr, "Error: Can't write to (%s)\n", lockpath);
                return 2;
            }
            close(handle);
            break;
        }
    }
    printf("End wait.\n");

    // Записываем msg (argv[2])
    FILE *handle = fopen(argv[1], "a");
    if (!handle)
    {
        fprintf(stderr, "Error: Can't open (%s)\n", argv[1]);
        return 3;
    }
    if (fprintf(handle, "%s\n", argv[2]) < 0)
    {
        fprintf(stderr, "Error: Can't write to (%s)\n", argv[1]);
        return 2;
    }
    fclose(handle);
    // для проверки, перед удалением lck
    sleep(10);
    if (remove(lockpath) < 0) {
        fprintf(stderr, "Error: Can't remove (%s)\n", lockpath);
        return 4;
    }
    return 0;
}
