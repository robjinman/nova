#include <iostream>
#include "application.hpp"
#include "version.hpp"

int main()
{
  std::cout << "Vulkan Test Lab - Version "
            << VulkanTestLab_VERSION_MAJOR
            << "."
            << VulkanTestLab_VERSION_MINOR
#ifndef NDEBUG
            << " (Debug)"
#endif
            << std::endl;

  ApplicationPtr app = CreateApplication();

  try {
    app->run();
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
