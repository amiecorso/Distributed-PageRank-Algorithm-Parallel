// SANDBOX
// TODO:
// - error checking
// - Optimization: this partition only needs to record credit/degree/neighbors for ITS partition nodes
// MPI stuff next
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "mpi.h"

long MAXNODES = 10000000;
long MAXID = 0;
long MYMAX = 0;
int *COUNTS; 
int *PART;
double *RECIP;
int **NEIGHBS;

int main(int argc, char const *argv[]) {
    // MPI INIT
    int ierr, procid, numprocs;

    ierr = MPI_Init(&argc, &argv);
    ierr = MPI_Comm_rank(MPI_COMM_WORLD, &procid);
    ierr = MPI_Comm_size(MPI_COMM_WORLD, &numprocs);


    COUNTS = calloc(MAXNODES, sizeof(int));
    PART = calloc(MAXNODES, sizeof(int));

    char *graphf = argv[1];
    char *partf = argv[2];
    int numrounds = atoi(argv[3]);
    int partitions = atoi(argv[4]);

    printf("I am process %i out of %i\n", procid, numprocs);
    printf("graphf: %s, partf: %s, numrounds: %i, numparts: %i\n", graphf, partf, numrounds, partitions);

    // timing stuff
    clock_t start, end;
    double elapsed;
    start = clock();

    // READ PARTITION FILE ==============
    unsigned char *f;
    int size;
    struct stat s;
    int fd = open(partf, O_RDONLY);
    int status = fstat(fd, & s);

    status = fstat (fd, & s);
    size = s.st_size;

    f = (char *) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (!f) {
        return 1;
    }
    char *numbuf = malloc(64); // make an array for storing characters from file
    int index = 0; // for filling our numbuf
    int n, d, p;
    int firstspace = 1;
    char c;
    char *token;
    for (int i = 0; i < size; i++) {
        c = f[i];
        //putchar(c);
        if (c == '\n') {
            numbuf[index] = '\0';
            token = strtok(numbuf, "\t\t");
            n = atoi(token);
            token = strtok(NULL, "\t\t");
            d = atoi(token);
            token = strtok(NULL, "\t\t");
            p = atoi(token);
            COUNTS[n] = d;
            PART[n] = p;
            if (p == procid) {
                if (n > MYMAX) MYMAX = n;
            }
            if (n > MAXID) MAXID = n;
            //printf("n: %i,  d: %i, p: %i\n", n, d, p);
            index = 0;
        }//endif
        else {
            numbuf[index] = c;
            ++index;
        }
    }

    // ALLOCATE 2-D array ======================
    NEIGHBS = malloc((MAXID + 1) * sizeof(int *));
    for (int i = 0; i <= MAXID; i++) {
        NEIGHBS[i] = malloc((COUNTS[i] + 1) * sizeof(int));
        NEIGHBS[i][0] = 1; // INDEX info
    }

    // READ GRAPH FILE (POPULATE NEIGHBS)================
    fd = open(graphf, O_RDONLY);
        // get size of file
    status = fstat (fd, & s);
    size = s.st_size;

    f = (char *) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (!f) {
        return 1;
    }
    char *newbuf = malloc(64); // make an array for storing characters from file
    index = 0; // for filling our numbuf
    int a, b;
    int nextindex;
    for (int i = 0; i < size; i++) {
        c = f[i];
        //putchar(c);
        if (c == '\n') {
            newbuf[index] = '\0';
            index = 0;
            token = strtok(newbuf, "\t\t");
            a = atoi(token);
            token = strtok(NULL, "\t\t");
            b = atoi(token);
            //fprintf(stderr, "a: %i, b: %i\n", a, b);
            // populate neighbor arrays
            nextindex = NEIGHBS[a][0];
            NEIGHBS[a][nextindex] = b;
            NEIGHBS[a][0] = nextindex + 1;
            // handle b
            nextindex = NEIGHBS[b][0];
            NEIGHBS[b][nextindex] = a;
            NEIGHBS[b][0] = nextindex + 1;
        }
        else {
            newbuf[index] = c;
            ++index;
        }
    }
    end = clock();
    elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time to read: %f seconds\n", elapsed);

// MPI_Send(buf, count, type, dest, tag, comm)
// buf: starting address to send buffer
// count: elements insend buffer
// type: MPI_Datatype of each send buffer element
// dest: node rank ID to send the buffer to
// tag: message tag (0)
// comm: communicator (MPI_COMM_WORLD)

// MPI_Recv(buf, count, type, src, tag, comm, status)
// mostly same
// buffer will be read INTO
// src is the rank that recv from (opp of dest)

// MPI_DOUBLE, MPI_CHAR, MPI_INT, etc.
// MPI_ANY_SOURCE
int timeout = 1000; //milliseconds
int *recvbuf = malloc(8 * sizeof(int));
MPI_Request rrequest;
MPI_Status rstat;
int flag = 0;
while (1) {
    start = clock();
    elapsed = 0;
    MPI_Irecv(recvbuf, 8, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &rrequest);
    while ((elapsed < timeout) && !flag) {
        MPI_Test(&rrequest, &flag, &rstat);
        if (flag)
            printf("proc %i received message with tag: %i\n", procid, rstat.MPI_TAG);
        elapsed = ((double) (clock() - start)) / (CLOCKS_PER_SEC / 1000);
    } //end inner while
    printf("timed out waiting for message\n");
    
}//endwhile
/*
    if (procid == 0) {
        int *buffer = malloc(8 * sizeof(int));
        for (int i = 0; i < 8; i++) {
            buffer[i] = i + procid;
        }
        int *recvbuf = malloc(8 * sizeof(int));

        MPI_Request request;
        MPI_Status stat;
        int request_complete = 0;
        int send_to = (procid + 1) % numprocs;

        for (int i = 0; i < 4; i++) {
            sleep(2);
            printf("proc %i sending message with tag %i\n", procid, i);
            MPI_Isend(buffer, 8, MPI_INT, send_to, i, MPI_COMM_WORLD, &request);
        }
        for (int i = 4; i < 8; i++) {
            printf("proc %i sending message with tag %i\n", procid, i);
            MPI_Isend(buffer, 8, MPI_INT, send_to, i, MPI_COMM_WORLD, &request);
        }
    }
    
    else {
        int *recvbuf = malloc(8 * sizeof(int));
        MPI_Request rrequest;
        MPI_Status rstat;
        int flag = 0;
        MPI_Irecv(recvbuf, 8, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &rrequest);
        for (int n = 0; n < 100; n++) {
            printf("working on neighbor and sleeping one second\n");
            sleep(1);

            MPI_Test(&rrequest, &flag, &rstat);
            if (flag)
                printf("proc %i received message with tag: %i\n", procid, rstat.MPI_TAG);
            while (flag) {
                flag = 0;
                MPI_Irecv(recvbuf, 8, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &rrequest);
                printf("in while\n");
                MPI_Test(&rrequest, &flag, &rstat);
                if (flag) {
                    printf("proc %i received message with tag: %i\n", procid, rstat.MPI_TAG);
                } //endif
            } //end while
        } // end for
    } //end else
*/
    ierr = MPI_Finalize();
    return 0;
}
