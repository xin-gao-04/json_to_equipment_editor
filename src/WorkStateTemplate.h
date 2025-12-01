#pragma once

#include "ParameterItem.h"
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QWidget>
#include <QVariantMap>
#include <QMap>
#include <QVector>
#include <QSet>

class WorkStateTemplate {
public:
    WorkStateTemplate(const QString& templateId, const QString& name);
    ~WorkStateTemplate();

    QString getTemplateId() const { return m_templateId; }
    QString getTemplateName() const { return m_name; }
    QString getName() const { return m_name; }
    const QList<ParameterItem*>& getParameters() const { return m_parameters; }
    
    void addParameter(ParameterItem* parameter);
    ParameterItem* getParameter(const QString& id) const;
    
    // 创建状态参数编辑界面
    QWidget* createStateWidget(int stateIndex, QWidget* parent);
    
    // 获取/设置状态值
    QVariantMap getStateValues(int stateIndex) const;
    void setStateValues(int stateIndex, const QVariantMap& values);
    
    // 从JSON加载
    static WorkStateTemplate* fromJson(const QJsonObject& json);
    
    struct VisibilityCase {
        QString value;
        QStringList showIds;
    };
    struct VisibilityRule {
        QString controllerId;
        QList<VisibilityCase> cases;
        QSet<QString> affectedIds;
    };
    struct OptionRule {
        QString controllerId;
        QString targetId;
        QMap<QString, QStringList> optionsByValue;
    };
    QStringList getStateTabTitles() const { return m_stateTabTitles; }
    int getStateTabCountOverride() const { return m_stateTabCountOverride; }
    const QVector<VisibilityRule>& getVisibilityRules() const { return m_visibilityRules; }
    const QVector<OptionRule>& getOptionRules() const { return m_optionRules; }
    QJsonArray getVisibilityRulesJson() const;
    QJsonArray getOptionRulesJson() const;
    QJsonArray getValidationRulesJson() const { return m_validationRules; }

private:
    QString m_templateId;
    QString m_name;
    QList<ParameterItem*> m_parameters;
    
    // 存储每个状态实例的参数值 stateIndex -> (parameterId -> value)
    mutable QMap<int, QVariantMap> m_stateValues;
    QVector<VisibilityRule> m_visibilityRules;
    QVector<OptionRule> m_optionRules;
    QJsonArray m_validationRules;
    QStringList m_stateTabTitles;
    int m_stateTabCountOverride = -1;
};
