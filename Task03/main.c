#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

//	Кулаков Владислав КБ-401

int cmp(const void *x, const void *y)
{
    if (*(long long*)x < *(long long *)y) return -1;
    else if (*(long long*)x == *(long long *)y) return 0;
    else return 1;
}

int parse(int handle, long long *num)
{
    char decnum[20];
    for(int i=0; i<20; ++i) decnum[i]=0;
    char sym;
    int j=0;
    int skip=0;
    while (read(handle, &sym, 1) > 0)
    {
        // Сброс очень больших чисел
        if (skip)
        {
            if ((sym >= '0') && (sym <= '9')) continue;
            else
            {
                for(int i=0; i<20; ++i) decnum[i]=0;
                j=0;
                skip=0;
            }
        }
        // Сборка числа
        if ((j == 0) && (sym == '-'))
        {
            decnum[j]=sym;
            ++j;
        }
        else if ((sym >= '0') && (sym <= '9'))
        {
            if (j < 19)
            {
                decnum[j]=sym;
                ++j;
            }
            else
            {
                fprintf(stderr, "Error: Too big number %s...\n", decnum);
                skip=1;
            }
        }
        // Возврат числа
        else
        {
            if (j == 0) continue;
            if ((j == 1) && (decnum[j]=='-')) j=0;
            else
            {
                *num = atoll(decnum);
                return 0;
            }
        }
    }
    if ((j == 1) && (decnum[j]=='-')) return 1;
    else if (j != 0)
    {
        *num = atoll(decnum);
        return 0;
    }
    // Конец файла
    return 1;
}

int main(int argc, char *argv[])
{
    // Проверка аргументов
    if (argc < 3)
    {
        fprintf(stderr, "Use: %s [input file]+ [output file]\n", argv[0]);
        return 1;
    }
    // Открываем выходной файл
    int handleout = open(argv[argc-1], O_WRONLY|O_EXCL|O_CREAT, 0666);
    if (handleout < 0)
    {
        fprintf(stderr, "Error: Can't open output file %s\n", argv[argc-1]);
        return 2;
    }
    // Переменные для обработки
    long long num;
    long long arrn=0;
    long long *arr = (long long *) malloc(0 * sizeof(long long));
    if (!arr)
    {
        fprintf(stderr, "Error: Can't alloc memorry\n");
        close(handleout);
        return 3;
    }
    // Цикл по именам файлов
    for (int i=1; i < argc-1; ++i)
    {
        int handle = open(argv[i], O_RDONLY);
        if (handle < 0) fprintf(stderr, "Error: Can't open input file %s\n", argv[i]);
        // Разбор чисел
        while (parse(handle, &num) == 0)
        {
            ++arrn;
            arr = realloc(arr, arrn*sizeof(long long));
            if (!arr)
            {
                fprintf(stderr, "Error: Can't realloc memorry\n");
                close(handleout);
                close(handle);
                return 3;
            }
            arr[arrn-1]=num;
        }
        close(handle);
    }
    // Сортировка и вывод
    qsort(arr, arrn, sizeof(long long), cmp);
    for (long long int i=0; i<arrn; i++)
    {
        dprintf(handleout, "%lld\n", arr[i]);
    }
    close(handleout);
    free(arr);
    return 0;
}
