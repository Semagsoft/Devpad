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
#include "aboutdialog.h"
#include <QGroupBox>
#include <QPushButton>
#include <QScrollArea>
#include <QDesktopServices>
#include <QUrl>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QTimer>
#include <QSoundEffect>
#include <QPainter>
#include <QDate>
#include <QMouseEvent>

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent),
    iconLabel(nullptr),
    m_effectTimer(nullptr),
    m_soundEffect(nullptr),
    m_effectStep(0) {
    setWindowTitle(tr("About Devpad"));
    setMinimumSize(500, 400);
    setupUI();
}

AboutDialog::~AboutDialog() {
    if (m_effectTimer) {
        m_effectTimer->stop();
    }
}

bool AboutDialog::eventFilter(QObject *obj, QEvent *event) {
    if (obj == iconLabel && event->type() == QEvent::MouseButtonPress) {
        startGoofyEffect();
        return true;
    }
    return QDialog::eventFilter(obj, event);
}

void AboutDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *titleLabel = new QLabel("<h2>Devpad</h2>", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    iconLabel = new QLabel(this);
    QPixmap pixmap(":/icons/devpad.svg");
    if (!pixmap.isNull()) {
        m_originalPixmap = pixmap;
        iconLabel->setPixmap(m_originalPixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setCursor(Qt::PointingHandCursor);
    iconLabel->installEventFilter(this);
    mainLayout->addWidget(iconLabel);

    QLabel *versionLabel = new QLabel(tr("Version 1.0 (C++/Qt6)"), this);
    versionLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(versionLabel);

    QLabel *copyrightLabel = new QLabel(tr("Copyright %1 - A C++/Qt text editor").arg(QDate::currentDate().year()), this);
    copyrightLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(copyrightLabel);

    mainLayout->addSpacing(10);

    licenseButton = new QPushButton(tr("Show License"), this);
    connect(licenseButton, &QPushButton::clicked, this, &AboutDialog::showLicense);
    mainLayout->addWidget(licenseButton, 0, Qt::AlignCenter);

    licenseText = new QLabel(this);
    licenseText->setWordWrap(true);
    licenseText->setText(
        "GNU GENERAL PUBLIC LICENSE\n"
        "Version 2, June 1991\n\n"
        "Copyright (C) 1989, 1991 Free Software Foundation, Inc.\n"
        "51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA\n\n"
        "Everyone is permitted to copy and distribute verbatim copies\n"
        "of this license document, but changing it is not allowed.\n\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
        "GNU General Public License for more details.\n\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA."
    );
    licenseText->setContentsMargins(10, 10, 10, 10);

    licenseScrollArea = new QScrollArea(this);
    licenseScrollArea->setWidget(licenseText);
    licenseScrollArea->setWidgetResizable(true);
    licenseScrollArea->setVisible(false);

    mainLayout->addWidget(licenseScrollArea);

    mainLayout->addSpacing(10);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *websiteButton = new QPushButton(tr("Visit Website"), this);
    QPushButton *closeButton = new QPushButton(tr("Close"), this);
    buttonLayout->addWidget(websiteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    connect(websiteButton, &QPushButton::clicked, this, &AboutDialog::openWebsite);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
}

void AboutDialog::startGoofyEffect() {
    if (m_effectStep > 0) return;

    m_effectStep = 1;

    if (!m_soundEffect) {
        m_soundEffect = new QSoundEffect(this);
        m_soundEffect->setSource(QUrl("qrc:/sounds/powerup.wav"));
        m_soundEffect->setVolume(0.5);
    }
    m_soundEffect->play();

    if (!m_effectTimer) {
        m_effectTimer = new QTimer(this);
        connect(m_effectTimer, &QTimer::timeout, this, &AboutDialog::onEffectTick);
    }
    m_effectTimer->start(70);
}

void AboutDialog::onEffectTick() {
    static const QList<QColor> rainbow = {
        QColor(255, 0, 0, 120),
        QColor(255, 165, 0, 120),
        QColor(255, 255, 0, 120),
        QColor(0, 255, 0, 120),
        QColor(0, 0, 255, 120),
        QColor(128, 0, 128, 120),
    };

    if (m_effectStep < 28) {
        QPixmap pm = m_originalPixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        pm = tintPixmap(pm, rainbow[m_effectStep % rainbow.size()]);
        iconLabel->setPixmap(pm);
        m_effectStep++;
    } else {
        m_effectTimer->stop();
        m_effectStep = 0;
        iconLabel->setPixmap(m_originalPixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

QPixmap AboutDialog::tintPixmap(const QPixmap &src, const QColor &tint) const {
    QPixmap result = src;
    QPainter p(&result);
    p.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    p.fillRect(result.rect(), tint);
    p.end();
    return result;
}

void AboutDialog::openWebsite() {
    QDesktopServices::openUrl(QUrl("https://semagsoft.com"));
}

void AboutDialog::showLicense() {
    licenseScrollArea->setVisible(!licenseScrollArea->isVisible());
    licenseButton->setText(licenseScrollArea->isVisible() ? tr("Hide License") : tr("Show License"));
}
