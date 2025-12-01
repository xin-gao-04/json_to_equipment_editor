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
#include <QFileInfo>
#include <QDir>
#include <QScopedValueRollback>
#include <QTabBar>
#include <QHash>
#include "ConfigEditorDialog.h"

EquipmentConfigWidget::EquipmentConfigWidget(QWidget* parent)
    : QTabWidget(parent)
{
    setTabPosition(QTabWidget::North);
    setMovable(true);
    setTabsClosable(false);
    setUsesScrollButtons(true);
    if (tabBar()) {
        tabBar()->setExpanding(false);
        tabBar()->setElideMode(Qt::ElideNone);
    }
    
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
    m_lastRootObject = rootObj;
    if (!rootObj.contains("equipment_config")) {
        emit validationError(QString(u8"配置文件格式错误：缺少equipment_config节点"));
        return false;
    }

    // 加载阶段屏蔽界面刷新与可见性检查，减少构建时的卡顿
    QScopedValueRollback<bool> loadingGuard(m_isLoading, true);
    struct CursorGuard {
        CursorGuard() { QApplication::setOverrideCursor(Qt::WaitCursor); }
        ~CursorGuard() { QApplication::restoreOverrideCursor(); }
    } cursorGuard;
    setUpdatesEnabled(false);
    
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
    setUpdatesEnabled(true);
    updateAllVisibility(); // 构建完成后统一刷新可见性
    
    // 设置当前文件路径
    m_currentFilePath = jsonFile;
    emit filePathChanged(m_currentFilePath);
    
    qDebug() << QString(u8"配置加载完成。设备类型数量: %1").arg(m_equipmentTypes.size());
    
    emit configChanged();
    return true;
}

