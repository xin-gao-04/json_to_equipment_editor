#include "DeviceTabWidget.h"
#include "WorkStateTabWidget.h"
#include "EquipmentConfigWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QScrollArea>
#include <QPushButton>
#include <QTimer>
#include <QDebug>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QMessageBox>

DeviceTabWidget::DeviceTabWidget(DeviceInstance* device, QWidget* parent)
    : QTabWidget(parent), m_device(device), m_saveDeviceButton(nullptr), m_saveBasicButton(nullptr)
{
    if (!m_device) {
        return;
    }
    
    createBasicParametersTab();
    createWorkStateTabs();
    m_lastStateCount = m_device->getWorkStateCount();
    
    // 启动定时器监听工作状态个数变化
    QTimer* updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &DeviceTabWidget::onWorkStateCountChanged);
    updateTimer->start(2000); // 每2秒检查一次
    
    // 启动定时器监听设备启用状态变化
    // 初始更新可见性
    updateVisibility();
}

void DeviceTabWidget::createBasicParametersTab()
{
    m_basicParamsWidget = new QWidget;
    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(m_basicParamsWidget);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(m_basicParamsWidget);
    
    // 创建基本参数组
    QGroupBox* basicGroup = new QGroupBox(u8"基本参数");
    QFormLayout* formLayout = new QFormLayout(basicGroup);
    
    EquipmentType* equipType = m_device->getEquipmentType();
    if (equipType) {
        const auto& basicParamTemplates = equipType->getBasicParameters();
        
        // 为每个基本参数创建独立的实例
        for (ParameterItem* templateParam : basicParamTemplates) {
            // 创建独立的ParameterItem实例
            ParameterItem* param = new ParameterItem(templateParam->getId(), 
                                                     templateParam->getLabel(), 
                                                     templateParam->getType());
            param->setParent(this);
            param->setUnit(templateParam->getUnit());
            param->setDefaultValue(templateParam->getDefaultValue());
            
            // 复制约束条件
            if (templateParam->getType() == "int" || templateParam->getType() == "double") {
                param->setRange(templateParam->getMinValue(), templateParam->getMaxValue());
            } else if (templateParam->getType() == "enum") {
                param->setOptions(templateParam->getOptions());
            }
            
            QWidget* editor = param->createEditor(basicGroup);
            
            // 设置当前值
            QVariant currentValue = m_device->getBasicValue(param->getId());
            if (currentValue.isValid()) {
                param->setValue(currentValue);
            } else {
                param->setValue(param->getDefaultValue());
                m_device->setBasicValue(param->getId(), param->getDefaultValue());
            }
            
            QString labelText = param->getLabel();
            if (!param->getUnit().isEmpty()) {
                labelText += QString(" (%1)").arg(param->getUnit());
            }
            
            formLayout->addRow(labelText, editor);
            
            // 存储参数实例以便后续更新
            m_basicParameterInstances[param->getId()] = param;
        }
    }
    
    mainLayout->addWidget(basicGroup);
    
    // 创建基本参数保存按钮
    QHBoxLayout* basicButtonLayout = new QHBoxLayout;
    m_saveBasicButton = new QPushButton(u8"保存基本参数");
    m_saveBasicButton->setMinimumHeight(30);
    connect(m_saveBasicButton, &QPushButton::clicked, this, &DeviceTabWidget::onSaveBasicButtonClicked);
    
    basicButtonLayout->addStretch();
    basicButtonLayout->addWidget(m_saveBasicButton);
    basicButtonLayout->addStretch();
    
    mainLayout->addLayout(basicButtonLayout);
    mainLayout->addStretch();
    
    addTab(scrollArea, u8"基本参数");
    
    // 启动定时器更新基本参数值
    QTimer* basicUpdateTimer = new QTimer(this);
    connect(basicUpdateTimer, &QTimer::timeout, this, &DeviceTabWidget::onBasicParameterChanged);
    basicUpdateTimer->start(1000);
}

