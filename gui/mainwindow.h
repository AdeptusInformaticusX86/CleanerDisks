#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QStatusBar>
#include <QMenuBar>

namespace cleaner {
namespace gui {

class DiskWidget;
class SecureDeleteWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void showAbout();
    void refreshDevices();
    void onStatusMessage(const QString& message, int timeout = 3000);

private:
    void setupUI();
    void createMenuBar();
    void createStatusBar();

    // Widgets
    QTabWidget* tabWidget_;
    DiskWidget* diskWidget_;
    SecureDeleteWidget* secureDeleteWidget_;
    QStatusBar* statusBar_;
};

} // namespace gui
} // namespace cleaner
