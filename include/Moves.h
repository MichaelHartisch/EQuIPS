#ifndef MOVES_H
#define MOVES_H

#include <vector>
#include <iostream>
#include <cassert>
#include <deque>

#include "ExpansionTree.h"
#include "Helper.h"
#include "MultiGame.h"

struct MultiGame;

using namespace std;

struct Assignment{
	int idx;
    int val;

    Assignment();
    Assignment(int index, int value);
    void Print(bool print_anyway=false);
};

struct PartialAssignment{
    int LinkerNode;
    std::vector < Assignment > SubGame;
    int Block; 
    bool IsInOrg;

    PartialAssignment(int b);

    PartialAssignment();

    void SetLinker(int & et_node);
    void Add(const int & idx, const int &val);
    void Print(ExpansionTree & et, bool print_anyway);
};

class Move{
    public:
    Quantifier Q;
    vector < vector < PartialAssignment > > Assignments;
    int MinBlock;
    int MaxBlock;
    vector < int > Blocks;
    bool IsNULL;

    Move(Quantifier q);
    Move(Quantifier q, int b);
    Move(Quantifier &  q, int b_min,int b_max);
    Move(MultiGame * g);
    Move(Move* m);
    void Add(const Move& mu, int link);
    int ExtractLinkerNode();
    Move Copy();

    int ExtractLastBlock() const;

    void Transform(ExpansionTree & tree);
    void Add(PartialAssignment pa, const int& block);
    void Print(ExpansionTree & et, bool print_anyway);
};
#endif
