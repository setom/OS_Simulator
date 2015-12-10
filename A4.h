#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

//5 threads
// Main, Timer, IO_Key, IO_Scr, IO_Mdm
#define NUM_THREADS 5

#define	NODE_ARRAY_SIZE 4

typedef enum {newPCB, running, waiting, interrupted, halted} State;

//compare function for qsort (Help from Stack Overflow: c-array-sorting-tips)
int compare ( const void* a, const void* b) {
    int int_a = * ((int*)a);
    int int_b = * ((int*)b);
    
    if (int_a == int_b){
        return 0;
    } else if (int_a < int_b) {
        return -1;
    } else {
        return 1;
    }
}

//typedef PCBNode
typedef struct PCBNode {
    int id;
    int quanta;
    int count;
    State state;
    //I/O interrupt array values
    int IO_Printer[NODE_ARRAY_SIZE];
    int IO_Keyboard[NODE_ARRAY_SIZE];
    int IO_Disk[NODE_ARRAY_SIZE];
    int IO_Modem[NODE_ARRAY_SIZE];;
    //Kernel service array values
    int M1[NODE_ARRAY_SIZE];
    int M2[NODE_ARRAY_SIZE];
    int M3[NODE_ARRAY_SIZE];
    int M4[NODE_ARRAY_SIZE];
    struct PCBNode* next;
} PCBNode;

//create a new PCBNode
struct PCBNode* createPCBNode(int theId, int theQuanta) {
    PCBNode* pcb = (PCBNode*) malloc(sizeof(PCBNode));
    pcb->id = theId;
    pcb->quanta = theQuanta;
    pcb->count = 0;
    pcb->state = newPCB;
    //fill the IO and Kernel Arrays with random numbers between 10 - quanta
    //Try to avoid super small numbers because they will fire too many interrupts for the simulation
    int k;
    for (k = 0; k < NODE_ARRAY_SIZE; k++){
        pcb->IO_Printer[k] = (rand() % theQuanta) + 1;
        pcb->IO_Keyboard[k] = (rand() % theQuanta) + 1;
        pcb->IO_Disk[k] = (rand() % theQuanta) + 1;
        pcb->IO_Modem[k] = (rand() % theQuanta) + 1;
        
        pcb->M1[k] = (rand() % theQuanta) + 1;
        pcb->M2[k] = (rand() % theQuanta) + 1;
        pcb->M3[k] = (rand() % theQuanta) + 1;
        pcb->M4[k] = (rand() % theQuanta) + 1;
    }
    pcb->next = NULL;
    
    // 	sort the arrays so that they are in ascending order
    qsort(pcb->IO_Printer, 4, sizeof(int), compare);
    qsort(pcb->IO_Keyboard, 4, sizeof(int), compare);
    qsort(pcb->IO_Disk, 4, sizeof(int), compare);
    qsort(pcb->IO_Modem, 4, sizeof(int), compare);
    
    qsort(pcb->M1, 4, sizeof(int), compare);
    qsort(pcb->M2, 4, sizeof(int), compare);
    qsort(pcb->M3, 4, sizeof(int), compare);
    qsort(pcb->M4, 4, sizeof(int), compare);
    
    
    return pcb;
}

//destroy a PCBNode
void destroyPCBNode(struct PCBNode* pcbNode){
    printf("Destroying PCB %d\n", pcbNode->id);
    free(pcbNode);
}

//Typedef of the Queue
typedef struct Queue {
    struct PCBNode* head;
    struct PCBNode* tail;
    int size; 
    int mutex;   
} Queue;

//create a queue
struct Queue createQueue(){
    Queue queue;
    queue.mutex = 0;
    queue.size = 0;
    queue.head = NULL;
    queue.tail = NULL;
    return queue;
}

//method definitions

//enqueue takes a queue and some priority and id values
//generates a new pcbnode from those int values and adds it
//to the queue
void enqueue (PCBNode* pcb, Queue* queue){
    
    //add it to the queue
    //if the queue doesn't exist, it is the new head
    //else, it is the next of the current tail
    if (queue->head == NULL){
        queue->head = pcb;
    } else {
        queue->tail->next = pcb;
    }
    
    //point the tail at the new node
    queue->tail = pcb;
    //increment the size of the queue
    queue->size++;
    
}

//dequeue takes a queue and dequeues the head of the queue
struct PCBNode* dequeue (Queue* queue){
    
    if(queue->size == 0){
        return NULL;
    }
    //get the head of the queue
    PCBNode* ret = queue->head;
    
    //set the new head and decrement size
    queue->head = queue->head->next;
    ret->next = NULL;
    queue->size--;
    
    //return the head
    return ret;
}

//Dequeue the first item in the Ready Queue and run it
//increment the count
//compare the total quanta
struct PCBNode* dequeueAndCheckTermination(Queue *queue){
	PCBNode* node = dequeue(queue);
	node->state = running;
	node->count++;
	if(node->count == node->quanta){
		node->state = halted;
		printf("Process %d TERMINATED, Process completed %d quanta of %d total quanta\n", node->id, node->count, node->quanta);
		destroyPCBNode(node);
		return(PCBNode*)0;
	}
	return node;
}

//Typedef of an AllQueues Struct
typedef struct AllQueues {
	Queue* ReadyQueue;
	Queue* MessageQueue;
	Queue* IO_1Queue;
    Queue* IO_2Queue;
	Queue* IO_3Queue;
} AllQueues;

//create an AllQueues
AllQueues* createAllQueues(){

	//counter for processes
	int Proc_ID = 1;
	//counter for loops
	int i, k;
	int s = 0;
	//int for random number generation
	//srand(time(0));
	int r;
	int r2;

	//Create a Ready Queue and populate it with apx 30 processes
    Queue ReadyQueue = createQueue();
    r = ((rand() % 10) + 25);
    for (i = 0; i < r; i ++){
        //random number of quanta that the process will consume
        r2 = ((rand() % 50) + 10);
        PCBNode* pcb = createPCBNode(Proc_ID, r2);
        enqueue(pcb, &ReadyQueue);
        Proc_ID++;
    }
    
    //create an Empty message queue
    Queue MessageQueue = createQueue();
    
    //create 3 empty IOQueues
    Queue IO1Queue = createQueue();
    Queue IO2Queue = createQueue();
    Queue IO3Queue = createQueue();
	
	PCBNode* pcb = (PCBNode*) malloc(sizeof(PCBNode));
	AllQueues* allQ = (AllQueues*) malloc(sizeof(AllQueues));
	allQ->ReadyQueue = &ReadyQueue;
	allQ->MessageQueue = &MessageQueue;
	allQ->IO_1Queue = &IO1Queue;
	allQ->IO_2Queue = &IO2Queue;
	allQ->IO_3Queue = &IO3Queue;
	printf("ReadyQueue Size = %d", allQ->ReadyQueue->size);
	scanf("%d", &r2);
	return allQ;
}

