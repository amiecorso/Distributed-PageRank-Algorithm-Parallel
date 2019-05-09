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
    Data *sendbuffer = malloc(sizeof(data) * MAXID);
    Data *markerbuffer = calloc(1, sizeof(data));
    Data *recvbuffer = malloc(sizeof(data));
    MPI_Request *srequest = malloc(sizeof(MPI_Request));
    int sum = 0;
    int tag;
    int src;

    for (int i = 1; i <= numrounds; i++) { // round
        start = clock();
        // SEND PHASE
        if (i > 1) {
            for (int n = 0; n <= MAXID; n++) {
                if (COUNTS[n] && (PART[n] == procid)) {
                    for (int proc = 0; proc < numprocs; proc ++) {
                        if (proc != procid) {
                            sendbuffer[n].ID = n;
                            sendbuffer[n].cred = ROUNDS[i - 1][n];
                            MPI_Isend(sendbuffer + n, 1, data, proc, 0, MPI_COMM_WORLD, srequest); 
                        } // don't send to self!
                    } //end for proc
                } //end if this my, valid node
            } //end for n (send phase)
            for (int proc = 0; proc < numprocs; proc++) {
                if (proc != procid)
                    MPI_Isend(markerbuffer, 1, MPI_INT, proc, 1, MPI_COMM_WORLD, srequest);
            } // end for send markers
        fprintf(stderr, "proc %i finished sending phase\n", procid);

            // RECV PHASE
            int gotmarker[numprocs];
            MPI_Status rstat;
            gotmarker[procid] = 1;
            sum = 1;
            while (sum < numprocs) {
                MPI_Recv(recvbuffer, 1, data, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &rstat);
                if (rstat.MPI_TAG == 0) {
                    ROUNDS[i - 1][recvbuffer[0].ID] = recvbuffer[0].cred;
                } // tag = 0
                else { //assuming tag = 1
                    sum += 1;
                }
            } //endwhile
        } // end if round > 1

        // CALC PHASE
        ROUNDS[i] = calloc(MAXID + 1, sizeof(double));
        for (int n = 0; n <= MAXID; n++) { // node
            if (PART[n] == procid) { // IF this is MY node
                double newcred = 0.0; // new credit for this node, this round
                int neighbcount = COUNTS[n];
                if (neighbcount) { // only need to perform update if this node has neighbors
                    for (int neighindex = 1; neighindex <= neighbcount; neighindex++) {
                        int neighbor = NEIGHBS[n][neighindex];
                        double neighcred = ROUNDS[i - 1][neighbor];
                        newcred += neighcred * RECIP[neighbor];
                    } //endfor neighindex
                    ROUNDS[i][n] = newcred;
                } //endif neighbcount
            } //end if my node
        }//endfor n
        MPI_Barrier(MPI_COMM_WORLD);
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
