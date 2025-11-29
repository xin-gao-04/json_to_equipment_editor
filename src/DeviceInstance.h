#pragma once

#include "EquipmentType.h"
#include <QString>
#include <QVariantMap>
#include <QList>

class DeviceInstance {
public:
    DeviceInstance(const QString& deviceId, const QString& deviceName, EquipmentType* equipmentType);
    
    QString getDeviceId() const { return m_deviceId; }
    QString getDeviceName() const { return m_deviceName; }
    EquipmentType* getEquipmentType() const { return m_equipmentType; }
    
    // 基本参数值管理
    QVariantMap getBasicValues() const { return m_basicValues; }
    QVariantMap getAllBasicValues() const { return m_basicValues; }
    void setBasicValues(const QVariantMap& values) { m_basicValues = values; }
    QVariant getBasicValue(const QString& parameterId) const;
    void setBasicValue(const QString& parameterId, const QVariant& value);
    
    // 工作状态管理
    int getWorkStateCount() const;
    void setWorkStateCount(int count);
    QVariantMap getWorkStateValues(int stateIndex) const;
    void setWorkStateValues(int stateIndex, const QVariantMap& values);
    
    // 设备启用状态
    bool isEnabled() const;
    
private:
    QString m_deviceId;
    QString m_deviceName;
    EquipmentType* m_equipmentType;
    QVariantMap m_basicValues;
    QList<QVariantMap> m_workStateValues;
}; 