#ifndef MULTIGAME_H
#define MULTIGAME_H

#include <vector>
#include <deque>

#include "DataStructures.h"

struct Move;
struct Assignment;
struct VariableInfo;
struct PartialAssignment;

using namespace std;

struct MultiGame{
    int NumGames;
    int NumBlocks;
    std::vector < int > BlocksInMove;
    Quantifier Q;
    std::vector <Move> Games;
    vector < Assignment > EarlierVars;
    ExpansionTree ExpansionTracker;

    MultiGame(const std::vector <Quantifier>& q_vec);
    MultiGame(const MultiGame& g);
    MultiGame(const MultiGame& g, int scenario, const Move& tao);    

    Move AllLowerBound(const std::vector < std::vector <VariableInfo> >& block_info);
    PartialAssignment AllLowerBound(const std::vector < std::vector <VariableInfo> >& block_info, int block);
    void Refine(Move phi_ell, Move mu);
    bool AllGamesQuantifierFree(const std::vector <Quantifier>& q);
    void Print();
};
#endif
