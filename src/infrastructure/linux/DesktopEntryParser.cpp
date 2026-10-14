#include "DesktopEntryParser.h"

#include <QHash>
#include <QStringList>

namespace
{
QString cleanExecCommand(const QString &exec)
{
    QString result;
    for (int index = 0; index < exec.size(); ++index) {
        const QChar character = exec.at(index);
        if (character != QLatin1Char('%') || index + 1 >= exec.size()) {
            result.append(character);
            continue;
        }

        const QChar code = exec.at(++index);
        if (code == QLatin1Char('%')) {
            result.append(QLatin1Char('%'));
        }
    }
    return result.trimmed();
}

bool desktopMatches(const QString &list, const QString &currentDesktop)
{
    const QStringList desktops = currentDesktop.split(QLatin1Char(':'), Qt::SkipEmptyParts);
    const QStringList entries = list.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &entry : entries) {
        if (desktops.contains(entry)) {
            return true;
        }
    }
    return false;
}
}

std::optional<domain::AppEntry> parseDesktopEntry(const QString &contents, const QString &currentDesktop)
{
    QHash<QString, QString> values;
    bool inDesktopEntry = false;

    const QStringList lines = contents.split(QLatin1Char('\n'));
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        if (line.startsWith(QLatin1Char('['))) {
            inDesktopEntry = line == QStringLiteral("[Desktop Entry]");
            continue;
        }
        if (!inDesktopEntry) {
            continue;
        }

        const int separator = line.indexOf(QLatin1Char('='));
        if (separator <= 0) {
            continue;
        }
        const QString key = line.left(separator).trimmed();
        if (key.contains(QLatin1Char('[')) || values.contains(key)) {
            continue;
        }
        values.insert(key, line.mid(separator + 1).trimmed());
    }

    if (values.value(QStringLiteral("Type")) != QStringLiteral("Application")) {
        return std::nullopt;
    }
    if (values.value(QStringLiteral("NoDisplay")) == QStringLiteral("true")
        || values.value(QStringLiteral("Hidden")) == QStringLiteral("true")) {
        return std::nullopt;
    }
    if (values.contains(QStringLiteral("OnlyShowIn"))
        && !desktopMatches(values.value(QStringLiteral("OnlyShowIn")), currentDesktop)) {
        return std::nullopt;
    }
    if (desktopMatches(values.value(QStringLiteral("NotShowIn")), currentDesktop)) {
        return std::nullopt;
    }

    const QString name = values.value(QStringLiteral("Name"));
    const QString command = cleanExecCommand(values.value(QStringLiteral("Exec")));
    if (name.isEmpty() || command.isEmpty()) {
        return std::nullopt;
    }

    return domain::AppEntry{
        name.toStdString(),
        command.toStdString(),
        values.value(QStringLiteral("Icon")).toStdString(),
        {}
    };
}
