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

int main(int argc, char *argv[])
{
    int rank, size, provided;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    myRank = rank;
    maxRank = size;
    if (myRank < WINE_MAKERS)
        Winer winer = new Winer();
    else
    {
        Student student = new Student();
    }
    MPI_Finalize();
    return 0;

}
