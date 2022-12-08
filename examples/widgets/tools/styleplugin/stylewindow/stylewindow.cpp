// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGridLayout>
#include <QGroupBox>
#include <QTextEdit>

#include "stylewindow.h"

StyleWindow::StyleWindow()
{
    QTextEdit *styledTextEdit = new QTextEdit(tr("The quick brown fox jumps over the lazy dog"));

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(styledTextEdit);

    QGroupBox *styleBox = new QGroupBox(tr("A simple styled text edit"));
    styleBox->setLayout(layout);

    QGridLayout *outerLayout = new QGridLayout;
    outerLayout->addWidget(styleBox, 0, 0);
    setLayout(outerLayout);

    setWindowTitle(tr("Style Plugin Example"));
}
