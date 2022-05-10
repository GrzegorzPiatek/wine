// msg types
#define MSG_ACK 1 //ack
#define SAFE_PLACE_REQUEST 2 //

struct msg
{
    int type;
    int wineAmount;
    int lamport;
    int ack;
};

