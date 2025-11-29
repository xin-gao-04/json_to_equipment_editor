#include "ParameterEditDialog.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QRegExp>
#include <QJsonArray>
#include <QStringList>
#include <QRegularExpression>

ParameterEditDialog::ParameterEditDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(u8"编辑参数");
    setMinimumWidth(400);

    m_idEdit = new QLineEdit;
    m_labelEdit = new QLineEdit;
    m_typeCombo = new QComboBox;
    m_typeCombo->addItems({"int", "double", "string", "enum", "Byte"});
    m_defaultEdit = new QLineEdit;
    m_minEdit = new QLineEdit;
    m_maxEdit = new QLineEdit;
    m_optionsEdit = new QTextEdit;
    m_unitEdit = new QLineEdit;
    m_errorLabel = new QLabel;
    m_errorLabel->setStyleSheet("color: #c62828;");

    connect(m_typeCombo,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ParameterEditDialog::onTypeChanged);

    QFormLayout* form = new QFormLayout;
    form->addRow(u8"ID", m_idEdit);
    form->addRow(u8"名称", m_labelEdit);
    form->addRow(u8"类型", m_typeCombo);
    form->addRow(u8"默认值", m_defaultEdit);
    form->addRow(u8"最小值", m_minEdit);
    form->addRow(u8"最大值", m_maxEdit);
    form->addRow(u8"枚举选项(每行一个)", m_optionsEdit);
    form->addRow(u8"单位", m_unitEdit);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &ParameterEditDialog::onAccept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &ParameterEditDialog::reject);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_errorLabel);
    layout->addWidget(m_buttons);

    toggleFields(m_typeCombo->currentText());
}

void ParameterEditDialog::setParameter(const QJsonObject& obj)
{
    m_idEdit->setText(obj["id"].toString());
    m_labelEdit->setText(obj["label"].toString());
    QString type = obj["type"].toString();
    int idx = m_typeCombo->findText(type);
    if (idx >= 0) {
        m_typeCombo->setCurrentIndex(idx);
    }
    m_defaultEdit->setText(obj["default"].toVariant().toString());
    if (obj.contains("range")) {
        QJsonArray r = obj["range"].toArray();
        if (r.size() == 2) {
            m_minEdit->setText(QString::number(r[0].toDouble()));
            m_maxEdit->setText(QString::number(r[1].toDouble()));
        }
    } else {
        m_minEdit->setText(obj["min"].toVariant().toString());
        m_maxEdit->setText(obj["max"].toVariant().toString());
    }
    if (obj.contains("options")) {
        QStringList opts;
        for (const auto& v : obj["options"].toArray()) {
            opts << v.toString();
        }
        m_optionsEdit->setPlainText(opts.join("\n"));
    }
    m_unitEdit->setText(obj["unit"].toString());
    toggleFields(type);
}

QJsonObject ParameterEditDialog::parameter() const
{
    QJsonObject obj;
    obj["id"] = m_idEdit->text().trimmed();
    obj["label"] = m_labelEdit->text().trimmed();
    QString type = m_typeCombo->currentText();
    obj["type"] = type;
    obj["unit"] = m_unitEdit->text().trimmed();
    obj["default"] = m_defaultEdit->text();

    if (type == "int" || type == "double" || type == "Byte") {
        double min = m_minEdit->text().toDouble();
        double max = m_maxEdit->text().toDouble();
        obj["range"] = QJsonArray{min, max};
    }
    if (type == "enum") {
        QStringList opts = m_optionsEdit->toPlainText().split(QRegExp("[\\r\\n]+"), QString::SkipEmptyParts);
        QJsonArray arr;
        for (const auto& o : opts) {
            arr.append(o.trimmed());
        }
        obj["options"] = arr;
    }
    return obj;
}

void ParameterEditDialog::toggleFields(const QString& type)
{
    bool isNumber = (type == "int" || type == "double" || type == "Byte");
    bool isEnum = (type == "enum");
    m_minEdit->setEnabled(isNumber);
    m_maxEdit->setEnabled(isNumber);
    m_optionsEdit->setEnabled(isEnum);
}

bool ParameterEditDialog::validate(QString& error) const
{
    if (m_idEdit->text().trimmed().isEmpty()) {
        error = u8"ID 不能为空";
        return false;
    }
    if (m_labelEdit->text().trimmed().isEmpty()) {
        error = u8"名称不能为空";
        return false;
    }
    QString type = m_typeCombo->currentText();
    if ((type == "int" || type == "double" || type == "Byte")) {
        bool ok1 = false, ok2 = false;
        m_minEdit->text().toDouble(&ok1);
        m_maxEdit->text().toDouble(&ok2);
        if (!ok1 || !ok2) {
            error = u8"范围必须是数字";
            return false;
        }
    }
    if (type == "enum") {
        QStringList opts = m_optionsEdit->toPlainText().split(QRegExp("[\\r\\n]+"), QString::SkipEmptyParts);
        if (opts.isEmpty()) {
            error = u8"枚举选项不能为空";
            return false;
        }
    }
    return true;
}

void ParameterEditDialog::onTypeChanged(int)
{
    toggleFields(m_typeCombo->currentText());
}

void ParameterEditDialog::onAccept()
{
    QString err;
    if (!validate(err)) {
        m_errorLabel->setText(err);
        return;
    }
    accept();
}
