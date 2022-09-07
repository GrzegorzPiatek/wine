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

    mutex inSafePlaceMtx;

    int wineAmount;

    int pendingRequests[WINE_MAKERS] = {0};
    int gotAckFrom[WINE_MAKERS] = {0};
    

protected:
    thread *mainThreadWiner, *communicationThreadWiner;

    void threadMainWiner();

    void threadCommunicateWiner();

    void sendAckToRest();

    void incrementAck();

    void resetAck(); //Po co?

    void incrementClock();

    void sendAck(int rank, int requestClock);

    void makeWine(); 

    void incrementAck();

    void safePlace();

    void winerReqHandler(Msg msg, int source rank);

    void winerAckHandler(Msg msg);

    void broadCastWiners(int tag);

    void log(string msg);

public:
    Winer();

};

Winer::Winer(){
    clcok = myRank;
    mainThreadWiner = new thread(&Winer::threadMainWiner,this);
    communicationThreadWiner = new thread(&Winer::threadCommunicateWiner,this);
}

void Winer::threadMainWiner(){
    while(true){
        //Tutaj inny order powinien być
        makeWine();
        gotAckFrom = {0};
        broadCastWiners(REQ) //Zapytanie winiarzy o bm
        inSafePlaceMtx.lock();
        safePlace();
        inSafePlaceMtx.unlock();
        
    
    }
}

void Winer::threadCommunicateWiner(){
    Msg msg;
    while(true){
        inSafePlaceMtx.lock();
        MPI_RECV(&msg,1,MPI_INT,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);

        
        if (status.MPI_SOURCE < WINE_MAKERS){ //Komunikacja między winiarzami
            if (status.TAG == REQ){
                winerReqHandler(msg,status.MPI_SOURCE);

            }
            else if(status.MPI_TAG == ACK)  {
                winerAckHandler(msg);
            }
            //Dla winiarzy chyba brak update
        } 
        //Elsa też brak

    }
}

void Winer::winerReqHandler(Msg msg, int sourceRank){
    if (clock > msg.clock || (clock == msg.clock && myRank > sourceRank)){
        sendAck(sourceRank,msg.clock);
    }
    else pendingRequests[sourceRank]=msg.clock;
}

void Winer::winerAckHandler(Msg msg){
    if (msg.clock == clock - 1){
        incrementAck();
        if (WINE_MAKERS - ackCounter <= SAFE_PLACES){ //Wzór z kartki z algorytmem 
            //Wejście do bezpiecznego miejsca
            inSafePlaceMtx.unlock();
        }
    }
}

void Winer::safePlace(){
    //Wysyłka do studentów o ilości wina jaką mam
    clockMtx.lock();
    Msg msg;
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
    clock++;
    clockMtx.unlock();
    
    //Komunikacja do studentów o chęci wymiany i ilości wina
    while (wineAmount > 0){
     //Czekanie na potwierdzenie wymiany od studenta
     MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

     //Odjęcie wina po wymianie
     wineAmount -= msg.wine;
    }
    
    sendAckToRest();

    
}
void Winer::sendAckToRest(){
    for (int i = 0; i < WINE_MAKERS;i++){
        if (pendingRequests[i]){
            sendAck(i);
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

void Winer::sendAck(int destinationRank, int requestClock){
    Msg msg;
    msg.clock = requestClock;
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
    msg.clock = clock;
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
    this_thread::sleep_for(chrono::seconds(rand() % 5 + 1));
    wineAmount = rand() % (MAX_WINE/2) + 1;
}

void Winer::log(string msg){
    cout << myRank << ':' << clock << '>' << msg <<endl;
}

























