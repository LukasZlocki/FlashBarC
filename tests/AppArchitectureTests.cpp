#include "../src/infrastructure/JsonAppCatalogRepository.h"
#include "../src/services/LaunchApp.h"
#include "../src/infrastructure/linux/DesktopEntryParser.h"
#include "../src/services/SearchApps.h"

#include <QFile>
#include <QProcess>
#include <QStringList>
#include <QTemporaryDir>

#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
void require(bool condition, const std::string &message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

class InMemoryCatalog final : public AppCatalogRepository
{
public:
    std::vector<domain::AppEntry> loadApps() const override
    {
        return {
            {"Files", "file-manager", "folder"},
            {"Terminal", "terminal", "terminal"}
        };
    }

    bool saveApp(const domain::AppEntry &) override
    {
        return true;
    }
};

class InMemoryRankedCatalog final : public AppCatalogRepository
{
public:
    std::vector<domain::AppEntry> loadApps() const override
    {
        return {
            {"Vscode", "vscode", "vscode"},
            {"Code Tools", "code-tools", "tools"},
            {"Code", "code", "code"}
        };
    }

    bool saveApp(const domain::AppEntry &) override
    {
        return true;
    }
};

class RecordingLauncher final : public AppLauncher
{
public:
    bool launch(const domain::AppEntry &appEntry) const override
    {
        launchedApp = appEntry;
        return launchResult;
    }

    mutable domain::AppEntry launchedApp;
    bool launchResult = true;
};

void writeFile(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    require(file.open(QIODevice::WriteOnly), "Unable to create test catalog");
    require(file.write(contents) == contents.size(), "Unable to write test catalog");
}

void testDiscoveredFallback()
{
    InMemoryCatalog catalog;
    SearchApps searchApps(catalog);
    searchApps.setDiscoveredApps({
        {"Code", "\"C:\\Tools\\code.exe\"", "C:\\Tools\\code.exe", "C:\\Tools\\code.exe", domain::AppSource::Discovered},
        {"Terminal Pro", "terminal-pro", "terminal", "/usr/bin/terminal-pro", domain::AppSource::Discovered},
        {"Files", "file-manager", "folder", "/usr/bin/nautilus", domain::AppSource::Discovered}
    });

    const std::vector<domain::AppEntry> codeMatches = searchApps.execute("code");
    require(codeMatches.size() == 1 && codeMatches.front().name == "Code",
            "Discovered apps should be searched when the catalog has no matches");

    const std::vector<domain::AppEntry> terminalMatches = searchApps.execute("terminal");
    require(terminalMatches.size() == 1 && terminalMatches.front().source == domain::AppSource::Catalog,
            "Discovered apps should not be searched when the catalog has matches");
}

void testQuotedWindowsCommandStaysOneArgument()
{
    const QStringList parts = QProcess::splitCommand(QStringLiteral("\"C:\\Program Files\\App\\app.exe\""));
    require(parts.size() == 1 && parts.front() == QStringLiteral("C:\\Program Files\\App\\app.exe"),
            "A quoted Windows executable path should stay one argument");
}

void testJsonSaveAvoidsDuplicates()
{
    QTemporaryDir directory;
    require(directory.isValid(), "Unable to create temporary test directory");

    const QString catalogPath = directory.filePath(QStringLiteral("apps.json"));
    writeFile(catalogPath, QByteArrayLiteral("{\"apps\":[{\"name\":\"Files\",\"command\":\"files\"}]}"));

    JsonAppCatalogRepository catalog(catalogPath);
    const domain::AppEntry appEntry{"Code", "\"C:\\Tools\\code.exe\"", "code", "C:\\Tools\\code.exe", domain::AppSource::Catalog};
    require(catalog.saveApp(appEntry), "Saving a new app should succeed");
    require(catalog.saveApp(appEntry), "Saving an existing app should succeed");

    const std::vector<domain::AppEntry> apps = catalog.loadApps();
    require(apps.size() == 2 && apps.back().path == appEntry.path,
            "Saving should append a new app once and keep existing entries");
}

void testLaunchRemembersDiscoveredApp()
{
    QTemporaryDir directory;
    require(directory.isValid(), "Unable to create temporary test directory");

    JsonAppCatalogRepository catalog(directory.filePath(QStringLiteral("apps.json")));
    RecordingLauncher launcher;
    LaunchApp launchApp(launcher, catalog);

    const domain::AppEntry discovered{"Code", "code", "code", "/usr/bin/code", domain::AppSource::Discovered};
    require(launchApp.execute(discovered), "A discovered app should launch");
    const std::vector<domain::AppEntry> saved = catalog.loadApps();
    require(saved.size() == 1 && saved.front().source == domain::AppSource::Catalog
                && saved.front().path == discovered.path,
            "Launching a discovered app should remember it in the catalog");

    launcher.launchResult = false;
    require(!launchApp.execute({"Other", "other", "", "", domain::AppSource::Discovered}),
            "A failed discovered launch should remain a failure");
    require(catalog.loadApps().size() == 1, "A failed launch should not be remembered");
}

void testDesktopEntryParser()
{
    const std::optional<domain::AppEntry> visible = parseDesktopEntry(QStringLiteral(
        "[Desktop Entry]\nType=Application\nName=Code\nName[de]=Code DE\n"
        "Exec=/usr/bin/code --new-window %F\nIcon=vscode\n"), QString());
    require(visible && visible->name == "Code" && visible->command == "/usr/bin/code --new-window"
                && visible->icon == "vscode",
            "Visible desktop entries should parse with field codes removed");

    const std::optional<domain::AppEntry> percent = parseDesktopEntry(QStringLiteral(
        "[Desktop Entry]\nType=Application\nName=Tool\nExec=tool 100%% %u\n"), QString());
    require(percent && percent->command == "tool 100%", "Escaped percent signs should be kept");

    require(!parseDesktopEntry(QStringLiteral("[Desktop Entry]\nType=Application\nName=Hidden\nExec=hidden\nNoDisplay=true\n"), QString()),
            "NoDisplay entries should be skipped");
    require(!parseDesktopEntry(QStringLiteral("[Desktop Entry]\nType=Link\nName=Link\nExec=link\n"), QString()),
            "Non-application entries should be skipped");
    require(!parseDesktopEntry(QStringLiteral("[Desktop Entry]\nType=Application\nName=Gnome\nExec=gnome\nOnlyShowIn=GNOME;\n"), QStringLiteral("KDE")),
            "OnlyShowIn should filter by current desktop");
    require(!parseDesktopEntry(QStringLiteral("[Desktop Entry]\nType=Application\nName=Kde\nExec=kde\nNotShowIn=KDE;\n"), QStringLiteral("KDE")),
            "NotShowIn should filter by current desktop");
}

void testSearchApps()
{
    InMemoryCatalog catalog;
    SearchApps searchApps(catalog);

    const std::vector<domain::AppEntry> matchingApps = searchApps.execute("tER");
    require(matchingApps.size() == 1 && matchingApps.front().name == "Terminal",
            "Search should match names without case sensitivity");
    require(searchApps.execute("").size() == 2, "An empty search should return all apps");

    InMemoryRankedCatalog rankedCatalog;
    SearchApps rankedSearch(rankedCatalog);
    const std::vector<domain::AppEntry> rankedMatches = rankedSearch.execute("code");
    require(rankedMatches.size() == 3 && rankedMatches[0].name == "Code" && rankedMatches[1].name == "Code Tools",
            "The closest match should be listed first");
}

void testLaunchApp()
{
    RecordingLauncher launcher;
    InMemoryCatalog catalog;
    LaunchApp launchApp(launcher, catalog);
    const domain::AppEntry appEntry{"Terminal", "terminal", "terminal"};

    require(launchApp.execute(appEntry), "Launch result should be forwarded");
    require(launcher.launchedApp.command == appEntry.command,
            "The selected app should be forwarded to the launcher");

    launcher.launchResult = false;
    require(!launchApp.execute(appEntry), "A failed launch should remain a failure");
}

void testJsonCatalog()
{
    QTemporaryDir directory;
    require(directory.isValid(), "Unable to create temporary test directory");

    const QString catalogPath = directory.filePath(QStringLiteral("apps.json"));
    writeFile(catalogPath, QByteArrayLiteral(
        "{\"apps\":[{\"name\":\"Files\",\"command\":\"files\",\"icon\":\"folder\"},"
        "{\"name\":\"Invalid\",\"command\":\"\"},null]}"));

    JsonAppCatalogRepository catalog(catalogPath);
    const std::vector<domain::AppEntry> apps = catalog.loadApps();
    require(apps.size() == 1 && apps.front().name == "Files",
            "The JSON catalog should load valid entries and skip invalid entries");

    const QString invalidPath = directory.filePath(QStringLiteral("invalid.json"));
    writeFile(invalidPath, QByteArrayLiteral("not json"));
    bool invalidJsonReported = false;
    try {
        JsonAppCatalogRepository(invalidPath).loadApps();
    } catch (const std::runtime_error &) {
        invalidJsonReported = true;
    }
    require(invalidJsonReported, "Invalid JSON should be reported");

    bool missingFileReported = false;
    try {
        JsonAppCatalogRepository(directory.filePath(QStringLiteral("missing.json"))).loadApps();
    } catch (const std::runtime_error &) {
        missingFileReported = true;
    }
    require(missingFileReported, "A missing catalog should be reported");
}
}

int main()
{
    try {
        testSearchApps();
        testLaunchApp();
        testJsonCatalog();
        testDiscoveredFallback();
        testQuotedWindowsCommandStaysOneArgument();
        testJsonSaveAvoidsDuplicates();
        testLaunchRemembersDiscoveredApp();
        testDesktopEntryParser();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}