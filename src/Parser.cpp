#include "Parser.h"

bool Parser::ContainsKeyword(const std::string &line, const std::vector<std::string> &keywords) {
    for (const std::string &kw : keywords) {
        if (line.find(kw) != std::string::npos) {
            return true;
        }
    }
    return false;
}

Parser::Parser(Parser& prs){
    this->TempFilename=prs.TempFilename;
    this->OrgFile=prs.OrgFile;
}

void Parser::CreateTemporaryLPFile(){

    std::ifstream inFile(OrgFile);
    if (!inFile) {
        std::cerr << "Error: Could not open " << OrgFile <<" for reading." << std::endl;
        return;
    }
    
    this->TempFilename = OrgFile+".lp";

    std::ofstream outFile(TempFilename);
    if (!outFile) {
        std::cerr << "Error: Could not open " << TempFilename << " for writing." << std::endl;
        return;
    }

    std::string line;
    bool inStandardSection = true; 
    
    while (std::getline(inFile, line)) {
        if (inStandardSection) {
            // Check if the line contains one of the keywords we want to omit
            if (ContainsKeyword(line, OmitKeywords)) 
                inStandardSection = false;
            else 
                outFile << line << "\n";
        } else {
            if (ContainsKeyword(line, IncludeKeywords)){
                inStandardSection = true;
                outFile << line << "\n";
            }
        }
    }
    
    inFile.close();
    outFile.close();
}

void Parser::RemoveTemporaryFile(){
    if (std::remove(TempFilename.c_str()) != 0) {
        std::cerr << "Warning: Could not delete temporary file: " << TempFilename << "\n";
    }
}
void Parser::AddConstraintFromString(GRBModel& unc_model, const string& constraint) {
    stringstream ss(constraint);
    string token;
    GRBLinExpr expr = 0;
    char sense;
    double rhs;
    bool JustStarted=true;
    while (ss >> token) {
        double coeff = 1.0; 
        string varName;

        if (token == "<=" || token == ">=" || token == "=") {
            sense = token[0];  // Store the constraint type ('<=' or '>=' or '=')
            break;             // Stop parsing when operator is found
        }
        else{
            assert(token[0] == '-' || token[0] == '+' || JustStarted);
            if (token[0] == '-') coeff=-1.0;
            else if (token[0] == '+') coeff=+1.0;
            else coeff=+1.0;
            JustStarted=false;
            ss >> token;
            if (isdigit(token[0])) {
                coeff = coeff*stod(token);
                ss >> token;
            }

            varName = token;

            try {
                GRBVar var = unc_model.getVarByName(varName);
                expr += coeff * var;  // Add term to the linear expression

            } catch (GRBException& e) {
                cerr << "Error: Variable " << varName << " not found in the model!" << endl;
                return;
            }
        }
    }

    // Read the RHS value
    ss >> rhs;

    // Add constraint to the model
    if (sense == '>') {
        unc_model.addConstr(expr >= rhs);
    } else if (sense == '<') {
        unc_model.addConstr(expr <= rhs);
    } else {
        unc_model.addConstr(expr == rhs);
    }
}

