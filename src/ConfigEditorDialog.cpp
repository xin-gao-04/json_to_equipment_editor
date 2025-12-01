#include "ConfigEditorDialog.h"
#include "ParameterEditDialog.h"
#include "TypeEditDialog.h"
#include "RuleEditorDialog.h"
#include "StyleHelper.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QMenu>
#include <QTextEdit>
#include <QFormLayout>
#include <QDateTime>

ConfigEditorDialog::ConfigEditorDialog(const QJsonObject& rootObj, const QString& filePath, QWidget* parent)
    : QDialog(parent), m_rootObj(rootObj), m_filePath(filePath)
{
    setWindowTitle(u8"结构编辑模式");
    setMinimumSize(1000, 650);
    setStyleSheet(StyleHelper::darkPanelStyle());

    // 拉取 equipment_types
    QJsonObject ecObj = m_rootObj.value("equipment_config").toObject();
    m_types = ecObj.value("equipment_types").toArray();

    m_typeList = new QListWidget;
    m_typeList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_typeList, &QListWidget::currentRowChanged, this, &ConfigEditorDialog::onTypeSelectionChanged);
    m_typeList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_typeList, &QListWidget::customContextMenuRequested, this, &ConfigEditorDialog::onTypeContextMenu);

    QPushButton* addTypeBtn = new QPushButton(u8"新增类型");
    QPushButton* editTypeBtn = new QPushButton(u8"编辑类型");
    QPushButton* removeTypeBtn = new QPushButton(u8"删除类型");
    connect(addTypeBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onAddType);
    connect(editTypeBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onEditType);
    connect(removeTypeBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onRemoveType);

    QVBoxLayout* typeLayout = new QVBoxLayout;
    typeLayout->addWidget(new QLabel(u8"设备类型"));
    typeLayout->addWidget(m_typeList, 1);
    typeLayout->addWidget(addTypeBtn);
    typeLayout->addWidget(editTypeBtn);
    typeLayout->addWidget(removeTypeBtn);

    // 右侧：设备数量 + 参数
    m_deviceCountSpin = new QSpinBox;
    m_deviceCountSpin->setRange(0, 64);
    connect(m_deviceCountSpin,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &ConfigEditorDialog::onDeviceCountChanged);

    QHBoxLayout* dcLayout = new QHBoxLayout;
    dcLayout->addWidget(new QLabel(u8"设备数量"));
    dcLayout->addWidget(m_deviceCountSpin);
    dcLayout->addStretch();

    m_basicList = new QListWidget;
    m_basicList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_basicList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_basicList, &QListWidget::customContextMenuRequested, this, &ConfigEditorDialog::onBasicContextMenu);
    QPushButton* addBasicBtn = new QPushButton(u8"新增基本参数");
    QPushButton* editBasicBtn = new QPushButton(u8"编辑基本参数");
    QPushButton* removeBasicBtn = new QPushButton(u8"删除基本参数");
    connect(addBasicBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onAddBasic);
    connect(editBasicBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onEditBasic);
    connect(removeBasicBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onRemoveBasic);

    QVBoxLayout* basicLayout = new QVBoxLayout;
    basicLayout->addWidget(new QLabel(u8"基本参数模板"));
    basicLayout->addWidget(m_basicList, 1);
    basicLayout->addWidget(addBasicBtn);
    basicLayout->addWidget(editBasicBtn);
    basicLayout->addWidget(removeBasicBtn);

    m_workList = new QListWidget;
    m_workList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_workList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_workList, &QListWidget::customContextMenuRequested, this, &ConfigEditorDialog::onWorkContextMenu);
    QPushButton* addWorkBtn = new QPushButton(u8"新增状态参数");
    QPushButton* editWorkBtn = new QPushButton(u8"编辑状态参数");
    QPushButton* removeWorkBtn = new QPushButton(u8"删除状态参数");
    connect(addWorkBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onAddWorkParam);
    connect(editWorkBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onEditWorkParam);
    connect(removeWorkBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onRemoveWorkParam);

    QVBoxLayout* workLayout = new QVBoxLayout;
    workLayout->addWidget(new QLabel(u8"工作状态模板参数"));
    workLayout->addWidget(m_workList, 1);
    workLayout->addWidget(addWorkBtn);
    workLayout->addWidget(editWorkBtn);
    workLayout->addWidget(removeWorkBtn);

    // 工作状态标签/数量设置
    m_stateCountOverride = new QSpinBox;
    m_stateCountOverride->setRange(-1, 64);
    m_stateCountOverride->setSpecialValueText(u8"不覆盖");
    connect(m_stateCountOverride,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &ConfigEditorDialog::onStateCountOverrideChanged);

    m_stateTitlesEdit = new QTextEdit;
    m_stateTitlesEdit->setPlaceholderText(u8"状态标签，一行一个；为空则使用默认“工作状态1/2/...”");
    connect(m_stateTitlesEdit, &QTextEdit::textChanged, this, &ConfigEditorDialog::onStateTitlesChanged);

    QGroupBox* stateMetaGroup = new QGroupBox(u8"状态页签配置");
    QFormLayout* stateMetaForm = new QFormLayout(stateMetaGroup);
    stateMetaForm->addRow(u8"状态数量覆盖", m_stateCountOverride);
    stateMetaForm->addRow(u8"状态标签列表", m_stateTitlesEdit);

    // 规则编辑区域（visibility_rules / option_rules / validation_rules）
    QGroupBox* rulesGroup = new QGroupBox(u8"模板规则（可见性 / 选项 / 校验说明）");
    m_rulesEdit = new QTextEdit;
    m_rulesEdit->setPlaceholderText(u8"此处编辑当前设备类型工作状态模板的规则 JSON。\n"
                                    u8"示例:\n"
                                    u8"{\n"
                                    u8"  \"visibility_rules\": [ ... ],\n"
                                    u8"  \"option_rules\": [ ... ],\n"
                                    u8"  \"validation_rules\": [ ... ]\n"
                                    u8"}");
    QFont monoFont = m_rulesEdit->font();
    monoFont.setFamily(QStringLiteral("Monospace"));
    m_rulesEdit->setFont(monoFont);
    QPushButton* applyRulesBtn = new QPushButton(u8"应用规则更改");
    connect(applyRulesBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onApplyRules);
    QPushButton* guiRulesBtn = new QPushButton(u8"图形化编辑...");
    connect(guiRulesBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onOpenRuleEditor);

    QVBoxLayout* rulesLayout = new QVBoxLayout(rulesGroup);
    rulesLayout->addWidget(m_rulesEdit, 1);
    QHBoxLayout* rulesButtonLayout = new QHBoxLayout;
    rulesButtonLayout->addWidget(applyRulesBtn);
    rulesButtonLayout->addWidget(guiRulesBtn);
    rulesButtonLayout->addStretch();
    rulesLayout->addLayout(rulesButtonLayout);

    QHBoxLayout* rightLayout = new QHBoxLayout;
    rightLayout->addLayout(basicLayout, 1);
    QVBoxLayout* workSide = new QVBoxLayout;
    workSide->addLayout(workLayout);
    workSide->addWidget(stateMetaGroup);
    workSide->addWidget(rulesGroup, 1);
    rightLayout->addLayout(workSide, 2);

    QVBoxLayout* rightPanel = new QVBoxLayout;
    rightPanel->addLayout(dcLayout);
    rightPanel->addLayout(rightLayout, 1);

    // 主体区域布局（不直接挂到 this，避免重复 setLayout 警告）
    QHBoxLayout* mainLayout = new QHBoxLayout;
    mainLayout->addLayout(typeLayout, 1);
    mainLayout->addLayout(rightPanel, 2);

    // 底部操作按钮：显式放置，避免样式导致的不可见问题
    m_saveButton = new QPushButton(u8"保存结构并重载");
    QPushButton* closeBtn = new QPushButton(u8"关闭");
    connect(m_saveButton, &QPushButton::clicked, this, &ConfigEditorDialog::onSave);
    connect(closeBtn, &QPushButton::clicked, this, &ConfigEditorDialog::reject);
    QHBoxLayout* actionLayout = new QHBoxLayout;
    actionLayout->addStretch();
    actionLayout->addWidget(m_saveButton);
    actionLayout->addWidget(closeBtn);

    QVBoxLayout* outer = new QVBoxLayout;
    outer->addLayout(mainLayout, 1);
    outer->addLayout(actionLayout);
    setLayout(outer);

    updateTypeList();
}

