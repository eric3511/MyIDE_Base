Qt Advanced Docking System 本质上是一个面向专业桌面软件的 **IDE 风格 Dock 框架**。
你可以把它当作 `QDockWidget` 的高阶替代品，但在大型项目里最好再封装一层 `DockService`，统一管理 DockWidget 的创建、显示、隐藏、保存、恢复和业务绑定。
最佳学习方式是：**先跑 demo，提炼初始化顺序，再封装 DockService，最后根据产品体验调 flags、auto-hide 和 stylesheet。**

## 1. 项目定位与整体架构

Qt Advanced Docking System 是一个基于 QWidget 的高级停靠布局框架，目标是实现类似 **Visual Studio、Qt Creator、JetBrains IDE、工业软件工作台** 里的可拖拽、可浮动、可标签化、可保存布局的 Dock 系统。

它相比 Qt 自带的 `QDockWidget` 更灵活，主要优势是：

- 支持复杂嵌套停靠区域；
- 支持 dock widget 标签页；
- 支持浮动窗口；
- 支持布局保存与恢复；
- 支持 perspective，多套布局方案；
- 支持 auto-hide 自动隐藏侧边栏；
- 支持 central widget 中央区域；
- 可通过 stylesheet 深度定制外观；
- 行为更接近现代 IDE。

项目地址：


`https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System`

官方文档：


`https://githubuser0xffff.github.io/Qt-Advanced-Docking-System/doc/user-guide.html`

---

## 2. 核心概念速览

先建立 ADS 的对象模型。理解这几个类，后面就很顺了。

### 2.1 核心类关系

|**类名**|**作用**|**类比 Qt 自带组件**|
|---|---|---|
|`ads::CDockManager`|整个 Dock 系统的管理器|类似多个 `QDockWidget` 的总控中心|
|`ads::CDockWidget`|一个可停靠面板|类似增强版 `QDockWidget`|
|`ads::CDockAreaWidget`|一组 DockWidget 的标签页区域|类似 tab 容器|
|`ads::CFloatingDockContainer`|浮动窗口容器|类似独立顶层窗口|
|`ads::CDockComponentsFactory`|自定义 Dock 组件创建逻辑|类似 UI 工厂|
|`ads::CDockManager::setConfigFlag`|全局行为配置|类似全局策略开关|

ADS 的结构可以粗略理解为：

```text
QMainWindow / QWidget
    |
    └── CDockManager
            |
            ├── CDockWidget 1
            ├── CDockWidget 2
            ├── CDockAreaWidget
            │       ├── CDockWidget tab A
            │       └── CDockWidget tab B
            |
            └── Floating Dock Container
```


### 2.2 和 QDockWidget 的关键区别

Qt 自带的 `QDockWidget` 适合简单主窗口，例如：

- 左侧属性栏；
- 右侧日志栏；
- 下方输出栏。

但如果你要做类似 IDE 的体验，比如：

- 任意拖动组合；
- 面板作为标签页堆叠；
- 用户保存自定义布局；
- 工具窗口自动隐藏；
- 多套布局方案切换；
- 面板拖到浮动窗口之后还可以继续拆分；

那么 ADS 明显更适合。

---

## 3. 编译与集成方式

ADS 支持 qmake、CMake，也支持 Qt5 / Qt6。你的项目如果是现代 CMake，推荐用 CMake 集成；如果是老项目，也可以用 `.pri` 文件快速接入。

---

### 3.1 获取源码


`git clone https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System.git`



建议你重点看：


`demo/ examples/ src/ doc/user-guide.html`

---



## 4. 最小可运行示例

下面从一个最小主窗口开始。

### 4.1 MainWindow.h

```cpp
#pragma once

#include <QMainWindow>

namespace ads {
class CDockManager;
class CDockWidget;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    void initDocking();
    ads::CDockWidget* createTextDockWidget(const QString& title, const QString& text);

private:
    ads::CDockManager* m_dockManager = nullptr;
};
```


---

### 4.2 MainWindow.cpp

