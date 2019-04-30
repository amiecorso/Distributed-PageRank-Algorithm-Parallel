// part2.c
// TODO:
// - error checking
// - Optimization: this partition only needs to record credit/degree/neighbors for ITS partition nodes
// MPI stuff next
#include <netdb.h>
#include <stddef.h>
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
int TIMEOUT = 100;

typedef struct Data {
    int ID;
    double cred;
} Data;

int main(int argc, char const *argv[]) {
    // MPI INIT
    int ierr, procid, numprocs;

    ierr = MPI_Init(&argc, &argv);
    ierr = MPI_Comm_rank(MPI_COMM_WORLD, &procid);
    ierr = MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    // MPI DATATYPE init
    int count = 2;
    int array_of_blocklengths[] = {1, 1};
    MPI_Aint array_of_displacements[] = {offsetof(Data, ID), offsetof(Data, cred)};
    MPI_Datatype array_of_types[] = {MPI_INT, MPI_DOUBLE};
    MPI_Datatype data;
    MPI_Type_create_struct(count, array_of_blocklengths, array_of_displacements, array_of_types, &data);
    MPI_Type_commit(&data);

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

    // CREATE RECIPROCAL ARRAY ============
    RECIP = calloc(MAXID + 1, sizeof(double));
    int cnt;
    for (int i = 0; i <= MAXID; i++) {
        cnt = COUNTS[i];
        if (cnt) {
            RECIP[i] = (1.0 / cnt);
        }
    } //endfor

    // PERFORM ROUNDS =====================
    double **ROUNDS = malloc((numrounds + 1) * sizeof(double *));
    ROUNDS[0] = malloc((MAXID + 1) * sizeof(double));
    for (int i = 0; i <= MAXID; i++) {
        ROUNDS[0][i] = 1.0; 
    }

    // MPI Variables
    int *reqbuf = malloc(sizeof(int));
    Data *senddatabuf = malloc(sizeof(data));
    Data *recvdatabuf = malloc(sizeof(data));
    int *recvreqbuf = malloc(sizeof(int));
    MPI_Request rrequest;
    MPI_Request datarequest;
    MPI_Status rstat;
    MPI_Status datastat;
    int dataflag = 0;
    int flag = 0;
    int source;
    int neighID;
    double ncred;

    MPI_Irecv(recvreqbuf, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &rrequest);
    for (int i = 1; i <= numrounds; i++) { // round
        MPI_Request *endroundreqs = calloc(numprocs,  sizeof(MPI_Request));
        MPI_Status *endroundstats = malloc(numprocs * sizeof(MPI_Status));
        int *endroundflags = calloc(numprocs, sizeof(int));
        int *endroundbufs = calloc(numprocs, sizeof(int));
        for (int p = 0; p < numprocs; p++) {
            MPI_Irecv(endroundbufs + p, 1, MPI_INT, p, 2, MPI_COMM_WORLD, endroundreqs + p);
        }
fprintf(stderr, "Proc %i, round %i\n", procid, i);
        start = clock();
        ROUNDS[i] = calloc(MAXID + 1, sizeof(double));
        for (int n = 0; n <= MAXID; n++) { // node
            if (PART[n] == procid) { // IF this is MY node
                double newcred = 0.0; // new credit for this node, this round
                int neighbcount = COUNTS[n];
                if (neighbcount) { // only need to perform update if this node has neighbors
                    for (int neighindex = 1; neighindex <= neighbcount; neighindex++) {
                        int neighbor = NEIGHBS[n][neighindex];
                        double neighcred = ROUNDS[i - 1][neighbor];
                        if (neighcred != 0) { // if we already have a value for this neighbor, go for it
                            newcred += neighcred * RECIP[neighbor];
                        } //end if neighcred
                        else {
                            flag = 0;
                            int extern_proc = PART[neighbor];
                            reqbuf[0] = neighbor;
                            MPI_Irecv(recvdatabuf, 1, data, extern_proc, 1, MPI_COMM_WORLD, &datarequest);
                            dataflag = 0;
//                                    fprintf(stderr, "proc %i sending request for neigh %i to proc %i\n", procid, neighbor, extern_proc);
                            MPI_Send(reqbuf, 1, MPI_INT, extern_proc, 0, MPI_COMM_WORLD);
                            fprintf(stderr, "proc %i sent request for getting node %i from proc %i\n", procid, neighbor, extern_proc);
                            while (!dataflag) { //while still don't have this node
                            fprintf(stderr, "proc %i in while waiting for data on node %i\n", procid, neighbor);
                            start = clock();
                            elapsed = 0;
                            // TIMEOUT
                            while ((elapsed < TIMEOUT) && !dataflag) {
                                MPI_Test(&datarequest, &dataflag, &datastat);
                                if (dataflag) {
                                    ROUNDS[i - 1][recvdatabuf[0].ID] = recvdatabuf[0].cred;
//                                    printf("proc %i received message with tag: %i from proc %i, node = %i, cred = %f\n", procid, rstat.MPI_TAG, rstat.MPI_SOURCE, recvdatabuf[0].ID, recvdatabuf[0].cred);
                                } //end if flag
                                elapsed = ((double) (clock() - start)) / (CLOCKS_PER_SEC / 1000);
                            } //end timeout while
                            // service a pending request
                            MPI_Test(&rrequest, &flag, &rstat);
                            if (flag) { // respond to request
                                fprintf(stderr, "proc %i (post-TIMEouT) received req msg from proc %i tag %i node %i\n", procid, rstat.MPI_SOURCE, rstat.MPI_TAG, *recvreqbuf);
                                source = rstat.MPI_SOURCE;
                                neighID = *recvreqbuf;
                                ncred = ROUNDS[i - 1][neighID];
                                senddatabuf->ID = neighID;
                                senddatabuf->cred = ncred;
//asdf                                fprintf(stderr, "proc %i responding to proc %i wit node %i and cred %f\n", procid, source, senddatabuf->ID, senddatabuf-> cred);
                                MPI_Send(senddatabuf, 1, data, source, 1, MPI_COMM_WORLD);
                                fprintf(stderr, "proc %i SENT data on node %i\n", procid, neighID);
                                MPI_Irecv(recvreqbuf, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &rrequest);
                                flag = 0;
                            } // end if flag
                            
                            } // end !dataflag while
                            // service a pending req before moving on to next neighbor
                            MPI_Test(&rrequest, &flag, &rstat);
                            if (flag) { // respond to request
                                fprintf(stderr, "proc %i (pre-NEXTNEIGH) received req msg from proc %i tag %i node %i\n", procid, rstat.MPI_SOURCE, rstat.MPI_TAG, *recvreqbuf);
                                source = rstat.MPI_SOURCE;
                                neighID = *recvreqbuf;
                                ncred = ROUNDS[i - 1][neighID];
                                senddatabuf->ID = neighID;
                                senddatabuf->cred = ncred;
    //asdf                                fprintf(stderr, "proc %i responding to proc %i wit node %i and cred %f\n", procid, source, senddatabuf->ID, senddatabuf-> cred);
                                MPI_Send(senddatabuf, 1, data, source, 1, MPI_COMM_WORLD);
                                fprintf(stderr, "proc %i SENT data on node %i\n", procid, neighID);
                                MPI_Irecv(recvreqbuf, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &rrequest);
                                flag = 0;
                            } // end if flag
                        } // end else
                    } //endfor neighindex
                    ROUNDS[i][n] = newcred;
                } //endif neighbcount
            } //end if my node
        }//endfor n
        endroundflags[procid] = 1; // set my own round completion status to 1
        reqbuf[0] = i; //indicate the round
        
        // SYNC ROUNDS
        int sum = 0;
        for (int p = 0; p < numprocs; p++) {
            if (p != procid) {
                MPI_Test(&endroundreqs[p], &endroundflags[p], &endroundstats[p]);
                MPI_Send(reqbuf, 1, MPI_INT, p, 2, MPI_COMM_WORLD); // broadcast round completion
            } //end if p != procid
            sum += endroundflags[p];
        }
        
        while (sum < numprocs) {
            // keep receiving and handling messages
            flag = 0;
            MPI_Irecv(recvreqbuf, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &rrequest);
            MPI_Test(&rrequest, &flag, &rstat);
            if (flag) { // respond to request
//                fprintf(stderr, "received req msg from proc %i tag %i node %i\n", rstat.MPI_SOURCE, rstat.MPI_TAG, *recvreqbuf);
                source = rstat.MPI_SOURCE;
                neighID = *recvreqbuf;
                ncred = ROUNDS[i - 1][neighID];
                senddatabuf->ID = neighID;
                senddatabuf->cred = ncred;
//                fprintf(stderr, "responding to proc %i wit node %i and cred %f\n", source, senddatabuf->ID, senddatabuf-> cred);
                MPI_Send(senddatabuf, 1, data, source, 1, MPI_COMM_WORLD);
                MPI_Irecv(recvreqbuf, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &rrequest);
            } // end if flag
            
            // test whether round is complete (perhaps re-broadcast here if necessary??
            sum = 0;
            for (int p = 0; p < numprocs; p++) {
                if (p != procid) {
                    MPI_Test(&endroundreqs[p], &endroundflags[p], &endroundstats[p]);
                    //MPI_Send(reqbuf, 1, MPI_INT, p, 2, MPI_COMM_WORLD); // broadcast round completion
                } //end if p != procid
                sum += endroundflags[p];
            }

        } //end while rounds not complete
        fprintf(stderr, "Proc %i: %i out of %i processes have completed round %i\n", procid, sum, numprocs, i);
        end = clock();
        elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
        printf("Round %i: %f seconds\n", i, elapsed);
    }//endfor i

    // WRITE OUTPUT
    char *outputfile = malloc(strlen("output.txt") + strlen("1") + 1);
    char *strid = malloc(8);
    sprintf(strid, "%d", procid);
    strcpy(outputfile, strid);
    strcat(outputfile, "_output.txt");
    
    start = clock();
    FILE *output;
    output = fopen(outputfile, "w");
    if (output == NULL) {   
        printf("Error: Could not open output file for writing.\n"); 
        exit(-1); // must include stdlib.h 
    } 

    for (int n = 0; n <= MAXID; n++) {
        if (COUNTS[n]) {
            fprintf(output, "%i\t", n);
            fprintf(output, "%i\t", COUNTS[n]);
            for (int i = 0; i < numrounds; i++) {
                fprintf(output, "%f\t", ROUNDS[i][n]);
            } //endfor i
            fprintf(output, "\n");
        } // endif
    } //endfor n
    fclose(output);
    end = clock();
    elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time to write: %f seconds\n", elapsed);

    ierr = MPI_Finalize();
    return 0;
}
