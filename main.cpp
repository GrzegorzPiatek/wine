#include "main.hpp"


enum Tag {
        REQ, 
        ACK,
        SREQ //Student request for wine
    };

struct Msg {
    Tag tag;
    int clock;
    int studentRank;
};


int const WINE_MAKERS = 1;
int const STUDENTS = 1;

int const SAFE_PLACES = 2;

int const MIN_ACK = WINE_MAKERS - SAFE_PLACES;

int const MAX_WINE = 10;
int const MIN_WINE = 1;

int const MIN_TIME_WAIT = 500; // ms 
int const MAX_TIME_WAIT = 1500; //ms

int myClock = 0;
int myRank;
int maxRank = WINE_MAKERS + STUDENTS;

MPI_Request request = {0};
MPI_Status status = {0};


void debug(Msg msg, const char* info){
    printf("[%d] %s tag:%d \n", myRank, info, msg.tag);
}


void tick(int newClock = 0){
    myClock = std::max(newClock, myClock) + 1;
}

void sendMsg(int destination, Tag tag, int studentRank=0){
    tick();
    Msg msg {
        .tag = tag,
        .clock = myClock,
        .studentRank = studentRank
    };
    char * info = "Send";
    sprintf(info, "%d", destination);
    debug(msg, info);
    MPI_Send(&msg, sizeof(Msg), MPI_BYTE, destination, 0, MPI_COMM_WORLD);
}

bool recvMsgWait(int *source, Msg *msg, int mpiTag = MPI_ANY_TAG){
    MPI_Recv(msg,
        sizeof(Msg),
        MPI_BYTE,
        MPI_ANY_SOURCE,
        mpiTag,
        MPI_COMM_WORLD,
        &status
    );
    debug(*msg, "Recv");
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
    debug(*msg, "Recv");
    int completed;
	MPI_Test(&request, &completed, &status);
    if(!completed)
        return false;
    else
        return true;
}

void consumeClock(){
    usleep(rand() % (MAX_TIME_WAIT - MIN_TIME_WAIT) + MIN_TIME_WAIT);
}

void students(){
    while(true){
        consumeClock();
        for(int i = 0; i < WINE_MAKERS; i++){
            sendMsg(i, REQ);
        }
        int source;
        Msg msg;
        recvMsgWait(&source, &msg);
        // wyślij NO do reszty winiarzy();
        //może pierwszy lepszy winiarz wchodzi a do pozostałych wysyłamy accept denied
    }
}


void sendFackOffToTheRestOFWineMakers(){
    for(int i = 0; i < WINE_MAKERS; i++){
        if(i != myRank)
            sendMsg(i, ACK);
    }
}


// @TODO
int getStudent(){
    int studentRank = 0;
    Msg msg;

    while(!studentRank)
        recvMsgWait(&studentRank,&msg, SREQ);
        
    return studentRank; 
}

void wineMakers(){
    while(true){
        // produkujemy wino
        consumeClock();
        int myStudentRank = getStudent();
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
            if(msg.tag = ACK){
                ackCounter++;
                continue;
            }
            
            if(msg.clock < myClock){
                sendMsg(source, ACK);
                if(msg.studentRank == myStudentRank){
                    myStudentRank = getStudent();
                }
            }
            else{ //msg.clock >= myClock // co z == ? powstaje konflikt oba nie wchodzą ? oba wchodzą? 
                ackCounter++;
            }
            tick(msg.clock); // czy my to zwiększamy tutaj
            //po odebraniu większego lamporta czy jak dostaniemy
            //nasze ack to ustawiamy lamporta na największego
            //jakiego dostaliśµuy podczas zbierania ACK
        }
        // mamy bezpieczne miejsce
        // sendMsg(); 

        // //odpowiedz innym winiarzom
        // for(int i = 0; i < WINE_MAKERS; i++){
        //     int source;
        //     Msg msg;
        //     if(recvMsgNoWait(&source, &msg)){
        //         if(msg.clock < myClock){
        //             sendMsg(source, ACK);
        //             tick();
        //         }
        //         else{
        //             tick(msg.clock);
        //         }
        //     }
        // }
    }
}


int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &maxRank);

    printf("INIT [%d] \n", myRank);

    if(myRank < WINE_MAKERS)
        wineMakers();
    else
        students();    

	MPI_Finalize();
    return 0;
}
