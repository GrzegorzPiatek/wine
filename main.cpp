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
    int source;
};



int const WINE_MAKERS = 2;
int const STUDENTS = 2;

int studentReq[2] = {0,0};

int const SAFE_PLACES = 1;

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
    printf("<%d> [%d] %s tag:%d \n",myClock, myRank, info, msg.tag);
}


void tick(int newClock = 0){
    myClock = std::max(newClock, myClock) + 1;
}

void sendMsg(int destination, Tag tag, int studentRank=0){
    tick();
    Msg msg {
        .tag = tag,
        .clock = myClock,
        .studentRank = studentRank,
        .source = myRank
    };
    printf("<%d> [%d] Wysylam do [%d] tag=%d\n",myClock, myRank, destination, tag );
    MPI_Send(&msg, sizeof(Msg), MPI_BYTE, destination, tag, MPI_COMM_WORLD);
}


void recvMsgWait(int *source, Msg *msg, int mpiTag = MPI_ANY_TAG){
    printf("Czekam na wiadomosc z tag= %d \n", mpiTag);
    MPI_Recv(msg, sizeof(Msg), MPI_BYTE, MPI_ANY_SOURCE, mpiTag, MPI_COMM_WORLD, &status);
    *source = msg->source;
    printf("<%d> [%d] Odebralem od [%d] tag=%d\n",myClock, myRank, *source, msg->tag);
}


void consumeTime(){
    usleep(3000000 ); // 1000 * rand() % (MAX_TIME_WAIT - MIN_TIME_WAIT) + MIN_TIME_WAIT);
}


void students(){
    while(true){
        consumeTime();
        printf("Wytrzezwialem, wysylam req o wino\n");
        for(int i = 0; i < WINE_MAKERS; i++){
            sendMsg(i, SREQ);
        }
        int source;
        Msg msg;
        printf("Czekam na wino\n");
        recvMsgWait(&source, &msg);
    }
}


int getStudent(){
    int studentRank = 0;
    Msg msg;
    printf("Czekam na spragnionego studenta\n");
    recvMsgWait(&studentRank,&msg, SREQ);
    printf("Znalazlem chetnego studenta z rank: %d\n", studentRank);
    return studentRank; 
}

void wineMakers(){
    while(true){
        // produkujemy wino
        consumeTime();
        printf("Mam wino\n");
        int myStudentRank = getStudent();
        //wyslij ze chcesz sie wymieniac
        for(int i = 0; i < WINE_MAKERS; i++){
            if(i != myRank)
                sendMsg(i, REQ);
        }
        
        // czekaj na akceptacje od innych winiarzy
        int ackCounter = 0;
        while(ackCounter < MIN_ACK){
            consumeTime();
            int source;
            Msg msg;
            recvMsgWait(&source, &msg);
            if(msg.tag == ACK){
                ackCounter++;
                continue;
            }
            
            if(msg.clock < myClock){
                sendMsg(source, ACK);
                if(msg.studentRank == myStudentRank){
                    myStudentRank = getStudent();
                    ackCounter = 0;
                }
                tick(msg.clock);
                continue;
            }

            else if (msg.clock == myClock){ 
                if (source < myRank){
                    sendMsg(source, ACK);
                    if(msg.studentRank == myStudentRank){
                        myStudentRank = getStudent();
                        ackCounter = 0;
                    }
                }
                tick(msg.clock);
                continue;
            }
        }
        // mamy bezpieczne miejsce
        printf("Mam bezpieczne miejsce i studenta: %d => WYMIANA\n", myStudentRank);
        sendMsg(myStudentRank, ACK); 
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
