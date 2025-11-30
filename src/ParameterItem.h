#pragma once

#include <QString>
#include <QVariant>
#include <QWidget>
#include <QJsonObject>
#include <QStringList>
#include <QObject>
#include <QRegularExpression>

class ParameterItem : public QObject {
    Q_OBJECT

public:
    ParameterItem(const QString& id, const QString& label, const QString& type);
    virtual ~ParameterItem() = default;

    // 基本属性
    QString getId() const { return m_id; }
    QString getLabel() const { return m_label; }
    QString getType() const { return m_type; }
    QString getUnit() const { return m_unit; }
    QVariant getDefaultValue() const { return m_defaultValue; }
    
    // 约束条件的getter方法
    double getMinValue() const { return m_minValue; }
    double getMaxValue() const { return m_maxValue; }
    QStringList getOptions() const { return m_options; }
    bool isArrayLike() const;
    
    // 设置属性
    void setUnit(const QString& unit) { m_unit = unit; }
    void setDefaultValue(const QVariant& value) { m_defaultValue = value; }
    void setRange(double min, double max) { m_minValue = min; m_maxValue = max; }
    void setOptions(const QStringList& options) { m_options = options; }
    
    // 验证
    bool validate(const QVariant& value) const;
    
    // 创建编辑器
    QWidget* createEditor(QWidget* parent);
    
    // 获取和设置值
    QVariant getValue() const;
    void setValue(const QVariant& value);
    
    // 可见性控制
    void setVisible(bool visible);
    bool isVisible() const;
    
    // 从JSON加载
    static ParameterItem* fromJson(const QJsonObject& json);
    static QRegularExpression stringAllowedPattern();

private:
    QString m_id;
    QString m_label;
    QString m_type;
    QString m_unit;
    QVariant m_defaultValue;
    QVariant m_currentValue;
    
    // 约束条件
    double m_minValue = 0.0;
    double m_maxValue = 100.0;
    QStringList m_options;
    
    // 编辑器widget
    QWidget* m_editor = nullptr;
}; 
