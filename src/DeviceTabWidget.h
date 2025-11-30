#pragma once

#include "DeviceInstance.h"
#include <QTabWidget>
#include <QWidget>
#include <QPushButton>
#include <QMap>
#include <QLabel>

class EquipmentConfigWidget;
class ParameterItem;

class DeviceTabWidget : public QTabWidget {
    Q_OBJECT

public:
    explicit DeviceTabWidget(DeviceInstance* device, QWidget* parent = nullptr);
    
    // 更新界面可见性
    void updateVisibility();

signals:
    void deviceSaveRequested(DeviceInstance* device);

private slots:
    void onBasicParameterChanged();
    void onWorkStateCountChanged();
    void onSaveDeviceButtonClicked();
    void onSaveBasicButtonClicked();

private:
    DeviceInstance* m_device;
    QWidget* m_basicParamsWidget;
    QPushButton* m_saveDeviceButton;
    QPushButton* m_saveBasicButton;
    QMap<QString, ParameterItem*> m_basicParameterInstances;
    QMap<QString, QLabel*> m_basicLabelWidgets;
    QMap<QString, QWidget*> m_basicRowWidgets;
    int m_lastStateCount = -1;

    void createBasicParametersTab();
    void createWorkStateTabs();
    void updateWorkStateTabs();
    void createSaveButtons();
    bool saveDeviceToJson(const QString& fileName);
    bool saveBasicParametersToJson(const QString& fileName);
}; 
