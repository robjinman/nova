#pragma once

#include <string>

//#define TRACE

class Logger;

class Trace {
  public:
    Trace(Logger& logger, const std::string& file, const std::string& func);
    ~Trace();

  private:
    Logger& m_logger;
    std::string m_file;
    std::string m_func;
};

#ifdef TRACE
  #define DBG_TRACE(logger) Trace t(logger, __FILE__, __FUNCTION__);
#else
  #define DBG_TRACE(logger)
#endif
