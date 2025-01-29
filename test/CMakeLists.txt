cmake_minimum_required(VERSION 3.24)

find_package(GTest REQUIRED)

file(GLOB_RECURSE SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(unitTests ${SRCS})

target_link_libraries(unitTests ${LIB_TARGET} GTest::gtest_main GTest::gmock_main)
target_compile_options(unitTests PRIVATE ${COMPILE_FLAGS})
