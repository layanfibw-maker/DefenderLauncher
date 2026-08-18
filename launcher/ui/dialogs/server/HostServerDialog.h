#pragma once
#include <QDialog>
#include <QProcess>

class QComboBox;
class QLineEdit;
class QPushButton;
class QTextEdit;
class QLabel;
class QNetworkAccessManager;

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
    QPushButton* m_tunnelBtn = nullptr;
    QTextEdit* m_console;
    QComboBox* m_typeBox = nullptr;
    QProcess* m_serverProcess = nullptr;
    QNetworkAccessManager* m_nam = nullptr;
    QProcess* m_playitProcess = nullptr;
    QLabel* m_ipLabel = nullptr;
    QString m_currentIp;

    void downloadServerJar(const QString& version);
    void downloadFabricJar(const QString& version, const QString& serverDir);
    void downloadForgeJar(const QString& version, const QString& serverDir);
    void startPlayit();
    void downloadPlayit();
};
