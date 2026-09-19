/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#ifndef PALETTECHANGEFILTER_H
#define PALETTECHANGEFILTER_H

#include <QEvent>
#include <QObject>

#ifdef HAVE_QT_DBUS
#include <QDBusVariant>
#endif

// Watches for system theme changes and invalidates the cached
// system-dark value in theme.cpp. Lives in its own header (rather than
// defined in theme.cpp with `#include "theme.moc"`) so AUTOMOC generates
// the moc output as a separate compilation unit — clang-tidy parses
// theme.cpp without a prior build and would otherwise fail with
// "'theme.moc' file not found".
class PaletteChangeFilter : public QObject
{
    Q_OBJECT
public:
    explicit PaletteChangeFilter(QObject* parent = nullptr);

public slots:
    void onSystemThemeChanged();
#ifdef HAVE_QT_DBUS
    void onPortalSettingChanged(const QString& nameSpace, const QString& key, const QDBusVariant& /*value*/);
#endif

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
};

#endif // PALETTECHANGEFILTER_H
