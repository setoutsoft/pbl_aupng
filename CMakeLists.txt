cmake_minimum_required (VERSION 3.10 FATAL_ERROR)
project(aupng)

add_definitions(-D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
add_definitions(-DUPNG_MEM_MGR)

add_subdirectory(gtest)

###################################################################
# aupng
###################################################################

add_library(aupng
    include/upng.h
    include/upng_config.h
    src/upng_internal.h
    src/upng.c
    src/upng_inflate.c
    src/upng_decode.c
)
target_include_directories(aupng
    PUBLIC include
)


###################################################################
# test_aupng
###################################################################
include_directories("gtest/include")
include_directories("stb")

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

