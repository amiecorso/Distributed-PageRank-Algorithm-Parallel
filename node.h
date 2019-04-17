// node.h
#ifndef NODE
#define NODE

#include <unordered_set>

using namespace std;

class Node {
    public:
        double credit;
        int degree;
        unordered_set<int> neighbors;
    
        Node(void); // default constructor
};

#endif
