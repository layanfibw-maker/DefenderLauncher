#pragma once
#include <QObject>
#include <QString>
#include <QStringList>

class RubisModInstaller : public QObject {
    Q_OBJECT
public:
    enum ModLoader { Fabric, Forge };
    explicit RubisModInstaller(const QString& instancePath, ModLoader loader, QObject* parent = nullptr);
    void install();
signals:
    void finished();
    void error(const QString& message);
private:
    QString m_instancePath;
    ModLoader m_loader;
    QStringList m_fabricMods = {"sodium", "lithium", "ferritecore", "entityculling", "immediatelyfast"};
    QStringList m_forgeMods = {"rubidium", "canary", "ferritecore", "entityculling", "immediatelyfast"};
    void downloadMod(const QString& modId);
};
