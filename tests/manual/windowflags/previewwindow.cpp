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

#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "previewwindow.h"

static QString windowFlagsToString(Qt::WindowFlags flags)
{
    QString text;

    Qt::WindowFlags type = (flags & Qt::WindowType_Mask);
    if (type == Qt::Window) {
        text = "Qt::Window";
    } else if (type == Qt::Dialog) {
        text = "Qt::Dialog";
    } else if (type == Qt::Sheet) {
        text = "Qt::Sheet";
    } else if (type == Qt::Drawer) {
        text = "Qt::Drawer";
    } else if (type == Qt::Popup) {
        text = "Qt::Popup";
    } else if (type == Qt::Tool) {
        text = "Qt::Tool";
    } else if (type == Qt::ToolTip) {
        text = "Qt::ToolTip";
    } else if (type == Qt::SplashScreen) {
        text = "Qt::SplashScreen";
    }

    if (flags & Qt::MSWindowsFixedSizeDialogHint)
        text += "\n| Qt::MSWindowsFixedSizeDialogHint";
    if (flags & Qt::X11BypassWindowManagerHint)
        text += "\n| Qt::X11BypassWindowManagerHint";
    if (flags & Qt::FramelessWindowHint)
        text += "\n| Qt::FramelessWindowHint";
    if (flags & Qt::WindowTitleHint)
        text += "\n| Qt::WindowTitleHint";
    if (flags & Qt::WindowSystemMenuHint)
        text += "\n| Qt::WindowSystemMenuHint";
    if (flags & Qt::WindowMinimizeButtonHint)
        text += "\n| Qt::WindowMinimizeButtonHint";
    if (flags & Qt::WindowMaximizeButtonHint)
        text += "\n| Qt::WindowMaximizeButtonHint";
    if (flags & Qt::WindowCloseButtonHint)
        text += "\n| Qt::WindowCloseButtonHint";
    if (flags & Qt::WindowContextHelpButtonHint)
        text += "\n| Qt::WindowContextHelpButtonHint";
    if (flags & Qt::WindowShadeButtonHint)
        text += "\n| Qt::WindowShadeButtonHint";
    if (flags & Qt::WindowStaysOnTopHint)
        text += "\n| Qt::WindowStaysOnTopHint";
    if (flags & Qt::CustomizeWindowHint)
        text += "\n| Qt::CustomizeWindowHint";
    return text;
}

PreviewWindow::PreviewWindow(QWidget *parent)
    : QWidget(parent)
{
    textEdit = new QTextEdit;
    textEdit->setReadOnly(true);
    textEdit->setLineWrapMode(QTextEdit::NoWrap);

    closeButton = new QPushButton(tr("&Close"));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));

    showNormalButton = new QPushButton(tr("Show normal"));
    connect(showNormalButton, SIGNAL(clicked()), this, SLOT(showNormal()));
    showMinimizedButton = new QPushButton(tr("Show minimized"));
    connect(showMinimizedButton, SIGNAL(clicked()), this, SLOT(showMinimized()));
    showMaximizedButton = new QPushButton(tr("Show maximized"));
    connect(showMaximizedButton, SIGNAL(clicked()), this, SLOT(showMaximized()));
    showFullScreenButton = new QPushButton(tr("Show fullscreen"));
    connect(showFullScreenButton, SIGNAL(clicked()), this, SLOT(showFullScreen()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(textEdit);
    layout->addWidget(showNormalButton);
    layout->addWidget(showMinimizedButton);
    layout->addWidget(showMaximizedButton);
    layout->addWidget(showFullScreenButton);
    layout->addWidget(closeButton);
    setLayout(layout);

    setWindowTitle(tr("Preview <QWidget>"));
}

void PreviewWindow::setWindowFlags(Qt::WindowFlags flags)
{
    QWidget::setWindowFlags(flags);

    QString text = windowFlagsToString(flags);
    textEdit->setPlainText(text);
}

PreviewDialog::PreviewDialog(QWidget *parent)
    : QDialog(parent)
{
    textEdit = new QTextEdit;
    textEdit->setReadOnly(true);
    textEdit->setLineWrapMode(QTextEdit::NoWrap);

    closeButton = new QPushButton(tr("&Close"));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));

    showNormalButton = new QPushButton(tr("Show normal"));
    connect(showNormalButton, SIGNAL(clicked()), this, SLOT(showNormal()));
    showMinimizedButton = new QPushButton(tr("Show minimized"));
    connect(showMinimizedButton, SIGNAL(clicked()), this, SLOT(showMinimized()));
    showMaximizedButton = new QPushButton(tr("Show maximized"));
    connect(showMaximizedButton, SIGNAL(clicked()), this, SLOT(showMaximized()));
    showFullScreenButton = new QPushButton(tr("Show fullscreen"));
    connect(showFullScreenButton, SIGNAL(clicked()), this, SLOT(showFullScreen()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(textEdit);
    layout->addWidget(showNormalButton);
    layout->addWidget(showMinimizedButton);
    layout->addWidget(showMaximizedButton);
    layout->addWidget(showFullScreenButton);
    layout->addWidget(closeButton);
    setLayout(layout);

    setWindowTitle(tr("Preview <QDialog>"));
}

void PreviewDialog::setWindowFlags(Qt::WindowFlags flags)
{
    QWidget::setWindowFlags(flags);

    QString text = windowFlagsToString(flags);
    textEdit->setPlainText(text);
}
