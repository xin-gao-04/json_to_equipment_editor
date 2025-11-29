#include "EquipmentConfigWidget.h"
#include "DeviceTabWidget.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QApplication>
#include <QTimer>

EquipmentConfigWidget::EquipmentConfigWidget(QWidget* parent)
    : QTabWidget(parent)
{
    setTabPosition(QTabWidget::North);
    setMovable(true);
    setTabsClosable(false);
    
    // 启动定时器定期更新所有设备的可见性
    QTimer* visibilityTimer = new QTimer(this);
    connect(visibilityTimer, &QTimer::timeout, this, &EquipmentConfigWidget::updateAllVisibility);
    visibilityTimer->start(2000); // 每2秒检查一次
}

EquipmentConfigWidget::~EquipmentConfigWidget()
{
    clearAll();
}

void EquipmentConfigWidget::clearAll()
{
    // 清理设备实例
    for (auto& deviceList : m_deviceInstances) {
        qDeleteAll(deviceList);
        deviceList.clear();
    }
    m_deviceInstances.clear();
    
    // 清理设备类型
    qDeleteAll(m_equipmentTypes);
    m_equipmentTypes.clear();
    
    // 清理所有tab
    while (count() > 0) {
        QWidget* widget = this->widget(0);
        removeTab(0);
        delete widget;
    }
}

bool EquipmentConfigWidget::loadFromJson(const QString& jsonFile)
{
    QFile file(jsonFile);
    if (!file.open(QIODevice::ReadOnly)) {
        emit validationError(QString(u8"无法打开配置文件: %1").arg(jsonFile));
        return false;
    }
    
    QByteArray jsonData = file.readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        emit validationError(QString(u8"JSON解析错误: %1").arg(parseError.errorString()));
        return false;
    }
    
    QJsonObject rootObj = doc.object();
    if (!rootObj.contains("equipment_config")) {
        emit validationError(QString(u8"配置文件格式错误：缺少equipment_config节点"));
        return false;
    }
    
    // 清理现有数据
    clearAll();
    
    QJsonObject configObj = rootObj["equipment_config"].toObject();
    if (configObj.contains("equipment_types")) {
        QJsonArray equipmentTypes = configObj["equipment_types"].toArray();
        
        for (const auto& typeValue : equipmentTypes) {
            QJsonObject typeObj = typeValue.toObject();
            EquipmentType* equipType = EquipmentType::fromJson(typeObj);
            
            if (equipType) {
                m_equipmentTypes.append(equipType);
                
                // 创建设备实例 - 优先从device_instances创建
                QList<DeviceInstance*> devices;
                
                if (typeObj.contains("device_instances")) {
                    // 从配置文件中的device_instances创建
                    QJsonArray devicesArray = typeObj["device_instances"].toArray();
                    
                    for (int i = 0; i < devicesArray.size(); ++i) {
                        QJsonObject deviceObj = devicesArray[i].toObject();
                        QString deviceId = deviceObj["device_id"].toString();
                        QString deviceName = deviceObj["device_name"].toString();
                        
                        // 如果ID或名称为空，使用默认生成的
                        if (deviceId.isEmpty()) {
                            deviceId = QString("%1_%2").arg(equipType->getTypeId()).arg(i);
                        }
                        if (deviceName.isEmpty()) {
                            deviceName = QString("%1 %2").arg(equipType->getTypeName()).arg(i + 1);
                        }
                        
                        DeviceInstance* device = new DeviceInstance(deviceId, deviceName, equipType);
                        devices.append(device);
                    }
                    
                    // 更新设备类型的device_count以匹配实际实例数量
                    // 注意：这里需要修改EquipmentType类以支持设置device_count
                    qDebug() << QString(u8"从device_instances创建了 %1 个 %2 设备").arg(devices.size()).arg(equipType->getTypeName());
                } else {
                    // 回退到使用device_count创建
                    for (int i = 0; i < equipType->getDeviceCount(); ++i) {
                        QString deviceId = QString("%1_%2").arg(equipType->getTypeId()).arg(i);
                        QString deviceName = QString("%1 %2").arg(equipType->getTypeName()).arg(i + 1);
                        DeviceInstance* device = new DeviceInstance(deviceId, deviceName, equipType);
                        devices.append(device);
                    }
                    qDebug() << QString(u8"从device_count创建了 %1 个 %2 设备").arg(devices.size()).arg(equipType->getTypeName());
                }
                
                m_deviceInstances[equipType->getTypeId()] = devices;
            }
        }
    }
    
    // 加载设备实例的实际参数值（如果存在）
    loadDeviceInstanceValues(configObj);
    
    // 创建界面
    createEquipmentTypeTabs();
    
    // 设置当前文件路径
    m_currentFilePath = jsonFile;
    emit filePathChanged(m_currentFilePath);
    
    qDebug() << QString(u8"配置加载完成。设备类型数量: %1").arg(m_equipmentTypes.size());
    
    emit configChanged();
    return true;
}

