#include "ParameterItem.h"
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QJsonArray>
#include <QDebug>
#include <QRegularExpressionValidator>

ParameterItem::ParameterItem(const QString& id, const QString& label, const QString& type)
    : m_id(id), m_label(label), m_type(type)
{
}

bool ParameterItem::validate(const QVariant& value) const
{
    if (m_type == "int") {
        bool ok;
        int intVal = value.toInt(&ok);
        if (!ok) return false;
        return intVal >= m_minValue && intVal <= m_maxValue;
    }
    else if (m_type == "double") {
        bool ok;
        double doubleVal = value.toDouble(&ok);
        if (!ok) return false;
        return doubleVal >= m_minValue && doubleVal <= m_maxValue;
    }
    else if (m_type == "string") {
        const QString text = value.toString();
        if (text.isEmpty()) {
            return false;
        }
        return stringAllowedPattern().match(text).hasMatch();
    }
    else if (m_type == "enum") {
        return m_options.contains(value.toString());
    }
    
    return true;
}

QWidget* ParameterItem::createEditor(QWidget* parent)
{
    if (m_editor) {
        return m_editor;
    }
    
    if (m_type == "int" || m_type == "Byte") {
        QSpinBox* spinBox = new QSpinBox(parent);
        spinBox->setRange(static_cast<int>(m_minValue), static_cast<int>(m_maxValue));
        spinBox->setValue(m_currentValue.isValid() ? m_currentValue.toInt() : m_defaultValue.toInt());
        
        QObject::connect(spinBox,
                         static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                         [this](int value) {
            m_currentValue = value;
        });
        
        m_editor = spinBox;
    }
    else if (m_type == "double") {
        QDoubleSpinBox* doubleSpinBox = new QDoubleSpinBox(parent);
        doubleSpinBox->setRange(m_minValue, m_maxValue);
        doubleSpinBox->setDecimals(2);
        doubleSpinBox->setValue(m_currentValue.isValid() ? m_currentValue.toDouble() : m_defaultValue.toDouble());
        
        QObject::connect(doubleSpinBox,
                         static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                         [this](double value) {
            m_currentValue = value;
        });
        
        m_editor = doubleSpinBox;
    }
    else if (m_type == "string") {
        QLineEdit* lineEdit = new QLineEdit(parent);
        lineEdit->setText(m_currentValue.isValid() ? m_currentValue.toString() : m_defaultValue.toString());
        lineEdit->setValidator(new QRegularExpressionValidator(stringAllowedPattern(), lineEdit));
        
        QObject::connect(lineEdit, &QLineEdit::textChanged, [this](const QString& text) {
            m_currentValue = text;
        });
        
        m_editor = lineEdit;
    }
    else if (m_type == "enum") {
        QComboBox* comboBox = new QComboBox(parent);
        comboBox->addItems(m_options);
        
        QString currentVal = m_currentValue.isValid() ? m_currentValue.toString() : m_defaultValue.toString();
        int index = m_options.indexOf(currentVal);
        if (index >= 0) {
            comboBox->setCurrentIndex(index);
        }
        
        QObject::connect(comboBox,
                         static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                         [this, comboBox](int index) {
            if (index >= 0) {
                m_currentValue = comboBox->itemText(index);
            }
        });
        
        m_editor = comboBox;
    }
    
    return m_editor;
}

QVariant ParameterItem::getValue() const
{
    return m_currentValue.isValid() ? m_currentValue : m_defaultValue;
}

void ParameterItem::setValue(const QVariant& value)
{
    m_currentValue = value;
    
    // 更新编辑器显示
    if (m_editor) {
        if (m_type == "int") {
            QSpinBox* spinBox = qobject_cast<QSpinBox*>(m_editor);
            if (spinBox) {
                spinBox->setValue(value.toInt());
            }
        }
        else if (m_type == "double") {
            QDoubleSpinBox* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(m_editor);
            if (doubleSpinBox) {
                doubleSpinBox->setValue(value.toDouble());
            }
        }
        else if (m_type == "string") {
            QLineEdit* lineEdit = qobject_cast<QLineEdit*>(m_editor);
            if (lineEdit) {
                lineEdit->setText(value.toString());
            }
        }
        else if (m_type == "enum") {
            QComboBox* comboBox = qobject_cast<QComboBox*>(m_editor);
            if (comboBox) {
                int index = m_options.indexOf(value.toString());
                if (index >= 0) {
                    comboBox->setCurrentIndex(index);
                }
            }
        }
    }
}

void ParameterItem::setVisible(bool visible)
{
    if (m_editor) {
        m_editor->setVisible(visible);
    }
}

bool ParameterItem::isVisible() const
{
    if (m_editor) {
        return m_editor->isVisible();
    }
    return true; // 默认可见
}

ParameterItem* ParameterItem::fromJson(const QJsonObject& json)
{
    QString id = json["id"].toString();
    QString label = json["label"].toString();
    QString type = json["type"].toString();
    
    ParameterItem* item = new ParameterItem(id, label, type);
    
    if (json.contains("unit")) {
        item->setUnit(json["unit"].toString());
    }
    
    if (json.contains("default")) {
        QVariant defaultValue;
        if (type == "int") {
            defaultValue = json["default"].toInt();
        } else if (type == "double") {
            defaultValue = json["default"].toDouble();
        } else {
            defaultValue = json["default"].toString();
        }
        item->setDefaultValue(defaultValue);
    }
    
    // 兼容 range 或 min/max 的数值范围定义
    double minVal = item->getMinValue();
    double maxVal = item->getMaxValue();
    if (json.contains("range")) {
        QJsonArray range = json["range"].toArray();
        if (range.size() == 2) {
            minVal = range[0].toDouble();
            maxVal = range[1].toDouble();
        }
    }
    if (json.contains("min")) {
        minVal = json["min"].toDouble();
    }
    if (json.contains("max")) {
        maxVal = json["max"].toDouble();
    }
    item->setRange(minVal, maxVal);
    
    if (json.contains("options")) {
        QJsonArray options = json["options"].toArray();
        QStringList optionList;
        for (const auto& option : options) {
            optionList << option.toString();
        }
        item->setOptions(optionList);
    }
    
    return item;
}

QRegularExpression ParameterItem::stringAllowedPattern()
{
    // 仅允许数字、空白、逗号、方括号、点和负号，便于输入数组或数值列表
    return QRegularExpression(QStringLiteral("^[0-9\\s,\\[\\]\\.-]*$"));
}
