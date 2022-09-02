#pragma ones

#define REQ 0
#define ACK 0


struct Msg {
    int clock;
    int targetOffert;
    int wine;
};

int isDebugOn = 0;

#define WINE_MAKERS = 3;
#define STUDENTS = 3;

#define SAFE_PLACES = 1;

#define MIN_ACK WINE_MAKERS - SAFE_PLACES;

#define MIN_TIME_WAIT = 1; 
#define MAX_TIME_WAIT = 2; 

int MAX_RANK = WINE_MAKERS + STUDENTS;