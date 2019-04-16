// CIS 630 Project 1 Part 1

#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <stdio.h>      /* printf, fopen */
#include <stdlib.h>

using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cerr << "Error: Not enough arguments" << endl;
        exit(EXIT_FAILURE);
    }
    // start timer
    auto start = chrono::high_resolution_clock::now();
    fstream f(argv[1], fstream::in );
    string s;
    getline( f, s, '\0');
    // stop timer
    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::seconds>(stop - start); 

    cout << "Time to read input file = " << duration.count() << "sec" << endl; 
    cout << s << endl;
    f.close();
    /*
    FILE * pFile;
    pFile = fopen(argv[1], "r");
    if (pFile==NULL) {
        printf("Error opening file");
        exit (EXIT_FAILURE);
    }
    else {
     // file operations here    
    }
    */
    return 0;
}
