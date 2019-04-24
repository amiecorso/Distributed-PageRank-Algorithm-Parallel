// part2.c
// TODO:
// - error checking
// - this partition only needs to record credit/degree/neighbors for ITS partition nodes
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

long MAXNODES = 10000000;
long MAXID = 0;
int *COUNTS; 
int *PART;
double *RECIP;
double *CREDIT;
int **NEIGHBS;

int main(int argc, char const *argv[]) {
    COUNTS = calloc(MAXNODES, sizeof(int));
    PART = calloc(MAXNODES, sizeof(int));

    char *graphf = argv[1];
    char *partf = argv[2];
    int rounds = atoi(argv[3]);
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

    // INITIALIZE CREDIT =================
    CREDIT = malloc((MAXID + 1) * sizeof(double));
    for (int i = 0; i <= MAXID; i++) {
        CREDIT[i] = 1.0; 
    }

    // CREATE RECIPROCAL ARRAY ============
    RECIP = calloc(MAXID + 1, sizeof(double));
    int count;
    for (int i = 0; i <= MAXID; i++) {
        count = COUNTS[i];
        if (count) {
            RECIP[i] = (1.0 / count);
        }
    } //endfor

    // PERFORM ROUNDS =====================
    int numrounds = atoi(argv[2]);
    double **ROUNDS = malloc(numrounds * sizeof(double *));
    for (int i = 0; i < numrounds; i++) { // round
        start = clock();
        ROUNDS[i] = malloc((MAXID + 1) * sizeof(double));
        for (int n = 0; n <= MAXID; n++) { // node
            double newcred = 0.0; // new credit for this node, this round
            int neighbcount = COUNTS[n];
            if (neighbcount) { // only need to perform update if this node has neighbors - i.e. if this node ID exists
                for (int neighindex = 1; neighindex <= neighbcount; neighindex++) {
                    int neighbor = NEIGHBS[n][neighindex];
                    double cred = CREDIT[neighbor];
                    double recip = RECIP[neighbor];
                    newcred += CREDIT[neighbor] * RECIP[neighbor];
                } //endfor neighindex
                ROUNDS[i][n] = newcred;
            } //endif
        }//endfor n
        // copy into array
        for (int j = 0; j <= MAXID; j++) {
            CREDIT[j] = ROUNDS[i][j];
        } 
        end = clock();
        elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
        printf("Round %i: %f seconds\n", i + 1, elapsed);

    }//endfor i


    // WRITE OUTPUT
    start = clock();
    FILE *output;
    output = fopen("output.txt", "w");
    if (output == NULL) {   
        printf("Error: Could not open output file for writing.\n"); 
        exit(-1); // must include stdlib.h 
    } 

    for (int n = 0; n <= MAXID; n++) {
        if (COUNTS[n]) {
            fprintf(output, "%i\t", n);
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
    return 0;
}
