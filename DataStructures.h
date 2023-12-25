#include"headers.h"
struct process_recieved
{
    int arrivaltime;
    int priority;
    int runningtime;
    int id;
    int memsize;
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
    int memsize;
};

struct msgbuff_gen
{
    long mtype;
    struct process_recieved proc;
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
//memory tree for buddy system
struct mem_node
{
    int size;
    int state; //0 for free 1 for allocated
    int process_id;
    struct mem_node* left;
    struct mem_node* right;
    int start;
    int end;
    bool is_free;
    struct mem_node* parent;
};
//this function creates a node in the tree
struct mem_node* create_node(int size,int state,int start,int end,struct mem_node* parent)
{
    struct mem_node* node=(struct mem_node*)malloc(sizeof(struct mem_node));
    node->size=size;
    node->state=state;
    node->left=NULL;
    node->right=NULL;
    node->process_id=-1;
    node->start=start;
    node->end=end;
    node->is_free=true;
    node->parent=parent;
    return node;
}
struct mem_node* getOptimum(struct mem_node* node,int size)
{
    if(node->left&&node->right)
    {
        //try left and right
        struct mem_node* temp=getOptimum(node->left,size);
        struct mem_node* temp2=getOptimum(node->right,size);
        if(temp==NULL&&temp2==NULL)
        {
            return NULL;
        }
        if(temp==NULL)
        {
            return temp2;
        }
        if(temp2==NULL)
        {
            return temp;
        }
        if(temp->size<=temp2->size)
        {
            return temp;
        }
        else
        {
            return temp2;
        }
        
    }
    else
    {
        if(node->size>=size&&node->state==0)
        {
            return node;
        }
        else
        {
            return NULL;
        }
    }
}
struct mem_node* insert_process(struct mem_node* node,int size,int process_id)
{
    struct mem_node* temp=getOptimum(node,size);
    if(temp==NULL)
    {
        return NULL;
    }
    while(temp->size/2>=size)
    {
        temp->left=create_node(temp->size/2,0,temp->start,temp->start+temp->size/2-1,temp);
        temp->right=create_node(temp->size/2,0,temp->start+temp->size/2,temp->end,temp);
        temp=temp->left;
    }
    temp->state=1;
    temp->process_id=process_id;
    temp->is_free=false;
    struct mem_node* parent=temp->parent;
    while(parent!=NULL)
    {
        parent->is_free=false;
        parent=parent->parent;
    }
    return temp;
}
//function checks if a node is free or not 
void remove_mem(struct mem_node* node,int process_id,FILE* mem)
{
    if(node==NULL)
    {
        return;
    }
    if(node->process_id==process_id)
    {
        node->state=0;
        node->process_id=-1;
        node->is_free=true;
        fprintf(mem,"from %d to %d\n",node->start,node->end);
        printf("Removed process from %d to %d\n",node->start,node->end);
        return; 
    }
    remove_mem(node->left,process_id,mem);
    remove_mem(node->right,process_id,mem);
    if(node->left) //check if both children node is now free to combine
    if(node->left->is_free && node->right->is_free)
    {
        node->state=0;
        node->process_id=-1;
        node->is_free=true; //both children are free now so is the parent
        free(node->left);
        free(node->right);
        node->left=NULL;
        node->right=NULL;
        return;
    }
}