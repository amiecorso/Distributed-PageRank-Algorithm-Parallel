// part.c
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

long MAXNODES = 5000000000;
long MAXID = 0;
int *COUNTS; 
double *RECIP;
double *CREDIT;
int **NEIGHBS;
int MAXNODE = 0; // largest count of neighbors

int main(int argc, char const *argv[]) {
    COUNTS = calloc(MAXNODES, sizeof(int));
    unsigned char *f;
    int size;
    struct stat s;
    int fd = open (argv[1], O_RDONLY);

    /* Get the size of the file. */
    int status = fstat (fd, & s);
    size = s.st_size;

    f = (char *) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (!f) {
        return 1;
    }
    char *numbuf = malloc(20); // make an array for storing characters from file
    int index = 0; // for filling our numbuf
    int a, b;
    int firstspace = 1;
    for (int i = 0; i < size; i++) {
        char c;
        c = f[i];
        //putchar(c);
        if (c == ' ') {
            if (firstspace) {
                numbuf[index] = '\0'; //terminate a
                a = atoi(numbuf);     // read it as an int
                printf("a: %i   ", a);
                index = 0;
                firstspace = 0;
                continue;
            }
            else
                continue;
        }
        if (c == '\n') {
            numbuf[index] = '\0';
            b = atoi(numbuf);
            printf("b: %i   \n", b);
            index = 0;
            firstspace = 1;
            // Count number of neighbors for each node 
            COUNTS[a] += 1;
            COUNTS[b] += 1;
            if (a > MAXID) MAXID = a;
            if (b > MAXID) MAXID = b;
            continue;
        }
        numbuf[index] = c;
        ++index;
        firstspace = 1;
    }
    // FIND MAX NODE
    for (int i = 0; i < MAXID; i++) {
        if (COUNTS[i] > MAXNODE) MAXNODE = COUNTS[i];
    } //endfor
    printf("most neighbors: %i\n", MAXNODE);
    // CREATE RECIPROCAL ARRAY
    RECIP = calloc(MAXID, sizeof(int));
    int count;
    for (int i = 0; i < MAXID; i++) {
        count = COUNTS[i];
        if (count) RECIP[i] = 1 / count;
    } //endfor

    // INITIALIZE CREDIT
    CREDIT = malloc((MAXID + 1) * sizeof(double));
    for (int i = 0; i <= MAXID; i++) {
        CREDIT[i] = 1.0; 
    }

    // ALLOCATE 2-D array
    NEIGHBS = malloc((MAXID + 1) * sizeof(int *));
    for (int i = 0; i <= MAXID; i++) {
        NEIGHBS[i] = malloc((MAXNODE + 1) * sizeof(int));
        NEIGHBS[i][0] = 1; // INDEX info
    }

    index = 0;
    int nextindex;
    for (int i = 0; i < size; i++) {
        char c;
        c = f[i];
        //putchar(c);
        if (c == ' ') {
            if (firstspace) {
                numbuf[index] = '\0'; //terminate a
                a = atoi(numbuf);     // read it as an int
                //printf("a: %i", a);
                index = 0;
                firstspace = 0;
                continue;
            }
            else
                continue;
        }
        if (c == '\n') {
            numbuf[index] = '\0';
            b = atoi(numbuf);
            //printf("b: %i   \n", b);
            index = 0;
            firstspace = 1;
            // handle a
            nextindex = NEIGHBS[a][0];
            //printf("nextindex: %i\n", nextindex);
            NEIGHBS[a][nextindex] = b;
            NEIGHBS[a][0] = nextindex + 1;
            // handle b
            nextindex = NEIGHBS[b][0];
            NEIGHBS[b][nextindex] = a;
            NEIGHBS[b][0] = nextindex + 1;
            continue;
        }
        numbuf[index] = c;
        ++index;
        firstspace = 1;
    }
    // PERFORM ROUNDS
    int numrounds = atoi(argv[2]);
    double **ROUNDS = malloc(numrounds * sizeof(double *));
    for (int i = 0; i < numrounds; i++) { // round
        ROUNDS[i] = malloc((MAXID + 1) * sizeof(double));
        for (int n = 0; n <= MAXID; n++) { // node
            double newcred = 0.0; // new credit for this node, this round
            int neighbcount = COUNTS[n];
            if (neighbcount) { // only need to perform update if this node has neighbors - i.e. if this node ID exists
                for (int neighindex = 1; neighindex <= neighbcount; neighindex++) {
                    int neighbor = NEIGHBS[n][neighindex];
                    newcred += CREDIT[neighbor] * RECIP[neighbor];
                } //endfor neighindex
                ROUNDS[i][n] = newcred;
            } //endif
        }//endfor n
        // copy into array
        for (int j = 0; j <= MAXID; j++) {
            CREDIT[j] = ROUNDS[i][j];
        } 
    }//endfor i

    // PRINT ROUND RESULTS


    // TODO: write to file
    return 0;

}
