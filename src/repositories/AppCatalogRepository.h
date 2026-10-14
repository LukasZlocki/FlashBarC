#pragma once

#include "../domain/AppEntry.h"

#include <vector>

class AppCatalogRepository
{
public:
    virtual ~AppCatalogRepository() = default;
    virtual std::vector<domain::AppEntry> loadApps() const = 0;
    virtual bool saveApp(const domain::AppEntry &appEntry) = 0;
};