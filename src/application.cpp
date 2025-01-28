#include "time.hpp"
#include "application.hpp"
#include "renderer.hpp"
#include "logger.hpp"
#include "exception.hpp"
#include "camera.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
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

  auto renderer = CreateRenderer(*window, *logger);

  auto texture1 = loadTexture("data/textures/texture1.png");
  auto texture1Id = renderer->addTexture(std::move(texture1));

  auto texture2 = loadTexture("data/textures/texture2.png");
  auto texture2Id = renderer->addTexture(std::move(texture2));

  auto model1 = loadModel("data/models/monkey.obj");
  model1->texture = texture1Id;
  auto model2 = loadModel("data/models/monkey.obj");
  model2->texture = texture2Id;

  auto model1Id = renderer->addModel(std::move(model1));
  auto model2Id = renderer->addModel(std::move(model2));

  Mat4x4f model1Translation = glm::translate(Mat4x4f(1), Vec3f(-1.5, 0, 0));
  Mat4x4f model2Translation = glm::translate(Mat4x4f(1), Vec3f(1.5, 0, 0));

  Timer timer;
  FrameRateLimiter frameRateLimiter(60);

  Camera camera;
  camera.translate(Vec3f(0, 0, 8));

  while(!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    renderer->setModelTransform(model1Id, model1Translation * glm::rotate(Mat4x4f(1),
      glm::radians<float>((90.f * timer.elapsed())), Vec3f(0.f, 1.f, 0.f)));

    renderer->setModelTransform(model2Id, model2Translation * glm::rotate(Mat4x4f(1),
      glm::radians<float>((-90.f * timer.elapsed())), Vec3f(0.f, 1.f, 0.f)));

    renderer->update(camera);

    frameRateLimiter.wait();
  }

  renderer.reset();

  glfwDestroyWindow(window);
  glfwTerminate();
}

} // namespace

ApplicationPtr CreateApplication()
{
  return std::make_unique<ApplicationImpl>();
}
