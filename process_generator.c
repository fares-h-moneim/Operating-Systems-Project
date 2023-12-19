#include "headers.h"
//process struct
struct process
{
    int arrivaltime;
    int priority;
    int runningtime;
    int id;
};
struct msgbuff
{
    long mtype;
    struct process proc;
};
//here are resources to be freed
struct process *processes;
int msgq;
//end of resources
void clearResources(int);

//reads input from file to make processes
int readInput(struct process **processes)
{
    FILE * pFile;
    pFile = fopen("processes.txt", "r");
    if(pFile==NULL)
    {
        perror("error in opening file\n");
        exit(1);
    }
    char line[100];
    int count=0;
    while(fgets(line,sizeof(line),pFile)!=NULL)
    {
        if(line[0]=='#')
        {
            continue;
        }
        count++;
    }
    fseek(pFile,0,SEEK_SET);
    *processes=(struct process *)malloc(count*sizeof(struct process));
    int i=0;
    while(fgets(line,sizeof(line),pFile)!=NULL)
    {
        if(line[0]=='#')
        {
            continue;
        }
        sscanf(line,"%d\t%d\t%d\t%d",&((*processes)[i].id),&((*processes)[i].arrivaltime),&((*processes)[i].runningtime),&((*processes)[i].priority));
        i++;
    }
    return count;
}
//starts the clock and scheduler process
int schedulerpid;
void Initiate(char *schedulerpath,char*algorithm,char*params,char*numbOfProcesses)
{
    int pid=fork();
    if(pid==0)
    {
    execl("./clk.out",(char*)NULL);
    }
    sleep(1);
    pid=fork();
    if(pid==0)
    {
    execl("./scheduler.out","scheduler.out",algorithm,params,numbOfProcesses,(char*)NULL);
    }
    else
    {
        schedulerpid=pid;
    }
    
}
//sends a process to the scheduler at its time
void sendToScheduler(struct process Process)
{
    //printf("sending process %d at time %d\n",Process.id,getClk());
    struct msgbuff message;
    message.mtype=1;
    message.proc=Process;
    int send=msgsnd(msgq, &message, sizeof(message.proc), !IPC_NOWAIT);
}
int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    key_t key=ftok("key",genSchedulerQ);
    msgq=msgget(key,IPC_CREAT|0666);
    key=ftok("key",clksem);
    int sem1=semget(key,1,IPC_CREAT|0666);
    //semaphore for clk
    // 1. Read the input files.
    int nProcesses=readInput(&processes);
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    const char schedulerpath[]="./scheduler.out";
    char algorithm[10];
    char params[5];
    printf("choose algorithm RR,SRTN,HPF\n");
    scanf("%s",algorithm);
    if(algorithm[0]=='R')
    {
        printf("enter the parameter Q\n");
        scanf("%s",params);
    }

    // 3. Initiate and create the scheduler and clock processes.
    char nProcessesstr[20];
    num_to_str(nProcessesstr,nProcesses);
    Initiate(schedulerpath,algorithm,params,nProcessesstr);
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    int i=0;
    while(i<nProcesses){
        down(sem1);
        int x=getClk();
        
        while(x==processes[i].arrivaltime){
            sendToScheduler(processes[i]);
            i++;
        }
        struct process Process;
        Process.id=-1;
        Process.arrivaltime=x;
        sendToScheduler(Process);
    }
    while(1)
    {
        down(sem1);
        struct process Process;
        Process.id=-2;
        Process.arrivaltime=getClk();
        sendToScheduler(Process);
    }
    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    kill(schedulerpid,SIGINT);
    destroyClk(true);
    free(processes);
    msgctl(msgq,IPC_RMID,(struct msqid_ds*)0);
    signal(SIGINT,SIG_DFL);
    raise(SIGINT);
    //TODO Clears all resources in case of interruption
}