```cpp
#include "MainWindow.h"

#include <QTextEdit>
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>

#include "DockManager.h"
#include "DockWidget.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    resize(1200, 800);

    initDocking();

    statusBar()->showMessage("ADS docking system ready.");
}

MainWindow::~MainWindow() = default;

void MainWindow::initDocking()
{
    using namespace ads;

    /*
     * 注意：
     * 配置 flags 必须在创建 CDockManager 之前设置。
     * 官方文档明确说明：如果 manager 创建之后再设置，可能导致行为异常甚至崩溃。
     */
    CDockManager::setConfigFlags(CDockManager::DefaultOpaqueConfig);

    CDockManager::setConfigFlag(CDockManager::OpaqueSplitterResize, true);
    CDockManager::setConfigFlag(CDockManager::XmlCompressionEnabled, false);
    CDockManager::setConfigFlag(CDockManager::FocusHighlighting, true);

    m_dockManager = new CDockManager(this);

    auto* projectDock = createTextDockWidget("Project", "Project tree here");
    auto* editorDock  = createTextDockWidget("Editor", "Main editor here");
    auto* logDock     = createTextDockWidget("Log", "Log output here");
    auto* propDock    = createTextDockWidget("Properties", "Properties here");

    m_dockManager->addDockWidget(ads::LeftDockWidgetArea, projectDock);
    m_dockManager->addDockWidget(ads::CenterDockWidgetArea, editorDock);
    m_dockManager->addDockWidget(ads::BottomDockWidgetArea, logDock);
    m_dockManager->addDockWidget(ads::RightDockWidgetArea, propDock);
}

ads::CDockWidget* MainWindow::createTextDockWidget(const QString& title, const QString& text)
{
    auto* dockWidget = new ads::CDockWidget(title);

    auto* editor = new QTextEdit;
    editor->setPlainText(text);

    dockWidget->setWidget(editor);

    return dockWidget;
}

```

---

### 4.3 main.cpp

```cpp
#include <QApplication>
#include "MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    MainWindow w;
    w.show();

    return app.exec();
}
```


这个示例会创建：

- 左侧 Project；
- 中央 Editor；
- 下方 Log；
- 右侧 Properties。

你可以直接拖动、浮动、标签化这些面板。

---

## 5. 配置 Flags：必须先掌握的部分

ADS 的配置 flags 是非常重要的一层。官方文档里反复强调：**必须在创建 `CDockManager` 之前设置配置项。**

错误写法：

```cpp
m_dockManager = new ads::CDockManager(this);

ads::CDockManager::setConfigFlag(
    ads::CDockManager::OpaqueSplitterResize,
    true
);
```


正确写法：

```cpp
ads::CDockManager::setConfigFlags(ads::CDockManager::DefaultOpaqueConfig);

ads::CDockManager::setConfigFlag(
    ads::CDockManager::OpaqueSplitterResize,
    true
);

m_dockManager = new ads::CDockManager(this);
```


---

### 5.1 常用全局配置表

|**配置项**|**作用**|**建议**|
|---|---|---|
|`DefaultOpaqueConfig`|使用实时拖动和实时 resize|普通桌面软件推荐|
|`DefaultNonOpaqueConfig`|拖动时只显示预览，不实时调整|重型 UI 或性能差时使用|
|`OpaqueSplitterResize`|splitter 拖动时实时调整|复杂 OpenGL / 图像控件可关闭|
|`XmlCompressionEnabled`|保存布局 XML 是否压缩|调试时关闭，发布时开启|
|`XmlAutoFormattingEnabled`|XML 是否格式化|调试时开启|
|`FocusHighlighting`|焦点高亮|IDE 类软件推荐开启|
|`AlwaysShowTabs`|单个 DockWidget 时也显示 tab|多面板 UI 推荐开启|
|`AllTabsHaveCloseButton`|所有 tab 显示关闭按钮|看产品风格|
|`RetainTabSizeWhenCloseButtonHidden`|隐藏关闭按钮时保持 tab 宽度|避免 tab 宽度跳动|
|`TabsAtBottom`|tab 显示到底部|特定 UI 风格使用|
|`DockAreaHasCloseButton`|dock area 显示关闭按钮|一般开启|
|`DockAreaHasUndockButton`|dock area 显示浮动按钮|一般开启|
|`DockAreaHasTabsMenuButton`|显示 tab 菜单按钮|dock 很多时推荐|

一个比较适合 IDE / 工业软件的配置：

```cpp
ads::CDockManager::setConfigFlags(ads::CDockManager::DefaultOpaqueConfig);

ads::CDockManager::setConfigFlag(ads::CDockManager::OpaqueSplitterResize, true);
ads::CDockManager::setConfigFlag(ads::CDockManager::XmlCompressionEnabled, false);
ads::CDockManager::setConfigFlag(ads::CDockManager::XmlAutoFormattingEnabled, true);
ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
ads::CDockManager::setConfigFlag(ads::CDockManager::AlwaysShowTabs, true);
ads::CDockManager::setConfigFlag(ads::CDockManager::RetainTabSizeWhenCloseButtonHidden, true);
ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasTabsMenuButton, true);
```


发布版可以改为：

```cpp
ads::CDockManager::setConfigFlag(ads::CDockManager::XmlCompressionEnabled, true);
ads::CDockManager::setConfigFlag(ads::CDockManager::XmlAutoFormattingEnabled, false);
```


