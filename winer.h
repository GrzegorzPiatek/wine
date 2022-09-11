#include <thread>
#include <vector>
#include <stdlib.h>
#include <utility>
#include <chrono>
#include <mutex>
#include "constants.h" 
#include <mpi.h> //Zły kompilator wpięty w IDE
#include <cstdlib>


using namespace std;
extern int myRank,maxRank;
extern MPI_Datatype MPI_MSG_TYPE;

class Winer{
private:
    mutex clockMtx;
    int clock;

    mutex ackMutex;
    int ackCounter;

    mutex inSafePlaceMtx;

    int wineAmount;

    int pendingRequests[WINE_MAKERS] = {0};

    MPI_Status status;

protected:
    thread *mainThreadWiner, *communicationThreadWiner;

    void threadMainWiner();

    void threadCommunicateWiner();

    void sendAckToRest();

    void incrementAck();

    void resetAck();

    void incrementClock();

    void sendAck(int rank, int requestClock);

    void makeWine(); 

    void safePlace();

    void winerReqHandler(Msg msg, int sourceRank);

    void winerAckHandler(Msg msg);

    void broadCastWiners();

    void log(string msg);

    void sendMsg(Msg *msg, int destinationRank, int tag);

public:
    Winer();

};

Winer::Winer(){
    cout << "WINIARZ: " << "myRank: " << myRank << "maxRank: " << maxRank << endl;

    clock = myRank;
    mainThreadWiner = new thread(&Winer::threadMainWiner,this);

    communicationThreadWiner = new thread(&Winer::threadCommunicateWiner,this);
}

void Winer::threadMainWiner(){
    log("inside main thread");

    while(true){
        makeWine();
        log("after makeWine()");
        broadCastWiners(); //Zapytanie winiarzy o bm
        log("after broadCastWiners()");
        inSafePlaceMtx.lock();
        safePlace();
        inSafePlaceMtx.unlock();  
        log("after safePlace()"); 
    }
}

void Winer::threadCommunicateWiner(){
    Msg msg;
    log("inside communicate thread");
    while(true){
        inSafePlaceMtx.lock();
        MPI_Recv(&msg, 1, MPI_MSG_TYPE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        log("recv" + to_string(status.MPI_TAG));
        
        if (status.MPI_SOURCE < WINE_MAKERS){ //Komunikacja między winiarzami
            if (status.MPI_TAG == REQ){
                winerReqHandler(msg,status.MPI_SOURCE);

            }
            else if(status.MPI_TAG == ACK)  {
                winerAckHandler(msg);
            }
            
        } 
        

    }
}

void Winer::winerReqHandler(Msg msg, int sourceRank){
    if (clock > msg.clockT || (clock == msg.clockT && myRank > sourceRank)){
        sendAck(sourceRank,msg.clockT);
    }
    else pendingRequests[sourceRank]=msg.clockT;
}

void Winer::winerAckHandler(Msg msg){
    if (msg.clockT == clock - 1){
        incrementAck();
        if (WINE_MAKERS - ackCounter <= SAFE_PLACES){ //Wzór z kartki z algorytmem 
            //Wejście do bezpiecznego miejsca
            inSafePlaceMtx.unlock();
            resetAck();
        }
    }
}

void Winer::safePlace(){
    //Wysyłka do studentów o ilości wina jaką mam
    clockMtx.lock();
    Msg msg;
    msg.wine = wineAmount;
    msg.clockT = clock;
    for (int i = WINE_MAKERS;i<maxRank;i++){
        log("In safePlace | Send to student: " + to_string(i));
        sendMsg(&msg, i, REQ);
    }
    clock++;
    clockMtx.unlock();
    
    while (wineAmount > 0){
     //Czekanie na potwierdzenie wymiany od studenta
     MPI_Recv(&msg, 1, MPI_MSG_TYPE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

     if (status.MPI_SOURCE >= WINE_MAKERS){ //Sprawdzenie czy wiadomość przyszła od studenta
        //Odjęcie wina po wymianie
        wineAmount -= msg.wine;
     }

    }
    
    sendAckToRest();
}

void Winer::sendAck(int destinationRank, int requestClock){
    Msg msg;
    msg.clockT = requestClock;
    log("Sending ACK to:  " + to_string(destinationRank));
    sendMsg(&msg, destinationRank, ACK);
    incrementClock();
}

void Winer::sendAckToRest(){
    log("Sending overdue ACK ")
    for (int i = 0; i < WINE_MAKERS;i++){
        if (pendingRequests[i]){
            sendAck(i,pendingRequests[i]);
            pendingRequests[i]=0; //resetujemy po wysłaniu
        }
    }
}

void Winer::incrementAck(){
    ackMutex.lock();
    ackCounter ++;
    ackMutex.unlock();
}

void Winer::resetAck(){
    ackMutex.lock();
    ackCounter = 0;
    ackMutex.unlock();
}

void Winer::incrementClock(){
    clockMtx.lock();
    clock++;
    clockMtx.unlock();
}


void Winer::broadCastWiners(){ //Wybieranie przez zegar nic więcej
    clockMtx.lock();
    Msg msg;
    msg.clockT = clock;
    for (int i = 0 ;i<WINE_MAKERS;i++){
        if (i==myRank) continue;
        log("In safePlace | Send to " + to_string(i));
        sendMsg(&msg, i, REQ);
    }
    clock++;
    clockMtx.unlock();
}

void Winer::makeWine(){
    // this_thread::sleep_for(chrono::seconds(rand() % 5 + 1));
    // sleep(rand() % 5 + 1);
    wineAmount = rand() % (MAX_WINE/2) + 1;
}

void Winer::log(string msg){
    cout << "W" << myRank << ':' << clock << '>' << msg <<endl;
}

void Winer::sendMsg(Msg *msg, int destinationRank, int tag) {
    MPI_Send(
        msg,
        1,
        MPI_MSG_TYPE,
        destinationRank,
        tag,
        MPI_COMM_WORLD
    );
}
