#include "EquipmentType.h"
#include <QJsonArray>
#include <QDebug>

EquipmentType::EquipmentType(const QString& typeId, const QString& typeName)
    : m_typeId(typeId), m_typeName(typeName)
{
}

EquipmentType::~EquipmentType()
{
    qDeleteAll(m_basicParameters);
    delete m_workStateTemplate;
}

void EquipmentType::addBasicParameter(ParameterItem* parameter)
{
    if (parameter) {
        m_basicParameters.append(parameter);
    }
}

void EquipmentType::setWorkStateTemplate(WorkStateTemplate* tmpl)
{
    if (m_workStateTemplate != tmpl) {
        delete m_workStateTemplate;
        m_workStateTemplate = tmpl;
    }
}

ParameterItem* EquipmentType::getBasicParameter(const QString& id) const
{
    for (ParameterItem* param : m_basicParameters) {
        if (param->getId() == id) {
            return param;
        }
    }
    return nullptr;
}

EquipmentType* EquipmentType::fromJson(const QJsonObject& json)
{
    QString typeId = json["type_id"].toString();
    QString typeName = json["type_name"].toString();
    
    EquipmentType* equipmentType = new EquipmentType(typeId, typeName);
    
    if (json.contains("device_count")) {
        equipmentType->setDeviceCount(json["device_count"].toInt());
    }
    
    // 加载基本参数
    if (json.contains("basic_parameters")) {
        QJsonArray basicParams = json["basic_parameters"].toArray();
        for (const auto& paramValue : basicParams) {
            QJsonObject paramObj = paramValue.toObject();
            ParameterItem* param = ParameterItem::fromJson(paramObj);
            if (param) {
                equipmentType->addBasicParameter(param);
            }
        }
    }
    
    // 加载工作状态模板
    if (json.contains("work_state_template")) {
        QJsonObject templateObj = json["work_state_template"].toObject();
        WorkStateTemplate* tmpl = WorkStateTemplate::fromJson(templateObj);
        if (tmpl) {
            equipmentType->setWorkStateTemplate(tmpl);
        }
    }
    
    return equipmentType;
} 