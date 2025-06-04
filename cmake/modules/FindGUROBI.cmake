# Locate Gurobi include directory
find_path(GUROBI_INCLUDE_DIRS
    NAMES gurobi_c++.h
    HINTS ${GUROBI_DIR} $ENV{GUROBI_HOME}
    PATH_SUFFIXES include
)

# Locate dynamic core Gurobi library (e.g., libgurobi100.so)
find_library(GUROBI_LIBRARY
    NAMES gurobi gurobi120 gurobi110 gurobi100
    HINTS ${GUROBI_DIR} $ENV{GUROBI_HOME}
    PATH_SUFFIXES lib
)

# Detect the compiler version (e.g., 9.4.0 -> 9)
execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
    OUTPUT_VARIABLE GXX_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REGEX MATCH "^[0-9]+" GXX_VERSION_SHORT ${GXX_VERSION})

# Define candidate C++ API static libraries based on G++ version
set(GUROBI_CXX_CANDIDATES
    gurobi_g++${GXX_VERSION_SHORT}    # e.g. libgurobi_g++9.a
    gurobi_c++                        # fallback (not recommended, but may exist)
)

# Search for the C++ static library (libgurobi_g++X.a or libgurobi_c++.a)
find_library(GUROBI_CXX_LIBRARY
    NAMES ${GUROBI_CXX_CANDIDATES}
    HINTS ${GUROBI_DIR} $ENV{GUROBI_HOME}
    PATH_SUFFIXES lib
)

# Optional debug variant (fallback to release version)
set(GUROBI_CXX_DEBUG_LIBRARY ${GUROBI_CXX_LIBRARY})

# Package handle check
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GUROBI REQUIRED_VARS
    GUROBI_LIBRARY
    GUROBI_CXX_LIBRARY
    GUROBI_INCLUDE_DIRS
    HANDLE_COMPONENTS
)

# Optionally print out what was found
message(STATUS "Found Gurobi include dir: ${GUROBI_INCLUDE_DIRS}")
message(STATUS "Found Gurobi core lib: ${GUROBI_LIBRARY}")
message(STATUS "Found Gurobi C++ lib: ${GUROBI_CXX_LIBRARY}")
