#include "RuleEditorDialog.h"

#include <QTabWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QMessageBox>
#include <QSharedPointer>
#include "StyleHelper.h"

RuleEditorDialog::RuleEditorDialog(const QJsonObject& rulesObj,
                                   const QStringList& availableParamIds,
                                   const QMap<QString, QStringList>& paramOptions,
                                   QWidget* parent)
    : QDialog(parent),
      m_rules(rulesObj),
      m_tabWidget(nullptr),
      m_visibilityList(nullptr),
      m_optionList(nullptr),
      m_validationList(nullptr),
      m_paramIds(availableParamIds),
      m_paramOptions(paramOptions)
{
    setWindowTitle(u8"规则编辑");
    setMinimumSize(780, 540);
    setStyleSheet(StyleHelper::darkPanelStyle());

    // 防止 Tab 文本被压缩，超出时使用滚动按钮
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->tabBar()->setExpanding(false);
    m_tabWidget->setUsesScrollButtons(true);

    buildUi();
    loadFromRules();
}

void RuleEditorDialog::buildUi()
{
    m_tabWidget = new QTabWidget(this);

    // 可见性规则标签页
    QWidget* visPage = new QWidget;
    QVBoxLayout* visLayout = new QVBoxLayout(visPage);
    m_visibilityList = new QListWidget;
    visLayout->addWidget(new QLabel(u8"可见性规则（控制参数显示隐藏）"));
    visLayout->addWidget(m_visibilityList, 1);
    QHBoxLayout* visBtnLayout = new QHBoxLayout;
    QPushButton* addVisBtn = new QPushButton(u8"新增");
    QPushButton* editVisBtn = new QPushButton(u8"编辑");
    QPushButton* delVisBtn = new QPushButton(u8"删除");
    visBtnLayout->addWidget(addVisBtn);
    visBtnLayout->addWidget(editVisBtn);
    visBtnLayout->addWidget(delVisBtn);
    visBtnLayout->addStretch();
    visLayout->addLayout(visBtnLayout);
    connect(addVisBtn, &QPushButton::clicked, this, &RuleEditorDialog::onAddVisibilityRule);
    connect(editVisBtn, &QPushButton::clicked, this, &RuleEditorDialog::onEditVisibilityRule);
    connect(delVisBtn, &QPushButton::clicked, this, &RuleEditorDialog::onRemoveVisibilityRule);

    // 选项规则标签页
    QWidget* optPage = new QWidget;
    QVBoxLayout* optLayout = new QVBoxLayout(optPage);
    m_optionList = new QListWidget;
    optLayout->addWidget(new QLabel(u8"选项规则（根据控制项值切换枚举选项）"));
    optLayout->addWidget(m_optionList, 1);
    QHBoxLayout* optBtnLayout = new QHBoxLayout;
    QPushButton* addOptBtn = new QPushButton(u8"新增");
    QPushButton* editOptBtn = new QPushButton(u8"编辑");
    QPushButton* delOptBtn = new QPushButton(u8"删除");
    optBtnLayout->addWidget(addOptBtn);
    optBtnLayout->addWidget(editOptBtn);
    optBtnLayout->addWidget(delOptBtn);
    optBtnLayout->addStretch();
    optLayout->addLayout(optBtnLayout);
    connect(addOptBtn, &QPushButton::clicked, this, &RuleEditorDialog::onAddOptionRule);
    connect(editOptBtn, &QPushButton::clicked, this, &RuleEditorDialog::onEditOptionRule);
    connect(delOptBtn, &QPushButton::clicked, this, &RuleEditorDialog::onRemoveOptionRule);

    // 校验说明标签页
    QWidget* valPage = new QWidget;
    QVBoxLayout* valLayout = new QVBoxLayout(valPage);
    m_validationList = new QListWidget;
    valLayout->addWidget(new QLabel(u8"校验规则说明（仅说明文本，用于描述业务规则）"));
    valLayout->addWidget(m_validationList, 1);
    QHBoxLayout* valBtnLayout = new QHBoxLayout;
    QPushButton* addValBtn = new QPushButton(u8"新增");
    QPushButton* editValBtn = new QPushButton(u8"编辑");
    QPushButton* delValBtn = new QPushButton(u8"删除");
    valBtnLayout->addWidget(addValBtn);
    valBtnLayout->addWidget(editValBtn);
    valBtnLayout->addWidget(delValBtn);
    valBtnLayout->addStretch();
    valLayout->addLayout(valBtnLayout);
    connect(addValBtn, &QPushButton::clicked, this, &RuleEditorDialog::onAddValidationRule);
    connect(editValBtn, &QPushButton::clicked, this, &RuleEditorDialog::onEditValidationRule);
    connect(delValBtn, &QPushButton::clicked, this, &RuleEditorDialog::onRemoveValidationRule);

    m_tabWidget->addTab(visPage, u8"可见性");
    m_tabWidget->addTab(optPage, u8"选项");
    m_tabWidget->addTab(valPage, u8"校验说明");

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        // 提交前从列表重新收集为 JSON
        m_rules.insert("visibility_rules", collectVisibilityRules());
        m_rules.insert("option_rules", collectOptionRules());
        m_rules.insert("validation_rules", collectValidationRules());
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &RuleEditorDialog::reject);

    QVBoxLayout* outer = new QVBoxLayout(this);
    outer->addWidget(m_tabWidget, 1);
    outer->addWidget(buttons);
}

