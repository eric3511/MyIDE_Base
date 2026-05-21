
`IPlugin` 是每个 Qt Creator C++ 插件必须继承的基类。理解它，就等于理解了插件“从出生到退出”的标准协议。

---

##  `IPlugin` 详解


1. `IPlugin` 为什么继承 `QObject`；
2. 插件生命周期函数各自负责什么；
3. `initialize()`、`extensionsInitialized()`、`delayedInitialize()`、`aboutToShutdown()` 的调用顺序；
4. `IPlugin::addObject()` 和 `removeObject()` 与对象池的关系；
5. 插件关闭时为什么需要 `ShutdownFlag`；
6. 为什么复杂逻辑不能全部塞进 `initialize()`。

---

## 1. 阅读入口

请打开 Qt Creator 源码目录：

`iplugin.h iplugin.cpp`

如果你的 Qt Creator 版本中还有相关私有实现，也可以顺手看一下，但本课重点只看公开接口和核心逻辑。

---

### 1.1 先看 `iplugin.h`

你要优先找到类似结构：

```cpp
class EXTENSIONSYSTEM_EXPORT IPlugin : public QObject
{
    Q_OBJECT

public:
    enum ShutdownFlag {
        SynchronousShutdown,
        AsynchronousShutdown
    };

    IPlugin();
    ~IPlugin() override;

    virtual bool initialize(const QStringList &arguments, QString *errorString) = 0;
    virtual void extensionsInitialized() {}
    virtual bool delayedInitialize() { return false; }
    virtual ShutdownFlag aboutToShutdown() { return SynchronousShutdown; }

protected:
    void addObject(QObject *obj);
    void removeObject(QObject *obj);
    void addAutoReleasedObject(QObject *obj);

    // ...
};
```

不同版本源码可能略有差异，以实际代码为准。

---

### 1.2 再看 `iplugin.cpp`

重点找这些实现：

```cpp
IPlugin::IPlugin()
IPlugin::~IPlugin()

void IPlugin::addObject(QObject *obj)
void IPlugin::removeObject(QObject *obj)
void IPlugin::addAutoReleasedObject(QObject *obj)
```

你会看到 `IPlugin` 本身不负责扫描插件、不负责解析依赖、不负责加载动态库。

它的角色更像是：

> 插件对象必须遵守的生命周期接口 + 对对象池的便捷封装。

---

## 2. 为什么 `IPlugin` 继承 `QObject`

`IPlugin` 通常定义为：


`class IPlugin : public QObject`

这不是随便继承的。Qt Creator 插件系统高度依赖 Qt 的对象模型。

---

### 2.1 `QObject` 带来的能力

继承 `QObject` 后，插件具备：

- 信号槽能力；
- 父子对象生命周期管理；
- 元对象系统；
- `qobject_cast`；
- 动态属性；
- 事件处理；
- 与 Qt 插件系统配合；
- 可以被放入对象池中通过接口查询。

尤其是：

`qobject_cast<T *>(object)`

对对象池非常重要。

---

### 2.2 插件对象本身也是 QObject

插件实例本身也是一个对象，可以：

- 作为其他 QObject 的 parent；
- 管理插件内部对象生命周期；
- 连接信号槽；
- 响应异步关闭；
- 挂接内部服务对象。

例如在你的插件中：

`auto action = new QAction(tr("My Plugin Action"), this);`

这里 `this` 就是插件对象。

这样插件销毁时，`action` 也会被 Qt 父子对象机制释放。

---

## 3. `IPlugin` 的生命周期函数

`IPlugin` 最重要的是生命周期函数。

典型顺序是：

```text
构造函数
   ↓
initialize()
   ↓
extensionsInitialized()
   ↓
delayedInitialize()
   ↓
运行中
   ↓
aboutToShutdown()
   ↓
析构函数
```

注意：真正的调用者不是 `IPlugin` 自己，而是 `PluginManager`。

---

### 3.1 构造函数

