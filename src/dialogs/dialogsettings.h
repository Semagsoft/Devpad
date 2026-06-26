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
#ifndef DIALOGSETTINGS_H
#define DIALOGSETTINGS_H

#include <QByteArray>
#include <QDialog>
#include <QSettings>
#include <QString>
#include <QVariant>

class DialogSettings {
public:
    explicit DialogSettings(const QString &prefix) : m_prefix(prefix) {}

    void save(const QString &key, const QVariant &value) const {
        m_settings.setValue(m_prefix + "_" + key, value);
    }

    QVariant load(const QString &key, const QVariant &defaultValue = QVariant()) const {
        return m_settings.value(m_prefix + "_" + key, defaultValue);
    }

    void saveGeometry(QDialog *dialog) const {
        save("Geometry", dialog->saveGeometry());
    }

    void restoreGeometry(QDialog *dialog) const {
        QByteArray geo = load("Geometry").toByteArray();
        if (!geo.isEmpty())
            dialog->restoreGeometry(geo);
    }

private:
    QString m_prefix;
    mutable QSettings m_settings;
};

#endif