void RuleEditorDialog::loadFromRules()
{
    // visibility_rules
    m_visibilityList->clear();
    QJsonArray visArr = m_rules.value("visibility_rules").toArray();
    for (const auto& v : visArr) {
        QJsonObject obj = v.toObject();
        QListWidgetItem* item = new QListWidgetItem(summarizeVisibilityRule(obj), m_visibilityList);
        item->setData(Qt::UserRole, obj);
    }

    // option_rules
    m_optionList->clear();
    QJsonArray optArr = m_rules.value("option_rules").toArray();
    for (const auto& v : optArr) {
        QJsonObject obj = v.toObject();
        QListWidgetItem* item = new QListWidgetItem(summarizeOptionRule(obj), m_optionList);
        item->setData(Qt::UserRole, obj);
    }

    // validation_rules
    m_validationList->clear();
    QJsonArray valArr = m_rules.value("validation_rules").toArray();
    for (const auto& v : valArr) {
        QJsonObject obj = v.toObject();
        QListWidgetItem* item = new QListWidgetItem(summarizeValidationRule(obj), m_validationList);
        item->setData(Qt::UserRole, obj);
    }
}

QJsonArray RuleEditorDialog::collectVisibilityRules() const
{
    QJsonArray arr;
    for (int i = 0; i < m_visibilityList->count(); ++i) {
        QListWidgetItem* item = m_visibilityList->item(i);
        QJsonObject obj = item->data(Qt::UserRole).toJsonObject();
        if (!obj.isEmpty()) {
            arr.append(obj);
        }
    }
    return arr;
}

QJsonArray RuleEditorDialog::collectOptionRules() const
{
    QJsonArray arr;
    for (int i = 0; i < m_optionList->count(); ++i) {
        QListWidgetItem* item = m_optionList->item(i);
        QJsonObject obj = item->data(Qt::UserRole).toJsonObject();
        if (!obj.isEmpty()) {
            arr.append(obj);
        }
    }
    return arr;
}

QJsonArray RuleEditorDialog::collectValidationRules() const
{
    QJsonArray arr;
    for (int i = 0; i < m_validationList->count(); ++i) {
        QListWidgetItem* item = m_validationList->item(i);
        QJsonObject obj = item->data(Qt::UserRole).toJsonObject();
        if (!obj.isEmpty()) {
            arr.append(obj);
        }
    }
    return arr;
}