插件对象被创建时会调用构造函数。

通常由：

`QPluginLoader::instance()`

触发。

构造函数中建议只做轻量操作。

适合：

- 初始化成员变量；
- 设置简单默认值；
- 不依赖外部插件的轻量准备。

不适合：

- 注册菜单；
- 查询其他插件；
- 启动线程；
- 访问对象池；
- 执行耗时任务。

原因是：

> 构造函数发生在插件实例创建阶段，此时插件系统还没有进入正式初始化协议。

---

### 3.2 `initialize()`

函数形式：
```cpp
virtual bool initialize(const QStringList &arguments, QString *errorString) = 0;
```

这是唯一一个通常必须实现的生命周期函数。

---

#### 3.2.1 `initialize()` 的职责

适合做：

- 创建插件核心对象；
- 注册菜单；
- 注册 Action；
- 注册对象到对象池；
- 读取基础配置；
- 注册设置页；
- 检查命令行参数；
- 初始化不依赖“所有插件已完成初始化”的功能。

例如：

```cpp
bool MyPluginPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    auto action = new QAction(tr("My Action"), this);
    // 注册 Action 到 Core::ActionManager

    m_service = new MyService(this);
    addObject(m_service);

    return true;
}
```

---

#### 3.2.2 `initialize()` 的返回值

返回：`true`表示初始化成功。

返回：`false`表示初始化失败。

失败时应该设置：

```cpp
if (errorString)
    *errorString = tr("Failed to initialize MyPlugin because ...");

```

例如：

```cpp
bool MyPluginPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    if (!checkEnvironment()) {
        if (errorString)
            *errorString = tr("Required environment is missing.");
        return false;
    }

    return true;
}
```


这会影响后续：

- 插件不会进入正常运行状态；
- `extensionsInitialized()` 通常不会作为正常插件继续执行；
- 错误信息会进入 `PluginSpec`；
- 插件管理 UI 可以展示错误。

---

#### 3.2.3 `initialize()` 中能不能使用依赖插件

可以，但要谨慎。

如果你的 JSON 中声明了硬依赖：

```json
"Dependencies" : [
    { "Name" : "Core", "Version" : "..." }
]
```

通常你的插件 `initialize()` 被调用时，依赖插件已经完成了 `initialize()`。

所以你可以在 `initialize()` 中使用 `Core` 提供的基础能力，比如：

```cpp
Core::ActionManager
Core::ICore
```

但不建议在 `initialize()` 中假设：

`所有插件都已经初始化完毕`

因为这不是事实。

这类跨全局插件联动，应该放在：

`extensionsInitialized()`

---

### 3.3 `extensionsInitialized()`

函数形式：

```cpp
virtual void extensionsInitialized();

```

这个函数在所有应加载插件的 `initialize()` 都完成之后调用。

---

#### 3.3.1 它解决什么问题

假设有三个插件：

```text
CorePlugin
ProjectExplorerPlugin
MyPlugin
```

加载过程可能是：

```text
CorePlugin.initialize()
ProjectExplorerPlugin.initialize()
MyPlugin.initialize()

CorePlugin.extensionsInitialized()
ProjectExplorerPlugin.extensionsInitialized()
MyPlugin.extensionsInitialized()
```


也就是说：

> 到 `extensionsInitialized()` 阶段，所有插件已经完成第一阶段初始化，很多服务对象已经被注册到对象池。

---

#### 3.3.2 适合做什么

适合：

- 从对象池查询其他插件注册的对象；
- 建立跨插件信号槽连接；
- 初始化依赖多个插件共同存在的逻辑；
- 进行插件之间的协作绑定。

例如：

```cpp
void MyPluginPlugin::extensionsInitialized()
{
    auto projectManager = PluginManager::getObject<IProjectManager>();
    auto editorManager = PluginManager::getObject<IEditorManager>();

    if (projectManager && editorManager) {
        connect(projectManager, &IProjectManager::projectOpened,
                editorManager, &IEditorManager::updateContext);
    }
}
```

