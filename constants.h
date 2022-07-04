#pragma ones

enum Tag {
        REQ, 
        ACK,
    };

enum STATUS {
    WAITING_FOR_SAFEPLACE,
    IN_SAFEPLACE
};


struct Msg {
    Tag tag;
    int clock;
    int studentRank;
};

int isDebugOn = 0;

int const WINE_MAKERS = 3;
int const STUDENTS = 3;

int const SAFE_PLACES = 1;

int const MIN_ACK = WINE_MAKERS - SAFE_PLACES;

int const MIN_TIME_WAIT = 1; 
int const MAX_TIME_WAIT = 2; 

int myClock = 0;
int myRank;
int maxRank = WINE_MAKERS;