//A4main.c

#include "A4.h"



//Loop functions:
void *mainThread(void* AQ){
	AllQueues* AllQ = (AllQueues*)AQ;
	printf("%d", AllQ->MessageQueue->size);
    while(1){
    	int i;
    	printf("AllQ size from main thread %d\n", AllQ->ReadyQueue->size);
    	int k;
    	scanf("%d", &k);
    	if(AllQ->ReadyQueue->size > 0){
    	printf("Hello from main!");
			PCBNode* curNode = dequeueAndCheckTermination(AllQ->ReadyQueue);
			//printf("New Node:      ");
			if(curNode != NULL){
				usleep(10000);
				enqueue(curNode, AllQ->ReadyQueue);
			}
		}
    }
}

void *timerThread(){
	while(1){
		printf("******\n");
		fflush(stdout);
		usleep(50000);
	}
}

int main(int argc, char * argv[]){

	//create the allQueues struct which contains:
	/*
	* ReadyQueue
	* Message Queue
	* IO 1, 2, 3
	*/
	AllQueues* allQ = createAllQueues();
	printf("AQ RQ size = %d", allQ->ReadyQueue->size);
	int k;
	scanf("%d", &k);
	
	//create NUM_THREADS number of threads
	pthread_t thread[NUM_THREADS];
	
	pthread_create(&thread[0], NULL, mainThread, ((void*)allQ));
	pthread_create(&thread[1], NULL, timerThread, NULL);
	
	pthread_join(thread[0], NULL);
	pthread_join(thread[1], NULL);
	
	pthread_exit(NULL);
	
}