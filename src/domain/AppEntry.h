#pragma once

#include <string>

namespace domain
{
enum class AppSource
{
    Catalog,
    Discovered
};

struct AppEntry
{
    std::string name;
    std::string command;
    std::string icon;
    std::string path;
    AppSource source = AppSource::Catalog;
};
}