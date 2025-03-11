#include "logger.hpp"
#include <android/log.h>
#include <mutex>
#include <thread>

namespace
{

const char* LOG_TAG = "eggplant";

class AndroidLogger : public Logger
{
  public:
    void debug(const std::string& msg, bool newline = true) override;
    void info(const std::string& msg, bool newline = true) override;
    void warn(const std::string& msg, bool newline = true) override;
    void error(const std::string& msg, bool newline = true) override;

  private:
    std::mutex m_mutex;
};

void AndroidLogger::debug(const std::string& msg, bool)
{
  std::lock_guard lock(m_mutex);
  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "%s", msg.c_str());
}

void AndroidLogger::info(const std::string& msg, bool)
{
  std::lock_guard lock(m_mutex);
  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "%s", msg.c_str());
}

void AndroidLogger::warn(const std::string& msg, bool)
{
  std::lock_guard lock(m_mutex);
  __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "%s", msg.c_str());
}

void AndroidLogger::error(const std::string& msg, bool)
{
  std::lock_guard lock(m_mutex);
  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s", msg.c_str());
}

} // namespace

LoggerPtr createAndroidLogger()
{
  return std::make_unique<AndroidLogger>();
}
