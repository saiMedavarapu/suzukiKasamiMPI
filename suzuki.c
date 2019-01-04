/*
 
 Name: Sai Medavarapu
 
 */
 



#include <mpi.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
double MPI_Wtime();
int State = 0;
//Function declaration for the critical section function
int criticalSection(int m);

void tokenSend(int numtasks, int temp1, int temp2, int tokenflag)
{
    int j,k;
    temp1 = rand()%numtasks+0;//Here we generate the random numbers
    do{temp2 = rand()%numtasks+0;}while(temp1==temp2);
    //printf("rank: %d temp1:%d\n\n", rank,temp1);
    tokenflag=0;
    j= 0;
    while(j < numtasks)
    {
        MPI_Send(&temp1,1,MPI_INT,j,1,MPI_COMM_WORLD);
        MPI_Send(&temp2,1,MPI_INT,j,2,MPI_COMM_WORLD);
        j++;
    }
    
}
void routing(int SN, int RN[], int rank, int token, int numtasks, int random_number,  int sleep_time, int State, int i, MPI_Status status)
{
    sleep(sleep_time);
    SN=rank;
    RN[SN]++;
    printf("Process with rank %d and sequence number %d is requesting critical section \nBroadcast message (%d:%d)\n \n",rank,RN[SN],rank,RN[SN]);
    
    if(rank != token)
    {
        for(i=0;i<numtasks;i++)
        {
            if(i != rank)
            {
                //printf("Iam the rank %d sending the sn value to %d \n \n",rank,i);
                MPI_Send(&SN,1,MPI_INT,i,3,MPI_COMM_WORLD);
            }
        }
        
        // printf("Recv1");
        MPI_Recv(&token,1,MPI_INT,MPI_ANY_SOURCE,4,MPI_COMM_WORLD,&status);
        printf("Rank %d has received the token from Rank %d and entering into critical section\n \n",rank,status.MPI_SOURCE);
    }
    if(rank == token)
        criticalSection(rank);// All the processor which gets the token would be entering into the criticalSection by executing this function.
    
    printf("Rank %d has exited critical section\n \n",rank);
    State=0;
}




//*** Push to queue
void push(int q[], int rank, int numtasks){
    //Check that the rank is not already in the queue
    int check=0, queue_size, i;
    queue_size = numtasks-1;
    while(i<queue_size)
    {
        if(q[i]!=rank)
        {
            i++;
        }
        else
        {
            check = 1;
            break;
        }
    }
    if(check!=1)
    {
        i=0;
        while(i<numtasks)
        {
            if(q[i]==-1)
            {
                q[i] = rank;
                break;
            }
            i++;
        }
    }
    //Use a for or while loop, do nothing if rank is in queue.
    
    //If rank not in queue, assign the first -1 value to "rank"
}

//*** Pop from queue
void pop(int q[], int rank, int numtasks){
    //Check that the rank IS in the queue
    //If rank is not found, then don't do anything else.
    int i;
    int queue_size=numtasks-1;
    for(i=0;i<queue_size;i++)
    {
        // Move the elements to the left side of the queue popping out the first element
        q[i]=q[i+1];
    }
    q[queue_size-1]=-1;
    //When rank is found, from that point on just shift everything to the left.
    //The last value set to -1
}


