#include "mywindow.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QPushButton>
#include <QStatusBar>
#include <QStyle>
#include <QVBoxLayout>

#include <QWKCore/styleagent.h>
#include <QWKWidgets/widgetwindowagent.h>

MyMainWindow::MyMainWindow(QWidget *parent) : QWidget(parent) {
    // QWK 必须的属性: 阻止系统为 native 子部件创建祖先
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    setWindowTitle(QStringLiteral("MyIDE - QWK + ADS Demo"));
    resize(1280, 800);

    setupUi();
    installWindowAgent();
    // ICore 注册由 main.cpp 在插件加载后完成 (PR-1 步骤8 App 集成):
    //   Core::ICore::setMainWindow(mywindow);
    //   Core::ICore::setMenuBar(mywindow->menuBar());
    //   ...
}

MyMainWindow::~MyMainWindow() = default;

void MyMainWindow::setCentralWidget(QWidget *w) {
    if (!m_centralContainer)
        return;

    auto *layout = qobject_cast<QVBoxLayout *>(m_centralContainer->layout());
    if (!layout) {
        layout = new QVBoxLayout(m_centralContainer);
        layout->setContentsMargins(0, 0, 0, 0);
    }

    // 清掉旧 central widget
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->setParent(m_centralContainer);
            item->widget()->hide();
        }
        delete item;
    }

    if (w)
        layout->addWidget(w);
}

void MyMainWindow::replaceMenuBar(QMenuBar *menuBar) {
    if (!menuBar || menuBar == m_menuBar)
        return;
    auto *titleLayout = qobject_cast<QHBoxLayout *>(m_titleBar->layout());
    if (!titleLayout)
        return;
    const int idx = titleLayout->indexOf(m_menuBar);
    if (idx < 0)
        return;
    titleLayout->removeWidget(m_menuBar);
    m_menuBar->hide();
    m_menuBar->deleteLater();

    m_menuBar = menuBar;
    m_menuBar->setParent(m_titleBar);
    titleLayout->insertWidget(idx, m_menuBar);

    // QWK: 菜单栏区域要响应自己的 hover/click, 不能被认为是拖动区
    if (m_windowAgent)
        m_windowAgent->setHitTestVisible(m_menuBar);
    // ICore::setMenuBar(m_menuBar) 由 main.cpp 在 replaceMenuBar 之后调用.
}

void MyMainWindow::setupUi() {
    // ---- 1. 自定义标题栏 (作为普通子部件) ----
    m_titleBar = new QWidget(this);
    m_titleBar->setObjectName(QStringLiteral("title-bar"));
    m_titleBar->setFixedHeight(32);

    auto *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(8, 0, 0, 0);
    titleLayout->setSpacing(0);

    // 图标与标题
    m_titleIcon = new QLabel(m_titleBar);
    m_titleIcon->setObjectName(QStringLiteral("title-icon"));
    m_titleIcon->setFixedSize(16, 16);
    m_titleIcon->setPixmap(style()->standardIcon(QStyle::SP_ComputerIcon).pixmap(16, 16));
    titleLayout->addWidget(m_titleIcon);

    m_titleLabel = new QLabel(m_titleBar);
    m_titleLabel->setObjectName(QStringLiteral("title-label"));
    m_titleLabel->setText(windowTitle());
    titleLayout->addSpacing(8);
    titleLayout->addWidget(m_titleLabel);

    titleLayout->addSpacing(16);

    // ---- 2. 菜单栏嵌入到标题栏中 ----
    m_menuBar = new QMenuBar(m_titleBar);

    auto *fileMenu = m_menuBar->addMenu(tr("&File"));
    auto *exitAction = fileMenu->addAction(tr("E&xit"), qApp, &QApplication::quit);
    exitAction->setShortcut(QKeySequence::Quit);

    auto *viewMenu = m_menuBar->addMenu(tr("&View"));
    auto *toggleThemeAction = viewMenu->addAction(tr("&Toggle Theme"), this, &MyMainWindow::toggleTheme);
    toggleThemeAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+T")));

    auto *helpMenu = m_menuBar->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About Qt"), qApp, &QApplication::aboutQt);
    helpMenu->addAction(tr("About &QWK"), this, [this]() {
        m_statusLabel->setText(tr("QWK 1.5.0 (qwindowkit) by stdware collections"));
    });

    titleLayout->addWidget(m_menuBar);
    titleLayout->addStretch();

    // ---- 3. 右侧按钮区 ----
    m_themeButton = new QPushButton(m_titleBar);
    m_themeButton->setObjectName(QStringLiteral("theme-button"));
    m_themeButton->setFixedSize(40, 32);
    m_themeButton->setText(QStringLiteral("Theme"));
    titleLayout->addWidget(m_themeButton);
    connect(m_themeButton, &QPushButton::clicked, this, &MyMainWindow::toggleTheme);

    m_minButton = new QPushButton(m_titleBar);
    m_minButton->setObjectName(QStringLiteral("min-button"));
    m_minButton->setFixedSize(40, 32);
    m_minButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarMinButton));
    titleLayout->addWidget(m_minButton);

    m_maxButton = new QPushButton(m_titleBar);
    m_maxButton->setObjectName(QStringLiteral("max-button"));
    m_maxButton->setFixedSize(40, 32);
    m_maxButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
    titleLayout->addWidget(m_maxButton);

    m_closeButton = new QPushButton(m_titleBar);
    m_closeButton->setObjectName(QStringLiteral("close-button"));
    m_closeButton->setFixedSize(40, 32);
    m_closeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    titleLayout->addWidget(m_closeButton);

    // ---- 4. 中央内容容器 (放 dockManager) ----
    m_centralContainer = new QWidget(this);
    m_centralContainer->setObjectName(QStringLiteral("central-container"));
    // auto *centralLayout = new QVBoxLayout(m_centralContainer);
    // centralLayout->setContentsMargins(0, 0, 0, 0);
    // centralLayout->setSpacing(0);

    // ---- 4b. 左侧模式栏容器 (供 ModeBarAdapter 挂 IMode 按钮) ----
    m_modeBarContainer = new QWidget(this);
    m_modeBarContainer->setObjectName(QStringLiteral("mode-bar-container"));
    m_modeBarContainer->setFixedWidth(48);
    auto *modeBarLayout = new QVBoxLayout(m_modeBarContainer);
    modeBarLayout->setContentsMargins(0, 4, 0, 4);
    modeBarLayout->setSpacing(2);
    modeBarLayout->addStretch();

    // ---- 5. 状态栏 ----
    m_statusBar = new QStatusBar(this);
    m_statusBar->setObjectName(QStringLiteral("status-bar"));
    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName(QStringLiteral("status-label"));
    m_statusLabel->setText(tr("QWK ready - drag the title bar to move"));
    m_statusBar->addWidget(m_statusLabel);

    // ---- 6. 主布局: 标题栏 + (模式栏 | 中央) + 状态栏 ----
    // 用 QVBoxLayout (而非 QMainWindow::setMenuWidget) 让 QWK 完整控制标题栏
    auto *bodyLayout = new QHBoxLayout;
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);
    bodyLayout->addWidget(m_modeBarContainer);
    bodyLayout->addWidget(m_centralContainer, 1);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_titleBar);
    mainLayout->addLayout(bodyLayout, 1);
    mainLayout->addWidget(m_statusBar);
}

