#include "disk_widget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QHeaderView>

namespace cleaner {
namespace gui {

DiskWidget::DiskWidget(QWidget* parent)
    : QWidget(parent)
    , diskTable_(new QTableWidget(this))
    , refreshButton_(new QPushButton("Refresh", this))
    , unlockButton_(new QPushButton("Unlock", this))
    , formatButton_(new QPushButton("Format", this))
    , wipeButton_(new QPushButton("Secure Wipe", this))
    , formatTypeCombo_(new QComboBox(this))
    , labelEdit_(new QLineEdit(this))
    , selectedRow_(-1)
{
    setupUI();
    refreshDisks();
}

void DiskWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Disk table
    diskTable_->setColumnCount(6);
    diskTable_->setHorizontalHeaderLabels(
        {"Device", "Size", "Filesystem", "Mount Point", "Removable", "Read-Only"});
    diskTable_->horizontalHeader()->setStretchLastSection(false);
    diskTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    diskTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    diskTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    diskTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(diskTable_, &QTableWidget::itemSelectionChanged,
            this, &DiskWidget::onDiskSelected);

    mainLayout->addWidget(diskTable_);

    // Operations group
    QGroupBox* opsGroup = new QGroupBox("Disk Operations", this);
    QHBoxLayout* opsLayout = new QHBoxLayout(opsGroup);

    opsLayout->addWidget(refreshButton_);
    opsLayout->addWidget(unlockButton_);

    // Format section
    opsLayout->addWidget(new QLabel("Format:", this));
    formatTypeCombo_->addItems({"ext4", "NTFS", "FAT32", "exFAT"});
    opsLayout->addWidget(formatTypeCombo_);

    opsLayout->addWidget(new QLabel("Label:", this));
    labelEdit_->setPlaceholderText("Volume label");
    opsLayout->addWidget(labelEdit_);
    opsLayout->addWidget(formatButton_);

    opsLayout->addWidget(wipeButton_);
    opsLayout->addStretch();

    mainLayout->addWidget(opsGroup);

    // Connect buttons
    connect(refreshButton_, &QPushButton::clicked, this, &DiskWidget::refreshDisks);
    connect(unlockButton_, &QPushButton::clicked, this, &DiskWidget::onUnlockClicked);
    connect(formatButton_, &QPushButton::clicked, this, &DiskWidget::onFormatClicked);
    connect(wipeButton_, &QPushButton::clicked, this, &DiskWidget::onWipeClicked);

