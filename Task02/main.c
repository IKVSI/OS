#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

//	Кулаков Владислав КБ-401

const int MaxSeek=INT_MAX-1;

int main(int argc, char *argv[])
{
    // Проверка числа аргументов
    if (argc != 2)
    {
        fprintf(stderr, "Use: | %s filename\n", argv[0]);
        return 1;
    }
    int handle = open(argv[1], O_WRONLY|O_EXCL|O_CREAT, 0666);
    // Проверка дескриптора
    if (handle < 0)
    {
        fprintf(stderr, "Error: File exists or You can't write in");
        return 2;
    }
    char sym;
    int seek=0;
    while(read(STDIN_FILENO, &sym, 1) > 0)
    {
        if (seek == MaxSeek)
        {
            if (lseek(handle, seek, SEEK_CUR) <= 0)
            {
                fprintf(stderr, "Error: Lseek fails");
                return 3;
            }
            seek=0;
        }
        // Увеличиваем смещение
        if (sym == 0) seek++;
        else if (seek != 0)
        {
            // Смещаем
            if (lseek(handle, seek, SEEK_CUR) <= 0)
            {
                fprintf(stderr, "Error: Lseek fails");
                return 3;
            }
            seek=0;
            // Дописываем символ
            if (write(handle, &sym, 1) <= 0)
            {
                fprintf(stderr, "Error: Write fails");
                return 4;
            }
        }
        // Пишем символы
        else
        {
            if (write(handle, &sym, 1) <= 0)
            {
                fprintf(stderr, "Error: Write fails");
                return 4;
            }
        }
    }
    // Проверка на последний 0
    if (sym == 0)
    {
        if (seek > 1)
        {
            if (lseek(handle, seek-1, SEEK_CUR) <= 0)
            {
                fprintf(stderr, "Error: Lseek fails");
                return 3;
            }
        }
        if (write(handle, &sym, 1) <= 0)
        {
            fprintf(stderr, "Error: Write fails");
            return 4;
        }
    }
    close(handle);
    return 0;
}
