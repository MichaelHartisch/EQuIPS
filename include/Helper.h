#ifndef HELPER_H
#define HELPER_H

#include <vector>
#include <iostream>
#include <cassert>
#include <deque>
#include <math.h>
using namespace std;

static void PrintDeque(std::deque <int> vec, bool print){
    if(!print) return;
    for (int i=0;i<vec.size(); i++ ){
        cerr << vec[i];
        if(i !=vec.size()-1) cerr << ",";
    }
    cerr << endl;
}

static void PrintVec(std::vector <int> vec, bool print){
    if(!print) return;
    for (int i=0;i<vec.size(); i++ ){
        cerr << vec[i];
        if(i !=vec.size()-1) cerr << ",";
    }
    cerr << endl;
}

static string Copy(const string& str, const deque < int >& link, const int& lim){
    //cerr <<"We copy "<< str << " with lim= " << lim << endl;
    string ReturnString=str;
    //cerr << "The deque: ";
    //PrintDeque(link);
    for (int i=1;i<= (lim<(link.size()-1)?lim:(link.size()-1));i++)
        ReturnString += "_" + to_string(link[i]);
    return ReturnString;
}

static string CopyNoDouble(const string& str, const deque < std::pair < int, bool> > & link, const int& lim){
    //cerr <<"We copy "<< str << " with lim= " << lim << endl;
    string ReturnString=str;
    //if (link.empty()||link.size()==1)
    //    return ReturnString;
    for (int i=1;i<= (lim<(link.size()-1)?lim:(link.size()-1));i++)
        ReturnString += "_" + to_string(link[i].first);
    return ReturnString;
}

static bool IsInteger(double val){
    if (abs(val-floor(val+.5))<1e-20)
        return true;
    else return false;
}

static int ApproximateDenominatorPowerTo10(double decimal){
    int counter=0;
    double helper=decimal;
    while (!IsInteger(helper)){
        helper*=10;
        counter++;
    }
    return counter;
}

enum Quantifier { UNIVERSAL = 0, EXISTENTIAL = 1 };
inline Quantifier Negate(Quantifier q) {
    if (q==UNIVERSAL) return EXISTENTIAL;
    else return UNIVERSAL;
}
#endif
