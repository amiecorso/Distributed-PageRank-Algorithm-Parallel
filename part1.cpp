// CIS 630 Project 1 Part 1

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <map>
#include <unordered_set>
#include <vector>
#include <string>
#include <stdio.h>      /* printf, fopen */
#include <stdlib.h>

using namespace std;

class Node;
typedef unordered_set<int>::iterator SetIterator;
map<int, Node> MAP;
typedef map<int, Node>::iterator MapIterator;


class Node {
    public:
        double credit;
        unordered_set<int> neighbors;
    
        Node(void) { // default constructor
            credit = 1;
        };

        void update_credit(map<int, Node> table) {
            double new_credit = 0;
            for (SetIterator set_iter = neighbors.begin(); set_iter != neighbors.end(); set_iter++) {
                new_credit += MAP[*set_iter].credit / MAP[*set_iter].neighbors.size();
                // cout  << new_credit << endl;
            }
            credit = new_credit;
        };
};

int main(int argc, char *argv[]) {
    //parse args
    if (argc < 3) {
        cerr << "Error: Not enough arguments" << endl;
        exit(EXIT_FAILURE);
    }
    istringstream iss(argv[2]);
    int rounds;
    if (!(iss >> rounds)) {
        cerr << "Error: specify an integer number of rounds" << endl;
    }
    // start timer
    auto start = chrono::high_resolution_clock::now();
    // read file into hash table
    ifstream infile(argv[1]);
    int a, b;
    while (infile >> a >> b) {
        MAP[a].neighbors.insert(b);
        MAP[b].neighbors.insert(a);
    }
    // stop timer
    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::seconds>(stop - start); 
    cout << "Time to read input file = " << duration.count() << "sec" << endl; 
    
    // iterate for number of rounds
    for (int i = 0; i < rounds; i++) {
        auto start = chrono::high_resolution_clock::now();
        for (MapIterator iter = MAP.begin(); iter != MAP.end(); iter++) {
            // cout << "Key: " << iter->first << endl << "new_credit values:" << endl;
            iter->second.update_credit(MAP);
        }
        auto stop = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::seconds>(stop - start); 
        cout << "Time for round " << i << ": " << duration.count() << "sec" << endl; 
    }
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