void DeviceTabWidget::createWorkStateTabs()
{
    if (!m_device || !m_device->getEquipmentType() || !m_device->getEquipmentType()->getWorkStateTemplate()) {
        return;
    }
    
    WorkStateTemplate* tmpl = m_device->getEquipmentType()->getWorkStateTemplate();
    int stateCount = m_device->getWorkStateCount();
    // 考虑模板定义的标签数量和可选覆盖
    int desiredCount = stateCount;
    if (tmpl->getStateTabCountOverride() > 0) {
        desiredCount = qMax(desiredCount, tmpl->getStateTabCountOverride());
    }
    desiredCount = qMax(desiredCount, tmpl->getStateTabTitles().size());
    if (desiredCount != stateCount) {
        m_device->setWorkStateCount(desiredCount);
        stateCount = desiredCount;
    }
    
    for (int i = 0; i < stateCount; ++i) {
        QString tabName = (i < tmpl->getStateTabTitles().size() && !tmpl->getStateTabTitles().at(i).isEmpty())
                            ? tmpl->getStateTabTitles().at(i)
                            : QString(u8"工作状态 %1").arg(i + 1);
        WorkStateTabWidget* stateWidget = new WorkStateTabWidget(m_device, i, tabName, this);
        stateWidget->setObjectName(QStringLiteral("work_state_%1").arg(i));
        addTab(stateWidget, tabName);
    }
    
    m_lastStateCount = stateCount;
    
    // 创建设备级保存按钮（作为最后一个tab）
    createSaveButtons();
}

void DeviceTabWidget::createSaveButtons()
{
    QWidget* saveTabWidget = new QWidget;
    QVBoxLayout* saveLayout = new QVBoxLayout(saveTabWidget);
    
    QLabel* titleLabel = new QLabel(QString(u8"设备保存操作 - %1").arg(m_device->getDeviceName()));
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; margin: 20px; }");
    
    m_saveDeviceButton = new QPushButton(u8"保存整个设备配置");
    m_saveDeviceButton->setMinimumHeight(40);
    m_saveDeviceButton->setStyleSheet("QPushButton { font-size: 12px; padding: 10px; }");
    connect(m_saveDeviceButton, &QPushButton::clicked, this, &DeviceTabWidget::onSaveDeviceButtonClicked);
    
    QLabel* helpLabel = new QLabel(u8"保存整个设备的基本参数和所有工作状态参数到单个JSON文件中。");
    helpLabel->setAlignment(Qt::AlignCenter);
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("QLabel { color: #666; margin: 10px; }");
    
    saveLayout->addStretch();
    saveLayout->addWidget(titleLabel);
    saveLayout->addWidget(m_saveDeviceButton);
    saveLayout->addWidget(helpLabel);
    saveLayout->addStretch();
    
    //addTab(saveTabWidget, u8"保存设备");
}

void DeviceTabWidget::onSaveDeviceButtonClicked()
{
    // 先更新所有参数值
    onBasicParameterChanged();
    
    // 获取主配置Widget来执行自动保存
    EquipmentConfigWidget* mainConfig = nullptr;
    QWidget* parent = this->parentWidget();
    while (parent) {
        mainConfig = qobject_cast<EquipmentConfigWidget*>(parent);
        if (mainConfig) {
            break;
        }
        parent = parent->parentWidget();
    }
    
    if (mainConfig) {
        if (mainConfig->hasCurrentFile()) {
            // 自动保存到当前文件
            if (mainConfig->autoSave()) {
                QMessageBox::information(this, u8"保存成功", 
                                         QString(u8"设备 %1 已保存到当前配置文件")
                                             .arg(m_device->getDeviceName()));
            }
        } else {
            // 如果没有当前文件，提供单独保存选项
            QString fileName = QFileDialog::getSaveFileName(this, 
                                                            QString(u8"保存设备配置 - %1").arg(m_device->getDeviceName()),
                                                            QString("device_%1.json").arg(m_device->getDeviceId()),
                                                            u8"JSON文件 (*.json)");
            
            if (!fileName.isEmpty()) {
                if (saveDeviceToJson(fileName)) {
                    QMessageBox::information(this, u8"保存成功", 
                                             QString(u8"设备 %1 配置已保存到: %2")
                                                 .arg(m_device->getDeviceName())
                                                 .arg(fileName));
                }
            }
        }
    }
}

