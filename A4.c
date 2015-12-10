//A4main.c

#include "A4.h"

//global variables for Queue
Queue ReadyQueue;
int MsgStatus;

//global variables that will be incremented by IO functions
int IO_Key;
int IO_Scr;
int IO_Mdm;
int Proc_ID;
int Deadlock;

//the pthread lock for the MsgStatus
pthread_mutex_t MsgStatusLock;
//mutex locks for IO integers
pthread_mutex_t IO_KeyLock;
pthread_mutex_t IO_ScrLock;
pthread_mutex_t IO_MdmLock;

//Thread Loops
void *mainThread(){
	while(1){
		while(ReadyQueue.size > 0){
			PCBNode* curNode = dequeueAndCheckTermination(&ReadyQueue);
			//if it returned a null node, it was terminated, start again
			if (curNode == NULL){
				continue;
			}
			//printf("Dequeued Node %d, Quanta %d of %d\n", curNode->id, curNode->count, curNode->quanta);
			while(!MsgStatus){
				//stay in a holding loop until the MsgStatus gets set to something
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
						pthread_mutex_unlock(&IO_KeyLock);
					} else {
						pthread_mutex_lock(&IO_KeyLock);
						printf("IO_Key Consumer! IO_Key value: %d\n", IO_Key);
						pthread_mutex_unlock(&IO_KeyLock);
					}
					Msg &= 0xFD;
				} 
				if ((Msg & 0x04) == 0x04){
					if(curNode->id % 2 == 0){
						pthread_mutex_lock(&IO_ScrLock);
						IO_Scr++;
						pthread_mutex_unlock(&IO_ScrLock);
					} else {
						pthread_mutex_lock(&IO_ScrLock);
						printf("IO_Scr Consumer! IO_Key value: %d\n", IO_Scr);
						pthread_mutex_unlock(&IO_ScrLock);
					}
					Msg &= 0xFC;
				} 
				if((Msg & 0x08) == 0x08){
					if(curNode->id % 2 == 0){
						pthread_mutex_lock(&IO_MdmLock);
						IO_Mdm++;
						pthread_mutex_unlock(&IO_MdmLock);
					} else {
						pthread_mutex_lock(&IO_MdmLock);
						printf("IO_Mdm Consumer! IO_Key value: %d\n", IO_Mdm);
						pthread_mutex_unlock(&IO_MdmLock);
					}
					Msg &= 0xFB;
				} 
				if((Msg & 0x01) == 0x01){
					//printf("TIMER Interrupt\n");
					enqueue(curNode, &ReadyQueue);
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
		usleep(r);
	}
}

void *IO_ScrThread(){
	while(1){
		int r = ((rand() % 50000) + 50000);
		pthread_mutex_lock(&MsgStatusLock);
		MsgStatus |= 0x04;
		pthread_mutex_unlock(&MsgStatusLock);
		usleep(r);
	}
}

void *IO_MdmThread(){
	while(1){
		int r = ((rand() % 50000) + 50000);
		pthread_mutex_lock(&MsgStatusLock);
		MsgStatus |= 0x08;
		pthread_mutex_unlock(&MsgStatusLock);
		usleep(r);
	}
}

void *DeadlockCheck(){
	while(1){
		Deadlock++;
		usleep(10000);
	}
}

//Main
int main(int argc, char * argv[]){

	int i, r, r2, IO_Key, IO_Scr, IO_Mdm = 0;
	Proc_ID = 0;
	srand(time(0));
	//populate the ReadyQueue with initial values
	ReadyQueue = createQueue();
    r = ((rand() % 10) + 25);
    for (i = 0; i < r; i ++){
        //random number of quanta that the process will consume
        r2 = ((rand() % 50) + 100);
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

