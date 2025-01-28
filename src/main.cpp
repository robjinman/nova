#include "version.hpp"
#include "logger.hpp"
#include "game.hpp"
#include "renderer.hpp"
#include "time.hpp"
#include "utils.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

namespace
{

static Game* g_game = nullptr;

void onKeyboardInput(GLFWwindow*, int key, int, int action, int)
{
  if (!g_game) {
    return;
  }

  const auto keyCode = [](int key) { return static_cast<KeyboardKey>(key); };

  if (action == GLFW_PRESS) {
    g_game->onKeyDown(keyCode(key));
  }
  else if (action == GLFW_RELEASE) {
    g_game->onKeyUp(keyCode(key));
  }
}

} // namespace

int main()
{
  auto logger = createLogger(std::cerr, std::cerr, std::cout, std::cout);

  try {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Don't create OpenGL context
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    std::string title = STR("Nova " << Nova_VERSION_MAJOR << "." << Nova_VERSION_MINOR);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, title.c_str(), nullptr, nullptr);

    {
      auto renderer = createRenderer(*window, *logger);
      auto game = createGame(*renderer, *logger);

      g_game = game.get();
      glfwSetKeyCallback(window, onKeyboardInput);

      FrameRateLimiter frameRateLimiter(60);

      while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        game->update(Vec3f(0)); // TODO

        frameRateLimiter.wait();
      }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
  }
  catch(const std::exception& e) {
    logger->error(e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
