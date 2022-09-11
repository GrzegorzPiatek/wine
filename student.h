#include <thread>
#include <vector>
#include <stdlib.h>
#include <utility>
#include <chrono>
#include <mutex>
#include "constants.h"
#include <mpi.h>
#include <cstdlib>

using namespace std;
extern int myRank, maxRank; //set it in main.cpp
extern MPI_Datatype MPI_MSG_TYPE;

class Student {
private:
    mutex clockMtx;
    int clock;

    mutex ackMtx;
    int ackCounter;

    int requestedOffer;
    int wine;

    mutex offerMtx;
    int wineOffers[WINE_MAKERS] = {0};
    int myTargetOffer;

    //first places just 0 for winers - never used
    int pendingRequests[STUDENTS+WINE_MAKERS] = {0}; 
    mutex exchangeMtx;

    MPI_Status status;


protected:
    thread *mainThread, *communicateThread;

    void threadMain();

    void threadCommunicate();

    void incrementAck();

    void resetAck();

    void incrementClock();

    void broadcastStudents();

    void sendAck(int rank);

    void sendAckToPendingRequests();

    void sendExchangeToWineMaker(int destinationRank, int wine);

    void sleepAndSetWine();

    int chooseOffer();

    void exchange();

    void updateOffer(int id, int wine);

    void studentReqHandler(Msg msg, int sourceRank); // return true for ACK response

    void studentAckHandler(Msg msg);

    void log(string msg);

    void sendMsg(Msg *msg, int destinationRank, int tag);

public:
    Student();
};

Student::Student() {
    cout <<"STUDENT: " <<"myRank: " << myRank << "maxRank: " << maxRank << endl;
    clock = myRank;
    mainThread = new thread(&Student::threadMain, this);
    communicateThread = new thread(&Student::threadCommunicate, this);
}

void Student::threadMain() {
    log("inside main thread");

    while(true){
        sleepAndSetWine();
        log("after sleepAndSetWine()");
        broadcastStudents();
        log("after broadcastStudents()");
        exchange();
        log("after exchange()");
    }
}

void Student::threadCommunicate() {
    Msg msg;
    log("inside communicate thread");

    while (true) {
        exchangeMtx.lock();
        MPI_Recv(&msg, 1, MPI_MSG_TYPE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        
        if(status.MPI_SOURCE >= WINE_MAKERS) { // msg from student
            if(status.MPI_TAG == REQ) {
                studentReqHandler(msg, status.MPI_SOURCE);
            }
            else if (status.MPI_TAG == ACK) {
                studentAckHandler(msg);
            }
            else if (status.MPI_TAG == UPD) {
                updateOffer(msg.targetOffer, - msg.wine);
            }
        }
        else { // msg from wine maker
            if(status.MPI_TAG == REQ) {
                updateOffer(status.MPI_SOURCE, msg.wine);
            }
        }
    }
}

void Student::studentReqHandler(Msg msg, int sourceRank) {
    if(clock > msg.clockT || (clock == msg.clockT && myRank > sourceRank)) {
        sendAck(sourceRank);
    } 
    else {
        pendingRequests[sourceRank] = 1;
    }
}

void Student::studentAckHandler(Msg msg) {
    incrementAck();
    if(ackCounter == STUDENTS -1) {
        exchangeMtx.unlock();
        resetAck();
    }
}

void Student::exchange() {
    log("Wait for exchange");
    exchangeMtx.lock();
    log("Start exchange");
    int wineMaker = chooseOffer();
    int getableWine = min(wine, wineOffers[wineMaker]);
    updateOffer(wineMaker, - getableWine);
    wine -= getableWine;
    sendExchangeToWineMaker(wineMaker, getableWine);
    sendAckToPendingRequests();
    exchangeMtx.unlock();
}

void Student::incrementAck() {
    ackMtx.lock();
    ackCounter++;
    ackMtx.unlock();
}

void Student::resetAck() {
    ackMtx.lock();
    ackCounter = 0;
    ackMtx.unlock();
}

void Student::incrementClock() {
    clockMtx.lock();
    clock++;
    clockMtx.unlock();
}

void Student::updateOffer(int id, int wine) {
    offerMtx.lock();
    wineOffers[id] += wine;
    offerMtx.unlock();
}

void Student::sendAck(int destinationRank) {
    Msg msg;
    msg.clockT = clock;
    sendMsg(&msg, destinationRank, ACK);
    incrementClock();
}

void Student::sendAckToPendingRequests() {
    for (int i = WINE_MAKERS; i < STUDENTS; i++) {
        if (pendingRequests[i]){
            sendAck(i);
        }
    }
}

void Student::sendExchangeToWineMaker(int destinationRank, int wine) {
    Msg msg;
    msg.clockT = clock;
    msg.wine = wine;
    sendMsg(&msg, destinationRank, ACK);
    incrementClock();
}


void Student::broadcastStudents() {
    log("Send req to all students");
    clockMtx.lock();
    Msg msg;
    msg.clockT = clock;
    for (int i = WINE_MAKERS; i < maxRank; i++) {
        if (i==myRank) continue;
        sendMsg(&msg, i, REQ);

    }
    clock++;
    clockMtx.unlock();
}

void Student::sleepAndSetWine() {
    // this_thread::sleep_for(chrono::seconds(rand() % 5 + 1));
    // sleep(rand() % 5 + 1);
    wine = rand() % (MAX_WINE/2) + 1;
}

int Student::chooseOffer() {
    int min = MAX_WINE + 1;
    int offer = -1;
    for(int i = 0; i < WINE_MAKERS; i++)
        if((0 < wineOffers[i]) &&  (wineOffers[i] < min))
            offer = i;
    Msg msg;
    if(offer <= 0){
        MPI_Recv(&msg, 1, MPI_MSG_TYPE, MPI_ANY_SOURCE,MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        updateOffer(status.MPI_SOURCE, msg.wine);
        return status.MPI_SOURCE;
    }
    return offer;
}

void Student::log(string msg) {
    cout << "S" << myRank << ":" <<  clock << ">" << msg << endl;
}

void Student::sendMsg(Msg *msg, int destinationRank, int tag) {
    MPI_Send(
        msg,
        1,
        MPI_MSG_TYPE,
        destinationRank,
        tag,
        MPI_COMM_WORLD
    );
}