这类逻辑放在 `extensionsInitialized()` 更合理。

---

### 3.4 `delayedInitialize()`

函数形式可能类似：

```cpp
virtual bool delayedInitialize();
```

不同版本可能返回值略有不同，但核心语义一样：**延迟初始化**。

---

#### 3.4.1 为什么需要延迟初始化

IDE 启动性能非常重要。

如果所有插件都在 `initialize()` 里做耗时任务：

```text
扫描文件
启动语言服务器
加载大型缓存
连接设备
初始化索引数据库
扫描工具链
```

Qt Creator 启动会变慢。

所以引入：

`delayedInitialize()`

用于把非首屏必要任务延后。

---

#### 3.4.2 适合放什么

适合：

- 启动后台索引；
- 扫描工具链；
- 延迟加载缓存；
- 初始化语言服务；
- 检查更新；
- 扫描外部设备；
- 建立可延后的网络连接。

对于 PLC IDE，尤其适合：

```cpp
扫描 PLC 设备
启动在线监控服务
加载历史变量缓存
启动 ST 语言语义分析服务
扫描编译器工具链
```


---

#### 3.4.3 不适合放什么

不适合：

- 用户启动后立即要看到的菜单；
- 基础服务注册；
- 必须给其他插件立即使用的接口；
- 影响插件依赖关系的初始化。

这些应该放在：

`initialize()`

或者：

`extensionsInitialized()`

---

### 3.5 `aboutToShutdown()`

函数形式：

```cpp
virtual ShutdownFlag aboutToShutdown();
```


这是插件关闭前的通知。

---

#### 3.5.1 适合做什么

适合：

- 保存配置；
- 关闭文件；
- 停止后台线程；
- 停止定时器；
- 断开设备连接；
- 取消网络请求；
- 从对象池移除对象；
- 释放外部资源。

例如：

```cpp
ExtensionSystem::IPlugin::ShutdownFlag MyPluginPlugin::aboutToShutdown()
{
    removeObject(m_service);
    m_service = nullptr;

    return SynchronousShutdown;
}
```


---

#### 3.5.2 为什么关闭流程很重要

对普通插件来说，关闭可能只是保存设置。

但对 PLC IDE 来说，关闭流程非常关键：

```text
在线监控线程必须停止
PLC 通信连接必须断开
下载任务不能中途悬挂
后台编译进程要退出
临时工程文件要保存
```

否则可能造成：

- 设备连接未释放；
- 后台线程崩溃；
- 工程状态丢失；
- 下次启动恢复异常；
- 调试会话残留。

---

## 4. `ShutdownFlag`：同步关闭与异步关闭

`aboutToShutdown()` 返回一个 `ShutdownFlag`。

常见值：

`SynchronousShutdown`
`AsynchronousShutdown`

---

### 4.1 `SynchronousShutdown`

表示插件可以立即关闭。

适合：

- 没有后台任务；
- 清理逻辑很快；
- 不需要等待异步操作完成。

例如：

`return SynchronousShutdown;`

你在最小插件中通常用这个。

---

### 4.2 `AsynchronousShutdown`

表示插件需要异步关闭。

适合：

- 后台线程需要结束；
- 网络请求需要取消；
- 外部进程需要退出；
- 设备通信需要断开；
- 保存大型数据需要时间。

概念上：

```text
PluginManager 调用 aboutToShutdown()
   ↓
插件返回 AsynchronousShutdown
   ↓
插件开始异步清理
   ↓
清理完成后通知 PluginManager
   ↓
PluginManager 继续关闭流程
```

具体通知方式要看当前版本 `IPlugin` 源码，可能涉及信号或特定回调。

---

### 4.3 PLC IDE 中的异步关闭示例

比如 `DevicePlugin` 正在和 PLC 通信：

```text
aboutToShutdown()
   ↓
停止在线监控
   ↓
发送断开连接请求
   ↓
等待通信线程退出
   ↓
通知关闭完成
```


