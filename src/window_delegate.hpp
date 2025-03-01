#pragma once

#include <memory>

// Renderer implementations may need to call functions specific to the windowing system or
// graphics API to create surfaces, graphics contexts, etc.
class WindowDelegate
{
  public:
    virtual ~WindowDelegate() = 0;
};

inline WindowDelegate::~WindowDelegate() {}

using WindowDelegatePtr = std::unique_ptr<WindowDelegate>;
