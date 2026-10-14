#pragma once

#include "../domain/AppEntry.h"
#include "../repositories/AppCatalogRepository.h"
#include "AppLauncher.h"

class LaunchApp
{
public:
    LaunchApp(const AppLauncher &appLauncher, AppCatalogRepository &appCatalogRepository);

    bool execute(const domain::AppEntry &appEntry);

private:
    const AppLauncher &appLauncher_;
    AppCatalogRepository &appCatalogRepository_;
};