void ConfigEditorDialog::updateTypeList()
{
    m_typeList->clear();
    for (int i = 0; i < m_types.size(); ++i) {
        QJsonObject obj = m_types[i].toObject();
        QString text = QString("%1 (%2)").arg(obj["type_name"].toString(), obj["type_id"].toString());
        QListWidgetItem* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, i);
        m_typeList->addItem(item);
    }
    if (m_typeList->count() > 0) {
        m_typeList->setCurrentRow(0);
    } else {
        m_deviceCountSpin->setValue(0);
        m_basicList->clear();
        m_workList->clear();
    }
}

int ConfigEditorDialog::currentTypeIndex() const
{
    QListWidgetItem* item = m_typeList->currentItem();
    if (!item) return -1;
    return item->data(Qt::UserRole).toInt();
}

QJsonObject ConfigEditorDialog::currentTypeObj() const
{
    int idx = currentTypeIndex();
    if (idx < 0 || idx >= m_types.size()) {
        return QJsonObject();
    }
    return m_types[idx].toObject();
}

void ConfigEditorDialog::onTypeSelectionChanged()
{
    updateDetail();
}

void ConfigEditorDialog::updateDetail()
{
    QJsonObject obj = currentTypeObj();
    if (obj.isEmpty()) {
        m_deviceCountSpin->setValue(0);
        m_basicList->clear();
        m_workList->clear();
        return;
    }
    m_deviceCountSpin->blockSignals(true);
    m_deviceCountSpin->setValue(obj.value("device_count").toInt(0));
    m_deviceCountSpin->blockSignals(false);
    updateParamLists(obj);
}

