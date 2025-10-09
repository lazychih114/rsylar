# 获取当前源文件所在目录的绝对路径
get_filename_component(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

# 将该目录下所有源文件添加到 SRC_LIST 变量中
aux_source_directory(${SRC_DIR} SRC_LIST)

# proto 文件列表（使用绝对路径以避免路径问题）
set(RAFT_PROTO_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/raftRPC.proto
	${CMAKE_CURRENT_SOURCE_DIR}/kvServerRPC.proto
)

# 生成 protobuf 源文件（.pb.cc / .pb.h）
protobuf_generate_cpp(RAFT_PROTO_SRCS RAFT_PROTO_HDRS ${RAFT_PROTO_FILES})

# 将生成的 proto 源编译为静态库并链接 protobuf runtime so callers get symbols
add_library(raftRpcPro STATIC ${RAFT_PROTO_SRCS})
# Ensure consumers can #include the generated headers
target_include_directories(raftRpcPro PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# link against protobuf libraries for generated code
target_link_libraries(raftRpcPro PUBLIC ${PROTOBUF_LIBRARIES})