---

## 6. DockWidget 的创建与管理

### 6.1 基本创建方式

```cpp
auto* dockWidget = new ads::CDockWidget("Console");

auto* textEdit = new QTextEdit;
textEdit->setReadOnly(true);

dockWidget->setWidget(textEdit);

m_dockManager->addDockWidget(ads::BottomDockWidgetArea, dockWidget);
```


`CDockWidget` 本身只是容器，真正的业务 UI 是你传进去的 QWidget。

---

### 6.2 添加到指定区域

常见区域枚举：

```cpp
ads::LeftDockWidgetArea
ads::RightDockWidgetArea
ads::TopDockWidgetArea
ads::BottomDockWidgetArea
ads::CenterDockWidgetArea
```


示例：

```cpp
auto* leftDock = new ads::CDockWidget("Navigator");
leftDock->setWidget(new QTextEdit);

m_dockManager->addDockWidget(ads::LeftDockWidgetArea, leftDock);
```


---

### 6.3 添加到已有 DockArea 形成标签页

```cpp
auto* firstDock = new ads::CDockWidget("Output");
firstDock->setWidget(new QTextEdit);

auto* dockArea = m_dockManager->addDockWidget(
    ads::BottomDockWidgetArea,
    firstDock
);

auto* secondDock = new ads::CDockWidget("Problems");
secondDock->setWidget(new QTextEdit);

m_dockManager->addDockWidget(
    ads::CenterDockWidgetArea,
    secondDock,
    dockArea
);
```

//?
这样 `Output` 和 `Problems` 会在同一个区域形成 tab。

---

### 6.4 设置 DockWidget 特性

`CDockWidget` 支持一些特性控制，例如关闭、浮动、删除行为等。常见用法：

```cpp
dockWidget->setFeature(
    ads::CDockWidget::DockWidgetClosable,
    true
);

dockWidget->setFeature(
    ads::CDockWidget::DockWidgetMovable,
    true
);

dockWidget->setFeature(
    ads::CDockWidget::DockWidgetFloatable,
    true
);
```


如果某个面板不希望用户关闭，例如主编辑区：

```cpp
dockWidget->setFeature(
    ads::CDockWidget::DockWidgetClosable,
    false
);
```


如果某个面板不允许浮动：

```cpp
dockWidget->setFeature(
    ads::CDockWidget::DockWidgetFloatable,
    false
);
```


---

## 7. Central Widget 中央区域设计

ADS 从较新版本开始支持 central widget 概念。这对 IDE 类软件很重要。

例如：

- 中央是编辑器；
- 左侧是项目树；
- 右侧是属性；
- 下方是日志；
- 中央编辑器不能被关闭；
- 其他窗口围绕中央区域 dock。

### 7.1 创建中央 DockWidget

```cpp
auto* editorDock = new ads::CDockWidget("Editor");

auto* editor = new QTextEdit;
editor->setPlainText("Central editor");

editorDock->setWidget(editor);

editorDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);
editorDock->setFeature(ads::CDockWidget::DockWidgetFloatable, false);

auto* centralArea = m_dockManager->setCentralWidget(editorDock);
```

这类设计适合：

- CAD / CAM 软件；
- 图像处理软件；
- 数据分析软件；
- IDE；
- 机器人调试平台；
- 工业监控平台。

---

### 7.2 推荐架构

```text
MainWindow
    ├── MenuBar
    ├── ToolBar
    ├── StatusBar
    └── CDockManager
            ├── Central: Editor / Canvas / Viewer
            ├── Left: Project / Resource / Device Tree
            ├── Right: Property / Inspector
            ├── Bottom: Console / Log / Problems
            └── AutoHide: Search / Assistant / Notifications
```


如果你做的是复杂工程软件，中央区域建议放：

- `QOpenGLWidget`
- 自研 Canvas
- 文档编辑器
- 图像查看器
- 多文档 tab 容器
- QGraphicsView
- 3D viewport

---

## 8. 布局保存与恢复

这是 ADS 非常实用的功能。

### 8.1 保存当前布局

```cpp
QByteArray state = m_dockManager->saveState();
```


写入文件：

```cpp
QFile file("layout.dat");
if (file.open(QIODevice::WriteOnly)) {
    file.write(m_dockManager->saveState());
}
```


---

### 8.2 恢复布局

```cpp
QFile file("layout.dat");
if (file.open(QIODevice::ReadOnly)) {
    QByteArray state = file.readAll();
    m_dockManager->restoreState(state);
}
```


注意：恢复布局之前，相关的 DockWidget 最好已经创建好。

典型流程：

```cpp
void MainWindow::init()
{
    createDockManager();
    createAllDockWidgets();
    restoreLayout();
}
```

