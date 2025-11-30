#pragma once

#include <QDialog>
#include <QJsonObject>
#include <QStringList>
#include <QMap>

class QListWidget;
class QListWidgetItem;
class QTabWidget;

// 面向结构编辑器的规则编辑对话框：
// 针对 work_state_template 下的 visibility_rules / option_rules / validation_rules
// 提供简单的人机界面编辑能力。
class RuleEditorDialog : public QDialog {
    Q_OBJECT
public:
    explicit RuleEditorDialog(const QJsonObject& rulesObj,
                              const QStringList& availableParamIds,
                              const QMap<QString, QStringList>& paramOptions,
                              QWidget* parent = nullptr);

    QJsonObject rules() const { return m_rules; }

private slots:
    // 可见性规则
    void onAddVisibilityRule();
    void onEditVisibilityRule();
    void onRemoveVisibilityRule();

    // 选项规则
    void onAddOptionRule();
    void onEditOptionRule();
    void onRemoveOptionRule();

    // 校验说明
    void onAddValidationRule();
    void onEditValidationRule();
    void onRemoveValidationRule();

private:
    QJsonObject m_rules;
    QTabWidget* m_tabWidget;
    QListWidget* m_visibilityList;
    QListWidget* m_optionList;
    QListWidget* m_validationList;
    QStringList m_paramIds;
    QMap<QString, QStringList> m_paramOptions;

    void buildUi();
    void loadFromRules();

    // 辅助：从列表重建 JSON
    QJsonArray collectVisibilityRules() const;
    QJsonArray collectOptionRules() const;
    QJsonArray collectValidationRules() const;

    // 辅助：单条规则编辑
    bool editVisibilityRuleObject(QJsonObject& obj, QWidget* parent);
    bool editOptionRuleObject(QJsonObject& obj, QWidget* parent);
    bool editValidationRuleObject(QJsonObject& obj, QWidget* parent);

    static QString summarizeVisibilityRule(const QJsonObject& obj);
    static QString summarizeOptionRule(const QJsonObject& obj);
    static QString summarizeValidationRule(const QJsonObject& obj);
    static QString summarizeVisibilityCase(const QJsonObject& obj);
}; 
