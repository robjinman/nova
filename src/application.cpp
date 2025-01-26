#include "application.hpp"
#include "renderer.hpp"
#include "logger.hpp"
#include "exception.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
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

TexturePtr loadTexture(const std::string& filePath)
{
  int width = 0;
  int height = 0;
  int channels = 0;
  stbi_uc* pixels = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

  if (!pixels) {
    EXCEPTION("Failed to load texture image");
  }

  TexturePtr texture = std::make_unique<Texture>();
  texture->width = width;
  texture->height = height;
  texture->channels = channels;
  texture->data.resize(width * height * channels);
  memcpy(texture->data.data(), pixels, width * height * channels);

  stbi_image_free(pixels);

  return texture;
}

void ApplicationImpl::run()
{
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Don't create OpenGL context
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan window", nullptr, nullptr);

  LoggerPtr logger = createLogger(std::cerr, std::cerr, std::cout, std::cout);

  {
    auto renderer = CreateRenderer(*window, *logger);

    auto texture = loadTexture("textures/texture.png");
    auto textureId = renderer->addTexture(std::move(texture));

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
    model->texture = textureId;

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