不要这样：

```cpp
restoreLayout();
createAllDockWidgets();
```


因为布局状态里记录的是 dock widget 的名字和结构，如果对象不存在，恢复效果会不完整。

---

### 8.3 用 QSettings 保存布局

```cpp
void MainWindow::saveLayout()
{
    QSettings settings("YourCompany", "YourApp");

    settings.setValue(
        "MainWindow/DockingState",
        m_dockManager->saveState()
    );
}

void MainWindow::restoreLayout()
{
    QSettings settings("YourCompany", "YourApp");

    QByteArray state = settings.value(
        "MainWindow/DockingState"
    ).toByteArray();

    if (!state.isEmpty()) {
        m_dockManager->restoreState(state);
    }
}
```


在关闭窗口时保存：

```cpp
void MainWindow::closeEvent(QCloseEvent* event)
{
    saveLayout();
    QMainWindow::closeEvent(event);
}
```


---

### 8.4 XML 可读性配置

调试阶段建议：

```cpp
ads::CDockManager::setConfigFlag(
    ads::CDockManager::XmlCompressionEnabled,
    false
);

ads::CDockManager::setConfigFlag(
    ads::CDockManager::XmlAutoFormattingEnabled,
    true
);
```

这样保存出来的数据更方便观察。

发布阶段建议：

```cpp
ads::CDockManager::setConfigFlag(
    ads::CDockManager::XmlCompressionEnabled,
    true
);

ads::CDockManager::setConfigFlag(
    ads::CDockManager::XmlAutoFormattingEnabled,
    false
);
```

---

## 9. Perspective：多套工作区布局

Perspective 可以理解成“命名布局”。

典型场景：

- Debug 布局；
- Edit 布局；
- Review 布局；
- Monitor 布局；
- Lite 布局；
- Expert 布局。

### 9.1 保存 perspective

```cpp
m_dockManager->addPerspective("Default");
```

或者保存当前状态到某个名称：

```cpp
m_dockManager->addPerspective("Debug");
```


---

### 9.2 打开 perspective

```cpp
m_dockManager->openPerspective("Debug");

```


---

### 9.3 和菜单结合

```cpp
auto* perspectiveMenu = menuBar()->addMenu("Perspectives");

auto* defaultAction = perspectiveMenu->addAction("Default");
connect(defaultAction, &QAction::triggered, this, [this]() {
    m_dockManager->openPerspective("Default");
});

auto* debugAction = perspectiveMenu->addAction("Debug");
connect(debugAction, &QAction::triggered, this, [this]() {
    m_dockManager->openPerspective("Debug");
});
```


---

## 10. 动态创建和销毁 DockWidget

复杂软件里，经常需要动态打开多个面板，比如：

- 打开多个文档；
- 多个日志窗口；
- 多个图像查看器；
- 多个设备调试窗口；
- 每个连接设备一个面板。

---

### 10.1 动态创建文档 Dock

```cpp
ads::CDockWidget* MainWindow::createDocumentDock(const QString& filePath)
{
    auto* dock = new ads::CDockWidget(QFileInfo(filePath).fileName());

    auto* editor = new QTextEdit;
    editor->setPlainText(QString("File: %1").arg(filePath));

    dock->setWidget(editor);
    dock->setObjectName(filePath);

    dock->setFeature(ads::CDockWidget::DockWidgetClosable, true);
    dock->setFeature(ads::CDockWidget::DockWidgetMovable, true);
    dock->setFeature(ads::CDockWidget::DockWidgetFloatable, true);

    m_dockManager->addDockWidget(ads::CenterDockWidgetArea, dock);

    return dock;
}
```


---

### 10.2 关闭时删除内容

ADS 文档提到有 `DeleteContentOnClose` 这类配置，用来在关闭 dock widget 时动态删除内容。

适合场景：

- 文档窗口；
- 临时结果窗口；
- 查询结果窗口；
- 图像预览窗口。

伪代码结构：

```cpp
dockWidget->setFeature(
    ads::CDockWidget::DockWidgetDeleteOnClose,
    true
);
```


不同版本枚举名可能略有差异，以你当前源码 `DockWidget.h` 为准。

---

### 10.3 建议的 Dock 生命周期策略

|**类型**|**生命周期建议**|**原因**|
|---|---|---|
|Project / Properties / Console|启动时创建，隐藏/显示控制|常驻工具窗口|
|Document / Image Viewer|按需创建，关闭删除|数量不固定|
|Device Panel|设备连接时创建，断开后删除|跟业务对象绑定|
|Search Result|每次搜索复用或按需创建|看产品需求|
|Log Viewer|常驻，不建议删除|方便恢复布局|

