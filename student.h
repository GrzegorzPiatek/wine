#include <thread>
#include <vector>
#include <stdlib.h>
#include <utility>
#include <chrono>
#include <mutex>
#include <constants.h>
#include <mpi>

using namespace std;
extern int myRank, maxRank; //set it in main.cpp

class Student {
private:
    mutex clockMtx;
    int clock;

    mutex ackMutex;
    int ackCounter;

    int requestedOffer;
    int wine;

    mutex offerMtx;
    int wineOffers[WINE_MAKERS] = {0};
    int myTargetOffer;

    int pendingRequests[STUDENTS] = {0};

    mutex exchangeMtx;

    MPI_Status status;

protected:
    thread *mainThread, *communicateThread;

    void threadMain();

    void threadCommunicate();

    void incrementAck();

    void resetAck();

    void incrementClock();

    void broadcastStudents(int tag, int targetWineMakerRank);

    void sendAck(int rank);

    void sendAckToPendingRequests();

    void sleepAndSetWine();

    int chooseOffer();

    void exchange();

    void updateOffer(int id, int wine);

    void studentReqHandler(Msg msg, int sourceRank); // return true for ACK response

    void studentAckHandler(Msg msg);
public:
    Student();
};

Student::Student() {
    clock = myRank;
    mainThread = new thread(&Student::threadMain, this);
    communicateThread = new thread(&Student::threadCommunicate, this);
}

void Student::threadMain() {
    while(true){
        sleepAndSetWine();
        broadcastStudents(REQ); //Funkcja 2-argumentowa
        exchange();
    }
}

void Student::threadCommunicate() {
    Msg msg;
    while (true) {
        MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        
        if(status.MPI_SOURCE >= WINE_MAKERS) { // msg from student
            if(status.MPI_SOURCE == REQ) {
                studentReqHandler(msg, status.MPI_SOURCE);
            }
            else if (status.MPI_TAG == ACK) {
                studentAckHandler(msg);
            }
            else if (status.MPI_TAG == UPD) {
                updateOffer(msg.targetOffert, - msg.wine) //Argumenty
            }
        }
        else { // msg from wine maker
            if(status.MPI_TAG == REQ) {
                updateOffer(status.MPI_SOURCE, msg.wine)
            }
        }
    }
}

void Student::studentReqHandler(Msg msg, int sourceRank) {
    if(clock > msg.clock || (clock == msg.clock && myRank > sourceRank)) { //Porównanie zamiast przypisania powinno być xd
        sendAck(sourceRank);
    } 
    else {
        pendingRequests[sourceRank] = 1;
    }
}

void Student::studentAckHandler(Msg msg) {
    if(ackCounter)
}

void Student::exchange() {
    exchangeMtx.lock();
    int wineMaker = chooseOffer();
    int getableWine = min(wine, wineOffers[wineMaker]);
    updateOffer(wineMaker, - getableWine);
    wine -= getableWine;
    sendExchangeToWineMaker(wineMaker, getableWine);
    sendAckToPendingRequests();
    exchangeMtx.unlock();
}

void Student::incrementAck() {
    ackMutex.lock();
    ackCounter++;
    ackMutex.unlock();
}

void Student::resetAck() {
    ackMutex.lock();
    ackCounter = 0;
    ackMutex.unlock();
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
    msg.clock = clock;
    MPI_Send(
            &msg,
            sizeof(Msg),
            MPI_BYTE,
            destinationRank,
            ACK,
            MPI_COMM_WORLD
        );
    incrementClock();
}

void Student::sendAckToPendingRequests() {
    for (int i = 0; i < STUDENTS; i++) {
        if (pendingRequests[i]){
            sendAck(i);
            //Nie powinno pójść i+WINE_MAKERS bo chcemy wysłać studentom którzy od nas nie dostali jeszcze ACK?
        }
    }
}

void Student::sendExchangeToWineMaker(int destinationRank, int wine) {
    Msg msg;
    msg.clock = clock;
    msg.wine = wine;
    MPI_Send(
            &msg,
            sizeof(Msg),
            MPI_BYTE,
            destinationRank,
            ACK,
            MPI_COMM_WORLD
        );
    incrementClock();
}

void Student::broadcastStudents(int tag, int targetWineMakerRank=0, int wine = 0) {
    clockMtx.lock();
    Msg msg;
    msg.clock = myClock;
    msg.targetRank = targetWineMakerRank;
    msg.wine = wine;
    for (int i = WINE_MAKERS; i < maxRank; i++) {
        if (i==myRank) continue;
        MPI_Send(
            &msg,
            sizeof(Msg),
            MPI_BYTE,
            i,
            tag,
            MPI_COMM_WORLD
        );
    }
    clock++;
    clockMtx.unlock();
}

void Student::sleepAndSetWine() {
    this_thread::sleep_for(chrono::seconds(rand() % 5 + 1));
    wine = rand() % (MAX_WINE/2) + 1;
}


int Student::chooseOffer() {
    int min = MAX_WINE + 1;
    int offer = -1;
    for(int i = 0; i < WINE_MAKERS; i++)
        if(0 < wineOffers[i] < min)
            offer = i;
    return offer;
}