    // Disable operations until disk is selected
    unlockButton_->setEnabled(false);
    formatButton_->setEnabled(false);
    wipeButton_->setEnabled(false);
}

void DiskWidget::refreshDisks() {
    emit statusMessage("Scanning disks...");
    disks_ = disk_ops::list_disks();
    updateDiskTable();
    emit statusMessage(QString("Found %1 disk(s)").arg(disks_.size()), 2000);
}

void DiskWidget::updateDiskTable() {
    diskTable_->setRowCount(disks_.size());

    for (size_t i = 0; i < disks_.size(); ++i) {
        const auto& disk = disks_[i];

        diskTable_->setItem(i, 0, new QTableWidgetItem(
            QString::fromStdString(disk.device_path)));

        // Size in GB
        double sizeGB = disk.total_size / (1024.0 * 1024.0 * 1024.0);
        diskTable_->setItem(i, 1, new QTableWidgetItem(
            QString::number(sizeGB, 'f', 2) + " GB"));

        diskTable_->setItem(i, 2, new QTableWidgetItem(
            QString::fromStdString(disk.filesystem)));

        diskTable_->setItem(i, 3, new QTableWidgetItem(
            QString::fromStdString(disk.mount_point)));

        diskTable_->setItem(i, 4, new QTableWidgetItem(
            disk.is_removable ? "Yes" : "No"));

        diskTable_->setItem(i, 5, new QTableWidgetItem(
            disk.is_read_only ? "Yes" : "No"));

        // Highlight read-only disks
        if (disk.is_read_only) {
            for (int col = 0; col < 6; ++col) {
                diskTable_->item(i, col)->setBackground(QColor(255, 200, 200));
            }
        }
    }
}

void DiskWidget::onDiskSelected() {
    auto selected = diskTable_->selectedItems();
    if (selected.isEmpty()) {
        selectedRow_ = -1;
        unlockButton_->setEnabled(false);
        formatButton_->setEnabled(false);
        wipeButton_->setEnabled(false);
        return;
    }

    selectedRow_ = selected.first()->row();
    unlockButton_->setEnabled(true);
    formatButton_->setEnabled(true);
    wipeButton_->setEnabled(true);

    if (selectedRow_ >= 0 && selectedRow_ < static_cast<int>(disks_.size())) {
        showDiskInfo(disks_[selectedRow_]);
    }
}

void DiskWidget::showDiskInfo(const platform::DiskInfo& info) {
    QString msg = QString("Selected: %1 (%2 GB)")
        .arg(QString::fromStdString(info.device_path))
        .arg(info.total_size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    emit statusMessage(msg);
}

void DiskWidget::onUnlockClicked() {
    if (selectedRow_ < 0) return;

    const auto& disk = disks_[selectedRow_];

    auto reply = QMessageBox::question(this, "Unlock Disk",
        QString("Attempt to unlock write-protection on %1?")
            .arg(QString::fromStdString(disk.device_path)),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        emit statusMessage("Unlocking disk...");
        auto result = disk_ops::unlock_disk(disk.device_path);

        if (result == platform::ResultCode::SUCCESS) {
            QMessageBox::information(this, "Success", "Disk unlocked successfully!");
            refreshDisks();
        } else {
            QMessageBox::warning(this, "Failed",
                "Failed to unlock disk. Check permissions or hardware switch.");
        }
    }
}

void DiskWidget::onFormatClicked() {
    if (selectedRow_ < 0) return;

    const auto& disk = disks_[selectedRow_];
    QString fsType = formatTypeCombo_->currentText();

    auto reply = QMessageBox::warning(this, "Format Disk",
        QString("WARNING: This will erase all data on %1!\n\nFormat as %2?\n\nThis cannot be undone!")
            .arg(QString::fromStdString(disk.device_path))
            .arg(fsType),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        emit statusMessage("Formatting disk...");

        disk_ops::FormatOptions options;
        options.label = labelEdit_->text().toStdString();

        // Map GUI selection to enum
        if (fsType == "ext4") options.filesystem = disk_ops::FilesystemType::EXT4;
        else if (fsType == "NTFS") options.filesystem = disk_ops::FilesystemType::NTFS;
        else if (fsType == "FAT32") options.filesystem = disk_ops::FilesystemType::FAT32;
        else if (fsType == "exFAT") options.filesystem = disk_ops::FilesystemType::EXFAT;

        auto result = disk_ops::format_disk(disk.device_path, options);

        if (result == platform::ResultCode::SUCCESS) {
            QMessageBox::information(this, "Success", "Disk formatted successfully!");
            refreshDisks();
        } else {
            QMessageBox::critical(this, "Failed", "Format operation failed!");
        }
    }
}

void DiskWidget::onWipeClicked() {
    if (selectedRow_ < 0) return;

    const auto& disk = disks_[selectedRow_];

    auto reply = QMessageBox::warning(this, "Secure Wipe",
        QString("WARNING: This will securely erase ALL data on %1!\n\n"
                "This operation cannot be undone and will take significant time!\n\n"
                "Continue?")
            .arg(QString::fromStdString(disk.device_path)),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        emit statusMessage("Wiping disk (this will take a while)...");

        // TODO: Add progress dialog
        auto result = disk_ops::secure_wipe_disk(disk.device_path, 3);

        if (result == platform::ResultCode::SUCCESS) {
            QMessageBox::information(this, "Success", "Disk wiped successfully!");
            refreshDisks();
        } else {
            QMessageBox::critical(this, "Failed", "Wipe operation failed!");
        }
    }
}

} // namespace gui
} // namespace cleaner
