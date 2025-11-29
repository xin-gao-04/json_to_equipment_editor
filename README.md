# 装备雷抗通抗参数设置控件

## 开发环境

qt 5.5.1 c++2015 cmake 

#### qtdir

C:\Qt\Qt5.9.1\5.9.1\msvc2015_64\lib\cmake\Qt5

## 项目概览（快速了解）
- 目的：加载/编辑/保存装备参数配置（JSON），支持多设备、多工作状态。
- 入口：`src/main.cpp` 创建 `EquipmentConfigWidget`，默认加载 `config/equipment_config.json`。
- 数据模型：`EquipmentType`（类型/模板）、`DeviceInstance`（实例值）、`WorkStateTemplate`（状态模板）、`ParameterItem`（单个参数+校验）。
- UI 结构：`EquipmentConfigWidget`（设备类型 Tab）→ `DeviceTabWidget`（基本参数 + 工作状态 Tabs）→ `WorkStateTabWidget`（具体参数表单）。
- 校验：保存前执行全量校验；数值按 min/max；字符串受正则限制（数字/空白/逗号/方括号/点/负号，便于数组输入）。
- 保存：`autoSave` 写回当前文件；设备/工作状态 Tab 可单独导出。序列化时保留参数定义与当前值。

## 当前实现速览
- 入口：`src/main.cpp` 创建 `EquipmentConfigWidget`，默认加载 `config/equipment_config.json`。
- 数据模型：`EquipmentType`（类型 + 基本参数模板 + 工作状态模板）、`DeviceInstance`（实例值 + 工作状态值列表）、`WorkStateTemplate`（工作状态参数模板）、`ParameterItem`（单个参数的编辑/校验/默认值）。
- UI 结构：顶层 `EquipmentConfigWidget` 以设备类型为 Tab；每个设备使用 `DeviceTabWidget`，包含“基本参数”与按 `work_state_count` 动态生成的 `WorkStateTabWidget`；定时器每秒把编辑器里的值写回 `DeviceInstance`。
- 显隐逻辑：`DeviceInstance::isEnabled` 通过 `radar_enabled/comm_enabled/enabled/参数出现` 等字段判断，禁用时隐藏工作状态 Tab 及相关字段。
- 保存：`EquipmentConfigWidget::autoSave` 写回当前文件；设备/工作状态 Tab 自带“保存”按钮可单独导出 JSON，序列化时值以字符串形式写出。

## 参数编辑技巧与使用手法
- 参数 → 控件：`int/Byte` 用 `QSpinBox`，`double` 用 `QDoubleSpinBox`（保留 2 位小数），`enum` 用 `QComboBox`，`string` 用 `QLineEdit`，集中于 `ParameterItem::createEditor`。
- 独立实例：设备/状态的参数会从模板复制出自己的 `ParameterItem`，默认值在创建时写入 `DeviceInstance`，避免不同状态之间串值。
- 状态数量驱动：`work_state_count` 变化会触发重新生成状态 Tab 并补齐缺失的状态值，定时器和手动赋值都会生效。
- 可见性联动：启用字段为 0/空时隐藏工作状态 Tab、计数字段和保存按钮；启用后恢复显示。
- 文件字段：保存时同时写出参数定义（标签、单位、默认值等），便于对比与回填。

## 使用提示
- 初次运行自动读取 `config/equipment_config.json`；菜单“文件”可重新打开或保存到其它位置。
- 修改后状态栏会提示“配置已修改”，按 `Ctrl+S` 或菜单“保存”即可写回当前文件；没有当前文件时会提示另存为。
- 设备或工作状态 Tab 底部的保存按钮可单独导出对应片段 JSON，便于局部调试。

## 已知问题 / 待解决
- 序列化时所有值写成字符串且 `double` 仅保留 2 位小数，若需要保持精度或严格类型需调整。
- 工作状态/基本参数的自动轮询更新使用定时器，重绘/性能可进一步优化为信号驱动。
- 字符串正则限制目前仅允许数字/空白/逗号/方括号/点/负号，如需录入字母或更复杂格式需放宽。

## 规划：新增“编辑模式”（可修改设备/字段定义，并直接回写 JSON）

