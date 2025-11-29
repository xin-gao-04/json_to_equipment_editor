#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QJsonObject>

class TypeEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit TypeEditDialog(QWidget* parent = nullptr);
    void setType(const QJsonObject& obj);
    QJsonObject type() const;

private slots:
    void onAccept();

private:
    QLineEdit* m_idEdit;
    QLineEdit* m_nameEdit;
    QSpinBox* m_deviceCountSpin;
};
