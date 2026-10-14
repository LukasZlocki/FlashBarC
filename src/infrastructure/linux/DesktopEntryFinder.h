#pragma once

#include "../../repositories/InstalledAppFinder.h"

class DesktopEntryFinder final : public InstalledAppFinder
{
public:
    std::vector<domain::AppEntry> scan() const override;
};
