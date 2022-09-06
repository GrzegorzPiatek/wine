#include <thread>
#include <vector>
#include <stdlib.h>
#include <utility>
#include <chrono>
#include <mutex>
#include <constants.h> // Błąd na tym?
#include <mpi> //Zły kompilator wpięty w IDE

using name std;
extern int myRank,maxRank;

class Winer{
private:
    mutex clockMtx;
    int clock;

    mutex ackMutex;
    int ackCounter;

    int wineAmount;

    int pendingRequests[WINE_MAKERS] = {0};
    

protected:
    thread *mainThreadWiner, *communicationThreadWiner;

    void threadMainWiner();

    void threadCommunicateWiner();

    void sendAckToRest();

    void incrementAck();

    void resetAck(); //Po co?

    void incrementClock();

    void sendAck(int rank);

    void makeWine(); 

    void incrementAck();

    void safePlace();

    void winerReqHandler(Msg msg, int source rank);

    void winerAckHandler(Msg msg);

    void broadCastWiners(int tag)

public:
    Winer();

};

Winer::Winer(){
    clcok = myRank;
    mainThreadWiner = new thread(&Winer::threadMainWiner,this);
    communicationThreadWiner = new thread(&Winer::threadCommunicateWiner,this)
}

void Winer::threadMainWiner(){
    while(true){
        //Tutaj inny order powinien być
        makeWine();
        broadCastWiners()
        
    
    }
}

void Winer::threadCommunicateWiner(){
    Msg msg;
    while(true){
        MPI_RECV(&msg,1,MPI_INT,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
        
        if (status.MPI_SOURCE < WINE_MAKERS){ //Komunikacja między winiarzami
            if (status.MPI_SOURCE == REQ){
                winerReqHandler(msg,status.MPI_SOURCE);

            }
            else if(status.MPI_SOURCE == ACK) {
                winerAckHandler(msg)
            }
            //Dla winiarzy chyba brak update
        } 
        //Elsa też brak

    }
}

void Winer::winerReqHandler(Msg msg, int sourceRank){
    if (clock > msg.clock || (clock == msg.clock && myRank > sourceRank)){
        sendAck(sourceRank)
    }
    else pendingRequests[sourceRank]=1;
}

void Winer::winerAckHandler(Msg msg){
    // if //Jak sobie z tym poradzić?
    if (WINE_MAKERS - ackCounter <= SAFE_PLACES){ //Wzór z kartki z algorytmem 
        //Wejście do bezpiecznego miejsca
        safePlace();
    }
}

void Winer::safePlace(){
    //TODO
    //Komunikacja do studentów o chęci wymiany i ilości wina
    while (true){
        clockMtx.lock();
        Msg msg;
        //Studentom chyba tylko zależy na ilości wina
        msg.wine = wineAmount;
     for (int i = WINE_MAKERS;i<maxRank;i++){
        MPI_Send(
            &msg,
            sizeof(Msg),
            MPI_BYTE,
            i,
            tag,
            MPI_COMM_WORLD
        );
     }
     //Czekanie na potwierdzenie wymiany od studenta
     MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

     //Odjęcie wina po wymianie
     wineAmount -= msg.wine

     if (wineAmount == 0){ //Wychodzimy z bezpiecznego miejsca i od nowa produkujemy wino
        sendAckToRest();
        makeWine();
        break;
     }
    }
    
}
void Winer::sendAckToRest(){
    for (int i = 0; i < WINE_MAKERS;i++){
        if (pendingRequests[i]){
            sendAck(i)
            pendingRequests[i]=0 //resetujemy po wysłaniu
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

void Winer::sendAck(int destinationRank){
    Msg msg;
    msg.clock = clock;
    MPI_Send(
            &msg,
            sizeof(msg),
            MPI_BYTE,
            destinationRank,
            ACK,
            MPI_COMM_WORLD
    );
    incrementClock();
}

void Winer::broadCastWiners(int tag){ //Wybieranie przez zegar nic więcej
    clockMtx.lock();
    Msg msg;
    msg.clock = myClock;

    for (int i = 0 ;i<WINE_MAKERS;i++){
        if i==myRank continue;
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

void Winer::makeWine(){
    wineAmount = rand() % (MAX_WINE/2) + 1;
}

























