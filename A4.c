//A4main.c

#include "A4.h"

//global variables for Queues
Queue ReadyQueue;
int MsgStatus;

//the pthread lock for the MsgStatus
pthread_mutex_t MsgStatusLock;


//Loop functions:
void *mainThread(){
	while(1){
		//printf("In Main\n");
		while(ReadyQueue.size > 0){
			PCBNode* curNode = dequeueAndCheckTermination(&ReadyQueue);
			//if it returned a null node, it was terminated, start again
			if (curNode == NULL){
				continue;
			}
			printf("Dequeued Node %d, Quanta %d of %d\n", curNode->id, curNode->count, curNode->quanta);
			//lock the status word, check the current interrupts
			while(!MsgStatus){
				//stay in a holding loop until the MsgStatus gets set to something
			}	
			//pthread_mutex_unlock(&MsgStatusLock);
			printf("There was a TIMER interrupt!");
			pthread_mutex_lock(&MsgStatusLock);
			MsgStatus = 0x00;
			pthread_mutex_unlock(&MsgStatusLock);
			if(curNode != NULL){
				enqueue(curNode, &ReadyQueue);
			}
			continue;
		}
		//usleep(10000);
	}
}

void *timerThread(){
	while(1){
		//printf("In TIMER\n");
		pthread_mutex_lock(&MsgStatusLock);
		//printf("Timer fire!\n");
		MsgStatus = 0x01;
		pthread_mutex_unlock(&MsgStatusLock);
		usleep(5000);
	}
}

int main(int argc, char * argv[]){
	
	int i, r, r2;
	int Proc_ID = 0;
	
	//populate the ReadyQueue with initial values
	ReadyQueue = createQueue();
    r = ((rand() % 10) + 25);
    for (i = 0; i < r; i ++){
        //random number of quanta that the process will consume
        r2 = ((rand() % 50) + 10);
        PCBNode* pcb = createPCBNode(Proc_ID, r2);
        enqueue(pcb, &ReadyQueue);
        Proc_ID++;
    }
    
    MsgStatus = 0x00;
    
    //initialize the mutex
	pthread_mutex_init(&MsgStatusLock, NULL);

	//create NUM_THREADS number of threads
	pthread_t thread[NUM_THREADS];
	
	pthread_create(&thread[0], NULL, mainThread, NULL);
	pthread_create(&thread[1], NULL, timerThread, NULL);
	
	pthread_join(thread[0], NULL);
	pthread_join(thread[1], NULL);
	
	pthread_exit(NULL);
	
}

