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

int isDebugOn = 0;

int const WINE_MAKERS = 3;
int const STUDENTS = 3;

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

char whoAmI;

void debug(Msg msg, const char* info){
    printf("%c[%d] <%d> %s tag:%d \n", whoAmI, myRank,myClock, info, msg.tag);
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
    printf("%c[%d] <%d> Wysylam do [%d] tag=%d\n", whoAmI,myRank,myClock, destination, tag );
    MPI_Send(&msg, sizeof(Msg), MPI_BYTE, destination, tag, MPI_COMM_WORLD);
}


void recvMsgWait(int *source, Msg *msg, int mpiTag = MPI_ANY_TAG){
    if (isDebugOn) printf("Czekam na wiadomosc z tag= %d \n", mpiTag);
    MPI_Recv(msg, sizeof(Msg), MPI_BYTE, MPI_ANY_SOURCE, mpiTag, MPI_COMM_WORLD, &status);
    *source = msg->source;
    printf("%c[%d] <%d> Odebralem od [%d] <%d> tag=%d\n", whoAmI,myRank,myClock, *source, msg->clock, msg->tag);
}


void consumeTime(){
    // usleep(3000000 );
    
    usleep( (rand() % (MAX_TIME_WAIT - MIN_TIME_WAIT) + MIN_TIME_WAIT) * 2000);
}


void students(){
    while(true){
        consumeTime();
        int source;
        Msg msg;
        printf("%c[%d] Czekam na wino\n", whoAmI, myRank);
        recvMsgWait(&source, &msg, ACK);
        printf("%c[%d] Wysylam winiarzowi ACK\n", whoAmI, myRank);
        sendMsg(source, ACK);

    }
}


int getStudent(){
    srand(time(NULL));

    int studentRank = rand()%((STUDENTS + WINE_MAKERS)-WINE_MAKERS) + WINE_MAKERS;
    // Msg msg;
    // printf("Czekam na spragnionego studenta\n");
    // recvMsgWait(&studentRank,&msg, SREQ);
    if (isDebugOn) printf("Znalazlem chetnego studenta z rank: %d\n", studentRank);
    return studentRank; 
}

void wineMakers(){
    while(true){
        // produkujemy wino
        consumeTime();
        printf("%c[%d] <%d> Wyprodukowalem wino\n", whoAmI, myRank,myClock);
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

            if (msg.clock == myClock){ 
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
            ackCounter++;
        }
        // mamy bezpieczne miejsce
        if (isDebugOn) printf("Mam bezpieczne miejsce i studenta: %d => WYMIANA\n", myStudentRank);
        Msg msg;
        sendMsg(myStudentRank, ACK); 
        MPI_Recv(&msg, sizeof(Msg), MPI_BYTE, myStudentRank, ACK, MPI_COMM_WORLD, &status);
        if (isDebugOn) printf("--- Po wymianie\n");
    }
}


int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &maxRank);

    if (argc > 1) 
        isDebugOn = 1;
    srand(time(NULL));
    if(myRank < WINE_MAKERS){
        whoAmI = 'W';
        printf("%cINIT [%d] \n",whoAmI, myRank);
        wineMakers();
    }
    else{
        whoAmI = 'S';
        printf("%c INIT [%d] \n",whoAmI, myRank);
        students();
    }

	MPI_Finalize();
    return 0;
}
