/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * PrimoDocument: in-house document model for next-gen editor.
 * High-perf piece-table / rope backing for primoEditor.
 * MVP wraps QString for correctness; future: incremental rope + gap buffer.
 */

#ifndef PRIMODOCUMENT_H
#define PRIMODOCUMENT_H

#include <QObject>
#include <QString>
#include <QStringList>

class PrimoDocument : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY textChanged)

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

    bool loadFromFile(const QString& path, QString* error = nullptr);
    bool saveToFile(const QString& path, QString* error = nullptr) const;

signals:
    void textChanged();
    void filePathChanged();
    void languageChanged();
    void modifiedChanged();

private:
    QString m_filePath;
    QString m_text;
    QString m_language;
    bool m_modified = false;
};

#endif // PRIMODOCUMENT_H
