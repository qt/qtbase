/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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

bool ControllerWindow::eventFilter(QObject *o, QEvent *e)
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

    previewWidget->setWindowFlags(flags);

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
