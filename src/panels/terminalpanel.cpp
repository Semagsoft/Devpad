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
#include "terminalpanel.h"
#include "settingsmanager.h"
#include "theme.h"
#include <qtermwidget.h>
#include <QApplication>
#include <QDir>
#include <QPointer>
#include <QMainWindow>
#include <QStandardPaths>
#include <QTabWidget>
#include <QFile>
#include <QResource>

TerminalPanel::TerminalPanel(QWidget *parent)
    : QDockWidget(tr("Terminal"), parent),
      terminalWidget(nullptr),
      m_isRunning(false) {
    setObjectName("TerminalPanel");
    setFeatures(QDockWidget::DockWidgetFeature::DockWidgetMovable |
                QDockWidget::DockWidgetFeature::DockWidgetFloatable |
                QDockWidget::DockWidgetFeature::DockWidgetClosable);
    setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    setTitleBarWidget(new QWidget(this));

    extractColorSchemes();

    setupUI();
}

TerminalPanel::~TerminalPanel() = default;

void TerminalPanel::extractColorSchemes() {
    QString schemeDirPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/devpad/color-schemes";
    QDir colorSchemeDir(schemeDirPath);
    if (!colorSchemeDir.exists()) {
        colorSchemeDir.mkpath(".");
    }

    QStringList schemeFiles = {
        "DevpadLight.colorscheme",
        "DevpadDark.colorscheme",
        "DevpadNord.colorscheme",
        "DevpadSolarizedLight.colorscheme",
        "DevpadMonokai.colorscheme",
        "DevpadGruvboxDark.colorscheme",
    };

    for (const QString &fileName : schemeFiles) {
        QString resourcePath = ":/color-schemes/" + fileName;
        QString destPath = colorSchemeDir.absoluteFilePath(fileName);

        if (!QFile::exists(destPath)) {
            if (!QFile::copy(resourcePath, destPath)) {
                qWarning("Failed to copy color scheme: %s", qPrintable(resourcePath));
            }
        }
    }

    QTermWidget::addCustomColorSchemeDir(colorSchemeDir.absolutePath());
}

