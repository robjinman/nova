#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#include <android/log.h>
#include <cstdlib>
#include <cstring>

void android_main(android_app* app)
{
  AAssetManager* assetManager = app->activity->assetManager;
  AAsset* asset = AAssetManager_open(assetManager, "map.svg", AASSET_MODE_BUFFER);
  size_t assetLength = AAsset_getLength(asset);
  char* buffer = (char*)malloc(assetLength + 1);
  memset(buffer, '\0', assetLength + 1);
  AAsset_read(asset, buffer, assetLength);

  __android_log_print(ANDROID_LOG_ERROR, "eggplant", "Data file: %s", buffer);

  while (!app->destroyRequested) {
    android_poll_source* source = nullptr;
    ALooper_pollOnce(-1, nullptr, nullptr, reinterpret_cast<void**>(&source));

    if (source != nullptr) {
      source->process(app, source);
    }
  }
}
