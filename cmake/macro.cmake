#from github.com/zsummer

# plat : WIN32 APPLE UNIX    (UNIX contain UNIX like)

# CMAKE_CXX_COMPILER_ID:  GNU Intel Clang AppleClang MSVC 
# example IF (CMAKE_CXX_COMPILER_ID MATCHES "Clang") ENDIF()
# SET(CMAKE_C_COMPILER /usr/bin/gcc-8)
# SET(CMAKE_CXX_COMPILER /usr/bin/g++-8)
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





# 源码分组: source_group    
# source_group(TREE <root> [PREFIX <prefix>] [FILES <src>...])  
# LIST作为参数传递过来时候不要解值(通过传递符号名传递) 否则LIST的内容只剩下第一个元素  

function(auto_group_source src_file_list)
    source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${${src_file_list}})
endfunction()




# 获得子目录列表  
function(get_sub_dir_name result_list dir_path)
    file(GLOB subs RELATIVE ${dir_path} ${dir_path}/*)
    set(dirlist "")
    foreach(sub ${subs})
        if(IS_DIRECTORY ${dir_path}/${sub})
            list(APPEND dirlist ${sub})
        endif()
    endforeach()
    set(${result_list} ${dirlist} PARENT_SCOPE)
endfunction()



# 自动include 头文件所在目录  
function(auto_include_from_source src_file_list)
    foreach(file_name ${${src_file_list}})
        string(REGEX REPLACE "[^/\\]+$" " " dir_path ${file_name} )
        list(APPEND dir_paths ${dir_path})
    endforeach()

    if(dir_paths)
        list(REMOVE_DUPLICATES dir_paths)
    endif()

    foreach(dir_path ${dir_paths})
        message("auto include: " ${dir_path} )
        include_directories(${dir_path})
    endforeach()
endfunction()




function(auto_include_sub_hpp_dir dir_path)
    message("find all sub dirs from ${dir_path}")
    file(GLOB_RECURSE hpps ${dir_path}/*.h ${dir_path}/*.hpp)
    auto_include_from_source(hpps)
endfunction()


function(auto_group_sub_cpp_dir dir_path)
    message("find all sub dirs from ${dir_path}")
    file(GLOB_RECURSE hpps ${dir_path}/*.h ${dir_path}/*.hpp ${dir_path}/*.cpp)
    auto_group_source(hpps)
endfunction()

function(auto_custom_target_sub_dir dir_path target)
    file(GLOB_RECURSE hpps ${dir_path}/*.h ${dir_path}/*.hpp)
    message(${hpps})
    add_custom_target(${target} SOURCES  ${hpps} )
endfunction()


function(auto_link_from_file src_file_list)
    foreach(file_name ${${src_file_list}})
        string(REGEX REPLACE "[^/\\]+$" " " dir_path ${file_name} )
        string(REGEX MATCH "[^/\\]+$" lib_name ${file_name} )
        list(APPEND dir_paths ${dir_path})
        list(APPEND lib_names ${lib_name})
    endforeach()

    if(dir_paths)
        list(REMOVE_DUPLICATES dir_paths)
    endif()

    if(lib_names)
        list(REMOVE_DUPLICATES lib_names)
    endif()

    foreach(dir_path ${dir_paths})
        message("auto include: " ${dir_path} )
        link_directories(${dir_path})
    endforeach()

    foreach(lib_name ${lib_names})
        message("auto include: " ${lib_name} )
        add_library(${lib_name})
    endforeach()

endfunction()




function(auto_link_sub_lib_dir dir_path suffix)
    message("find all sub dirs from ${dir_path}")
    if(WIN32)
        FILE(GLOB_RECURSE libs ${dir_path}/*${suffix}.so ${dir_path}/*${suffix}.a)
    else()
        FILE(GLOB_RECURSE libs ${dir_path}/*${suffix}.lib ${dir_path}/*${suffix}.dll)
    endif()
    auto_link_from_file(libs)
endfunction()


