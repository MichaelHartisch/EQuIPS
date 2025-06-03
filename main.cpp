#include <iostream>
#include <chrono>
//#include "gurobi_c++.h"

using namespace std;
using namespace chrono;

int main(int argc, char* argv[]) {

    double Start = clock();
   // GRBEnv Env = GRBEnv(true);
    //GRBModel MainModel(env, argv[1])
    //parse instance

    //call recursive function

    std::cout << "TIME(seconds): "<<(clock() - Start)/CLOCKS_PER_SEC << '\n';
    return 0;
}
