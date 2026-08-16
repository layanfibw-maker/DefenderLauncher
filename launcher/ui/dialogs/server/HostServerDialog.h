#pragma once
#include <QDialog>
#include <QProcess>

class QComboBox;
class QLineEdit;
class QPushButton;
class QTextEdit;

class HostServerDialog : public QDialog {
    Q_OBJECT
public:
    explicit HostServerDialog(QWidget* parent = nullptr);
    ~HostServerDialog();

private slots:
    void onLaunch();
    void onStop();
    void onOutput();

private:
    QComboBox* m_versionBox;
    QLineEdit* m_ramBox;
    QPushButton* m_launchBtn;
    QPushButton* m_stopBtn;
    QTextEdit* m_console;
    QProcess* m_serverProcess = nullptr;

    void downloadServerJar(const QString& version);
};
