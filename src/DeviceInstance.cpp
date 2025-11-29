#include "DeviceInstance.h"
#include <QDebug>

DeviceInstance::DeviceInstance(const QString& deviceId, const QString& deviceName, EquipmentType* equipmentType)
    : m_deviceId(deviceId), m_deviceName(deviceName), m_equipmentType(equipmentType)
{
    // 初始化基本参数的默认值
    if (m_equipmentType) {
        for (const ParameterItem* param : m_equipmentType->getBasicParameters()) {
            m_basicValues[param->getId()] = param->getDefaultValue();
        }
        
        // 初始化工作状态
        int defaultStateCount = getWorkStateCount();
        setWorkStateCount(defaultStateCount);
    }
}

QVariant DeviceInstance::getBasicValue(const QString& parameterId) const
{
    return m_basicValues.value(parameterId);
}

void DeviceInstance::setBasicValue(const QString& parameterId, const QVariant& value)
{
    m_basicValues[parameterId] = value;
}

int DeviceInstance::getWorkStateCount() const
{
    // 从基本参数中获取工作状态个数
    QVariant countValue = getBasicValue("work_state_count");
    if (countValue.isValid()) {
        return countValue.toInt();
    }
    
    // 默认返回1
    return 0;
}

void DeviceInstance::setWorkStateCount(int count)
{
    // 更新基本参数中的工作状态个数
    setBasicValue("work_state_count", count);
    qDebug() << QString(u8"当前工作状态列表大小为%1，需要增加到%2").arg(m_workStateValues.size()).arg(count);
    
    // 调整工作状态值列表大小
    while (m_workStateValues.size() < count) {
        // 添加新的工作状态，使用默认值
        QVariantMap defaultStateValues;
        if (m_equipmentType && m_equipmentType->getWorkStateTemplate()) {
            const auto& parameters = m_equipmentType->getWorkStateTemplate()->getParameters();
            for (const ParameterItem* param : parameters) {
                defaultStateValues[param->getId()] = param->getDefaultValue();
            }
        }
        m_workStateValues.append(defaultStateValues);
    }
    
    while (m_workStateValues.size() > count) {
        m_workStateValues.removeLast();
    }
}

QVariantMap DeviceInstance::getWorkStateValues(int stateIndex) const
{
    if (stateIndex >= 0 && stateIndex < m_workStateValues.size()) {
        return m_workStateValues[stateIndex];
    }
    else
    {
        if (m_workStateValues.size() > 0)
        {
            return m_workStateValues[0];
        }
        return QVariantMap();
    }
}

void DeviceInstance::setWorkStateValues(int stateIndex, const QVariantMap& values)
{
    if (stateIndex >= 0 && stateIndex < m_workStateValues.size()) {
        m_workStateValues[stateIndex] = values;
    }
}

bool DeviceInstance::isEnabled() const
{
    // 检查设备是否启用，根据不同设备类型的启用参数
    if (m_equipmentType) {
        QString typeId = m_equipmentType->getTypeId();
        QString typeName = m_equipmentType->getTypeName();
        
        // 根据设备类型检查对应的"参数出现"字段
        QStringList possibleEnabledParams;
        
        if (typeId.contains("radar") || typeName.contains("雷达")) {
            possibleEnabledParams << "radar_enabled" << "雷达参数出现";
        } else if (typeId.contains("comm") || typeName.contains("通信")) {
            possibleEnabledParams << "comm_enabled" << "通信参数出现";
        }
        
        // 添加通用的启用参数
        possibleEnabledParams << "enabled" << "参数出现";
        
        // 检查任何一个启用参数
        for (const QString& paramId : possibleEnabledParams) {
            QVariant enabledValue = getBasicValue(paramId);
            if (enabledValue.isValid()) {
                // 如果参数值为数字类型，检查是否不为0
                bool ok;
                int intValue = enabledValue.toInt(&ok);
                if (ok) {
                    return intValue != 0;
                }
                
                // 如果参数值为字符串类型，检查是否不为空且不为"0"
                QString stringValue = enabledValue.toString().trimmed();
                if (!stringValue.isEmpty() && stringValue != "0" && stringValue.toLower() != "false") {
                    return true;
                }
            }
        }
    }
    
    return true; // 默认启用
} 