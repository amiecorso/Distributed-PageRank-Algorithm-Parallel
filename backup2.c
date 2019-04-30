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
    MPI_Status rstat;
    int flag = 0;
    int source;
    int neighID;
    double ncred;

    for (int i = 1; i <= numrounds; i++) { // round
fprintf(stderr, "round %i\n", i);
        start = clock();
        ROUNDS[i] = calloc(MAXID + 1, sizeof(double));
        for (int n = 0; n <= MAXID; n++) { // node
            if (PART[n] == procid) { // IF this is MY node
                double newcred = 0.0; // new credit for this node, this round
                int neighbcount = COUNTS[n];
                if (neighbcount) { // only need to perform update if this node has neighbors - i.e. if this node ID exists
                    for (int neighindex = 1; neighindex <= neighbcount; neighindex++) {
                        int neighbor = NEIGHBS[n][neighindex];
//                        fprintf(stderr, "Round %i, neighbor %i\n", i, neighbor);
                        double neighcred = ROUNDS[i - 1][neighbor];
  //                      fprintf(stderr, "neighcred = %f\n", neighcred);
                        if (neighcred) { // if we already have a value for this neighbor, go for it
 //                           fprintf(stderr, "entering if\n");
                            newcred += neighcred * RECIP[neighbor];
                        } //end if neighcred
// put me here
                    } //endfor neighindex
//                    fprintf(stderr, "updating node %i credit to %f for round %i\n", n, newcred, i);
                    ROUNDS[i][n] = newcred;
                } //endif neighbcount
            } //end if my node
        }//endfor n
        end = clock();
        elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
        printf("Round %i: %f seconds\n", i, elapsed);
        // TODO: serve pending requests for this round until ALL nodes have completed this round (BARRIER)
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
/*
                        else {
                            fprintf(stderr, "entering else \n");
                            fprintf(stderr, "proc %i need to get neighbor %i\n", procid, neighbor);
                            while (ROUNDS[i - 1][neighbor] == 0.0) {
                                int extern_proc = PART[neighbor];
                                reqbuf[0] = neighbor;
                                fprintf(stderr, "proc %i sending request for neigh %i to proc %i\n", procid, neighbor, extern_proc);
                                MPI_Send(reqbuf, 1, MPI_INT, extern_proc, 0, MPI_COMM_WORLD);
                                start = clock();
                                elapsed = 0;
                                flag = 0;
                                MPI_Irecv(recvdatabuf, 1, data, extern_proc, 1, MPI_COMM_WORLD, &rrequest);
                                while ((elapsed < TIMEOUT) && !flag) {
                                    fprintf(stderr, "proc %i waiting for response for neigh %i\n", procid, neighbor);
                                    MPI_Test(&rrequest, &flag, &rstat);
                                    if (flag) {
                                        ROUNDS[i - 1][recvdatabuf[0].ID] = recvdatabuf[0].cred;
                                        printf("proc %i received message with tag: %i from proc %i\n", procid, rstat.MPI_TAG, rstat.MPI_SOURCE);
                                        break; // kill our loop
                                    } //end if flag
                                    elapsed = ((double) (clock() - start)) / (CLOCKS_PER_SEC / 1000);
                                } //end timeout while
                                printf("proc %i timed out waiting for response for neighbor %i\n", procid, neighbor);
                                // serve pending requests after timeout
                                MPI_Irecv(recvreqbuf, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &rrequest);
                                MPI_Test(&rrequest, &flag, &rstat);
                                if (flag) { // respond to request
                                   source = rstat.MPI_SOURCE;
                                   neighID = recvreqbuf[0];
                                   ncred = ROUNDS[i - 1][neighID];
                                   senddatabuf->ID = neighID;
                                   senddatabuf->cred = ncred;
                                   MPI_Send(senddatabuf, 1, data, source, 1, MPI_COMM_WORLD);
                                } // endif flag
                            } // end while still need neighbor...
                            newcred += ROUNDS[i - 1][neighbor] * RECIP[neighbor];
                        } // end else
                        // serve pending requests before next neighbor
                        flag = 0;
                        MPI_Irecv(recvreqbuf, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &rrequest);
                        MPI_Test(&rrequest, &flag, &rstat);
                        fprintf(stderr, "made it to line %i\n", __LINE__);
                        while (flag) {
                            fprintf(stderr, "inside while flag\n");
                            source = rstat.MPI_SOURCE;
                            neighID = recvreqbuf[0];
                            ncred = ROUNDS[i - 1][neighID];
                            senddatabuf->ID = neighID;
                            senddatabuf->cred = ncred;
                            MPI_Send(senddatabuf, 1, data, source, 1, MPI_COMM_WORLD);
                            flag = 0;
                            MPI_Irecv(recvreqbuf, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &rrequest);
                            MPI_Test(&rrequest, &flag, &rstat);
                        } //end while flag
*/
