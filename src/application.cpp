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

ModelPtr createModel(TextureId texture, const glm::mat4& transform)
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
  model->texture = texture;
  model->transform = transform;

  return model;
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

    auto texture1 = loadTexture("data/textures/texture1.png");
    auto texture1Id = renderer->addTexture(std::move(texture1));

    auto texture2 = loadTexture("data/textures/texture2.png");
    auto texture2Id = renderer->addTexture(std::move(texture2));

    //auto model1 = createModel(texture1Id, glm::translate(glm::mat4(1), glm::vec3(-1, 0, 0)));
    auto model2 = loadModel("data/models/monkey.obj");
    model2->texture = texture2Id;

    //auto model1Id = renderer->addModel(std::move(model1));
    auto model2Id = renderer->addModel(std::move(model2));

    while(!glfwWindowShouldClose(window)) {
      glfwPollEvents();
      renderer->beginFrame();

      //renderer->setModelTransform(model1Id, glm::rotate(renderer->getModelTransform(model1Id),
      //  glm::radians(90.f / 100.f), glm::vec3(0.f, 1.f, 0.f)));

      renderer->setModelTransform(model2Id, glm::rotate(renderer->getModelTransform(model2Id),
        glm::radians(90.f / 100.f), glm::vec3(0.f, 1.f, 0.f)));

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
