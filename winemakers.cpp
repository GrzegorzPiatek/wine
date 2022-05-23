#include <stdlib.h>
#include "constants.h"

class winemakers
{
private:
    int wineAmount;
    int id;
    int lamport;

public:
    winemakers(int);
    
    void setLamport(int);
    int incrementLamport();
    void send(int msg_type);
    void askSafePlace();
    int makeWine();
    int spendWine(int wineSpendedAmount);

    ~winemakers();
};


winemakers::winemakers(int id)
{
    this->id = id;
    this->lamport = 0;
}

void winemakers::setLamport(int lamport){
    this->lamport = MAX(lamport, this->lamport);
    this->incrementLamport();
}

int winemakers::incrementLamport(){
    return this->lamport++;
}

winemakers::~winemakers()
{
}


}