QString RuleEditorDialog::summarizeVisibilityRule(const QJsonObject& obj)
{
    const QString controller = obj.value("controller").toString();
    int caseCount = obj.value("cases").toArray().size();
    return QString(u8"控制参数: %1  （%2 个分支）").arg(controller).arg(caseCount);
}

QString RuleEditorDialog::summarizeOptionRule(const QJsonObject& obj)
{
    const QString controller = obj.value("controller").toString();
    const QString target = obj.value("target").toString();
    int valueCount = obj.value("options_by_value").toObject().size();
    return QString(u8"%1 → %2  （%3 种取值）").arg(controller, target).arg(valueCount);
}

QString RuleEditorDialog::summarizeValidationRule(const QJsonObject& obj)
{
    const QString id = obj.value("id").toString();
    const QString scope = obj.value("scope").toString();
    QString desc = obj.value("description").toString();
    if (desc.length() > 40) {
        desc = desc.left(40) + QStringLiteral("...");
    }
    return QString(u8"[%1] (%2) %3").arg(id, scope, desc);
}

QString RuleEditorDialog::summarizeVisibilityCase(const QJsonObject& obj)
{
    QString value = obj.value("value").toString();
    QStringList ids;
    QJsonArray arr = obj.value("show").toArray();
    for (const auto& v : arr) ids << v.toString();
    return QStringLiteral("%1 -> %2").arg(value, ids.join(","));
}