int main(int argc, char *argv[])
{
    int numtasks, rank, dest, source, rc, count, tag=1,i,token, critical_grant;
    int temp1=0,temp2=0;
    MPI_Init(&argc,&argv);
    struct {
        int ln[10];
        int queue[50];
    } status_queue;//This is the struct declaration
    
    
    int blocklengths[2]={10,10};
    MPI_Datatype types[2]={MPI_INT, MPI_INT};
    MPI_Aint displacements[2];
    MPI_Datatype ptype;
    MPI_Aint intex;
    MPI_Type_extent(MPI_INT, &intex);
    displacements[0] = (MPI_Aint) 0;
    displacements[1] = intex*10;
    MPI_Type_struct(2, blocklengths, displacements, types, &ptype);
    MPI_Type_commit(&ptype);
    MPI_Status Stat,status;   // required variable for receive routines
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    srand( time(NULL)+rank);
    MPI_Request request;
    int random_number=0;
    int RN[10];
    int SN=0;
    int ack = 0;
    int iflag=0;
    int flag=0;
    int flag2=0;
    int else_flag=1;
    int Initialization_Flag=0;
    int tokenflag=0;
    int recv_flag=1;
    
    //Taking inputs
    int simulation=atoi(argv[1]);
    int sleep_time=atoi(argv[2]);
    
    //Initializing LN and RN
    for(i=0;i<numtasks;i++)
    {
        status_queue.ln[i]=0;
        status_queue.queue[i]=-1;
    }
    
    for(i = 0; i < 10;i++)
        RN[i] = 0;
    // Processor with rank 0 is sending the token to a random processor.
    if (rank == 0)
    {
        //printf("Token 0 has the token. \n");
        tokenSend(numtasks, temp1,temp2,tokenflag);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    
    MPI_Recv(&critical_grant , 1 , MPI_INT , 0 , 1,MPI_COMM_WORLD,&status);
    if(rank == critical_grant)
        State = 2;
    MPI_Recv(&token , 1 , MPI_INT , 0 , 2,MPI_COMM_WORLD,&status);
    
    
    push(status_queue.queue, critical_grant, numtasks);
    
    // Here we are using the timing function to set the simulation time to limit the program, to 30 seconds
    time_t endwait;
    time_t start = time(NULL);
    endwait = start + simulation;
    
    
    //This while loop works only for 30 seconds and later exits after the 30 second simualtion limit time.
    while(start <= endwait)
    {
        
        start = time(NULL);
        
        //All the processors would be waiting for a request from the processor which needs the critical section.
        if(State == 0)
        {
            //Here the sn values are received for the processor which requested for the critical section.
            MPI_Iprobe(MPI_ANY_SOURCE,3,MPI_COMM_WORLD,&flag,&status);
            
            if(flag){
                MPI_Recv(&SN,1,MPI_INT,MPI_ANY_SOURCE,3,MPI_COMM_WORLD,&status);
                
                printf("Rank %d received critical section request from rank %d\n",rank,status.MPI_SOURCE);
                RN[SN]++;
            }
            
            if(rank == token)
            {
                int src_token = status.MPI_SOURCE;
                MPI_Send(&src_token,1,MPI_INT,status.MPI_SOURCE,4,MPI_COMM_WORLD);
                printf("\nRank %d is sending the token to rank %d\n",rank,status.MPI_SOURCE);
            }
            State = 2;
        }
        
        
        
        
        //All the processors which get the token would be entering into the criticalSection by using this loop.
        if(State == 2)
        {
            // routing(SN, RN, rank, token, numtasks, random_number, sleep_time, State, i, status);
            sleep(sleep_time);
            SN=rank;
            RN[SN]++;
            printf("Process with rank %d and sequence number %d is requesting critical section \nBroadcast message (%d:%d)\n \n",rank,RN[SN],rank,RN[SN]);
            
            if(rank != token)
            {
                for(i=0;i<numtasks;i++)
                {
                    if(i != rank)
                    {
                        //printf("Iam the rank %d sending the sn value to %d \n \n",rank,i);
                        MPI_Send(&SN,1,MPI_INT,i,3,MPI_COMM_WORLD); //*** May not need this?? I'm not sure.
                    }
                }
                
                // printf("Recv1");
                MPI_Recv(&token,1,MPI_INT,MPI_ANY_SOURCE,4,MPI_COMM_WORLD,&status);                 printf("Rank %d has received the token from Rank %d and entering into critical section\n \n",rank,status.MPI_SOURCE);
            }
            if(rank == token)
                criticalSection(rank);// All the processor which gets the token would be entering into the criticalSection by executing this function.
            
            printf("Rank %d has exited critical section\n \n",rank);
            
            
            State=0;
        }
    }
    
    MPI_Finalize();
    return 0;
}

int criticalSection(int m)
{
    printf("Hello! I'm the process :%d inside the critical section\n\n", m);
    sleep(2);
    return 0;
}