目标：在现有“配置编辑”（仅改值）基础上，提供一个可切换的“编辑模式”，允许修改/新增/删除设备、状态模板和参数定义（类型、默认值、范围/选项等），并直接更新 `config/equipment_config.json` 的结构，且不破坏现有加载/保存路径。

设计要点（阶段化实施）：
1) 模式切换与安全：在主窗口提供“编辑模式”开关，切换时弹窗提示并备份当前 JSON；默认只读显示，不影响现有使用路径。
2) 设备层面操作：在编辑模式下，为设备类型和设备实例提供“新增/删除/重命名”入口；修改后同步更新内存模型与 JSON。
3) 参数模板编辑：在 Basic/WorkState 模板区域提供“编辑字段”对话框，可修改 `id/label/type/default/range/options/unit`，并支持新增/删除字段；对话框中做即时校验（唯一性、必填、范围格式、枚举非空）。
4) 工作状态模板与规则：编辑模式下可编辑 `visibility_rules` 与 `option_rules`，支持增删规则、增删 case、重新绑定 controller/target。
5) 数据一致性与校验：统一校验函数，校验模板结构（重复 ID、非法类型、缺失 range/options），校验实例数据是否满足新模板；不合法时阻止保存并提示。
6) JSON 回写：完成编辑后提供“保存结构”动作，将模板和实例一并回写到 `config/equipment_config.json`（保留原有非 equipment_config 字段）；失败回滚到备份。
7) 兼容性与可回滚：保留现有读取/显示逻辑不变；编辑模式下的改动仅在用户确认保存结构后才落盘；提供“恢复备份”快捷入口。

实施节奏（建议）：
- Milestone 1：模式切换框架 + 备份/恢复 + 只读展示模板结构（树形/列表）。
- Milestone 2：参数模板编辑对话框（basic/work_state），含即时校验与预览。
- Milestone 3：设备类型/实例的增删改、工作状态模板字段编辑；支持 `visibility_rules`/`option_rules` 的基础编辑。
- Milestone 4：全量校验、JSON 回写、实例数据自动迁移（缺字段填默认、删字段丢弃）。
- Milestone 5：UX 打磨（批量操作、撤销/重做、差异高亮）。

UX 草案：
- 主窗口工具栏：新增“编辑模式”切换按钮；开启时高亮并提示“结构编辑中（已备份）”。
- 编辑面板：侧边栏树形结构（设备类型 → 模板字段 → 状态模板规则），右侧属性表+操作按钮（新增/删除/保存）。
- 字段编辑对话框：表单包含 id/label/type/default/min/max/options/unit；根据类型动态显隐 range/options；实时校验并提示。
- 操作安全：删除前二次确认；保存结构时汇总潜在破坏性变更（如字段删除将丢弃实例值）。

备份策略：
- 进入编辑模式或保存结构前，自动拷贝 `config/equipment_config.json` 到同目录 `equipment_config.backup.json`（可配环境变量关闭）。

## 需求梳理：

需要通过tabwidget + scroll组件为主。 tab页面内有fromLayout组织label和editor.以及确认按钮。 

功能需求主要是通过json文件组织数据内容和布局内容。每个待编辑的元素包括一个label项和一个value项。允许动态扩展。 

架构上可以分为：装备 -> 任意可扩展的装备组和装备组配置的单独的参数，例如 雷达参数出现  如果是0 则不出现，后面的基本参数组和工作状态组都不存在。  并包括 工作状态个数  可以标明工作状态组的个数。 

---

## 需求分析与优化方案

### 核心功能需求分析

1. **界面结构**：TabWidget + ScrollArea 的组合界面
2. **数据驱动**：通过JSON文件配置界面布局和数据结构
3. **动态扩展**：支持参数组的动态显示/隐藏和数量控制
4. **表单编辑**：Label + Editor 的参数编辑界面

### 架构特点
- **层次化结构**：装备 → 装备组 → 参数项
- **条件显示**：某些参数为0时，相关参数组不显示
- **数量控制**：工作状态个数决定工作状态组的显示数量

### 优化的JSON数据结构设计

