//TCSS 422 Assignment 4

//Multi-threaded Operating Systems Simulator
//Matt Seto & Arsh Singh

/*

How it works:

 - The ReadyQueue and IO Queues are initialized
 - The Main thread begins by dequeing the head of the Ready Queue
 - The Current Node-> count is compared to a preset list of array values, if a match is found it simulates an interrupt
 - IF THERE IS INTERRUPT
 	 - If there is an interrupt, a global IOStatus variable is bitwise set to flag the appropriate IO Thread that something is ready
 	 - The node is placed in an IO Waiting Queue until the IO Thread completes a random time interval
 	 - The Main loop restarts
 - IF THERE IS NO INTERRUPT
 	- The main queue waits for a semaphore flag from one or more of the IO Threads (polling the flag)
 	- Once the semaphore flag is set, it is retrieved, reset and decoded
 	- The Apropriate action is taken depending on which thread set the flag
 	- If timer is set, the currentNode is put back in the RQ
 	- If any of the IO flags are set, those nodes are put back in the RQ
 	- The Main loop restarts
 	
Notes: 
 - All Globals are encased in mutual exlusion locks to prevent race condition
 - IO Semaphore flag Key:
 	INDICATES THERE IS SOMETHING READY TO GO TO IO QUEUES
 	------------------------
 	NOMSG 	0000 0000 | 0x00
 	IO_KEY	0000 0001 | 0x01
 	IO_SCR 	0000 0010 | 0x02
 	IO_MDM 	0000 0100 | 0x04
 	
 	INDICATES THAT THE IO SERVICE HAS COMPLETED AND SOMETHING IS READY TO COME BACK TO RQ
 	------------------------
 	NOMSG 	0000 0000 | 0x00
 	IO_KEY	0001 0000 | 0x10
 	IO_SCR 	0010 0000 | 0x20
 	IO_MDM 	0100 0000 | 0x40

	Note that this format allows us to mask only one bit at a time. THEREFORE YOU CAN HAVE MULTIPLE FLAGS SET AT ONCE
	Ex: 0x63 (0110 0011) indicates tht tehre are SCR and MDM inturrupts ready to 
		return to the RQ. AND that SCR and KEY are ready to go to IO Waiting
	
 - There is a deadlock detector that tests all Global int mutexes at a specified interval
 

*/


#include "A4.h"

//Queues
Queue ReadyQueue;
Queue IO_KeyQueue;
Queue IO_ScrQueue;
Queue IO_MdmQueue;
int StatusSemaphore;

int InterruptArray[NUM_INTERRUPTS];

//global variables that will be incremented by IO functions
int IO_1;
int IO_2;
int IO_3;
int Proc_ID;
int Deadlock;
int totalCycles;

