cmake_minimum_required(VERSION 3.16)

project(VulkanTestLabDependencies)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

include(external_glm)
include(external_glfw)
include(external_shaderc)
