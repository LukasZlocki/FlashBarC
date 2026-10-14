#pragma once

#include "../domain/AppEntry.h"

#include <QMainWindow>
#include <QString>

#include <vector>

class QEvent;
class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QObject;
class QShowEvent;
class LaunchApp;
class SearchApps;
class InstalledAppFinder;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(SearchApps &searchApps, LaunchApp &launchApp, const InstalledAppFinder &installedAppFinder,
               QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    SearchApps &searchApps_;
    LaunchApp &launchApp_;
    std::vector<domain::AppEntry> visibleApps_;
    QLineEdit *searchField_ = nullptr;
    QListWidget *resultsList_ = nullptr;

    void updateResults(const QString &query);
    void launchItem(QListWidgetItem *item);
};