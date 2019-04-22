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
/*
int main(int argc, char *argv[]) {
    FILE *f_in;
	f_in = fopen(argv[1], "r");
    if (!f_in) {
        return 1;
    }
    buffer = malloc(100);
	fread(buffer, width*height*sizeof(Pixel), 1, f_in);
	fclose(f_in);
}
*/
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
            // TODO: we now have a and b! do things with them (count, store neighbors)
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

    // ALLOCATE 2-D array
    NEIGHBS = malloc(MAXID * sizeof(int *));
    for (int i = 0; i < MAXID; i++) {
        NEIGHBS[i] = malloc(MAXNODE * sizeof(int));
    }
    

    return 0;
}
