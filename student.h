#include <thread>
#include <vector>
#include <stdlib.h>
#include <utility>
#include <chrono>
#include <mutex>
#include "constants.h"
#include <mpi.h>
#include <cstdlib>
#include <time.h>

using namespace std;
extern int myRank, maxRank; //set it in main.cpp
extern MPI_Datatype MPI_MSG_TYPE;

class Student {
private:
    mutex clockMtx;
    int clock;

    mutex ackMtx;
    int ackCounter = 0;

    int requestedOffer;
    int wine = 0;

    mutex offerMtx;
    int wineOffers[WINE_MAKERS] = {0};
    int myTargetOffer;

    //first places just 0 for winers - never used
    int pendingRequests[STUDENTS+WINE_MAKERS] = {0}; 
    mutex exchangeMtx;

    MPI_Status status;
    
    bool needWine = false;


protected:
    thread *mainThread, *communicateThread;

    void threadMain();

    void threadCommunicate();

    void incrementAck();

    void resetAck();

    void incrementClock();

    void sendReqToStudents();

    void sendUpdToStudents(int wineMaker, int wine);

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
    srand(time(NULL));

    clock = 1;
    // mainThread = new thread(&Student::threadMain, this);
    communicateThread = new thread(&Student::threadCommunicate, this);
    threadMain();
}

void Student::threadMain() {
    log("inside main thread");

    while(true){
        if(wine == 0) sleepAndSetWine();
        log("after sleepAndSetWine()");
        sendReqToStudents();
        log("after sendReqToStudents()");
        exchange();
        log("after exchange()");
    }
}

void Student::threadCommunicate() {
    Msg msg;
    log("inside communicate thread");
    while (true) {
        // if (wine == 0) exchangeMtx.lock();
        MPI_Recv(&msg, 1, MPI_MSG_TYPE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        log("@@ Recv " + to_string(status.MPI_TAG) + " from " + to_string(status.MPI_SOURCE));
        if(status.MPI_SOURCE == myRank){
            needWine = true;
        }; //ignore selfmsg
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
            if(status.MPI_TAG == WINE) {
                updateOffer(status.MPI_SOURCE, msg.wine);
            }
        }
    }
}

void Student::studentReqHandler(Msg msg, int sourceRank) {
    if(!needWine || clock > msg.clockT || (clock == msg.clockT && myRank > sourceRank)) {
        sendAck(sourceRank);
    }
    else {
        pendingRequests[sourceRank] = 1;
        string logg = "";
        for (int i = 0; i<maxRank; i++){
            logg += (to_string(i)+":"+ to_string(pendingRequests[i])+" ");
        }
        log(" pendingRequests: " + logg);
    }
}

void Student::studentAckHandler(Msg msg) {
    incrementAck();
    if(ackCounter == STUDENTS -1) {
        // exchangeMtx.unlock();
        log("        ### MAM ACK");
        sleep(2);
        sendAck(myRank);
        MPI_Recv(&msg, 1, MPI_MSG_TYPE, myRank, REQ, MPI_COMM_WORLD, &status);
        needWine = false;
    }
}

void Student::exchange() {
    log("Wait for exchange");
    // exchangeMtx.lock();
    Msg msg;
    MPI_Recv(&msg, 1, MPI_MSG_TYPE, myRank, ACK, MPI_COMM_WORLD, &status);
    log("Start exchange");
    int wineMaker = chooseOffer();
    int getableWine = min(wine, wineOffers[wineMaker]);
    updateOffer(wineMaker, - getableWine);
    wine -= getableWine;
    sendExchangeToWineMaker(wineMaker, getableWine);
    sendUpdToStudents(wineMaker, getableWine);
    sendAckToPendingRequests();
    resetAck();
    // exchangeMtx.unlock();
    sendMsg(&msg,myRank, REQ);
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
    //incrementClock();
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
    sendMsg(&msg, destinationRank, EXCHANGE);
    incrementClock();
}


void Student::sendReqToStudents() {
    log("Send req to all students");
    // clockMtx.lock();
    Msg msg;
    msg.clockT = clock;
    for (int i = WINE_MAKERS; i < maxRank; i++) {
        if (i==myRank) continue;
        sendMsg(&msg, i, REQ);
    }
    // clock++;
    // clockMtx.unlock();
}

void Student::sendUpdToStudents(int wineMaker, int wine) {
    log("SendUpdToSudents");
    // clockMtx.lock();
    Msg msg;
    msg.clockT = clock;
    msg.targetOffer = wineMaker;
    msg.wine = wine;
    for (int i = WINE_MAKERS; i < maxRank; i++) {
        if (i==myRank) continue;
        sendMsg(&msg, i, UPD);
    }
    // clock++;
    // clockMtx.unlock();
}

void Student::sleepAndSetWine() {
    this_thread::sleep_for(chrono::seconds(3));
    // sleep(1);
    wine = 5;//rand() % (MAX_WINE/2) + 1;
}

int Student::chooseOffer() {
    int min = MAX_WINE + 1;
    int offer = -1;
    for(int i = 0; i < WINE_MAKERS; i++){
        if((0 < wineOffers[i]) &&  (wineOffers[i] < min)){
            offer = i;
        }
    }
    Msg msg;
    if(offer <= 0){
        MPI_Recv(&msg, 1, MPI_MSG_TYPE, MPI_ANY_SOURCE, WINE, MPI_COMM_WORLD, &status);
        updateOffer(status.MPI_SOURCE, msg.wine);
        return status.MPI_SOURCE;
    }
    log("choosed offer: " + to_string(offer));
    return offer;
}

void Student::log(string msg) {
    cout << "S" << myRank << ":" <<  clock << "> " << msg << " wine:" << wine << endl;
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
    log("Send: " + to_string(tag) + " to" + to_string(destinationRank));

}
