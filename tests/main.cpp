#include <QApplication>
#include <QDir>
#include <QString>

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");

    // Isolate all default-constructed QSettings (and QStandardPaths lookups)
    // from the real user configuration. SettingsManager redirects through
    // setTestingInstance()/createForTesting(); every feature-scoped store
    // that uses a bare QSettings (SessionManager, UpdateChecker, DialogSettings,
    // InlineFindBar, ...) lands here instead of ~/.config/Semagsoft/Devpad.conf.
    // PID-qualified so parallel ctest processes don't collide.
    const QString isolateBase = QDir::temp().filePath(QStringLiteral("devpad_test_%1").arg(QCoreApplication::applicationPid()));
    qputenv("XDG_CONFIG_HOME", (isolateBase + QStringLiteral("/config")).toUtf8());
    qputenv("XDG_DATA_HOME", (isolateBase + QStringLiteral("/data")).toUtf8());
    qputenv("XDG_CACHE_HOME", (isolateBase + QStringLiteral("/cache")).toUtf8());

    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
