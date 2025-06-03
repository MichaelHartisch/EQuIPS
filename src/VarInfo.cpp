#include "VarInfo.h"

VariableInfo::VariableInfo(string name, bool existential, bool universal, int numInOrder, int block){
    this->Name = name;
    this->Existential = existential;
    this->Universal = universal;
    this->Idx = numInOrder;
    this->Block = block;
}

void VariableInfo::PrintVariableInfo(){
    cerr << Name<< "(";
    if(Existential) cerr<<"E)";
    else cerr<<"U)";
    cerr << " from block "<< Block << " has index " << GRBNum << " and Bounds [" << LB <<","<<UB<<"]"<<endl;
}


void VariableInfo::SetBounds(int lb, int ub){
    this->LB = lb;
    this->UB = ub;
}

void VariableInfo::SetGRBNum(int num){
    GRBNum=num;
}
void VariableInfo::Print(){
    string q="";
    if(Existential) q="E";
    else q="U";
    cout << Name <<"(x"<<Idx<<"): " << q << "("<<Block <<"); GRBNum="<< GRBNum <<endl; 
}
