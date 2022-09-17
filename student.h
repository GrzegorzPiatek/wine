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
#include <algorithm>
#include <vector>

using namespace std;
extern MPI_Datatype MPI_MSG_TYPE;

class Student {
private:
    int STUDENTS, WINE_MAKERS, myRank, maxRank;
    mutex clockMtx;
    int clock;

    mutex ackMtx;
    int ackCounter;

    int requestedOffer;
    int wine;

    vector<int> wineOffers;

    int myTargetOffer;

    vector<int> pendingRequests;
    mutex exchangeMtx;

    MPI_Status status;
    
    bool needWine;

    int pendingUpdatesCounter = 0;

protected:
    thread *communicateThread;

    void init(int myRank, int students, int winers);

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

    void updatePendingUpdates(int value);

public:
    Student(int rank, int students, int winers);
};

Student::Student(int rank, int students, int winers) {
    init(rank, students, winers);
    communicateThread = new thread(&Student::threadCommunicate, this);
    threadMain();
}

void Student::init(int rank, int students, int winers) {
    myRank = rank;
    STUDENTS = students;
    WINE_MAKERS = winers;
    maxRank = STUDENTS + WINE_MAKERS;
    wineOffers.resize(STUDENTS);
    fill(wineOffers.begin(), wineOffers.end(), 0);

    pendingRequests.resize(STUDENTS);
    fill(pendingRequests.begin(), pendingRequests.end(), 0);
    
    clock = 1;
    wine = 0;
    ackCounter = 0;
    needWine = false;
    cout << "STUDENT " <<"myRank: " << myRank << endl;
}


void Student::threadMain() {
    log("inside main thread");

    while(true){
        if(wine == 0){
            log("IMPREZUJE");
            sleepAndSetWine();
            srand(time(NULL));
        }
        sendReqToStudents();
        log("Czekam na zgody");
        exchange();
    }
}

