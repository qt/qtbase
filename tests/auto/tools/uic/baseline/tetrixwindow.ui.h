/********************************************************************************
** Form generated from reading UI file 'tetrixwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
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
            TetrixWindow->setObjectName(QStringLiteral("TetrixWindow"));
        TetrixWindow->resize(537, 475);
        vboxLayout = new QVBoxLayout(TetrixWindow);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        gridLayout = new QGridLayout();
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        gridLayout->setContentsMargins(0, 0, 0, 0);
#endif
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        startButton = new QPushButton(TetrixWindow);
        startButton->setObjectName(QStringLiteral("startButton"));
        startButton->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(startButton, 4, 0, 1, 1);

        linesLcd = new QLCDNumber(TetrixWindow);
        linesLcd->setObjectName(QStringLiteral("linesLcd"));
        linesLcd->setSegmentStyle(QLCDNumber::Filled);

        gridLayout->addWidget(linesLcd, 3, 2, 1, 1);

        linesRemovedLabel = new QLabel(TetrixWindow);
        linesRemovedLabel->setObjectName(QStringLiteral("linesRemovedLabel"));
        linesRemovedLabel->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);

        gridLayout->addWidget(linesRemovedLabel, 2, 2, 1, 1);

        pauseButton = new QPushButton(TetrixWindow);
        pauseButton->setObjectName(QStringLiteral("pauseButton"));
        pauseButton->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(pauseButton, 5, 2, 1, 1);

        scoreLcd = new QLCDNumber(TetrixWindow);
        scoreLcd->setObjectName(QStringLiteral("scoreLcd"));
        scoreLcd->setSegmentStyle(QLCDNumber::Filled);

        gridLayout->addWidget(scoreLcd, 1, 2, 1, 1);

        board = new TetrixBoard(TetrixWindow);
        board->setObjectName(QStringLiteral("board"));
        board->setFocusPolicy(Qt::StrongFocus);
        board->setFrameShape(QFrame::Panel);
        board->setFrameShadow(QFrame::Sunken);

        gridLayout->addWidget(board, 0, 1, 6, 1);

        levelLabel = new QLabel(TetrixWindow);
        levelLabel->setObjectName(QStringLiteral("levelLabel"));
        levelLabel->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);

        gridLayout->addWidget(levelLabel, 2, 0, 1, 1);

        nextLabel = new QLabel(TetrixWindow);
        nextLabel->setObjectName(QStringLiteral("nextLabel"));
        nextLabel->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);

        gridLayout->addWidget(nextLabel, 0, 0, 1, 1);

        levelLcd = new QLCDNumber(TetrixWindow);
        levelLcd->setObjectName(QStringLiteral("levelLcd"));
        levelLcd->setSegmentStyle(QLCDNumber::Filled);

        gridLayout->addWidget(levelLcd, 3, 0, 1, 1);

        scoreLabel = new QLabel(TetrixWindow);
        scoreLabel->setObjectName(QStringLiteral("scoreLabel"));
        scoreLabel->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);

        gridLayout->addWidget(scoreLabel, 0, 2, 1, 1);

        nextPieceLabel = new QLabel(TetrixWindow);
        nextPieceLabel->setObjectName(QStringLiteral("nextPieceLabel"));
        nextPieceLabel->setFrameShape(QFrame::Box);
        nextPieceLabel->setFrameShadow(QFrame::Raised);
        nextPieceLabel->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(nextPieceLabel, 1, 0, 1, 1);

        quitButton = new QPushButton(TetrixWindow);
        quitButton->setObjectName(QStringLiteral("quitButton"));
        quitButton->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(quitButton, 4, 2, 1, 1);


        vboxLayout->addLayout(gridLayout);


        retranslateUi(TetrixWindow);

        QMetaObject::connectSlotsByName(TetrixWindow);
    } // setupUi

    void retranslateUi(QWidget *TetrixWindow)
    {
        TetrixWindow->setWindowTitle(QApplication::translate("TetrixWindow", "Tetrix", nullptr));
        startButton->setText(QApplication::translate("TetrixWindow", "&Start", nullptr));
        linesRemovedLabel->setText(QApplication::translate("TetrixWindow", "LINES REMOVED", nullptr));
        pauseButton->setText(QApplication::translate("TetrixWindow", "&Pause", nullptr));
        levelLabel->setText(QApplication::translate("TetrixWindow", "LEVEL", nullptr));
        nextLabel->setText(QApplication::translate("TetrixWindow", "NEXT", nullptr));
        scoreLabel->setText(QApplication::translate("TetrixWindow", "SCORE", nullptr));
        nextPieceLabel->setText(QString());
        quitButton->setText(QApplication::translate("TetrixWindow", "&Quit", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TetrixWindow: public Ui_TetrixWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TETRIXWINDOW_H