bool RuleEditorDialog::editVisibilityRuleObject(QJsonObject& obj, QWidget* parent)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(u8"编辑可见性规则");
    QVBoxLayout* layout = new QVBoxLayout(&dlg);

    // 控制参数下拉
    QComboBox* controllerCombo = new QComboBox;
    controllerCombo->addItems(m_paramIds);
    int idxCtrl = controllerCombo->findText(obj.value("controller").toString());
    if (idxCtrl >= 0) controllerCombo->setCurrentIndex(idxCtrl);
    layout->addWidget(new QLabel(u8"控制参数ID："));
    layout->addWidget(controllerCombo);

    // 分支列表
    QListWidget* casesList = new QListWidget;
    layout->addWidget(new QLabel(u8"分支（值 -> 显示参数列表）"));
    layout->addWidget(casesList, 1);
    auto addCaseItem = [this, casesList](const QJsonObject& caseObj) {
        QListWidgetItem* item = new QListWidgetItem(summarizeVisibilityCase(caseObj), casesList);
        item->setData(Qt::UserRole, caseObj);
    };
    auto controllerOptions = [this, controllerCombo]() {
        return m_paramOptions.value(controllerCombo->currentText());
    };
    QJsonArray casesArr = obj.value("cases").toArray();
    for (const auto& v : casesArr) {
        addCaseItem(v.toObject());
    }

    QPushButton* addCaseBtn = new QPushButton(u8"新增分支");
    QPushButton* editCaseBtn = new QPushButton(u8"编辑分支");
    QPushButton* delCaseBtn = new QPushButton(u8"删除分支");
    QHBoxLayout* caseBtns = new QHBoxLayout;
    caseBtns->addWidget(addCaseBtn);
    caseBtns->addWidget(editCaseBtn);
    caseBtns->addWidget(delCaseBtn);
    caseBtns->addStretch();
    layout->addLayout(caseBtns);

    auto editSingleCase = [this, controllerOptions](QJsonObject& caseObj, QWidget* parent) -> bool {
        QDialog cd(parent);
        cd.setWindowTitle(u8"编辑分支");
        QVBoxLayout* cl = new QVBoxLayout(&cd);
        QComboBox* valueCombo = new QComboBox;
        const QStringList optsForCtrl = controllerOptions();
        if (!optsForCtrl.isEmpty()) {
            valueCombo->addItems(optsForCtrl);
            valueCombo->setEditable(false);
        } else {
            valueCombo->setEditable(true);
        }
        valueCombo->setCurrentText(caseObj.value("value").toString());
        cl->addWidget(new QLabel(u8"控制值："));
        cl->addWidget(valueCombo);
        QListWidget* showList = new QListWidget;
        showList->setSelectionMode(QAbstractItemView::NoSelection);
        showList->setSortingEnabled(true);
        for (const QString& pid : m_paramIds) {
            QListWidgetItem* it = new QListWidgetItem(pid, showList);
            it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
            it->setCheckState(Qt::Unchecked);
        }
        QSet<QString> showSet;
        QJsonArray showArr = caseObj.value("show").toArray();
        for (const auto& v : showArr) showSet.insert(v.toString());
        for (int i = 0; i < showList->count(); ++i) {
            QListWidgetItem* it = showList->item(i);
            if (showSet.contains(it->text())) {
                it->setCheckState(Qt::Checked);
            }
        }
        cl->addWidget(new QLabel(u8"显示的参数（勾选）："));
        cl->addWidget(showList, 1);
        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        cl->addWidget(buttons);
        QObject::connect(buttons, &QDialogButtonBox::accepted, &cd, &QDialog::accept);
        QObject::connect(buttons, &QDialogButtonBox::rejected, &cd, &QDialog::reject);
        if (cd.exec() != QDialog::Accepted) return false;
        const QString val = valueCombo->currentText().trimmed();
        if (val.isEmpty()) {
            QMessageBox::warning(parent, u8"无效输入", u8"控制值不能为空。");
            return false;
        }
        QJsonArray newShow;
        for (int i = 0; i < showList->count(); ++i) {
            QListWidgetItem* it = showList->item(i);
            if (it->checkState() == Qt::Checked) {
                newShow.append(it->text());
            }
        }
        caseObj.insert("value", val);
        caseObj.insert("show", newShow);
        return true;
    };

    connect(addCaseBtn, &QPushButton::clicked, this, [casesList, addCaseItem, editSingleCase, this]() {
        QJsonObject obj;
        obj.insert("value", QString());
        obj.insert("show", QJsonArray());
        if (!editSingleCase(obj, casesList)) return;
        addCaseItem(obj);
    });
    connect(editCaseBtn, &QPushButton::clicked, this, [casesList, editSingleCase]( ) {
        QListWidgetItem* item = casesList->currentItem();
        if (!item) return;
        QJsonObject obj = item->data(Qt::UserRole).toJsonObject();
        if (!editSingleCase(obj, casesList)) return;
        item->setText(RuleEditorDialog::summarizeVisibilityCase(obj));
        item->setData(Qt::UserRole, obj);
    });
    connect(delCaseBtn, &QPushButton::clicked, this, [casesList]() {
        QListWidgetItem* item = casesList->currentItem();
        if (!item) return;
        delete item;
    });

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) {
        return false;
    }

    QString controller = controllerCombo->currentText().trimmed();
    if (controller.isEmpty()) {
        QMessageBox::warning(parent, u8"无效输入", u8"控制参数ID不能为空。");
        return false;
    }

    QJsonArray newCases;
    for (int i = 0; i < casesList->count(); ++i) {
        QListWidgetItem* item = casesList->item(i);
        QJsonObject c = item->data(Qt::UserRole).toJsonObject();
        if (!c.contains("value")) continue;
        newCases.append(c);
    }

    obj.insert("controller", controller);
    obj.insert("cases", newCases);
    return true;
}