QString ConfigEditorDialog::paramDisplay(const QJsonObject& obj)
{
    QString type = obj["type"].toString();
    QString label = obj["label"].toString();
    QString id = obj["id"].toString();
    return QString("%1 (%2) - %3").arg(label, id, type);
}

void ConfigEditorDialog::updateParamLists(const QJsonObject& typeObj)
{
    m_basicList->clear();
    m_workList->clear();
    QJsonArray basics = typeObj.value("basic_parameters").toArray();
    for (int i = 0; i < basics.size(); ++i) {
        QJsonObject p = basics[i].toObject();
        QListWidgetItem* item = new QListWidgetItem(paramDisplay(p));
        item->setData(Qt::UserRole, i);
        m_basicList->addItem(item);
    }
    QJsonObject wsObj = typeObj.value("work_state_template").toObject();
    QJsonArray wsParams = wsObj.value("parameters").toArray();
    for (int i = 0; i < wsParams.size(); ++i) {
        QJsonObject p = wsParams[i].toObject();
        QListWidgetItem* item = new QListWidgetItem(paramDisplay(p));
        item->setData(Qt::UserRole, i);
        m_workList->addItem(item);
    }

    // 同步状态数量覆盖与标签
    m_stateCountOverride->blockSignals(true);
    if (wsObj.contains("state_tab_count")) {
        m_stateCountOverride->setValue(wsObj.value("state_tab_count").toInt());
    } else {
        m_stateCountOverride->setValue(-1); // 不覆盖
    }
    m_stateCountOverride->blockSignals(false);

    m_stateTitlesEdit->blockSignals(true);
    QStringList titles;
    QJsonArray titlesArr = wsObj.value("state_tab_titles").toArray();
    for (const auto& v : titlesArr) {
        titles << v.toString();
    }
    m_stateTitlesEdit->setPlainText(titles.join("\n"));
    m_stateTitlesEdit->blockSignals(false);

    // 同步规则 JSON 视图
    if (m_rulesEdit) {
        QJsonObject rulesObj;
        if (wsObj.contains("visibility_rules")) {
            rulesObj.insert("visibility_rules", wsObj.value("visibility_rules"));
        }
        if (wsObj.contains("option_rules")) {
            rulesObj.insert("option_rules", wsObj.value("option_rules"));
        }
        if (wsObj.contains("validation_rules")) {
            rulesObj.insert("validation_rules", wsObj.value("validation_rules"));
        }
        m_rulesEdit->blockSignals(true);
        if (rulesObj.isEmpty()) {
            m_rulesEdit->clear();
        } else {
            QJsonDocument doc(rulesObj);
            m_rulesEdit->setPlainText(QString::fromUtf8(doc.toJson(QJsonDocument::Indented)));
        }
        m_rulesEdit->blockSignals(false);
    }
}

