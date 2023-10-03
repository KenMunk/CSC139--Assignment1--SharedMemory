/*
CSC139 
Fall 2023
First Assignment
Munk, Kenneth
Section # 01
OSs Tested on: Ubuntu 20.04 LTS (VM), Linux (ecs-pa-coding1.ecs.csus.edu)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>

// Size of shared memory block
// Pass this to ftruncate and mmap
#define SHM_SIZE 4096

// Global pointer to the shared memory block
// This should receive the return value of mmap
// Don't change this pointer in any function
void* gShmPtr;

// You won't necessarily need all the functions below
void Producer(int, int, int);
void InitSharedMemory(int, int);
void SetBufSize(int);
void SetItemCnt(int);
void SetIn(int);
void SetOut(int);
void SetHeaderVal(int, int);
int GetBufSize();
int GetItemCnt();
int GetIn();
int GetOut();
int GetHeaderVal(int);
void WriteAtBufIndex(int, int);
int ReadAtBufIndex(int);
int GetRand(int, int);


int main(int argc, char* argv[])
{
        pid_t pid;
        int bufSize; // Bounded buffer size
        int itemCnt; // Number of items to be produced
        int randSeed; // Seed for the random number generator 

        if(argc != 4){
		printf("Invalid number of command-line arguments\n");
		exit(1);
        }
	bufSize = atoi(argv[1]);
	itemCnt = atoi(argv[2]);
	randSeed = atoi(argv[3]);
	
	// Write code to check the validity of the command-line arguments

        if(bufSize <= 1 || bufSize > 500 || itemCnt <= 0 || randSeed <= 0){
                
                printf("Invalid argument detected\n");
                if(bufSize <= 0 || bufSize > 500){
                        printf("bufSize of %s is invalid and not in range of 2 to 500\n",argv[1]); 
                }
                
                if(itemCnt <= 0){
                        printf("itemCnt of %s is invalid\n", argv[1]); 
                }

                if(randSeed <= 0){
                        printf("randSeed of %s is invalid\n", argv[3]); 
                }

                exit(1);
        }


        // Function that creates a shared memory segment and initializes its header
        InitSharedMemory(bufSize, itemCnt);        

	/* fork a child process */ 
	pid = fork();

	if (pid < 0) { /* error occurred */
		fprintf(stderr, "Fork Failed\n");
		exit(1);
	}
	else if (pid == 0) { /* child process */
		printf("Launching Consumer \n");
		execlp("./consumer","consumer",NULL);
	}
	else { /* parent process */
		/* parent will wait for the child to complete */
		printf("Starting Producer\n");
		
               // The function that actually implements the production
               Producer(bufSize, itemCnt, randSeed);
		
	       printf("Producer done and waiting for consumer\n");
	       wait(NULL);
	       printf("Consumer Completed\n");
        }
    
        return 0;
}

//init shared memory
void InitSharedMemory(int bufSize, int itemCnt)
{
        int in = 0;
        int out = 0;
        const char *name = "OS_HW1_Kenneth_Sherwood_Munk"; // Name of shared memory object to be passed to shm_open

        int sharedMemoryDescriptor = shm_open(name, O_CREAT | O_RDWR, 0666);

        ftruncate(sharedMemoryDescriptor, SHM_SIZE);

        // Write code here to create a shared memory block and map it to gShmPtr  
        // Use the above name.
        // **Extremely Important: map the shared memory block for both reading and writing 
        // Use PROT_READ | PROT_WRITE

        gShmPtr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, sharedMemoryDescriptor, 0);

        // Write code here to set the values of the four integers in the header
        // Just call the functions provided below, like 
        
        printf("Setting Buffer Size Assigned as %i\n", bufSize);
        printf("Setting Item Count Assigned as %i\n", itemCnt);
        printf("Setting In Index set as %i\n", in);
        printf("Setting Out Index set as %i\n", out);
        SetItemCnt(itemCnt);
        SetBufSize(bufSize);
        SetIn(in);
        SetOut(out);
        printf("Shared Memory initialized and headers written to\n");
        printf("Buffer Size Assigned as %i\n", GetBufSize());
        printf("Item Count Assigned as %i\n", GetItemCnt());
        printf("In Index set as %i\n", GetIn());
        printf("Out Index set as %i\n", GetOut());



	   
}

