//A4main.c

#include "A4.h"

//global variables for Queue
Queue ReadyQueue;
int MsgStatus;


//global variables that will be incremented by IO functions
int IO_Key;
int IO_Scr;
int IO_Mdm;

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
			printf("Dequeued Node %d, Quanta %d of %d\n", curNode->id, curNode->count, curNode->quanta);
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
					printf("IO_KEY Interrupt\n");
					Msg &= 0xFD;
				} 
				if ((Msg & 0x04) == 0x04){
					printf("IO_SCR Interrupt\n");
					Msg &= 0xFC;
				} 
				if((Msg & 0x08) == 0x08){
					Msg &= 0xFB;
					printf("IO_MDM Interrupt\n");
				} 
				if((Msg & 0x01) == 0x01){
					printf("TIMER Interrupt\n");
					enqueue(curNode, &ReadyQueue);
					Msg &= 0xFE;
					break;
				}
			}
			continue;
		}
	}
}

void *timerThread(){
	while(1){
		//int r = ((rand() % 50000) + 5000);
		pthread_mutex_lock(&MsgStatusLock);
		MsgStatus |= 0x01;
		pthread_mutex_unlock(&MsgStatusLock);
		usleep(5000);
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


//Main
int main(int argc, char * argv[]){
	
	int i, r, r2, IO_Key, IO_Scr, IO_Mdm = 0;
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
	pthread_create(&thread[2], NULL, IO_KeyThread, NULL);
	pthread_create(&thread[3], NULL, IO_ScrThread, NULL);
	pthread_create(&thread[4], NULL, IO_MdmThread, NULL);
	
	pthread_join(thread[0], NULL);
	pthread_join(thread[1], NULL);
	pthread_join(thread[2], NULL);
	pthread_join(thread[3], NULL);
	pthread_join(thread[4], NULL);
	
	pthread_exit(NULL);
	
}

