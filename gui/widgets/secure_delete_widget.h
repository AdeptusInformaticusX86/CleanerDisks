#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QCheckBox>

namespace cleaner {
namespace gui {

class SecureDeleteWidget : public QWidget {
    Q_OBJECT

public:
    explicit SecureDeleteWidget(QWidget* parent = nullptr);
    ~SecureDeleteWidget() override = default;

signals:
    void statusMessage(const QString& message, int timeout = 3000);

private slots:
    void onBrowseFile();
    void onBrowseDirectory();
    void onAddToQueue();
    void onRemoveFromQueue();
    void onClearQueue();
    void onDeleteClicked();

private:
    void setupUI();
    QString getMethodDescription(const QString& method) const;

    // UI components
    QLineEdit* pathEdit_;
    QPushButton* browseFileButton_;
    QPushButton* browseDirButton_;
    QComboBox* methodCombo_;
    QCheckBox* recursiveCheck_;
    QPushButton* addButton_;

    QListWidget* queueList_;
    QPushButton* removeButton_;
    QPushButton* clearButton_;
    QPushButton* deleteButton_;
};

} // namespace gui
} // namespace cleaner
