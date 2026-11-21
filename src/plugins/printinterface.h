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

#ifndef PRINTINTERFACE_H
#define PRINTINTERFACE_H

class QObject;
class QPrinter;

/**
 * Service exported by the Print plugin and consumed by MainWindow. It is a
 * plain C++ interface (no Q_OBJECT) so the plugin does not need Devpad's
 * meta-object layout; the host obtains it from the plugin object registry and
 * uses dynamic_cast.
 */
class IPrintService
{
public:
    virtual ~IPrintService() = default;
    virtual void printEditor(QObject* editor, QPrinter* printer) = 0;
};

#define DEVPAD_PRINT_SERVICE_OBJECT "print_service"

#endif // PRINTINTERFACE_H
