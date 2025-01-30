cmake_minimum_required(VERSION 3.27)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(PROJECT_NAME Nova)

project(${PROJECT_NAME})

set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

message("System name is ${CMAKE_SYSTEM_NAME}")
message("Building ${CMAKE_BUILD_TYPE} configuration...")

set(EXE_TARGET "nova")
set(LIB_TARGET "libnova")

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(PLATFORM_LINUX 1)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(PLATFORM_WINDOWS 1)
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(PLATFORM_OSX 1)
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(tinyxml2 REQUIRED)
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
  add_library(${LIB_TARGET} ${CPP_SOURCES})
  add_executable(${EXE_TARGET} "${PROJECT_SOURCE_DIR}/src/application.cpp")
elseif(PLATFORM_WINDOWS)
  set(COMPILE_FLAGS /W4)
  set(DEBUG_COMPILE_FLAGS ${COMPILE_FLAGS})
  set(RELEASE_COMPILE_FLAGS ${COMPILE_FLAGS} /O2)
  add_library(${LIB_TARGET} ${CPP_SOURCES})
  add_executable(${EXE_TARGET} "${PROJECT_SOURCE_DIR}/src/application.cpp")
elseif(PLATFORM_OSX)
  set(COMPILE_FLAGS -Wextra -Wall)
  set(DEBUG_COMPILE_FLAGS ${COMPILE_FLAGS} -g)
  set(RELEASE_COMPILE_FLAGS ${COMPILE_FLAGS} -O3)
  add_library(${LIB_TARGET} ${CPP_SOURCES})
  add_executable(${EXE_TARGET} "${PROJECT_SOURCE_DIR}/src/application.cpp")
  set_target_properties(${EXE_TARGET} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "nova"
    MACOSX_BUNDLE_BUNDLE_NAME "Nova"
    MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
    MACOSX_BUNDLE_ICON_FILE "nova.icns"
  )
endif()

target_include_directories(${LIB_TARGET}
  PUBLIC
    "${PROJECT_BINARY_DIR}/include" # Location of version.hpp
    "${PROJECT_SOURCE_DIR}/src"
)

target_link_libraries(${LIB_TARGET}
  PUBLIC
    Vulkan::Vulkan
    tinyxml2::tinyxml2
)

target_link_libraries(${EXE_TARGET}
  PRIVATE
    ${LIB_TARGET}
    glfw
)

target_compile_options(${LIB_TARGET} PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_COMPILE_FLAGS}>")
target_compile_options(${LIB_TARGET} PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_COMPILE_FLAGS}>")

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

add_dependencies(${LIB_TARGET} frag_shaders)
add_dependencies(${LIB_TARGET} vert_shaders)

add_custom_target(
  link_data ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/data ${PROJECT_BINARY_DIR}/data
)

set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
include(InstallRequiredSystemLibraries)

if (PLATFORM_OSX)
  install(TARGETS ${EXE_TARGET}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
  )
  install(DIRECTORY "${PROJECT_SOURCE_DIR}/data"
    DESTINATION "${EXE_TARGET}.app/Contents/Resources")
  install(DIRECTORY "${PROJECT_BINARY_DIR}/shaders"
    DESTINATION "${EXE_TARGET}.app/Contents/Resources")
  install(FILES "${PROJECT_SOURCE_DIR}/osx/nova.icns"
    DESTINATION "${EXE_TARGET}.app/Contents/Resources")
  install(FILES "/usr/local/lib/libMoltenVK.dylib"
    DESTINATION "${EXE_TARGET}.app/Contents/Frameworks")
  install(FILES "${PROJECT_SOURCE_DIR}/osx/MoltenVK_icd.json"
    DESTINATION "${EXE_TARGET}.app/Contents/Resources/vulkan/icd.d")

  install(CODE "
    include(BundleUtilities)
    fixup_bundle(\"${CMAKE_INSTALL_PREFIX}/${EXE_TARGET}.app\" \"${CMAKE_INSTALL_PREFIX}/${EXE_TARGET}.app/Contents/Frameworks/libMoltenVK.dylib\" \"\")
    " COMPONENT Runtime)

  install(CODE "execute_process(COMMAND codesign --deep --force --sign - ${CMAKE_INSTALL_PREFIX}/${EXE_TARGET}.app)")
else()
  install(TARGETS ${EXE_TARGET} DESTINATION ".")
  install(DIRECTORY "${PROJECT_SOURCE_DIR}/data" DESTINATION ".")
  install(DIRECTORY "${PROJECT_BINARY_DIR}/shaders" DESTINATION ".")
endif()

set(CPACK_PACKAGE_VENDOR "Rob Jinman")
set(CPACK_PACKAGE_CONTACT "jinmanr@gmail.com")
include(CPack)

add_subdirectory("test")
