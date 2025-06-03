#ifndef PARSER_H
#define PARSER_H

#include<string>
#include "gurobi_c++.h"
#include "DataStructures.h"
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cassert>
#include <algorithm>

using namespace std;
class Parser{
    public:
    Parser(Parser& prs);
    void ParseQuantificationInfo(const GRBModel& main_model,std::vector <VariableInfo>& my_vars,
    std::vector <Quantifier>& quantification,  std::vector <int>& quantification_delimiter,
    std::vector < std::vector <VariableInfo> >&  , std::vector <int>& dict_g_2_i, std::vector <int>& dict_i_2_g);
    std::string TempFilename;
    void CreateUncertaintyModel(GRBModel& main_model);
    void AddConstraintFromString(GRBModel& unc_model, const string& constraint);
    bool ContainsKeyword(const std::string &line, const std::vector<std::string> &keywords);
    void CreateTemporaryLPFile();
    void RemoveTemporaryFile();
    string OrgFile;
    std::vector<std::string> IncludeKeywords = {"MAXIMIZE", "MINIMIZE", "BOUNDS", "END", "INTEGERS", "GENERALS", "BINARIES", "bounds", "binary"};
    std::vector<std::string> OmitKeywords = {"ORDER", "EXISTS", "ALL", "UNCERTAINTY SUBJECT TO"};
    Parser(const string &filename){
        OrgFile = filename;
    }
};
#endif