void Parser::CreateUncertaintyModel(GRBModel& unc_model){
    if(PRINT) cout << "Before removing constraints, there are " << unc_model.get(GRB_IntAttr_NumConstrs) << " constraints"<<endl;
    int numConstrs = unc_model.get(GRB_IntAttr_NumConstrs);
    GRBConstr* constraints = unc_model.getConstrs();
    for (int i = 0; i < numConstrs; ++i) {
        unc_model.remove(constraints[i]); // Remove each constraint
    }
    unc_model.setObjective(GRBLinExpr(0.0), GRB_MINIMIZE);
    unc_model.update();
    if(PRINT) cout << "After removing constraints, there are " << unc_model.get(GRB_IntAttr_NumConstrs) << " constraints left"<<endl;
    std::ifstream file(OrgFile); // Open the file
    if (!file) {
        std::cerr << "Error: Unable to open file " << OrgFile << std::endl;
        exit(0);
    }

    std::string line;
    bool startCollecting = false;
    if(PRINT) std::cout << "Constraints of the uncertainty set:" << std::endl; // Print the line

    while (std::getline(file, line)) {
        if (!startCollecting) {
            // Check if the key phrase is found
            if (line.find("UNCERTAINTY SUBJECT TO") != std::string::npos) {
                startCollecting = true;
            }
        } else {
            // Stop printing if any of the stopping keywords appear
            if ( ContainsKeyword(line, OmitKeywords) ||  ContainsKeyword(line, IncludeKeywords)){
                break;
            }
            AddConstraintFromString(unc_model,line);
            if(PRINT) std::cout << line << std::endl; // Print the line
        }
    }
    unc_model.update();

    
    file.close(); // Close the file DictGRBToIdx, DictIdxToGRB
}
void Parser::ParseQuantificationInfo(const GRBModel& main_model,std::vector <VariableInfo>& vars_info,
        std::vector <Quantifier>& quantification,  std::vector <int>& quantification_delimiter,
        std::vector < std::vector <VariableInfo> >& block_vars, std::vector <int>& dict_g_2_i, std::vector <int>& dict_i_2_g){
    //extract variable order from file
    std::ifstream file(OrgFile);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << OrgFile << std::endl;
    }
    std::string line;
    std::vector<std::string> collectedOrder;
    std::vector<std::string> collectedExists;
    std::vector<std::string> collectedUniv;
    bool collectOrder = false;   
    bool collectUniv = false;
    bool collectExist = false;  
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string word;

        while (iss >> word) {
            if (word == "ORDER") {
                collectOrder = true; // Start collecting variables after keyword "ORDER"
                collectUniv = false;
                collectExist = false; 
                continue;
            }
            if (word == "EXISTS") {
                collectOrder = false; 
                collectUniv = false;
                collectExist = true;  // Start collecting existential variables after keyword "EXISTS"
                continue;
            }
            if (word == "ALL") {
                collectOrder = false; 
                collectUniv = true; // Start collecting universal variables after keyword "ALL"
                collectExist = false;  
                continue;
            }
            if (word == "END" && collectOrder) {
                collectOrder = false; // Stop after collecting the order
                break;
            }
            if (collectOrder) {
                collectedOrder.push_back(word);
                dict_i_2_g.push_back(-1);
            }
            if (collectExist) {
                collectedExists.push_back(word);
                dict_i_2_g.push_back(-1);
            }
            if (collectUniv) {
                collectedUniv.push_back(word);
                dict_i_2_g.push_back(-1);
            }
        }
    }
    file.close();

    // GRBModel was already created when calling this function
        GRBVar* vars = main_model.getVars();

    int block=0;
    bool BlockIsExistential;
    quantification_delimiter.push_back(0);
    std::vector <VariableInfo> NewBlock;
    block_vars.push_back(NewBlock);
    for (int i=0; i<collectedOrder.size();i++) {
        bool IsExist = std::find(collectedExists.begin(), collectedExists.end(), collectedOrder[i]) != collectedExists.end();
        bool IsAll = std::find(collectedUniv.begin(), collectedUniv.end(), collectedOrder[i]) != collectedUniv.end();
        if (i==0){
            if (IsExist){
                BlockIsExistential=true;
                quantification.push_back(EXISTENTIAL);  
            }                
            else{
                BlockIsExistential=false;
                quantification.push_back(UNIVERSAL);
            } 
        }
        else if (quantification.back() != IsExist){
            BlockIsExistential=!BlockIsExistential;
            if (IsExist){
                quantification.push_back(EXISTENTIAL);  
            }                
            else{
                quantification.push_back(UNIVERSAL);
            } 
            block++;
            block_vars.push_back(NewBlock);
            quantification_delimiter.push_back(i);
        }
        vars_info.push_back(VariableInfo(collectedOrder[i],IsExist,IsAll,i,block));
    }
    for(int i=0;i<main_model.get(GRB_IntAttr_NumVars); i++){
        for (int j=0;j<vars_info.size();j++){
            //cout << "vars_info[j].Name="<<vars_info[j].Name<<endl;
            if (vars_info[j].Name==vars[i].get(GRB_StringAttr_VarName)){ 
                assert(i==vars[i].index());
                vars_info[j].SetGRBNum(i);
                dict_g_2_i.push_back(vars_info[j].Idx);
                assert(dict_i_2_g[vars_info[j].Idx]==-1);
                dict_i_2_g[vars_info[j].Idx]=i;
                //cout << "j="<<j<< "; dict_g_2_i.size()=" <<dict_g_2_i.size() << " ; vars_info[j].Idx="<<vars_info[j].Idx<<endl;
                assert(i==dict_g_2_i.size()-1);
                assert(dict_i_2_g[vars_info[j].Idx]==vars_info[j].GRBNum);
                if (vars[i].get(GRB_CharAttr_VType) == 'I' || vars[i].get(GRB_CharAttr_VType) == 'B'){
                    vars_info[j].SetBounds(floor(vars[i].get(GRB_DoubleAttr_LB)), ceil(vars[i].get(GRB_DoubleAttr_UB)));
                }
                else vars_info[j].SetBounds(vars[i].get(GRB_DoubleAttr_LB),vars[i].get(GRB_DoubleAttr_UB));
            }
        }
    }

    for (int i=0; i<vars_info.size();i++)
            block_vars[vars_info[i].Block].push_back(vars_info[i]);

}
