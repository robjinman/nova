cmake_minimum_required(VERSION 3.27)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(PROJECT_NAME Nova)

project(${PROJECT_NAME})

set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

message("System name is ${CMAKE_SYSTEM_NAME}")
message("Building ${CMAKE_BUILD_TYPE} configuration...")

set(EXE_TARGET "nova")
set(LIB_TARGET "libnova")

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(PLATFORM_LINUX 1)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(PLATFORM_WINDOWS 1)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(PLATFORM_OSX 1)
elseif(CMAKE_SYSTEM_NAME MATCHES "iOS")
  set(PLATFORM_IOS 1)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(tinyxml2 REQUIRED)
find_package(Stb REQUIRED)

configure_file(
  "${PROJECT_SOURCE_DIR}/src/version_in.hpp"
  "${PROJECT_BINARY_DIR}/include/version.hpp"
)

file(GLOB
  CPP_SOURCES
  "${PROJECT_SOURCE_DIR}/src/*.cpp"
  "${PROJECT_SOURCE_DIR}/src/vulkan/*.cpp"
)

add_library(${LIB_TARGET} ${CPP_SOURCES})

if(PLATFORM_LINUX)
  find_package(Vulkan REQUIRED)
  find_package(glfw3 REQUIRED)
  set(COMPILE_FLAGS -Wextra -Wall)
  set(DEBUG_COMPILE_FLAGS ${COMPILE_FLAGS} -g)
  set(RELEASE_COMPILE_FLAGS ${COMPILE_FLAGS} -O3)
  add_executable(${EXE_TARGET}
    "${PROJECT_SOURCE_DIR}/src/platform/default/application.cpp"
    "${PROJECT_SOURCE_DIR}/src/vulkan/platform/default/vulkan_window_delegate.cpp"
  )
  target_link_libraries(${EXE_TARGET}
    PRIVATE
      glfw
  )
elseif(PLATFORM_WINDOWS)
  find_package(Vulkan REQUIRED)
  find_package(glfw3 REQUIRED)
  set(COMPILE_FLAGS /W4)
  set(DEBUG_COMPILE_FLAGS ${COMPILE_FLAGS})
  set(RELEASE_COMPILE_FLAGS ${COMPILE_FLAGS} /O2)
  add_executable(${EXE_TARGET}
    "${PROJECT_SOURCE_DIR}/src/platform/default/application.cpp"
    "${PROJECT_SOURCE_DIR}/src/vulkan/platform/default/vulkan_window_delegate.cpp"
  )
  target_link_libraries(${EXE_TARGET}
    PRIVATE
      glfw
  )
elseif(PLATFORM_OSX)
  find_package(Vulkan REQUIRED)
  find_package(glfw3 REQUIRED)
  set(COMPILE_FLAGS -Wextra -Wall)
  set(DEBUG_COMPILE_FLAGS ${COMPILE_FLAGS} -g)
  set(RELEASE_COMPILE_FLAGS ${COMPILE_FLAGS} -O3)
  add_executable(${EXE_TARGET}
    "${PROJECT_SOURCE_DIR}/src/platform/default/application.cpp"
    "${PROJECT_SOURCE_DIR}/src/vulkan/platform/default/vulkan_window_delegate.cpp"
  )
  set_target_properties(${EXE_TARGET} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "nova"
    MACOSX_BUNDLE_BUNDLE_NAME "Nova"
    MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
    MACOSX_BUNDLE_ICON_FILE "nova.icns"
  )
  target_link_libraries(${EXE_TARGET}
    PRIVATE
      glfw
  )
elseif(PLATFORM_IOS)
  list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/platform/ios/cmake")
  find_package(Vulkan REQUIRED COMPONENTS MoltenVK)
  find_library(UIKIT_LIB UiKit)
  find_library(COREMEDIA_LIB CoreMedia)
  find_library(METAL_LIB Metal)
  set(MACOSX_BUNDLE_BUNDLE_NAME "Nova")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "0.1")
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "io.deadbeef.nova")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "0.1")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1")
  add_executable(${EXE_TARGET} MACOSX_BUNDLE
    "${PROJECT_SOURCE_DIR}/src/platform/ios/application.mm"
    "${PROJECT_SOURCE_DIR}/src/vulkan/platform/ios/vulkan_window_delegate.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/ios/hello.cpp"
    "$ENV{VULKAN_SDK}/../iOS/share/vulkan"
  )
  set_source_files_properties(
    $ENV{VULKAN_SDK}/../iOS/share/vulkan
    PROPERTIES
      MACOSX_PACKAGE_LOCATION Resources
  )
  target_link_libraries(${EXE_TARGET}
    PRIVATE
      ${UIKIT_LIB}
      ${COREMEDIA_LIB}
      ${METAL_LIB}
  )
  set(CMAKE_OSX_DEPLOYMENT_TARGET "17")
  set(CMAKE_XCODE_GENERATE_SCHEME "TRUE")
  set(CMAKE_XCODE_SCHEME_ENABLE_GPU_API_VALIDATION "FALSE")
  set(CMAKE_XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "DISABLED")
  set_target_properties(${EXE_TARGET} PROPERTIES
    BUNDLE_IDENTIFIER "io.deadbeef.nova"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "io.deadbeef.nova"
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES "YES"
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
    XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "NO"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
    MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "LW57Q4U5U7"
    XCODE_EMBED_FRAMEWORKS "${Vulkan_MoltenVK_LIBRARY};${Vulkan_LIBRARIES};${Vulkan_Layer_VALIDATION}" # TODO: ${Vulkan_Layer_VALIDATION}
    XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY "YES"
    XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY "YES"
    XCODE_ATTRIBUTE_SKIP_INSTALL "NO"
    XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
    XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
    XCODE_ATTRIBUTE_DEAD_CODE_STRIPPING "NO"
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
)

target_compile_options(${LIB_TARGET} PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_COMPILE_FLAGS}>")
target_compile_options(${LIB_TARGET} PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_COMPILE_FLAGS}>")

file(GLOB FRAG_SHADER_SOURCES "${PROJECT_SOURCE_DIR}/shaders/fragment/*.glsl")
file(GLOB VERT_SHADER_SOURCES "${PROJECT_SOURCE_DIR}/shaders/vertex/*.glsl")

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

add_custom_target(
  link_data ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/data ${PROJECT_BINARY_DIR}/data
)

set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
include(InstallRequiredSystemLibraries)

if(PLATFORM_OSX)
  install(TARGETS ${EXE_TARGET}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
  )
  install(DIRECTORY "${PROJECT_SOURCE_DIR}/data"
    DESTINATION "${EXE_TARGET}.app/Contents/Resources")
  install(DIRECTORY "${PROJECT_BINARY_DIR}/shaders"
    DESTINATION "${EXE_TARGET}.app/Contents/Resources")
  install(FILES "${PROJECT_SOURCE_DIR}/platform/osx/nova.icns"
    DESTINATION "${EXE_TARGET}.app/Contents/Resources")
  install(FILES "/usr/local/lib/libMoltenVK.dylib"
    DESTINATION "${EXE_TARGET}.app/Contents/Frameworks")
  install(FILES "${PROJECT_SOURCE_DIR}/platform/osx/MoltenVK_icd.json"
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

if(PLATOFRM_LINUX OR PLATFORM_OSX OR PLATFORM_WINDOWS)
  add_subdirectory("test")
endif()
