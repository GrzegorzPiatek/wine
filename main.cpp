#include <mpi.h>
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include "constants.h"


pthread_t commThread;

int wineMakerStatus = 0;

std::vector<int> requestingWineMakers;


void debug(Msg msg, const char* info){
    printf("[%d] <%d> %s tag:%d \n", myRank,myClock, info, msg.tag);
}


void tick(int newClock = 0){
    myClock = std::max(newClock, myClock) + 1;
}

void sendMsg(int destination, Tag tag, int studentRank=0){
    tick();
    Msg msg {
        .tag = tag,
        .clock = myClock,
        .studentRank = studentRank
    };
    //printf("[%d] <%d> Wysylam do [%d] tag=%d\n",myRank,myClock, destination, tag );
    MPI_Send(&msg, sizeof(Msg), MPI_BYTE, destination, tag, MPI_COMM_WORLD);
}


void consumeTime(){    
    sleep(2);// (rand() % (MAX_TIME_WAIT - MIN_TIME_WAIT) + MIN_TIME_WAIT));
}


int getStudent(){
    srandom(myRank);
    int studentRank = rand()%((STUDENTS + WINE_MAKERS)-WINE_MAKERS) + WINE_MAKERS;
    printf("[%d] <%d> Ubiegam sie o studenta: %d\n", myRank,myClock, studentRank);
    return studentRank; 
}

int countAck(int *ackList){
    int sum = 0;
    for (int i =0; i< WINE_MAKERS; i++){
        sum += ackList[i];
    }
    return sum;
}

void addAck(int *ackList, int newAckRank){
    ackList[newAckRank] = 1;
}

void clearAck(int *ackList){
    for(int i = 0; i< WINE_MAKERS; i++)
        ackList[i]=0;
}

void *communication(void *cos){
    Msg msg;
    int ackList[WINE_MAKERS] = {0};
    bool isRecvClockSmaller;
    MPI_Status status;
    while(true)
    {
        if(wineMakerStatus == WAITING_FOR_SAFEPLACE){
            MPI_Recv(&msg, sizeof(Msg), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            
            isRecvClockSmaller = msg.clock < myClock;
            int oldClock = myClock;
            tick(msg.clock);
            
            switch (status.MPI_TAG)
            {
            case REQ:
                if(isRecvClockSmaller || ((oldClock == msg.clock) && (status.MPI_SOURCE < myRank))){
                    // nowy student? 
                    printf("[%d] <%d> Wiadomosc to REQ wiec wysylam ACK bo jestem gorszy od [%d] <%d>\n", myRank,myClock, status.MPI_SOURCE, msg.clock);
                    sendMsg(status.MPI_SOURCE, ACK);
                    printf("[%d] <%d> Wysyłam ACK do [%d]\n", myRank,myClock, status.MPI_SOURCE); // debug
                }
                else if(wineMakerStatus == WAITING_FOR_SAFEPLACE){
                    printf("[%d] <%d> Dodaje do oczekujacych winiarza: [%d]\n", myRank,myClock, status.MPI_SOURCE); // debug
                    requestingWineMakers.push_back(status.MPI_SOURCE);
                }
                break;
            case ACK:
                addAck(ackList, status.MPI_SOURCE);
                printf("[%d] <%d> Otrzymałem ACK do [%d]\n", myRank,myClock, status.MPI_SOURCE); // debug
                if(countAck(ackList) == MIN_ACK){
                    wineMakerStatus = IN_SAFEPLACE;
                    clearAck(ackList);
                }
                break;
            
            default:
                break;
            }
        }
        // else{
        //     MPI_Recv(&msg, sizeof(Msg), MPI_BYTE, MPI_ANY_SOURCE, ACK, MPI_COMM_WORLD, &status);
        //     tick(msg.clock);
        // }
    }
}


void wineMakers(){
    while(true){
        // produkujemy wino
        consumeTime();
        wineMakerStatus = WAITING_FOR_SAFEPLACE;
        printf("[%d] <%d> Wyprodukowalem wino, ubiegam sie o bezpieczne miejsce\n", myRank,myClock);
        //int myStudentRank = getStudent();
        //wyslij ze chcesz sie wymieniac
        for(int i = 0; i < WINE_MAKERS; i++){
            if(i != myRank)
                sendMsg(i, REQ);
        }

        while(wineMakerStatus == WAITING_FOR_SAFEPLACE) rand();

        printf("%d <%d> Mam bezpieczne miejsce => WYMIANA\n", myRank, myClock);
        consumeTime();
        printf("[%d] <%d> Nastapila wymiana. Zwalniam bezpieczne miejsce\n", myRank,myClock);
        for(int rank : requestingWineMakers){
            sendMsg(rank, ACK);
        }
        requestingWineMakers.clear();
    }
}

void finalize() {
    pthread_join(commThread, NULL);
    MPI_Finalize();
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &maxRank);

    
    printf("INIT [%d] \n", myRank);
    pthread_create(&commThread, NULL, communication, 0);
    wineMakers();


	finalize();
    return 0;

}
