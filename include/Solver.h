#ifndef SOLVER_H
#define SOLVER_H

#include <string>
#include "gurobi_c++.h"
#include "MultiGame.h"
#include "Parser.h"
#include "BoundsManager.h"
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cassert>
#include <chrono>

class QISSolver{
    public:
    QISSolver(Parser prs, const chrono::steady_clock::time_point& start, const int& time_limit);


    GRBEnv MainEnv;
    GRBModel MainModel;
    GRBModel UncertaintyModel;

    chrono::steady_clock::time_point Start;
    int TimeLimit;
    int ModelCounter;
    int CurrentBlock;
    int NumConstrs;
    int NumConstrsUncertainty;
    int CounterExistIP;
    int CounterAllIP;
    bool IsOptimizationProblem;
    bool IsMinimizationProblem;
    double ObjLowerBound;
    double ObjUpperBound;
    GRBConstr objConstr;
    GRBConstr* Constrs;
    GRBConstr* ConstrsUncertainty;
    std::vector<double> RHS;
    std::vector<char> Sense;
    std::vector < double > L;
    std::vector < double > U;
    std::vector < double > r;
    Parser Prs;
    VariableBoundsManager UncBoundsManager;
    VariableBoundsManager MainBoundsManager;

    void SetObjectiveBounds(GRBLinExpr row);
    std::vector <VariableInfo> Vars;
    std::vector < std::vector <VariableInfo> > BlockVars;
    std::vector <Quantifier> Quantification;
    std::vector <int> QuantificationDelimiter;
    std::vector <int> DictGRBToIdx;
    std::vector <int> DictIdxToGRB;
    bool Solve();
    void CheckTimeLimit();
    double GetRemainingTime();
    Move RAReQIS(MultiGame & game);
    Move RAReQIS(MultiGame & game, Move stored_move);
    Move Wins(MultiGame & game);
    Move GetScenarioFromMainModel(const std::vector < std::vector <VariableInfo> >& block_info,MultiGame & game);
    Move GetScenarioFromUncertaintySet(const std::vector < std::vector <VariableInfo> >& block_info,MultiGame & game);
    void PrintForDebug();
    void ExtractParameters();
    void ChangeObjRHS(double new_obj);
    void CreateXVariables(GRBModel* model, MultiGame & game);
    void BuildExistentialModel(GRBModel* model, MultiGame & game);
    void BuildUniversalModel(GRBModel* model, MultiGame & game);
    void UpdateAssignedVariablesInUncertaintyModel(MultiGame * game);

};
#endif
