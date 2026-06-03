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
#include "mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("Semagsoft");
    app.setOrganizationDomain("semagsoft.com");
    app.setApplicationName("Devpad");
    app.setApplicationVersion("1.0.0");

    MainWindow mainWindow;

    for (int i = 1; i < argc; ++i)
    {
        QString arg = QString::fromLocal8Bit(argv[i]);

        if (arg == "--transfer" && i + 1 < argc)
        {
            QString transferPath = QString::fromLocal8Bit(argv[++i]);
            mainWindow.openTransferFile(transferPath);
            continue;
        }

        QString filePath = arg;
        if (!QFileInfo(filePath).isAbsolute())
            filePath = QDir::current().absoluteFilePath(filePath);

        QFileInfo fileInfo(filePath);
        if (fileInfo.isFile())
            mainWindow.openFileFromPath(filePath);
        else if (fileInfo.isDir())
            mainWindow.openFolderFromPath(filePath);
    }

    mainWindow.show();
    return app.exec();
}