```json
{
  "equipment_config": {
    "equipment_types": [
      {
        "type_id": "radar",
        "type_name": "雷达设备",
        "device_count": 2,
        "basic_parameters": [
          {
            "id": "radar_enabled",
            "label": "雷达参数出现",
            "type": "int",
            "default": 1,
            "range": [0, 1]
          },
          {
            "id": "work_state_count",
            "label": "工作状态个数",
            "type": "int",
            "default": 3,
            "range": [1, 10]
          }
        ],
        "work_state_template": {
          "name": "天线工作状态",
          "parameters": [
            {
              "id": "antenna_frequency",
              "label": "天线频率",
              "type": "double",
              "unit": "MHz",
              "default": 2400.0,
              "range": [2000.0, 3000.0]
            },
            {
              "id": "antenna_power",
              "label": "发射功率",
              "type": "double",
              "unit": "W",
              "default": 100.0,
              "range": [0.0, 1000.0]
            },
            {
              "id": "beam_width",
              "label": "波束宽度",
              "type": "double",
              "unit": "度",
              "default": 30.0,
              "range": [1.0, 180.0]
            },
            {
              "id": "polarization",
              "label": "极化方式",
              "type": "enum",
              "options": ["水平", "垂直", "圆极化"],
              "default": "水平"
            }
          ]
        }
      },
      {
        "type_id": "communication_type1",
        "type_name": "通信设备类型1",
        "device_count": 3,
        "basic_parameters": [
          {
            "id": "comm_enabled",
            "label": "通信参数出现",
            "type": "int",
            "default": 1,
            "range": [0, 1]
          },
          {
            "id": "work_state_count",
            "label": "工作状态个数",
            "type": "int",
            "default": 2,
            "range": [1, 5]
          }
        ],
        "work_state_template": {
          "name": "通信工作状态",
          "parameters": [
            {
              "id": "comm_frequency",
              "label": "通信频率",
              "type": "double",
              "unit": "MHz",
              "default": 900.0
            },
            {
              "id": "bandwidth",
              "label": "带宽",
              "type": "double",
              "unit": "MHz",
              "default": 20.0
            },
            {
              "id": "protocol",
              "label": "通信协议",
              "type": "enum",
              "options": ["TCP", "UDP", "自定义"],
              "default": "TCP"
            }
          ]
        }
      }
    ]
  }
}
```

### 核心类设计架构

```cpp
// 参数项基类
class ParameterItem {
public:
    QString id;
    QString label;
    QString type;
    QVariant defaultValue;
    QString unit;
    QVariantMap constraints; // 约束条件（范围、格式等）
    
    virtual QWidget* createEditor(QWidget* parent) = 0;
    virtual QVariant getValue() = 0;
    virtual void setValue(const QVariant& value) = 0;
    virtual bool validate(const QVariant& value) = 0;
};

// 工作状态模板类
class WorkStateTemplate {
public:
    QString name;
    QList<ParameterItem*> parameters;
    
    QWidget* createStateWidget(int stateIndex, QWidget* parent);
    QVariantMap getStateValues(int stateIndex);
    void setStateValues(int stateIndex, const QVariantMap& values);
};

// 设备类型类
class EquipmentType {
public:
    QString typeId;
    QString typeName;
    int deviceCount;
    QList<ParameterItem*> basicParameters;
    WorkStateTemplate* workStateTemplate;
    
    // 条件逻辑
    bool isEnabled(int deviceIndex, const QVariantMap& basicValues);
    int getWorkStateCount(int deviceIndex, const QVariantMap& basicValues);
    
    QWidget* createDeviceWidget(int deviceIndex, QWidget* parent);
};

// 设备实例类
class DeviceInstance {
public:
    QString deviceId;
    QString deviceName;
    EquipmentType* equipmentType;
    QVariantMap basicValues;
    QList<QVariantMap> workStateValues; // 每个工作状态的参数值
    
    bool isEnabled();
    int getWorkStateCount();
    void updateWorkStateCount(int newCount);
};

// 主配置控件 - 重新设计为多层Tab结构
class EquipmentConfigWidget : public QTabWidget {
Q_OBJECT
public:
    explicit EquipmentConfigWidget(QWidget* parent = nullptr);
    
    bool loadFromJson(const QString& jsonFile);
    bool saveToJson(const QString& jsonFile);
    void updateAllVisibility();
    bool validateAll();
    
signals:
    void configChanged();
    void validationError(const QString& message);
    
private slots:
    void onBasicParameterChanged(const QString& typeId, int deviceIndex, 
                                const QString& parameterId, const QVariant& value);
    void onWorkStateParameterChanged(const QString& typeId, int deviceIndex, 
                                   int stateIndex, const QString& parameterId, 
                                   const QVariant& value);
    void onWorkStateCountChanged(const QString& typeId, int deviceIndex, int newCount);
    
private:
    QList<EquipmentType*> equipmentTypes;
    QMap<QString, QList<DeviceInstance*>> deviceInstances; // typeId -> devices
    
    void createEquipmentTypeTabs();
    void createDeviceTabs(const QString& typeId, QTabWidget* parentTab);
    void createWorkStateTabs(DeviceInstance* device, QTabWidget* parentTab);
    void updateDeviceVisibility(const QString& typeId, int deviceIndex);
    void updateWorkStateTabs(DeviceInstance* device, QTabWidget* workStateTab);
};

// 设备Tab页面类
class DeviceTabWidget : public QTabWidget {
Q_OBJECT
public:
    explicit DeviceTabWidget(DeviceInstance* device, QWidget* parent = nullptr);
    
private slots:
    void onBasicParameterChanged();
    void onWorkStateCountChanged();
    
private:
    DeviceInstance* m_device;
    QWidget* m_basicParamsWidget;
    QTabWidget* m_workStateTabWidget;
    
    void createBasicParametersTab();
    void createWorkStateTabs();
    void updateWorkStateTabs(int newCount);
};

// 工作状态Tab页面类
class WorkStateTabWidget : public QScrollArea {
Q_OBJECT
public:
    explicit WorkStateTabWidget(DeviceInstance* device, int stateIndex, 
                               QWidget* parent = nullptr);
    
signals:
    void parameterChanged(const QString& parameterId, const QVariant& value);
    
private:
    DeviceInstance* m_device;
    int m_stateIndex;
    QMap<QString, QWidget*> m_parameterWidgets;
    
    void createParameterWidgets();
};
```

