# 设置debug版本信息 进程生成名字
if(${CMAKE_BUILD_TYPE} MATCHES "debug")
    if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
        set(EXECUTABLE_NAME "msf_debug_macox_guard")
    elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
        set(EXECUTABLE_NAME "msf_debug_linux_guard")
    endif()
else()
    if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
        set(EXECUTABLE_NAME "msf_release_macox_guard")
    elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
        set(EXECUTABLE_NAME "msf_release_linux_guard")
    endif()
endif()

# 设置链接库搜索目录
link_directories(
    /home/share/tomato/mod/libmsf/build/lib/msf_base/
    /home/share/tomato/mod/libmsf/build/lib/msf_sock/
    /home/share/tomato/mod/libmsf/build/lib/msf_event/
    /home/share/tomato/mod/libmsf/build/lib/msf_agent/
    /home/share/tomato/mod/libmsf/build/lib/
)
include (${PROJECT_SOURCE_DIR}/../cmake/utils.cmake)
#include (${PROJECT_SOURCE_DIR}/cmake/FindNUMA.cmake)
add_cplusplus_standard_support()

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -fPIC -O0 -Wall -g -ggdb")

# 设置依赖库
set(COMMLIBS msf_agent msf_event msf_sock msf_base numa pthread dl)

# 设置源代码路径
aux_source_directory (app DIR_SRCS)
add_executable(Mobile ${DIR_SRCS})
add_definitions(-Wno-builtin-macro-redefined)
msf_redefine_file(Mobile)
target_link_libraries(Mobile ${COMMLIBS})

#install (TARGETS lib LIBRARY DESTINATION lib)            # 安装到 usr/lib
#install (FILES * DESTINATION include/msf/base/)          # 安装到 usr/include/msf/base