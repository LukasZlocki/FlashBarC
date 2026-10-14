#pragma once

#include "../domain/AppEntry.h"
#include "../repositories/AppCatalogRepository.h"

#include <string>
#include <vector>

class SearchApps
{
public:
    explicit SearchApps(const AppCatalogRepository &appCatalogRepository);

    void setDiscoveredApps(std::vector<domain::AppEntry> discoveredApps);
    std::vector<domain::AppEntry> execute(const std::string &query) const;

private:
    std::vector<domain::AppEntry> apps_;
    std::vector<domain::AppEntry> discoveredApps_;
};