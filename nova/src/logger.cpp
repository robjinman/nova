#include "logger.hpp"
#include <iostream>
#include <mutex>
#include <thread>

namespace
{

class LoggerImpl : public Logger
{
  public:
    LoggerImpl(std::ostream& errorStream, std::ostream& warningStream, std::ostream& infoStream,
      std::ostream& debugStream);

    void debug(const std::string& msg, bool newline = true) override;
    void info(const std::string& msg, bool newline = true) override;
    void warn(const std::string& msg, bool newline = true) override;
    void error(const std::string& msg, bool newline = true) override;

  private:
    std::mutex m_mutex;
    std::ostream& m_error;
    std::ostream& m_warning;
    std::ostream& m_info;
    std::ostream& m_debug;

    void endMessage(std::ostream& stream, bool newline) const;
};

LoggerImpl::LoggerImpl(std::ostream& errorStream, std::ostream& warningStream,
  std::ostream& infoStream, std::ostream& debugStream)
  : m_error(errorStream)
  , m_warning(warningStream)
  , m_info(infoStream)
  , m_debug(debugStream)
{
}

void LoggerImpl::endMessage(std::ostream& stream, bool newline) const
{
  if (newline) {
    stream << std::endl;
  }
  else {
    stream << std::flush;
  }
}

void LoggerImpl::debug(const std::string& msg, bool newline)
{
  std::lock_guard lock(m_mutex);
  auto threadId = std::this_thread::get_id();
  m_debug << "[ DEBUG, " << threadId << " ] " << msg;
  endMessage(m_debug, newline);
}

void LoggerImpl::info(const std::string& msg, bool newline)
{
  std::lock_guard lock(m_mutex);
  m_info << "[ INFO ] " << msg;
  endMessage(m_info, newline);
}

void LoggerImpl::warn(const std::string& msg, bool newline)
{
  std::lock_guard lock(m_mutex);
  m_warning << "[ WARNING ] " << msg;
  endMessage(m_warning, newline);
}

void LoggerImpl::error(const std::string& msg, bool newline)
{
  std::lock_guard lock(m_mutex);
  m_error << "[ ERROR ] " << msg;
  endMessage(m_error, newline);
}

} // namespace

LoggerPtr createLogger(std::ostream& errorStream, std::ostream& warningStream,
  std::ostream& infoStream, std::ostream& debugStream)
{
  return std::make_unique<LoggerImpl>(errorStream, warningStream, infoStream, debugStream);
}
