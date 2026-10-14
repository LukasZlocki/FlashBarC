#pragma once

#include "../domain/AppEntry.h"

class AppLauncher
{
public:
    virtual ~AppLauncher() = default;
    virtual bool launch(const domain::AppEntry &appEntry) const = 0;
};