void MyMainWindow::installWindowAgent() {
    m_windowAgent = new QWK::WidgetWindowAgent(this);
    if (!m_windowAgent->setup(this)) {
        qWarning() << "QWK: failed to setup WidgetWindowAgent";
    }

    // 注册标题栏 (QWK 据此知道哪些区域属于"系统标题栏")
    m_windowAgent->setTitleBar(m_titleBar);

    // 注册系统按钮
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::Minimize, m_minButton);
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::Maximize, m_maxButton);
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::Close,    m_closeButton);

    connect(m_minButton, &QPushButton::clicked, this, &MyMainWindow::showMinimized);
    connect(m_maxButton, &QPushButton::clicked, this, [this]() {
        if (isMaximized()) showNormal();
        else showMaximized();
    });
    connect(m_closeButton, &QPushButton::clicked, this, &MyMainWindow::close);

    // 关键: 标记"可拖动"区域. 鼠标在以下部件上按下 → 视为系统拖动手势
    // 必须包含 m_titleBar 自身 (拖动空白处) 和标题文本/图标
    // m_windowAgent->setHitTestVisible(m_titleBar);
    // m_windowAgent->setHitTestVisible(m_titleLabel);
    // m_windowAgent->setHitTestVisible(m_titleIcon);
    // 注意: 菜单栏 / 系统按钮 / 主题按钮 不在列表中, 它们会正常响应自己的事件

    m_windowAgent->setHitTestVisible(m_menuBar);
    m_windowAgent->setHitTestVisible(m_themeButton);
    m_windowAgent->centralize();

    m_styleAgent = new QWK::StyleAgent(this);
    //TODO ...m_styleAgent


    applyTheme(m_darkTheme);
}

void MyMainWindow::toggleTheme() {
    m_darkTheme = !m_darkTheme;
    applyTheme(m_darkTheme);
}

void MyMainWindow::applyTheme(bool dark) {
    if (dark) {
        qApp->setStyleSheet(R"(
            QWidget { background: #2b2b2b; color: #e0e0e0; }
            QMenuBar { background: transparent; color: #e0e0e0; padding: 2px; }
            QMenuBar::item:selected { background: #3a76c0; }
            QMenu { background: #2b2b2b; color: #e0e0e0; border: 1px solid #1e1e1e; }
            QMenu::item:selected { background: #3a76c0; }
            #title-bar { background: #1e1e1e; }
            #title-label { color: #e0e0e0; font-weight: bold; }
            #min-button, #max-button, #theme-button {
                background: transparent; color: #e0e0e0; border: none;
            }
            #min-button:hover, #max-button:hover, #theme-button:hover { background: #3a3a3a; }
            #close-button { background: transparent; color: #e0e0e0; border: none; }
            #close-button:hover { background: #e81123; color: white; }
            QStatusBar { background: #1e1e1e; color: #e0e0e0; }
        )");
    } else {
        qApp->setStyleSheet(R"(
            #title-bar { background: #e8e8e8; }
            #title-label { color: #1a1a1a; font-weight: bold; }
            #min-button, #max-button, #theme-button, #close-button {
                background: transparent; color: #1a1a1a; border: none;
            }
            #min-button:hover, #max-button:hover, #theme-button:hover { background: #d0d0d0; }
            #close-button:hover { background: #e81123; color: white; }
        )");
    }
    m_statusLabel->setText(dark ? tr("Theme: Dark - drag the title bar to move") : tr("Theme: Light - drag the title bar to move"));
}
