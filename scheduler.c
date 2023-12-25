#include "DataStructures.h"
int msgq_generator;

struct process_recieved recieveMessage(int msgq)
{
    struct process_recieved Process;
    Process.id=-1;
    struct msgbuff_gen message;
    int rec=msgrcv(msgq, &message, sizeof(message.proc), 1, !IPC_NOWAIT);
    if(rec!=-1)
    Process=message.proc;
    
    return Process;
}

/// global vars
int current_time=0;
struct priority_queue q;
struct process* waiting_list;
int waiting_list_size=0;
struct process currently_running;
    //vars for statistics 
    FILE*scheduler_log,*scheduler_perf,*scheduler_mem;
    int total_waiting_time=0;
    int finished_process=0;
    int total_running_time=0;
    int* TA;
    float* WTA;
    //end of statistics vars
    //memory tree head
    struct mem_node* head;
//end of global vars
void process_end(int signum)
{
    printf("process notified scheduler of finish \n");
}
void run_p(struct process Process);
void stop_p(struct process Process);
void removeProcess()
{
    //memory freeing
    fprintf(scheduler_mem,"At time %d freed %d bytes from process %d ",current_time,currently_running.memsize,currently_running.id);
    remove_mem(head,currently_running.id,scheduler_mem);
    //
    TA[finished_process]=current_time-currently_running.arrivaltime;
    WTA[finished_process]=(1.0*TA[finished_process])/(currently_running.runningtime*1.0);
    total_waiting_time+=currently_running.wait_time;
    total_running_time+=currently_running.runningtime;
    float approx_WTA=round(WTA[finished_process]*100)/100.0;
    fprintf(scheduler_log,"At time %d process %d finished arr %d total %d remaining %d wait %d TA %d WTA %.2f\n",current_time,currently_running.id,currently_running.arrivaltime,currently_running.runningtime,currently_running.remaining_time,currently_running.wait_time,TA[finished_process],approx_WTA);
    printf("At time %d process %d finished arr %d total %d remaining %d wait %d TA %d WTA %.2f\n",current_time,currently_running.id,currently_running.arrivaltime,currently_running.runningtime,currently_running.remaining_time,currently_running.wait_time,TA[finished_process],approx_WTA);
    
    finished_process++;
    currently_running.id=-1;
}

void HPF(int current_time)
{
    if(currently_running.id!=-1)
    {
        return;
    }
    struct process Process=dequeue(&q);
    if(Process.id!=-1)
    {
        run_p(Process);
    }
}
void SRTN(int previous_time,int current_time)
{
    //display the current time and previous time when different
    if(currently_running.id==-1)
    {   
        struct process Process=dequeue(&q);
        if(Process.id!=-1)
        {
            run_p(Process);
        }
    }
    else
    {
        struct process Process = peek(q);
        if(Process.id==-1)
        {
            return;
        }
        if(currently_running.remaining_time>Process.remaining_time)
        {
            stop_p(currently_running);
            enqueue(&q,currently_running,currently_running.remaining_time);
            struct process Process=dequeue(&q);
            if(Process.id!=-1)
            {
                run_p(Process);
            }
        }
    }

}
int previous_RR_time=0;
void RR(int current_time,const int quantum)
{
    if(currently_running.id==-1) //if there is no process running
    {
        if(q.n>0) //if there is a process in the queue
        {
            struct process Process=dequeue(&q);
            run_p(Process);
            previous_RR_time=current_time;
        }
    }
    else
    {
        if((current_time-previous_RR_time)<quantum) //if the process has not finished its quantum
            return;
        previous_RR_time=current_time;
        if(q.n==0)
        {
            return;
        }
        stop_p(currently_running);
        enqueue(&q,currently_running,0);
        struct process Process=dequeue(&q);
        run_p(Process);
    }

}
//this function inserts the process in the waiting list ,later a function will try to allocate it in memory
void insert_waiting_list(struct process Process)
{
    waiting_list_size++;
    waiting_list=(struct process*)realloc(waiting_list,waiting_list_size*sizeof(struct process));
    waiting_list[waiting_list_size-1]=Process;
    
}
void remove_waiting_list(int index)
{
    for(int i=index;i<waiting_list_size-1;i++)
    {
        waiting_list[i]=waiting_list[i+1];
    }
    waiting_list_size--;
    waiting_list=(struct process*)realloc(waiting_list,waiting_list_size*sizeof(struct process));
}
//this function checks if any process from the waiting list can be allocated in memory