bool RuleEditorDialog::editOptionRuleObject(QJsonObject& obj, QWidget* parent)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(u8"编辑选项规则");
    QVBoxLayout* layout = new QVBoxLayout(&dlg);

    QComboBox* controllerEdit = new QComboBox;
    controllerEdit->addItems(m_paramIds);
    int idxCtrl = controllerEdit->findText(obj.value("controller").toString());
    if (idxCtrl >= 0) controllerEdit->setCurrentIndex(idxCtrl);
    QComboBox* targetEdit = new QComboBox;
    targetEdit->addItems(m_paramIds);
    int idxTgt = targetEdit->findText(obj.value("target").toString());
    if (idxTgt >= 0) targetEdit->setCurrentIndex(idxTgt);

    layout->addWidget(new QLabel(u8"控制参数ID："));
    layout->addWidget(controllerEdit);
    layout->addWidget(new QLabel(u8"目标枚举参数ID："));
    layout->addWidget(targetEdit);

    QListWidget* mapList = new QListWidget;
    layout->addWidget(new QLabel(u8"取值→选项映射（选择目标枚举的可选项）"));
    layout->addWidget(mapList, 1);

    auto summarizeMap = [](const QString& key, const QStringList& opts) {
        QString joined = opts.join(",");
        if (joined.size() > 60) joined = joined.left(60) + QStringLiteral("...");
        return QStringLiteral("%1: %2").arg(key, joined);
    };

    auto targetOptions = [this, targetEdit]() {
        return m_paramOptions.value(targetEdit->currentText());
    };
    auto controllerOptions = [this, controllerEdit]() {
        return m_paramOptions.value(controllerEdit->currentText());
    };

    auto editSingleMap = [targetOptions, controllerOptions, summarizeMap](QJsonObject& mapObj, QWidget* parent) -> bool {
        QDialog md(parent);
        md.setWindowTitle(u8"编辑映射");
        QVBoxLayout* ml = new QVBoxLayout(&md);
        QComboBox* keyCombo = new QComboBox;
        QStringList ctrlOpts = controllerOptions();
        if (!ctrlOpts.isEmpty()) {
            keyCombo->addItems(ctrlOpts);
            keyCombo->setEditable(false);
        } else {
            keyCombo->setEditable(true);
        }
        keyCombo->setCurrentText(mapObj.value("key").toString());
        ml->addWidget(new QLabel(u8"控制值："));
        ml->addWidget(keyCombo);

        QStringList optsList = targetOptions();
        QListWidget* optsWidget = new QListWidget;
        optsWidget->setSelectionMode(QAbstractItemView::NoSelection);
        optsWidget->setSortingEnabled(true);
        for (const QString& opt : optsList) {
            QListWidgetItem* it = new QListWidgetItem(opt, optsWidget);
            it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
            it->setCheckState(Qt::Unchecked);
        }
        // 预勾选
        QSet<QString> chosen;
        QJsonArray chosenArr = mapObj.value("values").toArray();
        for (const auto& v : chosenArr) chosen.insert(v.toString());
        for (int i = 0; i < optsWidget->count(); ++i) {
            QListWidgetItem* it = optsWidget->item(i);
            if (chosen.contains(it->text())) {
                it->setCheckState(Qt::Checked);
            }
        }

        ml->addWidget(new QLabel(u8"选择可用选项："));
        ml->addWidget(optsWidget, 1);

        // 当目标没有可选项时，提示用户必须手工输入（此时 optsWidget 为空）
        if (optsList.isEmpty()) {
            QLabel* warn = new QLabel(u8"当前目标参数没有预定义枚举选项，请先在参数模板中补充 options。");
            warn->setStyleSheet("color: #c62828;");
            ml->addWidget(warn);
        }

        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        ml->addWidget(buttons);
        QObject::connect(buttons, &QDialogButtonBox::accepted, &md, &QDialog::accept);
        QObject::connect(buttons, &QDialogButtonBox::rejected, &md, &QDialog::reject);

        if (md.exec() != QDialog::Accepted) return false;

        const QString key = keyCombo->currentText().trimmed();
        if (key.isEmpty()) {
            QMessageBox::warning(parent, u8"无效输入", u8"控制值不能为空。");
            return false;
        }

        QJsonArray values;
        for (int i = 0; i < optsWidget->count(); ++i) {
            QListWidgetItem* it = optsWidget->item(i);
            if (it->checkState() == Qt::Checked) {
                values.append(it->text());
            }
        }
        if (optsWidget->count() > 0 && values.isEmpty()) {
            QMessageBox::warning(parent, u8"无效输入", u8"请至少勾选一个选项，或先为目标参数配置 options。");
            return false;
        }

        mapObj.insert("key", key);
        mapObj.insert("values", values);
        return true;
    };

    // 预填 options_by_value
    QJsonObject mapObjAll = obj.value("options_by_value").toObject();
    for (auto it = mapObjAll.begin(); it != mapObjAll.end(); ++it) {
        QJsonObject entry;
        entry.insert("key", it.key());
        entry.insert("values", it.value().toArray());
        QStringList optList;
        QJsonArray arr = it.value().toArray();
        for (const auto& v : arr) optList << v.toString();
        QListWidgetItem* item = new QListWidgetItem(summarizeMap(it.key(), optList), mapList);
        item->setData(Qt::UserRole, entry);
    }

    QPushButton* addMapBtn = new QPushButton(u8"新增取值映射");
    QPushButton* editMapBtn = new QPushButton(u8"编辑选中映射");
    QPushButton* delMapBtn = new QPushButton(u8"删除选中映射");
    QHBoxLayout* mapBtnLayout = new QHBoxLayout;
    mapBtnLayout->addWidget(addMapBtn);
    mapBtnLayout->addWidget(editMapBtn);
    mapBtnLayout->addWidget(delMapBtn);
    mapBtnLayout->addStretch();
    layout->addLayout(mapBtnLayout);

    connect(addMapBtn, &QPushButton::clicked, this, [mapList, editSingleMap, summarizeMap]() {
        QJsonObject entry;
        if (!editSingleMap(entry, mapList)) return;
        QStringList opts;
        QJsonArray arr = entry.value("values").toArray();
        for (const auto& v : arr) opts << v.toString();
        QListWidgetItem* item = new QListWidgetItem(summarizeMap(entry.value("key").toString(), opts), mapList);
        item->setData(Qt::UserRole, entry);
    });
    connect(editMapBtn, &QPushButton::clicked, this, [mapList, editSingleMap, summarizeMap]() {
        QListWidgetItem* item = mapList->currentItem();
        if (!item) return;
        QJsonObject entry = item->data(Qt::UserRole).toJsonObject();
        if (!editSingleMap(entry, mapList)) return;
        QStringList opts;
        QJsonArray arr = entry.value("values").toArray();
        for (const auto& v : arr) opts << v.toString();
        item->setText(summarizeMap(entry.value("key").toString(), opts));
        item->setData(Qt::UserRole, entry);
    });
    connect(delMapBtn, &QPushButton::clicked, this, [mapList]() {
        QListWidgetItem* item = mapList->currentItem();
        if (!item) return;
        delete item;
    });

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    // 如果切换目标参数，清空映射（防止选项不匹配）
    auto prevTarget = QSharedPointer<QString>::create(targetEdit->currentText());
    QObject::connect(targetEdit, &QComboBox::currentTextChanged, &dlg, [mapList, prevTarget](const QString& newTarget) {
        if (newTarget == *prevTarget) return;
        mapList->clear();
        *prevTarget = newTarget;
    });

    if (dlg.exec() != QDialog::Accepted) {
        return false;
    }

    const QString controller = controllerEdit->currentText().trimmed();
    const QString target = targetEdit->currentText().trimmed();
    if (controller.isEmpty() || target.isEmpty()) {
        QMessageBox::warning(parent, u8"无效输入", u8"控制参数ID和目标参数ID不能为空。");
        return false;
    }

    QJsonObject newMap;
    for (int i = 0; i < mapList->count(); ++i) {
        QListWidgetItem* item = mapList->item(i);
        QJsonObject entry = item->data(Qt::UserRole).toJsonObject();
        QString key = entry.value("key").toString();
        QJsonArray values = entry.value("values").toArray();
        newMap.insert(key, values);
    }

    obj.insert("controller", controller);
    obj.insert("target", target);
    obj.insert("options_by_value", newMap);
    return true;
}

