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
#include "terminalbackend.h"
#include "theme.h"
#ifdef Q_OS_WIN
#include "terminalbackend_kodoterm.h"
#else
#include "terminalbackend_qtermwidget.h"
#endif
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMainWindow>
#include <QMenu>
#include <QPointer>
#include <QShortcut>
#include <QStandardPaths>

TerminalPanel::TerminalPanel(QWidget* parent) : QDockWidget(tr("Terminal"), parent), m_backend(nullptr), m_isRunning(false)
{
    setObjectName("TerminalPanel");
    setFeatures(QDockWidget::DockWidgetFeature::DockWidgetMovable | QDockWidget::DockWidgetFeature::DockWidgetFloatable |
                QDockWidget::DockWidgetFeature::DockWidgetClosable);
    setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    setTitleBarWidget(new QWidget(this));

#ifndef Q_OS_WIN
    extractColorSchemes();
#endif

    setupUI();
}

TerminalPanel::~TerminalPanel() = default;

#ifndef Q_OS_WIN
void TerminalPanel::extractColorSchemes()
{
    QString schemeDirPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/devpad/color-schemes";
    QDir colorSchemeDir(schemeDirPath);
    if (!colorSchemeDir.exists())
    {
        colorSchemeDir.mkpath(".");
    }

    QStringList schemeFiles = {
        "DevpadLight.colorscheme",
        "DevpadDark.colorscheme",
        "DevpadNord.colorscheme",
        "DevpadSolarizedLight.colorscheme",
        "DevpadMonokai.colorscheme",
        "DevpadGruvboxDark.colorscheme",
        "DevpadCatppuccinMocha.colorscheme",
        "DevpadCatppuccinMacchiato.colorscheme",
        "DevpadCatppuccinFrappe.colorscheme",
        "DevpadCatppuccinLatte.colorscheme",
        "DevpadTokyoNight.colorscheme",
        "DevpadTokyoNightStorm.colorscheme",
        "DevpadDracula.colorscheme",
        "DevpadOneDark.colorscheme",
        "DevpadAyuLight.colorscheme",
        "DevpadAyuDark.colorscheme",
    };

    for (const QString& fileName : schemeFiles)
    {
        QString resourcePath = ":/color-schemes/" + fileName;
        QString destPath = colorSchemeDir.absoluteFilePath(fileName);

        if (!QFile::exists(destPath))
        {
            if (!QFile::copy(resourcePath, destPath))
            {
                qWarning("Failed to copy color scheme: %s", qPrintable(resourcePath));
            }
        }
    }
}
#endif

