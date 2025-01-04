#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define SEM_1_PONG 0
#define SEM_2_PING 1

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("ERROR");
        exit(0);
    }

    // Save argument as an integer
    int pingpong_n = atoi(argv[1]);

    if (pingpong_n < 1)
    {
        printf("ERROR");
        exit(0);
    }

    // Opening both semaphores
    int sem_pong = find_free_id(0);
    int sem_ping = find_free_id(1);

    int sem_1_err = sem_open(sem_pong, 0);
    int sem_2_err = sem_open(sem_ping, 1);
    // if no argument is given, print error

    // Checking for errors and calling fork()
    if (sem_1_err != 0 && sem_2_err != 0)
    {
        int pid = fork();

        // Child process, prints "Ping"
        if (pid == 0)
        {
            for (int i = 0; i < pingpong_n; i++)
            {
                if (sem_1_err != 0 && sem_2_err != 0)
                {
                    sem_2_err = sem_down(sem_ping);
                    printf("Ping\n");
                    sem_1_err = sem_up(sem_pong);
                }
                else
                {
                    printf("Semaphore error");
                    break;
                }
            }
        }

        // Parent process, prints "Pong"
        else
        {
            for (int i = 0; i < pingpong_n; i++)
            {
                if (sem_1_err != 0 && sem_2_err != 0)
                {
                    sem_1_err = sem_down(sem_pong);
                    printf("\tPong\n");
                    sem_2_err = sem_up(sem_ping);
                }
                else
                {
                    printf("Semaphore error");
                    break;
                }
            }
            // Closing semaphores before exiting
            sem_1_err = sem_close(sem_pong);
            sem_2_err = sem_close(sem_ping);
            if (sem_1_err != 0 && sem_2_err != 0)
            {
                exit(0);
            }
            else
            {
                printf("Error when closing semaphores");
                exit(0);
            }
        }
    }

    else
    {
        printf("Error");
    }
    exit(0);
}
