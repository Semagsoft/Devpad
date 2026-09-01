/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * PrimoHighlighter: syntax highlight bridge for QSG primoEditor.
 * Wraps TuiHighlighter (all languages) + ThemeColors → QTextLayout::FormatRange.
 * Async worker for >50MB files.
 */

#ifndef PRIMOHIGHLIGHTER_H
#define PRIMOHIGHLIGHTER_H

#include "theme.h"

#include <QObject>
#include <QTextLayout>
#include <QThread>
#include <QVector>

struct PrimoToken
{
    int start = 0;
    int length = 0;
    QColor color;
};

class PrimoHighlighter : public QObject
{
    Q_OBJECT
public:
    explicit PrimoHighlighter(QObject* parent = nullptr) : QObject(parent)
    {
    }

    static QVector<QTextLayout::FormatRange> formatsForLine(const QString& line, const QString& language, const ThemeColors& colors);
    static QColor colorForKind(int kind, const ThemeColors& colors);

    // Async API: highlight chunk, returns via signal
    void requestHighlight(int version, int firstLine, const QStringList& lines, const QString& language, const ThemeColors& colors);

signals:
    void highlighted(int version, int firstLine, QVector<QVector<QTextLayout::FormatRange>> formats);

private:
    static ThemeColors fallbackTheme();
};

// Worker lives in background thread
class PrimoHighlighterWorker : public QObject
{
    Q_OBJECT
public slots:
    void doHighlight(int version, int firstLine, QStringList lines, QString language, ThemeColors colors);
signals:
    void done(int version, int firstLine, QVector<QVector<QTextLayout::FormatRange>> formats);
};

class PrimoHighlighterController : public QObject
{
    Q_OBJECT
public:
    explicit PrimoHighlighterController(QObject* parent = nullptr);
    ~PrimoHighlighterController() override;

    void request(int version, int firstLine, const QStringList& lines, const QString& language, const ThemeColors& colors);
    void cancelAll();

signals:
    void highlighted(int version, int firstLine, QVector<QVector<QTextLayout::FormatRange>> formats);
    void requestWorker(int version, int firstLine, QStringList lines, QString language, ThemeColors colors);

private:
    QThread* m_thread = nullptr;
    PrimoHighlighterWorker* m_worker = nullptr;
    int m_lastVersion = -1;
};

#endif // PRIMOHIGHLIGHTER_H
