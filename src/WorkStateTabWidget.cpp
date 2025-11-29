#include "WorkStateTabWidget.h"
#include "ParameterItem.h"
#include "EquipmentConfigWidget.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QTimer>
#include <QComboBox>
#include <QSignalBlocker>
#include <QDebug>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QMessageBox>

WorkStateTabWidget::WorkStateTabWidget(DeviceInstance* device, int stateIndex, QWidget* parent)
    : QScrollArea(parent), m_device(device), m_stateIndex(stateIndex), m_saveButton(nullptr)
{
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    createParameterWidgets();
    createSaveButton();
}

void WorkStateTabWidget::createParameterWidgets()
{
    m_contentWidget = new QWidget;
    QVBoxLayout* mainLayout = new QVBoxLayout(m_contentWidget);
    
    if (!m_device || !m_device->getEquipmentType() || !m_device->getEquipmentType()->getWorkStateTemplate()) {
        QLabel* errorLabel = new QLabel(u8"无工作状态模板");
        mainLayout->addWidget(errorLabel);
        setWidget(m_contentWidget);
        return;
    }
    
    WorkStateTemplate* tmpl = m_device->getEquipmentType()->getWorkStateTemplate();
    
    // 创建参数组
    QGroupBox* paramGroup = new QGroupBox(QString(u8"工作状态 %1 参数").arg(m_stateIndex + 1));
    QFormLayout* formLayout = new QFormLayout(paramGroup);
    
    // 获取当前状态的值
    QVariantMap currentValues = m_device->getWorkStateValues(m_stateIndex);
    
    // 为每个参数创建独立的编辑器
    const auto& templateParameters = tmpl->getParameters();
    for (ParameterItem* templateParam : templateParameters) {
        // 为每个工作状态创建独立的ParameterItem实例
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
        
        QWidget* editor = param->createEditor(paramGroup);
        
        // 设置当前值
        QVariant valueToSet;
        if (currentValues.contains(param->getId())) {
            valueToSet = currentValues[param->getId()];
        } else {
            valueToSet = param->getDefaultValue();
            // 如果没有值，设置默认值到设备实例中
            currentValues[param->getId()] = valueToSet;
        }
        
        param->setValue(valueToSet);
        
        QString labelText = param->getLabel();
        if (!param->getUnit().isEmpty()) {
            labelText += QString(" (%1)").arg(param->getUnit());
        }
        QLabel* labelWidget = new QLabel(labelText);
        formLayout->addRow(labelWidget, editor);
        
        // 存储参数引用以便后续更新
        m_parameterInstances[param->getId()] = param;
        m_labelWidgets[param->getId()] = labelWidget;
        
        qDebug() << QString(u8"创建参数编辑器: %1, 值: %2").arg(param->getLabel()).arg(valueToSet.toString());
    }
    
    // 确保设备实例有这个状态的值
    if (!currentValues.isEmpty()) {
        m_device->setWorkStateValues(m_stateIndex, currentValues);
    }
    
    mainLayout->addWidget(paramGroup);
    mainLayout->addStretch();
    
    setWidget(m_contentWidget);
    
    // 启动定时器定期更新状态值
    QTimer* updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &WorkStateTabWidget::updateStateValues);
    updateTimer->start(1000); // 每秒更新一次
    
    // 可见性规则联动
    const auto& rules = tmpl->getVisibilityRules();
    for (const auto& rule : rules) {
        ParameterItem* controllerParam = m_parameterInstances.value(rule.controllerId, nullptr);
        if (!controllerParam) {
            continue;
        }
        QComboBox* controllerBox = qobject_cast<QComboBox*>(controllerParam->createEditor(paramGroup));
        if (!controllerBox) {
            continue;
        }
        
        auto applyRule = [this, rule](const QString& currentValue) {
            // 找到匹配的case
            QStringList showIds;
            for (const auto& c : rule.cases) {
                if (c.value == currentValue) {
                    showIds = c.showIds;
                    break;
                }
            }
            // 如果没有匹配，默认全部显示
            bool hideOthers = !showIds.isEmpty();
            for (const QString& pid : rule.affectedIds) {
                if (!m_parameterInstances.contains(pid)) {
                    continue;
                }
                bool visible = !hideOthers || showIds.contains(pid);
                m_parameterInstances[pid]->setVisible(visible);
                if (m_labelWidgets.contains(pid)) {
                    m_labelWidgets[pid]->setVisible(visible);
                }
            }
        };
        
        connect(controllerBox, &QComboBox::currentTextChanged, this, applyRule);
        // 初始化一次
        applyRule(controllerParam->getValue().toString());
    }
    
    // 选项联动规则：根据控制项值刷新目标枚举的可选项
    const auto& optionRules = tmpl->getOptionRules();
    for (const auto& rule : optionRules) {
        ParameterItem* controllerParam = m_parameterInstances.value(rule.controllerId, nullptr);
        ParameterItem* targetParam = m_parameterInstances.value(rule.targetId, nullptr);
        if (!controllerParam || !targetParam) {
            continue;
        }
        QComboBox* controllerBox = qobject_cast<QComboBox*>(controllerParam->createEditor(paramGroup));
        QComboBox* targetBox = qobject_cast<QComboBox*>(targetParam->createEditor(paramGroup));
        if (!controllerBox || !targetBox) {
            continue;
        }
        
        auto applyOptions = [targetParam, targetBox, rule](const QString& currentValue) {
            QStringList opts = rule.optionsByValue.value(currentValue);
            if (opts.isEmpty()) {
                return; // 没有匹配时保持现有选项
            }
            const QSignalBlocker blocker(targetBox);
            targetBox->clear();
            targetBox->addItems(opts);
            targetParam->setOptions(opts);
            QString current = targetParam->getValue().toString();
            int idx = opts.indexOf(current);
            if (idx < 0 && !opts.isEmpty()) {
                targetParam->setValue(opts.first());
                targetBox->setCurrentIndex(0);
            } else if (idx >= 0) {
                targetBox->setCurrentIndex(idx);
            }
        };
        
        connect(controllerBox, &QComboBox::currentTextChanged, this, applyOptions);
        applyOptions(controllerParam->getValue().toString());
    }
}

