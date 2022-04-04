#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>

const size_t N = 100;

void *create_shared_memory(size_t size)
{
    // Our memory buffer will be readable and writable:
    int protection = PROT_READ | PROT_WRITE;

    // The buffer will be shared (meaning other processes can access it), but
    // anonymous (meaning third-party processes cannot obtain an address for it),
    // so only this process and its children will be able to use it:
    int visibility = MAP_SHARED | MAP_ANONYMOUS;

    // The remaining parameters to `mmap()` are not important for this use case,
    // but the manpage for `mmap` explains their purpose.
    return mmap(NULL, size, protection, visibility, -1, 0);
}

void do_routine(int fhandler, int *step)
{
    for (; *step < N; (*step) += 1)
    {
        // printf("Doing routine step=%d\n", *step);
        char buf[3];
        buf[2] = 0;
        int len = sprintf(buf, "%d", *step + 1);
        if (len == -1)
        {
            perror("Could not convert number properly.\n");
            exit(2);
        }

        sleep(1);
        write(fhandler, "Seconds passed: ", 16);
        write(fhandler, buf, len);
        write(fhandler, "\n", 1);
    }
}

void watch(int pid)
{
    int status;
    pid_t result;

    fflush(stdout);
    while (1)
    {
        if ((result = waitpid(pid, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0)
        {
            // Signal has been received
            printf("--> SIGNAL RECEIVED!\n");
            fflush(stdout);

            if (WIFSTOPPED(status))
            {
                printf("--> SIGNAL SIGSTOP!\n");
                fflush(stdout);

                printf("No stop for pid=%d!\n", pid);
                kill(result, SIGCONT);
            }
            else if (WIFEXITED(status))
            {
                printf("Process pid=%d exited normally.\n", pid);
                return;
            }
            else if (WIFCONTINUED(status))
            {
                printf("Process pid=%d is still alive.\n", pid);
            }
            else
            {
                printf("Process pid=%d got killed.\n", pid);
                return;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int fhandler;
    if (argc == 2)
    {
        fhandler = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0777);
        if (fhandler == -1)
        {
            perror("Something went wrong. Getting file handler failed.");
            return 1;
        }
    }

    int pid = fork();
    if (pid == 0)
    {
        // Manager process code
        // printf("Starting daemon...\nManager pid=%d, ppid=%d\n", getpid(), getppid());
        fflush(stdout);

        int pid_agent;
        int watcher_pid;
        printf("Manager started.\n");
        fflush(stdout);

        // This can only be used by manager and its children
        int *step = (int *)create_shared_memory(sizeof(int));
        *step = 0;
        while (*step < N)
        {
            // Here, agent is either dead or doesn't exist yet
            // If work not finished, then fork again
            if (*step < N)
            {
                // Set up watcher for SIGTSTP and SIGSTOP
                watcher_pid = fork();
                if (watcher_pid == 0)
                {
                    // printf("WATCHER pid=%d, ppid=%d\n", getpid(), getppid());
                    // replace dead pid_agent with new one
                    pid_agent = fork();
                    if (pid_agent == 0)
                    {
                        // Agent code
                        printf("Agent: fhandler=%d, pid=%d, ppid=%d\n", fhandler, getpid(), getppid());
                        fflush(stdout);
                        do_routine(fhandler, step);
                        exit(0);
                    }
                    watch(pid_agent);
                    exit(0);
                }
            }
            usleep(10000); // wait for agent to run (synchronize)
            waitpid(watcher_pid, NULL, 0); // wait for termination
            printf("Execution stopped. step=%d\n", *step);
        }
    }
    else
    {
        exit(0);
    }

    close(fhandler);
    printf("Closed file handler\n");
    exit(0);
}
