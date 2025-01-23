cmake_minimum_required(VERSION 3.27)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(PROJECT_NAME Nova)
set(TARGET_NAME nova)

project(${PROJECT_NAME})

set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

message("System name is ${CMAKE_SYSTEM_NAME}")
message("Building ${CMAKE_BUILD_TYPE} configuration...")

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(PLATFORM_LINUX 1)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(PLATFORM_WINDOWS 1)
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(PLATFORM_OSX 1)
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Stb REQUIRED)

configure_file(
  "${PROJECT_SOURCE_DIR}/src/version_in.hpp"
  "${PROJECT_BINARY_DIR}/include/version.hpp"
)

file(GLOB CPP_SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

if(PLATFORM_LINUX)
  set(COMPILE_FLAGS -Wextra -Wall)
  set(DEBUG_COMPILE_FLAGS ${COMPILE_FLAGS} -g)
  set(RELEASE_COMPILE_FLAGS ${COMPILE_FLAGS} -O3)
  add_executable(${TARGET_NAME} ${CPP_SOURCES})
elseif(PLATFORM_WINDOWS)
  set(COMPILE_FLAGS /W4)
  set(DEBUG_COMPILE_FLAGS ${COMPILE_FLAGS})
  set(RELEASE_COMPILE_FLAGS ${COMPILE_FLAGS} /O2)
  add_executable(${TARGET_NAME} ${CPP_SOURCES})
elseif(PLATFORM_OSX)
  set(COMPILE_FLAGS -Wextra -Wall)
  set(DEBUG_COMPILE_FLAGS ${COMPILE_FLAGS} -g)
  set(RELEASE_COMPILE_FLAGS ${COMPILE_FLAGS} -O3)
  set(CPACK_BUNDLE_NAME "Nova")
  set(CPACK_BUNDLE_PLIST "${CMAKE_SOURCE_DIR}/osx/Info.plist")
  set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/osx/nova.icns")
  set(CPACK_BUNDLE_STARTUP_COMMAND "${TARGET_NAME}")
  add_executable(${TARGET_NAME} ${CPP_SOURCES})
endif()

target_include_directories(${TARGET_NAME}
  PRIVATE
    "${PROJECT_BINARY_DIR}/include" # Location of version.hpp
    "${PROJECT_SOURCE_DIR}/src"
)

target_link_libraries(${TARGET_NAME}
  Vulkan::Vulkan
  glm::glm
  glfw
)

target_compile_options(${TARGET_NAME} PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_COMPILE_FLAGS}>")
target_compile_options(${TARGET_NAME} PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_COMPILE_FLAGS}>")

file(GLOB FRAG_SHADER_SOURCES "${PROJECT_SOURCE_DIR}/shaders/fragment/*.glsl")
file(GLOB VERT_SHADER_SOURCES "${PROJECT_SOURCE_DIR}/shaders/vertex/*.glsl")

find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

function(compile_shaders targetName shaderSources shaderStage shaderBinaryDir)
  if (CMAKE_BUILD_TYPE STREQUAL Debug)
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

compile_shaders(frag_shaders ${FRAG_SHADER_SOURCES} fragment "${PROJECT_BINARY_DIR}/shaders")
compile_shaders(vert_shaders ${VERT_SHADER_SOURCES} vertex "${PROJECT_BINARY_DIR}/shaders")

add_dependencies(${TARGET_NAME} frag_shaders)
add_dependencies(${TARGET_NAME} vert_shaders)

add_custom_target(
  link_textures ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/textures ${PROJECT_BINARY_DIR}/textures
)

set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
include(InstallRequiredSystemLibraries)

install(DIRECTORY "${PROJECT_SOURCE_DIR}/textures" DESTINATION ".")
install(DIRECTORY "${PROJECT_BINARY_DIR}/shaders" DESTINATION ".")

if (PLATFORM_LINUX OR PLATFORM_WINDOWS)
  install(TARGETS ${TARGET_NAME} DESTINATION ".")
endif()

set(CPACK_PACKAGE_VENDOR "Rob Jinman")
set(CPACK_PACKAGE_CONTACT "jinmanr@gmail.com")
include(CPack)
