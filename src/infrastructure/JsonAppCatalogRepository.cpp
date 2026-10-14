#include "JsonAppCatalogRepository.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>

#include <stdexcept>
#include <string>
#include <utility>

JsonAppCatalogRepository::JsonAppCatalogRepository(QString filePath)
    : filePath_(std::move(filePath))
{
}

std::vector<domain::AppEntry> JsonAppCatalogRepository::loadApps() const
{
    QFile appsFile(filePath_);
    if (!appsFile.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Unable to open app catalog: " + appsFile.errorString().toStdString());
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(appsFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        throw std::runtime_error("Invalid app catalog JSON: " + parseError.errorString().toStdString());
    }
    if (!document.isObject()) {
        throw std::runtime_error("App catalog JSON must be an object");
    }

    const QJsonValue appsValue = document.object().value(QStringLiteral("apps"));
    if (!appsValue.isArray()) {
        throw std::runtime_error("App catalog must contain an apps array");
    }

    std::vector<domain::AppEntry> apps;
    const QJsonArray appArray = appsValue.toArray();
    apps.reserve(static_cast<std::size_t>(appArray.size()));
    for (const QJsonValue &value : appArray) {
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject object = value.toObject();
        domain::AppEntry appEntry{
            object.value(QStringLiteral("name")).toString().toStdString(),
            object.value(QStringLiteral("command")).toString().toStdString(),
            object.value(QStringLiteral("icon")).toString().toStdString(),
            object.value(QStringLiteral("path")).toString().toStdString()
        };
        if (!appEntry.name.empty() && !appEntry.command.empty()) {
            apps.push_back(std::move(appEntry));
        }
    }

    return apps;
}

bool JsonAppCatalogRepository::saveApp(const domain::AppEntry &appEntry)
{
    QJsonObject root;
    QJsonArray appArray;

    if (QFileInfo::exists(filePath_)) {
        QFile appsFile(filePath_);
        if (!appsFile.open(QIODevice::ReadOnly)) {
            return false;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(appsFile.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            return false;
        }

        root = document.object();
        appArray = root.value(QStringLiteral("apps")).toArray();
    }

    const QString command = QString::fromStdString(appEntry.command);
    for (const QJsonValue &value : appArray) {
        if (value.toObject().value(QStringLiteral("command")).toString() == command) {
            return true;
        }
    }

    QJsonObject object;
    object.insert(QStringLiteral("name"), QString::fromStdString(appEntry.name));
    object.insert(QStringLiteral("command"), command);
    object.insert(QStringLiteral("icon"), QString::fromStdString(appEntry.icon));
    if (!appEntry.path.empty()) {
        object.insert(QStringLiteral("path"), QString::fromStdString(appEntry.path));
    }
    appArray.append(object);
    root.insert(QStringLiteral("apps"), appArray);

    QSaveFile saveFile(filePath_);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        return false;
    }
    if (saveFile.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0) {
        return false;
    }
    return saveFile.commit();
}
