/*
 * Devpad - Test-only plugin for plugin infrastructure tests
 * Copyright (C) 2026 Semagsoft
 *
 * Build-time definitions supply TEST_PLUGIN_ID. The manifest next to the
 * library carries the same id and the dependency list under test.
 */

#include "plugins/plugininterface.h"

#include <QObject>

class TestPlugin : public QObject, public IPlugin
{
    Q_OBJECT

public:
    PluginInfo info() const override
    {
        PluginInfo i;
        i.id = QStringLiteral(TEST_PLUGIN_ID);
        i.name = QStringLiteral(TEST_PLUGIN_ID);
        i.version = QStringLiteral("1.0.0");
        i.author = QStringLiteral("Semagsoft");
        i.description = QStringLiteral("Test-only plugin.");
        i.apiVersion = 1;
        i.enabled = true;
        return i;
    }

    bool initialize(IPluginHost*) override
    {
        return true;
    }
    void shutdown() override
    {
    }
};

extern "C" IPlugin* devpadCreatePlugin()
{
    return new TestPlugin;
}

#include "testplugin.moc"
