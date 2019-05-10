// Amie Corso
// CIS 630 Project 1
// part2.c
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


long MAXNODES = 2000000;
long MAXID = 0;
int *COUNTS; 
int *PART;
int *EXNEIGH; //does this node have EXTERNAL neighbors?
double *RECIP;
int **NEIGHBS;
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
            if (n > MAXID) MAXID = n;
            index = 0;
        }//endif
        else {
            numbuf[index] = c;
            ++index;
        }
    }

    // ALLOCATE 2-D array ======================
    fprintf(stderr, "PROC %i, MAXID = %li\n", procid, MAXID);
    NEIGHBS = malloc((MAXID + 1) * sizeof(int *));
    for (int i = 0; i <= MAXID; i++) {
//        if (COUNTS[i] && (PART[i] == procid)) {
            NEIGHBS[i] = malloc((COUNTS[i] + 1) * sizeof(int));
            NEIGHBS[i][0] = 1; // INDEX info
//        }
    }
    EXNEIGH = calloc(MAXID + 1, sizeof(int));

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
            // populate neighbor arrays (ONLY for MY nodes)
//            fprintf(stderr, "proc %i: a=%i, PART[a]=%i\n", procid, a, PART[a]);
//            if (PART[a] == procid) {
//                fprintf(stderr, "proc %i: adding neighbor b=%i for node a=%i\n", procid, b, a);
                nextindex = NEIGHBS[a][0];
                NEIGHBS[a][nextindex] = b;
                NEIGHBS[a][0] = nextindex + 1;
                if (PART[b] != procid) EXNEIGH[a] = 1;
//            }
//            fprintf(stderr, "proc %i: b=%i, PART[b]=%i\n", procid, b, PART[b]);
//            if (PART[b] == procid) {
//                fprintf(stderr, "proc %i: adding neighbor a=%i for node b=%i\n", procid, a, b);
                nextindex = NEIGHBS[b][0];
                NEIGHBS[b][nextindex] = a;
                NEIGHBS[b][0] = nextindex + 1;
                if (PART[a] != procid) EXNEIGH[b] = 1;
//            }
        }
        else {
            newbuf[index] = c;
            ++index;
        }
    }
    end = clock();
    elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Proc %i: Time to read: %f seconds\n", procid, elapsed);

    // CREATE RECIPROCAL ARRAY ============
    RECIP = calloc(MAXID + 1, sizeof(double));
    int cnt;
    for (int i = 0; i <= MAXID; i++) {
        cnt = COUNTS[i];
        if (cnt > 0) {
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
                if (EXNEIGH[n] && (PART[n] == procid)) {
                    for (int proc = 0; proc < numprocs; proc++) {
                        if (proc != procid) {
                            sendbuffer[n].ID = n;
                            sendbuffer[n].cred = ROUNDS[i - 1][n];
//                            fprintf(stderr, "proc %i, REAL n=%i, REAL cred=%f\n", procid, n, ROUNDS[i - 1][n]);
//                            fprintf(stderr, "proc %i, sending for node=%i, cred=%f\n", procid, (sendbuffer + n)->ID, (sendbuffer + n)->cred);
                            //MPI_Isend(sendbuffer + n, 1, data, proc, 0, MPI_COMM_WORLD, srequest); 
                            MPI_Send(sendbuffer + n, 1, data, proc, 0, MPI_COMM_WORLD); 
                        } // don't send to self!
                    } //end for proc
                } //end if this my, valid node
            } //end for n (send phase)
            //MPI_Barrier(MPI_COMM_WORLD);
            //sleep(10);
            for (int proc = 0; proc < numprocs; proc++) {
                if (proc != procid)
                    //MPI_Isend(markerbuffer, 1, MPI_INT, proc, 1, MPI_COMM_WORLD, srequest);
                    MPI_Send(markerbuffer, 1, MPI_INT, proc, 1, MPI_COMM_WORLD);
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
                    //fprintf(stderr, "R%i, proc %i received msg for node=%i, cred=%f, PART[n]=%i\n", i, procid, recvbuffer[0].ID, recvbuffer[0].cred, PART[recvbuffer[0].ID]);
                    ROUNDS[i - 1][recvbuffer[0].ID] = recvbuffer[0].cred;
                } // tag = 0
                else { //assuming tag = 1
                    if (rstat.MPI_TAG == 1) sum += 1;
                }
            } //endwhile
        } // end if round > 1

        // CALC PHASE
        ROUNDS[i] = calloc(MAXID + 1, sizeof(double));
        for (int n = 0; n <= MAXID; n++) { // node
            if (COUNTS[n] && (PART[n] == procid)) { // IF this is MY node
//            if (COUNTS[n]) {
                double newcred = 0.0; // new credit for this node, this round
                for (int neighindex = 1; neighindex <= COUNTS[n]; neighindex++) {
                    int neighbor = NEIGHBS[n][neighindex];
                    double neighcred = ROUNDS[i - 1][neighbor];
//                    if (n == 3)
//node3                    fprintf(stderr, "R%i, node:%i, neighbor:%i, oldcred:%f, recip:%f (degree=%i, 1/deg=%f)\n", i, n, neighbor, neighcred, RECIP[neighbor], COUNTS[neighbor], 1.0/COUNTS[neighbor]);
//                    newcred += neighcred * RECIP[neighbor];
                    newcred += neighcred / COUNTS[neighbor];
                } //endfor neighindex
                ROUNDS[i][n] = newcred;
            } //end if my, valid node
        }//endfor n
        end = clock();
        elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
        printf("Round %i: %f seconds\n", i, elapsed);
        MPI_Barrier(MPI_COMM_WORLD);
/*
        // WRITE ROUND
        char *outputfile = malloc(strlen("output.txt") + strlen("R1P1") + 1);
        char *strid = malloc(8);
        char *rID = malloc(8);
        sprintf(rID, "R%i", i);
        sprintf(strid, "P%d", procid);
        strcpy(outputfile, rID);
        strcat(outputfile, strid);
        strcat(outputfile, "_output.txt");
        FILE *output;
        output = fopen(outputfile, "w");
        if (output == NULL) {   
            printf("Error: Could not open output file for writing.\n"); 
            exit(-1); // must include stdlib.h 
        } 

        for (int n = 0; n <= MAXID; n++) {
            if (COUNTS[n]) {
                fprintf(output, "%i\t\t", n);
                fprintf(output, "%i\t\t", COUNTS[n]);
                fprintf(output, "%i\t", PART[n]);
                fprintf(output, "%f\n", ROUNDS[i][n]);
            } // endif
        } //endfor n
        fclose(output);
*/
    }//endfor i
    // WRITE OUTPUT
    char *outputfile = malloc(strlen("Partition0.out") + 1);
    sprintf(outputfile, "Partition%i.out", procid);
    
    start = clock();
    FILE *output;
    output = fopen(outputfile, "w");
    if (output == NULL) {   
        printf("Error: Could not open output file for writing.\n"); 
        exit(-1); // must include stdlib.h 
    } 

    for (int n = 0; n <= MAXID; n++) {
//        if (COUNTS[n] && (PART[n] == procid)) {
        if (COUNTS[n]){
            fprintf(output, "%i\t", n);
            fprintf(output, "%i\t", COUNTS[n]);
            fprintf(output, "%i\t", PART[n]);
            if (PART[n] == procid) {
            for (int i = 0; i <= numrounds; i++) {
                fprintf(output, "%f\t", ROUNDS[i][n]);
            }
            } //endif
            else{
            for (int i = 0; i <= numrounds; i++) {
                fprintf(output, "-\t");
            } //end else
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
