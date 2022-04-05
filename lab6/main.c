#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

int NUM_THREADS = 2;
const int MAX_LEN = 1000000;
const char *INFILEN = "input.txt";
const char *OUTFILEN_S = "output_sync.txt";
const char *OUTFILEN_P = "output_paral.txt";
const int MAX_THREADS = 256;
int thread_ind = 0;

sem_t semaphore;
pthread_t *threads = NULL;

struct args_t
{
    int *array;
    int l;
    int r;
    int rec_level;
    int in_paral;
};
struct args_t **info_handles = NULL;

void merge(int *array, int l, int mid, int r)
{
    int k = 0, i = l, j = mid + 1;
    int l_n = mid, r_n = r;
    int *subarray = NULL;
    if ((subarray = (int *)malloc((r - l + 1) * sizeof(int))) == NULL)
    {
        perror("Can't allocate memory.\n");
        exit(150);
    }

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
    for (; i <= l_n; ++i)
    {
        subarray[k++] = array[i];
    }
    for (; j <= r_n; ++j)
    {
        subarray[k++] = array[j];
    }

    // printf("Array[%d : %d]:\n", l, r + 1);
    for (i = l, k = 0; i <= r, k <= r - l; ++i, ++k)
    {
        array[i] = subarray[k];
        // printf("%d, ", array[i]);
    }
    // printf("\n");

    free(subarray);
}

void synchronous_sort(int *array, int l, int r)
{
    if (l < r)
    {
        int mid = (l + r) / 2;
        synchronous_sort(array, l, mid);
        synchronous_sort(array, mid + 1, r);
        merge(array, l, mid, r);
    }
}

void *parallel_sort(void *args)
{
    struct args_t _args = *(struct args_t *)args;
    printf("Args: %p, %d, %d; REC_LEVEL: %d", _args.array, _args.l, _args.r, _args.rec_level);
    struct args_t *left_args = NULL, *right_args = NULL;
    int CAN_PARAL = pow(2.0, (double)(_args.rec_level + 1)) >= NUM_THREADS && pow(2.0, (double)_args.rec_level) < NUM_THREADS;

    if (CAN_PARAL)
    {
        printf("On recursion level %d we make it parallel.\n", _args.rec_level);
    }
    if (_args.in_paral)
    {
        sem_wait(&semaphore);
    }

    if (_args.l < _args.r)
    {
        int mid = (_args.l + _args.r) / 2;

        if ((left_args = (struct args_t *)malloc(sizeof(struct args_t))) == NULL)
        {
            perror("Can't allocate memory.\n");
            exit(140);
        }
        left_args->array = _args.array;
        left_args->l = _args.l;
        left_args->r = mid;
        left_args->rec_level = _args.rec_level + 1;
        left_args->in_paral = CAN_PARAL;

        if ((right_args = (struct args_t *)malloc(sizeof(struct args_t))) == NULL)
        {
            perror("Can't allocate memory.\n");
            exit(140);
        }
        right_args->array = _args.array;
        right_args->l = mid + 1;
        right_args->r = _args.r;
        right_args->rec_level = _args.rec_level + 1;
        right_args->in_paral = CAN_PARAL;

        if (CAN_PARAL)
        {
            // two inner threads, they are controlled by semaphore from inside!!!
            pthread_t th[2];
            pthread_create(&th[0], NULL, &parallel_sort, left_args);
            pthread_create(&th[1], NULL, &parallel_sort, right_args);
            pthread_join(th[0], NULL);
            pthread_join(th[1], NULL);
        }
        else
        {
            parallel_sort(left_args);
            parallel_sort(right_args);
        }
        merge(_args.array, _args.l, mid, _args.r);

        if (_args.in_paral)
        {
            sem_post(&semaphore);
        }
        free(left_args);
        free(right_args);
    }
}

