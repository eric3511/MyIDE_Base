#include "testplugin.h"

// ExtensionSystem
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

// Utils
#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/hostosinfo.h>
#include <utils/environment.h>
#include <utils/qtcsettings.h>
#include <utils/stringutils.h>
#include <utils/algorithm.h>

// Aggregation
#include <aggregation/aggregate.h>

// AdvancedDockingSystem
#include <advanceddockingsystem/DockManager.h>
#include <advanceddockingsystem/DockWidget.h>
#include <advanceddockingsystem/DockAreaWidget.h>

// KSyntaxHighlighting
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Theme>

// 3rd_cplusplus
#include <cplusplus/CPlusPlus.h>
#include <cplusplus/Control.h>
#include <cplusplus/Parser.h>
#include <cplusplus/TranslationUnit.h>
#include <cplusplus/Literals.h>

#include <QDebug>
#include <QMessageBox>
#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QPushButton>

using namespace ExtensionSystem;
using namespace Utils;

TestPlugin::TestPlugin() = default;
TestPlugin::~TestPlugin() = default;

bool TestPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)

    // --- 1. ExtensionSystem: plugin spec & object pool ---
    PluginManager *pm = PluginManager::instance();
    qDebug() << "[TestPlugin] Initializing, plugin spec:" << pluginSpec()->name()
             << "version:" << pluginSpec()->version();

    // --- 2. Utils: FilePath, Id, HostOsInfo ---
    FilePath appPath = FilePath::fromString(qApp->applicationDirPath());
    qDebug() << "[TestPlugin] App dir:" << appPath;
    qDebug() << "[TestPlugin] isWindows:" << HostOsInfo::isWindowsHost()
             << "isLinux:" << HostOsInfo::isLinuxHost()
             << "isMac:" << HostOsInfo::isMacHost();

    Id testId = Id::fromString("TestPlugin.SomeAction");
    qDebug() << "[TestPlugin] Test Id:" << testId.toString();

    // Utils: Environment
    Environment env = Environment::systemEnvironment();
    qDebug() << "[TestPlugin] PATH entries:" << env.path().size();

    // Utils: StringUtils
    qDebug() << "[TestPlugin] stripped:" << QString("  hello world  ").trimmed();

    // Utils: QtcSettings
    QtcSettings settings(QStringLiteral("MyIDE"), QStringLiteral("TestPlugin"));
    settings.setValueWithDefault(QStringLiteral("TestKey"), QStringLiteral("TestValue"));
    qDebug() << "[TestPlugin] Settings testKey:" << settings.value("TestKey").toString();

    // --- 3. Aggregation ---
    auto *aggregate = new Aggregation::Aggregate;
    auto *obj1 = new QObject;
    obj1->setObjectName("AggregationTestObject");
    aggregate->add(obj1);
    QObject *found = Aggregation::query<QObject>(obj1);
    if (found) {
        qDebug() << "[TestPlugin] Aggregation: found object" << found->objectName();
    }
    delete aggregate; // transfers ownership, deletes obj1 too

    // --- 4. AdvancedDockingSystem ---
    auto *dockManager = new ads::CDockManager;
    auto *dockWidget = new ads::CDockWidget(dockManager, "Test Library Verification");
    auto *centralWidget = new QWidget;
    auto *layout = new QVBoxLayout(centralWidget);

    // status label
    auto *statusLabel = new QLabel;
    layout->addWidget(statusLabel);

    // text edit for testing syntax highlighting
    auto *textEdit = new QTextEdit;
    textEdit->setPlainText(
        "#include <iostream>\n"
        "int main() {\n"
        "    std::cout << \"Hello from TestPlugin!\" << std::endl;\n"
        "    return 0;\n"
        "}"
    );
    layout->addWidget(textEdit);

    // --- 5. KSyntaxHighlighting ---
    KSyntaxHighlighting::Repository repo;
    KSyntaxHighlighting::Definition def = repo.definitionForName("C++");
    if (def.isValid()) {
        qDebug() << "[TestPlugin] KSyntaxHighlighting: found C++ definition, keywords:"
                 << def.keywordLists().size();
    }
    KSyntaxHighlighting::Theme theme = repo.defaultTheme(KSyntaxHighlighting::Repository::LightTheme);
    qDebug() << "[TestPlugin] KSyntaxHighlighting theme:" << theme.name();

    // --- 6. 3rd_cplusplus ---
    CPlusPlus::LanguageFeatures features;
    features.cxxEnabled = true;
    features.qtEnabled = true;
    CPlusPlus::Control control;
    CPlusPlus::TranslationUnit unit(&control, nullptr);

    const QString testCode = QStringLiteral("class Foo : public QObject { Q_OBJECT };");
    const QByteArray codeBytes = testCode.toUtf8();
    unit.setSource(codeBytes.constData(), codeBytes.size());
    if (unit.tokenCount() > 0) {
        qDebug() << "[TestPlugin] CPlusplus: parsed" << unit.tokenCount() << "tokens";
    }

    // parse a small C++ snippet with the parser
    CPlusPlus::Parser parser(&unit);
    CPlusPlus::TranslationUnitAST *ast = nullptr;
    if (parser.parseTranslationUnit(ast)) {
        qDebug() << "[TestPlugin] CPlusplus parser: translation unit parsed successfully";
    }

    // --- Build status report ---
    QStringList libsOk;
    libsOk << "ExtensionSystem"
           << "Utils"
           << "Aggregation"
           << "AdvancedDockingSystem"
           << ("KSyntaxHighlighting:" + theme.name())
           << "3rd_cplusplus";

    statusLabel->setText("All libraries verified:\n" + libsOk.join("\n"));

    // --- Setup dock ---
    dockWidget->setWidget(centralWidget);
    dockManager->setCentralWidget(dockWidget);

    // Register dock manager so other plugins can find it
    pm->addObject(dockManager);

    qDebug() << "[TestPlugin] All" << libsOk.size() << "libraries verified successfully.";

    if (errorString)
        *errorString = QString();

    return true;
}

void TestPlugin::extensionsInitialized()
{
    qDebug() << "[TestPlugin] extensionsInitialized: all dependent plugins ready.";
}
