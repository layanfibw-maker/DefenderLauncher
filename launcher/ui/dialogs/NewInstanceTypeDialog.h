#pragma once
#include <QDialog>

class NewInstanceTypeDialog : public QDialog {
    Q_OBJECT
public:
    enum Choice { Fabric, Forge, Custom };
    explicit NewInstanceTypeDialog(QWidget* parent = nullptr);
    Choice choice() const { return m_choice; }
private:
    Choice m_choice = Custom;
};
