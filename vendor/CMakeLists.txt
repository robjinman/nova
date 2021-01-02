cmake_minimum_required(VERSION 3.16)

project(VulkanTestLabDependencies)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

set(SHADERC_CMAKE_ARGS "-DCMAKE_BUILD_TYPE=Release -DSHADERC_SKIP_TESTS=1")

include(external_glm)
include(external_glfw)
include(external_shaderc)
