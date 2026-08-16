#include "RubisModInstaller.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QEventLoop>

RubisModInstaller::RubisModInstaller(const QString& instancePath, ModLoader loader, QObject* parent)
    : QObject(parent), m_instancePath(instancePath), m_loader(loader) {}

void RubisModInstaller::install()
{
    QStringList mods = (m_loader == Fabric) ? m_fabricMods : m_forgeMods;
    for (const QString& modId : mods) {
        downloadMod(modId);
    }
    emit finished();
}

void RubisModInstaller::downloadMod(const QString& modId)
{
    QNetworkAccessManager manager;
    QString loaderStr = (m_loader == Fabric) ? "fabric" : "forge";
    
    // Cherche la derniere version du mod sur Modrinth
    QUrl url(QString("https://api.modrinth.com/v2/project/%1/version?loaders=[\"%2\"]").arg(modId, loaderStr));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "RubisLauncher/1.0");
    
    QEventLoop loop;
    QNetworkReply* reply = manager.get(request);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() != QNetworkReply::NoError) {
        emit error(QString("Erreur téléchargement %1: %2").arg(modId, reply->errorString()));
        reply->deleteLater();
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    reply->deleteLater();
    
    QJsonArray versions = doc.array();
    if (versions.isEmpty()) return;
    
    // Prend la premiere version compatible
    QJsonObject version = versions[0].toObject();
    QJsonArray files = version["files"].toArray();
    if (files.isEmpty()) return;
    
    QString downloadUrl = files[0].toObject()["url"].toString();
    QString filename = files[0].toObject()["filename"].toString();
    
    // Telecharge le fichier
    QNetworkRequest dlRequest(downloadUrl);
    QNetworkReply* dlReply = manager.get(dlRequest);
    connect(dlReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (dlReply->error() != QNetworkReply::NoError) {
        dlReply->deleteLater();
        return;
    }
    
    // Sauvegarde dans le dossier mods
    QDir modsDir(m_instancePath + "/mods");
    modsDir.mkpath(".");
    
    QFile file(modsDir.filePath(filename));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(dlReply->readAll());
        file.close();
    }
    dlReply->deleteLater();
}
