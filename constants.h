#pragma ones

#define REQ 0
#define ACK 1
#define UPD 2 // tell students to update offer after exchange

struct Msg {
    int clock;
    int targetOffert;
    int wine;
};

int isDebugOn = 0;

#define WINE_MAKERS 3;
#define STUDENTS 3;

#define SAFE_PLACES 1;

#define MIN_ACK WINE_MAKERS - SAFE_PLACES;

#define MIN_TIME_WAIT 1; 
#define MAX_TIME_WAIT 2; 

#define MAX_RANK  WINE_MAKERS + STUDENTS;

#define MAX_WINE 10