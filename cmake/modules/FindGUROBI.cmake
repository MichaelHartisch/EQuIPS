# FindGUROBI.cmake
if (NOT DEFINED GUROBI_HOME)
    if (DEFINED ENV{GUROBI_HOME})
        set(GUROBI_HOME "$ENV{GUROBI_HOME}" CACHE PATH "Gurobi root directory")
    endif()
endif()

string(ASCII 27 ESC)
set(ORANGE "${ESC}[38;5;208m")
set(BLUE "${ESC}[1;34m")
set(NC "${ESC}[0m") # Reset/No Color

if (NOT GUROBI_HOME)
    message("${BLUE} No path to the root directory of gurobi was provided. I now try to find it myself! ${NC}")
    file(GLOB_RECURSE SEARCH_DIRS_A "/Library/gurobi*/**/gurobi_c++.h")
    file(GLOB_RECURSE SEARCH_DIRS_B "/opt/gurobi*/**/gurobi_c++.h")
    file(GLOB_RECURSE SEARCH_DIRS_C "${HOME}/gurobi/gurobi*/**/gurobi_c++.h")

    list(APPEND GUROBI_DIRS ${SEARCH_DIRS_A})
    list(APPEND GUROBI_DIRS ${SEARCH_DIRS_B})
    list(APPEND GUROBI_DIRS ${SEARCH_DIRS_C})
    if(GUROBI_DIRS)
        message(STATUS "I found these headers:")
        foreach(GUROBI_DIR ${GUROBI_DIRS})
            message(STATUS "${GUROBI_DIR}")
        endforeach()
        foreach(GUROBI_DIR ${GUROBI_DIRS})
            set(GUROBI_HOME "${GUROBI_DIR}") #/Library/gurobi1200/macos_universal2/lib/gurobi_c++.h
            get_filename_component(GUROBI_HOME "${GUROBI_HOME}" DIRECTORY)  #/Library/gurobi1200/macos_universal2/lib
            get_filename_component(GUROBI_HOME "${GUROBI_HOME}" DIRECTORY)  #/Library/gurobi1200/macos_universal2

            if(NOT EXISTS "${GUROBI_HOME}/lib")
                continue()
            else()
                message("${BLUE}This folder was selected: ${GUROBI_HOME}")
                message("${BLUE}If you wish to select another Gurobi version, specify it via -D option, or by setting the environment variable, e.g. 
    ${ORANGE} cmake -B build -DGUROBI_HOME=/opt/gurobi1200/linux64 
${BLUE} or 
    ${ORANGE} export GUROBI_HOME=/Library/gurobi1200/macos_universal2 ")
                break()
            endif()
        endforeach()
    endif()
    if (NOT GUROBI_HOME)
        message(FATAL_ERROR "I was not able to find the root directory of Gurobi. Provide the respective path, which might look like this: -DGUROBI_HOME=/opt/gurobi1200/linux64")
    else()
        #message(STATUS "Found root. GUROBI_HOME: ${GUROBI_HOME}")
    endif()
else()
    if(NOT EXISTS "${GUROBI_HOME}/lib")
        message(FATAL_ERROR "The provided GUROBI_HOME does not seem to contain the folder 'lib'. Maybe a typo?")
    endif()
endif()

 # Locate Gurobi include directory
find_path(GUROBI_INCLUDE_DIRS
NAMES     gurobi_c++.h
HINTS     ${GUROBI_HOME}  ${GUROBI_HOME}   

PATH_SUFFIXES include
)

# Locate dynamic core library (e.g. libgurobi100.so)
find_library(GUROBI_LIBRARY
    NAMES     gurobi    gurobi120    gurobi110    gurobi100
    PATHS     "${GUROBI_HOME}/lib"
    NO_CMAKE_SYSTEM_PATH
)


# Find all versioned C++ API static archives in ${GUROBI_HOME}/lib
file(GLOB _ALL_GXX_LIBS
    LIST_DIRECTORIES false
    "${GUROBI_HOME}/lib/libgurobi_g++*.a"
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
    #    "No libgurobi_g++*.a found in ${GUROBI_HOME}/lib. "
    #    "Falling back to gurobi_c++.a."
    #)
    find_library(GUROBI_CXX_LIBRARY
        NAMES     gurobi_c++
        HINTS     ${GUROBI_HOME}
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
