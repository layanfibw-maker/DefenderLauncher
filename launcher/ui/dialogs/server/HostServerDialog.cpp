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
#include <QNetworkRequest>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkInterface>
#include <QClipboard>
#include <QApplication>

HostServerDialog::HostServerDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Heberger un serveur Minecraft"));
    setMinimumSize(600, 400);

    auto* layout = new QVBoxLayout(this);

    auto* versionLayout = new QHBoxLayout();
    versionLayout->addWidget(new QLabel(tr("Version Minecraft:")));
    m_versionBox = new QComboBox();
    m_versionBox->addItems({"1.21.4", "1.21.1", "1.20.4", "1.20.1", "1.19.4", "1.18.2", "1.16.5", "1.12.2", "1.8.9"});
    versionLayout->addWidget(m_versionBox);
    layout->addLayout(versionLayout);

    auto* ramLayout = new QHBoxLayout();
    ramLayout->addWidget(new QLabel(tr("RAM (Mo):")));
    m_ramBox = new QLineEdit("2048");
    ramLayout->addWidget(m_ramBox);
    layout->addLayout(ramLayout);

    auto* typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel(tr("Type de serveur:")));
    m_typeBox = new QComboBox();
    m_typeBox->addItems({tr("Vanilla"), tr("Fabric optimise"), tr("Forge optimise")});
    typeLayout->addWidget(m_typeBox);
    layout->addLayout(typeLayout);

    auto* btnLayout = new QHBoxLayout();
    m_launchBtn = new QPushButton(tr("Lancer le serveur"));
    m_stopBtn = new QPushButton(tr("Arreter"));
    m_stopBtn->setEnabled(false);
    m_tunnelBtn = new QPushButton(tr("Tunnel public (playit.gg)"));
    m_tunnelBtn->setEnabled(false);
    btnLayout->addWidget(m_launchBtn);
    btnLayout->addWidget(m_stopBtn);
    btnLayout->addWidget(m_tunnelBtn);
    layout->addLayout(btnLayout);

    m_console = new QTextEdit();
    m_console->setReadOnly(true);
    m_console->setStyleSheet("background-color: #1a0505; color: #ffffff; font-family: monospace;");
    layout->addWidget(m_console);

    auto* ipLayout = new QHBoxLayout();
    m_ipLabel = new QLabel(tr("IP locale : -"));
    m_ipLabel->setStyleSheet("font-weight: bold;");
    auto* copyBtn = new QPushButton(tr("Copier"));
    copyBtn->setMaximumWidth(80);
    ipLayout->addWidget(m_ipLabel);
    ipLayout->addWidget(copyBtn);
    layout->addLayout(ipLayout);

    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::finished, this, &HostServerDialog::onDownloadFinished);

    connect(copyBtn, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_currentIp);
    });
    connect(m_launchBtn, &QPushButton::clicked, this, &HostServerDialog::onLaunch);
    connect(m_stopBtn, &QPushButton::clicked, this, &HostServerDialog::onStop);
    connect(m_tunnelBtn, &QPushButton::clicked, this, &HostServerDialog::startPlayit);
}

HostServerDialog::~HostServerDialog()
{
    if (m_serverProcess && m_serverProcess->state() == QProcess::Running) {
        m_serverProcess->write("stop\n");
        m_serverProcess->waitForFinished(3000);
        m_serverProcess->kill();
    }
    if (m_playitProcess && m_playitProcess->state() == QProcess::Running) {
        m_playitProcess->kill();
    }
}

void HostServerDialog::onLaunch()
{
    m_version = m_versionBox->currentText();
    m_ram = m_ramBox->text();
    m_serverType = m_typeBox->currentIndex();
    m_serverDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/servers/" + m_version;
    QDir().mkpath(m_serverDir);

    QString jarPath = m_serverDir + "/server.jar";
    if (!QFile::exists(jarPath)) {
        m_console->append(tr("Telechargement du manifest Mojang..."));
        m_launchBtn->setEnabled(false);
        m_step = ManifestStep;
        startDownload();
    } else {
        launchServer();
    }
}

void HostServerDialog::startDownload()
{
    if (m_step == ManifestStep) {
        QUrl url("https://launchermeta.mojang.com/mc/game/version_manifest.json");
        QNetworkRequest req(url);
        m_nam->get(req);
    } else if (m_step == VersionStep) {
        QUrl url(m_versionUrl);
        QNetworkRequest req(url);
        m_nam->get(req);
    } else if (m_step == ServerJarStep) {
        m_console->append(tr("Telechargement du server.jar..."));
        QUrl url(m_serverJarUrl);
        QNetworkRequest req(url);
        m_nam->get(req);
    } else if (m_step == FabricStep) {
        m_console->append(tr("Telechargement de Fabric..."));
        QUrl fabricUrl("https://meta.fabricmc.net/v2/versions/loader/" + m_version + "/0.16.5/1.0.1/server/jar");
        QNetworkRequest req(fabricUrl);
        m_nam->get(req);
    }
}

