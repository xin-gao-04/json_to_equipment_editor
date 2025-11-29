#pragma once

#include <QDialog>
#include <QJsonObject>
#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QLabel>
#include <QDialogButtonBox>

class ParameterEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit ParameterEditDialog(QWidget* parent = nullptr);
    void setParameter(const QJsonObject& obj);
    QJsonObject parameter() const;

private slots:
    void onTypeChanged(int index);
    void onAccept();

private:
    QLineEdit* m_idEdit;
    QLineEdit* m_labelEdit;
    QComboBox* m_typeCombo;
    QLineEdit* m_defaultEdit;
    QLineEdit* m_minEdit;
    QLineEdit* m_maxEdit;
    QTextEdit* m_optionsEdit;
    QLineEdit* m_unitEdit;
    QLabel* m_errorLabel;
    QDialogButtonBox* m_buttons;

    void toggleFields(const QString& type);
    bool validate(QString& error) const;
};