bool RuleEditorDialog::editValidationRuleObject(QJsonObject& obj, QWidget* parent)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(u8"编辑校验说明");
    QVBoxLayout* layout = new QVBoxLayout(&dlg);

    QLineEdit* idEdit = new QLineEdit;
    idEdit->setText(obj.value("id").toString());
    QComboBox* scopeCombo = new QComboBox;
    scopeCombo->addItem("per_state");
    scopeCombo->addItem("per_device");
    scopeCombo->addItem("global");
    const QString currentScope = obj.value("scope").toString();
    int idxScope = scopeCombo->findText(currentScope);
    if (idxScope >= 0) {
        scopeCombo->setCurrentIndex(idxScope);
    }
    QPlainTextEdit* descEdit = new QPlainTextEdit;
    descEdit->setPlainText(obj.value("description").toString());

    layout->addWidget(new QLabel(u8"规则ID："));
    layout->addWidget(idEdit);
    layout->addWidget(new QLabel(u8"作用范围："));
    layout->addWidget(scopeCombo);
    layout->addWidget(new QLabel(u8"规则说明："));
    layout->addWidget(descEdit);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) {
        return false;
    }

    const QString id = idEdit->text().trimmed();
    if (id.isEmpty()) {
        QMessageBox::warning(parent, u8"无效输入", u8"规则ID不能为空。");
        return false;
    }

    obj.insert("id", id);
    obj.insert("scope", scopeCombo->currentText());
    obj.insert("description", descEdit->toPlainText());
    return true;
}

