#pragma once

#include <memory>

class IApplication {
public:
  virtual void run() = 0;
  virtual ~IApplication() {}
};

using ApplicationPtr = std::unique_ptr<IApplication>;

ApplicationPtr CreateApplication();
