A multi threaded Operating System simulator. 

The program simulates a number of processes being created and then sharing CPU time. Each process will run for a specified 
number of CPU cycles before it is completed. 

At some intervals, processes will require IO requests. At this point the processes are moved to IO queues
Each IO Queue is an independent thread that takes a random amount of time to complete. 
When the IO request is completed, a global semaphore is set and decoded. 
The program runs until all processes are completed. 



Brief outline of how it works: 

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