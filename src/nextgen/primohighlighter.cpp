/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "primohighlighter.h"

#include "theme.h"
#include "tui/tuihighlighter.h"

#include <QDebug>
#include <QTextCharFormat>

QColor PrimoHighlighter::colorForKind(int kind, const ThemeColors& colors)
{
    switch (static_cast<HighlightKind>(kind))
    {
    case HighlightKind::Keyword:
        return colors.keyword;
    case HighlightKind::String:
        return colors.string;
    case HighlightKind::Comment:
        return colors.comment;
    case HighlightKind::Number:
        return colors.number;
    case HighlightKind::Preprocessor:
        return colors.preprocessor;
    default:
        return colors.foreground;
    }
}

ThemeColors PrimoHighlighter::fallbackTheme()
{
    return getThemeColors(ThemeId::Dark);
}

QVector<QTextLayout::FormatRange> PrimoHighlighter::formatsForLine(const QString& line, const QString& language, const ThemeColors& colors)
{
    if (line.isEmpty() || language.isEmpty())
        return {};

    auto segs = TuiHighlighter::highlightLine(line, language);
    QVector<QTextLayout::FormatRange> out;
    out.reserve(segs.size());
    for (auto& s : segs)
    {
        QTextLayout::FormatRange fr;
        fr.start = s.start;
        fr.length = s.length;
        QTextCharFormat fmt;
        fmt.setForeground(colorForKind(static_cast<int>(s.kind), colors));
        // subtle: keywords bold-ish via weight
        if (s.kind == HighlightKind::Keyword)
        {
            QFont f;
            f.setWeight(QFont::DemiBold);
            fmt.setFont(f);
        }
        fr.format = fmt;
        out.append(fr);
    }
    return out;
}

void PrimoHighlighter::requestHighlight(int version, int firstLine, const QStringList& lines, const QString& language, const ThemeColors& colors)
{
    // Synchronous fallback (used for visible first chunk)
    Q_UNUSED(version);
    Q_UNUSED(firstLine);
    Q_UNUSED(lines);
    Q_UNUSED(language);
    Q_UNUSED(colors);
}

void PrimoHighlighterWorker::doHighlight(int version, int firstLine, QStringList lines, QString language, ThemeColors colors)
{
    QVector<QVector<QTextLayout::FormatRange>> res;
    res.reserve(lines.size());
    for (int i = 0; i < lines.size(); ++i)
    {
        // Cooperative cancellation point every 500 lines
        if (i % 500 == 0 && QThread::currentThread()->isInterruptionRequested())
            return;
        res.append(PrimoHighlighter::formatsForLine(lines.at(i), language, colors));
        // Yield for >50MB large files: tiny sleep to keep UI responsive
        if (i % 1000 == 0)
            QThread::yieldCurrentThread();
    }
    emit done(version, firstLine, res);
}

PrimoHighlighterController::PrimoHighlighterController(QObject* parent) : QObject(parent)
{
    m_thread = new QThread(this);
    m_worker = new PrimoHighlighterWorker;
    m_worker->moveToThread(m_thread);
    connect(this, &PrimoHighlighterController::requestWorker, m_worker, &PrimoHighlighterWorker::doHighlight);
    connect(m_worker, &PrimoHighlighterWorker::done, this, &PrimoHighlighterController::highlighted, Qt::QueuedConnection);
    m_thread->start(QThread::LowPriority);
}

PrimoHighlighterController::~PrimoHighlighterController()
{
    m_thread->requestInterruption();
    m_thread->quit();
    m_thread->wait(2000);
    delete m_worker;
}

void PrimoHighlighterController::request(int version, int firstLine, const QStringList& lines, const QString& language, const ThemeColors& colors)
{
    if (m_lastVersion != -1 && version != m_lastVersion)
    {
        m_thread->requestInterruption();
        // restart thread? Instead just enqueue new version; worker will early return on interruption check at next chunk.
        m_thread->quit();
        m_thread->wait(300);
        m_thread->start(QThread::LowPriority);
    }
    m_lastVersion = version;
    emit requestWorker(version, firstLine, lines, language, colors);
}

void PrimoHighlighterController::cancelAll()
{
    m_thread->requestInterruption();
}
