#include "StartMenuAppFinder.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <shobjidl.h>

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include <optional>
#include <string>

namespace
{
std::optional<domain::AppEntry> resolveShortcut(const QString &shortcutPath)
{
    IShellLinkW *shellLink = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&shellLink)))) {
        return std::nullopt;
    }

    std::optional<domain::AppEntry> result;
    IPersistFile *persistFile = nullptr;
    if (SUCCEEDED(shellLink->QueryInterface(IID_PPV_ARGS(&persistFile)))
        && SUCCEEDED(persistFile->Load(shortcutPath.toStdWString().c_str(), STGM_READ))) {
        wchar_t target[MAX_PATH] = {};
        if (SUCCEEDED(shellLink->GetPath(target, MAX_PATH, nullptr, 0))) {
            const QString targetPath = QString::fromWCharArray(target);
            const QFileInfo targetInfo(targetPath);
            if (targetInfo.suffix().compare(QStringLiteral("exe"), Qt::CaseInsensitive) == 0
                && targetInfo.isFile()) {
                const QString name = QFileInfo(shortcutPath).completeBaseName();
                if (!name.contains(QStringLiteral("uninstall"), Qt::CaseInsensitive)) {
                    result = domain::AppEntry{
                        name.toStdString(),
                        QStringLiteral("\"%1\"").arg(QDir::toNativeSeparators(targetPath)).toStdString(),
                        QDir::toNativeSeparators(targetPath).toStdString(),
                        QDir::toNativeSeparators(targetPath).toStdString(),
                        domain::AppSource::Discovered
                    };
                }
            }
        }
    }

    if (persistFile) {
        persistFile->Release();
    }
    shellLink->Release();
    return result;
}
}

std::vector<domain::AppEntry> StartMenuAppFinder::scan() const
{
    const bool comInitialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));

    const QStringList startMenuRoots = {
        qEnvironmentVariable("APPDATA") + QStringLiteral("/Microsoft/Windows/Start Menu/Programs"),
        qEnvironmentVariable("PROGRAMDATA") + QStringLiteral("/Microsoft/Windows/Start Menu/Programs")
    };

    std::vector<domain::AppEntry> discoveredApps;
    for (const QString &root : startMenuRoots) {
        QDirIterator iterator(root, QStringList{QStringLiteral("*.lnk")}, QDir::Files,
                              QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            const std::optional<domain::AppEntry> appEntry = resolveShortcut(iterator.next());
            if (appEntry) {
                discoveredApps.push_back(*appEntry);
            }
        }
    }

    if (comInitialized) {
        CoUninitialize();
    }
    return discoveredApps;
}
