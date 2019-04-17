// CIS 630 Project 1 Part 1

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <stdio.h>      /* printf, fopen */
#include <stdlib.h>

using namespace std;

void add_edge(int n1, int n2, unordered_map<int, vector<int> > &adj) {
    unordered_map<int, vector<int> >::iterator iter;
    iter = adj.find(n1);
    if (iter != adj.end()) { // if the entry already exists in our table
        iter->second.push_back(n2); // then add the neighbor to it
    }
    else {
        adj[n1].push_back(n2);
    }
};

void one_round(unordered_map<int, double> &ranks, unordered_map<int, double> &newranks, unordered_map<int, vector<int> > &adj) {
    unordered_map<int, vector<int> >::iterator iter;
    iter = adj.begin();
    while (iter != adj.end()) {
        vector<int> neighbors = iter->second;
        double newrank = 0;
        for (int i = 0; i < neighbors.size(); i++) {
            int neighbor = neighbors[i];
            double nrank = ranks[neighbor];
            double degree = adj[neighbor].size();
            newrank += nrank / degree;
        }//endfor
        newranks.insert(make_pair(iter->first, newrank));
        ++iter;
    }//endwhile
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
    // read file into hash table
    unordered_map<int, vector<int> > adj;
    ifstream infile(argv[1]);
    auto start = chrono::high_resolution_clock::now();
    if (infile.is_open()) {
        // start timer
        int a, b;
        while (infile >> a >> b) {
            add_edge(a, b, adj);
            add_edge(b, a, adj);
        }
    }
    infile.close();
    // stop timer
    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::seconds>(stop - start); 
    cout << "Time to read input file = " << duration.count() << "sec" << endl; 
    
    // initialize ranks (to 1)
    unordered_map<int, double> ranks;
    unordered_map<int,vector<int> >::iterator iter = adj.begin();
	while(iter != adj.end()) {
		ranks.insert(make_pair(iter->first, 1.));
		++iter;
	}

    // iterate for number of rounds
    for (int i = 0; i < rounds; i++) {
        auto start = chrono::high_resolution_clock::now();
        unordered_map<int, double> roundranks;
        one_round(ranks, roundranks, adj);
        auto stop = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::seconds>(stop - start); 
        cout << "Time for round " << i << ": " << duration.count() << "sec" << endl; 
        ranks = roundranks;
    }
    return 0;
}
