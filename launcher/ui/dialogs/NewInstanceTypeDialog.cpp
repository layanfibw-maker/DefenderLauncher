#include "NewInstanceTypeDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

NewInstanceTypeDialog::NewInstanceTypeDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Créer une instance"));
    setMinimumWidth(400);

    auto* layout = new QVBoxLayout(this);

    auto* label = new QLabel(tr("Quel type d'instance veux-tu créer ?"));
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("font-size: 14px; font-weight: bold; margin: 10px;");
    layout->addWidget(label);

    // Bouton Fabric optimisé
    auto* fabricBtn = new QPushButton(tr("⚡ Fabric optimisé\n(Sodium, Lithium, FerriteCore...)"));
    fabricBtn->setMinimumHeight(60);
    fabricBtn->setStyleSheet("text-align: center; font-size: 13px;");
    layout->addWidget(fabricBtn);

    // Bouton Forge optimisé
    auto* forgeBtn = new QPushButton(tr("⚙ Forge optimisé\n(Rubidium, Canary, FerriteCore...)"));
    forgeBtn->setMinimumHeight(60);
    forgeBtn->setStyleSheet("text-align: center; font-size: 13px;");
    layout->addWidget(forgeBtn);

    // Bouton Custom
    auto* customBtn = new QPushButton(tr("🎮 Instance personnalisée\n(Choisir moi-même)"));
    customBtn->setMinimumHeight(60);
    customBtn->setStyleSheet("text-align: center; font-size: 13px;");
    layout->addWidget(customBtn);

    connect(fabricBtn, &QPushButton::clicked, this, [this]() {
        m_choice = Fabric;
        accept();
    });
    connect(forgeBtn, &QPushButton::clicked, this, [this]() {
        m_choice = Forge;
        accept();
    });
    connect(customBtn, &QPushButton::clicked, this, [this]() {
        m_choice = Custom;
        accept();
    });
}
