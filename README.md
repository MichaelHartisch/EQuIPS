# EQuIPS
EQuIPS is an expansion-based solver for Quantified Integer Programs

# Building and Running
1. Install Gurobi. Make sure that environment variable GUROBI_HOME is set.
2. run "cmake -B build"
3. run "cmake --build build"
4. in the "build" folder run "./EQuIPS *instance*" (as a first test you can run ./EQuIPS ../instances/MiniMCN.qlp)

# File Format
Instances must be in the QLP file format [(see this documentation)](https://yasolqipsolver.github.io/yasol.github.io/About_Yasol/#the-qlp-file-format). 

# Troubleshooting
If you have trouble compiling or running EQuIPS, here is a list of potential fixes
* To help cmake find gurobi, set environment variable GUROBI_HOME to your Gurobi directory. This directory should contain the *dir* folder. If you do not explicitly provide this path, cmake will search for it, but it is more reliable to explicitly provide it, either by setting the environment variable variabe GUROBI_HOME, or use the cmake option -DGUROBI_HOME=/path/to/gurobi.
* To run EQuIPS, you also need a valid Gurobi license. You must set the environment variable variabe GRB_LICENSE_FILE to this file. Otherwise, you will simply get an (unintelligible) message "libc++abi: terminating due to uncaught exception of type GRBException", when trying to run EQuIPS

# References
Paper "An Expansion-Based Approach for Quantified Integer Programming" to appear in the proceeding of CP2025 [preprint]](https://arxiv.org/abs/2506.04452)