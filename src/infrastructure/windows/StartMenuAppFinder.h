#pragma once

#include "../../repositories/InstalledAppFinder.h"

class StartMenuAppFinder final : public InstalledAppFinder
{
public:
    std::vector<domain::AppEntry> scan() const override;
};
