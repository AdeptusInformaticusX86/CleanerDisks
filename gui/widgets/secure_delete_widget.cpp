#include "secure_delete_widget.h"
#include "core/file_operations.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>

namespace cleaner {
namespace gui {

SecureDeleteWidget::SecureDeleteWidget(QWidget* parent)
    : QWidget(parent)
    , pathEdit_(new QLineEdit(this))
    , browseFileButton_(new QPushButton("Browse File...", this))
    , browseDirButton_(new QPushButton("Browse Directory...", this))
    , methodCombo_(new QComboBox(this))
    , recursiveCheck_(new QCheckBox("Recursive (for directories)", this))
    , addButton_(new QPushButton("Add to Queue", this))
    , queueList_(new QListWidget(this))
    , removeButton_(new QPushButton("Remove", this))
    , clearButton_(new QPushButton("Clear All", this))
    , deleteButton_(new QPushButton("Delete All", this))
{
    setupUI();
}

void SecureDeleteWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Input group
    QGroupBox* inputGroup = new QGroupBox("Select Files/Directories", this);
    QVBoxLayout* inputLayout = new QVBoxLayout(inputGroup);

    QHBoxLayout* pathLayout = new QHBoxLayout();
    pathEdit_->setPlaceholderText("Enter file or directory path...");
    pathLayout->addWidget(new QLabel("Path:", this));
    pathLayout->addWidget(pathEdit_);
    pathLayout->addWidget(browseFileButton_);
    pathLayout->addWidget(browseDirButton_);
    inputLayout->addLayout(pathLayout);

    QHBoxLayout* optionsLayout = new QHBoxLayout();
    optionsLayout->addWidget(new QLabel("Method:", this));

    methodCombo_->addItem("Simple", "simple");
    methodCombo_->addItem("Zero Pass", "zero");
    methodCombo_->addItem("Random Pass", "random");
    methodCombo_->addItem("DoD 3-Pass (Recommended)", "dod3");
    methodCombo_->addItem("DoD 7-Pass", "dod7");
    methodCombo_->addItem("Gutmann 35-Pass", "gutmann");
    methodCombo_->setCurrentIndex(3); // DoD 3-Pass default

    optionsLayout->addWidget(methodCombo_);
    optionsLayout->addWidget(recursiveCheck_);
    optionsLayout->addWidget(addButton_);
    optionsLayout->addStretch();
    inputLayout->addLayout(optionsLayout);

    mainLayout->addWidget(inputGroup);

    // Queue group
    QGroupBox* queueGroup = new QGroupBox("Deletion Queue", this);
    QVBoxLayout* queueLayout = new QVBoxLayout(queueGroup);

    queueLayout->addWidget(queueList_);

    QHBoxLayout* queueButtonsLayout = new QHBoxLayout();
    removeButton_->setEnabled(false);
    queueButtonsLayout->addWidget(removeButton_);
    queueButtonsLayout->addWidget(clearButton_);
    queueButtonsLayout->addStretch();

    deleteButton_->setStyleSheet("QPushButton { background-color: #d9534f; color: white; font-weight: bold; padding: 10px; }");
    deleteButton_->setEnabled(false);
    queueButtonsLayout->addWidget(deleteButton_);

    queueLayout->addLayout(queueButtonsLayout);
    mainLayout->addWidget(queueGroup);

    // Method description
    QLabel* descLabel = new QLabel(
        "<b>Methods:</b><br>"
        "• <b>Simple:</b> Standard file deletion<br>"
        "• <b>Zero Pass:</b> Overwrite with zeros once<br>"
        "• <b>Random Pass:</b> Overwrite with random data once<br>"
        "• <b>DoD 3-Pass:</b> Zero, Ones, Random (DoD 5220.22-M) - Recommended<br>"
        "• <b>DoD 7-Pass:</b> Extended DoD method<br>"
        "• <b>Gutmann 35-Pass:</b> Most secure but very slow",
        this);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 10px; border-radius: 5px; }");
    mainLayout->addWidget(descLabel);

    // Connect signals
    connect(browseFileButton_, &QPushButton::clicked, this, &SecureDeleteWidget::onBrowseFile);
    connect(browseDirButton_, &QPushButton::clicked, this, &SecureDeleteWidget::onBrowseDirectory);
    connect(addButton_, &QPushButton::clicked, this, &SecureDeleteWidget::onAddToQueue);
    connect(removeButton_, &QPushButton::clicked, this, &SecureDeleteWidget::onRemoveFromQueue);
    connect(clearButton_, &QPushButton::clicked, this, &SecureDeleteWidget::onClearQueue);
    connect(deleteButton_, &QPushButton::clicked, this, &SecureDeleteWidget::onDeleteClicked);

