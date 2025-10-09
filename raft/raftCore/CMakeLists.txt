# 将该目录下所有源文件添加到 SRC_LIST 变量中
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)

add_library(raftCore STATIC ${SRC_LIST})
target_include_directories(raftCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(raftCore PUBLIC raftRpcPro skipList rpc rsylar ${PROTOBUF_LIBRARIES} ${Boost_LIBRARIES})