//the pthread lock for the StatusSemaphore
pthread_mutex_t StatusSemaphoreLock;
//the pthread lock for IOSemaphore
pthread_mutex_t IOSemaphoreLock;
//mutex locks for IO integers
pthread_mutex_t IO_1Lock;
pthread_mutex_t IO_2Lock;
pthread_mutex_t IO_3Lock;
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
				if(curNode->count == InterruptArray[i]){
					//reset the array value so it never gets used again
					InterruptArray[i] = 0;
					interruptSentinel = 1;
					//decide which thread interrupts it
					int r = (rand() % 3);
					pthread_mutex_lock(&StatusSemaphoreLock);
					switch (r) {
						case 0:
							//printf("Node %d Interrupted by IO_KEY at %d\n", curNode->id, curNode->count);
							StatusSemaphore |= 0x01;
							enqueue(curNode, &IO_KeyQueue);
							break;
						case 1: 
							//printf("Node %d Interrupted by IO_SCR at %d\n", curNode->id, curNode->count);
							StatusSemaphore|= 0x02;
							enqueue(curNode, &IO_ScrQueue); 
							break;
						case 2: 
							//printf("Node %d Interrupted by IO_MDM at %d\n", curNode->id, curNode->count);
							StatusSemaphore |= 0x04;
							enqueue(curNode, &IO_MdmQueue);
							break;
					}
					pthread_mutex_unlock(&StatusSemaphoreLock);
					if(interruptSentinel == 1){
						break;
					}
				}
			}
			//if it was interrupted, dequeue the next PCB and do it again, otherwise, give it CPU time
			if(interruptSentinel == 1){
				continue;
			} else {
				while(1){
				pthread_mutex_lock(&StatusSemaphoreLock);
				int Msg = StatusSemaphore;
				StatusSemaphore &= 0x0F;
				pthread_mutex_unlock(&StatusSemaphoreLock);
				//decode the MessageStatus word and determine what the interrupt was
				if ((Msg & 0x10) == 0x10){
					//printf("IO_KEY Request Completed\n");
					enqueue(dequeue(&IO_KeyQueue), &ReadyQueue);
					Msg &= 0xEF;
				} 
				if ((Msg & 0x20) == 0x20){
					//printf("IO_SCR Request Completed\n");
					enqueue(dequeue(&IO_ScrQueue), &ReadyQueue);
					Msg &= 0xDF;
				} 	
				if((Msg & 0x40) == 0x40){
					//printf("IO_MDM Request Completed\n");
					enqueue(dequeue(&IO_MdmQueue), &ReadyQueue);
					Msg &= 0xBF;
				} 
				if((Msg & 0x88) == 0x88){
					if(curNode != NULL){
						pthread_mutex_lock(&ReadyQLock);
						enqueue(curNode, &ReadyQueue);
						pthread_mutex_unlock(&ReadyQLock);
					}
					Msg &= 0x77;
					break;
				}
			}			
			}
		}
		printf("\n");
		printf("*****Final Results:*****\nIO_Key Value:\t%d\nIO_Scr Value:\t%d\nIO_MdM Value:\t%d\n", IO_1, IO_2, IO_3);
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
		pthread_mutex_lock(&StatusSemaphoreLock);
		StatusSemaphore |= 0x88;
		pthread_mutex_unlock(&StatusSemaphoreLock);
		usleep(2000);
	}
}

//IO Threads will poll the StatusSemaphore
//When the IO Thread finds its semaphore flag, it begins a random length interval
//at the end of that interval, the IO Thread sets the return flag 
//to indicate that the IO request is completed and the PCB is allowed back in the RQ
void *IO_KeyThread(){
	while(1){
		usleep(10000);
		pthread_mutex_lock(&StatusSemaphoreLock);
		int Msg = StatusSemaphore;
		StatusSemaphore &= 0xFE;
		pthread_mutex_unlock(&StatusSemaphoreLock);
		if(((Msg & 0x01) == 0x01) && IO_KeyQueue.size > 0){
			int r = ((rand() % 100000) + 500000);
			usleep(r);
			//Share mutual Resources
			pthread_mutex_lock(&IO_1Lock);
			pthread_mutex_lock(&IO_2Lock);
			IO_1++;
			IO_2++;
			//printf("IO_KEY RESOURCE SHARING: %d + %d = %d\n", IO_1, IO_2, IO_1+IO_2);
			pthread_mutex_unlock(&IO_2Lock);
			pthread_mutex_unlock(&IO_1Lock);
			pthread_mutex_lock(&StatusSemaphoreLock);
			StatusSemaphore |= 0x10;
			pthread_mutex_unlock(&StatusSemaphoreLock);
		}
	}
}

void *IO_ScrThread(){
	while(1){
		usleep(10000);
		pthread_mutex_lock(&StatusSemaphoreLock);
		int Msg = StatusSemaphore;
		StatusSemaphore &= 0xFD;
		pthread_mutex_unlock(&StatusSemaphoreLock);
		if(((Msg & 0x02) == 0x02) && IO_ScrQueue.size>0){
			int r = ((rand() % 100000) + 500000);
			usleep(r);
			//Share mutual Resources
			pthread_mutex_lock(&IO_2Lock);
			pthread_mutex_lock(&IO_3Lock);
			IO_2++;
			IO_3++;
			//printf("IO_SCR RESOURCE SHARING: %d + %d = %d\n", IO_2, IO_3, IO_2+IO_3);
			pthread_mutex_unlock(&IO_3Lock);
			pthread_mutex_unlock(&IO_2Lock);
			pthread_mutex_lock(&StatusSemaphoreLock);
			StatusSemaphore |= 0x20;
			pthread_mutex_unlock(&StatusSemaphoreLock);
		}
	}
}

