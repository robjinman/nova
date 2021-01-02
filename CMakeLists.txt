cmake_minimum_required(VERSION 3.16)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(PROJECT_NAME VulkanTestLab)
set(TARGET_NAME vulkanTestLab)

project(${PROJECT_NAME})

set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

message("Building ${CMAKE_BUILD_TYPE} configuration...")

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

set(GLSLC_EXE "${VENDOR_DIR}/shaderc/bin/glslc")

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

set(COMPILER_FLAGS -Wextra -Wall)
set(DEBUG_FLAGS ${COMPILER_FLAGS} -g)
set(RELEASE_FLAGS ${COMPILER_FLAGS} -O3)

target_compile_options(${TARGET_NAME} PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_FLAGS}>")
target_compile_options(${TARGET_NAME} PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_FLAGS}>")

set(SHADER_BINARY_DIR "${PROJECT_BINARY_DIR}/shaders")

file(GLOB FRAG_SHADER_SOURCES "${PROJECT_SOURCE_DIR}/shaders/fragment/*.glsl")
file(GLOB VERT_SHADER_SOURCES "${PROJECT_SOURCE_DIR}/shaders/vertex/*.glsl")

set(FRAG_SHADER_BINARIES "")
foreach(SHADER_SOURCE ${FRAG_SHADER_SOURCES})
  get_filename_component(SHADER_FILENAME ${SHADER_SOURCE} NAME)
  string(REGEX REPLACE "[.]glsl$" ".spv" SHADER_BINARY_NAME ${SHADER_FILENAME})
  set(SHADER_BINARY "${SHADER_BINARY_DIR}/fragment/${SHADER_BINARY_NAME}")
  list(APPEND FRAG_SHADER_BINARIES ${SHADER_BINARY})
  add_custom_command(
    OUTPUT ${SHADER_BINARY}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_BINARY_DIR}/fragment"
    COMMAND ${GLSLC_EXE} -fshader-stage=fragment ${SHADER_SOURCE} -o ${SHADER_BINARY}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    MAIN_DEPENDENCY ${SHADER_SOURCE}
  )
endforeach()
add_custom_target(frag_shaders DEPENDS ${FRAG_SHADER_BINARIES})

set(VERT_SHADER_BINARIES "")
foreach(SHADER_SOURCE ${VERT_SHADER_SOURCES})
  get_filename_component(SHADER_FILENAME ${SHADER_SOURCE} NAME)
  string(REGEX REPLACE "[.]glsl$" ".spv" SHADER_BINARY_NAME ${SHADER_FILENAME})
  set(SHADER_BINARY "${SHADER_BINARY_DIR}/vertex/${SHADER_BINARY_NAME}")
  list(APPEND VERT_SHADER_BINARIES ${SHADER_BINARY})
  add_custom_command(
    OUTPUT ${SHADER_BINARY}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_BINARY_DIR}/vertex"
    COMMAND ${GLSLC_EXE} -fshader-stage=vertex ${SHADER_SOURCE} -o ${SHADER_BINARY}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    MAIN_DEPENDENCY ${SHADER_SOURCE}
  )
endforeach()
add_custom_target(vert_shaders DEPENDS ${VERT_SHADER_BINARIES})

add_dependencies(${TARGET_NAME} frag_shaders)
add_dependencies(${TARGET_NAME} vert_shaders)
