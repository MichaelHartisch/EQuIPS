#include <iostream>
#include <chrono>
#include "Solver.h"
#include "Parser.h"
#include "gurobi_c++.h"

using namespace std;
using namespace chrono;

void printUsage(const std::string& programName) {
    std::cout << "Usage: " << programName << " <instance_name> [time_limit]\n"
              << "  instance_name - path to instance; must be in QLP file format \n"
              << "  time_limit    - [optional] time limit in seconds (positive integer)\n"
              << "Options:\n"
              << "  -h, --help    show this help message\n";
}

int main(int argc, char* argv[]) {
    std::string instanceName;
    int timeLimit;
    try {
        // Check for help flag first
        if (argc == 2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
            printUsage(argv[0]);
            return 0;
        }

        // Check the number of arguments
        if (argc > 3) 
            throw std::invalid_argument("Incorrect number of arguments.");

        // Parse and validate arguments
        instanceName = argv[1];
        if(argc == 3)
            timeLimit = std::atoi(argv[2]);
        else timeLimit =std::numeric_limits<int>::max();

        if (timeLimit <= 0) 
            throw std::invalid_argument("Time limit must be a positive integer.");

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        printUsage(argv[0]);
        return 0;
    }

    steady_clock::time_point Start = chrono::steady_clock::now();

    Parser InstanceParser(instanceName);
    InstanceParser.CreateTemporaryLPFile();

    QISSolver Solver(InstanceParser,Start,timeLimit);
    bool Solved=Solver.Solve();

    InstanceParser.RemoveTemporaryFile();

    steady_clock::duration timer = chrono::steady_clock::now() - Start;
    if(!Solved && chrono::duration_cast<milliseconds>(timer).count()/(double)1000 > timeLimit)
    std::cout << "Time: Limit ("<<chrono::duration_cast<milliseconds>(timer).count()/(double)1000 << " seconds) "<<  '\n';
    else 
    std::cout << "Time: "<<chrono::duration_cast<milliseconds>(timer).count()/(double)1000 << " seconds "<<  '\n';
    std::cout << "Solved Exist-IPs: " << Solver.CounterExistIP <<endl;
    std::cout << "Solved All-IPs: " << Solver.CounterAllIP <<endl;
    return 1;
}
