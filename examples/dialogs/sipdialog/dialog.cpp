/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>

#include "dialog.h"

//! [Dialog constructor part1]
Dialog::Dialog()
{
    desktopGeometry = QApplication::desktop()->availableGeometry(0);

    setWindowTitle(tr("SIP Dialog Example"));
    QScrollArea *scrollArea = new QScrollArea(this);
    QGroupBox *groupBox = new QGroupBox(scrollArea);
    groupBox->setTitle(tr("SIP Dialog Example"));
    QGridLayout *gridLayout = new QGridLayout(groupBox);
    groupBox->setLayout(gridLayout);
//! [Dialog constructor part1]

//! [Dialog constructor part2]
    QLineEdit* lineEdit = new QLineEdit(groupBox);
    lineEdit->setText(tr("Open and close the SIP"));
    lineEdit->setMinimumWidth(220);

    QLabel* label = new QLabel(groupBox);
    label->setText(tr("This dialog resizes if the SIP is opened"));
    label->setMinimumWidth(220);

    QPushButton* button = new QPushButton(groupBox);
    button->setText(tr("Close Dialog"));
    button->setMinimumWidth(220);
//! [Dialog constructor part2]

//! [Dialog constructor part3]
    if (desktopGeometry.height() < 400)
        gridLayout->setVerticalSpacing(80);
    else
        gridLayout->setVerticalSpacing(150);

    gridLayout->addWidget(label);
    gridLayout->addWidget(lineEdit);
    gridLayout->addWidget(button);
//! [Dialog constructor part3]

//! [Dialog constructor part4]
    scrollArea->setWidget(groupBox);
    QHBoxLayout* layout = new QHBoxLayout();
    layout->addWidget(scrollArea);
    setLayout(layout);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//! [Dialog constructor part4]

//! [Dialog constructor part5]
    connect(button, SIGNAL(clicked()),
        qApp, SLOT(closeAllWindows()));
    connect(QApplication::desktop(), SIGNAL(workAreaResized(int)), 
        this, SLOT(desktopResized(int)));
}
//! [Dialog constructor part5]

//! [desktopResized() function]
void Dialog::desktopResized(int screen)
{
    if (screen != 0)
        return;
    reactToSIP();
}
//! [desktopResized() function]

//! [reactToSIP() function]
void Dialog::reactToSIP()
{
    QRect availableGeometry = QApplication::desktop()->availableGeometry(0);

    if (desktopGeometry != availableGeometry) {
        if (windowState() | Qt::WindowMaximized)
            setWindowState(windowState() & ~Qt::WindowMaximized);

        setGeometry(availableGeometry);
    }

    desktopGeometry = availableGeometry;
}
//! [reactToSIP() function]
