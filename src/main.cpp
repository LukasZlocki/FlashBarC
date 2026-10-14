#include "ui/MainWindow.h"

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QStandardPaths>

#include "infrastructure/InstalledAppFinderFactory.h"
#include "infrastructure/JsonAppCatalogRepository.h"
#include "infrastructure/ProcessAppLauncher.h"
#include "services/LaunchApp.h"
#include "services/SearchApps.h"

#include <exception>
#include <memory>

namespace
{
// The bundled apps.json is read-only inside an AppImage, so the writable copy lives in the user data folder.
QString userCatalogPath()
{
    const QString dataDirectory = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/FlashBarCSandbox");
    QDir().mkpath(dataDirectory);

    const QString catalogPath = dataDirectory + QStringLiteral("/apps.json");
    const QString bundledPath = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("apps.json"));
    if (!QFileInfo::exists(catalogPath) && QFileInfo::exists(bundledPath)) {
        QFile::copy(bundledPath, catalogPath);
    }
    return catalogPath;
}
}

int main(int argc, char *argv[])
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")
        && !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
        qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("xcb"));
        qunsetenv("GTK_PATH");
    }

    QApplication app(argc, argv);

    try {
        JsonAppCatalogRepository appCatalog(userCatalogPath());
        ProcessAppLauncher processAppLauncher;
        SearchApps searchApps(appCatalog);
        LaunchApp launchApp(processAppLauncher, appCatalog);
        const std::unique_ptr<InstalledAppFinder> installedAppFinder = createInstalledAppFinder();
        MainWindow window(searchApps, launchApp, *installedAppFinder);
        window.show();

        return app.exec();
    } catch (const std::exception &error) {
        QMessageBox::critical(nullptr, QStringLiteral("App catalog error"),
                              QString::fromUtf8(error.what()));
        return 1;
    }
}