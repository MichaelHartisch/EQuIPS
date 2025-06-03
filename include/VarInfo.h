#ifndef DATASTRUCTURES_VARINFO_H
#define DATASTRUCTURES_VARINFO_H

#include <vector>
#include <iostream>
#include <cassert>
#include <deque>
using namespace std;

class VariableInfo{
public:
	string Name;
	bool Existential;
    bool Universal;
	int Idx;
    int Block;
    int GRBNum;
    int LB;
    int UB;

    VariableInfo(string name, bool existential, bool universal, int numInOrder, int block);
    void PrintVariableInfo();
    void SetBounds(int lb, int ub);
    void SetGRBNum(int num);
    void Print();
};
#endif