---

## 11. 显示、隐藏与菜单联动

桌面软件通常需要在 `View` 菜单里控制每个 Dock 面板显示隐藏。

### 11.1 创建 Toggle Action

ADS 通常提供与 dock widget 关联的 toggle action。常见写法类似：

```cpp
QAction* action = dockWidget->toggleViewAction();
menuBar()->addMenu("View")->addAction(action);
```


完整示例：

```cpp
void MainWindow::createViewMenu()
{
    auto* viewMenu = menuBar()->addMenu("View");

    for (auto* dock : m_allDockWidgets) {
        viewMenu->addAction(dock->toggleViewAction());
    }
}
```


这样用户关闭 DockWidget 后，可以从菜单重新打开。

---

### 11.2 推荐封装

```cpp
ads::CDockWidget* MainWindow::registerDockWidget(
    const QString& title,
    QWidget* content,
    ads::DockWidgetArea area
)
{
    auto* dock = new ads::CDockWidget(title);
    dock->setWidget(content);

    m_dockManager->addDockWidget(area, dock);
    m_allDockWidgets.push_back(dock);

    if (m_viewMenu) {
        m_viewMenu->addAction(dock->toggleViewAction());
    }

    return dock;
}
```


使用：

```cpp
registerDockWidget(
    "Project",
    new QTreeView,
    ads::LeftDockWidgetArea
);

registerDockWidget(
    "Console",
    new QTextEdit,
    ads::BottomDockWidgetArea
);
```

---

## 12. Auto-Hide 自动隐藏功能

ADS 4.x 之后强化了 Auto-Hide。它类似 Visual Studio 的“自动隐藏工具窗口”。

适合：

- 属性面板；
- 项目导航；
- 搜索；
- 辅助工具；
- 信息面板；
- 不常驻但需要快速访问的窗口。

### 12.1 Auto-Hide 的交互特点

根据项目 README 和文档描述，它支持：

- 将 DockWidget 拖到窗口边缘变成自动隐藏 tab；
- 将 floating widget 拖到边缘；
- auto-hide tab 可以排序；
- auto-hide tab 可以拖到其他边；
- auto-hide tab 可以重新 dock 或 float；
- auto-hide tab 有右键菜单；
- 可以从上下左右四个 sidebar 弹出。

---

### 12.2 启用和使用

不同版本 API 名称可能会有细微变化。通常用法是通过 DockManager 或 DockWidget 设置 auto-hide。

可以在源码中重点搜索这些关键词：

```text
AutoHide
SideBar
AutoHideDockContainer
CDockSideBar
```

大致调用风格可能类似：

```cpp
m_dockManager->addAutoHideDockWidget(
    ads::SideBarLocation::SideBarLeft,
    dockWidget
);
```


或者：

```cpp
dockWidget->setAutoHide(true);
```


具体以你 checkout 的版本头文件为准。

### 12.3 实战建议

Auto-Hide 很适合“不频繁操作，但用户需要快速呼出”的面板。

例如：

```text
左侧 Auto-Hide:
    - Project
    - Files
    - Search

右侧 Auto-Hide:
    - Properties
    - Inspector
    - Help

底部 Auto-Hide:
    - Console
    - Problems
    - Build Output
```


但不要把所有窗口都 auto-hide。否则用户会变成“找窗口小游戏”的高分玩家。

---

## 13. 案例与 Demo 阅读路线

项目里的 `demo` 和 `examples` 是最值得读的部分。

建议你按这个顺序看。

---

### 13.1 第一阶段：看最小示例

重点找：

`examples/`

关注：

- 如何创建 `CDockManager`；
- 如何创建 `CDockWidget`；
- 如何 `setWidget()`；
- 如何 `addDockWidget()`；
- 如何保存和恢复布局。

你要提取的模式是：

```text
配置全局 flags
    ↓
创建 DockManager
    ↓
创建 DockWidget
    ↓
塞入业务 QWidget
    ↓
添加到 DockManager
    ↓
保存/恢复状态
```


---

### 13.2 第二阶段：看 demo 主程序

重点目录：

`demo/`

你应该关注这些内容：

|**关注点**|**目的**|
|---|---|
|MainWindow 如何初始化 DockManager|学习完整架构|
|各种 DockWidget 如何创建|学习批量构建面板|
|菜单如何控制 Dock 显示隐藏|学习产品级 UI|
|Perspective 如何实现|学习多布局|
|Auto-Hide 如何操作|学习高级功能|
|样式表如何加载|学习外观定制|
|图片查看器 demo|学习复杂 QWidget 嵌入|

---

### 13.3 第三阶段：看 src 源码

重点文件通常包括：

