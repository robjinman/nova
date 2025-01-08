#include "application.hpp"
#include "renderer.hpp"
#include "logger.hpp"
#include <iostream>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

namespace
{

class ApplicationImpl : public Application
{
public:
  void run() override;
};

void ApplicationImpl::run()
{
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Don't create OpenGL context
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan window", nullptr, nullptr);

  LoggerPtr logger = createLogger(std::cerr, std::cerr, std::cout, std::cout);

  {
    auto renderer = CreateRenderer(*window, *logger);

    while(!glfwWindowShouldClose(window)) {
      glfwPollEvents();
      
      renderer->beginFrame();
      // TODO
      renderer->endFrame();
    }
  }

  glfwDestroyWindow(window);
  glfwTerminate();
}

} // namespace

ApplicationPtr CreateApplication()
{
  return std::make_unique<ApplicationImpl>();
}