void DeviceTabWidget::onSaveBasicButtonClicked()
{
    // 先更新基本参数值
    onBasicParameterChanged();
    
    // 获取主配置Widget来执行自动保存
    EquipmentConfigWidget* mainConfig = nullptr;
    QWidget* parent = this->parentWidget();
    while (parent) {
        mainConfig = qobject_cast<EquipmentConfigWidget*>(parent);
        if (mainConfig) {
            break;
        }
        parent = parent->parentWidget();
    }
    
    if (mainConfig) {
        if (mainConfig->hasCurrentFile()) {
            // 自动保存到当前文件
            if (mainConfig->autoSave()) {
                QMessageBox::information(this, u8"保存成功", 
                                         QString(u8"设备 %1 的基本参数已保存到当前配置文件")
                                             .arg(m_device->getDeviceName()));
            }
        } else {
            // 如果没有当前文件，提供单独保存选项
            QString fileName = QFileDialog::getSaveFileName(this, 
                                                            QString(u8"保存基本参数 - %1").arg(m_device->getDeviceName()),
                                                            QString("basic_params_%1.json").arg(m_device->getDeviceId()),
                                                            u8"JSON文件 (*.json)");
            
            if (!fileName.isEmpty()) {
                if (saveBasicParametersToJson(fileName)) {
                    QMessageBox::information(this, u8"保存成功", 
                                             QString(u8"设备 %1 的基本参数已保存到: %2")
                                                 .arg(m_device->getDeviceName())
                                                 .arg(fileName));
                }
            }
        }
    }
}

bool DeviceTabWidget::saveDeviceToJson(const QString& fileName)
{
    // 先更新所有参数值
    onBasicParameterChanged();
    
    QJsonObject rootObj;
    QJsonObject deviceObj;
    
    deviceObj["device_id"] = m_device->getDeviceId();
    deviceObj["device_name"] = m_device->getDeviceName();
    deviceObj["equipment_type"] = m_device->getEquipmentType()->getTypeName();
    
    // 保存基本参数值
    QJsonObject basicValuesObj;
    const auto& basicValues = m_device->getAllBasicValues();
    for (auto it = basicValues.begin(); it != basicValues.end(); ++it) {
        basicValuesObj[it.key()] = it.value().toString();
    }
    deviceObj["basic_parameters"] = basicValuesObj;
    
    // 保存所有工作状态值
    QJsonArray workStatesArray;
    for (int i = 0; i < m_device->getWorkStateCount(); ++i) {
        QJsonObject stateObj;
        stateObj["state_index"] = i;
        
        QJsonObject stateValuesObj;
        const QVariantMap& stateValues = m_device->getWorkStateValues(i);
        for (auto it = stateValues.begin(); it != stateValues.end(); ++it) {
            stateValuesObj[it.key()] = it.value().toString();
        }
        stateObj["parameter_values"] = stateValuesObj;
        
        workStatesArray.append(stateObj);
    }
    deviceObj["work_states"] = workStatesArray;
    
    rootObj["device_config"] = deviceObj;
    
    // 写入文件
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, u8"保存失败", 
                             QString(u8"无法创建文件: %1").arg(fileName));
        return false;
    }
    
    QJsonDocument doc(rootObj);
    file.write(doc.toJson());
    file.close();
    
    qDebug() << QString(u8"设备配置保存完成: %1").arg(fileName);
    return true;
}

bool DeviceTabWidget::saveBasicParametersToJson(const QString& fileName)
{
    // 先更新基本参数值
    onBasicParameterChanged();
    
    QJsonObject rootObj;
    QJsonObject basicObj;
    
    basicObj["device_id"] = m_device->getDeviceId();
    basicObj["device_name"] = m_device->getDeviceName();
    basicObj["equipment_type"] = m_device->getEquipmentType()->getTypeName();
    
    // 保存基本参数值和定义
    QJsonArray parametersArray;
    for (auto it = m_basicParameterInstances.begin(); it != m_basicParameterInstances.end(); ++it) {
        ParameterItem* param = it.value();
        QJsonObject paramObj;
        paramObj["id"] = param->getId();
        paramObj["label"] = param->getLabel();
        paramObj["type"] = param->getType();
        paramObj["unit"] = param->getUnit();
        paramObj["current_value"] = param->getValue().toString();
        paramObj["default_value"] = param->getDefaultValue().toString();
        
        parametersArray.append(paramObj);
    }
    basicObj["parameters"] = parametersArray;
    
    rootObj["basic_parameters"] = basicObj;
    
    // 写入文件
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, u8"保存失败", 
                             QString(u8"无法创建文件: %1").arg(fileName));
        return false;
    }
    
    QJsonDocument doc(rootObj);
    file.write(doc.toJson());
    file.close();
    
    qDebug() << QString(u8"基本参数保存完成: %1").arg(fileName);
    return true;
}