```text
src/DockManager.h
src/DockManager.cpp
src/DockWidget.h
src/DockWidget.cpp
src/DockAreaWidget.h
src/DockAreaWidget.cpp
src/DockContainerWidget.h
src/DockContainerWidget.cpp
src/FloatingDockContainer.h
src/FloatingDockContainer.cpp

```

阅读顺序建议：

```text
DockWidget
    ↓
DockAreaWidget
    ↓
DockContainerWidget
    ↓
DockManager
    ↓
FloatingDockContainer
    ↓
AutoHide 相关类
```


原因是：

- `CDockWidget` 是业务入口；
- `CDockAreaWidget` 负责 tab 区域；
- `CDockContainerWidget` 负责布局容器；
- `CDockManager` 负责总控；
- `FloatingDockContainer` 处理浮动；
- AutoHide 是高级功能，最后看。

---

## 14. 样式定制

ADS 支持通过 Qt stylesheet 改外观。Demo 里提供了类似 Visual Studio 的 CSS theme。

### 14.1 加载样式

```cpp
QFile file(":/styles/dark.css");
if (file.open(QIODevice::ReadOnly)) {
    qApp->setStyleSheet(QString::fromUtf8(file.readAll()));
}
```

---

### 14.2 常见可定制对象

可以搜索 demo 样式表里的 selector，例如：

```text
ads--CDockWidgetTab
ads--CDockAreaTitleBar
ads--CDockAreaWidget
ads--CDockContainerWidget
ads--CFloatingDockContainer
```


具体名称以项目当前版本为准。

---

### 14.3 工业软件推荐风格

如果你做工具软件，我建议：

- dock tab 高度保持紧凑；
- active tab 和 inactive tab 区分明显；
- 关闭按钮不要太抢眼；
- title bar hover 状态清楚；
- floating window 边界清晰；
- dark theme 下注意 splitter 可见性；
- focus highlighting 不要过亮。

---

## 15. 信号与事件处理

你可能会需要监听 DockWidget 的状态变化，比如：

- 用户关闭了面板；
- 面板 visibility 改变；
- 当前 tab 改变；
- 面板浮动；
- perspective 切换。

建议在源码中查看 `DockWidget.h`、`DockManager.h` 暴露的 signals。

典型用法：

```cpp
connect(dockWidget, &ads::CDockWidget::viewToggled,
        this, [this](bool visible) {
            qDebug() << "Dock visible:" << visible;
        });
```


或者：

```cpp
connect(dockWidget->toggleViewAction(), &QAction::toggled,
        this, [](bool checked) {
            qDebug() << "View action toggled:" << checked;
        });
```


不同版本 signal 名称可能不同。你可以直接查头文件，因为 ADS 的 API 命名比较直观。

---

## 16. 在大型项目里的推荐封装

对 10 年经验开发者来说，直接到处 new `CDockWidget` 肯定很快会失控。建议你做一层 Dock 服务。

### 16.1 DockId 设计

```cpp
enum class DockId
{
    Project,
    Properties,
    Console,
    Problems,
    Search,
    Editor,
    DeviceList,
    LogViewer
};
```

---

### 16.2 DockRegistry

```cpp
struct DockDescriptor
{
    DockId id;
    QString title;
    ads::DockWidgetArea defaultArea;
    bool closable = true;
    bool floatable = true;
    bool movable = true;
};
```


---

### 16.3 DockService 示例

```cpp
class DockService : public QObject
{
    Q_OBJECT

public:
    explicit DockService(ads::CDockManager* manager, QObject* parent = nullptr)
        : QObject(parent)
        , m_manager(manager)
    {
    }

    ads::CDockWidget* createDock(
        DockId id,
        const QString& title,
        QWidget* content,
        ads::DockWidgetArea area
    )
    {
        auto* dock = new ads::CDockWidget(title);
        dock->setWidget(content);

        dock->setObjectName(QString::number(static_cast<int>(id)));

        m_manager->addDockWidget(area, dock);

        m_docks.insert(id, dock);

        return dock;
    }

    ads::CDockWidget* dock(DockId id) const
    {
        return m_docks.value(id, nullptr);
    }

    void showDock(DockId id)
    {
        if (auto* d = dock(id)) {
            d->toggleView(true);
        }
    }

    void hideDock(DockId id)
    {
        if (auto* d = dock(id)) {
            d->toggleView(false);
        }
    }

private:
    ads::CDockManager* m_manager = nullptr;
    QHash<DockId, ads::CDockWidget*> m_docks;
};
```


这样业务代码只关心：

```cpp
dockService->showDock(DockId::Console);
dockService->hideDock(DockId::Properties);
```


而不是到处操作 ADS 细节。

