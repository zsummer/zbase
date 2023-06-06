#cmake_minimum_required(VERSION 3.8)
#project(zbase)
#enable_language(ASM)

#from github.com/zsummer

# plat : WIN32 APPLE UNIX    (UNIX contain UNIX like)

# CMAKE_CXX_COMPILER_ID:  GNU Intel Clang AppleClang MSVC 
# example IF (CMAKE_CXX_COMPILER_ID MATCHES "Clang") ENDIF()

# jump compiler works check
# if(WIN32)
#    set(CMAKE_C_COMPILER_WORKS TRUE)
#    set(CMAKE_CXX_COMPILER_WORKS TRUE)
# endif(WIN32)

# jump this project build when msvc 
# set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)

# set this project setup build when msvc 
# set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# show msvc folder
#  SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 

# use C++ 14
# set(CMAKE_CXX_FLAGS -std=c++14) 

# 
# CMAKE_SOURCE_DIR   cmake root dir 
# CMAKE_CURRENT_SOURCE_DIR current cmakelist.txt dir  
# EXECUTABLE_OUTPUT_PATH can set it change bin out dir
# CMAKE_MODULE_PATH can set it change module dir 
# PROJECT_NAME cur project name 

# include 
# include_directories  
# link_directories 
# link_libraries 

# 
# execute_process


cmake_minimum_required(VERSION 3.8)
project(zbase)

include (${PROJECT_SOURCE_DIR}/cmake/macro.cmake)
include (${PROJECT_SOURCE_DIR}/cmake/option.cmake)


add_definitions(-DOPEN_ZPROF)

if(USE_SHM)
    add_definitions(-DFN_LOG_USE_SHM=1)
endif()

#setDEBUG_SANDBOX ON)
if (DEBUG_SANDBOX)
    add_definitions(-DZDEBUG_UNINIT_MEMORY)
    add_definitions(-DZDEBUG_DEATH_MEMORY)
endif(DEBUG_SANDBOX)

option(OPEN_CHECK "check zmalloc logic " OFF) 
if (OPEN_CHECK)
    message("-- OPEN_CHECK is ON")
    add_definitions(-DZMALLOC_OPEN_CHECK=1)
    add_definitions(-DZDEBUG_UNINIT_MEMORY=1)
    add_definitions(-DZDEBUG_DEATH_MEMORY=1)
else()
    message("-- OPEN_CHECK is OFF")
endif(OPEN_CHECK)




file(GLOB_RECURSE src_files ${CMAKE_SOURCE_DIR}/src/include/*.h)
auto_group_source(src_files)
auto_include_from_source(src_files)
add_custom_target(${PROJECT_NAME} SOURCES  ${src_files} )





# tests  
include_directories(${CMAKE_SOURCE_DIR}/tests)
include_directories(${CMAKE_SOURCE_DIR}/tests/common)
auto_include_sub_hpp_dir(${CMAKE_SOURCE_DIR}/tests/common)
auto_group_sub_cpp_dir(${CMAKE_SOURCE_DIR}/tests/common)


get_sub_dir_name(sub_dirs ${CMAKE_SOURCE_DIR}/tests/)
foreach(sub_dir_name ${sub_dirs}) 
    set(test_sources "")
    if (${sub_dir_name} STREQUAL "common") 
        continue()
    endif()
    file(GLOB_RECURSE test_sources
    ${CMAKE_SOURCE_DIR}/tests/common/*.h 
    ${CMAKE_SOURCE_DIR}/tests/common/*.cpp 
    ${CMAKE_SOURCE_DIR}/tests/${sub_dir_name}/*.h 
    ${CMAKE_SOURCE_DIR}/tests/${sub_dir_name}/*.cpp)


    auto_group_source(test_sources)
    add_executable(${sub_dir_name}  ${test_sources})
    if (UNIX)
            #set_target_properties(${sub_dir_name} PROPERTIES COMPILE_FLAGS "" LINK_FLAGS "-ldl")
    else()
        if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
            set_target_properties(${sub_dir_name} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
        endif()
    endif()

    add_dependencies(${sub_dir_name} ${PROJECT_NAME})
endforeach()



