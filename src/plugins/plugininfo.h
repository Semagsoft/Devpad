/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef PLUGININFO_H
#define PLUGININFO_H

#include <QString>
#include <QStringList>

/**
 * Static metadata describing a Devpad plugin. Populated from the
 * <id>.plugin.json manifest next to the plugin library and exposed via
 * IPlugin::info().
 */
struct PluginInfo
{
    QString id;      // reverse-domain id, e.g. "org.example.todopanel"
    QString name;    // human-readable name
    QString version; // plugin version, e.g. "1.0.0"
    QString author;
    QString description;
    int apiVersion = 1;  // Devpad plugin API version the plugin targets
    QString library;     // basename of the shared library, e.g. "libtodopanel.so"
    QStringList depends; // plugin ids this plugin requires to be loaded first
    bool enabled = true; // default enabled state (manifest default, overridden by user preference)
};

#endif // PLUGININFO_H