void ConfigEditorDialog::onAddType()
{
    TypeEditDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QJsonObject obj = dlg.type();
        m_types.append(obj);
        m_changed = true;
        updateTypeList();
    }
}

void ConfigEditorDialog::onEditType()
{
    int idx = currentTypeIndex();
    if (idx < 0) return;
    QJsonObject obj = m_types[idx].toObject();
    TypeEditDialog dlg(this);
    dlg.setType(obj);
    if (dlg.exec() == QDialog::Accepted) {
        m_types[idx] = dlg.type();
        m_changed = true;
        updateTypeList();
    }
}

void ConfigEditorDialog::onRemoveType()
{
    int idx = currentTypeIndex();
    if (idx < 0) return;
    if (QMessageBox::question(this, u8"删除类型", u8"确定删除该设备类型及其定义？") != QMessageBox::Yes) {
        return;
    }
    m_types.removeAt(idx);
    m_changed = true;
    updateTypeList();
}

void ConfigEditorDialog::onDeviceCountChanged(int value)
{
    int idx = currentTypeIndex();
    if (idx < 0) return;
    QJsonObject obj = m_types[idx].toObject();
    obj["device_count"] = value;
    m_types[idx] = obj;
    m_changed = true;
}

void ConfigEditorDialog::onAddBasic()
{
    int idx = currentTypeIndex();
    if (idx < 0) return;
    ParameterEditDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QJsonObject typeObj = m_types[idx].toObject();
        QJsonArray basics = typeObj.value("basic_parameters").toArray();
        QJsonObject paramObj = dlg.parameter();
        QString newId = paramObj["id"].toString();
        for (const auto& v : basics) {
            if (v.toObject().value("id").toString() == newId) {
                QMessageBox::warning(this, u8"重复ID", u8"该ID已存在于基本参数中");
                return;
            }
        }
        basics.append(paramObj);
        typeObj["basic_parameters"] = basics;
        m_types[idx] = typeObj;
        m_changed = true;
        updateParamLists(typeObj);
    }
}

void ConfigEditorDialog::onEditBasic()
{
    int idx = currentTypeIndex();
    if (idx < 0) return;
    int row = m_basicList->currentRow();
    if (row < 0) return;
    QJsonObject typeObj = m_types[idx].toObject();
    QJsonArray basics = typeObj.value("basic_parameters").toArray();
    if (row >= basics.size()) return;
    ParameterEditDialog dlg(this);
    dlg.setParameter(basics[row].toObject());
    if (dlg.exec() == QDialog::Accepted) {
        QJsonObject paramObj = dlg.parameter();
        QString newId = paramObj["id"].toString();
        for (int i = 0; i < basics.size(); ++i) {
            if (i == row) continue;
            if (basics[i].toObject().value("id").toString() == newId) {
                QMessageBox::warning(this, u8"重复ID", u8"该ID已存在于基本参数中");
                return;
            }
        }
        basics[row] = paramObj;
        typeObj["basic_parameters"] = basics;
        m_types[idx] = typeObj;
        m_changed = true;
        updateParamLists(typeObj);
    }
}

