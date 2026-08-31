/********************************************************************************
** Form generated from reading UI file 'SearchBar.ui'
**
** Created by: Qt User Interface Compiler version 6.11.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SEARCHBAR_H
#define UI_SEARCHBAR_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SearchBar
{
public:
    QHBoxLayout *horizontalLayout;
    QToolButton *closeButton;
    QLabel *findLabel;
    QLineEdit *searchTextEdit;
    QToolButton *findPreviousButton;
    QToolButton *findNextButton;
    QToolButton *optionsButton;

    void setupUi(QWidget *SearchBar)
    {
        if (SearchBar->objectName().isEmpty())
            SearchBar->setObjectName("SearchBar");
        SearchBar->resize(399, 40);
        horizontalLayout = new QHBoxLayout(SearchBar);
        horizontalLayout->setObjectName("horizontalLayout");
        closeButton = new QToolButton(SearchBar);
        closeButton->setObjectName("closeButton");
        QIcon icon;
        QString iconThemeName = QString::fromUtf8("dialog-close");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon = QIcon::fromTheme(iconThemeName);
        } else {
            icon.addFile(QString::fromUtf8(""), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        }
        closeButton->setIcon(icon);

        horizontalLayout->addWidget(closeButton);

        findLabel = new QLabel(SearchBar);
        findLabel->setObjectName("findLabel");

        horizontalLayout->addWidget(findLabel);

        searchTextEdit = new QLineEdit(SearchBar);
        searchTextEdit->setObjectName("searchTextEdit");

        horizontalLayout->addWidget(searchTextEdit);

        findPreviousButton = new QToolButton(SearchBar);
        findPreviousButton->setObjectName("findPreviousButton");
        QIcon icon1;
        iconThemeName = QString::fromUtf8("go-previous");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon1 = QIcon::fromTheme(iconThemeName);
        } else {
            icon1.addFile(QString::fromUtf8(""), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        }
        findPreviousButton->setIcon(icon1);

        horizontalLayout->addWidget(findPreviousButton);

        findNextButton = new QToolButton(SearchBar);
        findNextButton->setObjectName("findNextButton");
        QIcon icon2;
        iconThemeName = QString::fromUtf8("go-next");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon2 = QIcon::fromTheme(iconThemeName);
        } else {
            icon2.addFile(QString::fromUtf8(""), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        }
        findNextButton->setIcon(icon2);

        horizontalLayout->addWidget(findNextButton);

        optionsButton = new QToolButton(SearchBar);
        optionsButton->setObjectName("optionsButton");
        QIcon icon3(QIcon::fromTheme(QString::fromUtf8("preferences-system")));
        optionsButton->setIcon(icon3);
        optionsButton->setPopupMode(QToolButton::InstantPopup);

        horizontalLayout->addWidget(optionsButton);


        retranslateUi(SearchBar);

        QMetaObject::connectSlotsByName(SearchBar);
    } // setupUi

    void retranslateUi(QWidget *SearchBar)
    {
        SearchBar->setWindowTitle(QCoreApplication::translate("SearchBar", "SearchBar", nullptr));
        closeButton->setText(QCoreApplication::translate("SearchBar", "X", nullptr));
        findLabel->setText(QCoreApplication::translate("SearchBar", "Find:", nullptr));
        findPreviousButton->setText(QCoreApplication::translate("SearchBar", "<", nullptr));
        findNextButton->setText(QCoreApplication::translate("SearchBar", ">", nullptr));
        optionsButton->setText(QCoreApplication::translate("SearchBar", "...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SearchBar: public Ui_SearchBar {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SEARCHBAR_H
