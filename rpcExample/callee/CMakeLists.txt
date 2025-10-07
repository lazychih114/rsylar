
find_package(Protobuf REQUIRED)

set(PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../friend.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

set(SRC_LIST friendService.cpp ${PROTO_SRCS})

add_executable(provider ${SRC_LIST})

# include generated pb headers and project root so headers like <mprpcchannel.h>
# and generated friend.pb.h are found
target_include_directories(provider PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}    # generated friend.pb.h
	${CMAKE_SOURCE_DIR}            # project root (contains rpc/include)
)

target_link_libraries(provider PRIVATE rpc protobuf pthread)
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)