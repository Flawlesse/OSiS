#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

int NUM_THREADS = 2;
const int MAX_LEN = 1000000;
const char *INFILEN = "input.txt";
const char *OUTFILEN = "output.txt";

void synchro_merge(int *array, int l, int mid, int r)
{
    int k = 0, i = l, j = mid + 1;
    int l_n = mid, r_n = r;
    int *subarray = (int *)malloc((r - l + 1) * sizeof(int));

    while (i <= l_n && j <= r_n)
    {
        if (array[i] < array[j])
        {
            subarray[k++] = array[i++];
        }
        else
        {
            subarray[k++] = array[j++];
        }
    }
    // then appending ends to the array
    for (; i <= l_n; ++i) {
        subarray[k++] = array[i];
    }
    for (; j <= r_n; ++j){
        subarray[k++] = array[j];
    }

    printf("Array[%d : %d]:\n", l, r + 1);
    for (i = l, k = 0; i <= r, k <= r - l; ++i, ++k) {
        array[i] = subarray[k];
        printf("%d, ", array[i]);
    }
    printf("\n");

    free(subarray);
}

void synchronous_sort(int *array, int l, int r)
{
    if (l < r)
    {
        int mid = (l + r) / 2;
        synchronous_sort(array, l, mid);
        synchronous_sort(array, mid + 1, r);
        synchro_merge(array, l, mid, r);
    }
}

void *parallel_sort(void *args)
{
}

int main(int argc, int *argv[])
{
    int inp_fh, out_fh;
    if (argc > 4)
    {
        perror("Too much command line args given.");
        exit(4);
    }
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
        if (argc == 4)
        {
            if ((out_fh = open((char *)argv[3], O_WRONLY | O_CREAT | O_TRUNC, 0777)) == -1)
            {
                fprintf(stderr, "Error opening file %s.\n", argv[3]);
                exit(3);
            }
        }
        else
        {
            if ((out_fh = open(OUTFILEN, O_WRONLY | O_CREAT | O_TRUNC, 0777)) == -1)
            {
                fprintf(stderr, "Error opening file %s.\n", OUTFILEN);
                exit(3);
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
    printf("\nLength is: %d\n", len);

    synchronous_sort(array, 0, len - 1);
    char buf[10];
    for (int i = 0; i < len - 1; ++i)
    {
        sprintf(buf, "%d, ", array[i]);
        write(out_fh, buf, strlen(buf));
    }
    sprintf(buf, "%d", array[len - 1]);
    write(out_fh, buf, strlen(buf));

    free(buffer);
    free(array);
    close(inp_fh);
    close(out_fh);
    return 0;
}