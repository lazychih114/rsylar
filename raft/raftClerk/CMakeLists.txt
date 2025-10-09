# 获取当前源文件所在目录的绝对路径
get_filename_component(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

# 将该目录下所有源文件添加到 SRC_LIST 变量中
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)

add_library(raftClerk OBJECT ${SRC_LIST})
target_include_directories(raftClerk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(raftClerk PUBLIC raftRpcPro raftCore)