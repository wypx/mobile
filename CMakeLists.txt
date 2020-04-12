set(MOBILE_SERVER_VERSION 0.1)

execute_process(COMMAND git log --pretty=format:%h -1 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE GITVER)
add_definitions(-DVERSION_TAG=${AGENT_SERVER_VERSION}-${GITVER})

# 设置debug版本信息 进程生成名字
if(${CMAKE_BUILD_TYPE} MATCHES "debug")
    file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/Debug) 
    if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
        file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/Darwin) 
    elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
        file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/Linux) 
    endif()
else()
    file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/Release) 
    if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
        file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/Darwin) 
    elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
        file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/Linux) 
    endif()
endif()
set(EXECUTABLE_NAME "Mobile")

include (${PROJECT_SOURCE_DIR}/../cmake/utils.cmake)
#include (${PROJECT_SOURCE_DIR}/cmake/FindNUMA.cmake)
add_cplusplus_standard_support()

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -fPIC -O0 -Wall -g -ggdb")

# 设置依赖库
set(COMMLIBS msf_agent msf_proto msf_event msf_sock msf_base protobuf.a protoc.a numa.a pthread dl)

# 设置源代码路径
aux_source_directory (app DIR_SRCS)
add_executable(${EXECUTABLE_NAME} ${DIR_SRCS})
add_definitions(-Wno-builtin-macro-redefined)
msf_redefine_file(${EXECUTABLE_NAME})
set_target_properties(${EXECUTABLE_NAME} PROPERTIES VERSION ${MOBILE_SERVER_VERSION})
target_link_libraries(${EXECUTABLE_NAME} ${COMMLIBS})

install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION bin)    # 二进制安装到 /home/luotang.me/bin
