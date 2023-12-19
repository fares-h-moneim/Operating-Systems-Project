#include <stdio.h>      //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300
#define genSchedulerQ 0
#define clksem 1

///==============================
//don't mess with this variable//
int * shmaddr;                 //
//===============================
union Semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};
void down(int sem)
{
    struct sembuf op;
    op.sem_num=0;
    op.sem_op=-1;
    op.sem_flg=!IPC_NOWAIT;
    if(semop(sem,&op,1)==-1)
    {
        perror("ERROR IN DOWN");
        exit(-1);
    }

}
void up(int sem)
{
    struct sembuf op;
    op.sem_num=0;
    op.sem_op=1;
    op.sem_flg=!IPC_NOWAIT;
    if(semop(sem,&op,1)==-1)
    {
        perror("ERROR IN DOWN");
        exit(-1);
    }
}
double round(double d)
{
    return (int)(d + 0.5);
}
double pow(double base, int exp)
{
    double result = 1;
    for(int i=0;i<exp;i++)
    {
        result*=base;
    }
    return result;
}
double sqrt(double num) {
    double guess = num / 2.0;
    double epsilon = 0.00001;

    while ((guess * guess - num) >= epsilon || (num - guess * guess) >= epsilon) {
        guess = (guess + num / guess) / 2.0;
    }

    return guess;
}


int getClk()
{
    return *shmaddr;
}

void num_to_str(char *str, int num) {
    sprintf(str, "%d", num);
}
/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *) shmat(shmid, (void *)0, 0);
}


/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}
