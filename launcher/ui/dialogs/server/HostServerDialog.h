#pragma once
#include <QDialog>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class QComboBox;
class QLineEdit;
class QPushButton;
class QTextEdit;
class QLabel;

class HostServerDialog : public QDialog {
    Q_OBJECT
public:
    explicit HostServerDialog(QWidget* parent = nullptr);
    ~HostServerDialog();

private slots:
    void onLaunch();
    void onStop();
    void onOutput();
    void onDownloadFinished(QNetworkReply* reply);

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
    QString m_serverDir;
    QString m_version;
    int m_serverType = 0;
    QString m_ram;

    enum DownloadStep { ManifestStep, VersionStep, ServerJarStep, FabricStep, Done };
    DownloadStep m_step = ManifestStep;
    QString m_versionUrl;
    QString m_serverJarUrl;

    void startDownload();
    void launchServer();
    void startPlayit();
};
