#include "HostServerDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QProcess>
#include <QDir>
#include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

HostServerDialog::HostServerDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Héberger un serveur Minecraft"));
    setMinimumSize(600, 400);

    auto* layout = new QVBoxLayout(this);

    // Version
    auto* versionLayout = new QHBoxLayout();
    versionLayout->addWidget(new QLabel(tr("Version Minecraft:")));
    m_versionBox = new QComboBox();
    m_versionBox->addItems({"1.21.4", "1.21.1", "1.20.4", "1.20.1", "1.19.4", "1.18.2", "1.16.5", "1.12.2", "1.8.9"});
    versionLayout->addWidget(m_versionBox);
    layout->addLayout(versionLayout);

    // RAM
    auto* ramLayout = new QHBoxLayout();
    ramLayout->addWidget(new QLabel(tr("RAM (Mo):")));
    m_ramBox = new QLineEdit("2048");
    ramLayout->addWidget(m_ramBox);
    layout->addLayout(ramLayout);

    // Boutons
    auto* btnLayout = new QHBoxLayout();
    m_launchBtn = new QPushButton(tr("Lancer le serveur"));
    m_stopBtn = new QPushButton(tr("Arrêter"));
    m_stopBtn->setEnabled(false);
    btnLayout->addWidget(m_launchBtn);
    btnLayout->addWidget(m_stopBtn);
    layout->addLayout(btnLayout);

    // Console
    m_console = new QTextEdit();
    m_console->setReadOnly(true);
    m_console->setStyleSheet("background-color: #1a0505; color: #ffffff; font-family: monospace;");
    layout->addWidget(m_console);

    connect(m_launchBtn, &QPushButton::clicked, this, &HostServerDialog::onLaunch);
    connect(m_stopBtn, &QPushButton::clicked, this, &HostServerDialog::onStop);
}

HostServerDialog::~HostServerDialog()
{
    if (m_serverProcess && m_serverProcess->state() == QProcess::Running) {
        m_serverProcess->write("stop\n");
        m_serverProcess->waitForFinished(3000);
        m_serverProcess->kill();
    }
}

void HostServerDialog::onLaunch()
{
    QString version = m_versionBox->currentText();
    QString ram = m_ramBox->text();
    QString serverDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/servers/" + version;
    QDir().mkpath(serverDir);
    QString jarPath = serverDir + "/server.jar";

    if (!QFile::exists(jarPath)) {
        m_console->append(tr("Téléchargement du serveur %1...").arg(version));
        downloadServerJar(version);
    }

    if (!QFile::exists(jarPath)) {
        m_console->append(tr("Erreur: impossible de télécharger le serveur."));
        return;
    }

    // Accepter l'EULA
    QFile eula(serverDir + "/eula.txt");
    if (eula.open(QIODevice::WriteOnly)) {
        eula.write("eula=true\n");
        eula.close();
    }

    m_serverProcess = new QProcess(this);
    m_serverProcess->setWorkingDirectory(serverDir);
    connect(m_serverProcess, &QProcess::readyReadStandardOutput, this, &HostServerDialog::onOutput);
    connect(m_serverProcess, &QProcess::readyReadStandardError, this, &HostServerDialog::onOutput);

    QStringList args = {"-Xmx" + ram + "M", "-Xms512M", "-jar", jarPath, "--nogui"};
    m_serverProcess->start("java", args);

    m_launchBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    m_console->append(tr("Serveur lancé sur le port 25565 !"));
    m_console->append(tr("Tes amis peuvent rejoindre avec ton IP locale."));
}

void HostServerDialog::onStop()
{
    if (m_serverProcess) {
        m_serverProcess->write("stop\n");
        m_serverProcess->waitForFinished(5000);
        m_serverProcess->kill();
        m_serverProcess = nullptr;
    }
    m_launchBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    m_console->append(tr("Serveur arrêté."));
}

void HostServerDialog::onOutput()
{
    if (m_serverProcess) {
        m_console->append(QString::fromUtf8(m_serverProcess->readAllStandardOutput()));
        m_console->append(QString::fromUtf8(m_serverProcess->readAllStandardError()));
    }
}

void HostServerDialog::downloadServerJar(const QString& version)
{
    QNetworkAccessManager manager;
    QEventLoop loop;

    // Cherche l'URL du server jar sur le manifest Mojang
    QNetworkRequest req(QUrl("https://launchermeta.mojang.com/mc/game/version_manifest.json"));
    auto* reply = manager.get(req);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    reply->deleteLater();

    for (auto v : doc.object()["versions"].toArray()) {
        if (v.toObject()["id"].toString() == version) {
            QString versionUrl = v.toObject()["url"].toString();
            auto* vReply = manager.get(QNetworkRequest(QUrl(versionUrl)));
            connect(vReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();

            QJsonDocument vDoc = QJsonDocument::fromJson(vReply->readAll());
            vReply->deleteLater();

            QString serverUrl = vDoc.object()["downloads"].toObject()["server"].toObject()["url"].toString();
            if (serverUrl.isEmpty()) {
                m_console->append(tr("Pas de serveur disponible pour cette version."));
                return;
            }

            auto* dlReply = manager.get(QNetworkRequest(QUrl(serverUrl)));
            connect(dlReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();

            QString serverDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/servers/" + version;
            QFile file(serverDir + "/server.jar");
            if (file.open(QIODevice::WriteOnly)) {
                file.write(dlReply->readAll());
                file.close();
                m_console->append(tr("Serveur téléchargé !"));
            }
            dlReply->deleteLater();
            return;
        }
    }
    m_console->append(tr("Version introuvable dans le manifest Mojang."));
}