### 技术架构设计

```
UI层 (多层Tab结构)
├─ 主TabWidget (装备类型切换)
├─ 设备TabWidget (设备实例切换)
├─ 状态TabWidget (工作状态切换)
└─ ScrollArea + FormLayout (参数编辑)

逻辑层 (Business Logic)
├─ EquipmentTypeManager (装备类型管理)
├─ DeviceInstanceManager (设备实例管理)  
├─ WorkStateManager (工作状态管理)
├─ ValidationEngine (参数验证引擎)
└─ ConfigSynchronizer (配置同步器)

数据层 (Data Layer)
├─ JsonConfigLoader (JSON配置加载器)
├─ EquipmentTypeModel (装备类型数据模型)
├─ DeviceInstanceModel (设备实例数据模型)
└─ WorkStateModel (工作状态数据模型)
```

### 界面层次结构设计

```
主窗口 (EquipmentConfigWidget)
├─ Tab: 雷达设备
│  ├─ Tab: 雷达设备1 (DeviceTabWidget)
│  │  ├─ Tab: 基本参数
│  │  ├─ Tab: 工作状态1 (WorkStateTabWidget)
│  │  ├─ Tab: 工作状态2 (WorkStateTabWidget)
│  │  └─ Tab: 工作状态3 (WorkStateTabWidget)
│  └─ Tab: 雷达设备2 (DeviceTabWidget)
│     ├─ Tab: 基本参数
│     ├─ Tab: 工作状态1
│     └─ Tab: 工作状态2
├─ Tab: 通信设备类型1
│  ├─ Tab: 通信设备1-1
│  │  ├─ Tab: 基本参数
│  │  ├─ Tab: 工作状态1
│  │  └─ Tab: 工作状态2
│  ├─ Tab: 通信设备1-2
│  └─ Tab: 通信设备1-3
└─ Tab: 通信设备类型2
   └─ ...
```

### 针对多设备多状态的实现特性

#### 1. 动态Tab管理
- **自动Tab创建**：根据设备数量和工作状态数量自动创建Tab页面
- **Tab标题更新**：实时更新Tab标题显示设备状态和参数概要
- **智能Tab切换**：记住用户最后操作的Tab位置