void write_results(int fd, int *array, int len)
{
    char buf[10];
    for (int i = 0; i < len - 1; ++i)
    {
        sprintf(buf, "%d, ", array[i]);
        write(fd, buf, strlen(buf));
    }
    sprintf(buf, "%d", array[len - 1]);
    write(fd, buf, strlen(buf));
}

int main(int argc, int *argv[])
{
    int inp_fh, out_fh_sync, out_fh_paral;
    if (argc > 5)
    {
        perror("Too much command line args given.");
        exit(5);
    }
    if (argc >= 2)
    {
        int nt = atoi((char *)argv[1]);
        if (nt)
        {
            if (nt > MAX_THREADS)
            {
                perror("Too much threads given!");
                exit(200);
            }
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
        if (argc >= 4)
        {
            if ((out_fh_sync = open((char *)argv[3], O_WRONLY | O_CREAT | O_TRUNC, 0777)) == -1)
            {
                fprintf(stderr, "Error opening file %s.\n", argv[3]);
                exit(3);
            }
        }
        else
        {
            if ((out_fh_sync = open(OUTFILEN_S, O_WRONLY | O_CREAT | O_TRUNC, 0777)) == -1)
            {
                fprintf(stderr, "Error opening file %s.\n", OUTFILEN_S);
                exit(3);
            }
        }
        if (argc == 5)
        {
            if ((out_fh_paral = open((char *)argv[4], O_WRONLY | O_CREAT | O_TRUNC, 0777)) == -1)
            {
                fprintf(stderr, "Error opening file %s.\n", argv[4]);
                exit(3);
            }
        }
        else
        {
            if ((out_fh_paral = open(OUTFILEN_P, O_WRONLY | O_CREAT | O_TRUNC, 0777)) == -1)
            {
                fprintf(stderr, "Error opening file %s.\n", OUTFILEN_P);
                exit(3);
            }
        }
    }
    // Allocate memory for thread handlers
    if ((threads = (pthread_t *)malloc(MAX_THREADS * sizeof(pthread_t))) == NULL)
    {
        perror("Cannot allocate memory for thread handlers.\n");
        exit(160);
    }
    if ((info_handles = (struct args_t **)malloc(MAX_THREADS * sizeof(struct args_t *))) == NULL)
    {
        perror("Cannot allocate memory for arguments.\n");
        exit(161);
    }

    // Get input data from file
    char *buffer = (char *)calloc(__INT_MAX__, 1);
    int *array_sync = (int *)calloc(MAX_LEN, sizeof(int));
    int *array_paral = (int *)calloc(MAX_LEN, sizeof(int));

    read(inp_fh, buffer, __INT_MAX__);
    int len = 0;

    char *word = strtok(buffer, ", ");
    while (word != NULL)
    {
        array_sync[len] = atoi(word);
        array_paral[len] = array_sync[len];

        word = strtok(NULL, ", ");
        ++len;
        if (len == MAX_LEN)
            break;
    }
    printf("\nLength is: %d\n", len);

    clock_t start, end;
    double cpu_time_used;
    // Synchronous sorting
    start = clock();
    synchronous_sort(array_sync, 0, len - 1);
    end = clock();
    cpu_time_used = ((double)(end - start)) / (CLOCKS_PER_SEC / 1000000);
    printf("Time elapsed for synchronous sorting: %.4f mcs.\n", cpu_time_used);
    write_results(out_fh_sync, array_sync, len);

    // Parallel sorting
    sem_init(&semaphore, 0, NUM_THREADS);
    struct args_t *args = (struct args_t *)malloc(sizeof(struct args_t));
    args->array = array_paral;
    args->l = 0;
    args->r = len - 1;
    args->rec_level = 0;
    args->in_paral = 0;
    parallel_sort(args);
    free(args);
    sem_destroy(&semaphore);
    write_results(out_fh_paral, array_paral, len);

    free(buffer);
    free(array_sync);
    free(array_paral);
    free(threads);
    free(info_handles);
    close(inp_fh);
    close(out_fh_sync);
    close(out_fh_paral);
    return 0;
}