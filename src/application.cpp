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
  ModelPtr model = std::make_unique<Model>();
  model->vertices = {
    {{ -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
    {{ 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }},
    {{ 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }},
    {{ -0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f }},

    {{ -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
    {{ 0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }},
    {{ 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }},
    {{ -0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f }}
  };
  model->indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
  };

  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Don't create OpenGL context
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan window", nullptr, nullptr);

  LoggerPtr logger = createLogger(std::cerr, std::cerr, std::cout, std::cout);

  {
    auto renderer = CreateRenderer(*window, *logger);

    renderer->addModel(std::move(model));

    while(!glfwWindowShouldClose(window)) {
      glfwPollEvents();
      renderer->update();
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
