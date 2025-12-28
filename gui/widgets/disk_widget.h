#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>

#include "core/disk_operations.hpp"

namespace cleaner {
namespace gui {

class DiskWidget : public QWidget {
    Q_OBJECT

public:
    explicit DiskWidget(QWidget* parent = nullptr);
    ~DiskWidget() override = default;

public slots:
    void refreshDisks();

signals:
    void statusMessage(const QString& message, int timeout = 3000);

private slots:
    void onDiskSelected();
    void onUnlockClicked();
    void onFormatClicked();
    void onWipeClicked();

private:
    void setupUI();
    void updateDiskTable();
    void showDiskInfo(const platform::DiskInfo& info);

    // UI components
    QTableWidget* diskTable_;
    QPushButton* refreshButton_;
    QPushButton* unlockButton_;
    QPushButton* formatButton_;
    QPushButton* wipeButton_;
    QComboBox* formatTypeCombo_;
    QLineEdit* labelEdit_;

    // Data
    std::vector<platform::DiskInfo> disks_;
    int selectedRow_;
};

} // namespace gui
} // namespace cleaner