bool EquipmentConfigWidget::createNewConfig(const QString& jsonFile)
{
    QFileInfo fi(jsonFile);
    QDir dir = fi.dir();
    if (!dir.exists() && !dir.mkpath(".")) {
        emit validationError(QString(u8"无法创建目录: %1").arg(dir.absolutePath()));
        return false;
    }

    QJsonObject rootObj;
    QJsonObject ecObj;
    ecObj.insert("equipment_types", QJsonArray());
    rootObj.insert("equipment_config", ecObj);

    QFile file(jsonFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit validationError(QString(u8"无法创建空白配置文件: %1").arg(jsonFile));
        return false;
    }
    QJsonDocument doc(rootObj);
    file.write(doc.toJson());
    file.close();

    return loadFromJson(jsonFile);
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
    if (!validateAll()) {
        emit validationError(u8"保存失败：存在非法或超出范围的输入，请检查高亮字段。");
        return false;
    }
    
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
    
    QJsonObject configObj = rootObj.value("equipment_config").toObject();
    QJsonArray existingTypesArray = configObj.value("equipment_types").toArray();
    QHash<QString, QJsonObject> existingTypeMap;
    for (const auto& v : existingTypesArray) {
        QJsonObject obj = v.toObject();
        QString tid = obj.value("type_id").toString();
        if (!tid.isEmpty()) {
            existingTypeMap.insert(tid, obj);
        }
    }

    QJsonArray equipmentTypesArray;
    
    // 序列化所有设备类型
    for (EquipmentType* equipType : m_equipmentTypes) {
        QJsonObject typeObj = existingTypeMap.value(equipType->getTypeId());
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
            QJsonObject templateObj = typeObj.value("work_state_template").toObject();
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
            
            // 保存自定义工作状态标签及可选的数量覆盖
            if (!wsTemplate->getStateTabTitles().isEmpty()) {
                QJsonArray titlesArr;
                for (const QString& t : wsTemplate->getStateTabTitles()) {
                    titlesArr.append(t);
                }
                templateObj["state_tab_titles"] = titlesArr;
            } else {
                templateObj.remove("state_tab_titles");
            }
            if (wsTemplate->getStateTabCountOverride() > 0) {
                templateObj["state_tab_count"] = wsTemplate->getStateTabCountOverride();
            } else {
                templateObj.remove("state_tab_count");
            }

            // 规则（可见性 / 选项 / 校验说明）
            QJsonArray visRules = wsTemplate->getVisibilityRulesJson();
            if (!visRules.isEmpty()) {
                templateObj["visibility_rules"] = visRules;
            } else {
                templateObj.remove("visibility_rules");
            }

            QJsonArray optRules = wsTemplate->getOptionRulesJson();
            if (!optRules.isEmpty()) {
                templateObj["option_rules"] = optRules;
            } else {
                templateObj.remove("option_rules");
            }

            QJsonArray validationRules = wsTemplate->getValidationRulesJson();
            if (validationRules.isEmpty()) {
                validationRules = templateObj.value("validation_rules").toArray();
            }
            if (!validationRules.isEmpty()) {
                templateObj["validation_rules"] = validationRules;
            } else {
                templateObj.remove("validation_rules");
            }
            typeObj["work_state_template"] = templateObj;
        } else {
            typeObj.remove("work_state_template");
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
    m_lastRootObject = rootObj;
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
                typeTabWidget->setUsesScrollButtons(true);
                if (typeTabWidget->tabBar()) {
                    typeTabWidget->tabBar()->setExpanding(false);
                    typeTabWidget->tabBar()->setElideMode(Qt::ElideNone);
                }
                
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
    if (m_isLoading) {
        return;
    }
    
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
    QStringList errors;
    auto parseFrequencies = [](const QString& text, QList<double>& out) -> bool {
        QString trimmed = text.trimmed();
        if (trimmed.isEmpty()) {
            return true;
        }
        if (!trimmed.startsWith('[') || !trimmed.endsWith(']')) {
            return false;
        }
        trimmed = trimmed.mid(1, trimmed.size() - 2).trimmed();
        if (trimmed.isEmpty()) {
            return true;
        }
        const QStringList parts = trimmed.split(',',  QString::SkipEmptyParts);
        for (const QString& p : parts) {
            bool ok = false;
            double v = p.trimmed().toDouble(&ok);
            if (!ok) {
                return false;
            }
            out.append(v);
        }
        return true;
    };

    auto normalizeValue = [](ParameterItem* param, const QVariant& value) {
        QVariant normalized = value;
        const QString type = param->getType();
        
        if (!normalized.isValid() || (type == "string" && normalized.toString().isEmpty())) {
            if (type == "string" && param->isArrayLike()) {
                normalized = QStringLiteral("[]");
            } else {
                normalized = param->getDefaultValue();
            }
        }
        
        if (type == "int" || type == "Byte") {
            bool ok = false;
            int v = normalized.toInt(&ok);
            if (!ok) {
                v = param->getDefaultValue().toInt();
            }
            int minVal = static_cast<int>(param->getMinValue());
            int maxVal = static_cast<int>(param->getMaxValue());
            if (v < minVal) v = minVal;
            if (v > maxVal) v = maxVal;
            normalized = v;
        } else if (type == "double") {
            bool ok = false;
            double v = normalized.toDouble(&ok);
            if (!ok) {
                v = param->getDefaultValue().toDouble();
            }
            double minVal = param->getMinValue();
            double maxVal = param->getMaxValue();
            if (v < minVal) v = minVal;
            if (v > maxVal) v = maxVal;
            normalized = v;
        } else if (type == "enum") {
            QString v = normalized.toString();
            if (!param->getOptions().contains(v)) {
                QString def = param->getDefaultValue().toString();
                if (param->getOptions().contains(def)) {
                    v = def;
                } else if (!param->getOptions().isEmpty()) {
                    v = param->getOptions().first();
                }
            }
            normalized = v;
        } else if (type == "string") {
            QString v = normalized.toString();
            if (!ParameterItem::stringAllowedPattern().match(v).hasMatch()) {
                v = param->getDefaultValue().toString();
            }
            normalized = v;
        }
        
        return normalized;
    };
    
    for (EquipmentType* equipType : m_equipmentTypes) {
        const auto& basicParams = equipType->getBasicParameters();
        WorkStateTemplate* wsTemplate = equipType->getWorkStateTemplate();
        const QList<DeviceInstance*>& devices = m_deviceInstances.value(equipType->getTypeId());
        for (DeviceInstance* device : devices) {
            // 基本参数校验
            for (ParameterItem* param : basicParams) {
                QVariant val = normalizeValue(param, device->getBasicValue(param->getId()));
                device->setBasicValue(param->getId(), val); // 回写自动填充或校正的值
                if (!param->validate(val)) {
                    errors << QString(u8"设备[%1] 基本参数[%2] 输入非法或超出范围").arg(device->getDeviceName(), param->getLabel());
                }
            }
            
            // 工作状态参数校验
            if (wsTemplate) {
                for (int stateIdx = 0; stateIdx < device->getWorkStateCount(); ++stateIdx) {
                    QVariantMap stateValues = device->getWorkStateValues(stateIdx);
                    for (ParameterItem* param : wsTemplate->getParameters()) {
                        QVariant val = normalizeValue(param, stateValues.value(param->getId(), param->getDefaultValue()));
                        stateValues[param->getId()] = val; // 回写自动填充或校正的值
                        if (!param->validate(val)) {
                            errors << QString(u8"设备[%1] 工作状态%2 参数[%3] 输入非法或超出范围")
                                       .arg(device->getDeviceName())
                                       .arg(stateIdx + 1)
                                       .arg(param->getLabel());
                        }
                    }
                    device->setWorkStateValues(stateIdx, stateValues);
                }
            }

            // 通用规则引擎：按 validation_rules 执行业务校验（当前用于 UHF）
            if (wsTemplate) {
                QJsonObject wsObj = equipType->getWorkStateTemplate()->getTemplateId().isEmpty()
                    ? QJsonObject()
                    : QJsonObject(); //占位
                // 从原始配置对象中获取 validation_rules（存放在 work_state_template 节点）
                // 由于这里没有直接保存模板的 JSON，改用 equipType->getWorkStateTemplate() 获取不到规则，故直接从 m_lastRootObject 中查找
            }
        }
    }

    // 从 m_lastRootObject 中提取 validation_rules 并执行
    if (!m_lastRootObject.isEmpty()) {
        QJsonArray types = m_lastRootObject.value("equipment_config").toObject().value("equipment_types").toArray();
        QMap<QString, QJsonObject> rulesByType;
        for (const auto& tv : types) {
            QJsonObject to = tv.toObject();
            QString tid = to.value("type_id").toString();
            QJsonObject ws = to.value("work_state_template").toObject();
            if (ws.contains("validation_rules")) {
                rulesByType.insert(tid, ws);
            }
        }

        auto getParamVal = [](const QVariantMap& vals, const QString& key) -> QString {
            QVariant v = vals.value(key);
            if (!v.isValid()) return QString();
            return v.toString();
        };

        auto matchWhen = [&](const QVariantMap& vals, const QJsonObject& when) -> bool {
            for (auto it = when.begin(); it != when.end(); ++it) {
                QString key = it.key();
                QJsonArray arr = it.value().toArray();
                QString cur = getParamVal(vals, key);
                bool hit = false;
                for (const auto& v : arr) {
                    if (cur == v.toString()) {
                        hit = true;
                        break;
                    }
                }
                if (!hit) return false;
            }
            return true;
        };

        auto getNumber = [](const QVariantMap& vals, const QString& key) -> double {
            bool ok = false;
            double d = vals.value(key).toDouble(&ok);
            return ok ? d : 0.0;
        };

        auto applyValidationRules = [&](const QString& typeId,
                                        DeviceInstance* device,
                                        int stateIdx,
                                        const QVariantMap& vals,
                                        const QJsonArray& rules) {
            for (const auto& rv : rules) {
                QJsonObject ro = rv.toObject();
                QString scope = ro.value("scope").toString();
                if (scope != "per_state") continue;
                QString rid = ro.value("id").toString();
                QString desc = ro.value("description").toString();
                QJsonArray constraints = ro.value("constraints").toArray();
                for (const auto& cv : constraints) {
                    QJsonObject co = cv.toObject();
                    if (co.contains("when")) {
                        if (!matchWhen(vals, co.value("when").toObject())) {
                            continue;
                        }
                    }
                    const QString prefix = QString(u8"设备[%1] 工作状态%2 规则[%3]")
                                               .arg(device->getDeviceName())
                                               .arg(stateIdx + 1)
                                               .arg(rid.isEmpty() ? QStringLiteral("未知") : rid);
                    const double eps = 1e-6;
                    if (co.contains("equal")) {
                        QJsonArray arr = co.value("equal").toArray();
                        if (arr.size() == 2) {
                            double a = getNumber(vals, arr.at(0).toString());
                            double b = getNumber(vals, arr.at(1).toString());
                            if (qAbs(a - b) > eps) {
                                errors << QString(u8"%1：%2 与 %3 应相等").arg(prefix, arr.at(0).toString(), arr.at(1).toString());
                            }
                        }
                    }
                    if (co.contains("less")) {
                        QJsonArray arr = co.value("less").toArray();
                        if (arr.size() == 2) {
                            double a = getNumber(vals, arr.at(0).toString());
                            double b = getNumber(vals, arr.at(1).toString());
                            if (!(a < b)) {
                                errors << QString(u8"%1：%2 应小于 %3").arg(prefix, arr.at(0).toString(), arr.at(1).toString());
                            }
                        }
                    }
                    if (co.contains("min")) {
                        QJsonObject mo = co.value("min").toObject();
                        for (auto it = mo.begin(); it != mo.end(); ++it) {
                            double v = getNumber(vals, it.key());
                            if (v + eps < it.value().toDouble()) {
                                errors << QString(u8"%1：%2 应≥%3").arg(prefix, it.key()).arg(it.value().toDouble());
                            }
                        }
                    }
                    if (co.contains("min_end_by_interval")) {
                        QJsonObject mo = co.value("min_end_by_interval").toObject();
                        QString startK = mo.value("start").toString();
                        QString endK = mo.value("end").toString();
                        QString countK = mo.value("count").toString();
                        QString intK = mo.value("interval").toString();
                        double start = getNumber(vals, startK);
                        double end = getNumber(vals, endK);
                        int count = vals.value(countK).toInt();
                        double interval = getNumber(vals, intK);
                        double minEnd = start + static_cast<double>(count) * interval;
                        if (end + eps < minEnd) {
                            errors << QString(u8"%1：%2 应≥%3（当前 %4，期望≥%5）")
                                      .arg(prefix, endK)
                                      .arg(minEnd, 0, 'g', 12)
                                      .arg(end, 0, 'g', 12)
                                      .arg(minEnd, 0, 'g', 12);
                        }
                    }
                    if (co.contains("list_between")) {
                        QJsonObject lo = co.value("list_between").toObject();
                        QString listK = lo.value("list").toString();
                        QString minK = lo.value("min_from").toString();
                        QString maxK = lo.value("max_from").toString();
                        QList<double> listVals;
                        if (!parseFrequencies(vals.value(listK).toString(), listVals)) {
                            errors << QString(u8"%1：%2 频率数组格式错误，应为形如[1,2,3]").arg(prefix, listK);
                        } else {
                            double minV = getNumber(vals, minK);
                            double maxV = getNumber(vals, maxK);
                            for (double f : listVals) {
                                if (!(f > minV && f < maxV)) {
                                    errors << QString(u8"%1：%2 中的频率%3 应在 (%4, %5) 内")
                                              .arg(prefix, listK)
                                              .arg(f, 0, 'g', 12)
                                              .arg(minV, 0, 'g', 12)
                                              .arg(maxV, 0, 'g', 12);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        };

        for (EquipmentType* equipType : m_equipmentTypes) {
            const auto& devices = m_deviceInstances.value(equipType->getTypeId());
            QJsonObject wsObj = rulesByType.value(equipType->getTypeId());
            QJsonArray vRules = wsObj.value("validation_rules").toArray();
            if (vRules.isEmpty()) continue;
            for (DeviceInstance* device : devices) {
                for (int stateIdx = 0; stateIdx < device->getWorkStateCount(); ++stateIdx) {
                    QVariantMap stateValues = device->getWorkStateValues(stateIdx);
                    applyValidationRules(equipType->getTypeId(), device, stateIdx, stateValues, vRules);
                }
            }
        }
    }
    
    if (!errors.isEmpty()) {
        emit validationError(errors.join("\n"));
        return false;
    }
    
    return true;
}

void EquipmentConfigWidget::onConfigurationChanged()
{
    emit configChanged();
}

bool EquipmentConfigWidget::openStructureEditor()
{
    // 使用原始 root 对象和当前文件构建编辑器
    if (m_lastRootObject.isEmpty()) {
        emit validationError(u8"当前没有加载可编辑的配置。");
        return false;
    }
    ConfigEditorDialog dlg(m_lastRootObject, m_currentFilePath, this);
    if (dlg.exec() == QDialog::Accepted && dlg.changed()) {
        // 重新加载文件以同步模型
        if (!m_currentFilePath.isEmpty()) {
            return loadFromJson(m_currentFilePath);
        }
    }
    return true;
}
