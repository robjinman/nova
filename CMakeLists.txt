cmake_minimum_required(VERSION 3.22)

# TODO: Move this
if(ANDROID_BUILD)
  set(PLATFORM_ANDROID 1)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
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

set(PROJECT_NAME Nova)
project(${PROJECT_NAME})

file(GLOB FRAG_SHADER_SOURCES "${PROJECT_SOURCE_DIR}/nova/src/vulkan/shaders/fragment/*.glsl")
file(GLOB VERT_SHADER_SOURCES "${PROJECT_SOURCE_DIR}/nova/src/vulkan/shaders/vertex/*.glsl")

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

compile_shaders(frag_shaders "${FRAG_SHADER_SOURCES}" fragment "${PROJECT_SOURCE_DIR}/data/shaders")
compile_shaders(vert_shaders "${VERT_SHADER_SOURCES}" vertex "${PROJECT_SOURCE_DIR}/data/shaders")

if(PLATFORM_ANDROID)
  add_custom_target(android_bundle ALL
    COMMAND "${PROJECT_SOURCE_DIR}/platform/android/gradlew" "bundle${CMAKE_BUILD_TYPE}"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/platform/android"
    USES_TERMINAL
    COMMENT "Running Gradle build..."
  )
  add_dependencies(android_bundle frag_shaders)
  add_dependencies(android_bundle vert_shaders)
else()
  add_subdirectory("nova" "libnova")

  set(EXE_TARGET "nova_exe")

  if(PLATFORM_LINUX)
    find_package(glfw3 REQUIRED)
    add_executable(${EXE_TARGET}
      "${PROJECT_SOURCE_DIR}/nova/src/platform/default/application.cpp"
      "${PROJECT_SOURCE_DIR}/nova/src/platform/default/file_system.cpp"
      "${PROJECT_SOURCE_DIR}/nova/src/vulkan/platform/default/vulkan_window_delegate.cpp"
    )
    target_link_libraries(${EXE_TARGET} PRIVATE
      glfw
    )
  elseif(PLATFORM_WINDOWS)
    find_package(glfw3 REQUIRED)
    add_executable(${EXE_TARGET}
      "${PROJECT_SOURCE_DIR}/nova//src/platform/default/application.cpp"
      "${PROJECT_SOURCE_DIR}/nova/src/platform/default/file_system.cpp"
      "${PROJECT_SOURCE_DIR}/nova/src/vulkan/platform/default/vulkan_window_delegate.cpp"
    )
    target_link_libraries(${EXE_TARGET} PRIVATE
      glfw
    )
  elseif(PLATFORM_OSX)
    find_package(glfw3 REQUIRED)
    add_executable(${EXE_TARGET}
      "${PROJECT_SOURCE_DIR}/nova/src/platform/default/application.cpp"
      "${PROJECT_SOURCE_DIR}/nova/src/platform/default/file_system.cpp"
      "${PROJECT_SOURCE_DIR}/nova/src/vulkan/platform/default/vulkan_window_delegate.cpp"
    )
    set_target_properties(${EXE_TARGET} PROPERTIES
      MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_GUI_IDENTIFIER "nova"
      MACOSX_BUNDLE_BUNDLE_NAME "Nova"
      MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
      MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
      MACOSX_BUNDLE_ICON_FILE "nova.icns"
    )
    target_link_libraries(${EXE_TARGET} PRIVATE
      glfw
    )
  elseif(PLATFORM_IOS)
    list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/platform/ios/cmake")
    set(MACOSX_BUNDLE_BUNDLE_NAME "Nova")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "0.1")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "io.deadbeef.nova")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "0.1")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1")
    add_executable(${EXE_TARGET} MACOSX_BUNDLE
      "${PROJECT_SOURCE_DIR}/nova/src/platform/ios/wrapper.mm"
      "${PROJECT_SOURCE_DIR}/nova/src/platform/default/file_system.cpp"
      "${PROJECT_SOURCE_DIR}/nova/src/platform/ios/application.cpp"
      "${PROJECT_SOURCE_DIR}/nova/src/vulkan/platform/ios/vulkan_window_delegate.mm"
      "${PROJECT_SOURCE_DIR}/data"
      "$ENV{VULKAN_SDK}/../iOS/share/vulkan"
    )
    set_source_files_properties(
      "${PROJECT_SOURCE_DIR}/data"
      PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
    set_source_files_properties(
      "$ENV{VULKAN_SDK}/../iOS/share/vulkan"
      PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
    target_link_libraries(${EXE_TARGET} PRIVATE
      "-framework UIKit"
      "-framework Foundation"
      "-framework CoreGraphics"
      "-framework Metal"
      "-framework QuartzCore"
      "-framework IOKit"
      "-framework IOSurface"
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
      XCODE_EMBED_FRAMEWORKS "${Vulkan_MoltenVK_LIBRARY};${Vulkan_LIBRARY};${Vulkan_Layer_VALIDATION}" # TODO: ${Vulkan_Layer_VALIDATION}
      XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY "YES"
      XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY "YES"
      XCODE_ATTRIBUTE_SKIP_INSTALL "NO"
      XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
      XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
      XCODE_ATTRIBUTE_DEAD_CODE_STRIPPING "NO"
    )
  endif()

  target_link_libraries(${EXE_TARGET} PRIVATE
    ${LIB_TARGET}
  )

  add_dependencies(${EXE_TARGET} frag_shaders)
  add_dependencies(${EXE_TARGET} vert_shaders)

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
  endif()

  set(CPACK_PACKAGE_VENDOR "Rob Jinman")
  set(CPACK_PACKAGE_CONTACT "jinmanr@gmail.com")
  include(CPack)

  if(PLATOFRM_LINUX OR PLATFORM_OSX OR PLATFORM_WINDOWS)
    add_subdirectory("test")
  endif()

endif()
