#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

// Global array where semaphores are stored
#define MAX_SEM 256
struct s_semaphore
{
    uint is_init; // 0 if semaphore is not initialized (closed)
    uint value;
    uint locked_procs; // Keeping track of how many processes are sleeping on this semaphores chan
    struct spinlock semlk;
};
typedef struct s_semaphore semaphore;
semaphore sem_array[MAX_SEM];

// Semaphore initialization function
uint64 seminit(void)
{
    for (int i = 0; i < MAX_SEM; i++)
    {
        if (sem_array[i].is_init != 1)
        {
            sem_array[i].is_init = 0;
        }
    }

    return 1;
}

uint64 find_free_id(void)
{
    int sem;
    argint(0, &sem);
    while (sem_array[sem].is_init != 0)
    {
        sem += 2;
    }

    return sem;
}

uint64
sys_sem_open(void)
{
    int sem;
    int value;
    argint(0, &sem);
    argint(1, &value);
    char name = sem;
    initlock(&sem_array[sem].semlk, &name);
    sem_array[sem].value = value;
    sem_array[sem].is_init = 1;
    sem_array[sem].locked_procs = 0;

    return 1;
}

uint64
sys_sem_close(void)
{
    int sem;
    argint(0, &sem);
    if (sem < 0 || sem > MAX_SEM || !sem_array[sem].is_init)
    {
        return 0;
    }
    sem_array[sem].is_init = 0;

    return 1;
}

uint64
sys_sem_up(void)
{
    int sem;
    argint(0, &sem);

    if (!(sem_array[sem].is_init == 1))
    {
        return 0;
    }

    acquire(&sem_array[sem].semlk);
    // Wake up sleeping processes if sem value is 0 and there are processes to unlock
    if (sem_array[sem].value == 0 && sem_array[sem].locked_procs != 0)
    {
        wakeup(&sem_array);
        sem_array[sem].locked_procs = 0;
    }
    else
    {
        sem_array[sem].value++;
    }
    release(&sem_array[sem].semlk);

    return 1;
}

uint64
sys_sem_down(void)
{
    int sem;
    argint(0, &sem);
    if (!(sem_array[sem].is_init == 1))
    {
        return 0;
    }

    acquire(&sem_array[sem].semlk);
    // Put calling process to sleep, if required
    if (sem_array[sem].value == 0)
    {
        sem_array[sem].locked_procs++;
        sleep(&sem_array, &sem_array[sem].semlk);
    }
    else
    {
        sem_array[sem].value--;
    }
    release(&sem_array[sem].semlk);

    return 1;
}
