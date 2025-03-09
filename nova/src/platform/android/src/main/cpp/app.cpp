#include "file_system.hpp"
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#include <android/log.h>
#include <cstdlib>
#include <cstring>

FileSystemPtr createAndroidFileSystem(AAssetManager& assetManager);

void android_main(android_app* app)
{
  AAssetManager* assetManager = app->activity->assetManager;

  FileSystemPtr fileSystem = createAndroidFileSystem(*assetManager);
  auto directory = fileSystem->directory("entities");
  __android_log_print(ANDROID_LOG_ERROR, "eggplant", "Begin");
  for (auto file : *directory) {
    __android_log_print(ANDROID_LOG_ERROR, "eggplant", "File name: %s", file.c_str());
  }
  __android_log_print(ANDROID_LOG_ERROR, "eggplant", "End");

  while (!app->destroyRequested) {
    android_poll_source* source = nullptr;
    ALooper_pollOnce(-1, nullptr, nullptr, reinterpret_cast<void**>(&source));

    if (source != nullptr) {
      source->process(app, source);
    }
  }
}