void *IO_MdmThread(){
	while(1){
		usleep(10000);
		pthread_mutex_lock(&StatusSemaphoreLock);
		int Msg = StatusSemaphore;
		StatusSemaphore &= 0xFB;
		pthread_mutex_unlock(&StatusSemaphoreLock);
		if(((Msg & 0x04) == 0x04) && IO_MdmQueue.size > 0){
			int r = ((rand() % 100000) + 500000);
			usleep(r);
			//Share mutual Resources
			pthread_mutex_lock(&IO_3Lock);
			pthread_mutex_lock(&IO_1Lock);
			IO_1++;
			IO_3++;
			//printf("IO_MDM RESOURCE SHARING: %d + %d = %d\n", IO_1, IO_3, IO_1+IO_3);
			pthread_mutex_unlock(&IO_1Lock);
			pthread_mutex_unlock(&IO_3Lock);
			pthread_mutex_lock(&StatusSemaphoreLock);
			StatusSemaphore |= 0x40;
			pthread_mutex_unlock(&StatusSemaphoreLock);
		}
	}
}

//on a consistent schedule, run the deadlock checker and determine if there is any 
//deadlock on the global variables
void *DeadlockCheck(){
	while(1){
		usleep(1000000);
		if (pthread_mutex_trylock(&IO_1Lock) == 0){
			if (pthread_mutex_trylock(&IO_2Lock) == 0){
				if (pthread_mutex_trylock(&IO_3Lock) == 0){
					//printf("ALL MUTEX LOCKS AVAILABLE\n");
					pthread_mutex_unlock(&IO_3Lock);
				} else {
					printf("Mutex 3 was NOT available\n");
					Deadlock++;
				}
				pthread_mutex_unlock(&IO_2Lock);
			} else {
				printf("Mutex 2 was NOT available\n");
				Deadlock++;
			}
			pthread_mutex_unlock(&IO_1Lock);
		} else {
			printf("Mutex 1 was NOT available\n");
			Deadlock++;
		}
	}
}

//Main
int main(int argc, char * argv[]){

	int i, r, r2, IO_1, IO_2, IO_3, totalCycles = 0;
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
    
    IO_KeyQueue = createQueue();
    IO_ScrQueue = createQueue();
    IO_MdmQueue = createQueue();
    
    StatusSemaphore = 0x00;
    
    //initialize the mutex
	pthread_mutex_init(&StatusSemaphoreLock, NULL);
	pthread_mutex_init(&IOSemaphoreLock, NULL);
	pthread_mutex_init(&IO_1Lock, NULL);
	pthread_mutex_init(&IO_2Lock, NULL);
	pthread_mutex_init(&IO_3Lock, NULL);
	pthread_mutex_init(&ReadyQLock, NULL);

	//create NUM_THREADS number of threads
	pthread_t thread[NUM_THREADS];
	
	pthread_create(&thread[0], NULL, mainThread, NULL);
	pthread_create(&thread[1], NULL, timerThread, NULL);
	pthread_create(&thread[2], NULL, IO_KeyThread, NULL);
	pthread_create(&thread[3], NULL, IO_ScrThread, NULL);
	pthread_create(&thread[4], NULL, IO_MdmThread, NULL);
	pthread_create(&thread[5], NULL, DeadlockCheck, NULL);
	
	pthread_join(thread[0], NULL);
	pthread_join(thread[1], NULL);
	pthread_join(thread[2], NULL);
	pthread_join(thread[3], NULL);
	pthread_join(thread[4], NULL);
	pthread_join(thread[5], NULL);
	
	pthread_exit(NULL);
	
}

