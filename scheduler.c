#include "headers.h"
int msgq_generator;
struct process_recieved
{
    int arrivaltime;
    int priority;
    int runningtime;
    int id;
};
struct process
{
    int arrivaltime;
    int priority;
    int runningtime;
    int id;
    int pid;
    //for statistics
    int wait_time;
    int start_time;
    int remaining_time;
    int end_time;
    int state; //0 for stopped 1 for running
};
//priority queue 
struct node
{
    struct process Process;
    int priority;
    struct node* next;
    struct node* prev; //added for less complexity on enqueue
};
struct priority_queue
{
    struct node* head;
    int n;
    struct node* tail;
};
void enqueue(struct priority_queue* q, struct process Process, int priority) {
    struct node* new_node = (struct node*)malloc(sizeof(struct node));
    new_node->Process = Process;
    new_node->priority = priority;
    new_node->next = NULL;
    new_node->prev = NULL;

    // If the queue is empty
    if (q->n == 0) {
        q->head = new_node;
        q->tail = new_node;
    } else {
        struct node* temp = q->head;

        // Traverse to find the correct position
        while (temp != NULL && temp->priority <= priority) {
            temp = temp->next;
        }

        // Inserting at the beginning
        if (temp == q->head) {
            new_node->next = q->head;
            q->head->prev = new_node;
            q->head = new_node;
        }
        // Inserting at the end
        else if (temp == NULL) {
            q->tail->next = new_node;
            new_node->prev = q->tail;
            q->tail = new_node;
        }
        // Inserting in the middle
        else {
            new_node->next = temp;
            new_node->prev = temp->prev;
            temp->prev->next = new_node;
            temp->prev = new_node;
        }
    }
    q->n++;
}
struct process dequeue(struct priority_queue* q)
{
    struct process Process;
    Process.id=-1;
    if(q->n==0)
    {
        return Process;
    }
    Process=q->head->Process;
    struct node* temp=q->head;
    q->head=q->head->next;
    if (q->head != NULL) {
        q->head->prev = NULL;
    }
    free(temp);
    q->n--;
    if(q->n==0)
    {
        q->tail=NULL;
    }
    return Process;
}
struct process peek(struct priority_queue q)
{
    struct process Process;
    Process.id=-1;
    if(q.n==0)
    {
        return Process;
    }
    Process=q.head->Process;
    return Process;
}
void print_queue(struct priority_queue q)
{
    struct node* temp=q.head;
    while(temp!=NULL)
    {
        printf("process id %d->",temp->Process.id);
        temp=temp->next;
    }
    if(q.n!=0)
    printf("\n");
}
//end of priority queue
struct msgbuff_gen
{
    long mtype;
    struct process_recieved proc;
};
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
struct process currently_running;
    //vars for statistics 
    FILE*scheduler_log,*scheduler_perf;
    int total_waiting_time=0;
    int finished_process=0;
    int total_running_time=0;
    int* TA;
    float* WTA;


    //end of statistics vars
//end of global vars
void process_end(int signum)
{
    printf("process notified scheduler of finish \n");
}
void run_p(struct process Process);
void stop_p(struct process Process);
void removeProcess()
{
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

int main(int argc, char * argv[])
{
    //queue and currenlty_running initializations
    q.head=NULL;
    q.tail=NULL;
    q.n=0;
    currently_running.id=-1;
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
            int pid=fork();
            if(pid==0)
            {
                //printf("process %d started at time %d\n",Process.id,getClk());
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
            enqueue(&q,Process,priority);
        }
       // print_queue(q);
        
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
        if(Process_recieved.id==-2 && q.n==0 && currently_running.id==-1)
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

