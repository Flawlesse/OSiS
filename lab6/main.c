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

// sem_t semaphore;
struct args_t
{
    int *array;
    int l;
    int m;
    int r;
};
int min(int x, int y) { return (x < y) ? x : y; }

void merge_synchro(int *array, int l, int m, int r)
{
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    // Create temp arrays
    int *L, *R;

    if ((L = (int *)malloc(n1 * sizeof(int))) == NULL)
    {
        perror("Allocation failed");
        exit(46);
    }
    if ((R = (int *)malloc(n2 * sizeof(int))) == NULL)
    {
        perror("Allocation failed");
        exit(50);
    }
    // printf("Got to here <merge_synchro>.\n");

    // Copy data to temp arrays L[] and R[]
    for (i = 0; i < n1; i++)
    {
        // printf("array[%d] ", l + i);
        L[i] = array[l + i];
        // printf("L[%d]=%d\n", i, L[i]);
    }
    // printf("First copy passed.\n");
    for (j = 0; j < n2; j++)
    {
        // printf("array[%d] ", m + 1 + j);
        R[j] = array[m + 1 + j];
        // printf("R[%d]=%d\n", j, R[i]);
    }
    // printf("After copy <merge_synchro>.\n");
    // sleep(10);

    // Merge the temp arrays back into array[l..r]
    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2)
    {
        if (L[i] <= R[j])
        {
            array[k++] = L[i++];
        }
        else
        {
            array[k++] = R[j++];
        }
    }
    while (i < n1)
    {
        array[k++] = L[i++];
    }
    while (j < n2)
    {
        array[k++] = R[j++];
    }
    // Free allocated memory
    free(L);
    free(R);
}

void synchronous_sort(int *array, int n)
{
    int curr_size;
    int left_start;

    for (curr_size = 1; curr_size <= n - 1; curr_size = 2 * curr_size)
    {
        for (left_start = 0; left_start < n - 1; left_start += 2 * curr_size)
        {
            int mid = min(left_start + curr_size - 1, n - 1);
            int right_end = min(left_start + 2 * curr_size - 1, n - 1);
            // printf("Got to here <synchronoush_sort>.\n");
            merge_synchro(array, left_start, mid, right_end);
        }
    }
}

void *merge_parallel(void *args)
{
    int *array = ((struct args_t *)args)->array;
    int left_start = ((struct args_t *)args)->l;
    int mid = ((struct args_t *)args)->m;
    int right_end = ((struct args_t *)args)->r;

    // sem_wait(&semaphore);
    merge_synchro(array, left_start, mid, right_end);
    // sem_post(&semaphore);
}

void parallel_sort(int *array, int n)
{
    int curr_size;
    int left_start;
    int curr_nthreads;

    pthread_t *threads = (pthread_t *)malloc(NUM_THREADS * sizeof(pthread_t)); //(pthread_t *)malloc((n / 2 + 1) * sizeof(pthread_t));
    if (threads == NULL)
    {
        perror("Cannot allocate memory for threads.\n");
        exit(123);
    }
    struct args_t *args = (struct args_t *)malloc(NUM_THREADS * sizeof(struct args_t)); //(struct args_t *)malloc((n / 2 + 1) * sizeof(struct args_t));
    if (args == NULL)
    {
        perror("Cannot allocate memory for args.\n");
        exit(128);
    }

    for (curr_size = 1; curr_size <= n - 1; curr_size = 2 * curr_size)
    {
        curr_nthreads = 0;
        for (left_start = 0; left_start < n - 1; left_start += 2 * curr_size)
        {
            int mid = min(left_start + curr_size - 1, n - 1);
            int right_end = min(left_start + 2 * curr_size - 1, n - 1);

            args[curr_nthreads].array = array;
            args[curr_nthreads].l = left_start;
            args[curr_nthreads].m = mid;
            args[curr_nthreads].r = right_end;

            pthread_create(&threads[curr_nthreads], NULL, &merge_parallel, &args[curr_nthreads]);
            curr_nthreads++;

            if (curr_nthreads == NUM_THREADS) {
                for (int i = 0; i < curr_nthreads; ++i){
                    pthread_join(threads[i], NULL);
                }
                curr_nthreads = 0;
            }
        }
        // We need to ALWAYS join at the end.
        for (int i = 0; i < curr_nthreads; ++i)
        {
            pthread_join(threads[i], NULL);
        }
    }

    free(threads);
    free(args);
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
    synchronous_sort(array_sync, len);
    end = clock();
    cpu_time_used = ((double)(end - start)) / (CLOCKS_PER_SEC / 1000);
    printf(
        "Time elapsed for synchronous sorting: %d ms.\n",
        (int)cpu_time_used);
    write_results(out_fh_sync, array_sync, len);

    // Parallel sorting
    // sem_init(&semaphore, 0, NUM_THREADS);
    struct args_t *args = (struct args_t *)malloc(sizeof(struct args_t));
    start = clock();
    parallel_sort(array_paral, len);
    end = clock();
    cpu_time_used = ((double)(end - start)) / (CLOCKS_PER_SEC / 1000);
    printf(
        "Time elapsed for parallel sorting (given %d threads): %d ms.\n",
        NUM_THREADS, (int)cpu_time_used);
    // sem_destroy(&semaphore);
    write_results(out_fh_paral, array_paral, len);

    free(buffer);
    free(array_sync);
    free(array_paral);
    close(inp_fh);
    close(out_fh_sync);
    close(out_fh_paral);
    return 0;
}