int main(int argc, char * argv[])
{
    //queue and currenlty_running initializations
    q.head=NULL;
    q.tail=NULL;
    q.n=0;
    currently_running.id=-1;
    //memory tree initializations
    head=create_node(1024,0,0,1023,NULL);
    //argv[1] is the scheduling algo //argv[2] is its parameters
    signal(SIGUSR1, process_end);
    key_t key=ftok("key",genSchedulerQ);
    msgq_generator=msgget(key,IPC_CREAT|0666);
    initClk();
    int previous_time=getClk();
    int quantum=atoi(argv[2]);
    int all_processes=atoi(argv[3]);
    //initiate the statistics
    scheduler_log=fopen("scheduler.log","w");
    scheduler_perf=fopen("scheduler.perf","w");
    scheduler_mem=fopen("memory.log","w");
    TA=(int*)malloc(all_processes*sizeof(int));
    WTA=(float*)malloc(all_processes*sizeof(float));
    while(1){
        struct process_recieved Process_recieved;
        while(1){
            Process_recieved=recieveMessage(msgq_generator);
            current_time=getClk();
            if(Process_recieved.id<=0)
                break;
            printf("recieved process %d at time %d \n",Process_recieved.id,current_time);
            struct process Process;
            Process.arrivaltime=Process_recieved.arrivaltime;
            Process.priority=Process_recieved.priority;
            Process.runningtime=Process_recieved.runningtime;
            Process.id=Process_recieved.id;
            Process.start_time=-1;
            Process.remaining_time=Process.runningtime;
            Process.state=0;
            Process.memsize=Process_recieved.memsize;
            insert_waiting_list(Process);
        }
        //check if any process in the waiting list can be allocated in memory
        for(int i=0;i<waiting_list_size;i++) //the complexity here is legendary
        {
            struct process Process=waiting_list[i];
            struct mem_node* inserted=insert_process(head,Process.memsize,Process.id);
            if(inserted!=NULL)
            {
                fprintf(scheduler_mem,"At time %d allocated %d bytes for process %d from %d to %d\n",current_time,Process.memsize,Process.id,inserted->start,inserted->end);
                int pid=fork();
                if(pid==0)
                {
                    char str[12];
                    num_to_str(str,Process.runningtime);
                    char *args[]={"./process.out",str,NULL};
                    execv(args[0],args);
                }
                else
                {
                    Process.pid=pid;
                }
                kill(pid,SIGSTOP);
                int priority=0;
                if(argv[1][0]=='H')
                {
                    priority=Process.priority;
                }
                if(argv[1][0]=='S')
                {
                priority=Process.remaining_time;
                }
                printf("inserting process %d with priority %d\n",Process.id,priority);
                enqueue(&q,Process,priority);
                print_queue(q);
                //remove inserted node from waiting list
                remove_waiting_list(i);
                i--;
            }
        }
        
        if(previous_time!=current_time) //if a time step occured instruct a process to decrement its remaining time
        {
            if(currently_running.id!=-1)
            {
                currently_running.remaining_time--;
                kill(currently_running.pid,SIGUSR1);
                if(currently_running.remaining_time==0)
                {
                    removeProcess();
                }
            }
        }
        if(Process_recieved.id==-2 && q.n==0 && currently_running.id==-1 && waiting_list_size==0)
        {
            //i finished btw
            printf("number of finished processes %d\n",finished_process);
            float CPU_utilization=(total_running_time*1.0)/(current_time*1.0);
            CPU_utilization=round(CPU_utilization*10000.0)/100.0;
            float avg_waiting_time=(total_waiting_time*1.0)/(all_processes*1.0);
            avg_waiting_time=round(avg_waiting_time*100.0)/100.0;
            float avg_turnaround_time=0;
            for(int i=0;i<all_processes;i++)
            {
                avg_turnaround_time+=WTA[i];
            }
            avg_turnaround_time=(avg_turnaround_time*1.0)/(all_processes*1.0);
            avg_turnaround_time=round(avg_turnaround_time*100.0)/100.0;
            float std_turnaround_time=0;
            for(int i=0;i<all_processes;i++)
            {
                std_turnaround_time+=pow(WTA[i]-avg_turnaround_time,2);
            }
            std_turnaround_time=(std_turnaround_time*1.0)/(all_processes*1.0);
            std_turnaround_time=sqrt(std_turnaround_time);
            std_turnaround_time=round(std_turnaround_time*100.0)/100.0;
            fprintf(scheduler_perf,"CPU utilization = %.2f%%\n",CPU_utilization);
            fprintf(scheduler_perf,"Avg WTA = %.2f\n",avg_turnaround_time);
            fprintf(scheduler_perf,"Avg Waiting = %.2f\n",avg_waiting_time);
            fprintf(scheduler_perf,"Std WTA = %.2f\n",std_turnaround_time);
            fclose(scheduler_perf);
            fclose(scheduler_mem);
            break;
        }
        if(argv[1][0]=='H')
        {
            HPF(current_time);
        }
        if(argv[1][0]=='R')
        {
            RR(current_time,quantum);  
        }
        if(argv[1][0]=='S')
        {
            SRTN(previous_time,current_time);  
        }
        previous_time=current_time;
    }
    //free resources
    fclose(scheduler_log);
    free(TA);
    free(WTA);
    
    destroyClk(true);
}
void run_p(struct process Process){
    Process.state=1;
    kill(Process.pid,SIGCONT);
    currently_running=Process;
    if(currently_running.start_time==-1)
    {
        currently_running.start_time=current_time;
        currently_running.wait_time=currently_running.start_time-currently_running.arrivaltime;
        fprintf(scheduler_log,"At time %d process %d started arr %d total %d remaining %d wait %d\n",current_time,currently_running.id,currently_running.arrivaltime,currently_running.runningtime,currently_running.remaining_time,currently_running.wait_time);
        printf("At time %d process %d started arr %d total %d remaining %d wait %d\n",current_time,currently_running.id,currently_running.arrivaltime,currently_running.runningtime,currently_running.remaining_time,currently_running.wait_time);
        
    }
    else
    {

        currently_running.wait_time=currently_running.start_time-currently_running.arrivaltime+(current_time-currently_running.start_time)-(currently_running.runningtime-currently_running.remaining_time);
        fprintf(scheduler_log,"At time %d process %d resumed arr %d total %d remaining %d wait %d\n",current_time,currently_running.id,currently_running.arrivaltime,currently_running.runningtime,currently_running.remaining_time,currently_running.wait_time);
        printf("At time %d process %d resumed arr %d total %d remaining %d wait %d\n",current_time,currently_running.id,currently_running.arrivaltime,currently_running.runningtime,currently_running.remaining_time,currently_running.wait_time);
        
    }
}
void stop_p(struct process Process)
{
    currently_running.state=0;
    kill(Process.pid,SIGSTOP);
    if(Process.remaining_time!=0)
    {
        fprintf(scheduler_log,"At time %d process %d stopped arr %d total %d remaining %d wait %d\n",current_time,Process.id,Process.arrivaltime,Process.runningtime,Process.remaining_time,Process.wait_time);
        printf("At time %d process %d stopped arr %d total %d remaining %d wait %d\n",current_time,Process.id,Process.arrivaltime,Process.runningtime,Process.remaining_time,Process.wait_time);
    }
}

