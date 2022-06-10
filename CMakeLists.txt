cmake_minimum_required(VERSION 3.16)
project(sortinglowerbounds)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable assertions in release mode
if(NOT uppercase_CMAKE_BUILD_TYPE STREQUAL "DEBUG")
    # NOTE: use `add_compile_options` rather than `add_definitions` since
    # `add_definitions` does not support generator expressions.
    add_compile_options($<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-UNDEBUG>)

    # Also remove /D NDEBUG to avoid MSVC warnings about conflicting defines.
    foreach (flags_var_to_scrub
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_RELWITHDEBINFO
            CMAKE_CXX_FLAGS_MINSIZEREL
            CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_RELWITHDEBINFO
            CMAKE_C_FLAGS_MINSIZEREL)
        string (REGEX REPLACE "(^| )[/-]D *NDEBUG($| )" " "
                "${flags_var_to_scrub}" "${${flags_var_to_scrub}}")
    endforeach()
endif()

# List of sources
set(SOURCES
        src/main.cpp
        src/expandedPoset.cpp
        src/utils.cpp
        src/eventLog.cpp
        src/storageProfile.cpp
        src/posetObjCore.cpp
        src/posetObj.cpp
        src/posetHandle.cpp
        src/posetContainer.cpp
        src/posetInfo.cpp
        src/isoTest.cpp
        src/posetMap.cpp
        src/TimeProfile.cpp
        src/niceGraph.cpp
        src/storeAndLoad.cpp
        src/config.cpp
        src/stats.cpp
        src/linExtCalculator.cpp
        src/mmapAllocator.cpp
        src/backwardSearch.cpp
        src/forwardSearch.cpp
        src/bidirSearch.cpp
        src/searchParams.cpp
        src/oldGenMap.cpp
        src/tui.cpp)

# User Options
if (NOT DEFINED NUMEL)
    message(SEND_ERROR "NUMEL is not defined. Run cmake with -DNUMEL=<node-name> or when using Clion go to Settings -> Build, Execution, Deployment -> CMake and add -DNUMEL=<number> under cmake options.")
endif ()

add_compile_definitions(NUMEL=${NUMEL})

if (NOT DEFINED VARIABLE_N)
    set(VARIABLE_N False)
endif()

if (VARIABLE_N)
    message(NOTICE "VARIABLE_N is enabled. Use this for testing only. Disable for maximum performance.")
    add_compile_definitions(VARIABLE_N=1)
endif()

# Check whether we need 128 bit integers for linear extensions
if (NUMEL GREATER_EQUAL "20")
    add_compile_definitions(LARGE_NUMBERS)
endif ()

add_executable(sortinglowerbounds ${SOURCES})

# Build type specific flags
if (NOT uppercase_"${CMAKE_BUILD_TYPE}" STREQUAL "DEBUG")
    message(STATUS "Maximum optimization for speed")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT supported OUTPUT error)

    if (supported)
        message(STATUS "IPO / LTO enabled")
        set_property(TARGET sortinglowerbounds PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else ()
        message(STATUS "IPO / LTO not supported: <${error}>")
    endif ()
    target_compile_options(sortinglowerbounds PRIVATE -O3 -march=native -g3)
else ()
    message(NOTICE "You are building in DEBUG mode. Use this for testing only. Use Release mode for maximum performance.")
    message(STATUS "Minimal optimization, debug info included")
    target_compile_options(sortinglowerbounds PRIVATE -Og -g3)
    target_compile_definitions(sortinglowerbounds PRIVATE DEBUG_ASSERTIONS)
endif ()

# threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(sortinglowerbounds PRIVATE Threads::Threads)

# boost program options
find_package(Boost COMPONENTS program_options REQUIRED)
target_include_directories(sortinglowerbounds PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(sortinglowerbounds PRIVATE ${Boost_LIBRARIES})

# FTXUI
include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
        GIT_TAG 63e8dadad951b478f77981011a92477993a3b86d
        )

FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
    FetchContent_Populate(ftxui)
    add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

target_link_libraries(sortinglowerbounds
        PRIVATE ftxui::screen
        PRIVATE ftxui::dom
        PRIVATE ftxui::component
        )

# openmp
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(sortinglowerbounds PUBLIC OpenMP::OpenMP_CXX)
endif()
