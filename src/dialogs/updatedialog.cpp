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
#include "updatedialog.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

UpdateDialog::UpdateDialog(Mode mode, const QString& version, const QString& releaseUrl, QWidget* parent)
    : QDialog(parent), m_mode(mode), m_version(version), m_releaseUrl(releaseUrl)
{
    setModal(true);
    setMinimumWidth(420);
    setupUi();
}

void UpdateDialog::showUpToDate(QWidget* parent, const QString& latestVersion)
{
    UpdateDialog dialog(Mode::UpToDate, latestVersion, QString(), parent);
    dialog.exec();
}

void UpdateDialog::showUpdateAvailable(QWidget* parent, const QString& latestVersion, const QString& releaseUrl)
{
    UpdateDialog dialog(Mode::UpdateAvailable, latestVersion, releaseUrl, parent);
    dialog.exec();
}

void UpdateDialog::showCheckFailed(QWidget* parent, const QString& error)
{
    UpdateDialog dialog(Mode::CheckFailed, error, QString(), parent);
    dialog.exec();
}

void UpdateDialog::setupUi()
{
    setWindowTitle(tr("Check for Updates"));

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QHBoxLayout* headerLayout = new QHBoxLayout();
    QString iconPath;
    switch (m_mode)
    {
    case Mode::UpToDate:
    case Mode::UpdateAvailable:
        iconPath = QStringLiteral(":/icons/Help/update.svg");
        break;
    case Mode::CheckFailed:
        iconPath = QStringLiteral(":/icons/View/error.svg");
        break;
    }

    QLabel* iconLabel = new QLabel(this);
    QPixmap pixmap(iconPath);
    if (!pixmap.isNull())
        iconLabel->setPixmap(pixmap.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    headerLayout->addWidget(iconLabel, 0, Qt::AlignTop);

    QVBoxLayout* textLayout = new QVBoxLayout();
    QLabel* titleLabel = new QLabel(this);
    titleLabel->setWordWrap(true);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleLabel->setFont(titleFont);

    QLabel* messageLabel = new QLabel(this);
    messageLabel->setWordWrap(true);
    messageLabel->setTextFormat(Qt::PlainText);

    switch (m_mode)
    {
    case Mode::UpToDate:
        titleLabel->setText(tr("You're up to date"));
        messageLabel->setText(tr("Devpad %1 is the latest version.").arg(m_version));
        break;
    case Mode::UpdateAvailable:
        titleLabel->setText(tr("Update Available"));
        messageLabel->setText(tr("A new version of Devpad is available (%1).\n\n"
                                 "You are currently running version %2.")
                                  .arg(m_version, QCoreApplication::applicationVersion()));
        break;
    case Mode::CheckFailed:
        titleLabel->setText(tr("Check for Updates Failed"));
        messageLabel->setText(m_version);
        break;
    }

    textLayout->addWidget(titleLabel);
    textLayout->addSpacing(8);
    textLayout->addWidget(messageLabel);
    textLayout->addStretch();
    headerLayout->addLayout(textLayout, 1);
    mainLayout->addLayout(headerLayout);

    mainLayout->addSpacing(12);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    if (m_mode == Mode::UpdateAvailable)
    {
        QPushButton* downloadButton = new QPushButton(tr("Download"), this);
        connect(downloadButton, &QPushButton::clicked, this, &UpdateDialog::downloadUpdate);
        buttonLayout->addWidget(downloadButton);
    }

    QPushButton* closeButton = new QPushButton(tr("Close"), this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);
}

void UpdateDialog::downloadUpdate()
{
    if (!m_releaseUrl.isEmpty())
        QDesktopServices::openUrl(QUrl(m_releaseUrl));
    accept();
}
