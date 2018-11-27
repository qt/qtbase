/********************************************************************************
** Form generated from reading UI file 'tetrixwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TETRIXWINDOW_H
#define TETRIXWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLCDNumber>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "tetrixboard.h"

QT_BEGIN_NAMESPACE

class Ui_TetrixWindow
{
public:
    QVBoxLayout *vboxLayout;
    QGridLayout *gridLayout;
    QPushButton *startButton;
    QLCDNumber *linesLcd;
    QLabel *linesRemovedLabel;
    QPushButton *pauseButton;
    QLCDNumber *scoreLcd;
    TetrixBoard *board;
    QLabel *levelLabel;
    QLabel *nextLabel;
    QLCDNumber *levelLcd;
    QLabel *scoreLabel;
    QLabel *nextPieceLabel;
    QPushButton *quitButton;

    void setupUi(QWidget *TetrixWindow)
    {
        if (TetrixWindow->objectName().isEmpty())
            TetrixWindow->setObjectName(QString::fromUtf8("TetrixWindow"));
        TetrixWindow->resize(537, 475);
        vboxLayout = new QVBoxLayout(TetrixWindow);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        gridLayout = new QGridLayout();
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        gridLayout->setContentsMargins(0, 0, 0, 0);
#endif
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        startButton = new QPushButton(TetrixWindow);
        startButton->setObjectName(QString::fromUtf8("startButton"));
        startButton->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(startButton, 4, 0, 1, 1);

        linesLcd = new QLCDNumber(TetrixWindow);
        linesLcd->setObjectName(QString::fromUtf8("linesLcd"));
        linesLcd->setSegmentStyle(QLCDNumber::Filled);

        gridLayout->addWidget(linesLcd, 3, 2, 1, 1);

        linesRemovedLabel = new QLabel(TetrixWindow);
        linesRemovedLabel->setObjectName(QString::fromUtf8("linesRemovedLabel"));
        linesRemovedLabel->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);

        gridLayout->addWidget(linesRemovedLabel, 2, 2, 1, 1);

        pauseButton = new QPushButton(TetrixWindow);
        pauseButton->setObjectName(QString::fromUtf8("pauseButton"));
        pauseButton->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(pauseButton, 5, 2, 1, 1);

        scoreLcd = new QLCDNumber(TetrixWindow);
        scoreLcd->setObjectName(QString::fromUtf8("scoreLcd"));
        scoreLcd->setSegmentStyle(QLCDNumber::Filled);

        gridLayout->addWidget(scoreLcd, 1, 2, 1, 1);

        board = new TetrixBoard(TetrixWindow);
        board->setObjectName(QString::fromUtf8("board"));
        board->setFocusPolicy(Qt::StrongFocus);
        board->setFrameShape(QFrame::Panel);
        board->setFrameShadow(QFrame::Sunken);

        gridLayout->addWidget(board, 0, 1, 6, 1);

        levelLabel = new QLabel(TetrixWindow);
        levelLabel->setObjectName(QString::fromUtf8("levelLabel"));
        levelLabel->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);

        gridLayout->addWidget(levelLabel, 2, 0, 1, 1);

        nextLabel = new QLabel(TetrixWindow);
        nextLabel->setObjectName(QString::fromUtf8("nextLabel"));
        nextLabel->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);

        gridLayout->addWidget(nextLabel, 0, 0, 1, 1);

        levelLcd = new QLCDNumber(TetrixWindow);
        levelLcd->setObjectName(QString::fromUtf8("levelLcd"));
        levelLcd->setSegmentStyle(QLCDNumber::Filled);

        gridLayout->addWidget(levelLcd, 3, 0, 1, 1);

        scoreLabel = new QLabel(TetrixWindow);
        scoreLabel->setObjectName(QString::fromUtf8("scoreLabel"));
        scoreLabel->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);

        gridLayout->addWidget(scoreLabel, 0, 2, 1, 1);

        nextPieceLabel = new QLabel(TetrixWindow);
        nextPieceLabel->setObjectName(QString::fromUtf8("nextPieceLabel"));
        nextPieceLabel->setFrameShape(QFrame::Box);
        nextPieceLabel->setFrameShadow(QFrame::Raised);
        nextPieceLabel->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(nextPieceLabel, 1, 0, 1, 1);

        quitButton = new QPushButton(TetrixWindow);
        quitButton->setObjectName(QString::fromUtf8("quitButton"));
        quitButton->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(quitButton, 4, 2, 1, 1);


        vboxLayout->addLayout(gridLayout);


        retranslateUi(TetrixWindow);

        QMetaObject::connectSlotsByName(TetrixWindow);
    } // setupUi

    void retranslateUi(QWidget *TetrixWindow)
    {
        TetrixWindow->setWindowTitle(QCoreApplication::translate("TetrixWindow", "Tetrix", nullptr));
        startButton->setText(QCoreApplication::translate("TetrixWindow", "&Start", nullptr));
        linesRemovedLabel->setText(QCoreApplication::translate("TetrixWindow", "LINES REMOVED", nullptr));
        pauseButton->setText(QCoreApplication::translate("TetrixWindow", "&Pause", nullptr));
        levelLabel->setText(QCoreApplication::translate("TetrixWindow", "LEVEL", nullptr));
        nextLabel->setText(QCoreApplication::translate("TetrixWindow", "NEXT", nullptr));
        scoreLabel->setText(QCoreApplication::translate("TetrixWindow", "SCORE", nullptr));
        nextPieceLabel->setText(QString());
        quitButton->setText(QCoreApplication::translate("TetrixWindow", "&Quit", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TetrixWindow: public Ui_TetrixWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TETRIXWINDOW_H
