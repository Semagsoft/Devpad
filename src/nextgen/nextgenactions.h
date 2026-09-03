/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * NextgenActions: QML bridge for Menu/Toolbar/StatusBar.
 * Persists showToolbar/showStatusbar via SettingsManager (shared with Widgets).
 * Exposes recent files, actions, encoding.
 */

#ifndef NEXTGENACTIONS_H
#define NEXTGENACTIONS_H

#include "theme.h"

#include <QObject>
#include <QStringList>

class PrimoEditor;

class NextgenActions : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool showToolbar READ showToolbar WRITE setShowToolbar NOTIFY showToolbarChanged)
    Q_PROPERTY(bool showStatusbar READ showStatusbar WRITE setShowStatusbar NOTIFY showStatusbarChanged)
    Q_PROPERTY(bool showMenuBar READ showMenuBar WRITE setShowMenuBar NOTIFY showMenuBarChanged)
    Q_PROPERTY(QStringList recentFiles READ recentFiles NOTIFY recentFilesChanged)
    Q_PROPERTY(QStringList recentFolders READ recentFolders NOTIFY recentFilesChanged)
    Q_PROPERTY(QString currentEncoding READ currentEncoding NOTIFY currentEncodingChanged)
    Q_PROPERTY(QObject* editor READ editorObj WRITE setEditorObj NOTIFY editorChanged)

public:
    explicit NextgenActions(QObject* parent = nullptr);
    explicit NextgenActions(PrimoEditor* editor, QObject* parent = nullptr);

    bool showToolbar() const;
    void setShowToolbar(bool v);
    bool showStatusbar() const;
    void setShowStatusbar(bool v);
    bool showMenuBar() const;
    void setShowMenuBar(bool v);

    QStringList recentFiles() const;
    QStringList recentFolders() const;
    QString currentEncoding() const;

    QStringList themeNames() const;
    QString currentThemeName() const;
    Q_INVOKABLE void setThemeByName(const QString& name);
    Q_INVOKABLE void setThemeById(int id);

    Q_INVOKABLE void refreshRecent();
    Q_INVOKABLE void clearRecentFiles();

    QObject* editorObj() const;
    void setEditorObj(QObject* obj);

public slots:
    void newFile();
    void openFile(const QString& path = QString());
    void openFileDialog();
    void openFolderDialog();
    void saveFile();
    void saveFileAs();
    void saveFileAsDialog();
    void closeFile();
    void exitApp();
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();
    void find();
    void replace();
    void findNext();
    void findPrevious();
    void goToLine();
    void toggleBookmark();
    void nextBookmark();
    void prevBookmark();
    void clearBookmarks();
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void toggleFullScreen();
    void toggleWordWrap();
    void showAbout();
    void showOptions();
    void reopenWithEncoding(const QString& enc);
    void saveWithEncoding(const QString& enc);
    void openRecentFile(const QString& path);
    void toggleTerminal();
    void findInFiles(const QString& pattern = QString());

signals:
    void showToolbarChanged();
    void showStatusbarChanged();
    void showMenuBarChanged();
    void recentFilesChanged();
    void currentEncodingChanged();
    void editorChanged();
    void themeChanged();
    void requestOpenFileDialog();
    void requestOpenFolderDialog();
    void requestSaveAsDialog();
    void requestGoToLineDialog();
    void requestFindDialog();
    void requestReplaceDialog();
    void requestAboutDialog();
    void requestOptionsDialog();
    void requestTerminalToggle();
    void requestFindInFiles();
    void showMessage(const QString& msg);

private:
    PrimoEditor* m_editor = nullptr;
};

#endif // NEXTGENACTIONS_H
