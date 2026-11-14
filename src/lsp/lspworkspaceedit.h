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
#ifndef LSPWORKSPACEEDIT_H
#define LSPWORKSPACEEDIT_H

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

namespace lsp
{

// Apply LSP TextEdits to plain text. Edits refer to the original document;
// they are applied bottom-most first so earlier ranges stay valid. Returns
// the modified text; *ok is set to false and *error to a message on
// structural problems (e.g. edits outside the document).
QString applyTextEditsToText(const QString& text, const QList<QJsonObject>& edits, bool* ok, QString* error = nullptr);

// Apply LSP TextEdits to the file on disk. Writes atomically. The file is
// decoded as UTF-8. Returns true on success.
bool applyTextEditsToFile(const QString& filePath, const QList<QJsonObject>& edits, QString* error = nullptr);

} // namespace lsp

#endif // LSPWORKSPACEEDIT_H
