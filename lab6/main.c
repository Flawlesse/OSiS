#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

int NUM_THREADS = 2;
const int MAX_LEN = 1000000;
const char *INFILEN = "input.txt";
const char *OUTFILEN = "output.txt";

int *merge_sort(int *up, int *down, unsigned int left, unsigned int right)
{
    if (left == right)
    {
        down[left] = up[left];
        return down;
    }

    unsigned int middle = left + (right - left) / 2;

    // разделяй и сортируй
    int *l_buff = merge_sort(up, down, left, middle);
    int *r_buff = merge_sort(up, down, middle + 1, right);

    // слияние двух отсортированных половин
    int *target = l_buff == up ? down : up;

    unsigned int l_cur = left, r_cur = middle + 1;
    for (unsigned int i = left; i <= right; i++)
    {
        if (l_cur <= middle && r_cur <= right)
        {
            if (l_buff[l_cur] < r_buff[r_cur])
            {
                target[i] = l_buff[l_cur];
                l_cur++;
            }
            else
            {
                target[i] = r_buff[r_cur];
                r_cur++;
            }
        }
        else if (l_cur <= middle)
        {
            target[i] = l_buff[l_cur];
            l_cur++;
        }
        else
        {
            target[i] = r_buff[r_cur];
            r_cur++;
        }
    }
    return target;
}

int main(int argc, int *argv[])
{
    int inp_fh, out_fh;
    if (argc >= 2)
    {
        int nt = atoi((char *)argv[1]);
        if (nt)
        {
            NUM_THREADS = nt;
        }
        else
        {
            perror("Incorrect number of threads!\n");
            exit(1);
        }
        if (argc >= 3)
        {
            if ((inp_fh = open((char *)argv[2], O_RDONLY)) == -1)
            {
                fprintf(stderr, "No such file %s.\n", argv[2]);
                exit(2);
            }
        }
        else
        {
            if ((inp_fh = open(INFILEN, O_RDONLY)) == -1)
            {
                fprintf(stderr, "No such file %s.\n", INFILEN);
                exit(2);
            }
        }
    }

    // get data from file
    char *buffer = (char *)calloc(__INT_MAX__, 1);
    int *array = (int *)calloc(MAX_LEN, sizeof(int));

    read(inp_fh, buffer, __INT_MAX__);
    int len = 0;

    char *word = strtok(buffer, ", ");
    while (word != NULL)
    {
        array[len] = atoi(word);
        word = strtok(NULL, ", ");
        ++len;
        if (len == MAX_LEN)
            break;
    }

    for (int i = 0; i < len; ++i)
    {
        printf("%d, ", array[i]);
    }
    printf("\nLength is: %d\n", len);

    free(buffer);
    free(array);
    close(inp_fh);
    close(out_fh);
    return 0;
}