void Student::threadCommunicate() {
    Msg msg;
    while (true) {
        MPI_Recv(&msg, 1, MPI_MSG_TYPE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        log("@@ Recv " + to_string(status.MPI_TAG) + " from " + to_string(status.MPI_SOURCE));
        if(status.MPI_SOURCE == myRank){
            needWine = true;
            continue;
        }
        else if(status.MPI_SOURCE >= WINE_MAKERS) { // msg from student
            if(status.MPI_TAG == REQ) {
                studentReqHandler(msg, status.MPI_SOURCE);
            }
            else if (status.MPI_TAG == ACK) {
                studentAckHandler(msg);
            }
            else if (status.MPI_TAG == UPD) {
                updateOffer(msg.targetOffer, msg.wine);
                // updatePendingUpdates(-1);
            }
        }
        else { // msg from wine maker
            if(status.MPI_TAG == WINE) {
                if(wineOffers[status.MPI_SOURCE]== 0)
                    updateOffer(status.MPI_SOURCE, msg.wine);
            }
        }
    }
}

void Student::studentReqHandler(Msg msg, int sourceRank) {
    if(!needWine || clock > msg.clockT || (clock == msg.clockT && myRank > sourceRank)) {
        sendAck(sourceRank);
        // updatePendingUpdates(1);
    }
    else {
        pendingRequests[sourceRank] = 1;
        // string logg = "";
        // for (int i = 0; i<maxRank; i++){
        //     logg += (to_string(i)+":"+ to_string(pendingRequests[i])+" ");
        // }
        // log(" pendingRequests: " + logg);
    }
}

void Student::studentAckHandler(Msg msg) {
    incrementAck();
    if(ackCounter == STUDENTS -1) {
        // exchangeMtx.unlock();
        // log("        ### MAM ACK");
        // sleep(2);
        // while(pendingUpdatesCounter){
        //     MPI_Recv(&msg, 1, MPI_MSG_TYPE, MPI_ANY_SOURCE, UPD, MPI_COMM_WORLD, &status);
        //     updateOffer(msg.targetOffer, msg.wine);
            // updatePendingUpdates(-1);
        // }
        sendAck(myRank);
        MPI_Recv(&msg, 1, MPI_MSG_TYPE, myRank, REQ, MPI_COMM_WORLD, &status);
        needWine = false;
    }
}

void Student::exchange() {
    // log("Wait for exchange");
    // exchangeMtx.lock();
    Msg msg;
    MPI_Recv(&msg, 1, MPI_MSG_TYPE, myRank, ACK, MPI_COMM_WORLD, &status);
    log("Otrzymalem zgody");
    int wineMaker = chooseOffer();
    int getableWine = min(wine, wineOffers[wineMaker]);
    int restOfWine = wineOffers[wineMaker] - getableWine;
    updateOffer(wineMaker, restOfWine);
    wine -= getableWine;
    sendExchangeToWineMaker(wineMaker, getableWine);
    sendUpdToStudents(wineMaker, restOfWine);
    sendAckToPendingRequests();
    resetAck();
    // exchangeMtx.unlock();
    sendMsg(&msg,myRank, REQ);
}

void Student::updatePendingUpdates(int value) {
    log("Pending updates: " + to_string(pendingUpdatesCounter) + "update value: " + to_string(value));
    pendingUpdatesCounter += value;
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
    string logg = "";
    for (int i = 0; i<WINE_MAKERS; i++){
        logg += (to_string(i)+":"+ to_string(wineOffers[i])+" ");
    }
    log("Before update wineOffers: " + logg);
    wineOffers[id] = wine;
    logg = "";
    for (int i = 0; i<WINE_MAKERS; i++){
        logg += (to_string(i)+":"+ to_string(wineOffers[i])+" ");
    }
    log("After update wineOffers: " + logg);
}

void Student::sendAck(int destinationRank) {
    Msg msg;
    msg.clockT = clock;
    sendMsg(&msg, destinationRank, ACK);
    //incrementClock();
}

void Student::sendAckToPendingRequests() {
    for (int i = WINE_MAKERS; i < maxRank; i++) {
        if (pendingRequests[i]){
            sendAck(i);
        }
    }
}

void Student::sendExchangeToWineMaker(int destinationRank, int exchangeWine) {
    Msg msg;
    msg.clockT = clock;
    msg.wine = exchangeWine;
    log("SEND TO WINER " + to_string(destinationRank) + "  WINE_EXCHANGE " + to_string(exchangeWine));
    sendMsg(&msg, destinationRank, EXCHANGE);
    incrementClock();
}


void Student::sendReqToStudents() {
    // log("Send req to all students");
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
    // log("SendUpdToSudents");
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
    this_thread::sleep_for(chrono::seconds(rand() % (MAX_TIME_WAIT) + MIN_TIME_WAIT));
    // sleep(1);
    wine = rand() % (MAX_WINE) + 1;
}

int Student::chooseOffer() {
    string logg = "";
    for (int i = 0; i<WINE_MAKERS; i++){
        logg += (to_string(i)+":"+ to_string(wineOffers[i])+" ");
    }
    log(" wineOffers: " + logg);
    int min = MAX_WINE + 1;
    int offerId = -1;
    for(int i = 0; i < WINE_MAKERS; i++){
        if((wineOffers[i] > 0) && (wineOffers[i] < min)){
            offerId = i;
            min = wineOffers[i];
        }
    }
    Msg msg;
    if(offerId < 0){
        MPI_Recv(&msg, 1, MPI_MSG_TYPE, MPI_ANY_SOURCE, WINE, MPI_COMM_WORLD, &status);
        offerId = status.MPI_SOURCE;
        updateOffer(offerId, msg.wine);
        log("poczekałem i dostałem oferte od " + to_string(offerId) +"z winem: " + to_string(msg.wine));
        return offerId;
    }
    log("wybrałem z dostępnych: " + to_string(offerId));
    return offerId;
}

void Student::log(string msg) {
    cout << "S " << myRank << ":" <<  clock << " > " << msg << " wine: " << wine << endl;
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
    // log("Send: " + to_string(tag) + " to" + to_string(destinationRank));
}