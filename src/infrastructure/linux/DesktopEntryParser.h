#pragma once

#include "../../domain/AppEntry.h"

#include <QString>

#include <optional>

std::optional<domain::AppEntry> parseDesktopEntry(const QString &contents, const QString &currentDesktop);
