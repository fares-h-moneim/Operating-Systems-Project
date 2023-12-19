#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
void time_step (int signum)
{
    remainingtime--;
}
void end (int signum)
{
    kill(getppid(),SIGUSR1);
    signal(SIGINT,SIG_DFL);
    destroyClk(false);
    raise(SIGINT);
}
int main(int agrc, char * argv[])
{
    signal(SIGINT, end);
    signal(SIGUSR1,time_step);
    remainingtime=  atoi(argv[1]);
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
    while (remainingtime > 0);
    raise(SIGINT);
    
    return 0;
}
