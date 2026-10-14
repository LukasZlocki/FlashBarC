#pragma once

#include "../repositories/InstalledAppFinder.h"

#include <memory>

std::unique_ptr<InstalledAppFinder> createInstalledAppFinder();
