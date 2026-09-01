/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "primoeditor.h"

#include "managers/settingsmanager.h"

#include <QFileInfo>

PrimoEditor::PrimoEditor(QObject* parent) : QObject(parent)
{
    m_doc = new PrimoDocument(this);
    connect(m_doc, &PrimoDocument::textChanged, this, &PrimoEditor::textChanged);
    connect(m_doc, &PrimoDocument::filePathChanged, this, &PrimoEditor::filePathChanged);
    connect(m_doc, &PrimoDocument::languageChanged, this, &PrimoEditor::languageChanged);

    // Apply default font from settings when available
    m_font = SettingsManager::instance().defaultFont();
    m_tabWidth = SettingsManager::instance().tabWidth();
    m_wordWrap = SettingsManager::instance().wordWrap();
}

QString PrimoEditor::filePath() const { return m_doc ? m_doc->filePath() : QString(); }

void PrimoEditor::setFilePath(const QString& path)
{
    if (m_doc)
        m_doc->setFilePath(path);
}

QString PrimoEditor::text() const { return m_doc ? m_doc->text() : QString(); }

void PrimoEditor::setText(const QString& t)
{
    if (m_doc)
        m_doc->setText(t);
}

QString PrimoEditor::language() const { return m_doc ? m_doc->language() : QString(); }

void PrimoEditor::setLanguage(const QString& lang)
{
    if (m_doc)
        m_doc->setLanguage(lang);
}

QString PrimoEditor::encoding() const { return m_encoding; }

void PrimoEditor::setEncoding(const QString& enc)
{
    if (m_encoding == enc)
        return;
    m_encoding = enc;
    emit encodingChanged();
}

QFont PrimoEditor::font() const { return m_font; }

void PrimoEditor::setFont(const QFont& f)
{
    if (m_font == f)
        return;
    m_font = f;
    emit fontChanged();
}

int PrimoEditor::tabWidth() const { return m_tabWidth; }

void PrimoEditor::setTabWidth(int w)
{
    if (m_tabWidth == w)
        return;
    m_tabWidth = w;
    emit tabWidthChanged();
}

bool PrimoEditor::wordWrap() const { return m_wordWrap; }

void PrimoEditor::setWordWrap(bool w)
{
    if (m_wordWrap == w)
        return;
    m_wordWrap = w;
    emit wordWrapChanged();
}

int PrimoEditor::cursorLine() const { return m_cursorLine; }

void PrimoEditor::setCursorLine(int line)
{
    if (m_cursorLine == line)
        return;
    m_cursorLine = line;
    emit cursorChanged();
}

int PrimoEditor::cursorColumn() const { return m_cursorColumn; }

void PrimoEditor::setCursorColumn(int col)
{
    if (m_cursorColumn == col)
        return;
    m_cursorColumn = col;
    emit cursorChanged();
}

PrimoDocument* PrimoEditor::document() const { return m_doc; }

void PrimoEditor::loadFile(const QString& path)
{
    if (!m_doc)
        return;
    QString error;
    bool ok = m_doc->loadFromFile(path, &error);
    if (ok)
    {
        QString lang = SettingsManager::instance().syntaxForFile(path);
        m_doc->setLanguage(lang);
        QFileInfo fi(path);
        emit fileLoaded(true, QString());
        Q_UNUSED(fi);
    }
    else
    {
        emit fileLoaded(false, error);
    }
}

bool PrimoEditor::save()
{
    if (!m_doc || m_doc->filePath().isEmpty())
        return false;
    QString error;
    bool ok = m_doc->saveToFile(m_doc->filePath(), &error);
    if (ok)
        m_doc->setModified(false);
    emit fileSaved(ok, error);
    return ok;
}

bool PrimoEditor::saveAs(const QString& path)
{
    if (!m_doc)
        return false;
    QString error;
    bool ok = m_doc->saveToFile(path, &error);
    if (ok)
    {
        m_doc->setFilePath(path);
        m_doc->setModified(false);
    }
    emit fileSaved(ok, error);
    return ok;
}

void PrimoEditor::setCursorPosition(int line, int column)
{
    bool changed = false;
    if (m_cursorLine != line)
    {
        m_cursorLine = line;
        changed = true;
    }
    if (m_cursorColumn != column)
    {
        m_cursorColumn = column;
        changed = true;
    }
    if (changed)
        emit cursorChanged();
}
