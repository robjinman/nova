cmake_minimum_required(VERSION 3.16)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(PROJECT_NAME VulkanTestLab)
set(TARGET_NAME vulkanTestLab)

set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}")

project(${PROJECT_NAME})

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(common)

set(VENDOR_DIR ${PROJECT_SOURCE_DIR}/vendor/build/${PLATFORM_NAME})
list(APPEND CMAKE_PREFIX_PATH "${VENDOR_DIR}/glfw_bin/lib/cmake")
list(APPEND CMAKE_PREFIX_PATH "${VENDOR_DIR}/glm_src/src/libglm/cmake/glm")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

configure_file(
  "${PROJECT_SOURCE_DIR}/src/version_in.hpp"
  "${PROJECT_BINARY_DIR}/include/version.hpp"
)

file(GLOB CPP_SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(${TARGET_NAME} ${CPP_SOURCES})

target_include_directories(
  ${TARGET_NAME}
  PRIVATE
    "${PROJECT_BINARY_DIR}/include" # Location of version.hpp
    "${PROJECT_SOURCE_DIR}/src"
)

target_link_libraries(${TARGET_NAME} vulkan glm::glm glfw)
