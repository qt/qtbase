/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QApplication>
#include <QWidget>
#include <QStackedWidget>
#include <QTabBar>
#include <QLabel>
#include <QLayout>
#include <QDesktopWidget>
#include <QTabWidget>

const int TabCount = 5;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget widget;
    QStackedWidget stackedWidget;
    QTabBar tabBar;
    tabBar.setDocumentMode(true);
    tabBar.setTabsClosable(true);
    tabBar.setMovable(true);
    tabBar.setExpanding(true);

    // top
    tabBar.setShape(QTabBar::RoundedNorth);
    // bottom
//    tabBar.setShape(QTabBar::RoundedSouth);
    // left
//    tabBar.setShape(QTabBar::RoundedWest);
    // right
//    tabBar.setShape(QTabBar::RoundedEast);

    QMap<int, QWidget*> tabs;
    for (int i = 0; i < TabCount; i++) {
        QString tabNumberString = QString::number(i);
        QLabel *label = new QLabel(QStringLiteral("Tab %1 content").arg(tabNumberString));
        tabs[i] = label;
        label->setAlignment(Qt::AlignCenter);
        stackedWidget.addWidget(label);
        tabBar.addTab(QStringLiteral("Tab %1").arg(tabNumberString));
    }

    QObject::connect(&tabBar, &QTabBar::tabMoved, [&tabs](int from, int to) {
        QWidget *thisWidget = tabs[from];
        QWidget *thatWidget = tabs[to];
        tabs[from] = thatWidget;
        tabs[to] = thisWidget;
    });

    QObject::connect(&tabBar, &QTabBar::currentChanged, [&stackedWidget, &tabs](int index) {
        if (index >= 0)
            stackedWidget.setCurrentWidget(tabs[index]);
    });

    QObject::connect(&tabBar, &QTabBar::tabCloseRequested, [&stackedWidget, &tabBar, &tabs](int index) {
        QWidget *widget = tabs[index];
        tabBar.removeTab(index);
        for (int i = index + 1; i < TabCount; i++)
            tabs[i-1] = tabs[i];
        int currentIndex = tabBar.currentIndex();
        if (currentIndex >= 0)
            stackedWidget.setCurrentWidget(tabs[currentIndex]);
        delete widget;
    });

    QLayout *layout;
    switch (tabBar.shape()) {
    case QTabBar::RoundedEast:
    case QTabBar::TriangularEast:
        tabBar.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        layout = new QHBoxLayout(&widget);
        layout->addWidget(&stackedWidget);
        layout->addWidget(&tabBar);
        break;
    case QTabBar::RoundedWest:
    case QTabBar::TriangularWest:
        tabBar.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        layout = new QHBoxLayout(&widget);
        layout->addWidget(&tabBar);
        layout->addWidget(&stackedWidget);
        break;
    case QTabBar::RoundedNorth:
    case QTabBar::TriangularNorth:
        tabBar.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        layout = new QVBoxLayout(&widget);
        layout->addWidget(&tabBar);
        layout->addWidget(&stackedWidget);
        break;
    case QTabBar::RoundedSouth:
    case QTabBar::TriangularSouth:
        tabBar.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        layout = new QVBoxLayout(&widget);
        layout->addWidget(&stackedWidget);
        layout->addWidget(&tabBar);
        break;
    }

    layout->setMargin(0);
    widget.resize(QApplication::desktop()->screenGeometry(&widget).size() * 0.5);
    widget.show();

    return app.exec();
}
