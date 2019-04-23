/*
 * Sample MPI code for communication between master node with slaves
 * we define a custom data structure named message named person
 * we send an instance of this structure to each client through the MPI API
 *
*/


#include <iostream>
#include <mpi.h>

struct Person {
    int id;
    char name[100];
};


int main(int argc, char* argv[])
{
    int rank, number_of_tasks;

    // initialize MPI
    MPI_Init(&argc, &argv);

    // extract current processes rank into the rank variable
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // extract the total number of processes into the number_of_tasks variable
    MPI_Comm_size(MPI_COMM_WORLD, &number_of_tasks);

    // required data types to define a custom datatype for MPI
    MPI_Datatype person_type, oldtypes[2];
    int blockcounts[2];
    MPI_Aint offsets[2], extent, lower_bound;

    /*
     * set offset of person.rank field to 0
     * define premitive data type of person.rank to MPI_INT
     * set number of elements we have from this data type
    */
    offsets[0] = 0;
    oldtypes[0] = MPI_INT;
    blockcounts[0] = 1;

    /*
     * extract size of MPI_INT into variable extent
     * set offset of person.name to 1*sizeof(MPI_INT)
     * define premitive data type of person.name as MPI_CHAR
     * set number of MPI_CHAR elements we have to 100
    */
    MPI_Type_get_extent(MPI_INT, &lower_bound, &extent);
    offsets[1] = 1*extent;
    oldtypes[1] = MPI_CHAR;
    blockcounts[1] = 100;

    // define structured data type and commit it into the MPI environment
    MPI_Type_create_struct(2, blockcounts, offsets, oldtypes, &person_type);
    MPI_Type_commit(&person_type);


    if(rank == 0) {
        std::cout << "in master node id is: " << rank << std::endl;

        // create an instance of the Person structure
        Person person = {1, "master"};

        /*
         * send the person object to another process with a rank == 1
         * MPI_Send(void *buf, int count, MPI_Datatype, int dest, int tag, MPI_Comm)
        */
        MPI_Send(&person, 1, person_type, 1, 1, MPI_COMM_WORLD);
    }
    else {
        std::cout << "in slave node id is: " << rank << std::endl;

        Person person;
        MPI_Status status;

        /*
         * receive message from master node with rank == 0
         * MPI_Recv(void *buf, int count, MPI_Datatype, int source, int tag, MPI_Comm, MPI_Status)
        */
        MPI_Recv(&person, 1, person_type, 0, 1, MPI_COMM_WORLD, &status);
        std::cout << "received data from master node." << std::endl << "id = " << person.id << ", name = " << person.name << std::endl;
    }

    MPI_Finalize();

    return 0;
}

