//TCSS 422 Assignment 4

//Multi-threaded Operating Systems Simulator
//Matt Seto & Arsh Singh

#include "A4wrong.h"

//Queues
Queue ReadyQueue;
Queue IO_KeyQueue;
Queue IO_ScrQueue;
Queue IO_MdmQueue;
int MsgStatus;

int InterruptArray[NUM_INTERRUPTS];

//global variables that will be incremented by IO functions
int IO_Key;
int IO_Scr;
int IO_Mdm;
int Proc_ID;
int Deadlock;
int totalCycles;

//the pthread lock for the MsgStatus
pthread_mutex_t MsgStatusLock;
//mutex locks for IO integers
pthread_mutex_t IO_KeyLock;
pthread_mutex_t IO_ScrLock;
pthread_mutex_t IO_MdmLock;
//mutex for ReadyQueue
pthread_mutex_t ReadyQLock;

//Thread Loops
void *mainThread(){
	while(1){
		while(ReadyQueue.size > 0){
			pthread_mutex_lock(&ReadyQLock);
			PCBNode* curNode = dequeueAndCheckTermination(&ReadyQueue);
			pthread_mutex_unlock(&ReadyQLock);
			//if it returned a null node, it was terminated, start again
			if (curNode == NULL){
				continue;
			}
			totalCycles++;
			int interruptSentinel = 0;
			//check the current PCB to see if its count will fire an interrupt
			for(int i = 0; i < NUM_INTERRUPTS; i++){
				while(curNode->count < InterruptArray[i]){
					if(curNode->count == InterruptArray[i]){
						//reset the array value so it never gets used again
						InterruptArray[i] = 0;
						interruptSentinel = 1;
						//decide which thread interrupts it
						int r = (rand() % 3);
						switch (r) {
							case 0:
								pthread_create(&thread[2], NULL, IO_KeyThread, NULL);
								enqueue(curNode, &IO_KeyQueue);
								break;
							case 1: 
								pthread_create(&thread[3], NULL, IO_ScrThread, NULL);
								enqueue(curNode, &IO_ScrQueue); 
								break;
							case 2: 
								pthread_create(&thread[4], NULL, IO_MdmThread, NULL);
								enqueue(curNode, &IO_ScrQueue);
								break;
						}
						break;
					}
				}
			}	
			//if it didn't get interrupted, then it must still be running, let it go until the timer fires
			if(interruptSentinel != 1){
				while(!MsgStatus){
					//stay in a holding loop until the MsgStatus gets set to something
				}	
			}
			while(1){
				pthread_mutex_lock(&MsgStatusLock);
				int Msg = MsgStatus;
				MsgStatus = 0x00;
				pthread_mutex_unlock(&MsgStatusLock);
				//decode the MessageStatus word and determine what the interrupt was
				if ((Msg & 0x02) == 0x02){
					//two types of Process, even numbers are producers, odd are consumers
					if(curNode->id % 2 == 0){
						pthread_mutex_lock(&IO_KeyLock);
						IO_Key++;
						if(curNode != NULL){
							enqueue(curNode, &IO_KeyQueue);
							curNode = NULL;
						}
						pthread_mutex_unlock(&IO_KeyLock);
						break;
					}
					Msg &= 0xFD;
				} 
				if ((Msg & 0x04) == 0x04){
					if(curNode->id % 2 == 0){
						pthread_mutex_lock(&IO_ScrLock);
						IO_Scr++;
						if(curNode != NULL){
							enqueue(curNode, &IO_ScrQueue);
							curNode = NULL;
						}
						pthread_mutex_unlock(&IO_ScrLock);
						break;
					}
					Msg &= 0xFC;
				} 
				if((Msg & 0x08) == 0x08){
					if(curNode->id % 2 == 0){
						pthread_mutex_lock(&IO_MdmLock);
						IO_Mdm++;
						if(curNode != NULL){
							enqueue(curNode, &IO_MdmQueue);
							curNode = NULL;
						}
						pthread_mutex_unlock(&IO_MdmLock);
						break;
					}
					Msg &= 0xFB;
				} 
				if((Msg & 0x01) == 0x01){
					//printf("TIMER Interrupt\n");
					if(curNode != NULL){
						pthread_mutex_lock(&ReadyQLock);
						enqueue(curNode, &ReadyQueue);
						pthread_mutex_unlock(&ReadyQLock);
					}
					Msg &= 0xFE;
					break;
				}
			}
			continue;
		}
		printf("\n");
		printf("*****Final Results:*****\nIO_Key Value:\t%d\nIO_Scr Value:\t%d\nIO_MdM Value:\t%d\n", IO_Key, IO_Scr, IO_Mdm);
		printf("--------------------\n");
		printf("Total Processes Run: %d\n", Proc_ID);
		printf("Total Cycles Run: %d\n", totalCycles);
		printf("Deadlock Occured %d times!\n", Deadlock);
		printf("\n");
		exit(1);
		pthread_exit(NULL);	
	}
}