#### 2. 模板化参数管理
- **参数模板复用**：工作状态使用相同的参数模板，减少配置复杂度
- **批量参数设置**：支持将一个工作状态的参数复制到其他状态
- **参数组继承**：新增工作状态自动继承模板的默认值

#### 3. 设备状态联动
- **条件显示控制**：基本参数中的"启用"开关控制后续Tab的显示
- **状态数量控制**：工作状态个数参数动态控制工作状态Tab的数量
- **实时状态同步**：参数修改时实时更新相关联的界面元素

#### 4. 大数据量优化
- **懒加载Tab**：只有当Tab被激活时才创建其内容widget
- **参数缓存机制**：缓存参数值，避免重复计算和验证
- **智能刷新策略**：只刷新受影响的Tab页面，而不是全部重绘

### 用户体验优化建议

#### 1. 导航和定位
- **面包屑导航**：显示当前位置 "雷达设备 > 雷达设备1 > 工作状态2"
- **快速跳转菜单**：右键菜单快速跳转到其他设备或状态
- **搜索定位功能**：快速搜索并定位到特定参数

#### 2. 参数编辑增强
- **参数值预览**：鼠标悬停显示参数的详细信息和有效范围
- **批量编辑模式**：选择多个工作状态进行批量参数修改
- **参数差异高亮**：高亮显示与默认值或其他状态不同的参数

#### 3. 状态管理
- **配置状态指示**：用颜色或图标表示设备的配置完整性
- **未保存提醒**：Tab标题显示 "*" 表示有未保存的修改
- **配置验证状态**：实时显示参数验证结果

#### 4. 操作便利性
- **右键菜单功能**：
  - 复制当前工作状态参数
  - 粘贴参数到其他工作状态
  - 重置为默认值
  - 删除工作状态（如果允许）
- **键盘快捷键支持**：
  - Ctrl+S: 保存配置
  - Ctrl+Z: 撤销操作
  - Ctrl+Tab: 切换Tab页面

### 更新的开发计划

#### 阶段一：多层Tab框架 (2-3天)
1. 实现EquipmentConfigWidget主框架
2. 创建EquipmentType和DeviceInstance数据模型
3. 实现基础的多层Tab创建逻辑

#### 阶段二：参数模板系统 (2-3天)  
1. 实现WorkStateTemplate参数模板类
2. 完成参数编辑器的创建和绑定
3. 实现基本的参数验证功能

#### 阶段三：动态管理逻辑 (3-4天)
1. 实现工作状态数量的动态控制
2. 添加条件显示逻辑（启用/禁用控制）
3. 完善Tab的动态创建和销毁

#### 阶段四：用户体验优化 (2-3天)
1. 添加参数批量操作功能
2. 实现配置状态的可视化反馈
3. 完善导航和快速定位功能

#### 阶段五：性能优化和完善 (2-3天)
1. 实现懒加载和缓存机制
2. 优化大数据量下的界面响应
3. 完善错误处理和用户提示

### 特殊考虑点

1. **内存管理**：多设备多状态会创建大量widget，需要合理的内存管理策略
2. **性能优化**：当设备数量和状态数量很大时，需要优化界面创建和更新速度
3. **数据一致性**：确保多个Tab之间的数据同步和一致性
4. **配置复杂度**：提供配置模板和向导，降低用户配置的复杂度

这个设计现在应该能够很好地满足你的需求：支持多种装备类型、每种类型多台设备、每台设备多个工作状态，并且通过多层Tab结构来组织大量的配置参数。

## 结构编辑器使用（GUI 入口与保存）
- 入口：主界面空白处右键选择“结构编辑模式”，或菜单“文件/结构编辑模式”。
- 类型/参数操作：在设备类型列表、基本参数模板列表、工作状态模板参数列表上右键，可执行“新增/编辑/删除”。
- 状态页签配置：工作状态区域下方的“状态页签配置”可设置状态数量覆盖（-1 表示不覆盖，保留 `work_state_count`）和状态标签列表（一行一个；留空使用默认“工作状态1/2/... ”）。
- 保存结构：右下角按钮“保存结构并重载”会备份当前 JSON（生成同名 `.backup.json`），写回 `config/equipment_config.json` 并自动重载界面，改动立即生效。
