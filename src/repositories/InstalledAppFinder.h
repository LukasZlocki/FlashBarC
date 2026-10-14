#pragma once

#include "../domain/AppEntry.h"

#include <vector>

class InstalledAppFinder
{
public:
    virtual ~InstalledAppFinder() = default;
    virtual std::vector<domain::AppEntry> scan() const = 0;
};
