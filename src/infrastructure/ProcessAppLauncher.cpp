#include "ProcessAppLauncher.h"

#include <QProcess>
#include <QStringList>

bool ProcessAppLauncher::launch(const domain::AppEntry &appEntry) const
{
    const QStringList commandParts = QProcess::splitCommand(QString::fromStdString(appEntry.command));
    return !commandParts.isEmpty()
        && QProcess::startDetached(commandParts.first(), commandParts.mid(1));
}