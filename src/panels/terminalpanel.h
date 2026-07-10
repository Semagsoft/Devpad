/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef TERMINALPANEL_H
#define TERMINALPANEL_H

#include <QDockWidget>
#include <QVBoxLayout>
#include <QWidget>

class QMainWindow;
class QTabWidget;

enum class ThemeId : int;
enum class TerminalPanelPosition : int;

class TerminalBackend;

class TerminalPanel : public QDockWidget {
    Q_OBJECT

public:
    explicit TerminalPanel(QWidget *parent = nullptr);
    ~TerminalPanel() override;

    void setWorkingDirectory(const QString &path);
    QString workingDirectory() const;

    void sendCommand(const QString &command);
    void refreshTheme();
    void toggle(QTabWidget *tabWidget, QMainWindow *mainWindow);
    void applyPosition(TerminalPanelPosition pos, QTabWidget *tabWidget, QMainWindow *mainWindow);

signals:
    void terminalStarted();
    void terminalStopped();
    void sessionExited();

private slots:
    void onSessionFinished();

protected:
    void showEvent(QShowEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void extractColorSchemes();
    void setupUI();
    void ensureTerminalWidget();
    void startTerminal();
    void stopTerminal();
    QString themeToColorScheme(ThemeId themeId) const;

    QWidget *panelWidget = nullptr;
    QVBoxLayout *mainLayout = nullptr;
    TerminalBackend *m_backend = nullptr;

    QString m_workingDirectory;
    bool m_isRunning;
};

#endif