void EquipmentConfigWidget::loadDeviceInstanceValues(const QJsonObject& configObj)
{
    if (!configObj.contains("equipment_types")) {
        return;
    }
    
    QJsonArray equipmentTypes = configObj["equipment_types"].toArray();
    
    for (const auto& typeValue : equipmentTypes) {
        QJsonObject typeObj = typeValue.toObject();
        QString typeId = typeObj["type_id"].toString();
        
        if (!m_deviceInstances.contains(typeId)) {
            continue;
        }
        
        // 加载设备实例的实际参数值
        if (typeObj.contains("device_instances")) {
            QJsonArray devicesArray = typeObj["device_instances"].toArray();
            QList<DeviceInstance*>& devices = m_deviceInstances[typeId];
            
            // 为每个JSON中的设备实例找到对应的DeviceInstance对象
            for (int i = 0; i < devicesArray.size(); ++i) {
                QJsonObject deviceObj = devicesArray[i].toObject();
                QString jsonDeviceId = deviceObj["device_id"].toString();
                
                // 通过设备ID找到对应的DeviceInstance
                DeviceInstance* device = nullptr;
                for (DeviceInstance* dev : devices) {
                    if (dev->getDeviceId() == jsonDeviceId) {
                        device = dev;
                        break;
                    }
                }
                
                // 如果没找到，尝试通过索引匹配（向后兼容）
                if (!device && i < devices.size()) {
                    device = devices[i];
                    qDebug() << QString(u8"警告：设备ID %1 不匹配，使用索引 %2 进行匹配").arg(jsonDeviceId).arg(i);
                }
                
                if (!device) {
                    qDebug() << QString(u8"警告：无法找到设备ID %1 对应的设备实例").arg(jsonDeviceId);
                    continue;
                }
                
                // 加载基本参数值
                if (deviceObj.contains("basic_values")) {
                    QJsonObject basicValuesObj = deviceObj["basic_values"].toObject();
                    for (auto it = basicValuesObj.begin(); it != basicValuesObj.end(); ++it) {
                        QString paramId = it.key();
                        QVariant value = it.value().toVariant();
                        device->setBasicValue(paramId, value);
                    }
                }
                
                // 加载工作状态值
                if (deviceObj.contains("work_states")) {
                    QJsonArray workStatesArray = deviceObj["work_states"].toArray();
                    
                    // 首先根据实际保存的状态数量更新设备的工作状态个数
                    int actualStateCount = workStatesArray.size();
                    if (actualStateCount > 0) {
                        device->setWorkStateCount(actualStateCount);
                    }
                    
                    for (int j = 0; j < workStatesArray.size(); ++j) {
                        QJsonObject stateObj = workStatesArray[j].toObject();
                        int stateIndex = stateObj["state_index"].toInt();
                        
                        if (stateObj.contains("values")) {
                            QJsonObject stateValuesObj = stateObj["values"].toObject();
                            QVariantMap stateValues;
                            
                            for (auto it = stateValuesObj.begin(); it != stateValuesObj.end(); ++it) {
                                stateValues[it.key()] = it.value().toVariant();
                            }
                            
                            device->setWorkStateValues(stateIndex, stateValues);
                        }
                    }
                }
            }
        }
    }
}

bool EquipmentConfigWidget::autoSave()
{
    if (!hasCurrentFile()) {
        emit validationError(u8"没有当前文件，无法自动保存。请先打开或保存文件。");
        return false;
    }
    
    return saveToJson(m_currentFilePath);
}

bool EquipmentConfigWidget::saveCurrentValues()
{
    // 这个方法只更新当前内存中的值，不写文件
    // 主要用于在保存前确保所有UI的值都同步到设备实例中
    
    // 触发所有设备更新其参数值
    emit configChanged();
    
    return true;
}

