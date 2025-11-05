project(raftExample)

set(SRC_LIST raftKvDB.cpp)
add_executable(raftCoreRun ${SRC_LIST})
target_link_libraries(raftCoreRun PUBLIC raftCore rsylar)
#################################

set(SRC_LIST2 caller.cpp)
add_executable(callerMain ${SRC_LIST2})
target_link_libraries(callerMain PUBLIC raftClerk rsylar)


SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)