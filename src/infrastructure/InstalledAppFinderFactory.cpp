#include "InstalledAppFinderFactory.h"

#if defined(_WIN32)
#include "windows/StartMenuAppFinder.h"
#elif defined(__linux__)
#include "linux/DesktopEntryFinder.h"
#endif

namespace
{
class NoInstalledAppFinder final : public InstalledAppFinder
{
public:
    std::vector<domain::AppEntry> scan() const override
    {
        return {};
    }
};
}

std::unique_ptr<InstalledAppFinder> createInstalledAppFinder()
{
#if defined(_WIN32)
    return std::make_unique<StartMenuAppFinder>();
#elif defined(__linux__)
    return std::make_unique<DesktopEntryFinder>();
#else
    return std::make_unique<NoInstalledAppFinder>();
#endif
}
