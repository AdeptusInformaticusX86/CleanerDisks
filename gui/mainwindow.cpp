#include "mainwindow.h"
#include "widgets/disk_widget.h"
#include "widgets/secure_delete_widget.h"

#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QVBoxLayout>

namespace cleaner {
namespace gui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , tabWidget_(new QTabWidget(this))
    , diskWidget_(new DiskWidget(this))
    , secureDeleteWidget_(new SecureDeleteWidget(this))
    , statusBar_(new QStatusBar(this))
{
    setupUI();
    createMenuBar();
    createStatusBar();

    setWindowTitle("CleanerFirmware");
    resize(800, 600);
}

void MainWindow::setupUI() {
    // Create tab widget
    tabWidget_->addTab(secureDeleteWidget_, "Secure Delete");
    tabWidget_->addTab(diskWidget_, "Disk Management");

    setCentralWidget(tabWidget_);

    // Connect signals
    connect(diskWidget_, &DiskWidget::statusMessage,
            this, &MainWindow::onStatusMessage);
    connect(secureDeleteWidget_, &SecureDeleteWidget::statusMessage,
            this, &MainWindow::onStatusMessage);
}

void MainWindow::createMenuBar() {
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");

    QAction* refreshAction = new QAction("&Refresh Devices", this);
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, this, &MainWindow::refreshDevices);
    fileMenu->addAction(refreshAction);

    fileMenu->addSeparator();

    QAction* exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
    fileMenu->addAction(exitAction);

    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");

    QAction* aboutAction = new QAction("&About", this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    helpMenu->addAction(aboutAction);
}

void MainWindow::createStatusBar() {
    setStatusBar(statusBar_);
    statusBar_->showMessage("Ready", 3000);
}

void MainWindow::showAbout() {
    QMessageBox::about(this, "About CleanerFirmware",
        "<h3>CleanerFirmware v1.0.0</h3>"
        "<p>Cross-platform secure file deletion and disk management utility.</p>"
        "<p>Features:</p>"
        "<ul>"
        "<li>Secure file deletion (DoD, Gutmann methods)</li>"
        "<li>Disk management and formatting</li>"
        "<li>Write-protected USB unlock</li>"
        "</ul>"
        "<p>Licensed under GPL v3</p>"
        "<p><a href='https://github.com/yourusername/cleanerfirmware'>GitHub Repository</a></p>"
    );
}

void MainWindow::refreshDevices() {
    onStatusMessage("Refreshing devices...");
    diskWidget_->refreshDisks();
    onStatusMessage("Devices refreshed", 2000);
}

void MainWindow::onStatusMessage(const QString& message, int timeout) {
    statusBar_->showMessage(message, timeout);
}

} // namespace gui
} // namespace cleaner
