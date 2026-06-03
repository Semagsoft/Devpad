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
#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QDate>
#include <QPixmap>

class QTimer;
class QSoundEffect;

class AboutDialog : public QDialog {
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);
    ~AboutDialog() override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void openWebsite();
    void showLicense();
    void onEffectTick();

private:
    void setupUI();
    void startGoofyEffect();
    QPixmap tintPixmap(const QPixmap &src, const QColor &tint) const;

    QLabel *licenseText;
    QScrollArea *licenseScrollArea;
    QPushButton *licenseButton;

    QLabel *iconLabel;
    QPixmap m_originalPixmap;
    QTimer *m_effectTimer;
    QSoundEffect *m_soundEffect;
    int m_effectStep;
};

#endif // ABOUTDIALOG_H
