

# 源码分组: source_group    
# source_group(TREE <root> [PREFIX <prefix>] [FILES <src>...])  
# LIST作为参数传递过来时候不要解值(通过传递符号名传递) 否则LIST的内容只剩下第一个元素  

function(auto_group_source sources)
    source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${${sources}})
endfunction()




# 获得子目录列表  
function(get_sub_dir result dir_path)
    file(GLOB subs RELATIVE ${dir_path} ${dir_path}/*)
    set(dirlist "")
    foreach(sub ${subs})
        if(IS_DIRECTORY ${dir_path}/${sub})
            list(APPEND dirlist ${sub})
        endif()
    endforeach()
    set(${result} ${dirlist} PARENT_SCOPE)
endfunction()



# 自动include 头文件所在目录  
function(auto_include_from_source sources)
    foreach(file_name ${${sources}})
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
    FILE(GLOB_RECURSE hpps ${dir_path}/*.h ${dir_path}/*.hpp)
    auto_include_from_source(hpps)
endfunction()

function(auto_group_sub_cpp_dir dir_path)
    message("find all sub dirs from ${dir_path}")
    FILE(GLOB_RECURSE hpps ${dir_path}/*.h ${dir_path}/*.hpp ${dir_path}/*.cpp)
    auto_group_source(hpps)
endfunction()