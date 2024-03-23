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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

include(FetchContent)
set(FETCHCONTENT_BASE_DIR ${PROJECT_SOURCE_DIR}/dependencies/${CMAKE_BUILD_TYPE})

FetchContent_Declare(glfw
  URL https://github.com/glfw/glfw/archive/refs/tags/3.4.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)

FetchContent_MakeAvailable(glfw)

FetchContent_Declare(glm
  URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)

FetchContent_MakeAvailable(glm)

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
    "${PROJECT_SOURCE_DIR}/vendor/stb/include"
)

target_link_libraries(${TARGET_NAME} Vulkan::Vulkan glm::glm glfw)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(COMPILE_FLAGS -Wextra -Wall)
  set(DEBUG_COMPILE_FLAGS ${COMPILE_FLAGS} -g)
  set(RELEASE_COMPILE_FLAGS ${COMPILE_FLAGS} -O3)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(COMPILE_FLAGS /W4)
  set(DEBUG_COMPILE_FLAGS ${COMPILE_FLAGS})
  set(RELEASE_COMPILE_FLAGS ${COMPILE_FLAGS} /O2)
endif()

target_compile_options(${TARGET_NAME} PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_COMPILE_FLAGS}>")
target_compile_options(${TARGET_NAME} PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_COMPILE_FLAGS}>")

set(SHADER_BINARY_DIR "${PROJECT_BINARY_DIR}/shaders")

file(GLOB FRAG_SHADER_SOURCES "${PROJECT_SOURCE_DIR}/shaders/fragment/*.glsl")
file(GLOB VERT_SHADER_SOURCES "${PROJECT_SOURCE_DIR}/shaders/vertex/*.glsl")

find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

set(FRAG_SHADER_BINARIES "")
foreach(SHADER_SOURCE ${FRAG_SHADER_SOURCES})
  get_filename_component(SHADER_FILENAME ${SHADER_SOURCE} NAME)
  string(REGEX REPLACE "[.]glsl$" ".spv" SHADER_BINARY_NAME ${SHADER_FILENAME})
  set(SHADER_BINARY "${SHADER_BINARY_DIR}/fragment/${SHADER_BINARY_NAME}")
  list(APPEND FRAG_SHADER_BINARIES ${SHADER_BINARY})
  add_custom_command(
    OUTPUT ${SHADER_BINARY}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_BINARY_DIR}/fragment"
    COMMAND ${glslc_executable} -fshader-stage=fragment ${SHADER_SOURCE} -o ${SHADER_BINARY}
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
    COMMAND ${glslc_executable} -fshader-stage=vertex ${SHADER_SOURCE} -o ${SHADER_BINARY}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    MAIN_DEPENDENCY ${SHADER_SOURCE}
  )
endforeach()
add_custom_target(vert_shaders DEPENDS ${VERT_SHADER_BINARIES})

add_dependencies(${TARGET_NAME} frag_shaders)
add_dependencies(${TARGET_NAME} vert_shaders)

add_custom_target(
  link_textures ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/textures ${PROJECT_BINARY_DIR}/textures
)