void DeviceTabWidget::updateWorkStateTabs()
{
    if (!m_device) {
        return;
    }
    
    WorkStateTemplate* tmpl = m_device->getEquipmentType() ? m_device->getEquipmentType()->getWorkStateTemplate() : nullptr;
    int baseCount = m_device->getWorkStateCount();
    int desiredCount = baseCount;
    if (tmpl) {
        if (tmpl->getStateTabCountOverride() > 0) {
            desiredCount = qMax(desiredCount, tmpl->getStateTabCountOverride());
        }
        desiredCount = qMax(desiredCount, tmpl->getStateTabTitles().size());
    }
    if (desiredCount != baseCount) {
        m_device->setWorkStateCount(desiredCount);
    }
    int newStateCount = desiredCount;
    
    int currentWorkStateCount = 0;
    for (int i = 1; i < count(); ++i) { // 跳过基本参数tab(0)
        if (qobject_cast<WorkStateTabWidget*>(widget(i))) {
            currentWorkStateCount++;
        }
    }
    
    if (newStateCount == currentWorkStateCount) {
        // 数量一致时刷新标签文本，防止自定义标签被默认覆盖
        for (int i = 1; i < count(); ++i) {
            WorkStateTabWidget* w = qobject_cast<WorkStateTabWidget*>(widget(i));
            if (!w) continue;
            QString tabName = (tmpl && (i - 1) < tmpl->getStateTabTitles().size() && !tmpl->getStateTabTitles().at(i - 1).isEmpty())
                                ? tmpl->getStateTabTitles().at(i - 1)
                                : QString(u8"工作状态 %1").arg(i);
            setTabText(i, tabName);
        }
        return; // 没有变化
    }
    
    // 移除所有工作状态tab，保留基本参数tab
    for (int i = count() - 1; i >= 1; --i) { // 从后往前删除，跳过基本参数tab(0)
        if (qobject_cast<WorkStateTabWidget*>(widget(i))) {
            QWidget* tabWidget = this->widget(i);
            removeTab(i);
            delete tabWidget;
        }
    }
    
    // 重新创建工作状态tab
    for (int i = 0; i < newStateCount; ++i) {
        QString tabName = (tmpl && i < tmpl->getStateTabTitles().size() && !tmpl->getStateTabTitles().at(i).isEmpty())
                            ? tmpl->getStateTabTitles().at(i)
                            : QString(u8"工作状态 %1").arg(i + 1);
        WorkStateTabWidget* stateWidget = new WorkStateTabWidget(m_device, i, tabName, this);
        addTab(stateWidget, tabName);
    }
    
    qDebug() << "Updated work state tabs. New count:" << newStateCount;
    m_lastStateCount = newStateCount;
}

void DeviceTabWidget::onBasicParameterChanged()
{
    if (!m_device) {
        return;
    }
    
    // 保存旧的工作状态数量
    int oldStateCount = m_device->getWorkStateCount();
    
    // 更新设备实例中的基本参数值（使用独立的参数实例）
    for (auto it = m_basicParameterInstances.begin(); it != m_basicParameterInstances.end(); ++it) {
        ParameterItem* param = it.value();
        QVariant newValue = param->getValue();
        m_device->setBasicValue(param->getId(), newValue);
    }
    
    // 检查工作状态数量是否发生变化
    int newStateCount = m_device->getWorkStateCount();
    if (oldStateCount != newStateCount) {
        qDebug() << QString(u8"检测到工作状态数量变化：%1 -> %2，调用setWorkStateCount进行初始化")
                     .arg(oldStateCount).arg(newStateCount);
        // 调用setWorkStateCount来正确初始化工作状态数据结构
        m_device->setWorkStateCount(newStateCount);
        
        // 立即更新工作状态标签页
        updateWorkStateTabs();
    }
    
    m_lastStateCount = newStateCount;
}

void DeviceTabWidget::onWorkStateCountChanged()
{
    int currentStateCount = m_device ? m_device->getWorkStateCount() : 0;
    
    if (m_lastStateCount != currentStateCount) {
        m_lastStateCount = currentStateCount;
        updateWorkStateTabs();
    }
}

