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
#include <QNetworkInterface>
#include <QClipboard>
#include <QApplication>

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

    // Type de serveur
    auto* typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel(tr("Type de serveur:")));
    m_typeBox = new QComboBox();
    m_typeBox->addItems({tr("Vanilla"), tr("Fabric optimise"), tr("Forge optimise")});
    typeLayout->addWidget(m_typeBox);
    layout->addLayout(typeLayout);

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

    auto* ipLayout = new QHBoxLayout();
    m_ipLabel = new QLabel(tr("IP locale : -"));
    m_ipLabel->setStyleSheet("font-weight: bold;");
    auto* copyBtn = new QPushButton(tr("Copier"));
    copyBtn->setMaximumWidth(80);
    ipLayout->addWidget(m_ipLabel);
    ipLayout->addWidget(copyBtn);
    layout->addLayout(ipLayout);
    connect(copyBtn, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_currentIp);
    });
    connect(m_launchBtn, &QPushButton::clicked, this, &HostServerDialog::onLaunch);
    connect(m_tunnelBtn, &QPushButton::clicked, this, &HostServerDialog::startPlayit);
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
    int serverType = m_typeBox->currentIndex(); // 0=Vanilla, 1=Fabric, 2=Forge
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

    // Choisir le jar selon le type
    QString loaderJar = jarPath;
    if (serverType == 1) {
        QString fabricJar = serverDir + "/fabric-server-launch.jar";
        if (!QFile::exists(fabricJar)) {
            m_console->append(tr("Telechargement de Fabric..."));
            downloadFabricJar(version, serverDir);
        }
        if (QFile::exists(fabricJar))
            loaderJar = fabricJar;
    } else if (serverType == 2) {
        downloadForgeJar(version, serverDir);
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

    QStringList args = {"-Xmx" + ram + "M", "-Xms512M", "-jar", loaderJar, "--nogui"};
    m_serverProcess->start("java", args);

    m_launchBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    m_tunnelBtn->setEnabled(true);
    for (auto& iface : QNetworkInterface::allInterfaces()) {
        for (auto& addr : iface.addressEntries()) {
            if (addr.ip().protocol() == QAbstractSocket::IPv4Protocol && addr.ip().isLoopback() == false) {
                m_currentIp = addr.ip().toString();
                m_ipLabel->setText(tr("IP locale : ") + m_currentIp + ":25565");
                break;
            }
        }
    }
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
    m_tunnelBtn->setEnabled(false);
    if (m_playitProcess && m_playitProcess->state() == QProcess::Running) {
        m_playitProcess->kill();
        m_playitProcess = nullptr;
    }
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

void HostServerDialog::downloadFabricJar(const QString& version, const QString& serverDir)
{
    QNetworkAccessManager manager;
    QEventLoop loop;

    // Recupere la derniere version de Fabric loader
    QNetworkRequest req(QUrl("https://meta.fabricmc.net/v2/versions/loader/" + version + "/0.16.5/1.0.1/server/jar"));
    auto* reply = manager.get(req);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        m_console->append(tr("Erreur telechargement Fabric: ") + reply->errorString());
        reply->deleteLater();
        return;
    }

    QFile file(serverDir + "/fabric-server-launch.jar");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(reply->readAll());
        file.close();
        m_console->append(tr("Fabric telecharge !"));
    }
    reply->deleteLater();
}

void HostServerDialog::downloadForgeJar(const QString& version, const QString& serverDir)
{
    m_console->append(tr("Forge non supporte pour l'instant, lancement en Vanilla."));
    Q_UNUSED(version);
    Q_UNUSED(serverDir);
}

void HostServerDialog::downloadPlayit()
{
    QString playitPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/playit.exe";
    if (QFile::exists(playitPath)) return;

    m_console->append(tr("Telechargement de playit.gg..."));
    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkRequest req(QUrl("https://github.com/playit-cloud/playit-agent/releases/latest/download/playit-windows_64.exe"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto* reply = manager.get(req);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QFile file(playitPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(reply->readAll());
        file.close();
        m_console->append(tr("playit.gg telecharge !"));
    }
    reply->deleteLater();
}

void HostServerDialog::startPlayit()
{
    downloadPlayit();
    QString playitPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/playit.exe";
    if (!QFile::exists(playitPath)) {
        m_console->append(tr("Erreur: playit.exe introuvable."));
        return;
    }

    m_playitProcess = new QProcess(this);
    connect(m_playitProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        QString output = QString::fromUtf8(m_playitProcess->readAllStandardOutput());
        m_console->append(output);
        // Detecter l'adresse du tunnel
        if (output.contains(".joinmc.link") || output.contains(".playit.gg")) {
            for (auto& line : output.split('\n')) {
                if (line.contains(".joinmc.link") || line.contains(".playit.gg")) {
                    m_ipLabel->setText(tr("Adresse publique : ") + line.trimmed());
                    m_currentIp = line.trimmed();
                }
            }
        }
    });
    connect(m_playitProcess, &QProcess::readyReadStandardError, this, [this]() {
        QString output = QString::fromUtf8(m_playitProcess->readAllStandardError());
        m_console->append(output);
        if (output.contains(".joinmc.link") || output.contains(".playit.gg")) {
            for (auto& line : output.split('\n')) {
                if (line.contains(".joinmc.link") || line.contains(".playit.gg")) {
                    m_ipLabel->setText(tr("Adresse publique : ") + line.trimmed());
                    m_currentIp = line.trimmed();
                }
            }
        }
    });

    m_playitProcess->start(playitPath, {});
    m_console->append(tr("playit.gg lance ! L'adresse publique apparaitra dans la console..."));
    m_tunnelBtn->setEnabled(false);
}
