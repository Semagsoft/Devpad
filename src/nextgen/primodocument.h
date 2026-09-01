/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * PrimoDocument: high-perf document model (piece-table-ish).
 * Optimized for QSG visible-range culling: line offsets cache.
 */

#ifndef PRIMODOCUMENT_H
#define PRIMODOCUMENT_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QUndoStack>
#include <QVector>

class PrimoDocument : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY textChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY canUndoChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY canRedoChanged)
    Q_PROPERTY(bool undoDisabled READ isUndoDisabled NOTIFY undoDisabledChanged)

public:
    explicit PrimoDocument(QObject* parent = nullptr);
    PrimoDocument(const QString& filePath, const QString& text, const QString& language, QObject* parent = nullptr);

    QString text() const;
    void setText(const QString& text);

    QString filePath() const;
    void setFilePath(const QString& path);

    QString language() const;
    void setLanguage(const QString& lang);

    bool isModified() const;
    void setModified(bool m);

    int lineCount() const;
    QStringList lines() const;

    Q_INVOKABLE QString lineAt(int line) const;
    Q_INVOKABLE void setLine(int line, const QString& content);
    Q_INVOKABLE void insertText(int position, const QString& t);
    Q_INVOKABLE void removeText(int position, int length);
    Q_INVOKABLE int length() const;
    Q_INVOKABLE int lineStartOffset(int line) const;
    Q_INVOKABLE QStringList visibleLines(int firstLine, int count) const;

    bool loadFromFile(const QString& path, QString* error = nullptr);
    bool saveToFile(const QString& path, QString* error = nullptr) const;

    // Undo/Redo (incremental disabled >50MB)
    bool canUndo() const;
    bool canRedo() const;
    bool isUndoDisabled() const;
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void clearUndoStack();
    void setUndoDisabled(bool d);

signals:
    void textChanged();
    void filePathChanged();
    void languageChanged();
    void modifiedChanged();
    void canUndoChanged();
    void canRedoChanged();
    void undoDisabledChanged();

private:
    void rebuildOffsets() const;
    void invalidateCache();
    void pushUndo(const QString& oldText, const QString& newText);

    QString m_filePath;
    QString m_text;
    QString m_language;
    bool m_modified = false;
    bool m_undoDisabled = false;

    mutable QVector<int> m_lineOffsets; // start offset per line
    mutable bool m_offsetsDirty = true;
    mutable QStringList m_linesCache;
    mutable bool m_linesDirty = true;

    QUndoStack* m_undoStack = nullptr;
    bool m_inUndoRedo = false;
};

#endif // PRIMODOCUMENT_H
