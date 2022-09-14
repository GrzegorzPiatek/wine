#include <thread>
#include <vector>
#include <stdlib.h>
#include <utility>
#include <chrono>
#include <mutex>
#include "constants.h" 
#include <mpi.h> //Zły kompilator wpięty w IDE
#include <cstdlib>
#include <time.h>


using namespace std;
extern int myRank,maxRank;
extern MPI_Datatype MPI_MSG_TYPE;

class Winer{
private:
    mutex clockMtx;
    int clock;

    mutex ackMutex;
    int ackCounter = 0;

    mutex inSafePlaceMtx;

    int wineAmount;

    int pendingRequests[WINE_MAKERS] = {0};

    MPI_Status status;

    int lastReqClock = 0;

    bool haveWine = false;

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
    srand(time(NULL));

    clock = 1;
    // mainThreadWiner = new thread(&Winer::threadMainWiner,this);

    communicationThreadWiner = new thread(&Winer::threadCommunicateWiner,this);
    threadMainWiner();
}

void Winer::threadMainWiner(){
    // log("inside main thread");

    while(true){
        log("PRODUKUJE");
        makeWine();
        // log("after makeWine()");
        broadCastWiners(); //Zapytanie winiarzy o bm
        log("Czekam na zgode");
        // inSafePlaceMtx.lock();
        safePlace();
        // inSafePlaceMtx.unlock();  
        // log("after safePlace()"); 
    }
}

void Winer::threadCommunicateWiner(){
    Msg msg;
    log("inside communicate thread");
    while(true){
        // if(wineAmount == 0) inSafePlaceMtx.lock();
        MPI_Recv(&msg, 1, MPI_MSG_TYPE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        // log("@@ Recv " + to_string(status.MPI_TAG) + " from " + to_string(status.MPI_SOURCE));
        if(status.MPI_SOURCE == myRank){
            haveWine = true;
        };        // log("recv" + to_string(status.MPI_TAG));
        
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
    if (!haveWine || clock > msg.clockT || (clock == msg.clockT && myRank > sourceRank)){
        sendAck(sourceRank,msg.clockT);
    }
    else pendingRequests[sourceRank]=msg.clockT;
}

void Winer::winerAckHandler(Msg msg){
    if (msg.clockT == lastReqClock){
        incrementAck();
        if (ackCounter == MIN_ACK){ //Wzór z kartki z algorytmem 
            //Wejście do bezpiecznego miejsca
            // inSafePlaceMtx.unlock();
            // log("        ### MAM ACK");
            sleep(2);
            sendAck(myRank, clock);
            MPI_Recv(&msg, 1, MPI_MSG_TYPE, myRank, REQ, MPI_COMM_WORLD, &status);
            resetAck();
        }
    }
}

void Winer::safePlace(){
    //Wysyłka do studentów o ilości wina jaką mam
    Msg msg;
    MPI_Recv(&msg, 1, MPI_MSG_TYPE, myRank, ACK, MPI_COMM_WORLD, &status);
    msg.wine = wineAmount;
    msg.clockT = clock;
    // clockMtx.lock();
    log("Mam bezpieczne miejsce");
    for (int i = WINE_MAKERS;i<maxRank;i++){
        // log("In safePlace | Send to student: " + to_string(i));
        sendMsg(&msg, i, WINE);
    }
    // clock++;
    // clockMtx.unlock();
    
    while (wineAmount > 0){
        //Czekanie na potwierdzenie wymiany od studenta
        MPI_Recv(&msg, 1, MPI_MSG_TYPE, MPI_ANY_SOURCE, EXCHANGE, MPI_COMM_WORLD, &status);
        wineAmount -= msg.wine;
        log("Oddałem wina " + to_string(msg.wine) + " Studentowi " + to_string(status.MPI_SOURCE));
    }
    incrementClock();
    sendAckToRest();
    sendMsg(&msg, myRank,REQ);
}

void Winer::sendAck(int destinationRank, int requestClock){
    Msg msg;
    msg.clockT = requestClock;
    // log("Sending ACK to:  " + to_string(destinationRank));
    sendMsg(&msg, destinationRank, ACK);
    // incrementClock();
}

void Winer::sendAckToRest(){
    // log("Sending overdue ACK ");
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
    // log("Increment ACK" + to_string(ackCounter));
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
    // clockMtx.lock();
    Msg msg;
    msg.clockT = clock;
    for (int i = 0 ;i<WINE_MAKERS;i++){
        if (i==myRank) continue;
        // log("in broadCastWiners | Send to " + to_string(i));
        sendMsg(&msg, i, REQ);
    }
    lastReqClock = clock;
    // clock++;
    // clockMtx.unlock();
}

void Winer::makeWine(){
    this_thread::sleep_for(chrono::seconds(rand() % (MAX_TIME_WAIT) + MIN_TIME_WAIT));
    // sleep(1);
    wineAmount = rand() % (MAX_WINE) + 1;
}

void Winer::log(string msg){
    cout << "W " << myRank << ":" << clock << "> " << msg << " wine:" << wineAmount << endl;
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
    // log("Send: " + to_string(tag) + " to" + to_string(destinationRank));
}