void TerminalPanel::setupUI() {
    panelWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(panelWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    setWidget(panelWidget);
}

void TerminalPanel::showEvent(QShowEvent *event) {
    QDockWidget::showEvent(event);
    if (terminalWidget && !m_isRunning) {
        startTerminal();
    } else if (!terminalWidget) {
        startTerminal();
    }
}

QString TerminalPanel::themeToColorScheme(ThemeId themeId) const {
    switch (themeId) {
        case ThemeId::Light:
            return "DevpadLight";
        case ThemeId::Dark:
            return "DevpadDark";
        case ThemeId::Nord:
            return "DevpadNord";
        case ThemeId::SolarizedLight:
            return "DevpadSolarizedLight";
        case ThemeId::Monokai:
            return "DevpadMonokai";
        case ThemeId::GruvboxDark:
            return "DevpadGruvboxDark";
        case ThemeId::System:
            return SettingsManager::instance().isDarkTheme() ? "DevpadDark" : "DevpadLight";
    }
    return "DevpadDark";
}

void TerminalPanel::ensureTerminalWidget() {
    if (terminalWidget) return;

    terminalWidget = new QTermWidget(0, panelWidget);

    terminalWidget->setHistorySize(-1);
    terminalWidget->setScrollBarPosition(QTermWidgetInterface::ScrollBarRight);
    terminalWidget->setTerminalFont(SettingsManager::instance().terminalFont());

    if (!m_workingDirectory.isEmpty() && QDir(m_workingDirectory).exists()) {
        terminalWidget->setWorkingDirectory(m_workingDirectory);
    }

    QString colorScheme = themeToColorScheme(SettingsManager::instance().theme());
    terminalWidget->setColorScheme(colorScheme);

    connect(terminalWidget, &QTermWidget::finished, this, &TerminalPanel::onSessionFinished);
    connect(terminalWidget, &QTermWidget::currentDirectoryChanged, this, &TerminalPanel::onCurrentDirectoryChanged);

    mainLayout->addWidget(terminalWidget);
}

void TerminalPanel::startTerminal() {
    if (m_isRunning) return;

    ensureTerminalWidget();

    terminalWidget->startShellProgram();
    m_isRunning = true;
    emit terminalStarted();
    terminalWidget->setFocus();
}

void TerminalPanel::stopTerminal() {
    if (!terminalWidget) return;

    mainLayout->removeWidget(terminalWidget);
    delete terminalWidget;
    terminalWidget = nullptr;
    m_isRunning = false;
    emit terminalStopped();
}

void TerminalPanel::setWorkingDirectory(const QString &path) {
    m_workingDirectory = path;
    if (terminalWidget && m_isRunning) {
        terminalWidget->setWorkingDirectory(path);
    }
}

QString TerminalPanel::workingDirectory() const {
    return m_workingDirectory;
}

void TerminalPanel::onSessionFinished() {
    m_isRunning = false;
    emit terminalStopped();
    if (isVisible() && terminalWidget) {
        QPointer<TerminalPanel> guard(this);
        QMetaObject::invokeMethod(this, [guard]() {
            if (guard && !guard->m_isRunning && guard->terminalWidget) {
                guard->terminalWidget->startShellProgram();
                guard->m_isRunning = true;
                emit guard->terminalStarted();
            }
        }, Qt::QueuedConnection);
    }
}

void TerminalPanel::onCurrentDirectoryChanged(const QString &dir) {
    m_workingDirectory = dir;
}

void TerminalPanel::applyPosition(TerminalPanelPosition pos, QTabWidget *tabWidget, QMainWindow *mainWindow)
{
    if (pos == TerminalPanelPosition::Tab)
    {
        mainWindow->removeDockWidget(this);
        int tabIndex = tabWidget->indexOf(this);
        if (tabIndex == -1)
            tabWidget->addTab(this, tr("Terminal"));
        setFeatures(QDockWidget::DockWidgetFeature::NoDockWidgetFeatures);
    }
    else
    {
        int tabIndex = tabWidget->indexOf(this);
        if (tabIndex != -1)
            tabWidget->removeTab(tabIndex);
        setFeatures(QDockWidget::DockWidgetFeature::DockWidgetMovable |
                    QDockWidget::DockWidgetFeature::DockWidgetFloatable |
                    QDockWidget::DockWidgetFeature::DockWidgetClosable);
        setTitleBarWidget(new QWidget(this));
        Qt::DockWidgetArea area = (pos == TerminalPanelPosition::Bottom) ? Qt::BottomDockWidgetArea : Qt::RightDockWidgetArea;
        mainWindow->addDockWidget(area, this);
    }
    setMinimumWidth(SettingsManager::instance().terminalPanelMinWidth());
}

void TerminalPanel::toggle(QTabWidget *tabWidget, QMainWindow *mainWindow)
{
    Q_UNUSED(mainWindow)
    TerminalPanelPosition position = SettingsManager::instance().terminalPanelPosition();

    if (position == TerminalPanelPosition::Tab)
    {
        int tabIndex = tabWidget->indexOf(this);
        if (tabIndex != -1)
        {
            if (tabWidget->currentIndex() == tabIndex)
                tabWidget->setCurrentIndex(0);
            tabWidget->removeTab(tabIndex);
            SettingsManager::instance().setShowTerminalPanel(false);
        }
        else
        {
            tabWidget->addTab(this, tr("Terminal"));
            tabWidget->setCurrentWidget(this);
            SettingsManager::instance().setShowTerminalPanel(true);
        }
    }
    else
    {
        if (isVisible())
        {
            hide();
            SettingsManager::instance().setShowTerminalPanel(false);
        }
        else
        {
            setMinimumWidth(SettingsManager::instance().terminalPanelMinWidth());
            show();
            SettingsManager::instance().setShowTerminalPanel(true);
        }
    }
}
