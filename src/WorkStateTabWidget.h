#pragma once

#include "DeviceInstance.h"
#include <QScrollArea>
#include <QWidget>
#include <QMap>
#include <QPushButton>

class ParameterItem;
class EquipmentConfigWidget;
class QLabel;

class WorkStateTabWidget : public QScrollArea {
    Q_OBJECT

public:
    explicit WorkStateTabWidget(DeviceInstance* device, int stateIndex, QWidget* parent = nullptr);

signals:
    void parameterChanged(const QString& parameterId, const QVariant& value);
    void saveRequested(int stateIndex);

private slots:
    void onParameterValueChanged();
    void onSaveButtonClicked();

private:
    DeviceInstance* m_device;
    int m_stateIndex;
    QWidget* m_contentWidget;
    QMap<QString, ParameterItem*> m_parameterInstances; // 存储每个参数的实例
    QMap<QString, QLabel*> m_labelWidgets; // 存储label以便联动显隐
    QPushButton* m_saveButton;
    
    void createParameterWidgets();
    void updateStateValues();
    void createSaveButton();
    bool saveStateToJson(const QString& fileName);
}; 
