#include "LaunchApp.h"

LaunchApp::LaunchApp(const AppLauncher &appLauncher, AppCatalogRepository &appCatalogRepository)
    : appLauncher_(appLauncher)
    , appCatalogRepository_(appCatalogRepository)
{
}

bool LaunchApp::execute(const domain::AppEntry &appEntry)
{
    if (!appLauncher_.launch(appEntry)) {
        return false;
    }

    if (appEntry.source == domain::AppSource::Discovered) {
        domain::AppEntry rememberedApp = appEntry;
        rememberedApp.source = domain::AppSource::Catalog;
        appCatalogRepository_.saveApp(rememberedApp);
    }
    return true;
}