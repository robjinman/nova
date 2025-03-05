cmake_minimum_required(VERSION 3.27)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_MAJOR "${VERSION_MAJOR}" PARENT_SCOPE)
set(VERSION_MINOR "${VERSION_MINOR}" PARENT_SCOPE)

set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}")
set(VERSION_STRING "${VERSION_STRING}" PARENT_SCOPE)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

message("System name is ${CMAKE_SYSTEM_NAME}")
message("Building ${CMAKE_BUILD_TYPE} configuration...")

set(LIB_TARGET "libnova")
set(LIB_TARGET "${LIB_TARGET}" PARENT_SCOPE)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(PLATFORM_LINUX 1)
  set(PLATFORM_LINUX "${PLATFORM_LINUX}" PARENT_SCOPE)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(PLATFORM_WINDOWS 1)
  set(PLATFORM_WINDOWS "${PLATFORM_WINDOWS}" PARENT_SCOPE)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(PLATFORM_OSX 1)
  set(PLATFORM_OSX "${PLATFORM_OSX}" PARENT_SCOPE)
elseif(CMAKE_SYSTEM_NAME MATCHES "iOS")
  set(PLATFORM_IOS 1)
  set(PLATFORM_IOS "${PLATFORM_IOS}" PARENT_SCOPE)
endif()

find_package(tinyxml2 REQUIRED)
find_package(Stb REQUIRED)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/version_in.hpp"
  "${PROJECT_BINARY_DIR}/include/version.hpp"
)

file(GLOB CPP_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/*.cpp"
)

add_library(${LIB_TARGET} ${CPP_SOURCES})

file(GLOB FRAG_SHADER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/shaders/fragment/*.glsl")
file(GLOB VERT_SHADER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/shaders/vertex/*.glsl")

find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

function(compile_shaders targetName shaderSources shaderStage shaderBinaryDir)
  if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(compile_flags -fshader-stage=${shaderStage} -g)
  else()
    set(compile_flags -fshader-stage=${shaderStage} -O)
  endif()

  set(shaderBinaries "")
  foreach(shaderSource ${shaderSources})
    get_filename_component(shaderFilename ${shaderSource} NAME)
    string(REGEX REPLACE "[.]glsl$" ".spv" shaderBinaryName ${shaderFilename})
    set(shaderBinary "${shaderBinaryDir}/${shaderStage}/${shaderBinaryName}")
    list(APPEND shaderBinaries ${shaderBinary})
    add_custom_command(
      OUTPUT ${shaderBinary}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${shaderBinaryDir}/${shaderStage}"
      COMMAND ${glslc_executable} ${compile_flags} ${shaderSource} -o ${shaderBinary}
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      MAIN_DEPENDENCY ${shaderSource}
    )
  endforeach()
  add_custom_target(${targetName} DEPENDS ${shaderBinaries})
endfunction()

compile_shaders(frag_shaders "${FRAG_SHADER_SOURCES}" fragment "${PROJECT_BINARY_DIR}/shaders")
compile_shaders(vert_shaders "${VERT_SHADER_SOURCES}" vertex "${PROJECT_BINARY_DIR}/shaders")

add_dependencies(${LIB_TARGET} frag_shaders)
add_dependencies(${LIB_TARGET} vert_shaders)

if(PLATFORM_LINUX)
  find_package(Vulkan REQUIRED)
  find_package(glfw3 REQUIRED)
  set(COMPILE_FLAGS -Wextra -Wall)
  set(DEBUG_COMPILE_FLAGS ${COMPILE_FLAGS} -g)
  set(RELEASE_COMPILE_FLAGS ${COMPILE_FLAGS} -O3)
elseif(PLATFORM_WINDOWS)
  find_package(Vulkan REQUIRED)
  find_package(glfw3 REQUIRED)
  set(COMPILE_FLAGS /W4)
  set(DEBUG_COMPILE_FLAGS ${COMPILE_FLAGS})
  set(RELEASE_COMPILE_FLAGS ${COMPILE_FLAGS} /O2)
elseif(PLATFORM_OSX)
  find_package(Vulkan REQUIRED)
  find_package(glfw3 REQUIRED)
  set(COMPILE_FLAGS -Wextra -Wall)
  set(DEBUG_COMPILE_FLAGS ${COMPILE_FLAGS} -g)
  set(RELEASE_COMPILE_FLAGS ${COMPILE_FLAGS} -O3)
elseif(PLATFORM_IOS)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/platform/ios/cmake")
  find_package(Vulkan REQUIRED COMPONENTS MoltenVK)
endif()

target_include_directories(${LIB_TARGET}
  PUBLIC
    "${PROJECT_BINARY_DIR}/include" # Location of version.hpp
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(${LIB_TARGET}
  PUBLIC
    Vulkan::Vulkan
    tinyxml2::tinyxml2
)

target_compile_options(${LIB_TARGET} PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_COMPILE_FLAGS}>")
target_compile_options(${LIB_TARGET} PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_COMPILE_FLAGS}>")
