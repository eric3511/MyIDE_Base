## QWindowKit 的核心使用可以浓缩为四句话：

1.先初始化 Qt 属性

在创建任何 QWidget 前设置：

```cpp
QGuiApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

```
2.每个顶层窗口绑定一个 Agent

```cpp
auto agent = new QWK::WidgetWindowAgent(this);
agent->setup(this);
```
3.声明标题栏和系统按钮

```cpp
agent->setTitleBar(titleBar);
agent->setSystemButton(QWK::WindowAgentBase::Maximize, maxButton);
```

声明标题栏中的可交互控件

```cpp
agent->setHitTestVisible(searchEdit, true);
```
从原理上看，QWindowKit 做的是：

```text
Qt 自定义 UI
+
原生窗口消息 / hit-test
+
跨平台窗口行为适配
=
既好看又接近原生体验的自定义窗口

```
它特别适合以下项目：

- 想做现代化无边框 Qt 桌面应用；
- 需要 Windows 11 Snap Layout；
- 希望 QWidget / QML 都能统一窗口风格；
- 不想自己维护复杂 native event 逻辑；
- 需要跨 Windows、macOS、Linux 的一致窗口体验。

## 核心原理

### 1 什么是无边框窗口？
Qt 中常见的无边框做法是：
```cpp
setWindowFlags(Qt::FramelessWindowHint);
```
这会移除系统标题栏和边框。

但问题也随之而来：

|**能力**|**移除系统边框后的问题**|
|---|---|
|拖动窗口|系统标题栏没了，不能拖动|
|缩放窗口|系统边框没了，不能拖边缩放|
|最大化 / 还原|需要自己处理|
|系统菜单|可能丢失|
|Windows Snap|可能失效|
|DPI / 多屏|容易出现边界计算问题|
|macOS 红黄绿按钮|可能显示异常或不可控|

QWindowKit 要解决的就是这些“移除原生边框后的副作用”。

---

### 2 QWindowKit 的 Agent 模型

QWindowKit 中有一个关键概念：

`WidgetWindowAgent`

可以把它理解为：

> 一个绑定到顶层窗口的窗口行为代理器。

它负责：

- 初始化窗口的特殊属性；
- 接管或参与原生事件处理；
- 计算标题栏区域；
- 计算拖拽区域；
- 标记系统按钮；
- 标记可交互子控件；
- 处理不同平台差异。

每个窗口都需要自己的 agent，因为每个顶层窗口都有自己的 native window handle、geometry、状态、标题栏区域和按钮控件。

---

### 3 Hit-Test 命中测试机制

无边框窗口中，最核心的机制是 **命中测试**。

当鼠标移动、按下或悬停时，操作系统会问：

> 鼠标现在在哪个窗口区域？

可能的回答包括：

|**区域类型**|**含义**|
|---|---|
|标题栏|可以拖动窗口|
|左边框|可以水平缩放|
|右边框|可以水平缩放|
|上边框|可以垂直缩放|
|下边框|可以垂直缩放|
|左上角|可以斜向缩放|
|最大化按钮|系统最大化按钮|
|最小化按钮|系统最小化按钮|
|关闭按钮|系统关闭按钮|
|客户区|普通应用内容区域|

在 Windows 中，典型结果包括：

```text
HTCAPTION
HTLEFT
HTRIGHT
HTTOP
HTBOTTOM
HTTOPLEFT
HTTOPRIGHT
HTBOTTOMLEFT
HTBOTTOMRIGHT
HTMINBUTTON
HTMAXBUTTON
HTCLOSE
HTCLIENT

```

QWindowKit 的任务就是根据 Qt 控件位置和系统坐标，返回正确的区域类型。

---

### 4 为什么 Windows 11 Snap Layout 需要特殊支持？

Windows 11 的 Snap Layout 是这样的体验：

> 鼠标悬停在最大化按钮上，系统弹出布局选择面板。

但这个功能依赖系统知道：

> 当前鼠标悬停的区域是“最大化按钮”。

如果你自己画了一个 QPushButton，然后点击时执行：

`showMaximized();`

这个按钮对 Windows 来说只是普通客户区控件，不是原生最大化按钮。

因此：

```cpp
agent->setSystemButton(QWK::WindowAgentBase::Maximize, maxButton);
```


这一步非常重要。