void ConfigEditorDialog::onRemoveBasic()
{
    int idx = currentTypeIndex();
    if (idx < 0) return;
    int row = m_basicList->currentRow();
    if (row < 0) return;
    if (QMessageBox::question(this, u8"删除参数", u8"确定删除该基本参数？") != QMessageBox::Yes) {
        return;
    }
    QJsonObject typeObj = m_types[idx].toObject();
    QJsonArray basics = typeObj.value("basic_parameters").toArray();
    if (row < basics.size()) {
        basics.removeAt(row);
        typeObj["basic_parameters"] = basics;
        m_types[idx] = typeObj;
        m_changed = true;
        updateParamLists(typeObj);
    }
}

void ConfigEditorDialog::onAddWorkParam()
{
    int idx = currentTypeIndex();
    if (idx < 0) return;
    ParameterEditDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QJsonObject typeObj = m_types[idx].toObject();
        QJsonObject ws = typeObj.value("work_state_template").toObject();
        QJsonArray params = ws.value("parameters").toArray();
        QJsonObject paramObj = dlg.parameter();
        QString newId = paramObj["id"].toString();
        for (const auto& v : params) {
            if (v.toObject().value("id").toString() == newId) {
                QMessageBox::warning(this, u8"重复ID", u8"该ID已存在于工作状态参数中");
                return;
            }
        }
        params.append(paramObj);
        ws["parameters"] = params;
        typeObj["work_state_template"] = ws;
        m_types[idx] = typeObj;
        m_changed = true;
        updateParamLists(typeObj);
    }
}

void ConfigEditorDialog::onEditWorkParam()
{
    int idx = currentTypeIndex();
    if (idx < 0) return;
    int row = m_workList->currentRow();
    if (row < 0) return;
    QJsonObject typeObj = m_types[idx].toObject();
    QJsonObject ws = typeObj.value("work_state_template").toObject();
    QJsonArray params = ws.value("parameters").toArray();
    if (row >= params.size()) return;
    ParameterEditDialog dlg(this);
    dlg.setParameter(params[row].toObject());
    if (dlg.exec() == QDialog::Accepted) {
        QJsonObject paramObj = dlg.parameter();
        QString newId = paramObj["id"].toString();
        for (int i = 0; i < params.size(); ++i) {
            if (i == row) continue;
            if (params[i].toObject().value("id").toString() == newId) {
                QMessageBox::warning(this, u8"重复ID", u8"该ID已存在于工作状态参数中");
                return;
            }
        }
        params[row] = paramObj;
        ws["parameters"] = params;
        typeObj["work_state_template"] = ws;
        m_types[idx] = typeObj;
        m_changed = true;
        updateParamLists(typeObj);
    }
}

void ConfigEditorDialog::onRemoveWorkParam()
{
    int idx = currentTypeIndex();
    if (idx < 0) return;
    int row = m_workList->currentRow();
    if (row < 0) return;
    if (QMessageBox::question(this, u8"删除参数", u8"确定删除该状态参数？") != QMessageBox::Yes) {
        return;
    }
    QJsonObject typeObj = m_types[idx].toObject();
    QJsonObject ws = typeObj.value("work_state_template").toObject();
    QJsonArray params = ws.value("parameters").toArray();
    if (row < params.size()) {
        params.removeAt(row);
        ws["parameters"] = params;
        typeObj["work_state_template"] = ws;
        m_types[idx] = typeObj;
        m_changed = true;
        updateParamLists(typeObj);
    }
}

void ConfigEditorDialog::backupFile()
{
    if (m_filePath.isEmpty()) return;
    QFileInfo fi(m_filePath);
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString backup = fi.path() + "/" + fi.completeBaseName() + ".backup_" + timestamp + ".json";
    QFile::remove(backup);
    QFile::copy(m_filePath, backup);
}

void ConfigEditorDialog::writeFile()
{
    QJsonObject ecObj = m_rootObj.value("equipment_config").toObject();
    ecObj["equipment_types"] = m_types;
    m_rootObj["equipment_config"] = ecObj;

    if (!m_filePath.isEmpty()) {
        QFile f(m_filePath);
        if (!f.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, u8"保存失败", u8"无法写入文件");
            return;
        }
        QJsonDocument doc(m_rootObj);
        f.write(doc.toJson());
        f.close();
    }
}

