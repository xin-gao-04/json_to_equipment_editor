#include "WorkStateTemplate.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QJsonArray>
#include <QDebug>

WorkStateTemplate::WorkStateTemplate(const QString& templateId, const QString& name)
    : m_templateId(templateId), m_name(name)
{
}

WorkStateTemplate::~WorkStateTemplate()
{
    qDeleteAll(m_parameters);
}

void WorkStateTemplate::addParameter(ParameterItem* parameter)
{
    if (parameter) {
        m_parameters.append(parameter);
    }
}

ParameterItem* WorkStateTemplate::getParameter(const QString& id) const
{
    for (ParameterItem* param : m_parameters) {
        if (param->getId() == id) {
            return param;
        }
    }
    return nullptr;
}

QWidget* WorkStateTemplate::createStateWidget(int stateIndex, QWidget* parent)
{
    QWidget* widget = new QWidget(parent);
    QFormLayout* layout = new QFormLayout(widget);
    
    for (ParameterItem* param : m_parameters) {
        QWidget* editor = param->createEditor(widget);
        
        // 设置这个状态实例的值
        if (m_stateValues.contains(stateIndex) && m_stateValues[stateIndex].contains(param->getId())) {
            param->setValue(m_stateValues[stateIndex][param->getId()]);
        } else {
            // 使用默认值
            param->setValue(param->getDefaultValue());
            if (!m_stateValues.contains(stateIndex)) {
                m_stateValues[stateIndex] = QVariantMap();
            }
            m_stateValues[stateIndex][param->getId()] = param->getDefaultValue();
        }
        
        QString labelText = param->getLabel();
        if (!param->getUnit().isEmpty()) {
            labelText += QString(" (%1)").arg(param->getUnit());
        }
        
        layout->addRow(labelText, editor);
    }
    
    return widget;
}

QVariantMap WorkStateTemplate::getStateValues(int stateIndex) const
{
    if (m_stateValues.contains(stateIndex)) {
        return m_stateValues[stateIndex];
    }
    
    // 返回默认值
    QVariantMap defaultValues;
    for (const ParameterItem* param : m_parameters) {
        defaultValues[param->getId()] = param->getDefaultValue();
    }
    return defaultValues;
}

void WorkStateTemplate::setStateValues(int stateIndex, const QVariantMap& values)
{
    m_stateValues[stateIndex] = values;
    
    // 更新参数项的值
    for (auto it = values.begin(); it != values.end(); ++it) {
        ParameterItem* param = getParameter(it.key());
        if (param) {
            param->setValue(it.value());
        }
    }
}

WorkStateTemplate* WorkStateTemplate::fromJson(const QJsonObject& json)
{
    QString templateId = json["template_id"].toString();
    QString name = json.contains("template_name") ? json["template_name"].toString() : json["name"].toString();
    
    WorkStateTemplate* tmpl = new WorkStateTemplate(templateId, name);
    
    if (json.contains("parameters")) {
        QJsonArray parameters = json["parameters"].toArray();
        for (const auto& paramValue : parameters) {
            QJsonObject paramObj = paramValue.toObject();
            ParameterItem* param = ParameterItem::fromJson(paramObj);
            if (param) {
                tmpl->addParameter(param);
            }
        }
    }
    
    // 可见性规则：controller -> cases(value -> showIds)
    if (json.contains("visibility_rules")) {
        QJsonArray rules = json["visibility_rules"].toArray();
        for (const auto& ruleVal : rules) {
            QJsonObject ruleObj = ruleVal.toObject();
            VisibilityRule rule;
            rule.controllerId = ruleObj["controller"].toString();
            
            if (ruleObj.contains("cases")) {
                QJsonArray cases = ruleObj["cases"].toArray();
                for (const auto& caseVal : cases) {
                    QJsonObject caseObj = caseVal.toObject();
                    VisibilityCase c;
                    c.value = caseObj["value"].toString();
                    if (caseObj.contains("show")) {
                        QJsonArray showArray = caseObj["show"].toArray();
                        for (const auto& sid : showArray) {
                            c.showIds << sid.toString();
                            rule.affectedIds.insert(sid.toString());
                        }
                    }
                    rule.cases.append(c);
                }
            }
            
            if (!rule.controllerId.isEmpty() && !rule.cases.isEmpty()) {
                tmpl->m_visibilityRules.append(rule);
            }
        }
    }
    
    // 选项规则：controller -> target -> options
    if (json.contains("option_rules")) {
        QJsonArray rules = json["option_rules"].toArray();
        for (const auto& ruleVal : rules) {
            QJsonObject ruleObj = ruleVal.toObject();
            OptionRule rule;
            rule.controllerId = ruleObj["controller"].toString();
            rule.targetId = ruleObj["target"].toString();
            if (ruleObj.contains("options_by_value")) {
                QJsonObject mapObj = ruleObj["options_by_value"].toObject();
                for (auto it = mapObj.begin(); it != mapObj.end(); ++it) {
                    QStringList opts;
                    QJsonArray arr = it.value().toArray();
                    for (const auto& o : arr) {
                        opts << o.toString();
                    }
                    rule.optionsByValue[it.key()] = opts;
                }
            }
            if (!rule.controllerId.isEmpty() && !rule.targetId.isEmpty() && !rule.optionsByValue.isEmpty()) {
                tmpl->m_optionRules.append(rule);
            }
        }
    }
    
    return tmpl;
}
