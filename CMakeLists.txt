cmake_minimum_required(VERSION 3.0)
project(zbase)
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
set(CMAKE_VERBOSE_MAKEFILEON ON)
if( NOT CMAKE_CONFIGURATION_TYPES )
    set( CMAKE_CONFIGURATION_TYPES Debug Release )
endif( NOT CMAKE_CONFIGURATION_TYPES )

set(CMAKE_BUILD_TYPE Release CACHE STRING "cache debug release "  )
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES} )

#分组添加到IDE
MACRO(GROUP_SRC_BY_DIR FILES)
    FOREACH(PATH_NAME ${${FILES}})
        STRING(REGEX REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/\(.*\) \\1 RELATIVE_PATH ${PATH_NAME})
        STRING(REGEX MATCH "(.+)/.*" HAS_GROUP ${RELATIVE_PATH})
        IF(HAS_GROUP)
                STRING(REGEX REPLACE "(.+)/.*" \\1 GROUP_NAME ${RELATIVE_PATH})
                STRING(REPLACE "/" "\\" GROUP_NAME ${GROUP_NAME})
                MESSAGE(${PATH_NAME} " into " ${GROUP_NAME})
                SOURCE_GROUP(${GROUP_NAME} FILES ${PATH_NAME})
        ELSE()
                SOURCE_GROUP("" FILES ${PATH_NAME})
        ENDIF()
    ENDFOREACH(PATH_NAME)
ENDMACRO(GROUP_SRC_BY_DIR)


FILE(GLOB_RECURSE SOURCES_H ${CMAKE_SOURCE_DIR}/src/include/*.h)
# 自动INCLUDE所有头文件的目录  (可能会影响到include的优先级)
# list(FILTER <list> <INCLUDE|EXCLUDE> REGEX <regular_expression>)  

GROUP_SRC_BY_DIR(SOURCES_H)
ADD_CUSTOM_TARGET(${PROJECT_NAME} SOURCES  ${SOURCES_H} )

FOREACH(FILE_NAME ${SOURCES_H})
        STRING( REGEX REPLACE "[^/\\\\]+$" " " DIR_NAME ${FILE_NAME} )
        MESSAGE(${FILE_NAME})
	MESSAGE(${DIR_NAME})
        LIST(APPEND DIR_NAME_LIST ${DIR_NAME})
ENDFOREACH()

IF(DIR_NAME_LIST)
    LIST(REMOVE_DUPLICATES DIR_NAME_LIST)
ENDIF()

FOREACH(DIR_NAME ${DIR_NAME_LIST})
        MESSAGE("auto include " ${DIR_NAME} )
        INCLUDE_DIRECTORIES(${DIR_NAME})
ENDFOREACH()
#通用设置部分 包括启用分组 设置 启动项目  
if(WIN32)
    ADD_DEFINITIONS(-DWIN32 -W3 )
    SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 
    SET_PROPERTY(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
else()
    SET(CMAKE_CXX_FLAGS -std=c++14)
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -Ofast -Wall")
    LINK_LIBRARIES(pthread m c)
endif()

ADD_DEFINITIONS(-DOPEN_ZPROF)

if(USE_SHM)
    ADD_DEFINITIONS(-DFN_LOG_USE_SHM=1)
endif()

#输出
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)


#自定义部分 
option(OPEN_CHECK "check zmalloc logic " OFF) 
if (OPEN_CHECK)
    message("-- OPEN_CHECK is ON")
    add_definitions(-DZMALLOC_OPEN_CHECK=1)
else()
    message("-- OPEN_CHECK is OFF")
endif(OPEN_CHECK)


include_directories(${CMAKE_SOURCE_DIR}/tests)
macro(SUB_DIR_LIST result curdir)
    file(GLOB children RELATIVE ${curdir} ${curdir}/*)
    set(dirlist "")
    foreach(child ${children})
        if(IS_DIRECTORY ${curdir}/${child})
            LIST(APPEND dirlist ${child})
        endif()
    endforeach()
    set(${result} ${dirlist})
endmacro()
SUB_DIR_LIST(SUB_DIRS ${CMAKE_SOURCE_DIR}/tests/)
FOREACH(SUB_DIR ${SUB_DIRS}) 
    set(EXAMPLE_SOURCE "")
    FILE(GLOB_RECURSE EXAMPLE_SOURCE  
    ${CMAKE_SOURCE_DIR}/tests/*.h 
    ${CMAKE_SOURCE_DIR}/tests/${SUB_DIR}/*.h 
    ${CMAKE_SOURCE_DIR}/tests/${SUB_DIR}/*.c 
    ${CMAKE_SOURCE_DIR}/tests/${SUB_DIR}/*.cc 
    ${CMAKE_SOURCE_DIR}/tests/${SUB_DIR}/*.cpp)
    Message(${EXAMPLE_SOURCE})
    GROUP_SRC_BY_DIR(EXAMPLE_SOURCE)
    ADD_EXECUTABLE(${SUB_DIR} ${ARENA_C} ${ARENA_H} ${EXAMPLE_SOURCE})
    if (UNIX)
            SET_TARGET_PROPERTIES(${SUB_DIR}
                PROPERTIES 
                COMPILE_FLAGS "" 
                LINK_FLAGS "-ldl" 
            )
    else()
        if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
            set_target_properties(${SUB_DIR} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
        endif()
    endif()

    add_dependencies(${SUB_DIR} ${PROJECT_NAME})
ENDFOREACH()