QWindowKit 通过 native event / hit-test 机制，把这个自定义按钮区域映射成系统理解的最大化按钮区域，从而让 Windows 11 的 Snap Layout 可以正常工作。

---

### 5 为什么需要区分标题栏和可交互控件？

假设标题栏中有一个搜索框：

```text
[App Icon] [Title]         [Search Box]       [-] [□] [×]
```


如果整个标题栏都被认定为可拖拽区域，那么用户点击搜索框时，系统会以为用户想拖动窗口，而不是输入文字。

所以必须告诉 QWindowKit：

`agent->setHitTestVisible(searchEdit, true);`

这样该控件区域会被视为 Qt 客户区控件，鼠标事件会正常发送给 `QLineEdit`。

---

### 6 为什么 setup 要尽早调用？

因为窗口 frame 与 client area 的关系，在 Qt 内部和操作系统内部都会影响：

- `geometry()`
- `frameGeometry()`
- `contentsRect()`
- `minimumSize()`
- `maximumSize()`
- maximize 后工作区适配
- DPI 缩放后的边界

如果 QWindowKit 在窗口已经 show 之后才介入，可能出现：

```text
Qt 已经按普通窗口计算了一次布局
↓
QWindowKit 又修改了窗口 frame 行为
↓
布局和系统窗口边界不一致

```

所以官方强调：

`agent->setup(this);`

越早越好。


## 使用说明 QWidget

最常见的使用场景：让 QWidget 顶层窗口使用自定义标题栏，同时保留原生窗口的拖拽、缩放、最大化、最小化、关闭、Snap Layout 等体验。

---

### 1 初始化要求

在任何 QWidget 创建之前，需要设置：

```cpp
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    QApplication app(argc, argv);

    // ...
    return app.exec();
}
```

#### 为什么要设置 `Qt::AA_DontCreateNativeWidgetSiblings`？

这个属性用于避免 Qt 为同级子控件创建额外 native window，从而减少原生窗口层级和 Qt 控件层级之间的冲突。

在无边框窗口、自定义标题栏和 hit-test 处理场景中，如果一些子控件拥有独立 native handle，可能会导致：

- 鼠标命中测试异常；
- 拖拽区域不准确；
- 子控件遮挡标题栏拖拽；
- Windows 原生消息处理不一致；
- Snap Layout 触发失败。

所以 QWindowKit 要求尽早设置它，而且必须在任何 widget 创建之前。

---

### 2 为顶层窗口创建 `WidgetWindowAgent`

每个顶层窗口都需要自己的 agent。

```cpp
#include <QWKWidgets/widgetwindowagent.h>

MyWidget::MyWidget(QWidget *parent)
    : QWidget(parent)
{
    auto agent = new QWK::WidgetWindowAgent(this);
    agent->setup(this);
}
```

或者如果你不想修改窗口构造函数：

```cpp
auto w = new MyWidget();

auto agent = new QWK::WidgetWindowAgent(w);
agent->setup(w);

w->show();
```

#### 注意点

官方强调：

`agent->setup(this);`

应该尽可能早调用，尤其当窗口需要设置大小约束时，例如：

```cpp
setMinimumSize(800, 600);
setMaximumSize(1600, 1200);

```

原因是 QWindowKit 会修改部分 Qt 内部窗口数据，这会影响 Qt 对窗口大小、frame geometry、client area 的计算。如果调用太晚，可能会出现：

- 初始尺寸错误；
- 最小尺寸约束不准；
- 最大化后内容区域偏移；
- 标题栏高度计算异常；
- 边缘缩放区域不准确。

---

### 3 构造自定义标题栏

QWindowKit 不强制你使用它提供的某个标题栏 UI。你可以自己写一个 QWidget 作为标题栏。

比如：

```cpp
class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);

    QPushButton *iconButton() const;
    QPushButton *minButton() const;
    QPushButton *maxButton() const;
    QPushButton *closeButton() const;
    QMenuBar *menuBar() const;

private:
    QPushButton *m_iconButton = nullptr;
    QPushButton *m_minButton = nullptr;
    QPushButton *m_maxButton = nullptr;
    QPushButton *m_closeButton = nullptr;
    QMenuBar *m_menuBar = nullptr;
};
```

然后将它放入主窗口布局顶部：

```cpp
auto titleBar = new TitleBar(this);

auto layout = new QVBoxLayout(this);
layout->setContentsMargins(0, 0, 0, 0);
layout->setSpacing(0);

layout->addWidget(titleBar);
layout->addWidget(mainContentWidget);
```

