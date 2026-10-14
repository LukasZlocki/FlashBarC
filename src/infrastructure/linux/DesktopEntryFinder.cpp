#include "DesktopEntryFinder.h"
#include "DesktopEntryParser.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSet>
#include <QStandardPaths>
#include <QString>
#include <QStringList>

#include <optional>

std::vector<domain::AppEntry> DesktopEntryFinder::scan() const
{
    QStringList directories = QStandardPaths::locateAll(
        QStandardPaths::GenericDataLocation, QStringLiteral("applications"),
        QStandardPaths::LocateDirectory);
    directories << QStringLiteral("/var/lib/flatpak/exports/share/applications")
                << QDir::homePath() + QStringLiteral("/.local/share/flatpak/exports/share/applications")
                << QStringLiteral("/var/lib/snapd/desktop/applications");

    const QString currentDesktop = QString::fromLocal8Bit(qgetenv("XDG_CURRENT_DESKTOP"));
    QSet<QString> seenFileNames;
    std::vector<domain::AppEntry> discoveredApps;

    for (const QString &directoryPath : directories) {
        const QDir directory(directoryPath);
        const QFileInfoList files = directory.entryInfoList(QStringList{QStringLiteral("*.desktop")}, QDir::Files);
        for (const QFileInfo &file : files) {
            if (seenFileNames.contains(file.fileName())) {
                continue;
            }
            seenFileNames.insert(file.fileName());

            QFile desktopFile(file.absoluteFilePath());
            if (!desktopFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                continue;
            }
            const std::optional<domain::AppEntry> parsedEntry =
                parseDesktopEntry(QString::fromUtf8(desktopFile.readAll()), currentDesktop);
            if (!parsedEntry) {
                continue;
            }

            domain::AppEntry appEntry = *parsedEntry;
            const QStringList commandParts = QProcess::splitCommand(QString::fromStdString(appEntry.command));
            if (commandParts.isEmpty()) {
                continue;
            }
            const QString executable = QStandardPaths::findExecutable(commandParts.first());
            if (executable.isEmpty()) {
                continue;
            }

            appEntry.path = executable.toStdString();
            appEntry.source = domain::AppSource::Discovered;
            discoveredApps.push_back(std::move(appEntry));
        }
    }

    return discoveredApps;
}
