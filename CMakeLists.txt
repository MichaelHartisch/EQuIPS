cmake_minimum_required(VERSION 3.24)
project(EQuIPS)

option(CXX "enable C++ compilation" ON)

if(CXX)
  enable_language(CXX)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
#set(GUROBI_LIBRARIES "${GUROBI_CXX_LIBRARY};${GUROBI_LIBRARY}" )

find_package(GUROBI REQUIRED)
include_directories(${GUROBI_INCLUDE_DIRS} "${CMAKE_SOURCE_DIR}/include")

if(GUROBI_FOUND)
  message(STATUS "GUROBI FOUND!")
else(GUROBI_FOUND)
  message(FATAL_ERROR "!!! GUROBI NOT FOUND !!!")
endif(GUROBI_FOUND)

message("C_GUROBI_LIB points to " ${C_GUROBI_LIB})
message("CPP_GUROBI_LIB points to " ${CPP_GUROBI_LIB})
message("GUROBI_LIBRARY points to " ${GUROBI_LIBRARY})
message("GUROBI_CXX_LIBRARY points to " ${GUROBI_CXX_LIBRARY})
message("GUROBI_INCLUDE_DIRS points to " ${GUROBI_INCLUDE_DIRS})


# list source files here
set(sources src/main.cpp src/Parser.cpp src/Solver.cpp src/MultiGame.cpp src/Moves.cpp src/VarInfo.cpp src/ExpansionTree.cpp)
set(CMAKE_CXX_STANDARD 17)

add_executable(EQuIPS ${sources} )
target_link_libraries(EQuIPS ${GUROBI_CXX_LIBRARY})
#target_link_libraries(EQuIPS optimized ${GUROBI_CXX_LIBRARY})
target_link_libraries(EQuIPS ${GUROBI_LIBRARY})
option(ENABLE_SANITIZERS "Enable sanitizers for debugging" OFF)

# Add sanitizer flags if the option is enabled
if (ENABLE_SANITIZERS)
    message(STATUS "Sanitizers enabled")

    # Define sanitizer flags
    set(SANITIZER_COMPILE_FLAGS "-fsanitize=address" "-fsanitize=undefined")
    set(SANITIZER_LINK_FLAGS "-fsanitize=address" "-fsanitize=undefined")

    # Check compiler compatibility
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(EQuIPS PRIVATE ${SANITIZER_COMPILE_FLAGS})
        target_link_options(EQuIPS PRIVATE ${SANITIZER_LINK_FLAGS})
    else()
        message(WARNING "Sanitizers may not be supported by this compiler.")
    endif()
else()
    message(STATUS "Sanitizers disabled")
endif()