再告诉 QWindowKit：

`agent->setTitleBar(titleBar);`

这一步非常关键。它告诉 QWindowKit：

> 这个 QWidget 区域属于标题栏，空白部分可以用于拖动窗口。

---

### 4 设置系统按钮角色

接下来，需要告诉 QWindowKit 哪些控件是窗口图标、最小化、最大化、关闭按钮：

```cpp
agent->setSystemButton(QWK::WindowAgentBase::WindowIcon, titleBar->iconButton());
agent->setSystemButton(QWK::WindowAgentBase::Minimize, titleBar->minButton());
agent->setSystemButton(QWK::WindowAgentBase::Maximize, titleBar->maxButton());
agent->setSystemButton(QWK::WindowAgentBase::Close, titleBar->closeButton());
```

#### 这一步的作用

它不是自动给按钮绑定点击行为，而是给 QWindowKit 提供“语义信息”。

尤其是在 Windows 11 上，系统需要知道哪个区域是最大化按钮，才能在鼠标悬停时显示 Snap Layout。

如果你不设置：

`QWK::WindowAgentBase::Maximize`

那么自定义最大化按钮可能只是一个普通按钮，Windows 11 不知道它是最大化按钮，也就不会触发 Snap Layout。

---

### 5 手动连接按钮行为

官方明确说明：

> 设置 system button hints 不代表点击事件会自动关联到窗口行为。

所以你还需要自己连接信号槽：

```cpp
connect(titleBar->minButton(), &QPushButton::clicked, this, &QWidget::showMinimized);

connect(titleBar->maxButton(), &QPushButton::clicked, this, [this]() {
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
});

connect(titleBar->closeButton(), &QPushButton::clicked, this, &QWidget::close);

```

如果是 `QMainWindow`，同样适用：

```cpp
connect(titleBar->minButton(), &QPushButton::clicked, mainWindow, &QMainWindow::showMinimized);
connect(titleBar->closeButton(), &QPushButton::clicked, mainWindow, &QMainWindow::close);
```

---

### 6 设置可交互控件区域

标题栏中有些控件需要响应鼠标，例如：

- 菜单栏；
- 搜索框；
- 工具按钮；
- 标签页；
- 用户头像按钮；
- 自定义下拉框。

这些控件位于标题栏内部，如果不特殊声明，QWindowKit 可能会把它们所在区域当成“拖拽窗口区域”。

因此需要调用：


`agent->setHitTestVisible(titleBar->menuBar(), true);`

意思是：

> 这个控件需要正常接收鼠标事件，不要把它当成拖拽区域。

可以给多个控件设置：

```cpp
agent->setHitTestVisible(titleBar->menuBar(), true);
agent->setHitTestVisible(titleBar->searchEdit(), true);
agent->setHitTestVisible(titleBar->settingsButton(), true);
```

### 原理说明

在无边框窗口中，系统需要知道鼠标当前位置属于哪种区域：

|**区域**|**系统含义**|
|---|---|
|标题栏空白处|可拖拽窗口|
|边框区域|可缩放窗口|
|最小化按钮|系统最小化按钮|
|最大化按钮|系统最大化按钮|
|关闭按钮|系统关闭按钮|
|普通控件|交给 Qt 自己处理鼠标事件|

这个判断过程通常称为 **hit-test**，也就是“命中测试”。

在 Windows 中对应的典型消息是：


`WM_NCHITTEST`

如果 QWindowKit 判断当前位置在标题栏空白区域，就可能返回类似：

`HTCAPTION`

系统收到后就知道：

> 用户拖的是标题栏，可以移动窗口。

如果在边缘，则返回类似：


`HTLEFT / HTRIGHT / HTTOP / HTBOTTOM`

系统就知道：

> 用户正在拖动窗口边缘，需要调整窗口大小。

而 `setHitTestVisible()` 的作用是告诉 QWindowKit：

> 这个子控件区域不要拦截成系统拖拽区域，要让 Qt 控件自己处理。

---

### 7 禁用窗口最大化

README 提到，如果想禁用窗口最大化，可以移除：


`Qt::WindowMaximizeButtonHint`

例如：

```cpp
setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
```

或者在初始化窗口 flags 时：

```cpp
setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
```

这样通常也会影响最大化按钮行为和系统菜单状态。

## 官方案例



## qss设置

