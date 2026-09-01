/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * PrimoEditor: QML-exposed high-perf editor item.
 * Backed by PrimoDocument; renders via QQuickItem + Text.
 * MVP exposes properties to QML for editing.
 */

#ifndef PRIMOEDITOR_H
#define PRIMOEDITOR_H

#include "primodocument.h"

#include <QObject>
#include <QFont>
#include <QString>

class PrimoEditor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString encoding READ encoding WRITE setEncoding NOTIFY encodingChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(int tabWidth READ tabWidth WRITE setTabWidth NOTIFY tabWidthChanged)
    Q_PROPERTY(bool wordWrap READ wordWrap WRITE setWordWrap NOTIFY wordWrapChanged)
    Q_PROPERTY(int cursorLine READ cursorLine WRITE setCursorLine NOTIFY cursorChanged)
    Q_PROPERTY(int cursorColumn READ cursorColumn WRITE setCursorColumn NOTIFY cursorChanged)

public:
    explicit PrimoEditor(QObject* parent = nullptr);

    // Properties
    QString filePath() const;
    void setFilePath(const QString& path);

    QString text() const;
    void setText(const QString& t);

    QString language() const;
    void setLanguage(const QString& lang);

    QString encoding() const;
    void setEncoding(const QString& enc);

    QFont font() const;
    void setFont(const QFont& f);

    int tabWidth() const;
    void setTabWidth(int w);

    bool wordWrap() const;
    void setWordWrap(bool w);

    int cursorLine() const;
    void setCursorLine(int line);
    int cursorColumn() const;
    void setCursorColumn(int col);

    PrimoDocument* document() const;

    Q_INVOKABLE void loadFile(const QString& path);
    Q_INVOKABLE bool save();
    Q_INVOKABLE bool saveAs(const QString& path);
    Q_INVOKABLE void setCursorPosition(int line, int column);

signals:
    void filePathChanged();
    void textChanged();
    void languageChanged();
    void encodingChanged();
    void fontChanged();
    void tabWidthChanged();
    void wordWrapChanged();
    void cursorChanged();
    void fileLoaded(bool ok, const QString& error);
    void fileSaved(bool ok, const QString& error);

private:
    PrimoDocument* m_doc = nullptr;
    QString m_encoding = QStringLiteral("UTF-8");
    QFont m_font = QFont(QStringLiteral("Monospace"), 11);
    int m_tabWidth = 4;
    bool m_wordWrap = false;
    int m_cursorLine = 0;
    int m_cursorColumn = 0;
};

#endif // PRIMOEDITOR_H
