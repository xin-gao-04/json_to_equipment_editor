#pragma once

#include <QDialog>
#include <QJsonObject>
#include <QJsonArray>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QPoint>
#include <QTextEdit>

class ConfigEditorDialog : public QDialog {
    Q_OBJECT
public:
    ConfigEditorDialog(const QJsonObject& rootObj, const QString& filePath, QWidget* parent = nullptr);
    QJsonObject resultConfig() const { return m_rootObj; }
    bool changed() const { return m_changed; }

private slots:
    void onTypeSelectionChanged();
    void onAddType();
    void onEditType();
    void onRemoveType();
    void onDeviceCountChanged(int value);
    void onAddBasic();
    void onEditBasic();
    void onRemoveBasic();
    void onAddWorkParam();
    void onEditWorkParam();
    void onRemoveWorkParam();
    void onSave();
    void onTypeContextMenu(const QPoint& pos);
    void onBasicContextMenu(const QPoint& pos);
    void onWorkContextMenu(const QPoint& pos);
    void onStateCountOverrideChanged(int value);
    void onStateTitlesChanged();

private:
    QJsonObject m_rootObj;
    QJsonArray m_types;
    QString m_filePath;
    bool m_changed = false;

    QListWidget* m_typeList;
    QSpinBox* m_deviceCountSpin;
    QListWidget* m_basicList;
    QListWidget* m_workList;
    QSpinBox* m_stateCountOverride;
    QTextEdit* m_stateTitlesEdit;
    QPushButton* m_saveButton;

    int currentTypeIndex() const;
    QJsonObject currentTypeObj() const;
    void updateTypeList();
    void updateDetail();
    void updateParamLists(const QJsonObject& typeObj);
    void backupFile();
    void writeFile();
    static QString paramDisplay(const QJsonObject& obj);
    void applyStateMetaChanges();
};
