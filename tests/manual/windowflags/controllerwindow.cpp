/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QApplication>
#include <QHBoxLayout>

#include "controllerwindow.h"
#include "controls.h"

//! [0]
ControllerWindow::ControllerWindow() : previewWidget(0)
{
    parentWindow = new QMainWindow;
    parentWindow->setWindowTitle(tr("Preview parent window"));
    QLabel *label = new QLabel(tr("Parent window"));
    parentWindow->setCentralWidget(label);

    previewWindow = new PreviewWindow;
    previewWindow->installEventFilter(this);
    previewDialog = new PreviewDialog;
    previewDialog->installEventFilter(this);

    createTypeGroupBox();

    quitButton = new QPushButton(tr("&Quit"));
    connect(quitButton, SIGNAL(clicked()), qApp, SLOT(quit()));

    QHBoxLayout *bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();

    QPushButton *updateControlsButton = new QPushButton(tr("&Update"));
    connect(updateControlsButton, SIGNAL(clicked()), this, SLOT(updateStateControl()));

    bottomLayout->addWidget(updateControlsButton);
    bottomLayout->addWidget(quitButton);

    hintsControl = new HintControl;
    hintsControl->setHints(previewWindow->windowFlags());
    connect(hintsControl, SIGNAL(changed(Qt::WindowFlags)), this, SLOT(updatePreview()));

    statesControl = new WindowStatesControl(WindowStatesControl::WantVisibleCheckBox|WindowStatesControl::WantActiveCheckBox);
    statesControl->setStates(previewWindow->windowState());
    statesControl->setVisibleValue(true);
    connect(statesControl, SIGNAL(changed()), this, SLOT(updatePreview()));

    typeControl = new TypeControl;
    typeControl->setType(previewWindow->windowFlags());
    connect(typeControl, SIGNAL(changed(Qt::WindowFlags)), this, SLOT(updatePreview()));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(widgetTypeGroupBox);
    mainLayout->addWidget(additionalOptionsGroupBox);
    mainLayout->addWidget(typeControl);
    mainLayout->addWidget(hintsControl);
    mainLayout->addWidget(statesControl);
    mainLayout->addLayout(bottomLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("Window Flags (Qt version %1, %2)")
                   .arg(QLatin1String(qVersion()),
#if QT_VERSION >= 0x050000
                        qApp->platformName()));
#else
                        QLatin1String("<unknown>")));
#endif
    updatePreview();
}

bool ControllerWindow::eventFilter(QObject *, QEvent *e)
{
    if (e->type() == QEvent::WindowStateChange)
        updateStateControl();
    return false;
}

void ControllerWindow::updateStateControl()
{
    if (previewWidget)
        statesControl->setStates(previewWidget->windowState());
}

void ControllerWindow::updatePreview()
{
    const Qt::WindowFlags flags = typeControl->type() | hintsControl->hints();

    previewWindow->hide();
    previewDialog->hide();

    if (previewWidgetButton->isChecked())
        previewWidget = previewWindow;
    else
        previewWidget = previewDialog;

    if (modalWindowCheckBox->isChecked()) {
        parentWindow->show();
        previewWidget->setWindowModality(Qt::WindowModal);
        previewWidget->setParent(parentWindow);
    } else {
        previewWidget->setWindowModality(Qt::NonModal);
        previewWidget->setParent(0);
        parentWindow->hide();
    }

    if (previewWidgetButton->isChecked())
        previewWindow->setWindowFlags(flags);
    else
        previewDialog->setWindowFlags(flags);

    if (fixedSizeWindowCheckBox->isChecked()) {
        previewWidget->setFixedSize(300, 300);
    } else {
        previewWidget->setFixedSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }

    QPoint pos = previewWidget->pos();
    if (pos.x() < 0)
        pos.setX(0);
    if (pos.y() < 0)
        pos.setY(0);
    previewWidget->move(pos);

    previewWidget->setWindowState(statesControl->states());
    previewWidget->setVisible(statesControl->visibleValue());
}

void ControllerWindow::createTypeGroupBox()
{
    widgetTypeGroupBox = new QGroupBox(tr("Widget Type"));
    previewWidgetButton = createRadioButton(tr("QWidget"));
    previewWidgetButton->setChecked(true);
    previewDialogButton = createRadioButton(tr("QDialog"));
    QHBoxLayout *l = new QHBoxLayout;
    l->addWidget(previewWidgetButton);
    l->addWidget(previewDialogButton);
    widgetTypeGroupBox->setLayout(l);

    additionalOptionsGroupBox = new QGroupBox(tr("Additional options"));
    l = new QHBoxLayout;
    modalWindowCheckBox = createCheckBox(tr("Modal window"));
    fixedSizeWindowCheckBox = createCheckBox(tr("Fixed size window"));
    l->addWidget(modalWindowCheckBox);
    l->addWidget(fixedSizeWindowCheckBox);
    additionalOptionsGroupBox->setLayout(l);
}
//! [5]

//! [6]

//! [7]
QCheckBox *ControllerWindow::createCheckBox(const QString &text)
{
    QCheckBox *checkBox = new QCheckBox(text);
    connect(checkBox, SIGNAL(clicked()), this, SLOT(updatePreview()));
    return checkBox;
}
//! [7]

//! [8]
QRadioButton *ControllerWindow::createRadioButton(const QString &text)
{
    QRadioButton *button = new QRadioButton(text);
    connect(button, SIGNAL(clicked()), this, SLOT(updatePreview()));
    return button;
}
//! [8]