void ConfigEditorDialog::onSave()
{
    if (m_changed) {
        backupFile();
        writeFile();
    }
    // 保持 m_changed 为 true 以便上层重新加载最新文件
    accept();
}

void ConfigEditorDialog::applyStateMetaChanges()
{
    int idx = currentTypeIndex();
    if (idx < 0) return;
    QJsonObject typeObj = m_types[idx].toObject();
    QJsonObject wsObj = typeObj.value("work_state_template").toObject();

    // 数量覆盖
    int count = m_stateCountOverride->value();
    if (count >= 0) {
        wsObj["state_tab_count"] = count;
    } else {
        wsObj.remove("state_tab_count");
    }

    // 标签列表
    // 兼容旧版 Qt：使用 QString::SkipEmptyParts 跳过空行
    QStringList titles = m_stateTitlesEdit->toPlainText().split(QRegularExpression("[\\r\\n]+"), QString::SkipEmptyParts);
    if (!titles.isEmpty()) {
        QJsonArray arr;
        for (const auto& t : titles) {
            arr.append(t.trimmed());
        }
        wsObj["state_tab_titles"] = arr;
    } else {
        wsObj.remove("state_tab_titles");
    }

    typeObj["work_state_template"] = wsObj;
    m_types[idx] = typeObj;
    m_changed = true;
}

void ConfigEditorDialog::onStateCountOverrideChanged(int)
{
    applyStateMetaChanges();
}

void ConfigEditorDialog::onStateTitlesChanged()
{
    applyStateMetaChanges();
}

void ConfigEditorDialog::onTypeContextMenu(const QPoint& pos)
{
    QMenu menu(this);
    QAction* addAct = menu.addAction(u8"新增类型");
    QAction* editAct = menu.addAction(u8"编辑类型");
    QAction* delAct = menu.addAction(u8"删除类型");
    QAction* chosen = menu.exec(m_typeList->viewport()->mapToGlobal(pos));
    if (!chosen) return;
    if (chosen == addAct) onAddType();
    else if (chosen == editAct) onEditType();
    else if (chosen == delAct) onRemoveType();
}

void ConfigEditorDialog::onBasicContextMenu(const QPoint& pos)
{
    QMenu menu(this);
    QAction* addAct = menu.addAction(u8"新增基本参数");
    QAction* editAct = menu.addAction(u8"编辑基本参数");
    QAction* delAct = menu.addAction(u8"删除基本参数");
    QAction* chosen = menu.exec(m_basicList->viewport()->mapToGlobal(pos));
    if (!chosen) return;
    if (chosen == addAct) onAddBasic();
    else if (chosen == editAct) onEditBasic();
    else if (chosen == delAct) onRemoveBasic();
}

void ConfigEditorDialog::onWorkContextMenu(const QPoint& pos)
{
    QMenu menu(this);
    QAction* addAct = menu.addAction(u8"新增状态参数");
    QAction* editAct = menu.addAction(u8"编辑状态参数");
    QAction* delAct = menu.addAction(u8"删除状态参数");
    QAction* chosen = menu.exec(m_workList->viewport()->mapToGlobal(pos));
    if (!chosen) return;
    if (chosen == addAct) onAddWorkParam();
    else if (chosen == editAct) onEditWorkParam();
    else if (chosen == delAct) onRemoveWorkParam();
}

