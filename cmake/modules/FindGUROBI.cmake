# FindGUROBI.cmake

# Locate Gurobi include directory
find_path(GUROBI_INCLUDE_DIRS
    NAMES     gurobi_c++.h
    HINTS     ${GUROBI_DIR} $ENV{GUROBI_HOME}
    PATH_SUFFIXES include
)

# Locate dynamic core library (e.g. libgurobi100.so)
find_library(GUROBI_LIBRARY
    NAMES     gurobi    gurobi120    gurobi110    gurobi100
    HINTS     ${GUROBI_DIR} $ENV{GUROBI_HOME}
    PATH_SUFFIXES lib
)

# Determine installation base (prefer GUROBI_DIR, otherwise GUROBI_HOME)
if(NOT GUROBI_DIR)
    set(_GurobiBase "$ENV{GUROBI_HOME}")
else()
    set(_GurobiBase "${GUROBI_DIR}")
endif()

# Find all versioned C++ API static archives in ${_GurobiBase}/lib
file(GLOB _ALL_GXX_LIBS
    LIST_DIRECTORIES false
    "${_GurobiBase}/lib/libgurobi_g++*.a"
)

# Select the archive with the highest version number
set(_best_lib    "")
set(_best_ver    "0.0")
foreach(_lib_path IN LISTS _ALL_GXX_LIBS)
    get_filename_component(_fname ${_lib_path} NAME)
    string(REGEX MATCH "g\\+\\+([0-9]+(\\.[0-9]+)*)" _match ${_fname})
    string(REGEX REPLACE "g\\+\\+([0-9]+(\\.[0-9]+)*)" "\\1" _ver ${_match})
    if(_ver VERSION_GREATER _best_ver)
        set(_best_ver ${_ver})
        set(_best_lib ${_lib_path})
    endif()
endforeach()

if(_best_lib)
    set(GUROBI_CXX_LIBRARY ${_best_lib})
    message(STATUS "Using Gurobi C++ API: ${GUROBI_CXX_LIBRARY}")
else()
    #message(WARNING
    #    "No libgurobi_g++*.a found in ${_GurobiBase}/lib. "
    #    "Falling back to gurobi_c++.a."
    #)
    find_library(GUROBI_CXX_LIBRARY
        NAMES     gurobi_c++
        HINTS     ${_GurobiBase} $ENV{GUROBI_HOME}
        PATH_SUFFIXES lib
    )
    if(NOT GUROBI_CXX_LIBRARY)
        message(FATAL_ERROR
            "Neither libgurobi_g++*.a nor gurobi_c++.a found. "
            "Set GUROBI_HOME or GUROBI_DIR correctly."
        )
    endif()
    message(STATUS "Using Gurobi C++ API: ${GUROBI_CXX_LIBRARY}")
endif()

# Use the same library for debug builds
set(GUROBI_CXX_DEBUG_LIBRARY ${GUROBI_CXX_LIBRARY})

# Verify that required variables are available
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GUROBI
    REQUIRED_VARS
        GUROBI_INCLUDE_DIRS
        GUROBI_LIBRARY
        GUROBI_CXX_LIBRARY
)

# Export variables for users of find_package(GUROBI)
set(GUROBI_FOUND        TRUE          CACHE BOOL     "Gurobi was found")
set(GUROBI_LIBRARIES    ${GUROBI_LIBRARY}    CACHE FILEPATH "Gurobi core shared lib")
set(GUROBI_CXX_LIBRARIES ${GUROBI_CXX_LIBRARY} CACHE FILEPATH "Gurobi C++ API static lib")
set(GUROBI_INCLUDE_DIR  ${GUROBI_INCLUDE_DIRS} CACHE PATH     "Gurobi include directory")
