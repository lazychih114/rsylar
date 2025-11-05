
project(rpc)

# explicit sources (we will generate pb sources from .proto at build time)
set(RPC_SRC
	mprpcchannel.cpp
	mprpcconfig.cpp
	mprpccontroller.cpp
	rpcprovider.cpp
)

# create rpc shared library
add_library(rpc SHARED ${RPC_SRC})

# expose include directories and protobuf link to consumers
target_include_directories(rpc PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}
	${Protobuf_INCLUDE_DIRS}
)
target_link_libraries(rpc PUBLIC ${Protobuf_LIBRARIES} rsylar)
