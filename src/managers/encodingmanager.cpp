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
#include "encodingmanager.h"

#include "appstrings.h"
#include "codeeditor.h"
#include "encodingutils.h"

#include <QAction>
#include <QComboBox>
#include <QMessageBox>

EncodingManager::EncodingManager(QObject* parent) : QObject(parent)
{
}

void EncodingManager::populateEncodingMenu(QMenu* menu, bool isReopen)
{
    menu->clear();
    for (const auto& info : supportedEncodings())
    {
        QAction* act = new QAction(info.displayName, menu);
        act->setData(info.displayName);
        connect(act, &QAction::triggered, this, [this, info, isReopen]() { emit encodingSelected(info.displayName, isReopen); });
        menu->addAction(act);
    }
}

void EncodingManager::updateEncodingSelector(QComboBox* comboBox, CodeEditor* editor)
{
    comboBox->blockSignals(true);
    comboBox->clear();
    if (editor)
    {
        const QString& currentEncoding = editor->encoding();
        for (const auto& info : supportedEncodings())
        {
            comboBox->addItem(info.displayName);
            if (info.displayName == currentEncoding)
            {
                comboBox->setCurrentIndex(comboBox->count() - 1);
            }
        }
    }
    comboBox->blockSignals(false);
    connectEncodingSelector(comboBox);
}

void EncodingManager::connectEncodingSelector(QComboBox* comboBox)
{
    disconnect(comboBox, nullptr, this, nullptr);
    connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this, comboBox]()
            {
                int index = comboBox->currentIndex();
                if (index < 0)
                    return;
                QString encoding = comboBox->itemText(index);
                emit encodingSelected(encoding, false);
            });
}
