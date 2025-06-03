#ifndef EXPANSION_TREE_H
#define EXPANSION_TREE_H

#include <vector>
#include <iostream>
#include <cassert>
#include <deque>
using namespace std;

struct Node {
    int Depth;
    int Parent;
    int Idx;
    int PosInNodes;
    std::vector<int> Successors;
    int PointerToGame;
    bool IsLeaf;
    bool IsRoot;
    int Block;
    int PosInMove;
    int PosInLeaves;
    bool Considered;

    void Print(bool print_anyway=false);
    Node(int block);
    Node(int & parent, int pos, int p2g, std::vector <Node> & nodes);
    void SetPointerToGame(int & p);

};

struct ExpansionTree {
    std::vector <Node> Nodes;
    std::vector <int> Leaves;
    std::vector <std::vector <int> > NodesByBlock;
    std::vector <int> NumPosPerDepth;
    int MaxDepth;
    deque <int> PathAbove;

    ExpansionTree(int block);
    ExpansionTree(const ExpansionTree& et);
    void SetPathAbove(deque <int> tracker);
    int Expand(int & n, int p2g);
    deque<int> RetrieveFullPath(const int & n) const;
    deque<int> RetrieveCurrentPath(const int & n) const;
    deque<int> RetrieveCurrentPathOfNodes(const int & n) const;
    deque<std::pair < int,bool >  > RetrieveCurrentPathNoDoubles(const int & n);
    int RetrievePredecessor(const int n, int lev);
    deque<int> RetrieveMovePositions(const int n) const;
    void ResetConsidered();
    void SetConsidered(const int & n);
    void Print(bool printAnyway=false);

};
#endif
