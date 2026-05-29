# 欢迎来到新版 IDE 项目 (Claude Code 行为指南)

本项目是一个基于 **Qt 6** 的新一代可扩展桌面 IDE，采用“轻量内核 + 插件化 + 协议驱动智能”的技术路线。部分核心模块提取自 Qt Creator，但整体架构更加轻量且专注于跨平台体验。

作为 AI 编程助手，在参与本项目开发时，请务必严格遵守以下架构边界、编码规范与行为准则。

## 1. 核心行为准则 (AI 必读)
- **不要假设与脑补**：如果没有看到类、函数或 CMake target，请先搜索现有代码，不要直接编造 API。
- **尊重模块边界**：严禁跨插件直接调用内部实现。所有跨模块通信必须通过 `ExtensionSystem` 获取公共接口或使用信号槽。
- **保护提取代码**：对于从 Qt Creator 提取的模块（如 `ExtensionSystem`, `ProjectExplorer`, `ActionManager`），尽量保持其原有核心逻辑，仅在适配层做修改。
- **暴露不确定性**：如果你对某个跨平台特性（特别是 Windows ConPTY）不确定，请在修改前明确指出风险。

## 2. 技术栈与架构选型

| 模块 | 技术选型 | 来源/备注 |
| :--- | :--- | :--- |
| **底层与 UI** | Qt 6 + QWindowKit + QAds | 现代化无边框与高级停靠系统 |
| **插件底座** | `ExtensionSystem` (PluginManager) | 提取自 Qt Creator，严格管理生命周期 |
| **核心管理器** | `ActionManager`, `Locator`, `Settings` | 提取自 Qt Creator `Core` 插件 |
| **日志系统** | spdlog | 自封装，已桥接 `qInstallMessageHandler` |
| **代码编辑器** | QScintilla | 开源组件，负责文本渲染与基础交互 |
| **语言智能** | LSP Client + Language Servers | 提取自 Qt Creator / 协议驱动补全跳转 |
| **调试智能** | DAP Client + Debug Adapters | 协议实现，不硬编码具体调试器 |
| **终端嵌入** | QTermWidget | **注意：** Windows 下需特殊处理 ConPTY |
| **项目与构建** | `ProjectExplorer` 模型 | 提取自 Qt Creator，管理构建目标 |
| **版本控制** | Git 命令行解析封装 | 使用 `QProcess` 调用，严防命令注入 |

## 3. 架构与依赖规则
- **单向依赖**：`App` -> `Core` -> `ExtensionSystem` -> `Shared Utilities`。
- **协议驱动**：`Editor` 插件不应该知道具体的 C++ 或 Python 语法，所有智能特性必须通过 `LanguageClient` (LSP) 路由。
- **调试解耦**：`Debugger` 插件仅负责 DAP 协议解析与状态机，具体运行目标由 `ProjectExplorer` 提供。
- **UI 分离**：业务逻辑严禁写在 `QWidget` 子类中，UI 状态需与底层 Model 同步。

## 4. 编码规范 (C++ & Qt)
- **标准**：使用现代 C++ (C++17/20，以 CMakeLists.txt 为准)。
- **命名**：类名 `PascalCase`，函数 `camelCase`，成员变量使用 `m_` 前缀。
- **内存管理**：优先使用 Qt 的 Parent-Child 对象树机制（`new T(parent)`）；非 QObject 优先使用 `std::unique_ptr`。
- **跨平台**：路径处理必须使用 `QDir`, `QFileInfo`, `QStandardPaths`，严禁硬编码 `/` 或 `\\`。
- **日志**：使用项目封装的 spdlog 宏（如 `LOG_INFO`, `LOG_ERROR`）。禁止在控制台打印敏感信息（如 Token、密钥）或高频 LSP 通信全文。

## 5. 常见开发任务指南

### 5.1 添加新插件
1. 在 `src/plugins/` 下创建目录。
2. 实现 `ExtensionSystem::IPlugin` 接口（处理 `initialize`, `extensionsInitialized` 等生命周期）。
3. 在 `CMakeLists.txt` 中正确链接 `ExtensionSystem` 和 `Core`。

### 5.2 注册全局动作 (Action)
必须通过 `Core::ActionManager` 注册，并分配全局唯一的 ID，以便统一管理快捷键和菜单栏。

### 5.3 调用外部进程 (Git/LSP/DAP)
- 必须使用 `QProcess`。
- 参数必须作为 `QStringList` 传入，严禁将用户输入直接拼接到命令字符串中。
- 必须处理 `QProcess::errorOccurred` 并输出到日志。

## 6. 构建与测试命令示例
*(AI 在执行构建前，请先读取根目录的 CMakeLists.txt 确认具体参数)*

