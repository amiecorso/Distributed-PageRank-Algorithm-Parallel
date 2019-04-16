// CIS 630 Project 1 Part 1

#include <iostream>
#include <fstream>
#include <chrono>
#include <map>
#include <vector>
#include <string>
#include <stdio.h>      /* printf, fopen */
#include <stdlib.h>

using namespace std;

map<int, vector<int> > MAP;
typedef map<int, vector<int> >::const_iterator MapIterator;
typedef vector<int>::const_iterator VecIterator;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cerr << "Error: Not enough arguments" << endl;
        exit(EXIT_FAILURE);
    }
    // start timer
    auto start = chrono::high_resolution_clock::now();
    ifstream infile(argv[1]);
    int a, b;
    while (infile >> a >> b) {
        MAP[a].push_back(b);
    }
    // print our map
    for (MapIterator iter = MAP.begin(); iter != MAP.end(); iter++)
    {
        cout << "Key: " << iter->first << endl << "Values:" << endl;
        for (VecIterator vec_iter = iter->second.begin(); vec_iter != iter->second.end(); vec_iter++)
            cout << " " << *vec_iter << endl;
    }
    // stop timer
    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::seconds>(stop - start); 

    cout << "Time to read input file = " << duration.count() << "sec" << endl; 
    infile.close();
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
