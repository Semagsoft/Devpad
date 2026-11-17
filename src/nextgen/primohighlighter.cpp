/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "primohighlighter.h"

#include "theme.h"
#include "tui/tuihighlighter.h"

#include <QDebug>
#include <QTextCharFormat>
#include <memory>

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
    // Cooperative cancellation: skip when a newer version was requested.
    auto token = m_cancelToken;
    if (!token || version != token->load(std::memory_order_acquire))
        return;

    QVector<QVector<QTextLayout::FormatRange>> res;
    res.reserve(lines.size());
    for (int i = 0; i < lines.size(); ++i)
    {
        // Cooperative cancellation point every 500 lines
        if (i % 500 == 0 && token->load(std::memory_order_acquire) != version)
            return;
        res.append(PrimoHighlighter::formatsForLine(lines.at(i), language, colors));
        // Yield for >50MB large files: tiny sleep to keep UI responsive
        if (i % 1000 == 0)
            QThread::yieldCurrentThread();
    }
    if (token->load(std::memory_order_acquire) != version)
        return;
    emit done(version, firstLine, res);
}

PrimoHighlighterController::PrimoHighlighterController(QObject* parent) : QObject(parent)
{
    // Shared cancellation token: set to the requested version on every
    // request; the worker bails out early (or drops its result) when it
    // observes a newer version. The thread runs until destruction — never
    // quit()/start() mid-flight (that is undefined behavior).
    m_cancelToken = std::make_shared<std::atomic<int>>(-1);
    m_thread = new QThread(this);
    m_worker = new PrimoHighlighterWorker;
    m_worker->setCancelToken(m_cancelToken);
    m_worker->moveToThread(m_thread);
    connect(this, &PrimoHighlighterController::requestWorker, m_worker, &PrimoHighlighterWorker::doHighlight);
    connect(m_worker, &PrimoHighlighterWorker::done, this, &PrimoHighlighterController::highlighted, Qt::QueuedConnection);
    m_thread->start(QThread::LowPriority);
}

PrimoHighlighterController::~PrimoHighlighterController()
{
    if (m_cancelToken)
        m_cancelToken->store(-2, std::memory_order_release); // cancel everything
    m_thread->quit();
    m_thread->wait();
    delete m_worker;
}

void PrimoHighlighterController::request(int version, int firstLine, const QStringList& lines, const QString& language, const ThemeColors& colors)
{
    if (m_cancelToken)
        m_cancelToken->store(version, std::memory_order_release);
    m_lastVersion = version;
    emit requestWorker(version, firstLine, lines, language, colors);
}

void PrimoHighlighterController::cancelAll()
{
    if (m_cancelToken)
        m_cancelToken->store(-2, std::memory_order_release);
}
