#include <mpi.h>
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include "constants.h"
#include <stdexcept>
#include <iostream>
#include "winer.h"
#include "student.h"

using namespace std;

int myRank, maxRank;
MPI_Datatype MPI_MSG_TYPE;

void registerMsgDatatype() {
    /* Stworzenie typu */
    /* Poniższe (aż do MPI_Type_commit) potrzebne tylko, jeżeli
       brzydzimy się czymś w rodzaju MPI_Send(&typ, sizeof(pakiet_t), MPI_BYTE....
    */
    /* sklejone z stackoverflow */
    const int FIELD_NO = 4;
    int       blocklengths[FIELD_NO] = {1,1,1,1};
    MPI_Datatype typy[FIELD_NO] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint     offsets[FIELD_NO]; 
    offsets[0] = offsetof(Msg, clockT);
    offsets[1] = offsetof(Msg, targetOffer);
    offsets[2] = offsetof(Msg, wine);
    offsets[3] = offsetof(Msg, criticCounter);

    MPI_Type_create_struct(FIELD_NO, blocklengths, offsets, typy, &MPI_MSG_TYPE);
    MPI_Type_commit(&MPI_MSG_TYPE);
}


int main(int argc, char *argv[])
{
    // for (int i = 0; i < argc; i++ ) {
    //     cout << i << ": " << argv[i] << endl;
    // }
    int numberOfWineMakers = atoi(argv[1]);
    int numberOfStudents = atoi(argv[2]);
    int numberOfSafePlaces = atoi(argv[3]);

    int rank, size, provided;    
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    registerMsgDatatype();

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if((numberOfStudents + numberOfWineMakers) != size) {
        throw invalid_argument("MPI size is not equal to number of students and wine makers!");
    }    
    myRank = rank;
    maxRank = size;

    if (myRank < numberOfWineMakers)
        Winer winer(myRank, numberOfWineMakers, numberOfStudents, numberOfSafePlaces);
    else
        Student student(myRank, numberOfStudents, numberOfWineMakers);
    
    MPI_Finalize();
    return 0;
}
