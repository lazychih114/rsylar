find_package(Protobuf REQUIRED)

set(PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../friend.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

set(SRC_LIST callFriendService.cpp ${PROTO_SRCS})
add_executable(consumer ${SRC_LIST})

target_include_directories(consumer PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_SOURCE_DIR}
)

target_link_libraries(consumer PRIVATE rpc protobuf)

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)