void *timerThread(){
	while(1){
		//timer always fires on a constant interval
		pthread_mutex_lock(&MsgStatusLock);
		MsgStatus |= 0x01;
		pthread_mutex_unlock(&MsgStatusLock);
		usleep(2000);
	}
}

void *IO_KeyThread(){
	while(1){
		int r = ((rand() % 50000) + 50000);
		pthread_mutex_lock(&MsgStatusLock);
		MsgStatus |= 0x02;
		pthread_mutex_unlock(&MsgStatusLock);
		pthread_mutex_lock(&IO_KeyLock);
		//printf("IO_Key Consumer! IO_Key value: %d\n", IO_Key);
		if(IO_KeyQueue.size > 0){
			pthread_mutex_lock(&ReadyQLock);
			enqueue(dequeue(&IO_KeyQueue), &ReadyQueue);
			pthread_mutex_unlock(&ReadyQLock);
		}
		pthread_mutex_unlock(&IO_KeyLock);
		usleep(r);
	}
}

void *IO_ScrThread(){
	while(1){
		int r = ((rand() % 50000) + 50000);
		pthread_mutex_lock(&MsgStatusLock);
		MsgStatus |= 0x04;
		pthread_mutex_unlock(&MsgStatusLock);
		pthread_mutex_lock(&IO_ScrLock);
		//printf("IO_Scr Consumer! IO_Scr value: %d\n", IO_Scr);
		if(IO_ScrQueue.size > 0){
			pthread_mutex_lock(&ReadyQLock);
			enqueue(dequeue(&IO_ScrQueue), &ReadyQueue);
			pthread_mutex_unlock(&ReadyQLock);
		}
		pthread_mutex_unlock(&IO_ScrLock);
		usleep(r);
	}
}

void *IO_MdmThread(){
	while(1){
		int r = ((rand() % 50000) + 50000);
		pthread_mutex_lock(&MsgStatusLock);
		MsgStatus |= 0x08;
		pthread_mutex_unlock(&MsgStatusLock);
		pthread_mutex_lock(&IO_MdmLock);
		//printf("IO_Mdm Consumer! IO_Mdm value: %d\n", IO_Mdm);
		if(IO_MdmQueue.size > 0){
			pthread_mutex_lock(&ReadyQLock);
			enqueue(dequeue(&IO_MdmQueue), &ReadyQueue);
			pthread_mutex_unlock(&ReadyQLock);
		}
		pthread_mutex_unlock(&IO_MdmLock);
		usleep(r);
	}
}

void *DeadlockCheck(){
	while(1){
		usleep(10000);
	}
}

//Main
int main(int argc, char * argv[]){

	int i, r, r2, IO_Key, IO_Scr, IO_Mdm, totalCycles = 0;
	Proc_ID = 0;
	srand(time(0));
	//populate the ReadyQueue with initial values
	ReadyQueue = createQueue();
    r = ((rand() % 10) + 35);
    for (i = 0; i < r; i ++){
        //random number of quanta that the process will consume
        r2 = ((rand() % 750) + 10);
        PCBNode* pcb = createPCBNode(Proc_ID, r2);
        enqueue(pcb, &ReadyQueue);
        Proc_ID++;
    }
    
    //populate the InterruptArray with values
    for(i=0; i < NUM_INTERRUPTS; i++){
    	r = ((rand() % 750) + 10);
    	InterruptArray[i] = r;
    }
    qsort(InterruptArray, NUM_INTERRUPTS, sizeof(int), compare);
    
    for(i=0; i< NUM_INTERRUPTS; i++){
    	printf("I = %d\n", InterruptArray[i]);
    }
    IO_KeyQueue = createQueue();
    IO_ScrQueue = createQueue();
    IO_MdmQueue = createQueue();
    
    MsgStatus = 0x00;
    
    //initialize the mutex
	pthread_mutex_init(&MsgStatusLock, NULL);
	pthread_mutex_init(&IO_KeyLock, NULL);
	pthread_mutex_init(&IO_ScrLock, NULL);
	pthread_mutex_init(&IO_MdmLock, NULL);
	pthread_mutex_init(&ReadyQLock, NULL);

	//create NUM_THREADS number of threads
	pthread_t thread[NUM_THREADS];
	
	pthread_create(&thread[0], NULL, mainThread, NULL);
	pthread_create(&thread[1], NULL, timerThread, NULL);
	
	pthread_join(thread[0], NULL);
	pthread_join(thread[1], NULL);
	
	pthread_exit(NULL);
	
}

