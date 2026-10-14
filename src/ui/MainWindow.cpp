#include "MainWindow.h"

#include <QEvent>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QFontMetrics>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPainter>
#include <QPen>
#include <QScreen>
#include <QShowEvent>
#include <QTimer>
#include <QtConcurrent>
#include <QVBoxLayout>

#include "../repositories/InstalledAppFinder.h"
#include "../services/LaunchApp.h"
#include "../services/SearchApps.h"

namespace
{
QIcon iconFor(const std::string &iconValue)
{
    const QString icon = QString::fromStdString(iconValue);
    const QFileInfo iconInfo(icon);
    if (iconInfo.isAbsolute()) {
        const QIcon fileIcon(icon);
        return fileIcon.isNull() ? QFileIconProvider().icon(iconInfo) : fileIcon;
    }
    return QIcon::fromTheme(icon);
}

QPixmap searchIconPixmap(int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(QStringLiteral("#9e9e9e")), 2, Qt::SolidLine, Qt::RoundCap));
    painter.drawEllipse(QPointF(size * 0.44, size * 0.44), size * 0.30, size * 0.30);
    painter.drawLine(QPointF(size * 0.66, size * 0.66), QPointF(size * 0.88, size * 0.88));
    return pixmap;
}
}

MainWindow::MainWindow(SearchApps &searchApps, LaunchApp &launchApp, const InstalledAppFinder &installedAppFinder,
                       QWidget *parent)
    : QMainWindow(parent)
    , searchApps_(searchApps)
    , launchApp_(launchApp)
{
    setWindowTitle(QStringLiteral("FlashBarCSandbox"));
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    const QFontMetrics fontMetrics(font());
    const int rowHeight = 36;
    const int searchBarHeight = 44;
    const int listPadding = 4;
    const int outerMargin = 12;
    const int spacing = 8;
    const int contentWidth = qMax(320, fontMetrics.horizontalAdvance(QString(10, QLatin1Char('W'))) + 48);

    auto *searchBar = new QWidget(this);
    searchBar->setObjectName(QStringLiteral("searchBar"));
    searchBar->setFixedHeight(searchBarHeight);
    searchBar->setStyleSheet(QStringLiteral("QWidget#searchBar { background-color: #2b2b2b; border-radius: 8px; }"));

    auto *searchIcon = new QLabel(searchBar);
    searchIcon->setPixmap(searchIconPixmap(18));
    searchIcon->setStyleSheet(QStringLiteral("background: transparent;"));

    searchField_ = new QLineEdit(searchBar);
    searchField_->setPlaceholderText(QStringLiteral("Start typing..."));
    searchField_->setStyleSheet(QStringLiteral("QLineEdit { background: transparent; color: #ffffff; border: none; font-size: 15px; }"));
    searchField_->installEventFilter(this);

    auto *searchLayout = new QHBoxLayout(searchBar);
    searchLayout->setContentsMargins(14, 0, 14, 0);
    searchLayout->setSpacing(10);
    searchLayout->addWidget(searchIcon);
    searchLayout->addWidget(searchField_);

    resultsList_ = new QListWidget(this);
    resultsList_->installEventFilter(this);
    resultsList_->setIconSize(QSize(24, 24));
    resultsList_->setUniformItemSizes(true);
    resultsList_->setFrameShape(QFrame::NoFrame);
    resultsList_->setFixedHeight(rowHeight * 5 + 2 * listPadding);
    resultsList_->setStyleSheet(QStringLiteral(
        "QListWidget { background-color: #2b2b2b; color: #ffffff; border: none; border-radius: 8px; padding: 4px; outline: none; }"
        "QListWidget::item { min-height: 36px; padding-left: 8px; border-radius: 6px; }"
        "QListWidget::item:selected { background-color: #3d3d3d; color: #ffffff; }"
        "QListWidget::item:hover { background-color: #353535; }"));

    connect(searchField_, &QLineEdit::textChanged, this, &MainWindow::updateResults);
    connect(resultsList_, &QListWidget::itemDoubleClicked, this, &MainWindow::launchItem);
    updateResults(QString());

    auto *layout = new QVBoxLayout;
    layout->setContentsMargins(outerMargin, outerMargin, outerMargin, outerMargin);
    layout->setSpacing(spacing);
    layout->addWidget(searchBar);
    layout->addWidget(resultsList_);

    auto *centralWidget = new QWidget(this);
    centralWidget->setObjectName(QStringLiteral("centralWidget"));
    centralWidget->setStyleSheet(QStringLiteral("QWidget#centralWidget { background-color: rgba(28, 28, 28, 204); border-radius: 12px; }"));
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);
    setFixedSize(contentWidth, outerMargin * 2 + searchBarHeight + spacing + resultsList_->height());

    using DiscoveryWatcher = QFutureWatcher<std::vector<domain::AppEntry>>;
    auto *discoveryWatcher = new DiscoveryWatcher(this);
    connect(discoveryWatcher, &DiscoveryWatcher::finished, this, [this, discoveryWatcher] {
        searchApps_.setDiscoveredApps(discoveryWatcher->result());
        updateResults(searchField_->text());
        discoveryWatcher->deleteLater();
    });
    const InstalledAppFinder *finder = &installedAppFinder;
    discoveryWatcher->setFuture(QtConcurrent::run([finder] { return finder->scan(); }));
}

void MainWindow::updateResults(const QString &query)
{
    resultsList_->clear();
    visibleApps_ = searchApps_.execute(query.toStdString());
    for (std::size_t index = 0; index < visibleApps_.size(); ++index) {
        const domain::AppEntry &appEntry = visibleApps_[index];
        auto *item = new QListWidgetItem(
            iconFor(appEntry.icon),
            QString::fromStdString(appEntry.name)
                + (appEntry.source == domain::AppSource::Discovered ? QStringLiteral(" (found)") : QString()));
        item->setData(Qt::UserRole, static_cast<int>(index));
        resultsList_->addItem(item);
    }
    if (resultsList_->count() > 0) {
        resultsList_->setCurrentRow(0);
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if ((watched == searchField_ || watched == resultsList_) && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            close();
            return true;
        }
        if (watched == searchField_) {
            const int resultCount = resultsList_->count();
            if (keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_Up) {
                if (resultCount == 0) {
                    return true;
                }
                const int currentRow = resultsList_->currentRow();
                const int nextRow = keyEvent->key() == Qt::Key_Down
                    ? (currentRow < 0 ? 0 : qMin(currentRow + 1, resultCount - 1))
                    : (currentRow < 0 ? resultCount - 1 : qMax(currentRow - 1, 0));
                resultsList_->setCurrentRow(nextRow);
                return true;
            }
            if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
                if (resultsList_->currentRow() < 0 && resultCount > 0) {
                    resultsList_->setCurrentRow(0);
                }
                launchItem(resultsList_->currentItem());
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::launchItem(QListWidgetItem *item)
{
    if (!item || !item->data(Qt::UserRole).isValid()) {
        return;
    }

    const int index = item->data(Qt::UserRole).toInt();
    if (index >= 0 && static_cast<std::size_t>(index) < visibleApps_.size()
        && launchApp_.execute(visibleApps_[static_cast<std::size_t>(index)])) {
        close();
    }
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    QTimer::singleShot(0, this, [this] {
        if (QScreen *screen = QGuiApplication::primaryScreen()) {
            move(screen->geometry().center() - rect().center());
        }
    });
}