---

## 17. 典型产品级 MainWindow 初始化顺序

推荐你的主窗口初始化顺序如下：

```cpp
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    initApplicationStyle();
    initDockingConfig();

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    createDockManager();
    createDockWidgets();
    createDockMenus();

    restoreDockLayout();

    connectSignals();
}
```


### 17.1 关键点
1.`initDockingConfig() 必须在 createDockManager() 之前。`

2.`createDockWidgets() 必须在 restoreDockLayout() 之前。`

3.`createDockMenus() 可以在 dock 创建后执行。`

---

## 18. 推荐工程结构

如果你要把 ADS 用在真实项目里，建议目录类似：

```text
src/
├── app/
│   ├── MainWindow.h
│   ├── MainWindow.cpp
│   └── Application.cpp
│
├── docking/
│   ├── DockService.h
│   ├── DockService.cpp
│   ├── DockIds.h
│   ├── DockFactory.h
│   └── DockFactory.cpp
│
├── widgets/
│   ├── ProjectWidget.h
│   ├── PropertyWidget.h
│   ├── ConsoleWidget.h
│   ├── EditorWidget.h
│   └── LogViewerWidget.h
│
└── resources/
    ├── style.qss
    └── icons.qrc
```


`DockFactory` 负责创建内容 widget：

```cpp
QWidget* DockFactory::createWidget(DockId id)
{
    switch (id) {
    case DockId::Project:
        return new ProjectWidget;

    case DockId::Properties:
        return new PropertyWidget;

    case DockId::Console:
        return new ConsoleWidget;

    case DockId::Editor:
        return new EditorWidget;

    default:
        return new QWidget;
    }
}
```


`DockService` 负责注册到 ADS。

---

## 19. 常见坑与解决方案

### 19.1 配置 flags 设置太晚

错误：

```cpp
m_dockManager = new ads::CDockManager(this);
ads::CDockManager::setConfigFlag(...);
```


正确：

```cpp
ads::CDockManager::setConfigFlag(...);
m_dockManager = new ads::CDockManager(this);
```


这是官方文档特别强调的点。

---

### 19.2 restoreState 之前没有创建 DockWidget

布局恢复依赖 dock widget 的存在。

正确顺序：

```cpp
createAllDockWidgets();
restoreState();
```

---

### 19.3 DockWidget 没有稳定 objectName

对于复杂布局保存，建议给每个 DockWidget 设置稳定名称：

`dockWidget->setObjectName("Dock.Project");`

不要用会变化的标题当唯一标识，比如：

`dockWidget->setObjectName(QString("Untitled-%1").arg(rand()));`

否则恢复布局很容易混乱。

---

### 19.4 内容 Widget 生命周期不清楚

如果你的 dock 关闭后内容要删除，显式设置删除策略。

如果是常驻工具窗口，关闭只隐藏，不删除。

建议：

```text
工具面板：隐藏
文档面板：关闭删除
临时结果：关闭删除
日志面板：隐藏
```

---

### 19.5 重型 QWidget 拖动卡顿

例如 dock 里嵌入：

- OpenGL；
- 视频窗口；
- 大图像渲染；
- QWebEngineView；
- 复杂表格；
- 自绘 canvas。

可以考虑：

```cpp
ads::CDockManager::setConfigFlags(
    ads::CDockManager::DefaultNonOpaqueConfig
);
```

或者关闭实时 splitter resize：

```cpp
ads::CDockManager::setConfigFlag(
    ads::CDockManager::OpaqueSplitterResize,
    false
);
```


---

### 19.6 样式表影响内部按钮

Qt stylesheet 很容易“一招打全场”，比如全局写：


`QPushButton {     border: none; }`

可能会影响 ADS 内部 tab close button、title bar button。

建议限定范围：

`#MainToolBar QPushButton {     border: none; }`

或者针对 ADS selector 单独写。

---

## 20. 一个完整的教学版 MainWindow 示例

下面给你一个更接近真实项目的版本。

### 20.1 MainWindow.h

```cpp
#pragma once

#include <QMainWindow>
#include <QVector>

namespace ads {
class CDockManager;
class CDockWidget;
}

class QMenu;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void initDockingConfig();
    void createDockManager();
    void createMenus();
    void createDockWidgets();
    void createViewMenu();
    void saveDockLayout();
    void restoreDockLayout();

    ads::CDockWidget* createDock(
        const QString& objectName,
        const QString& title,
        QWidget* content
    );

private:
    ads::CDockManager* m_dockManager = nullptr;
    QMenu* m_viewMenu = nullptr;

    QVector<ads::CDockWidget*> m_docks;
};
```

---

### 20.2 MainWindow.cpp

