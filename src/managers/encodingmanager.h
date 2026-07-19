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
#ifndef ENCODINGMANAGER_H
#define ENCODINGMANAGER_H

#include <QMenu>
#include <QObject>
#include <QString>

class CodeEditor;
class QComboBox;

class EncodingManager : public QObject
{
    Q_OBJECT

public:
    explicit EncodingManager(QObject* parent = nullptr);

    void populateEncodingMenu(QMenu* menu, bool isReopen);

    void updateEncodingSelector(QComboBox* comboBox, CodeEditor* editor);

signals:
    void encodingSelected(const QString& encoding, bool reopen);

private:
    void connectEncodingSelector(QComboBox* comboBox);
};

#endif
