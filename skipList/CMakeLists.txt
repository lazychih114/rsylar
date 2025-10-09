project(skipList)

# skipList is a header-only library (templates implemented in header).
# Expose it as an INTERFACE target so other targets can link to it for
# include directories without requiring source files.
add_library(skipList INTERFACE)
target_include_directories(skipList INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)