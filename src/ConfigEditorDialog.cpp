#include "ConfigEditorDialog.h"
#include "ParameterEditDialog.h"
#include "TypeEditDialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFileInfo>

ConfigEditorDialog::ConfigEditorDialog(const QJsonObject& rootObj, const QString& filePath, QWidget* parent)
    : QDialog(parent), m_rootObj(rootObj), m_filePath(filePath)
{
    setWindowTitle("Edit Mode - Structure Editor");
    setMinimumSize(900, 600);

    // 拉取 equipment_types
    QJsonObject ecObj = m_rootObj.value("equipment_config").toObject();
    m_types = ecObj.value("equipment_types").toArray();

    m_typeList = new QListWidget;
    m_typeList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_typeList, &QListWidget::currentRowChanged, this, &ConfigEditorDialog::onTypeSelectionChanged);

    QPushButton* addTypeBtn = new QPushButton("Add Type");
    QPushButton* editTypeBtn = new QPushButton("Edit Type");
    QPushButton* removeTypeBtn = new QPushButton("Remove Type");
    connect(addTypeBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onAddType);
    connect(editTypeBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onEditType);
    connect(removeTypeBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onRemoveType);

    QVBoxLayout* typeLayout = new QVBoxLayout;
    typeLayout->addWidget(new QLabel("Equipment Types"));
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
    dcLayout->addWidget(new QLabel("Device Count"));
    dcLayout->addWidget(m_deviceCountSpin);
    dcLayout->addStretch();

    m_basicList = new QListWidget;
    m_basicList->setSelectionMode(QAbstractItemView::SingleSelection);
    QPushButton* addBasicBtn = new QPushButton("Add Basic Param");
    QPushButton* editBasicBtn = new QPushButton("Edit Basic Param");
    QPushButton* removeBasicBtn = new QPushButton("Remove Basic Param");
    connect(addBasicBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onAddBasic);
    connect(editBasicBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onEditBasic);
    connect(removeBasicBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onRemoveBasic);

    QVBoxLayout* basicLayout = new QVBoxLayout;
    basicLayout->addWidget(new QLabel("Basic Parameter Template"));
    basicLayout->addWidget(m_basicList, 1);
    basicLayout->addWidget(addBasicBtn);
    basicLayout->addWidget(editBasicBtn);
    basicLayout->addWidget(removeBasicBtn);

    m_workList = new QListWidget;
    m_workList->setSelectionMode(QAbstractItemView::SingleSelection);
    QPushButton* addWorkBtn = new QPushButton("Add State Param");
    QPushButton* editWorkBtn = new QPushButton("Edit State Param");
    QPushButton* removeWorkBtn = new QPushButton("Remove State Param");
    connect(addWorkBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onAddWorkParam);
    connect(editWorkBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onEditWorkParam);
    connect(removeWorkBtn, &QPushButton::clicked, this, &ConfigEditorDialog::onRemoveWorkParam);

    QVBoxLayout* workLayout = new QVBoxLayout;
    workLayout->addWidget(new QLabel("Work State Template Parameters"));
    workLayout->addWidget(m_workList, 1);
    workLayout->addWidget(addWorkBtn);
    workLayout->addWidget(editWorkBtn);
    workLayout->addWidget(removeWorkBtn);

    QHBoxLayout* rightLayout = new QHBoxLayout;
    rightLayout->addLayout(basicLayout, 1);
    rightLayout->addLayout(workLayout, 1);

    QVBoxLayout* rightPanel = new QVBoxLayout;
    rightPanel->addLayout(dcLayout);
    rightPanel->addLayout(rightLayout, 1);

    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->addLayout(typeLayout, 1);
    mainLayout->addLayout(rightPanel, 2);

    m_saveButton = new QPushButton("Save & Reload");
    connect(m_saveButton, &QPushButton::clicked, this, &ConfigEditorDialog::onSave);
    QDialogButtonBox* bottomButtons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(bottomButtons, &QDialogButtonBox::rejected, this, &ConfigEditorDialog::reject);

    QVBoxLayout* outer = new QVBoxLayout;
    outer->addLayout(mainLayout, 1);
    outer->addWidget(m_saveButton);
    outer->addWidget(bottomButtons);
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
    if (QMessageBox::question(this, "Remove Type", "Delete this equipment type and its definitions?") != QMessageBox::Yes) {
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
                QMessageBox::warning(this, "Duplicate ID", "This ID already exists in basic parameters");
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
                QMessageBox::warning(this, "Duplicate ID", "This ID already exists in basic parameters");
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
    if (QMessageBox::question(this, "Remove Parameter", "Delete this basic parameter?") != QMessageBox::Yes) {
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
                QMessageBox::warning(this, "Duplicate ID", "This ID already exists in work state parameters");
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
                QMessageBox::warning(this, "Duplicate ID", "This ID already exists in work state parameters");
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
    if (QMessageBox::question(this, "Remove Parameter", "Delete this work-state parameter?") != QMessageBox::Yes) {
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
    QString backup = fi.path() + "/" + fi.completeBaseName() + ".backup.json";
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
            QMessageBox::warning(this, "Save failed", "Cannot open file for writing");
            return;
        }
        QJsonDocument doc(m_rootObj);
        f.write(doc.toJson());
        f.close();
    }
}

void ConfigEditorDialog::onSave()
{
    if (!m_changed) {
        accept();
        return;
    }
    backupFile();
    writeFile();
    m_changed = false;
    accept();
}
