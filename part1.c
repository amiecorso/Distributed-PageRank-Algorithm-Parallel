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

int main(int argc, char const *argv[])
{
    unsigned char *f;
    int size;
    struct stat s;
    int fd = open (argv[1], O_RDONLY);

    /* Get the size of the file. */
    int status = fstat (fd, & s);
    size = s.st_size;

    f = (char *) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
    for (int i = 0; i < size; i++) {
        char c;

        c = f[i];
        putchar(c);
    }
//    for (int i = 0; i < 100; i++) {
//        printf("%s", f[i]);
//    }

    return 0;
}