这种场景就不适合粗暴同步返回。

---

## 5. `IPlugin` 与对象池

`IPlugin` 通常提供几个保护方法：

```cpp
void addObject(QObject *obj);
void removeObject(QObject *obj);
void addAutoReleasedObject(QObject *obj);
```

这些方法是对 `PluginManager` 对象池的封装。

---

### 5.1 `addObject()`

概念上类似：

```cpp
void IPlugin::addObject(QObject *obj)
{
    PluginManager::addObject(obj);
}
```

它用于把插件提供的服务对象注册到全局对象池。

例如：

```cpp
m_service = new MyService(this);
addObject(m_service);
```


其他插件可以通过：

```cpp
auto service = PluginManager::getObject<IMyService>();
```

获取它。

---

### 5.2 `removeObject()`

概念上类似：

```cpp
void IPlugin::removeObject(QObject *obj)
{
    PluginManager::removeObject(obj);
}
```


用于从对象池移除对象。

通常在：

`aboutToShutdown()`

或插件析构前调用。

例如：

```cpp
ExtensionSystem::IPlugin::ShutdownFlag MyPluginPlugin::aboutToShutdown()
{
    removeObject(m_service);
    return SynchronousShutdown;
}
```

---

### 5.3 `addAutoReleasedObject()`

某些版本中有：

```cpp
addAutoReleasedObject(QObject *obj);
```


它的作用通常是：

- 添加对象到对象池；
- 在插件销毁或关闭时自动释放 / 移除。

具体行为要以源码为准。

读源码时你要确认：

1. 它是否调用了 `PluginManager::addObject()`；
2. 它是否记录了对象列表；
3. 它在析构或关闭时是否自动 remove；
4. 它是否负责 delete 对象。

---

### 5.4 对象池不等于对象所有权

这里一定要小心。

对象池常见设计是：

`对象池只保存 QObject 指针 不一定拥有对象生命周期`

也就是说：


`PluginManager::addObject(obj);`

不一定表示 `PluginManager` 会 delete 这个对象。

通常责任是：

`谁创建，谁释放 谁注册，谁移除`

所以推荐：

```cpp
m_service = new MyService(this);
addObject(m_service);
//并在关闭时：
removeObject(m_service);
```

这样对象由插件父子关系管理，注册关系由插件自己清理。

---

## 6. 生命周期中各阶段该放什么

这是本课最重要的实战规则。

---

### 6.1 推荐职责划分

|**阶段**|**推荐做**|**避免做**|
|---|---|---|
|构造函数|初始化成员默认值|访问对象池、注册 UI、启动线程|
|`initialize()`|注册菜单、Action、服务对象、设置页|耗时任务、依赖所有插件完成初始化|
|`extensionsInitialized()`|跨插件连接、查询其他服务|耗时任务|
|`delayedInitialize()`|后台任务、索引、扫描、缓存加载|必须立即可用的服务注册|
|`aboutToShutdown()`|停止任务、移除对象、保存状态|开启新任务、复杂 UI 交互|
|析构函数|最后兜底清理|依赖其他插件仍然可用|

---

### 6.2 一个良好的插件结构

```cpp
bool MyPluginPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)

    if (!checkBasicEnvironment()) {
        if (errorString)
            *errorString = tr("Basic environment check failed.");
        return false;
    }

    createActions();
    createMenus();
    createServices();
    registerObjects();

    return true;
}

void MyPluginPlugin::extensionsInitialized()
{
    connectToProjectSystem();
    connectToEditorSystem();
}

bool MyPluginPlugin::delayedInitialize()
{
    startBackgroundIndexing();
    return true;
}

ExtensionSystem::IPlugin::ShutdownFlag MyPluginPlugin::aboutToShutdown()
{
    stopBackgroundIndexing();
    unregisterObjects();
    saveSettings();

    return SynchronousShutdown;
}
```

这种结构清晰、可维护、可调试。

