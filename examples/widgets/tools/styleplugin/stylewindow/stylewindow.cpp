// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>

#include "stylewindow.h"

StyleWindow::StyleWindow()
{
    QPushButton *styledButton = new QPushButton(tr("Big Red Button"));

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(styledButton);

    QGroupBox *styleBox = new QGroupBox(tr("A simple style button"));
    styleBox->setLayout(layout);

    QGridLayout *outerLayout = new QGridLayout;
    outerLayout->addWidget(styleBox, 0, 0);
    setLayout(outerLayout);

    setWindowTitle(tr("Style Plugin Example"));
}
