#pragma once

#include "window_delegate.hpp"
#include <memory>

class Application
{
  public:
    virtual void update() = 0;

    virtual ~Application() {}
};

using ApplicationPtr = std::unique_ptr<Application>;

ApplicationPtr createApplication(const char* bundlePath, WindowDelegatePtr windowDelegate);
