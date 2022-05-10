// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "circlewidget.h"
#include "window.h"

#include <QtWidgets>

//! [0]
Window::Window()
{
    aliasedLabel = createLabel(tr("Aliased"));
    antialiasedLabel = createLabel(tr("Antialiased"));
    intLabel = createLabel(tr("Int"));
    floatLabel = createLabel(tr("Float"));

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(aliasedLabel, 0, 1);
    layout->addWidget(antialiasedLabel, 0, 2);
    layout->addWidget(intLabel, 1, 0);
    layout->addWidget(floatLabel, 2, 0);
//! [0]

//! [1]
    QTimer *timer = new QTimer(this);

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            circleWidgets[i][j] = new CircleWidget;
            circleWidgets[i][j]->setAntialiased(j != 0);
            circleWidgets[i][j]->setFloatBased(i != 0);

            connect(timer, &QTimer::timeout,
                    circleWidgets[i][j], &CircleWidget::nextAnimationFrame);

            layout->addWidget(circleWidgets[i][j], i + 1, j + 1);
        }
    }
//! [1] //! [2]
    timer->start(100);
    setLayout(layout);

    setWindowTitle(tr("Concentric Circles"));
}
//! [2]

//! [3]
QLabel *Window::createLabel(const QString &text)
{
    QLabel *label = new QLabel(text);
    label->setAlignment(Qt::AlignCenter);
    label->setMargin(2);
    label->setFrameStyle(QFrame::Box | QFrame::Sunken);
    return label;
}
//! [3]
