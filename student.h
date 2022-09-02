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

    MPI_Status status;

protected:
    thread *mainThread, *communicateThread;

    void main();

    void communicate();

    void incrementAck();

    void resetAck();

    void incrementClock();

    void broadcastStudents(Tag tag, int targetWineMakerRank);

    void sleepAndSetWine();

    int chooseOffer();

    void updateOffer(int id, int wine);

    bool studentReqHandler(Msg msg, int source); // return true for ACK response

    void studentAckHandler(Msg msg);
public:
    Student();
};

Student::Student() {
    clock = myRank;
    mainThread = new thread(&Student::main, this);
    communicateThread = new thread(&Student::communicate, this);
}

void Student::main() {
    sleepAndSetWine();
    
}

void Student::communicate() {
    Msg msg;
    while (true) {
        MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        
        if(status.MPI_SOURCE >= WINE_MAKERS) { // msg from student
            if(status.MPI_SOURCE == REQ) {
                bool sendAck = studentReqHandler(msg, status.MPI_SOURCE);
            }
            else if (status.MPI_TAG == ACK) {
                studentAckHandler(msg);
            }
        }
        else { // msg from wine maker
            if(status.MPI_TAG == REQ) {
                updateOffer(status.MPI_SOURCE, msg.wine)
            }
        }
    }
}

bool Student::studentReqHandler(Msg msg, int, source) {
    if (msg.targetOffer != myTargetOffer) {
        return true;
    } 
    
    if(clock > msg.clock || (clock = msg.clock && myRank > source)) {
        chooseOffer(); // TODO case when requester change his mind 
        return true;
    }
    return false;
}

void Student::studentAckHandler(Msg msg) {
    if(ackCounter)
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

void Student::updateOffer(int id, int wine){
    wineOffers[id] += wine;
}


void Student::broadcastStudents(Tag tag, int targetWineMakerRank=-1) {
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
    wine = rand() % 5 + 1;
}


int Student::chooseOffer() {
    int min = 1;
    for(int i = 0; i < WINE_MAKERS; i++)
        if(0 < wineOffers[i] < min)
            min = wineOffers[i];
    return min;
}