void ConfigEditorDialog::onOpenRuleEditor()
{
    int idx = currentTypeIndex();
    if (idx < 0) return;

    QJsonObject typeObj = m_types[idx].toObject();
    QJsonObject wsObj = typeObj.value("work_state_template").toObject();

    // 提取当前规则到独立对象
    QJsonObject rulesObj;
    if (wsObj.contains("visibility_rules")) {
        rulesObj.insert("visibility_rules", wsObj.value("visibility_rules"));
    }
    if (wsObj.contains("option_rules")) {
        rulesObj.insert("option_rules", wsObj.value("option_rules"));
    }
    if (wsObj.contains("validation_rules")) {
        rulesObj.insert("validation_rules", wsObj.value("validation_rules"));
    }

    // 汇总当前工作状态参数ID及其枚举选项，供规则编辑下拉选择
    QStringList paramIds;
    QMap<QString, QStringList> paramOptions;
    QJsonArray paramsArr = wsObj.value("parameters").toArray();
    for (const auto& v : paramsArr) {
        QJsonObject p = v.toObject();
        QString id = p.value("id").toString();
        paramIds << id;
        QString type = p.value("type").toString();
        if (type == "enum") {
            QStringList opts;
            QJsonArray arr = p.value("options").toArray();
            for (const auto& o : arr) {
                opts << o.toString();
            }
            paramOptions[id] = opts;
        }
    }

    RuleEditorDialog dlg(rulesObj, paramIds, paramOptions, this);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    QJsonObject newRules = dlg.rules();
    if (newRules.contains("visibility_rules")) {
        wsObj.insert("visibility_rules", newRules.value("visibility_rules"));
    } else {
        wsObj.remove("visibility_rules");
    }
    if (newRules.contains("option_rules")) {
        wsObj.insert("option_rules", newRules.value("option_rules"));
    } else {
        wsObj.remove("option_rules");
    }
    if (newRules.contains("validation_rules")) {
        wsObj.insert("validation_rules", newRules.value("validation_rules"));
    } else {
        wsObj.remove("validation_rules");
    }

    typeObj["work_state_template"] = wsObj;
    m_types[idx] = typeObj;
    m_changed = true;

    // 同步文本规则编辑框显示
    if (m_rulesEdit) {
        QJsonObject displayRules;
        if (wsObj.contains("visibility_rules")) {
            displayRules.insert("visibility_rules", wsObj.value("visibility_rules"));
        }
        if (wsObj.contains("option_rules")) {
            displayRules.insert("option_rules", wsObj.value("option_rules"));
        }
        if (wsObj.contains("validation_rules")) {
            displayRules.insert("validation_rules", wsObj.value("validation_rules"));
        }
        m_rulesEdit->blockSignals(true);
        if (displayRules.isEmpty()) {
            m_rulesEdit->clear();
        } else {
            QJsonDocument doc(displayRules);
            m_rulesEdit->setPlainText(QString::fromUtf8(doc.toJson(QJsonDocument::Indented)));
        }
        m_rulesEdit->blockSignals(false);
    }
}

void ConfigEditorDialog::onApplyRules()
{
    int idx = currentTypeIndex();
    if (idx < 0 || !m_rulesEdit) return;

    const QString text = m_rulesEdit->toPlainText().trimmed();
    QJsonObject typeObj = m_types[idx].toObject();
    QJsonObject wsObj = typeObj.value("work_state_template").toObject();

    if (text.isEmpty()) {
        // 清空规则
        wsObj.remove("visibility_rules");
        wsObj.remove("option_rules");
        wsObj.remove("validation_rules");
        typeObj["work_state_template"] = wsObj;
        m_types[idx] = typeObj;
        m_changed = true;
        QMessageBox::information(this, u8"规则更新", u8"已清除当前类型的模板规则。");
        return;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, u8"解析失败",
                             u8"规则 JSON 解析错误：\n" + err.errorString());
        return;
    }

    QJsonObject rulesObj = doc.object();
    if (rulesObj.contains("visibility_rules")) {
        wsObj.insert("visibility_rules", rulesObj.value("visibility_rules"));
    } else {
        wsObj.remove("visibility_rules");
    }
    if (rulesObj.contains("option_rules")) {
        wsObj.insert("option_rules", rulesObj.value("option_rules"));
    } else {
        wsObj.remove("option_rules");
    }
    if (rulesObj.contains("validation_rules")) {
        wsObj.insert("validation_rules", rulesObj.value("validation_rules"));
    } else {
        wsObj.remove("validation_rules");
    }

    typeObj["work_state_template"] = wsObj;
    m_types[idx] = typeObj;
    m_changed = true;
    QMessageBox::information(this, u8"规则更新", u8"规则已应用到当前设备类型。");
}