void WorkStateTabWidget::createSaveButton()
{
    if (!m_contentWidget) {
        return;
    }
    
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(m_contentWidget->layout());
    if (!mainLayout) {
        return;
    }
    
    // 创建按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    
    m_saveButton = new QPushButton(QString(u8"保存工作状态 %1").arg(m_stateIndex + 1));
    m_saveButton->setMinimumHeight(30);
    connect(m_saveButton, &QPushButton::clicked, this, &WorkStateTabWidget::onSaveButtonClicked);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addStretch();
    
    // 插入到最后一个stretch之前
    mainLayout->insertLayout(mainLayout->count() - 1, buttonLayout);
}

void WorkStateTabWidget::onParameterValueChanged()
{
    updateStateValues();
}

void WorkStateTabWidget::onSaveButtonClicked()
{
    // 先更新当前值
    updateStateValues();
    
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
                                         QString(u8"工作状态 %1 已保存到当前配置文件")
                                             .arg(m_stateIndex + 1));
            }
        } else {
            // 如果没有当前文件，提供单独保存选项
            QString fileName = QFileDialog::getSaveFileName(this, 
                                                            QString(u8"保存工作状态 %1").arg(m_stateIndex + 1),
                                                            QString("work_state_%1_%2.json")
                                                                .arg(m_device->getDeviceId())
                                                                .arg(m_stateIndex + 1),
                                                            u8"JSON文件 (*.json)");
            
            if (!fileName.isEmpty()) {
                if (saveStateToJson(fileName)) {
                    QMessageBox::information(this, u8"保存成功", 
                                             QString(u8"工作状态 %1 已保存到: %2")
                                                 .arg(m_stateIndex + 1)
                                                 .arg(fileName));
                }
            }
        }
    }
}

bool WorkStateTabWidget::saveStateToJson(const QString& fileName)
{
    // 先更新当前值
    updateStateValues();
    
    QJsonObject rootObj;
    QJsonObject stateObj;
    
    stateObj["device_id"] = m_device->getDeviceId();
    stateObj["device_name"] = m_device->getDeviceName();
    stateObj["state_index"] = m_stateIndex;
    stateObj["equipment_type"] = m_device->getEquipmentType()->getTypeName();
    
    // 保存参数值
    QJsonObject valuesObj;
    QVariantMap currentValues = m_device->getWorkStateValues(m_stateIndex);
    for (auto it = currentValues.begin(); it != currentValues.end(); ++it) {
        valuesObj[it.key()] = it.value().toString();
    }
    stateObj["parameter_values"] = valuesObj;
    
    // 保存参数定义（方便查看）
    QJsonArray paramDefsArray;
    for (auto it = m_parameterInstances.begin(); it != m_parameterInstances.end(); ++it) {
        ParameterItem* param = it.value();
        QJsonObject paramObj;
        paramObj["id"] = param->getId();
        paramObj["label"] = param->getLabel();
        paramObj["type"] = param->getType();
        paramObj["unit"] = param->getUnit();
        paramObj["current_value"] = param->getValue().toString();
        
        paramDefsArray.append(paramObj);
    }
    stateObj["parameter_definitions"] = paramDefsArray;
    
    rootObj["work_state"] = stateObj;
    
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
    
    qDebug() << QString(u8"工作状态 %1 保存完成: %2").arg(m_stateIndex + 1).arg(fileName);
    return true;
}

void WorkStateTabWidget::updateStateValues()
{
    if (!m_device) {
        return;
    }
    
    QVariantMap newValues;
    
    // 收集所有参数的当前值
    for (auto it = m_parameterInstances.begin(); it != m_parameterInstances.end(); ++it) {
        ParameterItem* param = it.value();
        newValues[param->getId()] = param->getValue();
    }
    
    // 更新设备实例中的状态值
    m_device->setWorkStateValues(m_stateIndex, newValues);
} 
