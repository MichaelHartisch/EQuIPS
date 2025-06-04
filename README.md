# EQuIPS
EQuIPS is an expansion-based solver for Quantified Integer Programs

# Building and Running
1. Install Gurobi. Make sure that environment variable GUROBI_HOME is set.
2. run "cmake -B build"
3. run "cmake --build build"
4. in the "build" folder run "./EQuIPS *instance*" (as a first test you can run ./EQuIPS ../instances/MiniMCN.qlp)

# File Format
Instances must be in the QLP file format [(see this documentation)](https://yasolqipsolver.github.io/yasol.github.io/About_Yasol/#the-qlp-file-format). 

# References
Paper "An Expansion-Based Approach for Quantified Integer Programming" to appear in the proceeding of CP2025.