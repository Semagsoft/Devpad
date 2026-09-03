/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * NextgenTabModel: draggable tabs between panes + split view.
 * Simple QStringList model with move support, persistent via SettingsManager? For MVP, in-memory.
 */

#ifndef NEXTGENTABMODEL_H
#define NEXTGENTABMODEL_H

#include <QObject>
#include <QStringList>

class NextgenTabModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList tabs READ tabs NOTIFY tabsChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int pane2Index READ pane2Index WRITE setPane2Index NOTIFY pane2IndexChanged)
    Q_PROPERTY(bool splitVisible READ splitVisible WRITE setSplitVisible NOTIFY splitVisibleChanged)
    Q_PROPERTY(int count READ count NOTIFY tabsChanged)

public:
    explicit NextgenTabModel(QObject* parent = nullptr);

    QStringList tabs() const;
    int currentIndex() const;
    void setCurrentIndex(int idx);
    int pane2Index() const;
    void setPane2Index(int idx);
    bool splitVisible() const;
    void setSplitVisible(bool v);
    int count() const;

    Q_INVOKABLE QString tabAt(int idx) const;
    Q_INVOKABLE QString displayName(int idx) const;
    Q_INVOKABLE void addTab(const QString& filePath);
    Q_INVOKABLE void closeTab(int idx);
    Q_INVOKABLE void moveTab(int from, int to);
    Q_INVOKABLE void moveTabToPane(int fromPane, int fromIdx, int toPane, int toIdx);
    Q_INVOKABLE QString currentFile() const;
    Q_INVOKABLE QString pane2File() const;
    Q_INVOKABLE void setCurrentFile(const QString& path);
    Q_INVOKABLE void detachTab(int idx);

signals:
    void tabsChanged();
    void currentIndexChanged();
    void pane2IndexChanged();
    void splitVisibleChanged();
    void tabClosed(int idx);
    void tabMoved(int from, int to);

private:
    QStringList m_tabs;
    int m_current = 0;
    int m_pane2Current = 0;
    bool m_splitVisible = false;
};

#endif // NEXTGENTABMODEL_H