void HostServerDialog::onDownloadFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        m_console->append(tr("Erreur: ") + reply->errorString());
        m_launchBtn->setEnabled(true);
        return;
    }

    if (m_step == ManifestStep) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        for (auto v : doc.object()["versions"].toArray()) {
            if (v.toObject()["id"].toString() == m_version) {
                m_versionUrl = v.toObject()["url"].toString();
                m_step = VersionStep;
                startDownload();
                return;
            }
        }
        m_console->append(tr("Version introuvable."));
        m_launchBtn->setEnabled(true);

    } else if (m_step == VersionStep) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        m_serverJarUrl = doc.object()["downloads"].toObject()["server"].toObject()["url"].toString();
        if (m_serverJarUrl.isEmpty()) {
            m_console->append(tr("Pas de serveur pour cette version."));
            m_launchBtn->setEnabled(true);
            return;
        }
        m_step = ServerJarStep;
        startDownload();

    } else if (m_step == ServerJarStep) {
        QFile file(m_serverDir + "/server.jar");
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            m_console->append(tr("server.jar telecharge !"));
        }
        if (m_serverType == 1) {
            m_step = FabricStep;
            startDownload();
        } else {
            launchServer();
        }

    } else if (m_step == FabricStep) {
        QFile file(m_serverDir + "/fabric-server-launch.jar");
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            m_console->append(tr("Fabric telecharge !"));
        }
        launchServer();
    }
}

void HostServerDialog::launchServer()
{
    QFile eula(m_serverDir + "/eula.txt");
    if (eula.open(QIODevice::WriteOnly)) {
        eula.write("eula=true\n");
        eula.close();
    }

    QString jarPath = m_serverDir + "/server.jar";
    if (m_serverType == 1) {
        QString fabricJar = m_serverDir + "/fabric-server-launch.jar";
        if (QFile::exists(fabricJar))
            jarPath = fabricJar;
    }

    if (m_serverProcess) {
        m_serverProcess->kill();
        m_serverProcess->deleteLater();
        m_serverProcess = nullptr;
    }

    m_serverProcess = new QProcess(this);
    m_serverProcess->setWorkingDirectory(m_serverDir);
    connect(m_serverProcess, &QProcess::readyReadStandardOutput, this, &HostServerDialog::onOutput);
    connect(m_serverProcess, &QProcess::readyReadStandardError, this, &HostServerDialog::onOutput);

    QStringList args = {"-Xmx" + m_ram + "M", "-Xms512M", "-jar", jarPath, "--nogui"};
    m_serverProcess->start("java", args);

    m_launchBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    m_tunnelBtn->setEnabled(true);

    for (auto& iface : QNetworkInterface::allInterfaces()) {
        for (auto& addr : iface.addressEntries()) {
            if (addr.ip().protocol() == QAbstractSocket::IPv4Protocol && !addr.ip().isLoopback()) {
                m_currentIp = addr.ip().toString();
                m_ipLabel->setText(tr("IP locale : ") + m_currentIp + ":25565");
                break;
            }
        }
    }

    m_console->append(tr("Serveur lance sur le port 25565 !"));
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
    if (m_playitProcess && m_playitProcess->state() == QProcess::Running) {
        m_playitProcess->kill();
        m_playitProcess = nullptr;
    }
    m_launchBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    m_tunnelBtn->setEnabled(false);
    m_console->append(tr("Serveur arrete."));
}

void HostServerDialog::onOutput()
{
    if (m_serverProcess) {
        m_console->append(QString::fromUtf8(m_serverProcess->readAllStandardOutput()));
        m_console->append(QString::fromUtf8(m_serverProcess->readAllStandardError()));
    }
}

void HostServerDialog::startPlayit()
{
    QString playitPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/playit.exe";
    if (!QFile::exists(playitPath)) {
        m_console->append(tr("Telechargement de playit.gg..."));
        QUrl playitUrl("https://github.com/playit-cloud/playit-agent/releases/latest/download/playit-windows_64.exe");
        QNetworkRequest req(playitUrl);
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        m_nam->get(req);
        return;
    }

    m_playitProcess = new QProcess(this);
    connect(m_playitProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        QString output = QString::fromUtf8(m_playitProcess->readAllStandardOutput());
        m_console->append(output);
        for (auto& line : output.split('\n')) {
            if (line.contains(".joinmc.link") || line.contains(".playit.gg")) {
                m_ipLabel->setText(tr("Adresse publique : ") + line.trimmed());
                m_currentIp = line.trimmed();
            }
        }
    });
    connect(m_playitProcess, &QProcess::readyReadStandardError, this, [this]() {
        QString output = QString::fromUtf8(m_playitProcess->readAllStandardError());
        m_console->append(output);
        for (auto& line : output.split('\n')) {
            if (line.contains(".joinmc.link") || line.contains(".playit.gg")) {
                m_ipLabel->setText(tr("Adresse publique : ") + line.trimmed());
                m_currentIp = line.trimmed();
            }
        }
    });
    m_playitProcess->start(playitPath, {});
    m_console->append(tr("playit.gg lance !"));
    m_tunnelBtn->setEnabled(false);
}