void Producer(int bufSize, int itemCnt, int randSeed)
{
    int in = 0;
    int out = 0;
    int itemsLeft = itemCnt;
        
    srand(randSeed);

    // Write code here to produce itemCnt integer values in the range specificed in the problem description
    // Use the functions provided below to get/set the values of shared variables "in" and "out"
    // Use the provided function WriteAtBufIndex() to write into the bounded buffer 	
    // Use the provided function GetRand() to generate a random number in the specified range
    // **Extremely Important: Remember to set the value of any shared variable you change locally
    // Use the following print statement to report the production of an item:
    // printf("Producing Item %d with value %d at Index %d\n", i, val, in);
    // where i is the item number, val is the item value, in is its index in the bounded buffer
    	
	
	//How we can go about this is to have a counter of how many items have been written
        //then while the next index for in is not the index of out we produce an item into the index for in
        //although this will result in a 1 entry buffer between in and out it matches
        //up with the pseudocode that was covered in the class.  I could allow for the
        //use of writing into an index when it is matching the out index but the risk there is
        //that there will be a data collision which is not preferred.  Same pattern will be used for the
        //consumer but with the next out index chasing the in index and not reading the next index until the
        //in index is 1 space away

        //for when the items have all been used we can set the in index to -1 which would allow for the
        //consumer to complete cycling through and check if the in index is -1 as an exit sequence

        //the producer will be the only process that updates the in data in the header
        //the consumer will be the only process that updates the out data in the header
        //this will allow for an asyncronous chase scenario to allow for timing
        //variations between processes since the consumer is making a few more computations
        //than the producer for the condition checks

        while(itemsLeft > 0){
                out = GetOut();
                in = GetIn();
                if((in+1)%(bufSize) != out){
                        int payload;
                        payload = GetRand(2,2500);
                        printf("Producing item %d with value %d at Index %d\n", 1+(itemCnt-itemsLeft), payload, in);
                        WriteAtBufIndex(in, payload);
                        in = (in + 1)%bufSize;
                        itemsLeft--;
                        SetIn(in);
                }
                else{
                        //printf("Waiting for data at position %i to get consumed\n");
                }
        }
        SetIn(-1);
        printf("Producer Completed\n");

}

// Set the value of shared variable "bufSize"
void SetBufSize(int val)
{
        SetHeaderVal(0, val);
}

// Set the value of shared variable "itemCnt"
void SetItemCnt(int val)
{
        SetHeaderVal(1, val);
}

// Set the value of shared variable "in"
void SetIn(int val)
{
        SetHeaderVal(2, val);
}

// Set the value of shared variable "out"
void SetOut(int val)
{
        SetHeaderVal(3, val);
}

// Get the ith value in the header
int GetHeaderVal(int i)
{
        int val;
        void* ptr = gShmPtr + i*sizeof(int);
        memcpy(&val, ptr, sizeof(int));
        return val;
}

// Set the ith value in the header
void SetHeaderVal(int i, int val)
{
       // Write the implementation
       //using memset (Nope that was a big mistake)
       //syntax void* memset(void* dest, int ch, size_t count);

       //Do what the professor did and use memcpy
       int payload = val;
       
       void* ptr = gShmPtr + i*sizeof(int);
       memcpy(ptr, &payload, sizeof(int));

}

// Get the value of shared variable "bufSize"
int GetBufSize()
{       
        return GetHeaderVal(0);
}

// Get the value of shared variable "itemCnt"
int GetItemCnt()
{
        return GetHeaderVal(1);
}

// Get the value of shared variable "in"
int GetIn()
{
        return GetHeaderVal(2);
}

// Get the value of shared variable "out"
int GetOut()
{             
        return GetHeaderVal(3);
}


// Write the given val at the given index in the bounded buffer 
void WriteAtBufIndex(int indx, int val)
{
        // Skip the four-integer header and go to the given index 
        void* ptr = gShmPtr + 4*sizeof(int) + indx*sizeof(int);
	memcpy(ptr, &val, sizeof(int));
}

// Read the val at the given index in the bounded buffer
int ReadAtBufIndex(int indx)
{
        int output;
        // Write the implementation
        void* ptr = gShmPtr + 4*sizeof(int) + indx*sizeof(int);
        memcpy(&output, ptr, sizeof(int));
        return(output);
}

// Get a random number in the range [x, y]
int GetRand(int x, int y)
{
	int r = rand();
	r = x + r % (y-x+1);
        return r;
}