bool EquipmentConfigWidget::saveToJson(const QString& jsonFile)
{
    // 先尝试读取原有文件内容
    QJsonObject rootObj;
    
    QFile existingFile(jsonFile);
    if (existingFile.exists() && existingFile.open(QIODevice::ReadOnly)) {
        QByteArray existingJsonData = existingFile.readAll();
        existingFile.close();
        
        QJsonParseError parseError;
        QJsonDocument existingDoc = QJsonDocument::fromJson(existingJsonData, &parseError);
        
        if (parseError.error == QJsonParseError::NoError) {
            rootObj = existingDoc.object();
            qDebug() << QString(u8"读取到原有JSON文件内容，保留其他对象");
        } else {
            qDebug() << QString(u8"原有JSON文件解析失败，创建新的根对象: %1").arg(parseError.errorString());
            rootObj = QJsonObject(); // 创建新的根对象
        }
    } else {
        qDebug() << QString(u8"原有JSON文件不存在或无法读取，创建新的根对象");
        rootObj = QJsonObject(); // 创建新的根对象
    }
    
    QJsonObject configObj;
    QJsonArray equipmentTypesArray;
    
    // 序列化所有设备类型
    for (EquipmentType* equipType : m_equipmentTypes) {
        QJsonObject typeObj;
        typeObj["type_id"] = equipType->getTypeId();
        typeObj["type_name"] = equipType->getTypeName();
        
        // 使用实际的设备实例数量，而不是模板的device_count
        int actualDeviceCount = 0;
        if (m_deviceInstances.contains(equipType->getTypeId())) {
            actualDeviceCount = m_deviceInstances[equipType->getTypeId()].size();
        }
        typeObj["device_count"] = actualDeviceCount;
        
        // 序列化基本参数
        QJsonArray basicParamsArray;
        const auto& basicParams = equipType->getBasicParameters();
        for (ParameterItem* param : basicParams) {
            QJsonObject paramObj;
            paramObj["id"] = param->getId();
            paramObj["label"] = param->getLabel();
            paramObj["type"] = param->getType();
            paramObj["unit"] = param->getUnit();
            paramObj["default"] = param->getDefaultValue().toString();
            
            if (param->getType() == "int" || param->getType() == "double") {
                QJsonArray rangeArray;
                rangeArray.append(param->getMinValue());
                rangeArray.append(param->getMaxValue());
                paramObj["range"] = rangeArray;
            } else if (param->getType() == "enum") {
                QJsonArray optionsArray;
                for (const QString& option : param->getOptions()) {
                    optionsArray.append(option);
                }
                paramObj["options"] = optionsArray;
            }
            
            basicParamsArray.append(paramObj);
        }
        typeObj["basic_parameters"] = basicParamsArray;
        
        // 序列化工作状态模板
        WorkStateTemplate* wsTemplate = equipType->getWorkStateTemplate();
        if (wsTemplate) {
            QJsonObject templateObj;
            templateObj["template_id"] = wsTemplate->getTemplateId();
            templateObj["template_name"] = wsTemplate->getTemplateName();
            
            QJsonArray wsParamsArray;
            const auto& wsParams = wsTemplate->getParameters();
            for (ParameterItem* param : wsParams) {
                QJsonObject paramObj;
                paramObj["id"] = param->getId();
                paramObj["label"] = param->getLabel();
                paramObj["type"] = param->getType();
                paramObj["unit"] = param->getUnit();
                paramObj["default"] = param->getDefaultValue().toString();
                
                if (param->getType() == "int" || param->getType() == "double") {
                    QJsonArray rangeArray;
                    rangeArray.append(param->getMinValue());
                    rangeArray.append(param->getMaxValue());
                    paramObj["range"] = rangeArray;
                } else if (param->getType() == "enum") {
                    QJsonArray optionsArray;
                    for (const QString& option : param->getOptions()) {
                        optionsArray.append(option);
                    }
                    paramObj["options"] = optionsArray;
                }
                
                wsParamsArray.append(paramObj);
            }
            templateObj["parameters"] = wsParamsArray;
            typeObj["work_state_template"] = templateObj;
        }
        
        // 序列化设备实例的实际参数值
        if (m_deviceInstances.contains(equipType->getTypeId())) {
            QJsonArray devicesArray;
            const QList<DeviceInstance*>& devices = m_deviceInstances[equipType->getTypeId()];
            
            for (DeviceInstance* device : devices) {
                QJsonObject deviceObj;
                deviceObj["device_id"] = device->getDeviceId();
                deviceObj["device_name"] = device->getDeviceName();
                
                // 保存基本参数值
                QJsonObject basicValuesObj;
                const auto& basicValues = device->getAllBasicValues();
                for (auto it = basicValues.begin(); it != basicValues.end(); ++it) {
                    basicValuesObj[it.key()] = it.value().toString();
                }
                deviceObj["basic_values"] = basicValuesObj;
                
                // 保存工作状态值
                QJsonArray workStatesArray;
                for (int i = 0; i < device->getWorkStateCount(); ++i) {
                    QJsonObject stateObj;
                    stateObj["state_index"] = i;
                    
                    QJsonObject stateValuesObj;
                    const QVariantMap& stateValues = device->getWorkStateValues(i);
                    for (auto it = stateValues.begin(); it != stateValues.end(); ++it) {
                        stateValuesObj[it.key()] = it.value().toString();
                    }
                    stateObj["values"] = stateValuesObj;
                    
                    workStatesArray.append(stateObj);
                }
                deviceObj["work_states"] = workStatesArray;
                
                devicesArray.append(deviceObj);
            }
            typeObj["device_instances"] = devicesArray;
        }
        
        equipmentTypesArray.append(typeObj);
    }
    
    configObj["equipment_types"] = equipmentTypesArray;
    // 只更新equipment_config部分，保留rootObj中的其他对象
    rootObj["equipment_config"] = configObj;
    
    // 写入文件
    QFile file(jsonFile);
    if (!file.open(QIODevice::WriteOnly)) {
        emit validationError(QString(u8"无法创建配置文件: %1").arg(jsonFile));
        return false;
    }
    
    QJsonDocument doc(rootObj);
    file.write(doc.toJson());
    file.close();
    
    qDebug() << QString(u8"配置保存完成: %1").arg(jsonFile);
    return true;
}

