
project(rpc)

# explicit sources (we will generate pb sources from .proto at build time)
set(RPC_COMMON_SRC
	mprpcchannel.cpp
	mprpcconfig.cpp
	mprpccontroller.cpp
	rpcprovider.cpp
)

# include directory for generated protobuf headers (must come first)
# Do not pollute directory-global include paths; attach include dirs to the
# rpc target so consumers linking to rpc get the correct include paths.


set(RPC_SRC ${RPC_COMMON_SRC})


# create rpc shared library
add_library(rpc SHARED ${RPC_SRC} ${src_common})

# expose include directories and protobuf link to consumers
target_include_directories(rpc PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}
	${Protobuf_INCLUDE_DIRS}
)
target_link_libraries(rpc PUBLIC ${Protobuf_LIBRARIES} rsylar)

# expose src list for top-level caching if needed
# set(src_rpc ${RPC_SRC} CACHE INTERNAL "rpc sources")

SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)