void RuleEditorDialog::onAddVisibilityRule()
{
    QJsonObject obj;
    if (!editVisibilityRuleObject(obj, this)) {
        return;
    }
    QListWidgetItem* item = new QListWidgetItem(summarizeVisibilityRule(obj), m_visibilityList);
    item->setData(Qt::UserRole, obj);
}

void RuleEditorDialog::onEditVisibilityRule()
{
    QListWidgetItem* item = m_visibilityList->currentItem();
    if (!item) return;
    QJsonObject obj = item->data(Qt::UserRole).toJsonObject();
    if (!editVisibilityRuleObject(obj, this)) {
        return;
    }
    item->setText(summarizeVisibilityRule(obj));
    item->setData(Qt::UserRole, obj);
}

void RuleEditorDialog::onRemoveVisibilityRule()
{
    QListWidgetItem* item = m_visibilityList->currentItem();
    if (!item) return;
    delete item;
}

void RuleEditorDialog::onAddOptionRule()
{
    QJsonObject obj;
    if (!editOptionRuleObject(obj, this)) {
        return;
    }
    QListWidgetItem* item = new QListWidgetItem(summarizeOptionRule(obj), m_optionList);
    item->setData(Qt::UserRole, obj);
}

void RuleEditorDialog::onEditOptionRule()
{
    QListWidgetItem* item = m_optionList->currentItem();
    if (!item) return;
    QJsonObject obj = item->data(Qt::UserRole).toJsonObject();
    if (!editOptionRuleObject(obj, this)) {
        return;
    }
    item->setText(summarizeOptionRule(obj));
    item->setData(Qt::UserRole, obj);
}

void RuleEditorDialog::onRemoveOptionRule()
{
    QListWidgetItem* item = m_optionList->currentItem();
    if (!item) return;
    delete item;
}

void RuleEditorDialog::onAddValidationRule()
{
    QJsonObject obj;
    if (!editValidationRuleObject(obj, this)) {
        return;
    }
    QListWidgetItem* item = new QListWidgetItem(summarizeValidationRule(obj), m_validationList);
    item->setData(Qt::UserRole, obj);
}

void RuleEditorDialog::onEditValidationRule()
{
    QListWidgetItem* item = m_validationList->currentItem();
    if (!item) return;
    QJsonObject obj = item->data(Qt::UserRole).toJsonObject();
    if (!editValidationRuleObject(obj, this)) {
        return;
    }
    item->setText(summarizeValidationRule(obj));
    item->setData(Qt::UserRole, obj);
}

void RuleEditorDialog::onRemoveValidationRule()
{
    QListWidgetItem* item = m_validationList->currentItem();
    if (!item) return;
    delete item;
}