void TerminalPanel::setupUI()
{
    panelWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(panelWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    setWidget(panelWidget);
}

void TerminalPanel::showEvent(QShowEvent* event)
{
    QDockWidget::showEvent(event);
    if (!m_backend || (m_backend && !m_isRunning))
    {
        startTerminal();
    }
}

QString TerminalPanel::themeToColorScheme(ThemeId themeId) const
{
    switch (themeId)
    {
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
    case ThemeId::CatppuccinMocha:
        return "DevpadCatppuccinMocha";
    case ThemeId::CatppuccinMacchiato:
        return "DevpadCatppuccinMacchiato";
    case ThemeId::CatppuccinFrappe:
        return "DevpadCatppuccinFrappe";
    case ThemeId::CatppuccinLatte:
        return "DevpadCatppuccinLatte";
    case ThemeId::TokyoNight:
        return "DevpadTokyoNight";
    case ThemeId::TokyoNightStorm:
        return "DevpadTokyoNightStorm";
    case ThemeId::Dracula:
        return "DevpadDracula";
    case ThemeId::OneDark:
        return "DevpadOneDark";
    case ThemeId::AyuLight:
        return "DevpadAyuLight";
    case ThemeId::AyuDark:
        return "DevpadAyuDark";
    case ThemeId::System:
        return SettingsManager::instance().isDarkTheme() ? "DevpadDark" : "DevpadLight";
    }
    return "DevpadDark";
}

void TerminalPanel::ensureTerminalWidget()
{
    if (m_backend)
        return;

#ifdef Q_OS_WIN
    m_backend = new TerminalBackendKodoTerm(panelWidget);
#else
    m_backend = new TerminalBackendQTermWidget(panelWidget);
#endif

    if (!m_workingDirectory.isEmpty() && QDir(m_workingDirectory).exists())
    {
        m_backend->setWorkingDirectory(m_workingDirectory);
    }

    connect(m_backend, &TerminalBackend::finished, this, &TerminalPanel::onSessionFinished);

    mainLayout->addWidget(m_backend, 1);

    auto copyAction = new QShortcut(QKeySequence::Copy, this);
    connect(copyAction, &QShortcut::activated,
            [this]()
            {
                if (m_backend && m_backend->hasFocus())
                {
                    m_backend->copyClipboard();
                }
            });

    auto pasteAction = new QShortcut(QKeySequence::Paste, this);
    connect(pasteAction, &QShortcut::activated,
            [this]()
            {
                if (m_backend && m_backend->hasFocus())
                {
                    m_backend->pasteClipboard();
                }
            });
}

void TerminalPanel::startTerminal()
{
    if (m_isRunning)
        return;

    ensureTerminalWidget();

    m_backend->start();
    m_isRunning = true;
    emit terminalStarted();
    m_backend->setFocus();

    refreshTheme();
}

void TerminalPanel::stopTerminal()
{
    if (!m_backend)
        return;

    mainLayout->removeWidget(m_backend);
    delete m_backend;
    m_backend = nullptr;
    m_isRunning = false;
    emit terminalStopped();
}

void TerminalPanel::setWorkingDirectory(const QString& path)
{
    m_workingDirectory = path;
    if (m_backend && m_isRunning)
    {
        m_backend->setWorkingDirectory(path);
#ifdef Q_OS_WIN
        m_backend->sendText(QString("cd \"%1\" && cls\n").arg(path));
#else
        m_backend->sendText(QString("cd \"%1\" && clear\n").arg(path));
#endif
    }
}

QString TerminalPanel::workingDirectory() const
{
    return m_workingDirectory;
}

void TerminalPanel::refreshTheme()
{
    if (!m_backend)
        return;

    m_backend->setTerminalFont(SettingsManager::instance().terminalFont());
    ThemeId tid = SettingsManager::instance().theme();
    QString scheme = themeToColorScheme(tid);
    m_backend->applyTheme(scheme);
}

void TerminalPanel::sendCommand(const QString& command)
{
    ensureTerminalWidget();
    if (m_isRunning)
    {
        m_backend->sendText(command);
    }
    else
    {
        startTerminal();
        QPointer<TerminalPanel> guard(this);
        QMetaObject::invokeMethod(
            this,
            [guard, command]()
            {
                if (guard && guard->m_backend)
                {
                    guard->m_backend->sendText(command);
                }
            },
            Qt::QueuedConnection);
    }
}

void TerminalPanel::onSessionFinished()
{
    m_isRunning = false;
    emit terminalStopped();
    emit sessionExited();
}

void TerminalPanel::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);

    QAction* copyAct = menu.addAction(QIcon(":/icons/Edit/copy.svg"), tr("Copy"));

    QAction* pasteAct = menu.addAction(QIcon(":/icons/Edit/paste.svg"), tr("Paste"));

    menu.addSeparator();

    QAction* clearAct = menu.addAction(QIcon(":/icons/Common/clear.svg"), tr("Clear"));

    menu.addSeparator();

    QAction* copyPathAct = menu.addAction(QIcon(":/icons/Edit/copy.svg"), tr("Copy Path"));

    QAction* showInFmAct = menu.addAction(QIcon(":/icons/Common/openinfolder.svg"), tr("Show in File Manager"));

    QAction* chosen = menu.exec(event->globalPos());
    if (!chosen)
        return;

    if (chosen == copyAct)
        m_backend->copyClipboard();
    else if (chosen == pasteAct)
        m_backend->pasteClipboard();
    else if (chosen == clearAct)
        m_backend->sendText("clear\n");
    else if (chosen == copyPathAct)
        QApplication::clipboard()->setText(m_workingDirectory);
    else if (chosen == showInFmAct)
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_workingDirectory));
}

void TerminalPanel::applyPosition(TerminalPanelPosition pos, QTabWidget* tabWidget, QMainWindow* mainWindow)
{
    if (pos == TerminalPanelPosition::Tab)
    {
        mainWindow->removeDockWidget(this);
        int tabIndex = tabWidget->indexOf(this);
        if (tabIndex == -1)
            tabWidget->addTab(this, QIcon(":/icons/View/terminal.svg"), tr("Terminal"));
        setFeatures(QDockWidget::DockWidgetFeature::NoDockWidgetFeatures);
    }
    else
    {
        int tabIndex = tabWidget->indexOf(this);
        if (tabIndex != -1)
            tabWidget->removeTab(tabIndex);
        setFeatures(QDockWidget::DockWidgetFeature::DockWidgetMovable | QDockWidget::DockWidgetFeature::DockWidgetFloatable |
                    QDockWidget::DockWidgetFeature::DockWidgetClosable);
        setTitleBarWidget(new QWidget(this));
        Qt::DockWidgetArea area = (pos == TerminalPanelPosition::Bottom) ? Qt::BottomDockWidgetArea : Qt::RightDockWidgetArea;
        mainWindow->addDockWidget(area, this);
    }
    setMinimumWidth(SettingsManager::instance().terminalPanelMinWidth());
}

void TerminalPanel::toggle(QTabWidget* tabWidget, QMainWindow* mainWindow)
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
            tabWidget->addTab(this, QIcon(":/icons/View/terminal.svg"), tr("Terminal"));
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
