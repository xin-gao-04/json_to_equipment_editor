#include "TypeEditDialog.h"
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QMessageBox>

TypeEditDialog::TypeEditDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Edit Equipment Type");
    setMinimumWidth(320);

    m_idEdit = new QLineEdit;
    m_nameEdit = new QLineEdit;
    m_deviceCountSpin = new QSpinBox;
    m_deviceCountSpin->setRange(0, 64);
    m_deviceCountSpin->setValue(1);

    QFormLayout* form = new QFormLayout;
    form->addRow("Type ID", m_idEdit);
    form->addRow("Type Name", m_nameEdit);
    form->addRow("Device Count", m_deviceCountSpin);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &TypeEditDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &TypeEditDialog::reject);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

void TypeEditDialog::setType(const QJsonObject& obj)
{
    m_idEdit->setText(obj["type_id"].toString());
    m_nameEdit->setText(obj["type_name"].toString());
    m_deviceCountSpin->setValue(obj["device_count"].toInt(1));
}

QJsonObject TypeEditDialog::type() const
{
    QJsonObject obj;
    obj["type_id"] = m_idEdit->text().trimmed();
    obj["type_name"] = m_nameEdit->text().trimmed();
    obj["device_count"] = m_deviceCountSpin->value();
    return obj;
}

void TypeEditDialog::onAccept()
{
    if (m_idEdit->text().trimmed().isEmpty() || m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation failed", "Type ID and Type Name cannot be empty");
        return;
    }
    accept();
}