```cpp
#include "MainWindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QFile>
#include <QMenu>
#include <QMenuBar>
#include <QSettings>
#include <QStatusBar>
#include <QTextEdit>
#include <QTreeView>
#include <QTableView>
#include <QLabel>

#include "DockManager.h"
#include "DockWidget.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    resize(1400, 900);

    initDockingConfig();
    createMenus();
    createDockManager();
    createDockWidgets();
    createViewMenu();
    restoreDockLayout();

    statusBar()->showMessage("Ready");
}

MainWindow::~MainWindow() = default;

void MainWindow::initDockingConfig()
{
    using namespace ads;

    CDockManager::setConfigFlags(CDockManager::DefaultOpaqueConfig);

    CDockManager::setConfigFlag(CDockManager::OpaqueSplitterResize, true);
    CDockManager::setConfigFlag(CDockManager::XmlCompressionEnabled, false);
    CDockManager::setConfigFlag(CDockManager::XmlAutoFormattingEnabled, true);
    CDockManager::setConfigFlag(CDockManager::FocusHighlighting, true);
    CDockManager::setConfigFlag(CDockManager::AlwaysShowTabs, true);
    CDockManager::setConfigFlag(CDockManager::RetainTabSizeWhenCloseButtonHidden, true);
    CDockManager::setConfigFlag(CDockManager::DockAreaHasTabsMenuButton, true);
}

void MainWindow::createMenus()
{
    auto* fileMenu = menuBar()->addMenu("File");

    auto* exitAction = fileMenu->addAction("Exit");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    m_viewMenu = menuBar()->addMenu("View");

    auto* layoutMenu = menuBar()->addMenu("Layout");

    auto* saveLayoutAction = layoutMenu->addAction("Save Layout");
    connect(saveLayoutAction, &QAction::triggered, this, &MainWindow::saveDockLayout);

    auto* restoreLayoutAction = layoutMenu->addAction("Restore Layout");
    connect(restoreLayoutAction, &QAction::triggered, this, &MainWindow::restoreDockLayout);

    auto* resetLayoutAction = layoutMenu->addAction("Reset Layout");
    connect(resetLayoutAction, &QAction::triggered, this, [this]() {
        // 真实项目中可恢复默认布局，或者删除 QSettings 后重启
        QSettings settings("YourCompany", "YourApp");
        settings.remove("Docking/State");
    });
}

void MainWindow::createDockManager()
{
    m_dockManager = new ads::CDockManager(this);
}

ads::CDockWidget* MainWindow::createDock(
    const QString& objectName,
    const QString& title,
    QWidget* content
)
{
    auto* dock = new ads::CDockWidget(title);
    dock->setObjectName(objectName);
    dock->setWidget(content);

    m_docks.push_back(dock);

    return dock;
}

void MainWindow::createDockWidgets()
{
    auto* projectDock = createDock(
        "Dock.Project",
        "Project",
        new QTreeView
    );

    auto* propertiesDock = createDock(
        "Dock.Properties",
        "Properties",
        new QTableView
    );

    auto* consoleDock = createDock(
        "Dock.Console",
        "Console",
        new QTextEdit
    );

    auto* problemsDock = createDock(
        "Dock.Problems",
        "Problems",
        new QTextEdit
    );

    auto* editorDock = createDock(
        "Dock.Editor",
        "Editor",
        new QTextEdit
    );

    editorDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);
    editorDock->setFeature(ads::CDockWidget::DockWidgetFloatable, false);

    auto* centralArea = m_dockManager->setCentralWidget(editorDock);

    m_dockManager->addDockWidget(ads::LeftDockWidgetArea, projectDock);
    m_dockManager->addDockWidget(ads::RightDockWidgetArea, propertiesDock);
    m_dockManager->addDockWidget(ads::BottomDockWidgetArea, consoleDock);

    m_dockManager->addDockWidget(
        ads::CenterDockWidgetArea,
        problemsDock,
        consoleDock->dockAreaWidget()
    );

    Q_UNUSED(centralArea);
}

void MainWindow::createViewMenu()
{
    for (auto* dock : m_docks) {
        m_viewMenu->addAction(dock->toggleViewAction());
    }
}

void MainWindow::saveDockLayout()
{
    QSettings settings("YourCompany", "YourApp");

    settings.setValue(
        "Docking/State",
        m_dockManager->saveState()
    );
}

void MainWindow::restoreDockLayout()
{
    QSettings settings("YourCompany", "YourApp");

    const QByteArray state = settings.value("Docking/State").toByteArray();

    if (!state.isEmpty()) {
        m_dockManager->restoreState(state);
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveDockLayout();
    QMainWindow::closeEvent(event);
}
```


---