void DeviceTabWidget::updateVisibility()
{
    if (!m_device) {
        return;
    }
    
    bool deviceEnabled = m_device->isEnabled();
    WorkStateTemplate* tmpl = m_device->getEquipmentType() ? m_device->getEquipmentType()->getWorkStateTemplate() : nullptr;
    
    // 收集所有工作状态标签页
    QList<QWidget*> workStateTabs;
    QList<int> workStateIndices;
    
    for (int i = 1; i < count(); ++i) { // 跳过基本参数tab(0)
        QWidget* tabWidget = widget(i);
        if (tabWidget && qobject_cast<WorkStateTabWidget*>(tabWidget)) {
            workStateTabs.append(tabWidget);
            workStateIndices.append(i);
        }
    }
    
    if (!deviceEnabled) {
        // 设备未启用：隐藏所有工作状态标签页
        for (int i = workStateIndices.size() - 1; i >= 0; --i) { // 从后往前删除，避免索引变化
            int index = workStateIndices[i];
            QWidget* tabWidget = widget(index);
            removeTab(index);
            tabWidget->hide();
        }
        
        // 隐藏基本参数中的工作状态数量参数
        for (auto it = m_basicParameterInstances.begin(); it != m_basicParameterInstances.end(); ++it) {
            ParameterItem* param = it.value();
            if (param->getId() == "work_state_count" || param->getId().contains("工作状态")) {
                param->setVisible(false);
            }
        }
        
        // 隐藏保存设备按钮
        if (m_saveDeviceButton) {
            m_saveDeviceButton->setVisible(false);
        }
        
        // 修改基本参数保存按钮文本
        if (m_saveBasicButton) {
            m_saveBasicButton->setText(u8"保存基本参数（设备未启用）");
            m_saveBasicButton->setEnabled(false);
        }
    } else {
        // 设备启用：确保工作状态标签页正确显示
        int expectedStateCount = m_device->getWorkStateCount();
        if (tmpl) {
            if (tmpl->getStateTabCountOverride() > 0) {
                expectedStateCount = qMax(expectedStateCount, tmpl->getStateTabCountOverride());
            }
            expectedStateCount = qMax(expectedStateCount, tmpl->getStateTabTitles().size());
        }
        
        // 如果没有工作状态标签页或数量不对，重新创建
        if (workStateTabs.size() != expectedStateCount) {
            // 清理现有的工作状态标签页
            for (int i = workStateIndices.size() - 1; i >= 0; --i) {
                int index = workStateIndices[i];
                QWidget* tabWidget = widget(index);
                removeTab(index);
                delete tabWidget; // 删除widget防止内存泄漏
            }
            
            // 重新创建正确数量的工作状态tabs
            for (int i = 0; i < expectedStateCount; ++i) {
                QString tabName = (tmpl && i < tmpl->getStateTabTitles().size() && !tmpl->getStateTabTitles().at(i).isEmpty())
                                    ? tmpl->getStateTabTitles().at(i)
                                    : QString(u8"工作状态 %1").arg(i + 1);
                WorkStateTabWidget* stateWidget = new WorkStateTabWidget(m_device, i, tabName, this);
                addTab(stateWidget, tabName);
            }
        } else {
            // 显示现有的工作状态标签页
            for (int i = 0; i < workStateTabs.size(); ++i) {
                QWidget* tabWidget = workStateTabs[i];
                QString tabName = (tmpl && i < tmpl->getStateTabTitles().size() && !tmpl->getStateTabTitles().at(i).isEmpty())
                                    ? tmpl->getStateTabTitles().at(i)
                                    : QString(u8"工作状态 %1").arg(i + 1);
                int idxTab = indexOf(tabWidget);
                if (idxTab < 0) { // 如果标签页不在tab widget中
                    addTab(tabWidget, tabName);
                } else {
                    setTabText(idxTab, tabName);
                }
                tabWidget->show();
            }
        }
        
        // 显示所有参数
        for (auto it = m_basicParameterInstances.begin(); it != m_basicParameterInstances.end(); ++it) {
            ParameterItem* param = it.value();
            param->setVisible(true);
        }
        
        // 显示保存设备按钮
        if (m_saveDeviceButton) {
            m_saveDeviceButton->setVisible(true);
        }
        
        // 恢复基本参数保存按钮
        if (m_saveBasicButton) {
            m_saveBasicButton->setText(u8"保存基本参数");
            m_saveBasicButton->setEnabled(true);
        }
    }
} 
