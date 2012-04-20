/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>

#include "controllerwindow.h"
#include "controls.h"

//! [0]
ControllerWindow::ControllerWindow()
{
    parentWindow = new QMainWindow;
    parentWindow->setWindowTitle(tr("Preview parent window"));
    QLabel *label = new QLabel(tr("Parent window"));
    parentWindow->setCentralWidget(label);

    previewWindow = new PreviewWindow;
    previewDialog = new PreviewDialog;

    createTypeGroupBox();

    quitButton = new QPushButton(tr("&Quit"));
    connect(quitButton, SIGNAL(clicked()), qApp, SLOT(quit()));

    QHBoxLayout *bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    bottomLayout->addWidget(quitButton);

    hintsControl = new HintControl;
    hintsControl->setHints(previewWindow->windowFlags());
    connect(hintsControl, SIGNAL(changed(Qt::WindowFlags)), this, SLOT(updatePreview()));

    statesControl = new WindowStatesControl(WindowStatesControl::WantVisibleCheckBox);
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
                   .arg(QLatin1String(qVersion()), qApp->platformName()));
    updatePreview();
}

void ControllerWindow::updatePreview()
{
    const Qt::WindowFlags flags = typeControl->type() | hintsControl->hints();

    previewWindow->hide();
    previewDialog->hide();
    QWidget *widget = 0;
    if (previewWidgetButton->isChecked())
        widget = previewWindow;
    else
        widget = previewDialog;

    if (modalWindowCheckBox->isChecked()) {
        parentWindow->show();
        widget->setWindowModality(Qt::WindowModal);
        widget->setParent(parentWindow);
    } else {
        widget->setWindowModality(Qt::NonModal);
        widget->setParent(0);
        parentWindow->hide();
    }

    widget->setWindowFlags(flags);

    if (fixedSizeWindowCheckBox->isChecked()) {
        widget->setFixedSize(300, 300);
    } else {
        widget->setFixedSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }

    QPoint pos = widget->pos();
    if (pos.x() < 0)
        pos.setX(0);
    if (pos.y() < 0)
        pos.setY(0);
    widget->move(pos);

    widget->setWindowState(statesControl->states());
    widget->setVisible(statesControl->visibleValue());
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
