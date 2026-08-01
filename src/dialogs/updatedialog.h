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
#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include <QDialog>
#include <QString>

class QLabel;
class QPushButton;

class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Mode
    {
        UpToDate,
        UpdateAvailable,
        CheckFailed
    };

    static void showUpToDate(QWidget* parent, const QString& latestVersion);
    static void showUpdateAvailable(QWidget* parent, const QString& latestVersion, const QString& releaseUrl);
    static void showCheckFailed(QWidget* parent, const QString& error);

private:
    UpdateDialog(Mode mode, const QString& version, const QString& releaseUrl, QWidget* parent = nullptr);

    void setupUi();
    void downloadUpdate();

    Mode m_mode;
    QString m_version;
    QString m_releaseUrl;
};

#endif // UPDATEDIALOG_H
