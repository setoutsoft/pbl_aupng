cmake_minimum_required (VERSION 3.10 FATAL_ERROR)
project(aupng)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)


add_subdirectory(gtest)

###################################################################
# aupng
###################################################################

add_library(aupng
    src/upng.h
    src/upng_internal.h
    src/upng.c
    src/upng_inflate.c
    src/upng_decode.c
    src/upng_config.h
)
target_include_directories(aupng
    PUBLIC src
)


###################################################################
# test_aupng
###################################################################
include_directories("gtest/include")
  
add_executable(test_aupng
    test/DebugAllocator.hpp
    test/DebugAllocator.cpp
    test/test_common.hpp

    test/test_single_picture.cpp
    test/test_memory.cpp
    test/test_multiple_frames.cpp
)
target_link_libraries(test_aupng
    PRIVATE aupng
    PRIVATE gtest
)