void EquipmentConfigWidget::createEquipmentTypeTabs()
{
    for (EquipmentType* equipType : m_equipmentTypes) {
        if (m_deviceInstances.contains(equipType->getTypeId())) {
            const QList<DeviceInstance*>& devices = m_deviceInstances[equipType->getTypeId()];
            
            // 如果只有一个设备实例，跳过中间层，直接显示设备详情
            if (devices.size() == 1) {
                DeviceTabWidget* deviceTabWidget = new DeviceTabWidget(devices.first(), this);
                addTab(deviceTabWidget, equipType->getTypeName());
                qDebug() << QString(u8"设备类型 %1 只有一个实例，直接显示设备详情").arg(equipType->getTypeName());
            } else {
                // 多个设备实例时，保持原有的三层结构
                QTabWidget* typeTabWidget = new QTabWidget(this);
                typeTabWidget->setTabPosition(QTabWidget::North);
                
                createDeviceTabs(equipType->getTypeId(), typeTabWidget);
                
                addTab(typeTabWidget, equipType->getTypeName());
                qDebug() << QString(u8"设备类型 %1 有 %2 个实例，显示设备列表").arg(equipType->getTypeName()).arg(devices.size());
            }
        }
    }
}

void EquipmentConfigWidget::createDeviceTabs(const QString& typeId, QTabWidget* parentTab)
{
    if (!m_deviceInstances.contains(typeId)) {
        return;
    }
    
    const QList<DeviceInstance*>& devices = m_deviceInstances[typeId];
    
    for (DeviceInstance* device : devices) {
        DeviceTabWidget* deviceTabWidget = new DeviceTabWidget(device, parentTab);
        parentTab->addTab(deviceTabWidget, device->getDeviceName());
    }
}

void EquipmentConfigWidget::updateAllVisibility()
{
    // 遍历所有装备类型的tab
    for (int i = 0; i < count(); ++i) {
        QWidget* tabWidget = widget(i);
        
        // 检查是否是直接的设备Tab（只有一个设备实例的情况）
        DeviceTabWidget* deviceTabWidget = qobject_cast<DeviceTabWidget*>(tabWidget);
        if (deviceTabWidget) {
            // 直接更新单个设备的可见性
            deviceTabWidget->updateVisibility();
            continue;
        }
        
        // 检查是否是设备类型Tab（多个设备实例的情况）
        QTabWidget* typeTabWidget = qobject_cast<QTabWidget*>(tabWidget);
        if (typeTabWidget) {
            // 遍历该类型下的所有设备tab
            for (int j = 0; j < typeTabWidget->count(); ++j) {
                DeviceTabWidget* nestedDeviceTab = qobject_cast<DeviceTabWidget*>(typeTabWidget->widget(j));
                if (nestedDeviceTab) {
                    nestedDeviceTab->updateVisibility();
                }
            }
        }
    }
    
    qDebug() << "所有设备可见性已更新（支持两层和三层结构）";
}

bool EquipmentConfigWidget::validateAll()
{
    // TODO: 实现验证逻辑
    qDebug() << "验证逻辑待实现";
    return true;
}

void EquipmentConfigWidget::onConfigurationChanged()
{
    emit configChanged();
} 