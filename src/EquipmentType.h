#pragma once

#include "ParameterItem.h"
#include "WorkStateTemplate.h"
#include <QString>
#include <QList>
#include <QJsonObject>

class EquipmentType {
public:
    EquipmentType(const QString& typeId, const QString& typeName);
    ~EquipmentType();

    QString getTypeId() const { return m_typeId; }
    QString getTypeName() const { return m_typeName; }
    int getDeviceCount() const { return m_deviceCount; }
    const QList<ParameterItem*>& getBasicParameters() const { return m_basicParameters; }
    WorkStateTemplate* getWorkStateTemplate() const { return m_workStateTemplate; }
    
    void setDeviceCount(int count) { m_deviceCount = count; }
    void addBasicParameter(ParameterItem* parameter);
    void setWorkStateTemplate(WorkStateTemplate* tmpl);
    
    // 获取基本参数
    ParameterItem* getBasicParameter(const QString& id) const;
    
    // 从JSON加载
    static EquipmentType* fromJson(const QJsonObject& json);

private:
    QString m_typeId;
    QString m_typeName;
    int m_deviceCount = 1;
    QList<ParameterItem*> m_basicParameters;
    WorkStateTemplate* m_workStateTemplate = nullptr;
}; 