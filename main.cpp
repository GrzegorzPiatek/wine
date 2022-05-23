#include <cstdlib>
#include <stdio.h>


enum Tag {REQ, ACK};

struct Msg {
    Tag tag;
    int time;
    int wine;
};


int const WINE_MAKERS = 5;
int const STUDENTS = 5;

int const SAFE_PLACES = 4;

int const MAX_WINE = 10;
int const MIN_WINE = 1;

int const MIN_TIME_WAIT = 500; // ms 
int const MIN_TIME_WAIT = 1500; //ms

bool NOWAIT = false;
int myRank;
int maxRank;
int myTime;

int main(int argc, char const *argv[])
{
    
    return 0;
}
