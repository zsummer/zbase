
set(CMAKE_VERBOSE_MAKEFILEON ON)
if( NOT CMAKE_CONFIGURATION_TYPES )
    set( CMAKE_CONFIGURATION_TYPES Debug Release )
endif( NOT CMAKE_CONFIGURATION_TYPES )

set(CMAKE_BUILD_TYPE Release CACHE STRING "cache debug release "  )
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES} )


#通用设置部分 包括启用分组 设置 启动项目  
if(WIN32)
    add_definitions(-DWIN32 -W3 )
    set_property(GLOBAL PROPERTY USE_FOLDERS ON) 
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
else()
    set(CMAKE_CXX_FLAGS -std=c++14)
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
    set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall")
    link_libraries(pthread m c dl)
endif()


#输出
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)






