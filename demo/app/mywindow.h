#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QMenuBar;
class QPushButton;
class QStatusBar;
QT_END_NAMESPACE

namespace QWK {
class WidgetWindowAgent;
class StyleAgent;
}

// MyMainWindow: 演示 QWK 1.5.0 的关键 API
//   * WidgetWindowAgent       -- 把 agent 挂到 QWidget
//   * setTitleBar             -- 注册自定义标题栏 (支持窗口拖动)
//   * setSystemButton         -- 注册最小化/最大化/关闭按钮
//   * setHitTestVisible       -- 标记可拖动区域 (关键!)
//   * centralize              -- 窗口居中
//   * StyleAgent              -- 主题切换
//   * 自定义 setCentralWidget -- 兼容 QMainWindow 的 API 习惯
//
// 为什么继承 QWidget 而非 QMainWindow:
//   QMainWindow::setMenuWidget(m_titleBar) 会让 QMainWindow 在 m_titleBar 上
//   安装自己的事件处理, 抢先于 QWK 处理鼠标事件, 导致 setHitTestVisible 失效、
//   标题栏无法拖动. 改用 QWidget + QVBoxLayout 后, QWK 完全掌控标题栏, 拖动正常.
class MyMainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MyMainWindow(QWidget *parent = nullptr);
    ~MyMainWindow() override;

    // 模拟 QMainWindow::setCentralWidget 的接口, 方便 main.cpp 调用
    void setCentralWidget(QWidget *w);

    // --- 新主窗口契约 (供 ICore / ActionManager / 适配器使用) ---
    // 标题栏中的菜单栏 (ActionManager 的 MENU_BAR 容器挂在这里)
    QMenuBar *menuBar() const { return m_menuBar; }
    // 底部状态栏 (StatusBarManager / ProgressView 用)
    QStatusBar *statusBar() const { return m_statusBar; }
    // 中央内容容器 (ADS CDockManager 挂在这里)
    QWidget *centralContainer() const { return m_centralContainer; }
    // 左侧模式栏容器 (ModeBarAdapter 把 IMode 按钮添加到这里)
    QWidget *modeBarContainer() const { return m_modeBarContainer; }

    // 把 AM 创建的菜单栏替换掉 setupUi() 中的占位菜单栏.
    // 在 ActionManager::createMenuBar(MENU_BAR) 后由 host 调用.
    void replaceMenuBar(QMenuBar *menuBar);

public slots:
    void toggleTheme();

private:
    void setupUi();
    void installWindowAgent();
    void applyTheme(bool dark);

    QWK::WidgetWindowAgent *m_windowAgent = nullptr;
    QWK::StyleAgent *m_styleAgent = nullptr;

    QWidget *m_titleBar = nullptr;
    QLabel *m_titleIcon = nullptr;
    QLabel *m_titleLabel = nullptr;
    QMenuBar *m_menuBar = nullptr;
    QPushButton *m_themeButton = nullptr;
    QPushButton *m_minButton = nullptr;
    QPushButton *m_maxButton = nullptr;
    QPushButton *m_closeButton = nullptr;

    QWidget *m_centralContainer = nullptr;
    QWidget *m_modeBarContainer = nullptr;
    QStatusBar *m_statusBar = nullptr;
    QLabel *m_statusLabel = nullptr;

    bool m_darkTheme = false;
};
