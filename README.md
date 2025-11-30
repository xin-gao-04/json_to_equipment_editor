# 装备参数配置工具（Qt）

## 环境
- Qt 5.5+（本地使用 Qt5 Widgets）
- C++17 / CMake
- 入口：`src/main.cpp`，默认加载 `config/equipment_config.json`

## 功能概览
- JSON 驱动：`equipment_config.json` 定义设备类型、基本参数、工作状态模板及规则。
- 三层 UI：`EquipmentConfigWidget`（设备类型 Tab）→ `DeviceTabWidget`（基本参数 + 工作状态 Tabs）→ `WorkStateTabWidget`（状态参数表单）。
- 数据模型：`EquipmentType`（模板） + `DeviceInstance`（实例值） + `WorkStateTemplate`（状态参数模板） + `ParameterItem`（单个参数的编辑与校验）。
- 校验与保存：编辑过程中定时将值写回实例，保存时执行全量校验并写回当前 JSON；设备/状态 Tab 可单独导出。

## 规则支持
- 可见性规则（`visibility_rules`）：控制参数值 → 显示哪些参数。
- 选项规则（`option_rules`）：控制参数值 → 目标枚举参数的可选项集合。
- 校验说明（`validation_rules`）：记录业务校验描述（目前作为说明文本，后端 `validateAll` 仍执行数值校验）。
- 超短波（`uhf`）示例：工作方式枚举（定频/跳频/扩频）驱动信号类型选项与可见参数；校验说明涵盖起止频率一致/跳频个数与范围等规则。

## 结构编辑器（右键主界面空白处或菜单“结构编辑模式”）
- 类型/参数编辑：列表支持新增/编辑/删除设备类型、基本参数、工作状态参数，自动校验 ID 冲突。
- 状态页签配置：可设置工作状态数量覆盖和自定义标签列表。
- 规则编辑：
  - 文本方式：直接编辑规则 JSON，并点击“应用规则更改”写回。
  - 图形化方式：点击“图形化编辑...”弹出对话框，分三页：
    - 可见性：控制参数 ID 下拉选择；每个分支编辑控制值 + 勾选显示的参数（从当前模板参数列表选择，避免手输错误）。
    - 选项：控制参数/目标参数均为下拉；取值→选项列表以文本录入（每行 `值: 选项1,选项2`）。
    - 校验说明：编辑规则 ID、作用范围（per_state/per_device/global）、规则描述。
  - 规则保存后会同步到 `work_state_template` 下的 `visibility_rules/option_rules/validation_rules`。
- 保存结构：点击“保存结构并重载”，会备份原 JSON 并写回，主界面自动重载。

## 运行
```bash
mkdir -p build && cd build
cmake ..
cmake --build . -j4
./bin/EquipmentConfig           # 默认读取 config/equipment_config.json
```
构建时会自动将 `config/equipment_config.json` 拷贝到 `build/bin/equipment_config.json`。

## 主要代码入口
- `src/main.cpp`：启动窗口、菜单/右键入口（结构编辑器），应用全局样式。
- `src/EquipmentConfigWidget.cpp`：JSON 加载/保存、Tab 创建、全量校验。
- `src/DeviceTabWidget.cpp`：基本参数页、工作状态 Tab 动态生成/可见性更新。
- `src/WorkStateTabWidget.cpp`：状态参数表单、可见性和枚举选项联动。
- `src/ConfigEditorDialog.cpp`：结构编辑器，类型/参数/规则编辑，规则文本区与图形化入口。
- `src/RuleEditorDialog.cpp`：图形化规则编辑（可见性/选项/校验说明），控制/目标参数用下拉选择，映射项用勾选列表防止手输错误。
- `src/ParameterItem.*`：参数编辑器生成与基础校验。

## 已知日志提示
- `Populating font family aliases ... "Segoe UI"`：macOS 缺少 Segoe UI，字体别名映射的正常提示；可将全局字体改为系统已有字体以消除。
- `TSM AdjustCapsLockLED...`：macOS 输入法框架日志，无功能影响。

## 已知限制/改进方向
- 校验：已开始支持数据驱动的 `validation_rules`（per_state），当前实现了等于/小于/最小值/跳频终止频率计算/频率列表区间校验。更多复杂校验可按需扩展。
- 控制值输入：当控制参数是枚举时使用下拉；非枚举仍可手填，若需进一步约束可为控制参数补充 options 或扩展枚举值域。
- 序列化仍以字符串写值，`double` 精度如需更高需再扩展。

## 版本记录
- 2025-11-30：规则编辑器新增图形化界面；控制/目标参数下拉选择，映射参数勾选列表（避免手输 ID/选项错误）；修正 Qt 过时 API 警告。
- 2025-11-30（续）：图形化规则编辑进一步下拉/勾选化：可见性/选项规则的控制值若对应枚举，使用下拉选择；选项映射针对目标枚举的 options 勾选选择，减少手动输入。
- 2025-11-30（续2）：校验引擎接入 `validation_rules`（等于/小于/最小值/跳频终止频率/频率数组区间）；UHF 校验规则改为数据驱动。
- 2025-11-30（续3）：界面深色现代化皮肤沉淀到 `StyleHelper`，供结构/规则编辑器复用，样式与渐变按钮提升操作直觉。
- 2025-11-30（续4）：提升可读性：分组标题颜色加亮，Tab 宽度/间距增大，减少文字被压缩省略。
- 2025-11-30（续5）：Tab 显示固定宽度且溢出使用滚动按钮；右键菜单改为浅色文字、深色背景，分组/标签颜色进一步提亮以增强可见度。
