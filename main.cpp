#include <mpi.h>
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include "constants.h"
#include <stdexcept>
#include <iostream>

using namespace std;

int myRank, maxRank;

int main(int argc, char *argv[])
{
    // if (argc < 4) {
    //     cerr << "Exacly 3 arguments needed. [1] number of wine maker, [2] number of students, [3] number of safe places";
    // }
    
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &maxRank);


    MPI_Finalize();
    return 0;

}
