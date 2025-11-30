#pragma once

#include "EquipmentType.h"
#include "DeviceInstance.h"
#include <QTabWidget>
#include <QList>
#include <QMap>
#include <QString>
#include <QJsonObject>

class EquipmentConfigWidget : public QTabWidget {
    Q_OBJECT

public:
    explicit EquipmentConfigWidget(QWidget* parent = nullptr);
    ~EquipmentConfigWidget();

    bool loadFromJson(const QString& jsonFile);
    bool saveToJson(const QString& jsonFile);
    bool autoSave(); // 自动保存到当前文件
    bool saveCurrentValues(); // 保存当前所有参数值（不改变文件结构）
    void updateAllVisibility();
    bool validateAll();
    bool openStructureEditor(); // 打开结构编辑模式

    // 文件路径管理
    QString getCurrentFilePath() const { return m_currentFilePath; }
    void setCurrentFilePath(const QString& filePath) { m_currentFilePath = filePath; }
    bool hasCurrentFile() const { return !m_currentFilePath.isEmpty(); }

signals:
    void configChanged();
    void validationError(const QString& message);
    void filePathChanged(const QString& filePath);

private slots:
    void onConfigurationChanged();

private:
    QList<EquipmentType*> m_equipmentTypes;
    QMap<QString, QList<DeviceInstance*>> m_deviceInstances; // typeId -> devices
    QString m_currentFilePath; // 当前打开的文件路径
    QJsonObject m_lastRootObject; // 缓存当前配置的原始 JSON
    bool m_isLoading = false; // 标记是否处于加载阶段，避免重复刷新

    void createEquipmentTypeTabs();
    void createDeviceTabs(const QString& typeId, QTabWidget* parentTab);
    void clearAll();
    
    // 参数值的保存和加载
    void loadDeviceInstanceValues(const QJsonObject& configObj);
    void saveDeviceInstanceValues(QJsonObject& configObj) const;
}; 
