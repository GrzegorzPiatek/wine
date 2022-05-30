#include "main.hpp"


enum Tag {REQ, ACK};

struct Msg {
    Tag tag;
    int time;
};


int const WINE_MAKERS = 5;
int const STUDENTS = 5;

int const SAFE_PLACES = 4;

int const MIN_ACK = WINE_MAKERS - SAFE_PLACES;

int const MAX_WINE = 10;
int const MIN_WINE = 1;

int const MIN_TIME_WAIT = 500; // ms 
int const MAX_TIME_WAIT = 1500; //ms

int myTime = 0;
int myRank;

MPI_Request request = {0};
MPI_Status status = {0};

void tick(int newTime = 0){
    myTime = std::max(newTime, myTime) + 1;
}

void sendMsg(int destination, Tag tag){
    Msg msg {
        .tag = tag,
        .time = myTime
    };
    MPI_Send(&msg, sizeof(Msg), MPI_BYTE, destination, 0, MPI_COMM_WORLD);

}

bool recvMsgWait(int *source, Msg *msg){
    MPI_Recv(msg,
        sizeof(Msg),
        MPI_BYTE,
        MPI_ANY_SOURCE,
        MPI_ANY_TAG,
        MPI_COMM_WORLD,
        &status
    );
    int completed;
	MPI_Test(&request, &completed, &status);
    if(!completed)
        return false;
    else
        return true;
}

bool recvMsgNoWait(int *source, Msg *msg){
    MPI_Irecv(msg,
        sizeof(Msg),
        MPI_BYTE,
        MPI_ANY_SOURCE,
        MPI_ANY_TAG,
        MPI_COMM_WORLD,
        &request
    );
    int completed;
	MPI_Test(&request, &completed, &status);
    if(!completed)
        return false;
    else
        return true;
}

void consumeTime(){
    usleep(rand() % (MAX_TIME_WAIT - MIN_TIME_WAIT) + MIN_TIME_WAIT);
}

void students(){
    while(true){
        consumeTime();
        for(int i = 0; i < WINE_MAKERS; i++){
            sendMsg(i, REQ);
        }
        int source;
        Msg msg;
        recvMsgWait(&source, &msg);
    }
}

void wineMakers(){
    while(true){
        // produkujemy wino
        consumeTime();
        
        //wyslij ze chcesz sie wymieniac
        for(int i = 0; i < WINE_MAKERS; i++){
            if(i != myRank)
                sendMsg(i, REQ);
        }
        
        // czekaj na akceptacje od innych winiarzy
        int ackCounter = 0;
        while(ackCounter < MIN_ACK){
            int source;
            Msg msg;
            recvMsgWait(&source, &msg);
            if(msg.tag = ACK)
                ackCounter++;
        }

        // mamy bezpieczne miejsce
        // @TOD WYMIANA


        //odpowiedz innym winiarzom
        for(int i = 0; i < WINE_MAKERS; i++){
            int source;
            Msg msg;
            if(recvMsgNoWait(&source, &msg)){
                if(msg.time < myTime){
                    sendMsg(source, ACK);
                    tick();
                }
                else{
                    tick(msg.time);
                }
            }
        }
    }
}


int main(int argc, char const *argv[])
{

    return 0;
}