    connect(queueList_, &QListWidget::itemSelectionChanged, this, [this]() {
        removeButton_->setEnabled(!queueList_->selectedItems().isEmpty());
    });
}

void SecureDeleteWidget::onBrowseFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "Select File to Securely Delete",
        QDir::homePath());

    if (!fileName.isEmpty()) {
        pathEdit_->setText(fileName);
    }
}

void SecureDeleteWidget::onBrowseDirectory() {
    QString dirName = QFileDialog::getExistingDirectory(this,
        "Select Directory to Securely Delete",
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dirName.isEmpty()) {
        pathEdit_->setText(dirName);
    }
}

void SecureDeleteWidget::onAddToQueue() {
    QString path = pathEdit_->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::warning(this, "Empty Path", "Please enter or select a file/directory.");
        return;
    }

    QString method = methodCombo_->currentText();
    QString methodData = methodCombo_->currentData().toString();

    QString queueItem = QString("%1 [%2]").arg(path).arg(method);
    queueList_->addItem(queueItem);

    pathEdit_->clear();
    emit statusMessage(QString("Added to queue: %1").arg(path), 2000);

    deleteButton_->setEnabled(true);
}

void SecureDeleteWidget::onRemoveFromQueue() {
    auto selected = queueList_->selectedItems();
    if (selected.isEmpty()) return;

    delete selected.first();
    emit statusMessage("Item removed from queue", 2000);

    if (queueList_->count() == 0) {
        deleteButton_->setEnabled(false);
    }
}

void SecureDeleteWidget::onClearQueue() {
    if (queueList_->count() == 0) return;

    auto reply = QMessageBox::question(this, "Clear Queue",
        "Remove all items from queue?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        queueList_->clear();
        deleteButton_->setEnabled(false);
        emit statusMessage("Queue cleared", 2000);
    }
}

void SecureDeleteWidget::onDeleteClicked() {
    if (queueList_->count() == 0) return;

    auto reply = QMessageBox::warning(this, "Secure Delete",
        QString("WARNING: You are about to securely delete %1 item(s).\n\n"
                "This operation CANNOT be undone!\n\n"
                "Are you sure you want to continue?")
            .arg(queueList_->count()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    QProgressDialog progress("Securely deleting files...", "Cancel", 0, queueList_->count(), this);
    progress.setWindowModality(Qt::WindowModal);

    int successCount = 0;
    int failCount = 0;

    for (int i = 0; i < queueList_->count(); ++i) {
        if (progress.wasCanceled()) break;

        QString item = queueList_->item(i)->text();
        // Parse: "path [method]"
        int bracketPos = item.lastIndexOf('[');
        QString path = item.left(bracketPos).trimmed();
        QString methodStr = item.mid(bracketPos + 1, item.length() - bracketPos - 2);

        progress.setLabelText(QString("Deleting: %1\nMethod: %2").arg(path).arg(methodStr));
        progress.setValue(i);

        // Map method string to enum
        file_ops::DeletionMethod method = file_ops::DeletionMethod::SIMPLE;
        if (methodStr.contains("Zero")) method = file_ops::DeletionMethod::ZERO_PASS;
        else if (methodStr.contains("Random")) method = file_ops::DeletionMethod::RANDOM_PASS;
        else if (methodStr.contains("DoD 3")) method = file_ops::DeletionMethod::DOD_3_PASS;
        else if (methodStr.contains("DoD 7")) method = file_ops::DeletionMethod::DOD_7_PASS;
        else if (methodStr.contains("Gutmann")) method = file_ops::DeletionMethod::GUTMANN_35_PASS;

        // TODO: Check if file or directory and call appropriate function
        auto result = file_ops::secure_delete_file(path.toStdString(), method);

        if (result == platform::ResultCode::SUCCESS) {
            successCount++;
        } else {
            failCount++;
        }
    }

    progress.setValue(queueList_->count());

    // Show results
    QString resultMsg = QString("Deletion complete!\n\nSuccess: %1\nFailed: %2")
        .arg(successCount).arg(failCount);

    if (failCount > 0) {
        QMessageBox::warning(this, "Deletion Complete", resultMsg);
    } else {
        QMessageBox::information(this, "Deletion Complete", resultMsg);
    }

    queueList_->clear();
    deleteButton_->setEnabled(false);
    emit statusMessage("Deletion operation completed", 3000);
}

QString SecureDeleteWidget::getMethodDescription(const QString& method) const {
    // Not used in current implementation but kept for future use
    return method;
}

} // namespace gui
} // namespace cleaner
