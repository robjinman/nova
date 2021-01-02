cmake_minimum_required(VERSION 3.19)
include(common)

set(SHADERC_ARCHIVE_URL "https://storage.googleapis.com/shaderc/artifacts/prod/graphics_shader_compiler/shaderc/linux/continuous_gcc_release/350/20201209-133815/install.tgz")
set(SHADERC_ARCHIVE_NAME "shaderc.tgz")
set(SHADERC_ARCHIVE_PATH "${PROJECT_BINARY_DIR}/${SHADERC_ARCHIVE_NAME}")
set(SHADERC_PATH "${PROJECT_BINARY_DIR}/shaderc")

if (NOT EXISTS ${SHADERC_ARCHIVE_NAME})
  message("Downloading shaderc precompiled binaries...")
  file(DOWNLOAD ${SHADERC_ARCHIVE_URL} ${SHADERC_ARCHIVE_PATH})
endif()

if (NOT EXISTS ${SHADERC_PATH})
  message("Extracting shaderc precompiled binaries...")
  file(ARCHIVE_EXTRACT
       INPUT ${SHADERC_ARCHIVE_PATH}
       DESTINATION ${CMAKE_BINARY_DIR})
  file(RENAME "${CMAKE_BINARY_DIR}/install" ${SHADERC_PATH})
endif()
