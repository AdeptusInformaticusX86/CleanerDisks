#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>

#ifdef __linux__
#include <unistd.h>
#endif

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Set application metadata
    QApplication::setApplicationName("CleanerFirmware");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("CleanerFirmware");

#ifdef __linux__
    // Check if running as root on Linux
    if (geteuid() != 0) {
        QMessageBox::warning(nullptr, "CleanerFirmware",
            "Some disk operations require root privileges.\n\n"
            "Please run with: sudo ./cleanerfirmware-gui\n\n"
            "File deletion operations will work without root.",
            QMessageBox::Ok);
    }
#endif

    cleaner::gui::MainWindow window;
    window.show();

    return app.exec();
}
