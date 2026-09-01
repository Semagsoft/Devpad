/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "primodocument.h"

#include "core/fileservice.h"

#include <QFileInfo>

PrimoDocument::PrimoDocument(QObject* parent) : QObject(parent) {}

PrimoDocument::PrimoDocument(const QString& filePath, const QString& text, const QString& language, QObject* parent)
    : QObject(parent), m_filePath(filePath), m_text(text), m_language(language)
{
}

QString PrimoDocument::text() const { return m_text; }

void PrimoDocument::setText(const QString& text)
{
    if (m_text == text)
        return;
    m_text = text;
    m_modified = true;
    emit textChanged();
    emit modifiedChanged();
}

QString PrimoDocument::filePath() const { return m_filePath; }

void PrimoDocument::setFilePath(const QString& path)
{
    if (m_filePath == path)
        return;
    m_filePath = path;
    emit filePathChanged();
}

QString PrimoDocument::language() const { return m_language; }

void PrimoDocument::setLanguage(const QString& lang)
{
    if (m_language == lang)
        return;
    m_language = lang;
    emit languageChanged();
}

bool PrimoDocument::isModified() const { return m_modified; }

void PrimoDocument::setModified(bool m)
{
    if (m_modified == m)
        return;
    m_modified = m;
    emit modifiedChanged();
}

int PrimoDocument::lineCount() const
{
    if (m_text.isEmpty())
        return 1;
    return m_text.count(QLatin1Char('\n')) + 1;
}

QStringList PrimoDocument::lines() const { return m_text.split(QLatin1Char('\n')); }

QString PrimoDocument::lineAt(int line) const
{
    QStringList ls = lines();
    if (line < 0 || line >= ls.size())
        return {};
    return ls.at(line);
}

void PrimoDocument::setLine(int line, const QString& content)
{
    QStringList ls = lines();
    if (line < 0 || line >= ls.size())
        return;
    ls[line] = content;
    setText(ls.join(QLatin1Char('\n')));
}

void PrimoDocument::insertText(int position, const QString& t)
{
    if (position < 0)
        position = 0;
    if (position > m_text.size())
        position = m_text.size();
    m_text.insert(position, t);
    m_modified = true;
    emit textChanged();
    emit modifiedChanged();
}

void PrimoDocument::removeText(int position, int length)
{
    if (position < 0 || length <= 0 || position >= m_text.size())
        return;
    m_text.remove(position, length);
    m_modified = true;
    emit textChanged();
    emit modifiedChanged();
}

int PrimoDocument::length() const { return m_text.size(); }

bool PrimoDocument::loadFromFile(const QString& path, QString* error)
{
    FileLoadResult res = FileService::load(path);
    if (!res.ok)
    {
        if (error)
            *error = res.error;
        return false;
    }
    m_filePath = path;
    m_text = res.text;
    m_modified = false;
    emit filePathChanged();
    emit textChanged();
    emit modifiedChanged();
    return true;
}

bool PrimoDocument::saveToFile(const QString& path, QString* error) const
{
    QString err;
    bool ok = FileService::save(path, m_text, QStringLiteral("UTF-8"), &err);
    if (!ok && error)
        *error = err;
    return ok;
}
