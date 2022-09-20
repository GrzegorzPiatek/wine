#ifndef CONSTANTS_H
#define CONSTANTS_H

#define REQ 0
#define ACK 1
#define UPD 2 // tell students to update offer after exchange
#define EXCHANGE 4
#define WINE 5

// #define WINE_MAKERS 2
// #define STUDENTS 2

// #define SAFE_PLACES 1

#define MIN_ACK WINE_MAKERS - SAFE_PLACES

#define MIN_TIME_WAIT 1
#define MAX_TIME_WAIT 2

#define MAX_RANK  WINE_MAKERS + STUDENTS

#define MAX_WINE 5


struct Msg {
    int clockT;
    int targetOffer;
    int wine;
    int criticCounter;
};

// extern int STUDENTS, WINE_MAKERS, SAFE_PLACES;


#endif //CONSTANTS_H
