#pragma once

#include "../services/AppLauncher.h"

class ProcessAppLauncher final : public AppLauncher
{
public:
    bool launch(const domain::AppEntry &appEntry) const override;
};